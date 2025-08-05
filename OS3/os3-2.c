#include <stdio.h> 
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define PTE_SIZE (4)
#define PAGE_INVALID (0)
#define PAGE_VALID (1)
#define MAX_REPERENCES (256)

typedef struct {
    unsigned char frame; //frame number
    unsigned char vflag;  //valid flag
    unsigned char ref;  //reference counter
    unsigned char pad;  //padding
} pte; 

typedef struct {
    int pid; 
    int ref_len; 
    unsigned char *references; //참조할 페이지 인덱스 목록
} process_raw; 

typedef struct {
    process_raw raw; 
    int next_ref; //다음 참고 index
    int page_fault;  //page_fault count
    int L1_Frame; //L1이 저장되어있는 프레임 번호
} pcb; 


static int PAGESIZE; 
static int VAS_PAGES; 
static int PAS_FRAMES; 
static int PAS_SIZE; 
static int VAS_SIZE; 
static int PAGETABLE_FRAMES; 

static int L2_Frame; //L2 페이지 테이블 프레임의 갯수수
static unsigned char *pas = NULL;  //physical memory space 
static pcb process[MAX_REPERENCES]; 
static int proccess_cnt = 0;
static int nxt_free_frame = 0;
static int PT_entry; 

void load_setting() {
    if(fread(&PAGESIZE, sizeof(int), 1, stdin)!=1) { return; }
    if(fread(&PAS_FRAMES, sizeof(int), 1, stdin)!=1 ) { return; }
    if(fread(&VAS_PAGES, sizeof(int), 1, stdin)!=1) { return; }

    PAS_SIZE = PAGESIZE*PAS_FRAMES; 
    VAS_SIZE = PAGESIZE*VAS_PAGES; 
    PAGETABLE_FRAMES = VAS_PAGES*PTE_SIZE/PAGESIZE; 
    PT_entry = PAGESIZE/PTE_SIZE;  //한 페이지에 가능한 pte갯수
    L2_Frame = VAS_PAGES/PT_entry; 

    pas = malloc(PAS_SIZE); 
    memset(pas, 0, PAS_SIZE); 
}

int set_frame() {
    if(nxt_free_frame>=PAS_FRAMES) return -1; 
    return nxt_free_frame++; 
}

static pte *frame_move(int i) {
    return (pte *)(pas + i*PAGESIZE); 
}

int cur_process = 0; 
int load_process() {
    while(proccess_cnt<MAX_REPERENCES){
        int pid, ref_len; 
        if(fread(&pid, sizeof(int), 1, stdin)!=1) break; 
        if(fread(&ref_len, sizeof(int), 1, stdin)!=1) break; 
        
        pcb *p = &process[cur_process];
        cur_process++; 
        proccess_cnt++;  
        p->raw.pid = pid; 
        p->raw.ref_len = ref_len; 
        p->raw.references = malloc(ref_len); 
        if(fread(p->raw.references, 1, ref_len, stdin)!=ref_len) {
            return -1; 
        }

        p->L1_Frame = set_frame(); 
        if(p->L1_Frame == -1) {return -1; }
        memset(frame_move(p->L1_Frame), 0, PT_entry*sizeof(pte)); 

        p->next_ref = 0; 
        p->page_fault = 0; 
    }
    return 1; 
}

int simulation() {
    int check = proccess_cnt; 

    while(check > 0 ){
        check = 0; 
        for(int i = 0; i< proccess_cnt; ++i) {
            pcb *p = &process[i]; 
            if(p->next_ref >= p->raw.ref_len) continue; 
            check ++ ; 

            unsigned char page_check = p->raw.references[p->next_ref]; 
            pte *L1 = frame_move(p->L1_Frame); 
            unsigned char L1_index = page_check/PT_entry; 
            unsigned char L2_index = page_check%PT_entry; 
            
            if(L1[L1_index].vflag == PAGE_INVALID) {
                int new_frame = set_frame(); 
                if(new_frame == -1) return -1; 
                
                L1[L1_index].frame = new_frame; 
                L1[L1_index].vflag = PAGE_VALID; 
                L1[L1_index].ref = 1; 
                memset(frame_move(new_frame), 0, PT_entry*sizeof(pte)); 
                p->page_fault ++; 
            }

            pte *L2 = frame_move(L1[L1_index].frame); 

            if(L2[L2_index].vflag == PAGE_INVALID) {
                int frame = set_frame(); 
                if(frame == -1) {
                    return -1; 
                }
                L2[L2_index].frame = frame; 
                L2[L2_index].vflag = PAGE_VALID; 
                L2[L2_index].ref = 1; 
                p->page_fault++; 
            }else {
                L2[L2_index].ref++; 
            }
            p->next_ref++; 
        }
    }
    return 1; 
}

void print_result() {
    int total_pageFault = 0; 
    int total_ref = 0; 
    int total_frame = nxt_free_frame; 

    for(int i = 0; i<proccess_cnt; ++i) {
        pcb *p = &process[i];
        
        fprintf(stdout, "** Process %03d: Allocated Frames=%03d PageFaults/References=%03d/%03d\n",
             p->raw.pid, 1+p->page_fault, p->page_fault, p->next_ref); 
        
        pte *L1 = frame_move(p->L1_Frame); 
        for(int j =0; j<PT_entry; ++j) {
            if(L1[j].vflag == PAGE_INVALID) continue; 
            fprintf(stdout, "(L1PT) %03d -> %03d\n", j, L1[j].frame); 

            pte *L2 = frame_move(L1[j].frame); 
            for(int k = 0; k<PT_entry; ++k) {
                if(L2[k].vflag == PAGE_VALID) {
                    fprintf(stdout, "(L2PT) %03d -> %03d REF=%03d\n", 
                        j*PT_entry+k, L2[k].frame, L2[k].ref); 
                }
            }
        }

        total_pageFault += p-> page_fault; 
        total_ref += p->next_ref; 
    }
    fprintf(stdout, "Total: Allocated Frames=%03d Page Faults/References=%03d/%03d\n",
        total_frame, total_pageFault, total_ref
    ); 
}


void mem_clear() {
    for(int i = 0; i< proccess_cnt; ++i) {
        free(process[i].raw.references); 
    }
    free(pas); 
}

int main(int argc, char *argv[]) {
    //입력 값 받기
    load_setting(); 
    if(load_process()==-1||simulation()==-1) fprintf(stdout, "Out of memory!!\n"); 
    //출력
    print_result(); 

    //memory 정리
    mem_clear(); 

    return 0;
}
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
    int page_fault; //page_fault count
    int first_frame; //시작 프레임 번호호
} pcb; 


static int PAGESIZE; 
static int VAS_PAGES; 
static int PAS_FRAMES; 
static int PAS_SIZE; 
static int VAS_SIZE; 
static int PAGETABLE_FRAMES; 

static unsigned char *pas = NULL;  //physical memory space 
static pcb process[MAX_REPERENCES]; 
static int proccess_cnt = 0;
static int nxt_free_frame = 0;
int cur_process = 0;

void load_setting() { //pagesize, pas_frames, vas_pages input
    if(fread(&PAGESIZE, sizeof(int), 1, stdin)!=1) { return; }
    if(fread(&PAS_FRAMES, sizeof(int), 1, stdin)!=1 ) { return; }
    if(fread(&VAS_PAGES, sizeof(int), 1, stdin)!=1) { return; }

    PAS_SIZE = PAGESIZE*PAS_FRAMES; 
    VAS_SIZE = PAGESIZE*VAS_PAGES; 
    PAGETABLE_FRAMES = VAS_PAGES*PTE_SIZE/PAGESIZE; 

    pas = malloc(PAS_SIZE); 
    memset(pas, 0, PAS_SIZE); 
}

int set_frame() { //아직 할당되지 않은 프레임번호를 전달
    if(nxt_free_frame>=PAS_FRAMES) return -1;  //프레임이 모두 사용되었을 경우 -> -1반환환
    return nxt_free_frame++; 
}

static pte *frame_move(int i) { //i번째 프레임을 pte* 형태로
    return (pte *)(pas + i*PAGESIZE); 
}
 
int load_process() { //프로세스 정보를 받아오는 함수
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

        //시작프레임을 set_frame()으로 할당하기 
        int base = set_frame(); 
        if(base == -1) {return -1; }
        for(int i = 1; i<PAGETABLE_FRAMES; ++i) { //나머지 PAGETABLE_FRAMES만큼 할당
            if(set_frame()==-1) {return -1; }
        }
        p->first_frame = base; 
        memset(frame_move(base), 0, VAS_PAGES*sizeof(pte)); 

        p->next_ref = 0; 
        p->page_fault = 0; 
    }
    return 1; 
}

int simulation() { //프로세스마다 순서에 따라서 가상 페이지를 참조하도록 하는 함수
    int check = proccess_cnt; 

    while(check > 0 ){
        check = 0; 
        for(int i = 0; i< proccess_cnt; ++i) {
            pcb *p = &process[i]; 
            if(p->next_ref >= p->raw.ref_len) continue; 
            check ++ ; 

            unsigned char page_check = p->raw.references[p->next_ref]; 
            pte *table = frame_move(p->first_frame); 
            
            //페이지테이블에서 유효하지 않을 경우
            //새 프레임 할당 & PAGE_FAULT 증가가
            if(table[page_check].vflag == PAGE_INVALID) { 
                int new_frame = set_frame();  
                if(new_frame == -1) return -1; 
                
                table[page_check].frame = new_frame; 
                table[page_check].vflag = PAGE_VALID; 
                table[page_check].ref = 1; 
                p->page_fault ++; 
            }else {
                table[page_check].ref++; 
            }
            p->next_ref++; 
        }
    }
    return 1; 
}

void print_result() { //프로세스마다 출력 및 총 합계 출력
    int total_pageFault = 0; 
    int total_ref = 0; 
    int total_frame = nxt_free_frame; 

    for(int i = 0; i<proccess_cnt; ++i) {
        pcb *p = &process[i];
        fprintf(stdout, "** Process %03d: Allocated Frames=%03d PageFaults/References=%03d/%03d\n",
             p->raw.pid, p->page_fault+PAGETABLE_FRAMES, p->page_fault, p->next_ref); 
        
        pte *table = frame_move(p->first_frame); 
        for(int j =0; j<VAS_PAGES; ++j) {
            if(table[j].vflag == PAGE_VALID) {
                fprintf(stdout, "%03d -> %03d REF=%03d\n", j, table[j].frame, table[j].ref); 
            }
        }

        total_pageFault += p-> page_fault; 
        total_ref += p->next_ref; 
    }
    fprintf(stdout, "Total: Allocated Frames=%03d Page Faults/References=%03d/%03d\n",
        total_frame, total_pageFault, total_ref
    ); 
}

void mem_clear() { //메모리 해제 
    for(int i = 0; i< proccess_cnt; ++i) {
        free(process[i].raw.references); 
    }
    free(pas); 
}

int main(int argc, char *argv[]) {
    //입력 값 받기
    load_setting(); 
    
    //메모리 부족을 확인하고 -1이면 OUT OF MEMORY가 출력되도록 작성 
    if(load_process()==-1||simulation()==-1) fprintf(stdout, "Out of memory!!\n"); 
    //출력
    print_result(); 

    //memory 정리
    mem_clear(); 

    return 0;
}
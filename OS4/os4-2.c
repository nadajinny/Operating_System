#include <stdio.h> 
#include <stdlib.h> 
#include <stdint.h> 
#include <string.h>

#define MAX_FILES 16
#define MAX_BLOCKS 208
#define BLOCK_SIZE 16
#define MAX_FILENAME_LEN 24
#define MAX_FILE_BLOCKS 3

typedef struct {
    int fsSize;
    int inode_cnt; 
    int block_cnt; 
    int block_size; 
    int free_inode; 
    int free_block; 

    unsigned char inode_bitmap[MAX_FILES]; 
    unsigned char block_bitmap[MAX_BLOCKS]; 
    unsigned char padding[8]; 
} superblock; 

typedef struct {
    unsigned int size; 
    unsigned char indirect_block; 
    unsigned char block[MAX_FILE_BLOCKS];
    char file_name[MAX_FILENAME_LEN]; 
} inode; 

typedef struct {
    superblock SB; 
    inode inodes[MAX_FILES]; 
    unsigned char data_block[MAX_BLOCKS][BLOCK_SIZE]; 
} filesystem; 

static filesystem fs; 

//--------과제 2번 해당 함수-------------------------------

//파일 이름 기반 inode 인덱스 탐색
int find_inode(const char *name) {
    for(int i = 0; i<MAX_FILES; ++i) {
        if(strcmp(fs.inodes[i].file_name, name)==0) return i; 
    }
    return -1; //해당 파일 없음
}

//direct block 데이터 출력
void direct_block(const inode *node) {
    for (int i = 0; i < MAX_FILE_BLOCKS; ++i) {
        if(node->block[i]!=0xFF) { //invalid block check 
            fprintf(stdout, "Block [%02d] Direct: %.*s\n", node->block[i], BLOCK_SIZE, fs.data_block[node->block[i]]); 
        }
    }
}

//indirect block 테이블 + 블록 내의 데이터 출력
void indirect_block(const inode *node) {
    if(node->indirect_block == 0xFF) return ; //indirect block 없음음

    unsigned char *indirect = fs.data_block[node->indirect_block]; 
    fprintf(stdout, "Block [%02d] Indirect Table: ", node->indirect_block); 
    
    for(int i = 0;i<BLOCK_SIZE; ++i) { 

        if(indirect[i]!=0xFF) {
            if(i!=0) fprintf(stdout, ", "); 
            fprintf(stdout, "%u", indirect[i]); 
        }
    }
    fprintf(stdout, "\n"); 

    //indirect block 번호에 해당하는 데이터 출력 
    for(int i = 0; i<BLOCK_SIZE; ++i) {
        if(indirect[i]==0xFF) break; 
        fprintf(stdout, "Block [%02d] Indirect: %.*s\n", indirect[i], BLOCK_SIZE, fs.data_block[indirect[i]]); 
    }
}

//direct + indirect 블록으로 전체 파일내용 출력 
void file_content(const inode *node) {
    char content[4096] = {0}; 
    int offset = 0;

    //direct 내용
    for(int i = 0;i<MAX_FILE_BLOCKS; ++i) {
        if(node->block[i]!=0xFF) {
            memcpy(content + offset, fs.data_block[node->block[i]], BLOCK_SIZE); 
            offset += BLOCK_SIZE; 
        }
    }
    //indirect 내용
    if(node->indirect_block!=0xFF){
        unsigned char *indirect = fs.data_block[node->indirect_block]; 
        for(int i = 0; i<BLOCK_SIZE; ++i) {
            if(indirect[i]==0xFF) continue; 
            memcpy(content+offset, fs.data_block[indirect[i]], BLOCK_SIZE); 
            offset+=BLOCK_SIZE; 
        }
    }

    content[node->size] = '\0';  //파일 크기까지만 자름
    fprintf(stdout, "File Contents: %s\n\n", content); 

}

//파일 내용 결과 모두 출력 
void print_result(const inode *node, int idx) {
    fprintf(stdout, "Inode [%02d]: %s (file size : %u B)\n", idx, node->file_name, node->size);

    direct_block(node); 
    indirect_block(node); 
    file_content(node); 
}

//stdin에서 read 명령어 파싱 후 출력 
void read_commands(FILE *input) {
    unsigned char cmd;

    while(fread(&cmd, 1, 1, input)==1) {
        if (cmd!=0x01) break; 
        
        int name_length = 0;
        if (fread(&name_length, sizeof(int), 1, input)!=1) break; 

        char file_name[256] = {0}; 
        if(fread(file_name, 1, name_length, input)!=name_length) break; 

        fprintf(stdout, "CMD : READ %s \n", file_name); 

        int idx = find_inode(file_name); 
        if(idx == -1) continue; 

        print_result(&fs.inodes[idx],idx); 
    }
}
//---------------------------------------

//---------과제 1번 코드와 동일-------------
void superblock_info() {
    fprintf(stdout, "Filesystem Status: \n"); 
    fprintf(stdout, "Superblock Information: \n"); 
    fprintf(stdout, "\tFilesystem Size: %d bytes \n", fs.SB.fsSize); 
    fprintf(stdout, "\tBlock Size: %d bytes\n", fs.SB.block_size); 
    fprintf(stdout, "\tAvailable Inodes: %d/%d\n", fs.SB.free_inode, fs.SB.inode_cnt); 
    fprintf(stdout, "\tAvailable Blocks: %d/%d\n", fs.SB.free_block, fs.SB.block_cnt); 
}

void indirect_block_info(const inode *node) {
    if(node->indirect_block != 0xFF) {
        fprintf(stdout, "\t\tIndirect block: %u\n", node->indirect_block); 
        unsigned char *indirect = fs.data_block[node->indirect_block]; 
        fprintf(stdout, "\t\tIndirect data blocks: "); 
        for(int j = 0;j<BLOCK_SIZE; ++j) {
            if(indirect[j]!=0xFF) { 
                if(j!=0) fprintf(stdout, ", ");
                fprintf(stdout, "%u", indirect[j]); 
            }
        }
        fprintf(stdout, "\n"); 
    }
}

void direct_block_info(const inode *node) {
    fprintf(stdout, "\t\tDirect blocks: "); 
    for(int j = 0;j<MAX_FILE_BLOCKS; ++j) {
        if(node->block[j] != 0xFF) {
            if(j!=0) fprintf(stdout, ", "); 
            fprintf(stdout, "%u", node->block[j]); 
        }
    }
    fprintf(stdout, "\n"); 
}

void inode_detail_info(int idx, const inode *node){
    fprintf(stdout, "\tFile: %s\n", node->file_name); 
    fprintf(stdout, "\tSize: %d\n", node->size); 
    fprintf(stdout, "\t\tInode: %d\n", idx); 
    
    direct_block_info(node); 
    indirect_block_info(node); 
}
//----------------------------------------------------

int main() {
    if(fread(&fs, sizeof(filesystem),1,stdin)!=1) {
        fprintf(stderr, "Read error\n"); 
        return 1; 
    }   

    read_commands(stdin); 
    
    superblock_info(); 
    fprintf(stdout, "Detailed File Information:\n"); 
    for(int i = 0; i<MAX_FILES; ++i) {
        if(fs.SB.inode_bitmap[i]) {
            inode_detail_info(i,&fs.inodes[i]); 
        }
    }

    return 0;
}
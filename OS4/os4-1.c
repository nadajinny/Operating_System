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

    unsigned char inode_bitmap[MAX_FILES];  //inode 사용여부 표시
    unsigned char block_bitmap[MAX_BLOCKS];  //데이터 사용여부 표시
    unsigned char padding[8];  //정렬용
} superblock; 

typedef struct {
    unsigned int size; 
    unsigned char indirect_block; 
    unsigned char block[MAX_FILE_BLOCKS];
    char file_name[MAX_FILENAME_LEN]; 
} inode; 

//파일 시스템 구조
typedef struct {
    superblock SB; //슈퍼블록
    inode inodes[MAX_FILES];  //inode 블록
    unsigned char data_block[MAX_BLOCKS][BLOCK_SIZE]; //data 블록  
} filesystem; 

static filesystem fs; 

//수퍼블록 정보
void superblock_info() {
    fprintf(stdout, "Filesystem Status: \n"); 
    fprintf(stdout, "Superblock Information: \n"); 
    fprintf(stdout, "\tFilesystem Size: %d bytes \n", fs.SB.fsSize); 
    fprintf(stdout, "\tBlock Size: %d bytes\n", fs.SB.block_size); 
    fprintf(stdout, "\tAvailable Inodes: %d/%d\n", fs.SB.free_inode, fs.SB.inode_cnt); 
    fprintf(stdout, "\tAvailable Blocks: %d/%d\n", fs.SB.free_block, fs.SB.block_cnt); 
}

//indirect block 정보 출력 
void indirect_block_info(const inode *node) {
    if(node->indirect_block != 0xFF) { //indirect block이 있는 경우
        fprintf(stdout, "\t\tIndirect block: %u\n", node->indirect_block); 
        unsigned char *indirect = fs.data_block[node->indirect_block]; 
        fprintf(stdout, "\t\tIndirect data blocks: "); 
        for(int j = 0;j<BLOCK_SIZE; ++j) {
            if(indirect[j]!=0xFF) {  //유효한 indirect block인지지 확인
                if(j!=0) fprintf(stdout, ", ");
                fprintf(stdout, "%u", indirect[j]); 
            }
        }
        fprintf(stdout, "\n"); 
    }
}

//direct block 출력 
void direct_block_info(const inode *node) {
    fprintf(stdout, "\t\tDirect blocks: "); 
    for(int j = 0;j<MAX_FILE_BLOCKS; ++j) {
        if(node->block[j] != 0xFF) { //유효한 direct block인지 확인
            if(j!=0) fprintf(stdout, ", "); 
            fprintf(stdout, "%u", node->block[j]); 
        }
    }
    fprintf(stdout, "\n"); 
}

//inode 상세정보 출력 
void inode_detail_info(int idx, const inode *node){
    fprintf(stdout, "\tFile: %s\n", node->file_name); 
    fprintf(stdout, "\tSize: %d\n", node->size); 
    fprintf(stdout, "\t\tInode: %d\n", idx); 
    
    direct_block_info(node); 
    indirect_block_info(node); 
}


int main() {
    if(fread(&fs, sizeof(filesystem),1,stdin)!=1) {
        fprintf(stderr, "Read error\n"); 
        return 1; 
    }   
    superblock_info(); 

    fprintf(stdout, "Detailed File Information:\n"); 

    for(int i = 0; i<MAX_FILES; ++i) {
        if(fs.SB.inode_bitmap[i]) inode_detail_info(i, &fs.inodes[i]); 
    }
    return 0;
}
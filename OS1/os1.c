#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int pid; 
    int arrival_time;
    int code_bytes;
} process;

typedef struct {
    unsigned char op; 
    unsigned char len; 
} code_tuple; 

int main(int argc, char* argv[]){
    process data; 
    while(fread(&data, sizeof(process),1,  stdin)==1){
        fprintf(stdout, "%d %d %d\n", data.pid, data.arrival_time, data.code_bytes);
        //바이너리 파일에 있는 프로세스 정보를 출력해야함.
        //1개의 tuple은 2개의 byte로 이루어져있다. - 나눈다음에 tuple로 두기 
        for(int i = 0;i<data.code_bytes/2; i++){//각 tuple마다 읽어서 출력 
            code_tuple codeTuple; 
            if(fread(&codeTuple, sizeof(code_tuple),1,  stdin)!=1) break; 
            fprintf(stdout, "%d %d\n", codeTuple.op, codeTuple.len); //op는 16진수 
        }
    }
    return 0;
}
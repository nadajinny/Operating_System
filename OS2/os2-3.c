#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>

//list.h

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({                      \
        const __typeof__(((type *)0)->member) *__mptr = (ptr);  \
        (type *)((char *)__mptr - offsetof(type, member)); })

#define LIST_POISON1  ((void *) 0x00100100)
#define LIST_POISON2  ((void *) 0x00200200)

struct list_head {
	struct list_head* next, * prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

static inline void __list_add(struct list_head* new,
	struct list_head* prev,
	struct list_head* next) {
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void list_add(struct list_head* new, struct list_head* head) {
	__list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head* new, struct list_head* head) {
	__list_add(new, head->prev, head);
}

static inline void __list_del(struct list_head* prev, struct list_head* next) {
	next->prev = prev;
	prev->next = next;
}

static inline void list_del(struct list_head* entry) {
	__list_del(entry->prev, entry->next);
	entry->next = LIST_POISON1;
	entry->prev = LIST_POISON2;
}

static inline void list_del_init(struct list_head* entry) {
	__list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry);
}

static inline void list_move(struct list_head* list, struct list_head* head) {
	__list_del(list->prev, list->next);
	list_add(list, head);
}

static inline void list_move_tail(struct list_head* list,
	struct list_head* head) {
	__list_del(list->prev, list->next);
	list_add_tail(list, head);
}

static inline int list_empty(const struct list_head* head) {
	return head->next == head;
}

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define list_for_each(pos, head) \
  for (pos = (head)->next; pos != (head);	\
       pos = pos->next)

#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; prefetch(pos->prev), pos != (head); \
        	pos = pos->prev)

#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, __typeof__(*pos), member);	\
	     &pos->member != (head);					\
	     pos = list_entry(pos->member.next, __typeof__(*pos), member))

#define list_for_each_entry_reverse(pos, head, member)			\
	for (pos = list_entry((head)->prev, __typeof__(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = list_entry(pos->member.prev, __typeof__(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_entry((head)->next, __typeof__(*pos), member),	\
		n = list_entry(pos->member.next, __typeof__(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.next, __typeof__(*n), member))

#define list_for_each_entry_safe_reverse(pos, n, head, member)		\
	for (pos = list_entry((head)->prev, __typeof__(*pos), member),	\
		n = list_entry(pos->member.prev, __typeof__(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.prev, __typeof__(*n), member))

#if 0    //DEBUG
#define debug(fmt, args...) fprintf(stderr, fmt, ##args)
#else
#define debug(fmt, args...)
#endif

//구조체 정리 
typedef struct {
	unsigned char op; 
	unsigned char len; 
} code_tuple; 

typedef struct {
	int pid; 
	int arrival_time; 
	int code_bytes; 
	int load; 
	int pc; 
	code_tuple *code; 
	struct list_head job, ready, wait; 
} process;

typedef struct {
	struct list_head ready; 
	struct list_head wait; 
	process *cur_process; 
	int idle_time; 
	int cs_time; 
	int io_time; 
	int mig_time; 
} cpu; 


//초기 선언자
#define Context_switching_time 10 
#define pid_idle 100


LIST_HEAD(Q_JOB); 
LIST_HEAD(Q_READY); 
LIST_HEAD(Q_WAIT); 

cpu *CPU1; 
cpu	*CPU2; 


int prev_pid1 = 100; 
int prev_pid2 = 100; 

int total_clock = 0;


void FINAL_REPORT(){
	//util연산은 어떻게 해야하나?
	//CPU util 은 "전체 시뮬레이션 시간 가운데, 실제로 사용자, 커널 작업(op = 0 또는 1)을 실행한 비율"
	//전체 시뮬레이션 시간 -  total_clock 
	//실제로 사용자가 작업을 실행한 비율 - cpu_idle_time을 제외한 total_clock 
	//비율 - 퍼센테이지로 표현
	
	
	double cpu1_util = 100.0 * (total_clock - CPU1->idle_time) / total_clock; 
	double cpu2_util = 100.0 * (total_clock - CPU2->idle_time) / total_clock ; 


	fprintf(stdout, "*** TOTAL CLOCKS: %04d CPU1 IDLE: %04d CPU2 IDLE: %04d CPU1 UTIL: %2.2f%% CPU2 UTIL: %2.2f%% TOTAL UTIL: %2.2f%%\n",
		 total_clock, CPU1->idle_time, CPU2->idle_time,cpu1_util, cpu2_util, (cpu1_util+cpu2_util)/2.0); 
}


int cpu_length(cpu *c){
	int length = 0;
	struct list_head *now; 

	list_for_each(now, &c->ready) {
		length ++ ; 
	}

	return length; 
}


void Dynamic_free(){ //--------동적할당 해제 
	process *data; 
	process *nxt; 

	list_for_each_entry_safe(data, nxt, &Q_JOB, job){
		list_del(&data->job); 
		free(data->code); 
		free(data); 
	}
	free(CPU1); free(CPU2); 

}


void input(){ //------입력 받기 및 정리 
	process *data; 

	while(1){
		data = malloc(sizeof(*data)); 

		if(fread(&data->pid, sizeof(int), 1, stdin)!= 1) { free(data); break;  }
		if(fread(&data->arrival_time, sizeof(int), 1, stdin)!= 1) { free(data); break; }
		if(fread(&data->code_bytes, sizeof(int), 1, stdin)!=1) {  free(data); break;  }

		INIT_LIST_HEAD(&data->job); 
		INIT_LIST_HEAD(&data->ready); 
		INIT_LIST_HEAD(&data->wait); 

		data->code = malloc(sizeof(code_tuple)*data->code_bytes/2); 
		data->pc = 0;
		code_tuple codeTuple; 
		data->load = 0; 
		for(int i = 0;i<data->code_bytes/2; i++) {
			if(fread(&codeTuple, sizeof(code_tuple), 1, stdin)!=1) break;

			data->code[i] = codeTuple; 
		}
		list_add_tail(&data->job, &Q_JOB); 
	}
	
}

process *make_idle() {
	process *p_idle; 

	p_idle = malloc(sizeof(*p_idle)); 
	p_idle->pid = 100; 
	p_idle->arrival_time = 0; 
	p_idle->code_bytes = 2; 
	p_idle->pc = 0; 
	p_idle->load = 0; 
	p_idle->code = malloc(sizeof(code_tuple)); 
	p_idle->code[0].op = 0xFF; 
	p_idle->code[0].len = 0;

	INIT_LIST_HEAD(&p_idle->job); 
	INIT_LIST_HEAD(&p_idle->ready); 
	INIT_LIST_HEAD(&p_idle->wait); 


	return p_idle; 
}

void set_idle_process(){ //-------idle 프로세스 세팅 
	process *idle1 = make_idle(); 
	process *idle2 = make_idle(); 

	list_add_tail(&idle1->job, &Q_JOB); 
	list_add_tail(&idle2->job, &Q_JOB); 
}

void set_cpu(){ //CPU 세팅 
	CPU1 = malloc(sizeof(cpu)); 
	CPU2 = malloc(sizeof(cpu)); 

	INIT_LIST_HEAD(&CPU1->ready); 
	INIT_LIST_HEAD(&CPU1->wait); 
	CPU1->cur_process = NULL;
	CPU1->idle_time = 0;
	CPU1->cs_time = 0;
	CPU1->io_time = 0; 
	CPU1->mig_time = 0; 

	INIT_LIST_HEAD(&CPU2->ready); 
	INIT_LIST_HEAD(&CPU2->wait); 
	CPU2->cur_process = NULL; 
	CPU2->idle_time = 0;
	CPU2->cs_time = 0;
	CPU2->io_time = 0; 
	CPU2->mig_time = 0; 
}

int empty_readyqueue(cpu *c){ //idle_process를 제외하고 readyqueue 가 비어있는지지
	struct list_head *now; 
	list_for_each(now, &c->ready) {
		process *p = list_entry(now, process, ready); 
		if(p->pid != pid_idle) return 0;
	}
	return 1; 
}

int count_readyqueue(cpu *c){ //idle_process를 제외하고 readyqueue에 몇개의 프로세스가 있는지지
	struct list_head *now; 
	int len = 0; 
	list_for_each(now, &c->ready) {
		process *p = list_entry(now, process, ready); 
		if(p->pid != pid_idle) len++;
	}
	return len; 
}

int cpu_free(cpu *c) {  //cpu가 현재 아무것도 하지 않는 상태인지 
	if(empty_readyqueue(c) 
		&& c->cs_time == 0
		&& c->io_time == 0
	) return 1; 

	else return 0;
}

int user_process_over() { //Q_job에 할당한 프로세스가 모두 끝났는지 확인인
	process *p ; 
	list_for_each_entry(p, &Q_JOB, job) {
		if(p->pid== pid_idle) continue; 
		if(p->pc*2< p->code_bytes) return 0;
	}
	return 1; 
}

int over(){ 
	
	//idle을 제외하고 ready큐가 비어있는지
	//cpu가 완전히 놀고 있는지
	//모든 user프로세스가 끝났는지 

	if(!cpu_free(CPU1)) return 0;
	if(!cpu_free(CPU2)) return 0;
	if(!user_process_over()) return 0;

	return 1; 

}






void load_balance(){
	//job큐의 앞에서부터 arrival시간으로 인해 도달한 프로세스(무조건 time이 동일한 애만 해야한다.)만 ready queue에 넣고.
	//cpu의 ready queue의 길이를 비교하고 짧은 cpu에 프로세스를 넣는다. 
	//push완료 -> 다음 순서를 cur로 잡고 다음 순서를 돌아야함. 
	struct list_head *now, *next; 
	int a = 0;
	list_for_each_safe(now, next, &Q_JOB) {
		process *check_p = list_entry(now, process, job); 
		
		if(check_p->arrival_time > total_clock) continue; 

		if(check_p->load) continue; 

		
		if(CPU1->cs_time>=1  && CPU2->cs_time>=1) {
			continue;  
		}

		if(check_p->pid==100&& a==0){
			check_p->load = 1; 
			list_add_tail(&check_p->ready, &CPU1->ready); 
			fprintf(stdout, "%04d CPU%d: Loaded PID: %03d\tArrival: %03d\tCodesize: %03d\n" 
				,total_clock, 1, check_p->pid, check_p->arrival_time, check_p->code_bytes ); 
			a++; 
			continue; 
		}
		if(check_p->pid==100&& a==1){
			list_add_tail(&check_p->ready, &CPU2->ready); 
			check_p->load = 1; 
			fprintf(stdout, "%04d CPU%d: Loaded PID: %03d\tArrival: %03d\tCodesize: %03d\n" 
				,total_clock, 2, check_p->pid, check_p->arrival_time, check_p->code_bytes ); 
			continue; 
		}

		if(cpu_length(CPU1)<=cpu_length(CPU2)){
			list_add_tail(&check_p->ready, &CPU1->ready); 
			check_p->load = 1; 
			fprintf(stdout, "%04d CPU%d: Loaded PID: %03d\tArrival: %03d\tCodesize: %03d\n" 
				,total_clock, 1, check_p->pid, check_p->arrival_time, check_p->code_bytes ); 
		}
		else {
			list_add_tail(&check_p->ready, &CPU2->ready); 
			check_p->load = 1; 
			fprintf(stdout, "%04d CPU%d: Loaded PID: %03d\tArrival: %03d\tCodesize: %03d\n" 
				,total_clock, 2, check_p->pid, check_p->arrival_time, check_p->code_bytes ); 
		}
		
	}

}

void cpu_time(cpu *c, int num){
	//context switching 중일때
	//현재 실행 가능한 프로세스가 없을 때 <-> 현재 실행 가능한 프로세스가 있을 때
	int flag = 0; 

	
	if(c->cur_process
		&&c->cur_process->pid == pid_idle 
		&&c->cs_time == 0
		&& !list_empty(&c->ready)) {
			
		list_add_tail(&c->cur_process->ready, &c->ready); 
		c->cur_process = NULL; 
	}

	if(!c->cur_process) { //현재 프로세스를 실행시키는 게 없을 때
		if (!empty_readyqueue(c)) {
			process *p; 
			p = list_entry(c->ready.next, process, ready); 
			if(p->pid == pid_idle) {
				if(count_readyqueue(c) == 0) { return ; }
				
				list_move_tail(&p->ready, &c->ready); 
			}
		}

		c->cur_process = list_entry(c->ready.next , process, ready); 
		list_del(&c->cur_process->ready); 
		if(total_clock==0) {
			if(num==1){
				prev_pid1 = c->cur_process->pid; 
			}else {
				prev_pid2 = c->cur_process->pid; 				}		
		} 
		else {
			c->cs_time = Context_switching_time;
			c->idle_time += Context_switching_time; 
		}
		
		
	}

	if(c->cs_time == 0 ){
		code_tuple *cur_tuple = &c->cur_process->code[c->cur_process->pc]; 
		if(cur_tuple->op == 0) {
			if(--cur_tuple->len == 0) c->cur_process->pc++; 
		}
		// else if(cur_tuple->op == 1){
		// 	if(c->io_time == 0) {
		// 		c->io_time = cur_tuple->len; 
		// 		flag = 1; 
		// 		c->idle_time += c->io_time; 
		// 	}

		// 	c->io_time--; 
		// 	if(c->io_time == 0) {
		// 		c->cur_process->pc++;
		// 	}	
		// }

	}

	if(c->cur_process &&c->cur_process->pid ==pid_idle &&c->cs_time == 0 &&!flag) c->idle_time += 1; 

	//프로세스 종료 
	if(c->cur_process&&c->cur_process->pc*2 >= c->cur_process->code_bytes) {
		if(c->cur_process->pid!=100) {
			c->cur_process = NULL; 
		}
		else {
			list_add_tail(&c->cur_process->ready, &c->ready); 
			c->cur_process = NULL; 
		}
		
		c->io_time = 0 ; 
	} 

}

void switching(cpu *c, int num) {

	if(c->cs_time > 0) { //context switching중일 때에 cs_time감소, idle타임 증가
		c->cs_time --; 
		if(c->cs_time == 0 ) {
			int prev_pid = num == 1 ? prev_pid1 : prev_pid2;   
			fprintf(stdout, "%04d CPU%d: Switched\tfrom: %03d\tto: %03d\n",
				total_clock, num,prev_pid,c->cur_process->pid);
			if(num==1){
				prev_pid1 = c->cur_process->pid; 
			}else {
				prev_pid2 = c->cur_process->pid; 				}
		}		
	}
}


int migration_condition(int num) { //migration을 할 조건인지 확인
	cpu *c, *from; 
	if (num == 1) { c = CPU1; from = CPU2; } 
	else { c = CPU2; from = CPU1; }
	if(cpu_free(c)&&c->mig_time == 0 &&
	(c->cur_process==NULL || c->cur_process->pid == pid_idle)&& 
	(count_readyqueue(from)>=1)) return num;
	return 0;
}

void migration(int num) { //migration 처리 
	struct list_head *now, *next; 
	process *p;
	cpu *c, *from; 
	if(num == 1) { c = CPU1; from = CPU2; }
	else { c = CPU2; from = CPU1; }
	
	if(c->mig_time > 0 ){
		c->mig_time --; 
		if(c->mig_time == 0) {
			list_for_each_safe(now, next, &c->wait) {
				p = list_entry(now, process, wait); 
				list_del(&p->wait); 
				list_add_tail(&p->ready, &c->ready); 
				fprintf(stdout, "%04d  CPU%d: Migrate : COMPLETED!\tPID: %03d\n", total_clock, num, p->pid); 
			}
		}
	}
	else if(migration_condition(num)) {
		int move = count_readyqueue(from)/2; 
		now = from->ready.prev; 
		while(move-- > 0 && now != &from->ready){
			p = list_entry(now, process, ready); 
			now = now->prev; 
			if(p->pid == pid_idle) { move++; continue; }
			list_del(&p->ready); 
			list_add_tail(&p->wait, &c->wait); 
		}
		c->mig_time = 30; 
	}

}


void simulation(){
	
	migration(1); //1로의 migration cpu1에 프로세스 넣기
	migration(2);  
	
	switching(CPU1, 1);
	switching(CPU2, 2);  

	load_balance(); 

	cpu_time(CPU1, 1); 
	cpu_time(CPU2, 2); 

	total_clock++; 
	
}


int main(int argc, char* argv[]){	
	input(); 
	
	set_cpu(); 
	
	set_idle_process(); 

	load_balance(); 
	while(1){
		simulation(); 
		if(over()) break;
	}
	FINAL_REPORT(); 


	Dynamic_free(); 

	return 0;
}
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define DELAY 1
#define MAX_PROCESS 10
#define MAX_ARRIVE_TIME 20
#define MAX_BURST_TIME 20
#define MAX_IO_TIME 5
#define IO_FREQ 3
#define MAX_PERIOD 200

int Time;        // 시간 경과 표시
int Gantt[1000]; // 간트 차트
int Gantt2[1000]; // 간트 차트 for multilevelQueue

int Context_switch= 0;

typedef struct{
	int pid;
	int arrive_time;
	int waiting_time;
	int execute_time; //실행된 시간
	int burst_time;     //CPU burst time
	int remain_time;    //Remaining Burst time
	int finish_time;
	int io_start_time;  //=remain_cpu_time
	int io_remain_time; //I/O연산이 발생할 경우 할당되는 시
	int priority;
	int period;
}PCB;

typedef struct{
	int process;
	int context_switch;
	int sum_wait;
	int sum_burst;
	int sum_turn;
	int io_count;
	float avg_wait;
	float avg_burst;
	float avg_turn;
}RESULT;

typedef struct{
	int front;
	int rear;
	int size;
	int count;
	PCB*buffer;
}Queue;


PCB running_Q;    // Process on Cpu
RESULT result[10]; // Queue의 결과들을 저장해논 배

/*Get Process Number from User*/
int process_num(){
	int p_num = 0;
	while (1){
		printf("프로세스 갯수 입력(1-%d) : ", MAX_PROCESS);
		scanf("%d", &p_num);
		if (1 <=p_num && p_num <= MAX_PROCESS)
			break;
		else
			printf("1이상 %d 이하의 정수를 입력해주세요.\n", MAX_PROCESS);
	}
	return p_num;
}

/* Initialize PCB */
PCB init_PCB(){
	PCB pcb;
	pcb.pid = 0;
	pcb.arrive_time = 0;
	pcb.execute_time = 0;
	pcb.priority = 0;
	pcb.remain_time = 0;
	pcb.waiting_time = 0;
	pcb.io_start_time = 0;
	pcb.io_remain_time = 0;
	pcb.period = 0;
	return pcb;
}

/*Initialize Circular Queue with designated process_number */
void init_Queue(Queue *queue,int p_num){
	p_num += 1;   //n+1 size의 queue에 n개의 process가 들어갈 수 있다.
	queue->buffer = (PCB*)malloc(p_num*sizeof(PCB));
	memset(queue->buffer, 0, p_num * sizeof(PCB));
	queue->size = p_num;
	queue->front = 0;
	queue->rear = 0;
	queue->count = 0;
}

/*Get process in head of the Queue*/
PCB get_Queue_head(Queue * queue){ //큐 첫번째 항목 반환;
	return queue->buffer[(queue->front+1) % queue->size];
}

/* return True if Queue is full */
int is_Queue_full(Queue *queue){
	if (queue->front == (queue->rear+1)%queue->size)
		return 1;
	else
		return 0;
}

/* Dequeue */
void dequeue(Queue *queue){
	if(queue->count == 0){
		printf("dequeue_큐가 비어있습니다!\n");
		return;
	}
	queue->count -=1;
	queue->buffer[queue->front] = init_PCB();
	queue->front = (queue->front+1)%queue->size;
}

/* Enqueue: rear를 증가시킨 후 그 자리에 process삽입 */
void enqueue(Queue *queue, PCB process){
	if((queue->rear+1)%queue->size == queue->front){
		printf("enqueue_큐가 꽉차있습니다!\n");
		exit(-1);
	}
	queue->count += 1;
	queue->rear = (queue->rear+1) % queue->size;
	queue->buffer[queue->rear] = process;
}

/* Get number of process in Queue */
int get_Queue_count(Queue *queue){
	return queue->count;
}

/* Copy queue to copy_Q */
void copy_Queue(Queue*queue, Queue*copy_Q){
	copy_Q->front = queue->front;
	copy_Q->rear  = queue->rear;
	copy_Q->count = queue->count;
	copy_Q->size  = queue->size;

	for(int i=0; i<queue->size; i++){
		copy_Q->buffer[i] = queue->buffer[i];
	}
}


/* ready_Q -> running_Q
	running_Q의 작업이 끝나지 않았다면 ready_Q로 */
PCB dispatcher(Queue*ready_Q, PCB running_Q){
	if(running_Q.remain_time != 0){ //작업이 아직 끝나지 않음
		enqueue(ready_Q,running_Q);
	}
	running_Q = get_Queue_head(ready_Q);
	Context_switch += 1;
	return running_Q;
}


/*Sort Queue by arrival time ->for FCFS */
void sort_by_arrival(Queue *queue){
	int i,j,minindex;
	PCB min;
	PCB temp;
	for(int i=(queue->front + 1)%queue->size; (i % queue->size)!=((queue->rear) % queue->size); i++){
		i = i% queue->size;
		minindex = i;
		min = queue->buffer[i];
		for(int j=(i+1)% queue->size; (j % queue->size)!=((queue->rear+1) % queue->size); j++){
		j = j % queue->size;
			temp = queue->buffer[j];
			if(min.arrive_time > temp.arrive_time || 
			                (min.arrive_time == temp.arrive_time && min.pid > temp.pid)){
				minindex = j;
				min = temp;
			}
		}
		queue->buffer[minindex] = queue->buffer[i];
		queue->buffer[i] = min;
	}
}

/*Sort Queue by remaining time ->for SJF */
void sort_by_remain(Queue *queue){
	int i,j,minindex;
	PCB min;
	PCB temp;
	for(int i=(queue->front + 1)%queue->size; (i % queue->size)!=((queue->rear) % queue->size); i++){
		i = i % queue->size;
		minindex = i;
		min = queue->buffer[i];
		for(int j=(i+1)% queue->size; (j % queue->size)!=((queue->rear+1) % queue->size); j++){
			j = j % queue->size;
			temp = queue->buffer[j];
			if(min.remain_time > temp.remain_time || 
			                (min.remain_time == temp.remain_time && min.pid > temp.pid)){
				minindex = j;
				min = temp;
			}
		}
		queue->buffer[minindex] = queue->buffer[i];
		queue->buffer[i] = min;
	}
}

/*Sort Queue by priority time ->for priority first */
void sort_by_priority(Queue *queue){
	int i,j,minindex;
	PCB min;
	PCB temp;
	for(int i=(queue->front + 1)%queue->size; (i % queue->size)!=((queue->rear) % queue->size); i++){
		i = i % queue->size;
		minindex = i;
		min = queue->buffer[i];
		for(int j=(i+1)% queue->size; (j % queue->size)!=((queue->rear+1) % queue->size); j++){
			j = j % queue->size;
			temp = queue->buffer[j];
			if(min.priority > temp.priority ||  
				(min.priority == temp.priority && min.pid < temp.pid)){
				minindex = j;
				min = temp;
			}
		}
		queue->buffer[minindex] = queue->buffer[i];
		queue->buffer[i] = min;
	}
}

/*random_process()의 random한 priority를 생성해준다*/
int* random_priority_array(int* priority_arr,int size){
	int temp;
	int mix1, mix2;
	int mix_num = 50;
	srand(time(NULL));
	
	for(int i=0; i<size; i++) //initialize array
		priority_arr[i] = i+1;
	for(int i=0; i<mix_num; i++){ //mix_num만큼 swap
		mix1 = rand() % size;
		mix2 = rand() % size;
		temp = priority_arr[mix1];
		priority_arr[mix1] = priority_arr[mix2];
		priority_arr[mix2] = temp;
	}
	return priority_arr;
}

/*Initialize가 된 Queue에 대해서 size크기 만큼 random한 process생성*/ 
void fill_random_process(Queue *queue){
	int temp_p_arr[100];
	int *priority_arr = random_priority_array(temp_p_arr,queue->size);
	srand(time(NULL));
	for(int i=queue->front + 1; i< queue->size; i++){
		queue->buffer[i].pid = i;
		queue->buffer[i].arrive_time = rand() % MAX_ARRIVE_TIME;    // 0~MAX_ARRIVE_TIME-1
		queue->buffer[i].burst_time  = rand() % MAX_BURST_TIME + 1; // 1~MAX_BURST_TIME
		queue->buffer[i].waiting_time = 0;
		queue->buffer[i].execute_time = 0;
		queue->buffer[i].finish_time = 0;
		queue->buffer[i].remain_time = queue->buffer[i].burst_time; //initialize with burst_time
		queue->buffer[i].priority = priority_arr[i]; //random priority
		queue->buffer[i].period = ((rand() % 4 +1) * MAX_PERIOD) /4; // 25,50,75,100
		if(rand() % IO_FREQ == 0 && queue->buffer[i].remain_time>1){  //IO_FREQ꼴에 한번 씩 IO연산 할당
			queue->buffer[i].io_remain_time = rand() % MAX_IO_TIME +1;  //1 ~ MAX_IO_TIME
			queue->buffer[i].io_start_time = rand() % queue->buffer[i].remain_time;
			if (queue->buffer[i].io_start_time == 0)
				queue->buffer[i].io_start_time = 1;
		}
		queue->count += 1;
		queue->rear += 1;
	}
	queue->rear = queue->rear % queue->size;
}

/*Print all the Processes existing in the Queue*/
void print_Queue(Queue*queue){
	PCB temp;

	printf("[Process_Information]\n");
	printf("| PID | arrive_t | finish_t | wait_t | burst_t | execute_t | remain_t | priority | io_start| io_remain| period |\n");

	for(int i=(queue->front + 1); (i % queue->size)!=((queue->rear+1) % queue->size); i++){
		i = i % queue->size;
		temp = queue->buffer[i];
		printf("|  %d  |    %d     |    %d     |   %d    |    %d    |     %d     |    %d     |     %d    |    %d    |    %d    |   %d   |\n"
			,temp.pid,temp.arrive_time,temp.finish_time,temp.waiting_time,temp.burst_time,temp.execute_time
			,temp.remain_time, temp.priority,temp.io_start_time,temp.io_remain_time, temp.period);
	}
	printf("\n");
}

/*Print process in running Queue*/
void print_running_Q(PCB running_Q){
	printf("\nProcess_Information\n");
	printf("| PID | arrive_t | wait_t | burst_t | execute_t | remain_t | priority | io_start| io_remain|\n");

	printf("|  %d  |    %d     |   %d    |    %d    |     %d     |    %d     |     %d    |    %d    |    %d    | \n"
			,running_Q.pid,running_Q.arrive_time,running_Q.waiting_time,running_Q.burst_time
			,running_Q.execute_time,running_Q.remain_time,running_Q.priority
			,running_Q.io_start_time,running_Q.io_remain_time);
}

void init_Gantt(){
	for(int i=0; i<1000; i++)
		Gantt[i] = 0;
}
/*Print Gannt Chart until given time*/
void print_Gantt(int time){
	printf("[Gantt Chart]\n");
	printf("process: ");
	if(Gantt[0] == 0)
		printf("|");
	for(int i=0; i < time; i++){
		if(Gantt[i] != Gantt[i-1]){
			if(Gantt[i] >= 10)
				printf("|%d|",Gantt[i]);
			else
				printf("|%d",Gantt[i]);
		}
		else{
			if(Gantt[i] >= 10)
				printf("%d|",Gantt[i]);
			else
				printf("%d",Gantt[i]);
		}
	}
	printf("|\n");
	printf("------------------------------------------------------------------\n");
}

void print_algorithm(int a){
	if(a == 0)
		printf("|         FCFS        |");
	else if(a == 1)
		printf("|  Nonpreemptive SJF  |");
	else if(a == 2)
		printf("|Nonpreemptive Priorit|");
	else if(a == 3)
		printf("|      Round Robin    |");
	else if(a == 4)
		printf("|    Preemptive SJF   |");
	else if(a == 5)
		printf("| Preemptive Priority |");
	else if(a == 6)
		printf("|  Pre_aging Priority |");
	else if(a == 7)
		printf("|  Multilevel Queue   |");
	else if(a == 8)
		printf("| Multilevel Feedback |");
	else if(a == 9)
		printf("|   Rate - Monotonic  |");
	else if(a == 10)
		printf("|   Earliest Deadline |");
	
	printf("\n");
	printf("-----------------------\n");
}

/*Print Result of queue (wait, burst등의 연산이 완료 되어 있어야함!)  a = algorithm_num  */
void print_result(Queue *queue, int a){
	int sum_wait =0, sum_burst = 0, sum_turn = 0;
  //queue->front+1에서 queue->rear까지 출력
	for(int i=(queue->front + 1); (i % queue->size)!=((queue->rear+1) % queue->size); i++){
		sum_wait  = sum_wait  + queue->buffer[i].waiting_time;
	}
	printf("\n*****RESULT*****\n");
	printf("|      Algorithm     |   Number of Process  |  Number of Context Switch  |\n");
	print_algorithm(a);
	printf("      %8d        | %15d            | \n\n",queue->count, Context_switch);
	printf("| Sum of Waiting time | Average Waiting time |\n");
	printf("|         %d          |        %.3f        |\n",sum_wait, (float)sum_wait/queue->count);
	printf("|  Sum of Burst time  |  Average Burst time  |\n");
	printf("|         %d          |        %.3f        |\n",sum_burst, (float)sum_burst/queue->count);
	printf("|  Sum of Turn Time   |   Average Turn Time  |\n");
	printf("|         %d          |        %.3f        |\n",sum_turn, (float)sum_turn/queue->count);
}

void initialize_result(){
	for(int i= 0; i<15; i++){
		result[i].sum_wait = 0;
		result[i].sum_burst = 0;
		result[i].sum_turn = 0;
	}
}

//Result에 저장된 알고리즘별 최종 값들을 출력한다.
void print_all_result(){
	printf("\n*****Summary of all Results*****\n");
	for(int i=0; i<11; i++){
		printf("----------------------------------------------------------------------------\n");
		printf("|      Algorithm      |\n");
		print_algorithm(i);
		printf("|  Number of Process  |    Context_Switch    |\n");
		printf("|         %d          |          %d          | \n",result[i].process, result[i].context_switch);
		printf("| Sum of Waiting time | Average Waiting time |\n");
		printf("|         %d          |        %.3f        |\n",result[i].sum_wait, result[i].avg_wait);
		printf("|  Sum of Burst time  |  Average Burst time  |\n");
		printf("|         %d          |        %.3f        |\n",result[i].sum_burst, result[i].avg_burst);
		printf("|  Sum of Turn Time   |   Average Turn Time  |\n");
		printf("|         %d          |        %.3f        |\n",result[i].sum_turn, result[i].avg_turn);
		printf("----------------------------------------------------------------------------\n\n");
	}
}

void FCFS(Queue *queue){
	printf("\n*****FCFS*****\n");
	Context_switch = 0;
	Time = 0;
	int io_count =0;

	init_Gantt();
	Queue FCFS;    //생성된 프로세스들을 저장한 queue를 받아서 저장한다.
	init_Queue(&FCFS, queue->size-1);
	copy_Queue(queue, &FCFS); 

	Queue ready_Q, waiting_Q, terminated_Q;
	init_Queue(&ready_Q, queue->size-1);
	init_Queue(&waiting_Q, queue->size-1);
	init_Queue(&terminated_Q, queue->size-1);

	PCB running_Q;
	running_Q = init_PCB();
	
	while(is_Queue_full(&terminated_Q) != 1){ //terminated_Q가 꽉찰때 까지 time을 1씩 늘려가며 반복		
		if (Time == 200){
			printf("[Ready_Q]\n");
			printf("front:%d, rear:%d, count:%d\n",ready_Q.front,ready_Q.rear,ready_Q.count);
			print_Queue(&ready_Q);
			printf("[Waiting_Q]\n");
			print_Queue(&waiting_Q);
			break;
		}
		
	
		for(int i=FCFS.front+1; FCFS.buffer[i].pid!=0; i++){ //TIME에 도달할 때 ready_Q에 들어간다
			if(FCFS.buffer[i].arrive_time != Time) 
				break;
			else{
				enqueue(&ready_Q, get_Queue_head(&FCFS));
				dequeue(&FCFS);
			}
		}	
		//sort_by_arrival(&ready_Q);
      
		if(waiting_Q.buffer[waiting_Q.front+1].pid!=0){ //waiting_Q에 프로세스가 존재한다면 io_time을 1줄인다.
			waiting_Q.buffer[waiting_Q.front+1].io_remain_time -= 1;
			
			if(waiting_Q.buffer[waiting_Q.front+1].io_remain_time ==0){ //I.O 입출력이 끝난다면, ready_q로 보낸다
				if (waiting_Q.buffer[waiting_Q.front+1].remain_time ==0){
					//waiting_Q.buffer[waiting_Q.front+1].arrive_time = Time;
					waiting_Q.buffer[waiting_Q.front+1].finish_time = Time;
					enqueue(&terminated_Q, waiting_Q.buffer[waiting_Q.front+1]);
					dequeue(&waiting_Q);
				}
				else{
					//waiting_Q.buffer[waiting_Q.front+1].arrive_time = Time;  //Time으로 도착시간을 변경한뒤 enqueue
					enqueue(&ready_Q, waiting_Q.buffer[waiting_Q.front+1]);
					dequeue(&waiting_Q);
					}
			}
		}
		//sort_by_arrival(&ready_Q);	
		
		if(running_Q.pid !=0){ //run_Q존재, ready_q도 존재
			running_Q.remain_time -= 1;
			running_Q.execute_time += 1;
			
			if(running_Q.remain_time <= 0 & running_Q.io_remain_time == 0){ //no more burst and no more i/o
				running_Q.finish_time = Time;
				enqueue(&terminated_Q,running_Q);
				running_Q = init_PCB();
			}
			
			if(running_Q.io_remain_time !=0 &&
					(running_Q.io_start_time == running_Q.remain_time)){ //만약 IO연산을 수행해야 한다면
				io_count += 1;
				enqueue(&waiting_Q, running_Q); //waiting_Q로 이동시킨다.
				running_Q = init_PCB();         //run_Q를 초기화
			}
		}

		if(running_Q.pid == 0 && get_Queue_head(&ready_Q).pid!=0){ //run_Q empty, is ready_Q : ready->run
			running_Q = get_Queue_head(&ready_Q);
			dequeue(&ready_Q);
			Context_switch+=1;
		}
		
		Gantt[Time] = running_Q.pid;
		Time += 1;	
	}
	for(int i=terminated_Q.front+1; i<= terminated_Q.rear; i++){
		PCB temp;
		temp = terminated_Q.buffer[i];
		terminated_Q.buffer[i].waiting_time = temp.finish_time - temp.execute_time - temp.arrive_time;
	}
	print_Queue(&terminated_Q);
	print_Gantt(Time-1);
	
	result[0].process = queue->size-1;
	result[0].context_switch = Context_switch;
	for(int i=(terminated_Q.front+1); i<=terminated_Q.rear; i++){
		result[0].sum_wait += terminated_Q.buffer[i].waiting_time;
		result[0].sum_burst += terminated_Q.buffer[i].burst_time;
		result[0].sum_turn += terminated_Q.buffer[i].waiting_time + terminated_Q.buffer[i].burst_time;
	}
	result[0].avg_wait = (float)result[0].sum_wait / result[0].process;
	result[0].avg_burst = (float)result[0].sum_burst / result[0].process;
	result[0].avg_turn = (float)result[0].sum_turn / result[0].process;
	result[0].io_count = io_count;
}


void Nonpreemptive_SJF(Queue *queue){
	printf("\n**Nonpreemtive SJF**\n");
	Context_switch = 0;
	Time = 0;
	int io_count =0;

	init_Gantt();
	Queue SJF;    //생성된 프로세스들을 저장한 queue를 받아서 저장한다.
	init_Queue(&SJF, queue->size-1);
	copy_Queue(queue, &SJF); 

	Queue ready_Q, waiting_Q, terminated_Q;
	init_Queue(&ready_Q, queue->size-1);
	init_Queue(&waiting_Q, queue->size-1);
	init_Queue(&terminated_Q, queue->size-1);

	PCB running_Q;
	running_Q = init_PCB();

	sort_by_arrival(&SJF); // SJF에 있는 프로세스들을 도착시간 순으로 정렬한다
	
	while(is_Queue_full(&terminated_Q) != 1){ //terminated_Q가 꽉찰때 까지 time을 1씩 늘려가며 반복		
	
		for(int i=SJF.front+1; SJF.buffer[i].pid!=0; i++){ //TIME에 도달할 때 ready_Q에 들어간다
			if(SJF.buffer[i].arrive_time != Time) 
				break;
			else{
				enqueue(&ready_Q, get_Queue_head(&SJF));
				dequeue(&SJF);
			}
		}
      
		if(waiting_Q.buffer[waiting_Q.front+1].pid!=0){ //waiting_Q에 프로세스가 존재한다면 io_time을 1줄인다.
			waiting_Q.buffer[waiting_Q.front+1].io_remain_time -= 1;
			
			if(waiting_Q.buffer[waiting_Q.front+1].io_remain_time ==0){ //I.O 입출력이 끝난다면, ready_q로 보낸다
				if (waiting_Q.buffer[waiting_Q.front+1].remain_time ==0){
					waiting_Q.buffer[waiting_Q.front+1].finish_time = Time;
					enqueue(&terminated_Q, waiting_Q.buffer[waiting_Q.front+1]);
					dequeue(&waiting_Q);
				}
				else{
					enqueue(&ready_Q, waiting_Q.buffer[waiting_Q.front+1]);
					dequeue(&waiting_Q);
					}
			}
		}
		sort_by_remain(&ready_Q);	
		
		if(running_Q.pid !=0){ //run_Q존재, ready_q도 존재
			running_Q.remain_time -= 1;
			running_Q.execute_time += 1;
			
			if(running_Q.remain_time <= 0 & running_Q.io_remain_time == 0){ //no more burst and no more i/o
				running_Q.finish_time = Time;
				enqueue(&terminated_Q,running_Q);
				running_Q = init_PCB();
			}
			
			if(running_Q.io_remain_time !=0 &&
					(running_Q.io_start_time == running_Q.remain_time)){ //만약 IO연산을 수행해야 한다면
				io_count += 1;
				enqueue(&waiting_Q, running_Q); //waiting_Q로 이동시킨다.
				running_Q = init_PCB();         //run_Q를 초기화
			}
		}

		if(running_Q.pid == 0 && get_Queue_head(&ready_Q).pid!=0){ //run_Q empty, is ready_Q : ready->run
			running_Q = get_Queue_head(&ready_Q);
			dequeue(&ready_Q);
			Context_switch+=1;
		}
		
		Gantt[Time] = running_Q.pid;
		Time += 1;	
	}
	for(int i=terminated_Q.front+1; i<= terminated_Q.rear; i++){
		PCB temp;
		temp = terminated_Q.buffer[i];
		terminated_Q.buffer[i].waiting_time = temp.finish_time - temp.execute_time - temp.arrive_time;
	}
	print_Queue(&terminated_Q);	
	print_Gantt(Time-1);

	result[1].process = queue->size-1;
	result[1].context_switch = Context_switch;
	for(int i=(terminated_Q.front+1); i<=terminated_Q.rear; i++){
		result[1].sum_wait += terminated_Q.buffer[i].waiting_time;
		result[1].sum_burst += terminated_Q.buffer[i].burst_time;
		result[1].sum_turn += terminated_Q.buffer[i].waiting_time + terminated_Q.buffer[i].burst_time;
	}
	result[1].avg_wait = (float)result[1].sum_wait / result[1].process;
	result[1].avg_burst = (float)result[1].sum_burst / result[1].process;
	result[1].avg_turn = (float)result[1].sum_turn / result[1].process;
	result[1].io_count = io_count;
}


void Nonpreemptive_Priority(Queue *queue){
	printf("\n**Nonpreemtive Priority**\n");
	Context_switch = 0;
	Time = 0;
	int io_count =0;

	init_Gantt();
	Queue PRIORITY;    //생성된 프로세스들을 저장한 queue를 받아서 저장한다.
	init_Queue(&PRIORITY,queue->size-1);
	copy_Queue(queue, &PRIORITY); 

	Queue ready_Q, waiting_Q, terminated_Q;
	init_Queue(&ready_Q, queue->size-1);
	init_Queue(&waiting_Q, queue->size-1);
	init_Queue(&terminated_Q, queue->size-1);

	PCB running_Q;
	running_Q = init_PCB();

	sort_by_arrival(&PRIORITY); // SJF에 있는 프로세스들을 도착시간 순으로 정렬한다
	
	while(is_Queue_full(&terminated_Q) != 1){ //terminated_Q가 꽉찰때 까지 time을 1씩 늘려가며 반복		
		for(int i=PRIORITY.front+1; PRIORITY.buffer[i].pid!=0; i++){ //TIME에 도달할 때 ready_Q에 들어간다
			if(PRIORITY.buffer[i].arrive_time != Time) 
				break;
			else{
				enqueue(&ready_Q, get_Queue_head(&PRIORITY));
				dequeue(&PRIORITY);
			}
		}
		sort_by_priority(&ready_Q);
      
		if(waiting_Q.buffer[waiting_Q.front+1].pid!=0){ //waiting_Q에 프로세스가 존재한다면 io_time을 1줄인다.
			waiting_Q.buffer[waiting_Q.front+1].io_remain_time -= 1;
			
			if(waiting_Q.buffer[waiting_Q.front+1].io_remain_time ==0){ //I.O 입출력이 끝난다면, ready_q로 보낸다
				if (waiting_Q.buffer[waiting_Q.front+1].remain_time ==0){
					waiting_Q.buffer[waiting_Q.front+1].finish_time = Time;
					enqueue(&terminated_Q, waiting_Q.buffer[waiting_Q.front+1]);
					dequeue(&waiting_Q);
				}
				else{
					enqueue(&ready_Q, waiting_Q.buffer[waiting_Q.front+1]);
					dequeue(&waiting_Q);
					}
			}
		}
		sort_by_priority(&ready_Q);	
		
		if(running_Q.pid !=0){ //run_Q존재, ready_q도 존재
			running_Q.remain_time -= 1;
			running_Q.execute_time += 1;
			
			if(running_Q.remain_time <= 0 & running_Q.io_remain_time == 0){ //no more burst and no more i/o
				running_Q.finish_time = Time;
				enqueue(&terminated_Q,running_Q);
				running_Q = init_PCB();
			}
			
			if(running_Q.io_remain_time !=0 &&
					(running_Q.io_start_time == running_Q.remain_time)){ //만약 IO연산을 수행해야 한다면
				io_count += 1;
				enqueue(&waiting_Q, running_Q); //waiting_Q로 이동시킨다.
				running_Q = init_PCB();         //run_Q를 초기화
			}
		}

		if(running_Q.pid == 0 && get_Queue_head(&ready_Q).pid!=0){ //run_Q empty, is ready_Q : ready->run
			running_Q = get_Queue_head(&ready_Q);
			dequeue(&ready_Q);
			Context_switch+=1;
		}
		
		Gantt[Time] = running_Q.pid;
		Time += 1;	
	}
	for(int i=terminated_Q.front+1; i<= terminated_Q.rear; i++){
		PCB temp;
		temp = terminated_Q.buffer[i];
		terminated_Q.buffer[i].waiting_time = temp.finish_time - temp.execute_time - temp.arrive_time;
	}
	print_Queue(&terminated_Q);
	print_Gantt(Time-1);
	
	result[2].process = queue->size-1;
	result[2].context_switch = Context_switch;
	for(int i=(terminated_Q.front+1); i<=terminated_Q.rear; i++){
		result[2].sum_wait += terminated_Q.buffer[i].waiting_time;
		result[2].sum_burst += terminated_Q.buffer[i].burst_time;
		result[2].sum_turn += terminated_Q.buffer[i].waiting_time + terminated_Q.buffer[i].burst_time;
	}
	result[2].avg_wait = (float)result[2].sum_wait / result[2].process;
	result[2].avg_burst = (float)result[2].sum_burst / result[2].process;
	result[2].avg_turn = (float)result[2].sum_turn / result[2].process;
	result[2].io_count = io_count;
}


void Round_Robin(Queue *queue){
	printf("\n***Round Robin***\n");
	Context_switch = 0;
	Time = 0;
	int io_count =0;
	int time_quantum = 3;

	init_Gantt();
	Queue RR;    //생성된 프로세스들을 저장한 queue를 받아서 저장한다.
	init_Queue(&RR, queue->size-1);
	copy_Queue(queue, &RR); 

	Queue ready_Q, waiting_Q, terminated_Q;
	init_Queue(&ready_Q, queue->size-1);
	init_Queue(&waiting_Q, queue->size-1);
	init_Queue(&terminated_Q, queue->size-1);

	PCB running_Q;
	running_Q = init_PCB();

	sort_by_arrival(&RR); // FCFS에 있는 프로세스들을 도착시간 순으로 정렬한다
	
	while(is_Queue_full(&terminated_Q) != 1){ //terminated_Q가 꽉찰때 까지 time을 1씩 늘려가며 반복		
		for(int i=RR.front+1; RR.buffer[i].pid!=0; i++){ //TIME에 도달할 때 ready_Q에 들어간다
			if(RR.buffer[i].arrive_time != Time) 
				break;
			else{
				enqueue(&ready_Q, get_Queue_head(&RR));
				dequeue(&RR);
			}
		}	
		//sort_by_arrival(&ready_Q);
      
		if(waiting_Q.buffer[waiting_Q.front+1].pid!=0){ //waiting_Q에 프로세스가 존재한다면 io_time을 1줄인다.
			waiting_Q.buffer[waiting_Q.front+1].io_remain_time -= 1;
			
			if(waiting_Q.buffer[waiting_Q.front+1].io_remain_time ==0){ //I.O 입출력이 끝난다면, ready_q로 보낸다
				if (waiting_Q.buffer[waiting_Q.front+1].remain_time ==0){
					//waiting_Q.buffer[waiting_Q.front+1].arrive_time = Time;
					waiting_Q.buffer[waiting_Q.front+1].finish_time = Time;
					enqueue(&terminated_Q, waiting_Q.buffer[waiting_Q.front+1]);
					dequeue(&waiting_Q);
				}
				else{
					//waiting_Q.buffer[waiting_Q.front+1].arrive_time = Time;  //Time으로 도착시간을 변경한뒤 enqueue
					enqueue(&ready_Q, waiting_Q.buffer[waiting_Q.front+1]);
					dequeue(&waiting_Q);
					}
			}
		}
		//sort_by_arrival(&ready_Q);	
		
		if(running_Q.pid !=0){ //run_Q존재, ready_q도 존재
			running_Q.remain_time -= 1;
			running_Q.execute_time += 1;
			
			if(running_Q.remain_time <= 0 & running_Q.io_remain_time == 0){ //no more burst and no more i/o
				running_Q.finish_time = Time;
				enqueue(&terminated_Q,running_Q);
				running_Q = init_PCB();
			}
			
			if(running_Q.io_remain_time !=0 &&
					(running_Q.io_start_time == running_Q.remain_time)){ //만약 IO연산을 수행해야 한다면
				io_count += 1;
				enqueue(&waiting_Q, running_Q); //waiting_Q로 이동시킨다.
				running_Q = init_PCB();         //run_Q를 초기화
			}
			
			if(running_Q.pid!=0 && (running_Q.execute_time % time_quantum) ==0){
				if (running_Q.execute_time>0){
					enqueue(&ready_Q,running_Q);
					running_Q = init_PCB();
				}
			}
		}

		if(running_Q.pid == 0 && get_Queue_head(&ready_Q).pid!=0){ //run_Q empty, is ready_Q : ready->run
			running_Q = get_Queue_head(&ready_Q);
			dequeue(&ready_Q);
			Context_switch+=1;
		}
		
		Gantt[Time] = running_Q.pid;
		Time += 1;	
	}
	for(int i=terminated_Q.front+1; i<= terminated_Q.rear; i++){
		PCB temp;
		temp = terminated_Q.buffer[i];
		terminated_Q.buffer[i].waiting_time = temp.finish_time - temp.execute_time - temp.arrive_time;
	}
	print_Queue(&terminated_Q);
	print_Gantt(Time-1);
	
	result[3].process = queue->size-1;
	result[3].context_switch = Context_switch;
	for(int i=(terminated_Q.front+1); i<=terminated_Q.rear; i++){
		result[3].sum_wait += terminated_Q.buffer[i].waiting_time;
		result[3].sum_burst += terminated_Q.buffer[i].burst_time;
		result[3].sum_turn += terminated_Q.buffer[i].waiting_time + terminated_Q.buffer[i].burst_time;
	}
	result[3].avg_wait = (float)result[3].sum_wait / result[3].process;
	result[3].avg_burst = (float)result[3].sum_burst / result[3].process;
	result[3].avg_turn = (float)result[3].sum_turn / result[3].process;
	result[3].io_count = io_count;
}


void Preemptive_SJF(Queue *queue){
	printf("\n**Preemtive SJF**\n");
	Context_switch = 0;
	Time = 0;
	int io_count =0;

	init_Gantt();
	Queue SJF;    //생성된 프로세스들을 저장한 queue를 받아서 저장한다.
	init_Queue(&SJF, queue->size-1);
	copy_Queue(queue, &SJF); 

	Queue ready_Q, waiting_Q, terminated_Q;
	init_Queue(&ready_Q, queue->size-1);
	init_Queue(&waiting_Q, queue->size-1);
	init_Queue(&terminated_Q, queue->size-1);

	PCB running_Q;
	running_Q = init_PCB();

	sort_by_arrival(&SJF); // SJF에 있는 프로세스들을 도착시간 순으로 정렬한다
	
	while(is_Queue_full(&terminated_Q) != 1){ //terminated_Q가 꽉찰때 까지 time을 1씩 늘려가며 반복		
		for(int i=SJF.front+1; SJF.buffer[i].pid!=0; i++){ //TIME에 도달할 때 ready_Q에 들어간다
			if(SJF.buffer[i].arrive_time != Time) 
				break;
			else{
				enqueue(&ready_Q, get_Queue_head(&SJF));
				dequeue(&SJF);
			}
		}
		sort_by_remain(&ready_Q);
      
		if(waiting_Q.buffer[waiting_Q.front+1].pid!=0){ //waiting_Q에 프로세스가 존재한다면 io_time을 1줄인다.
			waiting_Q.buffer[waiting_Q.front+1].io_remain_time -= 1;
			
			if(waiting_Q.buffer[waiting_Q.front+1].io_remain_time ==0){ //I.O 입출력이 끝난다면, ready_q로 보낸다
				if (waiting_Q.buffer[waiting_Q.front+1].remain_time ==0){
					waiting_Q.buffer[waiting_Q.front+1].finish_time = Time;
					enqueue(&terminated_Q, waiting_Q.buffer[waiting_Q.front+1]);
					dequeue(&waiting_Q);
				}
				else{
					enqueue(&ready_Q, waiting_Q.buffer[waiting_Q.front+1]);
					dequeue(&waiting_Q);
				}
			}
		}
		sort_by_remain(&ready_Q);	
		
		if(running_Q.pid !=0){ //run_Q존재
			running_Q.remain_time -= 1;
			running_Q.execute_time += 1;
			
			if(running_Q.remain_time <= 0 & running_Q.io_remain_time == 0){ //no more burst and no more i/o
				running_Q.finish_time = Time;
				enqueue(&terminated_Q,running_Q);
				running_Q = init_PCB();
			}
			
			if(running_Q.io_remain_time !=0 &&
					(running_Q.io_start_time == running_Q.remain_time)){ //만약 IO연산을 수행해야 한다면
				io_count += 1;
				enqueue(&waiting_Q, running_Q); //waiting_Q로 이동시킨다.
				running_Q = init_PCB();         //run_Q를 초기화
			}
			if(running_Q.pid != 0 && get_Queue_head(&ready_Q).pid!=0){ //만약 running_q가 아직 있고, ready_Q도 있다면
				if(running_Q.remain_time > get_Queue_head(&ready_Q).remain_time){ //ready_Q가 더 shortest_job이라면 비운다
					enqueue(&ready_Q,running_Q);
					running_Q = init_PCB();
				}
			}
		}

		if(running_Q.pid == 0 && get_Queue_head(&ready_Q).pid!=0){ //run_Q empty, is ready_Q : ready->run
			running_Q = get_Queue_head(&ready_Q);
			dequeue(&ready_Q);
			Context_switch+=1;
		}
		
		Gantt[Time] = running_Q.pid;
		Time += 1;	
	}
	for(int i=terminated_Q.front+1; i<= terminated_Q.rear; i++){
		PCB temp;
		temp = terminated_Q.buffer[i];
		terminated_Q.buffer[i].waiting_time = temp.finish_time - temp.execute_time - temp.arrive_time;
	}
	print_Queue(&terminated_Q);	
	print_Gantt(Time-1);

	
	result[4].process = queue->size-1;
	result[4].context_switch = Context_switch;
	for(int i=(terminated_Q.front+1); i<=terminated_Q.rear; i++){
		result[4].sum_wait += terminated_Q.buffer[i].waiting_time;
		result[4].sum_burst += terminated_Q.buffer[i].burst_time;
		result[4].sum_turn += terminated_Q.buffer[i].waiting_time + terminated_Q.buffer[i].burst_time;
	}
	result[4].avg_wait = (float)result[4].sum_wait / result[4].process;
	result[4].avg_burst = (float)result[4].sum_burst / result[4].process;
	result[4].avg_turn = (float)result[4].sum_turn / result[4].process;
	result[4].io_count = io_count;
}

void Preemptive_Priority(Queue *queue,int aging){
	int result_num;
	if(aging){
		printf("\n**Preemtive Priority(aging)**\n");
		result_num = 6;
	}
	else{
		printf("\n**Preemtive Priority**\n");
		result_num = 5;
	}
	Context_switch = 0;
	Time = 0;
	int io_count =0;

	init_Gantt();
	Queue PRIORITY;    //생성된 프로세스들을 저장한 queue를 받아서 저장한다.
	init_Queue(&PRIORITY, queue->size-1);
	copy_Queue(queue, &PRIORITY); 

	Queue ready_Q, waiting_Q, terminated_Q;
	init_Queue(&ready_Q, queue->size-1);
	init_Queue(&waiting_Q, queue->size-1);
	init_Queue(&terminated_Q, queue->size-1);

	PCB running_Q;
	running_Q = init_PCB();

	sort_by_arrival(&PRIORITY); // SJF에 있는 프로세스들을 도착시간 순으로 정렬한다
	
	while(is_Queue_full(&terminated_Q) != 1){ //terminated_Q가 꽉찰때 까지 time을 1씩 늘려가며 반복		
		if (Time == 200){
			printf("TIME:%d",Time);
			printf("[Ready_Q]\n");
			printf("front:%d, rear:%d, count:%d\n",ready_Q.front,ready_Q.rear,ready_Q.count);
			print_Queue(&ready_Q);
			printf("[Waiting_Q]\n");
			print_Queue(&waiting_Q);
		}		
	
		for(int i=PRIORITY.front+1; PRIORITY.buffer[i].pid!=0; i++){ //TIME에 도달할 때 ready_Q에 들어간다
			if(PRIORITY.buffer[i].arrive_time != Time) 
				break;
			else{
				enqueue(&ready_Q, get_Queue_head(&PRIORITY));
				dequeue(&PRIORITY);
			}
		}
		sort_by_priority(&ready_Q);
      
		if(waiting_Q.buffer[waiting_Q.front+1].pid!=0){ //waiting_Q에 프로세스가 존재한다면 io_time을 1줄인다.
			waiting_Q.buffer[waiting_Q.front+1].io_remain_time -= 1;
			
			if(waiting_Q.buffer[waiting_Q.front+1].io_remain_time ==0){ //I.O 입출력이 끝난다면, ready_q로 보낸다
				if (waiting_Q.buffer[waiting_Q.front+1].remain_time ==0){
					waiting_Q.buffer[waiting_Q.front+1].finish_time = Time;
					enqueue(&terminated_Q, waiting_Q.buffer[waiting_Q.front+1]);
					dequeue(&waiting_Q);
				}
				else{
					enqueue(&ready_Q, waiting_Q.buffer[waiting_Q.front+1]);
					dequeue(&waiting_Q);
					}
			}
		}
		sort_by_priority(&ready_Q);	
		
		if(running_Q.pid !=0){ //run_Q존재, ready_q도 존재
			running_Q.remain_time -= 1;
			running_Q.execute_time += 1;
			
			if(running_Q.remain_time <= 0 & running_Q.io_remain_time == 0){ //no more burst and no more i/o
				running_Q.finish_time = Time;
				enqueue(&terminated_Q,running_Q);
				running_Q = init_PCB();
			}
			
			if(running_Q.io_remain_time !=0 &&
					(running_Q.io_start_time == running_Q.remain_time)){ //만약 IO연산을 수행해야 한다면
				io_count += 1;
				enqueue(&waiting_Q, running_Q); //waiting_Q로 이동시킨다.
				running_Q = init_PCB();         //run_Q를 초기화
			}
			if(running_Q.pid != 0 && get_Queue_head(&ready_Q).pid!=0){ //만약 running_q가 아직 있고, ready_Q도 있다면
				if(running_Q.priority > get_Queue_head(&ready_Q).priority){ //ready_Q가 더 shortest_job이라면 비운다
					enqueue(&ready_Q,running_Q);
					running_Q = init_PCB();
				}
			}
		}

		if(running_Q.pid == 0 && get_Queue_head(&ready_Q).pid!=0){ //run_Q empty, is ready_Q : ready->run
			running_Q = get_Queue_head(&ready_Q);
			dequeue(&ready_Q);
			Context_switch+=1;
		}
		
		if(aging && Time % 10 == 0){
			for(int i=(ready_Q.front + 1) % ready_Q.size; (i % ready_Q.size)!=((ready_Q.rear+1) % ready_Q.size); i++){
				if(Time - ready_Q.buffer[i].execute_time - ready_Q.buffer[i].arrive_time  >=10)
					ready_Q.buffer[i].priority -= 2;
			}
			for(int i=(waiting_Q.front + 1) % waiting_Q.size; (i % waiting_Q.size)!=((waiting_Q.rear+1) % waiting_Q.size); i++){
				if(Time - waiting_Q.buffer[i].execute_time - waiting_Q.buffer[i].arrive_time >=10)
					waiting_Q.buffer[i].priority -= 2;
			}
		}

		Gantt[Time] = running_Q.pid;
		Time += 1;	
	}
	for(int i=terminated_Q.front+1; i<= terminated_Q.rear; i++){
		PCB temp;
		temp = terminated_Q.buffer[i];
		terminated_Q.buffer[i].waiting_time = temp.finish_time - temp.execute_time - temp.arrive_time;
	}
	print_Queue(&terminated_Q);
	print_Gantt(Time-1);

	result[result_num].process = queue->size-1;
	result[result_num].context_switch = Context_switch;
	for(int i=(terminated_Q.front+1); i<=terminated_Q.rear; i++){
		result[result_num].sum_wait += terminated_Q.buffer[i].waiting_time;
		result[result_num].sum_burst += terminated_Q.buffer[i].burst_time;
		result[result_num].sum_turn += terminated_Q.buffer[i].waiting_time + terminated_Q.buffer[i].burst_time;
	}

	result[result_num].avg_wait = (float)result[result_num].sum_wait / result[result_num].process;
	result[result_num].avg_burst = (float)result[result_num].sum_burst / result[result_num].process;
	result[result_num].avg_turn = (float)result[result_num].sum_turn / result[result_num].process;
	result[result_num].io_count = io_count;
}



/* if aging ->Multilevel Feedback Queue*/
void Multilevel_Queue(Queue *queue,int aging){
	int result_num;
	int time_quantum = 3;

	if(aging){
		printf("\n**Multilevel Feedback Queue**\n");
		result_num = 8;
	}
	else{
		printf("\n**Multilevel Queue**\n");
		result_num = 7;
	}
	Context_switch = 0;
	Time = 0;
	int io_count =0;

	for(int i=0; i<1000; i++){
		Gantt[i] = 0;
		Gantt2[i] = 0;
	}
	Queue RR;    //생성된 프로세스들을 저장한 queue를 받아서 저장한다.
	Queue FCFS;
	init_Queue(&RR, queue->size-1);
	init_Queue(&FCFS, queue->size-1);
	
	for(int i=queue->front+1; i<=queue->rear; i++){
		if(queue->buffer[i].priority < 6)  //priority가 5보다 작을 때
			enqueue(&RR, queue->buffer[i]);
		else
			enqueue(&FCFS, queue->buffer[i]);
	}

	Queue ready_Q, waiting_Q, terminated_Q;
	Queue ready_Q2, waiting_Q2;

	init_Queue(&ready_Q, queue->size-1);
	init_Queue(&waiting_Q, queue->size-1);
	init_Queue(&ready_Q2, queue->size-1);
	init_Queue(&waiting_Q2, queue->size-1);
	init_Queue(&terminated_Q, queue->size-1);

	PCB running_Q,running_Q2;
	running_Q = init_PCB();
	running_Q2 = init_PCB();

	sort_by_arrival(&RR); 
	sort_by_arrival(&FCFS);
	printf("[RR]\n");
	printf("front:%d rear:%d count:%d \n",RR.front, RR.rear, RR.count);
	print_Queue(&RR);
	printf("[FCFS]\n");
	printf("front:%d rear:%d count:%d \n",FCFS.front, FCFS.rear, FCFS.count);
	print_Queue(&FCFS);
	
	while(is_Queue_full(&terminated_Q) != 1){ //terminated_Q가 꽉찰때 까지 time을 1씩 늘려가며 반복		
		//RR에서 arrive_time에 따라서 ready_Q로 들어간다 
		for(int i=RR.front+1; RR.buffer[i].pid!=0; i++){ 
			if(RR.buffer[i].arrive_time != Time) 
				break;
			else{
				enqueue(&ready_Q, get_Queue_head(&RR));
				dequeue(&RR);
			}
		}
		//FCFS에서 arrive_time에 따라서 ready_Q2로 들어간다 
		for(int i=FCFS.front+1; FCFS.buffer[i].pid!=0; i++){ 
			if(FCFS.buffer[i].arrive_time != Time) 
				break;
			else{
				enqueue(&ready_Q2, get_Queue_head(&FCFS));
				dequeue(&FCFS);
			}
		}
		if(Time == 140)
			break;	
		
		if(waiting_Q.buffer[waiting_Q.front+1].pid!=0){ //waiting_Q에 프로세스가 존재한다면 io_time을 1줄인다.
			waiting_Q.buffer[waiting_Q.front+1].io_remain_time -= 1;
			
			if(waiting_Q.buffer[waiting_Q.front+1].io_remain_time ==0){ //I.O 입출력이 끝난다면, ready_q로 보낸다
				if (waiting_Q.buffer[waiting_Q.front+1].remain_time ==0){
						waiting_Q.buffer[waiting_Q.front+1].finish_time = Time;
					enqueue(&terminated_Q, waiting_Q.buffer[waiting_Q.front+1]);
					dequeue(&waiting_Q);
				}
				else{
					enqueue(&ready_Q, waiting_Q.buffer[waiting_Q.front+1]);
					dequeue(&waiting_Q);
				}
			}
		}


		/* 70%의 시간이 RR에 할당된다 */
		if(Time % 10 < 7){
				
			if(running_Q.pid !=0){ //run_Q존재, ready_q도 존재
				running_Q.remain_time -= 1;

				running_Q.execute_time += 1;
			
				if(running_Q.remain_time <= 0 & running_Q.io_remain_time == 0){ //no more burst and no more i/o
					running_Q.finish_time = Time;
					enqueue(&terminated_Q,running_Q);
					running_Q = init_PCB();
				}
			
				if(running_Q.io_remain_time !=0 &&
						(running_Q.io_start_time == running_Q.remain_time)){ //만약 IO연산을 수행해야 한다면
					io_count += 1;
					enqueue(&waiting_Q, running_Q); //waiting_Q로 이동시킨다.
					running_Q = init_PCB();         //run_Q를 초기화
				}
			
				if(running_Q.pid!=0 && (running_Q.execute_time % time_quantum) ==0){
					if (running_Q.execute_time>0){
						enqueue(&ready_Q,running_Q);
						running_Q = init_PCB();
					}
				}
			}

			if(running_Q.pid == 0 && get_Queue_head(&ready_Q).pid!=0){ //run_Q empty, is ready_Q : ready->run
				running_Q = get_Queue_head(&ready_Q);
				dequeue(&ready_Q);
				Context_switch+=1;
			}

			Gantt[Time] = running_Q.pid;	
		}


		if(waiting_Q2.buffer[waiting_Q2.front+1].pid!=0){ //waiting_Q에 프로세스가 존재한다면 io_time을 1줄인다.
			waiting_Q2.buffer[waiting_Q2.front+1].io_remain_time -= 1;
			
			if(waiting_Q2.buffer[waiting_Q2.front+1].io_remain_time ==0){ //I.O 입출력이 끝난다면, ready_q로 보낸다
				if (waiting_Q2.buffer[waiting_Q2.front+1].remain_time ==0){
					//waiting_Q.buffer[waiting_Q.front+1].arrive_time = Time;
					waiting_Q2.buffer[waiting_Q2.front+1].finish_time = Time;
					enqueue(&terminated_Q, waiting_Q2.buffer[waiting_Q2.front+1]);
					dequeue(&waiting_Q2);
				}
				else{
					//waiting_Q.buffer[waiting_Q.front+1].arrive_time = Time;  //Time으로 도착시간을 변경한뒤 enqueue
					enqueue(&ready_Q2, waiting_Q2.buffer[waiting_Q2.front+1]);
					dequeue(&waiting_Q2);
				}
			}
		}
		/*30%의 시간이 FCFS에 할당*/
		if(Time % 10 >=7){

			if(running_Q2.pid !=0){ //run_Q존재, ready_q도 존재
				running_Q2.remain_time -= 1;
				running_Q2.execute_time += 1;
			
				if(running_Q2.remain_time <= 0 & running_Q2.io_remain_time == 0){ //no more burst and no more i/o
					running_Q2.finish_time = Time;
					enqueue(&terminated_Q,running_Q2);
					running_Q2 = init_PCB();
				}
			
				if(running_Q2.io_remain_time !=0 &&
						(running_Q2.io_start_time == running_Q2.remain_time)){ //만약 IO연산을 수행해야 한다면
					io_count += 1;
					enqueue(&waiting_Q2, running_Q2); //waiting_Q로 이동시킨다.
					running_Q2 = init_PCB();         //run_Q를 초기화
				}
			}
		
			if(running_Q2.pid == 0 && get_Queue_head(&ready_Q2).pid!=0){ //run_Q empty, is ready_Q : ready->run
				running_Q2 = get_Queue_head(&ready_Q2);
				dequeue(&ready_Q2);
				Context_switch+=1;
			}
		
			if(aging && (Time % 9 == 0)){
				for(int i=(ready_Q2.front + 1) % ready_Q2.size; (i % ready_Q2.size)!=((ready_Q2.rear+1) % ready_Q2.size); i++){
					if(Time - ready_Q2.buffer[i].execute_time - ready_Q2.buffer[i].arrive_time  >=10)
						ready_Q2.buffer[i].priority -= 3;
						if(ready_Q2.buffer[i].priority < 6){
							enqueue(&ready_Q,ready_Q2.buffer[i]);
							dequeue(&ready_Q2);
						}
				}
				for(int i=(waiting_Q2.front + 1) % ready_Q2.size;(i % waiting_Q2.size)!=((waiting_Q2.rear+1) % waiting_Q2.size); i++){
					if(Time - waiting_Q2.buffer[i].execute_time - waiting_Q2.buffer[i].arrive_time >=10){
						waiting_Q2.buffer[i].priority -= 3;
						if(waiting_Q2.buffer[i].priority < 6){
							enqueue(&waiting_Q,waiting_Q2.buffer[i]);
							dequeue(&waiting_Q2);
						}
					}
				}
			}	
		Gantt2[Time] = running_Q2.pid;
		}
		Time += 1;
	}

	for(int i=terminated_Q.front+1; i<= terminated_Q.rear; i++){
		PCB temp;
		temp = terminated_Q.buffer[i];
		terminated_Q.buffer[i].waiting_time = temp.finish_time - temp.execute_time - temp.arrive_time;
	}
	print_Queue(&terminated_Q);
	print_Gantt(Time-1);
	for(int i=0; i<Time; i++) //Gantt2출력을 위해 대체
		Gantt[i] = Gantt2[i];
	print_Gantt(Time-1);

	result[result_num].process = queue->size-1;
	result[result_num].context_switch = Context_switch;
	for(int i=(terminated_Q.front+1); i<=terminated_Q.rear; i++){
		result[result_num].sum_wait += terminated_Q.buffer[i].waiting_time;
		result[result_num].sum_burst += terminated_Q.buffer[i].burst_time;
		result[result_num].sum_turn += terminated_Q.buffer[i].waiting_time + terminated_Q.buffer[i].burst_time;
	}
	result[result_num].avg_wait = (float)result[result_num].sum_wait / result[result_num].process;
	result[result_num].avg_burst = (float)result[result_num].sum_burst / result[result_num].process;
	result[result_num].avg_turn = (float)result[result_num].sum_turn / result[result_num].process;
	result[result_num].io_count = io_count;
}


void Realtime(Queue *queue, int edf){
	int result_num;

	Context_switch = 0;
	Time = 0;
	int io_count =0;

	init_Gantt();
	Queue PRIORITY;    //생성된 프로세스들을 저장한 queue를 받아서 저장한다.
	init_Queue(&PRIORITY, queue->size-1);
	copy_Queue(queue, &PRIORITY); 

	Queue ready_Q, waiting_Q, terminated_Q;
	init_Queue(&ready_Q, 5 * (queue->size-1));
	init_Queue(&waiting_Q,5 * ( queue->size-1));
	init_Queue(&terminated_Q, 5 *(queue->size-1));

	PCB running_Q;
	running_Q = init_PCB();
	
	if(edf ==0){ //rate_monotonic
		result_num = 9;
		printf("\n*******RATE MONOTONIC********\n");
	}
	else{  //earliest deadline first
		result_num = 10;
		printf("\n**EARLIEST DEADLINE FIRST****\n");
	
	}
	for(int i=PRIORITY.front+1; PRIORITY.buffer[i].pid!=0; i++){
		PRIORITY.buffer[i].priority = PRIORITY.buffer[i].period; 
		PRIORITY.buffer[i].arrive_time = 0;
	}
	for(int i=PRIORITY.rear+1; i<PRIORITY.size; i++)
		PRIORITY.buffer[i].pid = 0;

	sort_by_priority(&PRIORITY); // SJF에 있는 프로세스들을 도착시간 순으로 정렬한다
	print_Queue(&PRIORITY);

	while(is_Queue_full(&terminated_Q) != 1){ //terminated_Q가 꽉찰때 까지 time을 1씩 늘려가며 반복		
		if(edf == 1){
			for(int i=(ready_Q.front+1)%ready_Q.size; i <= ready_Q.rear; i++)
				ready_Q.buffer[i].priority -= 1;
			running_Q.priority -= 1;
			for(int i=waiting_Q.front+1; i <= waiting_Q.rear; i++)
				waiting_Q.buffer[i].priority -= 1;
		}
		
		//25마다 period를 체크해서 period인데 ready_Q에 남아있다면, deadline miss이다
		if(Time == 201)
			break;	
		for(int i=(PRIORITY.front+1); i != PRIORITY.rear+1; i++){ //TIME에 도달할 때 ready_Q에 들어간다
			if((Time % PRIORITY.buffer[i].period) == 0) {
				int cnt =0;
				for(int j=ready_Q.front+1; j<=ready_Q.rear; j++){
					if(ready_Q.buffer[j].pid == PRIORITY.buffer[i].pid)
						printf("Time [%d], [%d] deadline miss!!\n",Time,PRIORITY.buffer[i].pid);
				}
				PRIORITY.buffer[i].arrive_time = Time;
				enqueue(&ready_Q, PRIORITY.buffer[i]);
			}
		}
		sort_by_priority(&ready_Q);
      
		if(waiting_Q.buffer[waiting_Q.front+1].pid!=0){ //waiting_Q에 프로세스가 존재한다면 io_time을 1줄인다.
			waiting_Q.buffer[waiting_Q.front+1].io_remain_time -= 1;
			
			if(waiting_Q.buffer[waiting_Q.front+1].io_remain_time ==0){ //I.O 입출력이 끝난다면, ready_q로 보낸다
				if (waiting_Q.buffer[waiting_Q.front+1].remain_time ==0){
					waiting_Q.buffer[waiting_Q.front+1].finish_time = Time;
					enqueue(&terminated_Q, waiting_Q.buffer[waiting_Q.front+1]);
					dequeue(&waiting_Q);
				}
				else{
					enqueue(&ready_Q, waiting_Q.buffer[waiting_Q.front+1]);
					dequeue(&waiting_Q);
					}
			}
		}
		sort_by_priority(&ready_Q);	
		
		if(running_Q.pid !=0){ //run_Q존재, ready_q도 존재
			running_Q.remain_time -= 1;
			running_Q.execute_time += 1;
			
			if(running_Q.remain_time <= 0 & running_Q.io_remain_time == 0){ //no more burst and no more i/o
				running_Q.finish_time = Time;
				enqueue(&terminated_Q,running_Q);
				running_Q = init_PCB();
			}
			
			if(running_Q.io_remain_time !=0 &&
					(running_Q.io_start_time == running_Q.remain_time)){ //만약 IO연산을 수행해야 한다면
				io_count += 1;
				enqueue(&waiting_Q, running_Q); //waiting_Q로 이동시킨다.
				running_Q = init_PCB();         //run_Q를 초기화
			}
			if(running_Q.pid != 0 && get_Queue_head(&ready_Q).pid!=0){ //만약 running_q가 아직 있고, ready_Q도 있다면
				if(running_Q.priority > get_Queue_head(&ready_Q).priority){ //ready_Q가 더 shortest_job이라면 비운다
					enqueue(&ready_Q,running_Q);
					running_Q = init_PCB();
				}
			}
		}

		if(running_Q.pid == 0 && get_Queue_head(&ready_Q).pid!=0){ //run_Q empty, is ready_Q : ready->run
			running_Q = get_Queue_head(&ready_Q);
			dequeue(&ready_Q);
			Context_switch+=1;
		}
		Gantt[Time] = running_Q.pid;
		Time += 1;	
	}
	for(int i=terminated_Q.front+1; i<= terminated_Q.rear; i++){
		PCB temp;
		temp = terminated_Q.buffer[i];
		terminated_Q.buffer[i].waiting_time = temp.finish_time - temp.execute_time - temp.arrive_time;
	}
	print_Queue(&terminated_Q);
	print_Gantt(Time-1);

	result[result_num].process = queue->size-1;
	result[result_num].context_switch = Context_switch;
	for(int i=(terminated_Q.front+1); i<=terminated_Q.rear; i++){
		result[result_num].sum_wait += terminated_Q.buffer[i].waiting_time;
		result[result_num].sum_burst += terminated_Q.buffer[i].burst_time;
		result[result_num].sum_turn += terminated_Q.buffer[i].waiting_time + terminated_Q.buffer[i].burst_time;
	}
	result[result_num].avg_wait = (float)result[result_num].sum_wait / result[result_num].process;
	result[result_num].avg_burst = (float)result[result_num].sum_burst / result[result_num].process;
	result[result_num].avg_turn = (float)result[result_num].sum_turn / result[result_num].process;
	result[result_num].io_count = io_count;
}




int main(void){
	printf("************2014190701 이호준************\n");
	printf("***************CPU SIMULATOR**************");
	
	while(1){
		int p_num;
		int num;
		Queue ready_Q;
		printf("\n\n[MENU}\n");
		printf("0. 프로세스 생성\n");
		printf("1. FCFS 실행\n");
		printf("2. Nonpreemptive SJF 실행\n");
		printf("3. Nonpreemptive_Priority 실행\n");
		printf("4. Round Robin 실행\n");
		printf("5. Preemptive SJF 실행\n");
		printf("6. Preemptive Priority실행\n");
		printf("7. Preemptive Priority(aging)실행\n");
		printf("8. Multilevel Queue 실행\n");
		printf("9. Multilevel Feedback Queue실행\n");
		printf("10. Rate Monotonic 실행\n");
		printf("11. Earliest Deadline First실행\n");
				
		printf("12. 전체 알고리즘 평가\n");
		printf("13. 종료\n");
	
		printf("수행 하고 싶은 명령을 선택하세요:\n ");
		scanf("%d",&num);
		
		switch(num){
			case(0):
				initialize_result();
				p_num = process_num();
				init_Queue(&ready_Q,p_num);
				fill_random_process(&ready_Q);
				printf("***생성된 큐 출력***\n");
				print_Queue(&ready_Q);
				printf("[도착 순서대로 보기]\n");
				sort_by_arrival(&ready_Q);
				print_Queue(&ready_Q);
				break;
			case(1):
				FCFS(&ready_Q);
				break;
			case(2):
				Nonpreemptive_SJF(&ready_Q);
				break;
			case(3):
				Nonpreemptive_Priority(&ready_Q);
				break;
			case(4):
				Round_Robin(&ready_Q);
				break;
			case(5):
				Preemptive_SJF(&ready_Q);
				break;
			case(6):
				Preemptive_Priority(&ready_Q,0);
				break;
			case(7):
				Preemptive_Priority(&ready_Q,1);
				break;
			case(8):
				Multilevel_Queue(&ready_Q,0);
				break;
			case(9):
				Multilevel_Queue(&ready_Q,1);
				break;
			case(10):
				Realtime(&ready_Q,0);
				break;
			case(11):
				Realtime(&ready_Q,1);
				break;
			case(12):
				FCFS(&ready_Q);
				Nonpreemptive_SJF(&ready_Q);
				Nonpreemptive_Priority(&ready_Q);
				Round_Robin(&ready_Q);
				Preemptive_SJF(&ready_Q);
				Preemptive_Priority(&ready_Q,0);
				Preemptive_Priority(&ready_Q,1);
				Multilevel_Queue(&ready_Q,0);
				Multilevel_Queue(&ready_Q,1);
				Realtime(&ready_Q,0);
				Realtime(&ready_Q,1);
				print_all_result();
				break;
			case(13):
				return 0;
			default:
				printf("잘못된 선택입니다. 다시 입력해주세요!\n");
				break;
		}
	}

	return 0;
}






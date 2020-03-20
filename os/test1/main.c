#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>

#include "Deque.h"
#include "PriorityQueue.h"

#define PROCESS_NUM 10
#define MAX_CPUBURST 100
#define MAX_ARRIVETIME 20
#define IO 10
#define MAX_IO 50
#define PREEMPTIVE 10

#define QUANTUM 5
#define BUF_SIZE 10

/* ******* 선택 ******** */
#define RR // RR SJF 선택

int DataPriorityComp(PCB * a, PCB * b)
{
  return b->arrive_time - a->arrive_time;
}

enum {READY, RUN, WAIT, TERMINATE};
void read_childproc(int sig);

int main(void)
{
  int i, j, fds[PROCESS_NUM][2], cpuburst;
  //int fds[2];
  static int t, context_switch, sum_burst, sum_io;
  PCB pcb[PROCESS_NUM];
  PCB * ptr = NULL, ** ptr1 = NULL, * ptr2 = NULL, * temp = NULL;
  void * nptr = NULL;

  /*
  char str2[] = "I";
  char str3[] = "N";
  */

  char buf[BUF_SIZE];
  int pn[100000];
  pid_t pid = -1;
  static Deque deque;
  static Deque rq;
  static PQueue pq;
  for(i=0; i<PROCESS_NUM; i++)
    pipe(fds[i]);
  //pipe(fds);

  FILE * fp = fopen("os.dump", "wt");
  FILE * fp1 = fopen("log.dump", "wt");
  
	struct sigaction act;
	act.sa_handler=read_childproc;
	sigemptyset(&act.sa_mask);
	act.sa_flags=0;
	sigaction(SIGCHLD, &act, 0);
  //struct sigaction sa;
  
  struct itimerval timer;
  //sa.sa_handler = &read_childproc;
  
  sigaction(SIGVTALRM, &act, 0);

  timer.it_value.tv_sec = 1; // 초 단위
  timer.it_value.tv_usec = 0; // microsec
  timer.it_interval = timer.it_value;
  
  setitimer(ITIMER_VIRTUAL, &timer, 0);

  DequeInit(&deque);
  DequeInit(&rq);
  PQueueInit(&pq, DataPriorityComp);
  srand(time(NULL));
  
  for(i=0; i<PROCESS_NUM; i++)
  {
    pid = fork();
    if(pid) // parnet process
    {
      //pipe(fds[pid]);
      pcb[i].pid = pid;
      cpuburst = rand() % MAX_CPUBURST + 1;
      pcb[i].state = READY;
      pcb[i].cpu_burst = pcb[i].remain_time = cpuburst;
      pcb[i].arrive_time = rand() % MAX_ARRIVETIME;
      pcb[i].waiting_time = pcb[i].execute_time = pcb[i].finish_time = 0;
      pcb[i].io = rand() % IO;
      pcb[i].io_time = rand() % MAX_IO + 1;
      pcb[i].preemptive = rand() % PREEMPTIVE;
      PEnqueue(&pq, &(pcb[i]));
    }
    else
      break;
  }

  // Sorting
  if(pid) // parent process
  {
    for(i=0; i<PROCESS_NUM; i++)
    {
      temp = PDequeue(&pq);
      sum_burst += temp->cpu_burst;
      if(!temp->io)
        sum_io += temp->io_time;
      //pn[(temp->pid)] = i;
      DQAddLast(&deque, temp);
     }
  }

  if(!pid) // child process
  {
    int temp = pn[getpid()];
    read(fds[temp][0], (void*)buf, 8);
    //read(fds[0], (void*)buf, 8);
    ptr1 = (PCB**)buf;
    ptr2 = *ptr1;
    while(1)
    {
      if(ptr2->remain_time == 0)
        break;
    }

    if(!(ptr2->io))
    {
      ptr2->state = WAIT;
      //while(t-(ptr1->execute_time)<ptr1->io_time){};
      
      ptr2->state = TERMINATE;
      exit(2);
    }

    ptr2->state = TERMINATE;
    exit(3);
  } 
  else // parent process
  {
    while(1)
    { 
      if(DQIsEmpty(&deque))
        break;
      ptr = DQRemoveFirst(&deque);
      write(fds[pn[ptr->pid]][1], &ptr, 8);
      //write(fds[1], &ptr, 8);

      ptr->state = RUN;

      while(1)
      {
        #ifdef RR
        if(ptr->remain_time < QUANTUM)
          j = ptr->remain_time;
        else
          j = QUANTUM;
        #endif

        #ifdef SJF
        j = ptr->remain_time;
        #endif

        if(!(ptr->preemptive))
          j = ptr -> remain_time; // ptr is preemptive
        else if(!DQIsEmpty(&deque))
        {
          temp = DQGetFirst(&deque);
          if(!(temp->preemptive) && (t+j > temp->arrive_time))
            j = temp->arrive_time - t;

          #ifdef SJF
          if((ptr->remain_time) - (temp->arrive_time) + t > temp->remain_time)
            if(temp->arrive_time - t > 0)
              j = temp->arrive_time - t;
          #endif
        }

        t += j;
        ptr->execute_time += j;
        ptr->remain_time -= j;
        fprintf(fp1, "%d %d %d\n", context_switch, ptr->pid, ptr->remain_time);
        if(!(ptr->remain_time))
        {
          ptr->finish_time = t;
          ptr->waiting_time = t - (ptr->cpu_burst);
          if(!ptr->io && DQIsEmpty(&deque))
            t += ptr->io_time;
          ptr->state = TERMINATE;
          DQAddLast(&rq, ptr);
          fprintf(fp, "%d %d %d %d %d %d %d %d %d %d %d\n", ptr->pid, ptr->state, ptr->cpu_burst, ptr->arrive_time, ptr->waiting_time, ptr->execute_time, ptr->remain_time, ptr->finish_time, ptr->io, ptr->io_time, ptr->preemptive);
          break;
        }

        if(!DQIsEmpty(&deque) && DQGetFirst(&deque)->arrive_time <= t)
        {
          ptr->state = READY;
          DQAddLast(&deque, ptr);
          break;
        }
      }

      context_switch++;
    }
  }

  j = 0;
  for(i=0; i<PROCESS_NUM; i++)
    j += DQRemoveFirst(&rq)->waiting_time;
  sleep(1);

  puts("\n*****Summary of all Results*****");
  puts("-------------------------------");
  puts("| Number of Process |");
  printf("   %d\n\n",PROCESS_NUM);
  puts("| Context_Switch |");
  printf("   %d\n\n", context_switch);
  puts("| Sum of Executing time |");
  printf("   %d\n\n", t);
  /*
  puts("| Sum of Waiting time |");
  printf("   %d\n\n", j); 
  */
  puts("| Sum of Burst time |");
  printf("   %d\n\n", sum_burst);
  puts("| Sum of IO time |");
  printf("   %d\n\n", sum_io);
  /*
  puts("| Average of Burst Time |");
  printf("   %.3f\n\n", (double)sum_burst/PROCESS_NUM);
  */
  puts("| Average of Waiting Time |");
  printf("   %.3f\n\n", (double)j/PROCESS_NUM);
  puts("-------------------------------");

  fclose(fp);
  fclose(fp1);
  while(1){};
}

void read_childproc(int sig)
{
  static int where;
  if(sig == SIGCHLD)
  {
    int status;
    pid_t id=waitpid(-1, &status, WNOHANG);

    if(WIFEXITED(status))
    {
      printf("Removed proc id: %d \n", id);
      printf("Child send: %d \n", WEXITSTATUS(status));
    }
  }
  else if(sig == SIGVTALRM) 
  {
    static int count;
    static int arr[3];
    FILE * fp2 = fopen("log.dump", "rt");

    fseek(fp2, where, SEEK_SET);
    fscanf(fp2, "%d %d %d\n", &arr[0], &arr[1], &arr[2]);

    where = ftell(fp2);
    printf("%d %d %d\n", arr[0], arr[1], arr[2]);
    if(feof(fp2))
      exit(5);
    fclose(fp2);
  }
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

/*
void timer_handling(int sig)
{
  static int count;
  static int arr[12];
  fscanf(fp1, "%d %d %d %d %d %d %d %d %d %d %d %d", &arr[0], &arr[1], &arr[2], &arr[3], &arr[4], &arr[5], &arr[6], &arr[7], &arr[8], &arr[9], &arr[10], &arr[11]);
  printf("%d %d %d %d %d %d %d %d %d %d %d %d\n", arr[0], arr[1], arr[2], arr[3], arr[4], arr[5], arr[6], arr[7], arr[8], arr[9], arr[10], arr[11]);
}
*/

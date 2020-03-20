#ifndef __PCB_H__
#define __PCB_H__

typedef struct
{
  int pid;
  int state;
  int cpu_burst;
  int arrive_time;
  int waiting_time;
  int execute_time;
  int remain_time;
  int finish_time;
  int io;
  int io_time;
  int preemptive;
} PCB;

#endif

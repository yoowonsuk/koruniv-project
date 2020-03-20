#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

void read_childproc(int sig){
	int status;
	pid_t id=waitpid(-1, &status, WNOHANG);
	if(WIFEXITED(status))
		printf("Removed proc id: %d \n", id);
}

int main(int argc, char *argv[]){
  int i;
	pid_t pid;
	struct sigaction act;
	act.sa_handler=read_childproc;
	sigemptyset(&act.sa_mask);
	act.sa_flags=0;
//	sigaction(SIGCHLD, &act, 0);

  for(i=0; i<10; i++)  {
  	pid=fork();
    if(pid == 0)
      break;
  }
	if(pid==0)	{
		return 12;
	}
	return 0;
}

#include <unistd.h>
#include <stdio.h>

int main(void)
{
  int pid = fork();

  if(pid)
    printf("%d\n", getpid());
  else
    printf("child %d\n", getpid());

  return 0;
}

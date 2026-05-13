#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(int argc, char **argv)
{
  int n = 3;
  int xst;

  if(argc > 1) n = atoi(argv[1]);
  if(n < 1) n = 1;

  printf("logdemo: starting %d child processes\n", n);

  for(int i = 0; i < n; i++){
    int pid = fork();
    if(pid < 0){
      fprintf(2, "logdemo: fork failed\n");
      exit(1);
    }
    if(pid == 0){
      int fd = open("README", O_RDONLY);
      char buf[32];
      if(fd >= 0){
        read(fd, buf, sizeof(buf));
        close(fd);
      }
      pause(1);
      exit(i);
    }
  }

  for(int i = 0; i < n; i++) wait(&xst);

  printf("logdemo: done\n");
  exit(0);
}

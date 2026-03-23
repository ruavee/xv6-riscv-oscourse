#include "kernel/types.h"
#include "user/user.h"

static void
emit_args(int argc, char **argv, int use_mutex, int mfd)
{
  for(int i = 1; i < argc; i++){
    for(int j = 0; argv[i][j] != '\0'; j++){
      if(use_mutex && mutex_lock(mfd) < 0){
        fprintf(2, "mutexdemo: mutex_lock failed\n");
        exit(1);
      }

      printf("%d: arg %d, char '%c'\n", getpid(), i - 1, argv[i][j]);

      if(use_mutex && mutex_unlock(mfd) < 0){
        fprintf(2, "mutexdemo: mutex_unlock failed\n");
        exit(1);
      }

      pause(1);
    }
  }
}

int
main(int argc, char **argv)
{
  int pid, status;
  int mfd;

  if(argc < 2){
    fprintf(2, "usage: mutexdemo arg ...\n");
    exit(1);
  }

  printf("without mutex\n");
  pid = fork();
  if(pid < 0){
    fprintf(2, "mutexdemo: fork failed\n");
    exit(1);
  }
  if(pid == 0){
    emit_args(argc, argv, 0, -1);
    exit(0);
  }
  emit_args(argc, argv, 0, -1);
  wait(&status);

  mfd = mutex();
  if(mfd < 0){
    fprintf(2, "mutexdemo: mutex failed\n");
    exit(1);
  }

  printf("with mutex\n");
  pid = fork();
  if(pid < 0){
    fprintf(2, "mutexdemo: fork failed\n");
    close(mfd);
    exit(1);
  }
  if(pid == 0){
    emit_args(argc, argv, 1, mfd);
    close(mfd);
    exit(0);
  }
  emit_args(argc, argv, 1, mfd);
  wait(&status);

  close(mfd);
  exit(0);
}

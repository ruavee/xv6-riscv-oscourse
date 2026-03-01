#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define TICKS_PER_SEC 10

static void
die(const char *msg)
{
  fprintf(2, "%s\n", msg);
  exit(1);
}

static void
run_mode(int do_kill)
{
  int pid = fork();
  if (pid < 0) die("fork failed");

  if (pid == 0) {
    int sec = 10;
    pause(sec * TICKS_PER_SEC);
    exit(1);
  }

  printf("parent: pid=%d, child pid=%d\n", getpid(), pid);

  if (do_kill) {
    printf("parent: sending kill to child %d\n", pid);
    if(kill(pid) < 0)
      fprintf(2, "parent: kill(%d) failed\n", pid);
  }

  int st = 0;
  int wpid = wait(&st);
  if (wpid < 0) die("wait failed");

  printf("parent: child pid=%d finished, status=%d\n", wpid, st);
}

int
main(int argc, char *argv[])
{
  if (argc != 2) {
    fprintf(2, "usage: task1 a|b\n");
    exit(1);
  }

  if (argv[1][0] == 'a') {
    run_mode(0);
  } else if (argv[1][0] == 'b') {
    run_mode(1);
  } else {
    die("unknown mode (use a or b)");
  }

  exit(0);
}
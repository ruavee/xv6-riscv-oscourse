#include "kernel/types.h"
#include "kernel/procinfo.h"
#include "user/user.h"

int ps_listinfo(struct procinfo *, int);

static void
check(int cond, char *msg)
{
  if(!cond) {
    fprintf(2, "ps_test: %s\n", msg);
    exit(1);
  }
}

static int
contains_pid(struct procinfo *list, int n, int pid)
{
  for(int i = 0; i < n; i++) {
    if(list[i].pid == pid) return 1;
  }
  return 0;
}

int
main(void)
{
  int count;
  int pid;
  struct procinfo one[1];
  struct procinfo *list;
  int n;
  int child;

  count = ps_listinfo(0, 0);
  check(count > 0, "NULL plist must return positive process count");

  pid = getpid();
  check(ps_listinfo(one, 1) > 1, "small buffer must report required size");

  check(ps_listinfo((struct procinfo *)0xffffffffffffffffL, 1) < 0,
        "bad user pointer must fail");

  child = fork();
  check(child >= 0, "fork failed");
  if(child == 0) {
    pause(200);
    exit(0);
  }

  list = malloc(sizeof(*list) * (count + 8));
  check(list != 0, "malloc failed");

  n = ps_listinfo(list, count + 8);
  check(n > 0, "ps_listinfo failed on valid buffer");
  check(n <= count + 8, "valid buffer unexpectedly too small");
  check(contains_pid(list, n, pid), "current process missing from list");
  check(contains_pid(list, n, child), "child process missing from list");

  free(list);
  wait(0);

  printf("ps_test: OK\n");
  exit(0);
}

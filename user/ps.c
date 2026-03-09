#include "kernel/types.h"
#include "kernel/procinfo.h"
#include "user/user.h"

int ps_listinfo(struct procinfo *, int);

static char *
state_name(int state)
{
  switch(state) {
  case UNUSED:
    return "UNUSED";
  case USED:
    return "USED";
  case SLEEPING:
    return "SLEEPING";
  case RUNNABLE:
    return "RUNNABLE";
  case RUNNING:
    return "RUNNING";
  case ZOMBIE:
    return "ZOMBIE";
  default:
    return "?";
  }
}

static char *
parent_name(struct procinfo *list, int n, int ppid)
{
  int i;
  if(ppid == 0) return "-";
  for(i = 0; i < n; i++) {
    if(list[i].pid == ppid)
      return list[i].name;
  }
  return "?";
}

int
main(void)
{
  struct procinfo *list;
  int lim;
  int n;

  lim = 8;
  list = 0;

  for(;;) {
    struct procinfo *new_list;

    new_list = malloc(sizeof(*new_list) * lim);
    if(new_list == 0) {
      fprintf(2, "ps: malloc failed\n");
      if(list)
        free(list);
      exit(1);
    }
    if(list)
      free(list);
    list = new_list;

    n = ps_listinfo(list, lim);
    if(n < 0) {
      fprintf(2, "ps: ps_listinfo failed\n");
      free(list);
      exit(1);
    }
    if(n <= lim)
      break;
    lim = n;
  }

  printf("pid\tname\tstate\tppid\tpname\n");
  for(int i = 0; i < n; i++) {
    printf("%d\t%s\t%s\t%d\t%s\n",
           list[i].pid,
           list[i].name,
           state_name(list[i].state),
           list[i].ppid,
           parent_name(list, n, list[i].ppid));
  }

  free(list);
  exit(0);
}

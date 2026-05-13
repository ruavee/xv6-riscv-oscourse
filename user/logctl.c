#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/diag.h"
#include "user/user.h"

static int
isnum(const char *s)
{
  if(s == 0 || *s == 0) return 0;
  for(; *s; s++)
    if(*s < '0' || *s > '9') return 0;
  return 1;
}

static int
classmask(const char *s)
{
  if(strcmp(s, "all") == 0)
    return LOG_ALL;
  if(strcmp(s, "sys") == 0 || strcmp(s, "syscall") == 0 || strcmp(s, "syscalls") == 0)
    return LOG_SYSCALL;
  if(strcmp(s, "intr") == 0 || strcmp(s, "interrupt") == 0 || strcmp(s, "interrupts") == 0)
    return LOG_INTR;
  if(strcmp(s, "proc") == 0 || strcmp(s, "process") == 0 || strcmp(s, "processes") == 0)
    return LOG_PROC;
  if(strcmp(s, "exec") == 0)
    return LOG_EXEC;
  return 0;
}

static void
printmask(int mask)
{
  printf("logging:");
  if(mask == 0){
    printf(" off\n");
    return;
  }
  if(mask & LOG_SYSCALL) printf(" syscall");
  if(mask & LOG_INTR)    printf(" intr");
  if(mask & LOG_PROC)    printf(" proc");
  if(mask & LOG_EXEC)    printf(" exec");
  printf("\n");
}

static void
usage(void)
{
  fprintf(2, "usage:\n");
  fprintf(2, "  logctl\n");
  fprintf(2, "  logctl on  <all|syscall|intr|proc|exec>... [ticks]\n");
  fprintf(2, "  logctl off [all|syscall|intr|proc|exec]...\n");
  exit(1);
}

int
main(int argc, char **argv)
{
  int enable = 0, mask = 0, duration = 0, last;

  if(argc == 1){
    printmask(logctl(0, -1, 0));
    exit(0);
  }

  if(strcmp(argv[1], "on") == 0) enable = 1;
  else if(strcmp(argv[1], "off") == 0) enable = 0;
  else usage();

  last = argc;
  if(enable && argc > 2 && isnum(argv[argc-1])){
    duration = atoi(argv[argc-1]);
    last--;
  }

  if(last == 2) mask = LOG_ALL;
  else {
    for(int i = 2; i < last; i++){
      int m = classmask(argv[i]);
      if(m == 0) usage();
      mask |= m;
    }
  }

  printmask(logctl(mask, enable, duration));
  exit(0);
}

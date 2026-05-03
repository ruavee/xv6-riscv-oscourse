#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

static char buf[DMSG_BUF_SIZE + 1];

int
main(int argc, char **argv)
{
  int n;

  if(argc != 1){
    fprintf(2, "usage: dmesg\n");
    exit(1);
  }

  n = dmesg(buf, sizeof(buf));
  if(n < 0){
    fprintf(2, "dmesg: syscall failed\n");
    exit(1);
  }

  printf("%s", buf);
  exit(0);
}

#include "types.h"
#include "riscv.h"
#include "defs.h"

uint64
sys_addab(void)
{
  int a = 0, b = 0;

  argint(0, &a);
  argint(1, &b);

  long long s = (long long)a + (long long)b;
  return (uint64)s;
}

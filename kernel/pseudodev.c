#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"

struct {
  struct spinlock seed_lock;
  struct spinlock nullstat_lock;
  uint64 seed;
  uint64 nullstat_bytes;
} pseudodev_state;

static char zerobuf[64] = {0};

static int
pseudoread(int minor, int user_dst, uint64 dst, int n)
{
  int i;
  uint64 value;
  char buf[64];

  switch(minor){
  case PSEUDO_NULL:
    return 0;
  case PSEUDO_ZERO:
    for(i = 0; i < n; ){
      int chunk = n - i;
      if(chunk > sizeof(zerobuf))
        chunk = sizeof(zerobuf);
      if(either_copyout(user_dst, dst + i, zerobuf, chunk) < 0)
        return -1;
      i += chunk;
    }
    return n;
  case PSEUDO_URANDOM:
    for(i = 0; i < n; ){
      int j, chunk = n - i;
      uint64 seed;

      if(chunk > sizeof(buf))
        chunk = sizeof(buf);
      acquire(&pseudodev_state.seed_lock);
      seed = pseudodev_state.seed;
      for(j = 0; j < chunk; j++){
        seed = seed * 6364136223846793005ULL + 1;
        buf[j] = seed >> 32;
      }
      pseudodev_state.seed = seed;
      release(&pseudodev_state.seed_lock);
      if(either_copyout(user_dst, dst + i, buf, chunk) < 0)
        return -1;
      i += chunk;
    }
    return n;
  case PSEUDO_NULLSTAT:
    if(n != sizeof(uint64))
      return -1;
    acquire(&pseudodev_state.nullstat_lock);
    value = pseudodev_state.nullstat_bytes;
    release(&pseudodev_state.nullstat_lock);
    if(either_copyout(user_dst, dst, (char *)&value, sizeof(value)) < 0)
      return -1;
    return sizeof(value);
  default:
    return -1;
  }
}

static int
pseudowrite(int minor, int user_src, uint64 src, int n)
{
  uint64 value;

  switch(minor){
  case PSEUDO_NULL:
    return n;
  case PSEUDO_ZERO:
    return -1;
  case PSEUDO_URANDOM:
    if(n != sizeof(uint64))
      return -1;
    if(either_copyin((char *)&value, user_src, src, sizeof(value)) < 0)
      return -1;
    acquire(&pseudodev_state.seed_lock);
    pseudodev_state.seed = value;
    release(&pseudodev_state.seed_lock);
    return sizeof(value);
  case PSEUDO_NULLSTAT:
    acquire(&pseudodev_state.nullstat_lock);
    pseudodev_state.nullstat_bytes += n;
    release(&pseudodev_state.nullstat_lock);
    return n;
  default:
    return -1;
  }
}

void
pseudodevinit(void)
{
  initlock(&pseudodev_state.seed_lock, "pseudodev_seed");
  initlock(&pseudodev_state.nullstat_lock, "pseudodev_nullstat");
  pseudodev_state.seed = 1;
  pseudodev_state.nullstat_bytes = 0;
  devsw[PSEUDO].read = pseudoread;
  devsw[PSEUDO].write = pseudowrite;
}

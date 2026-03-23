#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"

static int
ismutex(struct file *f)
{
  return f != 0 && f->type == FD_MUTEX && f->mutex != 0;
}

int
mutexalloc(struct file **f)
{
  struct sleeplock *lk;

  lk = 0;
  *f = 0;
  if((*f = filealloc()) == 0) goto bad;
  if((lk = (struct sleeplock*)kalloc()) == 0) goto bad;

  initsleeplock(lk, "mutex");

  (*f)->type = FD_MUTEX;
  (*f)->readable = 0;
  (*f)->writable = 0;
  (*f)->pipe = 0;
  (*f)->ip = 0;
  (*f)->mutex = lk;
  (*f)->off = 0;
  (*f)->major = 0;

  printf("mutexalloc: file=%p mutex=%p\n", *f, lk);
  return 0;

bad:
  if(lk){
    printf("mutexalloc: kfree failed mutex=%p\n", lk);
    kfree((char*)lk);
  }
  if(*f) fileclose(*f);
  *f = 0;
  return -1;
}

void
mutexclose(struct sleeplock *lk)
{
  if(lk == 0) return;
  printf("mutexclose: mutex=%p\n", lk);
  kfree((char*)lk);
}

int
mutexlock(struct file *f)
{
  if(!ismutex(f)) return -1;
  acquiresleep(f->mutex);
  return 0;
}

int
mutexunlock(struct file *f)
{
  if(!ismutex(f) || !holdingsleep(f->mutex)) return -1;
  releasesleep(f->mutex);
  return 0;
}

int
mutexunlockifheld(struct file *f)
{
  if(!ismutex(f)) return -1;
  if(!holdingsleep(f->mutex)) return 0;
  releasesleep(f->mutex);
  return 1;
}

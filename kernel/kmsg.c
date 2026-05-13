//
// In-kernel cyclic diagnostic message buffer and logging control.
//

#include <stdarg.h>

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "diag.h"

struct kmsg_buf {
  struct spinlock lock;
  char data[DMSG_BUF_SIZE];
  uint head;
  uint tail;
  int full;
};

struct log_ctl {
  struct spinlock lock;
  int mask;
  uint until[4];
};

static struct kmsg_buf kmsg;
static struct log_ctl lctl;

static char digits[] = "0123456789abcdef";

static int
log_index(int class)
{
  switch(class){
  case LOG_SYSCALL: return 0;
  case LOG_INTR:    return 1;
  case LOG_PROC:    return 2;
  case LOG_EXEC:    return 3;
  default:          return -1;
  }
}

static uint
now_ticks(void)
{
  uint t;

  acquire(&tickslock);
  t = ticks;
  release(&tickslock);
  return t;
}

static void
log_expire_locked(uint t)
{
  for(int class = LOG_SYSCALL; class <= LOG_EXEC; class <<= 1){
    int i = log_index(class);
    if(i >= 0 && (lctl.mask & class) && lctl.until[i] != 0 && t >= lctl.until[i]){
      lctl.mask &= ~class;
      lctl.until[i] = 0;
    }
  }
}

static void
kmsg_putc_locked(int c)
{
  if(kmsg.full) kmsg.head = (kmsg.head + 1) % DMSG_BUF_SIZE;
  kmsg.data[kmsg.tail] = c;
  kmsg.tail = (kmsg.tail + 1) % DMSG_BUF_SIZE;
  kmsg.full = (kmsg.tail == kmsg.head);
}

static void
kmsg_puts_locked(const char *s)
{
  if(s == 0) s = "(null)";
  while(*s) kmsg_putc_locked(*s++);
}

static void
kmsg_printint_locked(long long xx, int base, int sign)
{
  char buf[20];
  int i;
  unsigned long long x;

  if(sign && (sign = (xx < 0))) x = -xx;
  else x = xx;

  i = 0;
  do {
    buf[i++] = digits[x % base];
  } while((x /= base) != 0);

  if(sign) buf[i++] = '-';
  while(--i >= 0) kmsg_putc_locked(buf[i]);
}

static void
kmsg_printptr_locked(uint64 x)
{
  int i;

  kmsg_putc_locked('0');
  kmsg_putc_locked('x');
  for(i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
    kmsg_putc_locked(digits[x >> (sizeof(uint64) * 8 - 4)]);
}

static void
kmsg_vprintf_locked(const char *fmt, va_list ap)
{
  int i, cx, c0, c1, c2;
  char *s;

  for(i = 0; (cx = fmt[i] & 0xff) != 0; i++){
    if(cx != '%'){
      kmsg_putc_locked(cx);
      continue;
    }
    i++;
    c0 = fmt[i+0] & 0xff;
    c1 = c2 = 0;
    if(c0) c1 = fmt[i+1] & 0xff;
    if(c1) c2 = fmt[i+2] & 0xff;
    if(c0 == 'd')
      kmsg_printint_locked(va_arg(ap, int), 10, 1);
    else if(c0 == 'l' && c1 == 'd'){
      kmsg_printint_locked(va_arg(ap, uint64), 10, 1);
      i += 1;
    }
    else if(c0 == 'l' && c1 == 'l' && c2 == 'd'){
      kmsg_printint_locked(va_arg(ap, uint64), 10, 1);
      i += 2;
    }
    else if(c0 == 'u') kmsg_printint_locked(va_arg(ap, uint32), 10, 0);
    else if(c0 == 'l' && c1 == 'u'){
      kmsg_printint_locked(va_arg(ap, uint64), 10, 0);
      i += 1;
    }
    else if(c0 == 'l' && c1 == 'l' && c2 == 'u'){
      kmsg_printint_locked(va_arg(ap, uint64), 10, 0);
      i += 2;
    }
    else if(c0 == 'x') kmsg_printint_locked(va_arg(ap, uint32), 16, 0);
    else if(c0 == 'l' && c1 == 'x'){
      kmsg_printint_locked(va_arg(ap, uint64), 16, 0);
      i += 1;
    }
    else if(c0 == 'l' && c1 == 'l' && c2 == 'x'){
      kmsg_printint_locked(va_arg(ap, uint64), 16, 0);
      i += 2;
    }
    else if(c0 == 'p') kmsg_printptr_locked(va_arg(ap, uint64));
    else if(c0 == 'c') kmsg_putc_locked(va_arg(ap, uint));
    else if(c0 == 's'){
      if((s = va_arg(ap, char*)) == 0) s = "(null)";
      for(; *s; s++) kmsg_putc_locked(*s);
    }
    else if(c0 == '%') kmsg_putc_locked('%');
    else if(c0 == 0) break;
    else {
      kmsg_putc_locked('%');
      kmsg_putc_locked(c0);
    }
  }
}

void
kmsginit(void)
{
  initlock(&kmsg.lock, "kmsg");
  kmsg.head = 0;
  kmsg.tail = 0;
  kmsg.full = 0;

  initlock(&lctl.lock, "logctl");
  lctl.mask = 0;
  for(int i = 0; i < 4; i++) lctl.until[i] = 0;

  pr_msg("diagnostic message buffer initialized: %d bytes", DMSG_BUF_SIZE);
}

void
pr_msg(const char *fmt, ...)
{
  va_list ap;
  uint t = now_ticks();

  acquire(&kmsg.lock);
  kmsg_putc_locked('[');
  kmsg_printint_locked(t, 10, 0);
  kmsg_puts_locked("] ");

  va_start(ap, fmt);
  kmsg_vprintf_locked(fmt, ap);
  va_end(ap);

  kmsg_putc_locked('\n');
  release(&kmsg.lock);
}

int
kmsg_read_user(uint64 dst, int n)
{
  struct proc *p = myproc();
  uint len, start, off;
  int copied = 0;

  if(n <= 0) return -1;

  acquire(&kmsg.lock);

  if(kmsg.full) len = DMSG_BUF_SIZE;
  else if(kmsg.tail >= kmsg.head) len = kmsg.tail - kmsg.head;
  else len = DMSG_BUF_SIZE - kmsg.head + kmsg.tail;

  start = kmsg.head;

  if(kmsg.full && len > 0){
    for(off = 0; off < len; off++){
      char c = kmsg.data[(start + off) % DMSG_BUF_SIZE];
      if(c == '\n'){
        off++;
        start = (start + off) % DMSG_BUF_SIZE;
        len -= off;
        break;
      }
    }
  }

  while(copied < n - 1 && copied < len){
    char c = kmsg.data[(start + copied) % DMSG_BUF_SIZE];
    if(copyout(p->pagetable, dst + copied, &c, 1) < 0){
      release(&kmsg.lock);
      return -1;
    }
    copied++;
  }

  char zero = 0;
  if(copyout(p->pagetable, dst + copied, &zero, 1) < 0){
    release(&kmsg.lock);
    return -1;
  }

  release(&kmsg.lock);
  return copied;
}

int
logctl(int mask, int enable, int duration)
{
  uint t = now_ticks();
  uint deadline = 0;

  mask &= LOG_ALL;
  if(duration > 0) deadline = t + duration;

  acquire(&lctl.lock);
  log_expire_locked(t);

  if(enable < 0){
    int cur = lctl.mask;
    release(&lctl.lock);
    return cur;
  }

  if(enable){
    lctl.mask |= mask;
    for(int class = LOG_SYSCALL; class <= LOG_EXEC; class <<= 1){
      int i = log_index(class);
      if((mask & class) && i >= 0) lctl.until[i] = deadline;
    }
  } else {
    lctl.mask &= ~mask;
    for(int class = LOG_SYSCALL; class <= LOG_EXEC; class <<= 1){
      int i = log_index(class);
      if((mask & class) && i >= 0) lctl.until[i] = 0;
    }
  }

  int cur = lctl.mask;
  release(&lctl.lock);
  return cur;
}

int
log_enabled(int class)
{
  uint t = now_ticks();
  int i = log_index(class);
  int enabled = 0;

  if(i < 0) return 0;

  acquire(&lctl.lock);
  log_expire_locked(t);
  if(lctl.mask & class) enabled = 1;
  release(&lctl.lock);

  return enabled;
}

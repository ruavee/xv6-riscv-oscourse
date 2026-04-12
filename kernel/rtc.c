#include "types.h"
#include "spinlock.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

static struct spinlock rtc_lock;

#define RtcReg(reg) ((volatile uint32 *)(reg))
#define ReadRtcReg(reg) (*(RtcReg(reg)))

static uint32
rtclow(void)
{
  return ReadRtcReg(RTC0_LOW);
}

static uint32
rtchigh(void)
{
  return ReadRtcReg(RTC0_HIGH);
}

void
rtcinit(void)
{
  initlock(&rtc_lock, "rtc");
}

uint64
rtctime(void)
{
  uint32 low, high;

  acquire(&rtc_lock);
  low = rtclow();
  high = rtchigh();
  release(&rtc_lock);

  return ((uint64)high << 32) | low;
}

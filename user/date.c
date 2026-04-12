#include "kernel/types.h"
#include "user/user.h"

#define NSECS_PER_SEC 1000000000ULL
#define SECS_PER_MIN 60ULL
#define SECS_PER_HOUR (60ULL * SECS_PER_MIN)
#define SECS_PER_DAY (24ULL * SECS_PER_HOUR)

static void
putc(char c)
{
  write(1, &c, 1);
}

static void
print_padded(uint value, int width)
{
  char buf[16];
  int i;

  for(i = width - 1; i >= 0; i--){
    buf[i] = '0' + value % 10;
    value /= 10;
  }
  write(1, buf, width);
}

static int
is_leap(int year)
{
  if(year % 400 == 0) return 1;
  if(year % 100 == 0) return 0;
  return year % 4 == 0;
}

static int
days_in_month(int year, int month)
{
  static int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if(month == 2 && is_leap(year)) return 29;
  return days[month - 1];
}

int
main(int argc, char *argv[])
{
  uint64 ns, secs, days, curtime;
  uint frac_ns, hour, minute, second;
  int year, month, day;

  (void)argv;

  if(argc != 1){
    fprintf(2, "usage: date\n");
    exit(1);
  }

  ns = rtctime();
  secs = ns / NSECS_PER_SEC;
  frac_ns = ns % NSECS_PER_SEC;

  days = secs / SECS_PER_DAY;
  curtime = secs % SECS_PER_DAY;

  year = 1970;
  while(1){
    int year_days = is_leap(year) ? 366 : 365;
    if(days < year_days) break;
    days -= year_days;
    year++;
  }

  month = 1;
  while(1){
    int month_days = days_in_month(year, month);
    if(days < month_days) break;
    days -= month_days;
    month++;
  }
  day = days + 1;

  hour = curtime / SECS_PER_HOUR;
  curtime %= SECS_PER_HOUR;
  minute = curtime / SECS_PER_MIN;
  second = curtime % SECS_PER_MIN;

  printf("%d-", year);
  print_padded(month, 2);
  putc('-');
  print_padded(day, 2);
  putc(' ');
  print_padded(hour, 2);
  putc(':');
  print_padded(minute, 2);
  putc(':');
  print_padded(second, 2);
  putc('.');
  print_padded(frac_ns, 9);
  putc('\n');

  exit(0);
}

#include "kernel/types.h"
#include "user/user.h"

#define NSECS_PER_SEC 1000000000LL
#define SECS_PER_MIN 60LL
#define SECS_PER_HOUR (60LL * SECS_PER_MIN)
#define SECS_PER_DAY (24LL * SECS_PER_HOUR)
#define INT64_MAX_U 9223372036854775807ULL
#define DAYS_TO_UNIX_EPOCH 719468LL
#define YEARS_PER_ERA 400LL
#define DAYS_PER_ERA 146097LL
#define DAYS_PER_4_YEARS 1460LL
#define DAYS_PER_100_YEARS 36524LL
#define LAST_DAY_OF_ERA 146096LL
#define DAYS_PER_COMMON_YEAR 365LL
#define MONTH_FORMULA_SCALE 5LL
#define MONTH_FORMULA_OFFSET 2LL
#define DAYS_PER_5_MONTH_BLOCK 153LL
#define MARCH_BASED_MONTHS 10LL
#define MONTHS_FROM_MARCH_TO_JANUARY 9LL
#define MONTHS_FROM_JANUARY_TO_MARCH 3LL

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

static void
print_ll(long long value)
{
  char buf[32];
  unsigned long long x;
  int i;

  if(value < 0){
    putc('-');
    x = -(value + 1);
    x++;
  } else x = value;

  i = 0;
  do {
    buf[i++] = '0' + x % 10;
    x /= 10;
  } while(x != 0);

  while(--i >= 0) putc(buf[i]);
}

static long long
div_floor(long long n, long long d)
{
  long long q, r;

  q = n / d;
  r = n % d;
  if(r < 0) q--;
  return q;
}

static long long
signed_rtctime(void)
{
  uint64 raw;

  raw = rtctime();
  if(raw <= INT64_MAX_U) return raw;
  return -1 - (long long)(~raw);
}

static void
civil_from_days(long long z, long long *year, uint *month, uint *day)
{
  long long era, doe, yoe, y, doy, mp;

  z += DAYS_TO_UNIX_EPOCH;
  era = div_floor(z, DAYS_PER_ERA);
  doe = z - era * DAYS_PER_ERA;
  yoe = (doe - doe / DAYS_PER_4_YEARS + doe / DAYS_PER_100_YEARS -
         doe / LAST_DAY_OF_ERA) / DAYS_PER_COMMON_YEAR;
  y = yoe + era * YEARS_PER_ERA;
  doy = doe - (DAYS_PER_COMMON_YEAR * yoe + yoe / 4 - yoe / 100);
  mp = (MONTH_FORMULA_SCALE * doy + MONTH_FORMULA_OFFSET) /
       DAYS_PER_5_MONTH_BLOCK;

  *day = doy - (DAYS_PER_5_MONTH_BLOCK * mp + MONTH_FORMULA_OFFSET) /
         MONTH_FORMULA_SCALE + 1;
  *month = mp + (mp < MARCH_BASED_MONTHS ? MONTHS_FROM_JANUARY_TO_MARCH :
                                          -MONTHS_FROM_MARCH_TO_JANUARY);
  *year = y + (*month <= 2);
}

int
main(int argc, char *argv[])
{
  long long ns, secs, days, curtime;
  long long frac;
  long long year;
  uint frac_ns, hour, minute, second;
  uint month, day;

  (void)argv;

  if(argc != 1){
    fprintf(2, "usage: date\n");
    exit(1);
  }

  ns = signed_rtctime();
  secs = ns / NSECS_PER_SEC;
  frac = ns % NSECS_PER_SEC;
  if(frac < 0){
    frac += NSECS_PER_SEC;
    secs--;
  }
  frac_ns = frac;

  days = div_floor(secs, SECS_PER_DAY);
  curtime = secs % SECS_PER_DAY;
  if(curtime < 0) curtime += SECS_PER_DAY;

  civil_from_days(days, &year, &month, &day);

  hour = curtime / SECS_PER_HOUR;
  curtime %= SECS_PER_HOUR;
  minute = curtime / SECS_PER_MIN;
  second = curtime % SECS_PER_MIN;

  print_ll(year);
  putc('-');
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

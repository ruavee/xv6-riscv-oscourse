#include "kernel/types.h"
#include "user/user.h"

#define BUFSZ 100

static void
eputs(const char *s)
{
  write(2, s, strlen(s));
}

static int
readline(char *buf, int max)
{
  int i = 0;
  while(i + 1 < max){
    char c;
    int cc = read(0, &c, 1);
    if(cc < 0){
      eputs("read error\n");
      return -1;
    }
    if(cc == 0) break;
    if(c == '\n' || c == '\r') break;

    buf[i++] = c;
  }

  if(i + 1 == max){
    buf[i] = 0;
    eputs("input too long (buffer overflow)\n");
    return -2;
  }

  buf[i] = 0;
  return i;
}

static int
is_valid_int_token(const char *s)
{
  if(*s == '-' || *s == '+') s++;
  if(*s == 0) return 0;
  for(; *s; s++)
    if(*s < '0' || *s > '9') return 0;
  return 1;
}

static int
atoi_signed(const char *s)
{
  int sign = 1;
  if(*s == '-'){ sign = -1; s++; }
  else if(*s == '+'){ s++; }
  return sign * atoi(s);
}

int
main(void)
{
  char buf[BUFSZ];

  int n = readline(buf, BUFSZ);
  if(n < 0)
    return 1;

  if(n == 0){
    eputs("empty line\n");
    return 1;
  }

  printf("|%s|\n", buf);

  char *p = buf;
  while(*p == ' ') p++;
  if(*p == 0){
    eputs("line has only spaces\n");
    return 1;
  }

  char *q = p;
  while(*q && *q != ' ') q++;
  if(*q == 0){
    eputs("bad format: expected two numbers separated by space\n");
    return 1;
  }

  *q = 0;
  q++;
  while(*q == ' ') q++;
  if(*q == 0){
    eputs("bad format: missing second number\n");
    return 1;
  }

  char *r = q;
  while(*r && *r != ' ') r++;
  if(*r != 0){
    eputs("bad format: extra data after second number\n");
    return 1;
  }

  if(!is_valid_int_token(p) || !is_valid_int_token(q)){
    eputs("bad number format (use optional +/- and digits only)\n");
    return 1;
  }

  int a = atoi_signed(p);
  int b = atoi_signed(q);

  uint64 s = addab(a, b);
  printf("%ld\n", s);

  return 0;
}

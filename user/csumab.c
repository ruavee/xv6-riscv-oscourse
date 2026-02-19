#include "kernel/types.h"
#include "user/user.h"

#define BUFSZ 100

static int
readline(char *buf, int max)
{
  int i = 0;
  while(i + 1 < max){
    char c;
    int cc = read(0, &c, 1);
    if(cc < 1) break;
    if(c == '\n' || c == '\r') break;
    buf[i++] = c;
  }
  buf[i] = '\0';
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
  if(*s == '-') { sign = -1; s++; }
  else if(*s == '+') s++;
  return sign * atoi(s);
}

int
main(int argc, char **argv)
{
  char buf[BUFSZ];
  readline(buf, BUFSZ);

  printf("|%s|\n", buf);

  char *p = buf;
  while(*p == ' ') p++;

  char *q = p;
  while(*q && *q != ' ') q++;

  if(*q == 0){
    printf("expected: <int> <int>\n");
    return 1;
  }

  *q = 0;
  q++;
  while(*q == ' ') q++;

  if(!is_valid_int_token(p) || !is_valid_int_token(q)){
    fprintf(2, "bad number format (use optional +/- and digits only)\n");
    return 1;
  }

  int a = atoi_signed(p);
  int b = atoi_signed(q);

  uint64 s = addab(a, b);
  printf("%ld\n", (long)s);
}

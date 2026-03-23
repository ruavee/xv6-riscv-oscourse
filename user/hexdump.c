#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

static char
hexdigit(int x)
{
  if(x < 10) return '0' + x;
  return 'A' + (x - 10);
}

static void
printhexbyte(unsigned char b)
{
  char out[2];

  out[0] = hexdigit((b >> 4) & 0xF);
  out[1] = hexdigit(b & 0xF);
  write(1, out, sizeof(out));
}

int
main(int argc, char **argv)
{
  int fd, want, first;
  char buf[64];

  if(argc != 3){
    fprintf(2, "usage: hexdump count file\n");
    exit(1);
  }

  want = atoi(argv[1]);
  if(want < 0){
    fprintf(2, "hexdump: invalid count\n");
    exit(1);
  }

  fd = open(argv[2], O_RDONLY);
  if(fd < 0){
    fprintf(2, "hexdump: cannot open %s\n", argv[2]);
    exit(1);
  }

  first = 1;
  while(want > 0){
    int i, chunk, got;

    chunk = want;
    if(chunk > sizeof(buf)) 
      chunk = sizeof(buf);
    got = read(fd, buf, chunk);
    if(got < 0){
      fprintf(2, "hexdump: read error\n");
      close(fd);
      exit(1);
    }
    if(got == 0) break;

    for(i = 0; i < got; i++){
      if(!first) write(1, " ", 1);
      printhexbyte((unsigned char)buf[i]);
      first = 0;
    }
    want -= got;
  }

  write(1, "\n", 1);
  close(fd);
  exit(0);
}

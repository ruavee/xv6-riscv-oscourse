#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

static int
hexvalue(char c)
{
  if(c >= '0' && c <= '9')
    return c - '0';
  if(c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if(c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

int
main(int argc, char **argv)
{
  char *hex;
  char *buf;
  int fd, hexlen, nbytes;

  if(argc != 3){
    fprintf(2, "usage: hexwrite hexbytes file\n");
    exit(1);
  }

  hex = argv[1];
  hexlen = strlen(hex);
  if(hexlen % 2 != 0){
    fprintf(2, "hexwrite: invalid hex string\n");
    exit(1);
  }

  nbytes = hexlen / 2;
  buf = nbytes > 0 ? malloc(nbytes) : 0;
  if(nbytes > 0 && buf == 0){
    fprintf(2, "hexwrite: out of memory\n");
    exit(1);
  }

  for(int i = 0; i < nbytes; i++){
    int hi = hexvalue(hex[2 * i]);
    int lo = hexvalue(hex[2 * i + 1]);

    if(hi < 0 || lo < 0){
      fprintf(2, "hexwrite: invalid hex string\n");
      free(buf);
      exit(1);
    }
    buf[i] = (hi << 4) | lo;
  }

  fd = open(argv[2], O_WRONLY);
  if(fd < 0){
    fprintf(2, "hexwrite: cannot open %s\n", argv[2]);
    free(buf);
    exit(1);
  }

  if(write(fd, buf, nbytes) != nbytes){
    fprintf(2, "Write error\n");
    close(fd);
    free(buf);
    exit(1);
  }

  close(fd);
  free(buf);
  exit(0);
}

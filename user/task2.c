#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static void
die(const char *msg)
{
  fprintf(2, "%s\n", msg);
  exit(1);
}

static void
write_all(int fd, const char *buf, int n)
{
  int off = 0;
  while (off < n) {
    int m = write(fd, buf + off, n - off);
    if(m < 0) die("write failed");
    if(m == 0) die("write returned 0");
    off += m;
  }
}

int
main(int argc, char *argv[])
{
  int pipefd[2];

  if(pipe(pipefd) < 0)
    die("pipe failed");

  int pid = fork();
  if(pid < 0)
    die("fork failed");

  if(pid == 0){
    close(pipefd[1]);

    close(0);
    if(dup(pipefd[0]) < 0)
      die("dup failed");
    close(pipefd[0]);

    char *wcargv[] = {"/wc", 0};
    exec("/wc", wcargv);

    die("exec /wc failed");
  }

  close(pipefd[0]);

  for(int i = 1; i < argc; i++){
    int len = strlen(argv[i]);
    if(len > 0)
      write_all(pipefd[1], argv[i], len);
    write_all(pipefd[1], "\n", 1);
  }

  if(close(pipefd[1]) < 0)
    die("close write end failed");

  int st = 0;
  if(wait(&st) < 0)
    die("wait failed");

  exit(0);
}
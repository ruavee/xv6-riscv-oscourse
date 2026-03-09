#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static void
die(const char *msg)
{
  fprintf(2, "%s\n", msg);
  exit(1);
}

static int
write_all(int fd, const char *buf, int n)
{
  int off = 0;
  while (off < n) {
    int m = write(fd, buf + off, n - off);
    if (m <= 0) return -1;
    off += m;
  }
  return 0;
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
    if(close(pipefd[1]) < 0)
      die("close child write end failed");
    if(close(0) < 0)
      die("close stdin failed");
    if(dup(pipefd[0]) < 0)
      die("dup failed");
    if(close(pipefd[0]) < 0)
      die("close child read end failed");

    char *wcargv[] = {"/wc", 0};
    exec("/wc", wcargv);

    die("exec /wc failed");
  }

  if(close(pipefd[0]) < 0)
    die("close parent read end failed");

  for(int i = 1; i < argc; i++){
    int len = strlen(argv[i]);
    if (len > 0 && write_all(pipefd[1], argv[i], len) < 0)
      die("write failed");
    if (write_all(pipefd[1], "\n", 1) < 0)
      die("write failed");
  }

  if(close(pipefd[1]) < 0)
    die("close write end failed");

  int st = 0;
  if(wait(&st) < 0)
    die("wait failed");

  exit(0);
}
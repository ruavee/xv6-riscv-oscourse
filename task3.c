#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void die(const char *msg) {
  perror(msg);
  exit(1);
}

static int write_all(int fd, const void *buf, size_t n) {
  const char *p = (const char *)buf;
  size_t off = 0;
  while (off < n) {
    ssize_t w = write(fd, p + off, n - off);
    if (w < 0) {
      if (errno == EINTR) continue;
      return -1;
    }
    if (w == 0) {
      errno = EIO;
      return -1;
    }
    off += (size_t)w;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  int pipefd[2];
  if (pipe(pipefd) < 0) die("pipe");

  pid_t pid = fork();
  if (pid < 0) die("fork");

  if (pid == 0) {
    if(close(pipefd[1]) < 0)
      die("close (child write end)");

    char buf[16384];
    for (;;) {
      ssize_t r = read(pipefd[0], buf, sizeof(buf));
      if (r < 0) {
        if (errno == EINTR) continue;
        die("read");
      }
      if (r == 0) break;

      if (write_all(STDOUT_FILENO, buf, (size_t)r) < 0)
        die("write");
    }

    if (close(pipefd[0]) < 0)
      die("close (child read end)");
    exit(0);
  }

  if (close(pipefd[0]) < 0)
    die("close (parent read end)");

  for (int i = 1; i < argc; i++) {
    size_t len = strlen(argv[i]);
    if (len > 0) write_all(pipefd[1], argv[i], len);
    write_all(pipefd[1], "\n", 1);
  }

  if (close(pipefd[1]) < 0) die("close (parent write end)");

  int status = 0;
  if (waitpid(pid, &status, 0) < 0) die("waitpid");

  return 1;
}
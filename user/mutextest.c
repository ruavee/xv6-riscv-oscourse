#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static void
fail(char *msg)
{
  fprintf(2, "mutextest: %s\n", msg);
  exit(1);
}

static void
must(int ok, char *msg)
{
  if(!ok) fail(msg);
}

static void
send_byte(int fd)
{
  char ch;

  ch = 'x';
  if(write(fd, &ch, 1) != 1) fail("pipe write failed");
}

static void
recv_byte(int fd)
{
  char ch;

  if(read(fd, &ch, 1) != 1) fail("pipe read failed");
}

static void
wait_ok(char *msg)
{
  int status;

  if(wait(&status) < 0) fail("wait failed");
  if(status != 0) fail(msg);
}

static void
test_rw_stat(void)
{
  int fd;
  char ch;
  struct stat st;

  fd = mutex();
  must(fd >= 0, "mutex alloc for rw/stat");

  ch = 'a';
  must(read(fd, &ch, 1) < 0, "read on mutex must fail");
  must(write(fd, &ch, 1) < 0, "write on mutex must fail");
  must(fstat(fd, &st) < 0, "fstat on mutex must fail");
  must(close(fd) == 0, "close after rw/stat");

  printf("mutextest: rw/stat checks passed\n");
}

static void
test_close_by_owner(void)
{
  int fd, alias;

  fd = mutex();
  must(fd >= 0, "mutex alloc for owner close");
  alias = dup(fd);
  must(alias >= 0, "dup for owner close");

  must(mutex_lock(fd) == 0, "lock before owner close");
  must(close(fd) == 0, "close locked mutex by owner");
  must(mutex_unlock(alias) < 0, "owner close must auto-unlock");
  must(mutex_lock(alias) == 0, "relock after owner close");
  must(mutex_unlock(alias) == 0, "unlock alias after relock");
  must(close(alias) == 0, "close alias after owner close");

  printf("mutextest: close by owner passed\n");
}

static void
test_close_by_other(void)
{
  int fd, ready[2], ack[2];
  int pid;

  fd = mutex();
  must(fd >= 0, "mutex alloc for close by other");
  must(pipe(ready) == 0, "pipe ready");
  must(pipe(ack) == 0, "pipe ack");

  pid = fork();
  must(pid >= 0, "fork for close by other");

  if(pid == 0){
    close(ready[0]);
    close(ack[1]);
    must(mutex_lock(fd) == 0, "child lock before parent close");
    send_byte(ready[1]);
    recv_byte(ack[0]);
    must(mutex_unlock(fd) == 0, "child unlock after parent close");
    close(fd);
    close(ready[1]);
    close(ack[0]);
    exit(0);
  }

  close(ready[1]);
  close(ack[0]);
  recv_byte(ready[0]);
  must(close(fd) == 0, "parent close while child owns mutex");
  send_byte(ack[1]);
  wait_ok("child failed after parent close");
  close(ready[0]);
  close(ack[1]);

  printf("mutextest: close by other passed\n");
}

static void
test_unlock_by_other(void)
{
  int fd, ready[2], ack[2];
  int pid;

  fd = mutex();
  must(fd >= 0, "mutex alloc for unlock by other");
  must(pipe(ready) == 0, "pipe ready for unlock by other");
  must(pipe(ack) == 0, "pipe ack for unlock by other");

  pid = fork();
  must(pid >= 0, "fork for unlock by other");

  if(pid == 0){
    close(ready[0]);
    close(ack[1]);
    must(mutex_lock(fd) == 0, "child lock for foreign unlock");
    send_byte(ready[1]);
    recv_byte(ack[0]);
    must(mutex_unlock(fd) == 0, "child unlock after foreign attempt");
    close(fd);
    close(ready[1]);
    close(ack[0]);
    exit(0);
  }

  close(ready[1]);
  close(ack[0]);
  recv_byte(ready[0]);
  must(mutex_unlock(fd) < 0, "foreign unlock must fail");
  send_byte(ack[1]);
  wait_ok("child failed after foreign unlock");
  close(fd);
  close(ready[0]);
  close(ack[1]);

  printf("mutextest: unlock by other passed\n");
}

static void
test_exit_unlocks_mutex(void)
{
  int fd;
  int pid;

  fd = mutex();
  must(fd >= 0, "mutex alloc for exit unlock");

  pid = fork();
  must(pid >= 0, "fork for exit unlock");

  if(pid == 0){
    must(mutex_lock(fd) == 0, "child lock before exit");
    exit(0);
  }

  wait_ok("child failed while exiting with mutex");
  must(mutex_lock(fd) == 0, "parent relock after child exit");
  must(mutex_unlock(fd) == 0, "parent unlock after child exit");
  close(fd);

  printf("mutextest: exit closes and unlocks mutex passed\n");
}

static void
test_exit_wakes_waiter(void)
{
  int fd, p[2];
  int holder, waiter;

  fd = mutex();
  must(fd >= 0, "mutex alloc for waiter wakeup");
  must(pipe(p) == 0, "pipe for waiter wakeup");

  holder = fork();
  must(holder >= 0, "fork holder");
  if(holder == 0){
    close(p[0]);
    must(mutex_lock(fd) == 0, "holder lock");
    send_byte(p[1]);
    pause(5);
    exit(0);
  }

  waiter = fork();
  must(waiter >= 0, "fork waiter");
  if(waiter == 0){
    close(p[0]);
    must(mutex_lock(fd) == 0, "waiter lock after holder exit");
    send_byte(p[1]);
    must(mutex_unlock(fd) == 0, "waiter unlock");
    close(fd);
    close(p[1]);
    exit(0);
  }

  close(p[1]);
  recv_byte(p[0]);
  recv_byte(p[0]);
  wait_ok("holder failed while exiting");
  wait_ok("waiter failed to acquire after holder exit");
  close(fd);
  close(p[0]);

  printf("mutextest: exit wakes blocked waiter passed\n");
}

int
main(void)
{
  test_rw_stat();
  test_close_by_owner();
  test_close_by_other();
  test_unlock_by_other();
  test_exit_unlocks_mutex();
  test_exit_wakes_waiter();
  printf("mutextest: all checks passed\n");
  exit(0);
}

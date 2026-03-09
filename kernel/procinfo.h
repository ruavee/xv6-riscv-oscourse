#ifndef XV6_PROCINFO_H
#define XV6_PROCINFO_H

#include "types.h"

#define PROCINFO_NAME_MAX 16

enum procstate {
  UNUSED,
  USED,
  SLEEPING,
  RUNNABLE,
  RUNNING,
  ZOMBIE,
};

struct procinfo {
  int pid;
  char name[PROCINFO_NAME_MAX];
  enum procstate state;
  int ppid;
};

#endif

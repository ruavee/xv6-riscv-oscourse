#ifndef XV6_DIAG_H
#define XV6_DIAG_H

#define LOG_SYSCALL 0x1
#define LOG_INTR    0x2
#define LOG_PROC    0x4
#define LOG_EXEC    0x8
#define LOG_ALL     (LOG_SYSCALL | LOG_INTR | LOG_PROC | LOG_EXEC)

#endif

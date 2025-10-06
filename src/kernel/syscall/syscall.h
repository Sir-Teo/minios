#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include <stddef.h>

// System call numbers
#define SYS_READ    0
#define SYS_WRITE   1
#define SYS_OPEN    2
#define SYS_CLOSE   3
#define SYS_EXIT    4
#define SYS_FORK    5
#define SYS_EXEC    6
#define SYS_WAIT    7
#define SYS_MMAP    8
#define SYS_MUNMAP  9
#define SYS_YIELD   10
#define SYS_GETPID  11

#define MAX_SYSCALLS 256

// System call handler type
typedef int64_t (*syscall_handler_t)(uint64_t arg1, uint64_t arg2,
                                      uint64_t arg3, uint64_t arg4,
                                      uint64_t arg5);

// Initialize system call subsystem
void syscall_init(void);

// System call implementations
int64_t sys_read(uint64_t fd, uint64_t buf, uint64_t count);
int64_t sys_write(uint64_t fd, uint64_t buf, uint64_t count);
int64_t sys_open(uint64_t path, uint64_t flags, uint64_t mode);
int64_t sys_close(uint64_t fd);
int64_t sys_exit(uint64_t exit_code);
int64_t sys_fork(void);
int64_t sys_exec(uint64_t path, uint64_t argv, uint64_t envp);
int64_t sys_wait(uint64_t pid, uint64_t status, uint64_t options);
int64_t sys_mmap(uint64_t addr, uint64_t length, uint64_t prot,
                 uint64_t flags, uint64_t fd);
int64_t sys_munmap(uint64_t addr, uint64_t length);
int64_t sys_yield(void);
int64_t sys_getpid(void);

// Common system call dispatcher (called from assembly)
int64_t syscall_dispatch(uint64_t syscall_num, uint64_t arg1, uint64_t arg2,
                         uint64_t arg3, uint64_t arg4, uint64_t arg5);

#endif // SYSCALL_H

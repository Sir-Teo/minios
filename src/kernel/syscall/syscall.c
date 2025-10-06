#include "syscall.h"
#include "kprintf.h"
#include "sched/scheduler.h"
#include "sched/task.h"
#include <stddef.h>

// MSR definitions for syscall/sysret
#define MSR_STAR    0xC0000081  // Segment selectors for syscall/sysret
#define MSR_LSTAR   0xC0000082  // RIP for syscall entry
#define MSR_CSTAR   0xC0000083  // RIP for compatibility mode (unused)
#define MSR_SFMASK  0xC0000084  // RFLAGS mask

// External assembly entry point
extern void syscall_entry(void);

// Write MSR helper
static inline void wrmsr(uint32_t msr, uint64_t value) {
    uint32_t low = value & 0xFFFFFFFF;
    uint32_t high = value >> 32;
    __asm__ volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

// Read MSR helper
static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t low, high;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

// System call table
static syscall_handler_t syscall_table[MAX_SYSCALLS];

void syscall_init(void) {
    kprintf("[SYSCALL] Initializing system call subsystem...\n");

    // Initialize system call table
    for (int i = 0; i < MAX_SYSCALLS; i++) {
        syscall_table[i] = NULL;
    }

    // Register system call handlers
    syscall_table[SYS_READ]    = (syscall_handler_t)sys_read;
    syscall_table[SYS_WRITE]   = (syscall_handler_t)sys_write;
    syscall_table[SYS_OPEN]    = (syscall_handler_t)sys_open;
    syscall_table[SYS_CLOSE]   = (syscall_handler_t)sys_close;
    syscall_table[SYS_EXIT]    = (syscall_handler_t)sys_exit;
    syscall_table[SYS_FORK]    = (syscall_handler_t)sys_fork;
    syscall_table[SYS_EXEC]    = (syscall_handler_t)sys_exec;
    syscall_table[SYS_WAIT]    = (syscall_handler_t)sys_wait;
    syscall_table[SYS_MMAP]    = (syscall_handler_t)sys_mmap;
    syscall_table[SYS_MUNMAP]  = (syscall_handler_t)sys_munmap;
    syscall_table[SYS_YIELD]   = (syscall_handler_t)sys_yield;
    syscall_table[SYS_GETPID]  = (syscall_handler_t)sys_getpid;

    // Configure MSRs for syscall/sysret
    // STAR: [63:48] = user CS/SS base, [47:32] = kernel CS/SS base
    // Kernel CS = 0x08, User CS = 0x18 (assumes standard GDT layout)
    uint64_t star = ((uint64_t)0x18 << 48) | ((uint64_t)0x08 << 32);
    wrmsr(MSR_STAR, star);

    // LSTAR: Entry point for syscall instruction
    wrmsr(MSR_LSTAR, (uint64_t)syscall_entry);

    // SFMASK: Clear interrupt flag when entering syscall
    wrmsr(MSR_SFMASK, 0x200);  // Clear IF (bit 9)

    kprintf("[SYSCALL] System call subsystem initialized\n");
    kprintf("[SYSCALL] Entry point: %p\n", syscall_entry);
    kprintf("[SYSCALL] Registered %d syscalls\n", 12);
}

// Main system call dispatcher
int64_t syscall_dispatch(uint64_t syscall_num, uint64_t arg1, uint64_t arg2,
                         uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    // Validate syscall number
    if (syscall_num >= MAX_SYSCALLS || syscall_table[syscall_num] == NULL) {
        kprintf("[SYSCALL] Invalid syscall number: %llu\n", syscall_num);
        return -1;  // -ENOSYS
    }

    // Call the appropriate handler
    syscall_handler_t handler = syscall_table[syscall_num];
    return handler(arg1, arg2, arg3, arg4, arg5);
}

// ============================================================================
// System Call Implementations
// ============================================================================

int64_t sys_read(uint64_t fd, uint64_t buf, uint64_t count) {
    // TODO: Implement file descriptor reading
    // For now, just a stub
    kprintf("[SYSCALL] sys_read(fd=%llu, buf=%p, count=%llu)\n", fd, (void*)buf, count);
    return -1;  // Not implemented
}

int64_t sys_write(uint64_t fd, uint64_t buf, uint64_t count) {
    // Simple implementation: write to serial console if fd == 1 (stdout)
    if (fd == 1 || fd == 2) {  // stdout or stderr
        const char *str = (const char *)buf;
        for (size_t i = 0; i < count; i++) {
            kprintf("%c", str[i]);
        }
        return (int64_t)count;
    }

    return -1;  // Bad file descriptor
}

int64_t sys_open(uint64_t path, uint64_t flags, uint64_t mode) {
    // TODO: Implement file opening
    kprintf("[SYSCALL] sys_open(path=%p, flags=%llu, mode=%llu)\n",
            (void*)path, flags, mode);
    return -1;  // Not implemented
}

int64_t sys_close(uint64_t fd) {
    // TODO: Implement file closing
    kprintf("[SYSCALL] sys_close(fd=%llu)\n", fd);
    return -1;  // Not implemented
}

int64_t sys_exit(uint64_t exit_code) {
    kprintf("[SYSCALL] sys_exit(code=%llu) - Task exiting\n", exit_code);
    task_exit((int)exit_code);
    // Should never return
    return 0;
}

int64_t sys_fork(void) {
    // TODO: Implement process forking
    kprintf("[SYSCALL] sys_fork() - Not implemented\n");
    return -1;  // Not implemented
}

int64_t sys_exec(uint64_t path, uint64_t argv, uint64_t envp) {
    // TODO: Implement program execution
    kprintf("[SYSCALL] sys_exec(path=%p) - Not implemented\n", (void*)path);
    return -1;  // Not implemented
}

int64_t sys_wait(uint64_t pid, uint64_t status, uint64_t options) {
    // TODO: Implement process waiting
    kprintf("[SYSCALL] sys_wait(pid=%llu) - Not implemented\n", pid);
    return -1;  // Not implemented
}

int64_t sys_mmap(uint64_t addr, uint64_t length, uint64_t prot,
                 uint64_t flags, uint64_t fd) {
    // TODO: Implement memory mapping
    kprintf("[SYSCALL] sys_mmap(addr=%p, len=%llu) - Not implemented\n",
            (void*)addr, length);
    return -1;  // Not implemented
}

int64_t sys_munmap(uint64_t addr, uint64_t length) {
    // TODO: Implement memory unmapping
    kprintf("[SYSCALL] sys_munmap(addr=%p, len=%llu) - Not implemented\n",
            (void*)addr, length);
    return -1;  // Not implemented
}

int64_t sys_yield(void) {
    // Yield the CPU to another task
    sched_yield();
    return 0;
}

int64_t sys_getpid(void) {
    // Get current process ID
    task_t *current = sched_get_current_task();
    if (current) {
        return (int64_t)current->pid;
    }
    return -1;
}

# miniOS Development Roadmap

**Goal:** Build a modern, fully-functional x86_64 operating system from scratch with:
- Preemptive multitasking
- User mode support
- System calls
- Virtual memory management
- Device drivers (keyboard, disk, network)
- Filesystem support
- Eventually: Multi-core support, networking, GUI

---

## Phase 0: Foundation ✅ COMPLETED

**Status:** ✅ Done  
**Completion:** 100%

### Implemented:
- [x] Project structure with clean architecture
- [x] Limine bootloader integration (UEFI + BIOS)
- [x] Cross-compilation toolchain (x86_64-elf-gcc)
- [x] Build system (GNU Makefile)
- [x] CI/CD pipeline (GitHub Actions)
- [x] Serial console driver (16550A UART)
- [x] Framebuffer driver for graphics
- [x] Basic kernel logging and diagnostics

**Files:** `GNUmakefile`, `linker.lds`, `limine.conf`, `.github/workflows/ci.yml`

---

## Phase 1: Core CPU & Memory ✅ COMPLETED

**Status:** ✅ Done  
**Completion:** 100%

### Implemented:
- [x] GDT (Global Descriptor Table) with kernel/user segments
- [x] TSS (Task State Segment) for privilege transitions
- [x] IDT (Interrupt Descriptor Table) with 256 entries
- [x] ISR handlers for CPU exceptions (0-31)
- [x] IRQ handlers for hardware interrupts (32-47)
- [x] Physical Memory Manager (bitmap-based page frame allocator)
- [x] Kernel heap allocator (kmalloc/kfree)
- [x] Memory map parsing from Limine
- [x] HHDM (Higher Half Direct Map) setup

**Files:** 
- `src/arch/x86_64/interrupts/{gdt.c,idt.c,gdt_asm.S,isr_asm.S}`
- `src/kernel/mm/{pmm.c,kmalloc.c}`

**Metrics:**
- 60 KB kernel binary
- 1,050 lines of C code
- 205 lines of assembly

---

## Phase 2: Virtual Memory Management ✅ COMPLETED

**Status:** ✅ Done
**Completion:** 100%

### Implemented:
- [x] 4-level page table implementation (PML4 → PDPT → PD → PT)
- [x] Virtual address space management
- [x] Page table creation and mapping functions
- [x] CR3 switching for address space changes
- [x] Kernel space identity mapping
- [x] Address space isolation
- [x] Comprehensive test suite (25+ test cases)
- [ ] Copy-on-write (COW) support (deferred to later)
- [ ] Demand paging infrastructure (deferred to later)

### Key Functions Needed:
```c
void vmm_init(void);
address_space_t* vmm_create_address_space(void);
bool vmm_map_page(address_space_t *as, uint64_t virt, uint64_t phys, uint64_t flags);
bool vmm_unmap_page(address_space_t *as, uint64_t virt);
void vmm_switch_address_space(address_space_t *as);
```

**Files to Create:**
- `src/arch/x86_64/mm/vmm.{c,h}` - Virtual memory manager
- `src/arch/x86_64/mm/paging.{c,h}` - Page table manipulation

**Tests:**
- Map and access virtual pages
- Create multiple address spaces
- Switch between address spaces
- Handle page faults gracefully

---

## Phase 3: Timer & Interrupts ✅ COMPLETED

**Status:** ✅ Done
**Completion:** 100%

### Implemented:
- [x] PIT (Programmable Interval Timer) driver
- [x] Configurable timer frequency (e.g., 100Hz, 1000Hz)
- [x] Timer interrupt handler (IRQ0)
- [x] Tick counting mechanism
- [x] Sleep functionality
- [x] Callback mechanism for timer events
- [x] PIC remapping for proper IRQ handling
- [x] Comprehensive test suite (10+ test cases)
- [ ] APIC initialization (deferred to advanced phase)
- [ ] Local APIC timer (deferred to advanced phase)

### Key Functions Implemented:
```c
void pit_init(uint32_t frequency);
uint64_t pit_get_ticks(void);
void pit_sleep(uint64_t ticks);
void pit_set_callback(pit_callback_t callback);
void pit_irq_handler(void);
```

**Files Created:**
- `src/drivers/timer/pit.{c,h}` - PIT driver (~170 LOC)
- `src/tests/test_pit.c` - Comprehensive test suite (~340 LOC)

**Tests:**
- Timer initialization at various frequencies
- Tick counting and overflow handling
- Sleep function accuracy
- Callback mechanism
- Multiple consecutive sleeps
- High-frequency timer operation
- Timer accuracy within tolerance
- Edge cases (zero-tick sleep)

**Metrics:**
- ~170 LOC (production code)
- ~340 LOC (test code)
- 10 test cases covering all functionality
- Binary size increased by ~5 KB

**Note:** APIC timer implementation has been deferred to Phase 12 (Advanced Features) as PIT provides sufficient timer functionality for basic scheduling. The PIT driver is production-ready and fully tested.

---

## Phase 4: Multitasking & Scheduling

**Status:** 📋 Planned  
**Completion:** 0%

### Goals:
- [ ] Task/Process structure definition
- [ ] Context switching (save/restore registers)
- [ ] Kernel stacks for each task
- [ ] Task state management (running, ready, blocked)
- [ ] Simple round-robin scheduler
- [ ] Priority-based scheduling
- [ ] Idle task
- [ ] Task creation and termination

### Key Structures:
```c
typedef struct task {
    uint64_t pid;
    uint64_t *stack;
    cpu_state_t cpu_state;
    address_space_t *address_space;
    task_state_t state;
    struct task *next;
} task_t;
```

### Key Functions Needed:
```c
void sched_init(void);
task_t* task_create(void (*entry)(void), uint64_t flags);
void task_exit(int exit_code);
void sched_yield(void);
void schedule(void);  // Called from timer interrupt
```

**Files to Create:**
- `src/kernel/sched/scheduler.{c,h}`
- `src/kernel/sched/task.{c,h}`
- `src/arch/x86_64/context_switch.S`

**Tests:**
- Create multiple tasks
- Tasks execute in round-robin
- Context switch preserves state
- Task termination works

---

## Phase 5: System Calls

**Status:** 📋 Planned  
**Completion:** 0%

### Goals:
- [ ] syscall/sysret instruction setup (MSR configuration)
- [ ] System call table
- [ ] System call dispatcher
- [ ] Basic syscalls:
  - `sys_write` - Write to console/file
  - `sys_read` - Read from console/file
  - `sys_exit` - Terminate process
  - `sys_fork` - Create new process
  - `sys_exec` - Execute program
  - `sys_wait` - Wait for child process
  - `sys_mmap` - Memory mapping
  - `sys_munmap` - Unmap memory

### Key Functions Needed:
```c
void syscall_init(void);
int64_t syscall_handler(uint64_t syscall_num, uint64_t arg1, 
                        uint64_t arg2, uint64_t arg3, uint64_t arg4);
```

**Files to Create:**
- `src/kernel/syscall/syscall.{c,h}`
- `src/kernel/syscall/syscall_table.c`
- `src/arch/x86_64/syscall_entry.S`

**Tests:**
- Each syscall from user mode
- Parameter passing
- Return values
- Error handling

---

## Phase 6: User Mode

**Status:** 📋 Planned  
**Completion:** 0%

### Goals:
- [ ] User mode (ring 3) transitions
- [ ] User stack setup
- [ ] Kernel/user memory separation
- [ ] User page table setup
- [ ] User mode entry point
- [ ] Simple user program loader
- [ ] Protection checks

### Key Functions Needed:
```c
void enter_usermode(uint64_t entry, uint64_t stack);
bool is_usermode_address(uint64_t addr);
```

**Files to Create:**
- `src/kernel/user/usermode.{c,h}`
- `src/arch/x86_64/usermode_entry.S`

**Tests:**
- Jump to user mode
- Syscall from user mode works
- User cannot access kernel memory
- Privilege violations trigger faults

---

## Phase 7: ELF Loader

**Status:** 📋 Planned  
**Completion:** 0%

### Goals:
- [ ] ELF64 parser
- [ ] Program header loading
- [ ] BSS section zeroing
- [ ] Entry point detection
- [ ] Load user programs from memory
- [ ] Dynamic linking basics (optional)

### Key Functions Needed:
```c
task_t* elf_load(void *elf_data, size_t size);
bool elf_validate(void *elf_data);
```

**Files to Create:**
- `src/kernel/loader/elf.{c,h}`

**Tests:**
- Load simple ELF binary
- Execute loaded program
- Multiple programs

---

## Phase 8: Device Drivers

**Status:** 📋 Planned  
**Completion:** 0%

### Goals:
- [ ] Keyboard driver (PS/2)
- [ ] Keyboard input buffer
- [ ] Scancode to ASCII conversion
- [ ] Mouse driver (PS/2) - optional
- [ ] ATA/AHCI disk driver
- [ ] Disk read/write operations

### Key Functions Needed:
```c
void keyboard_init(void);
char keyboard_getchar(void);
void disk_init(void);
int disk_read(uint64_t lba, void *buffer, size_t sectors);
int disk_write(uint64_t lba, const void *buffer, size_t sectors);
```

**Files to Create:**
- `src/drivers/keyboard/ps2_keyboard.{c,h}`
- `src/drivers/disk/ata.{c,h}` or `ahci.{c,h}`

**Tests:**
- Read keyboard input
- Disk read/write operations

---

## Phase 9: Virtual File System (VFS)

**Status:** 📋 Planned  
**Completion:** 0%

### Goals:
- [ ] VFS abstraction layer
- [ ] File descriptor table
- [ ] inode structure
- [ ] Directory operations
- [ ] File operations (open, read, write, close)
- [ ] Path resolution

### Key Structures:
```c
typedef struct vfs_node {
    char name[256];
    uint64_t inode;
    uint64_t size;
    uint64_t flags;
    struct vfs_node *parent;
} vfs_node_t;
```

**Files to Create:**
- `src/kernel/fs/vfs.{c,h}`

---

## Phase 10: File System Implementation

**Status:** 📋 Planned  
**Completion:** 0%

### Goals:
- [ ] Simple filesystem (e.g., ext2 or custom)
- [ ] Superblock parsing
- [ ] Inode reading
- [ ] Directory traversal
- [ ] File read/write

**Files to Create:**
- `src/kernel/fs/ext2.{c,h}` or `simplefs.{c,h}`

---

## Phase 11: Shell & User Programs

**Status:** 📋 Planned  
**Completion:** 0%

### Goals:
- [ ] Simple shell (kernel or user mode)
- [ ] Command parsing
- [ ] Built-in commands (ls, cat, echo, etc.)
- [ ] Program execution
- [ ] Standard I/O redirection

**Files to Create:**
- `src/user/shell/shell.c`

---

## Phase 12: Advanced Features

**Status:** 📋 Future  
**Completion:** 0%

### Goals:
- [ ] SMP (Symmetric Multiprocessing) support
- [ ] Per-CPU data structures
- [ ] Spinlocks and mutexes
- [ ] Network stack (TCP/IP)
- [ ] Network drivers (e1000, virtio-net)
- [ ] GUI framework
- [ ] Windowing system

---

## Development Principles

1. **Incremental Development:** Build one feature at a time, test thoroughly
2. **Clean Architecture:** Separate arch-specific from generic code
3. **Testing:** Test each phase before moving to next
4. **Documentation:** Document as you go
5. **Git Commits:** Commit after each working feature
6. **CI/CD:** Keep automated tests passing

---

## Success Metrics by Phase

| Phase | Lines of Code | Binary Size | Features |
|-------|---------------|-------------|----------|
| 0-1   | ~1,300        | 60 KB       | Boot + Basic MM |
| 2     | ~2,100        | 75 KB       | + VMM ✅ |
| 3     | ~2,600        | 80 KB       | + Timer ✅ |
| 4     | ~3,000        | 90 KB       | + Multitasking |
| 5-6   | ~3,700        | 110 KB      | + Syscalls + User Mode |
| 7-8   | ~4,500        | 130 KB      | + ELF + Drivers |
| 9-10  | ~5,500        | 160 KB      | + Filesystem |
| 11    | ~6,500        | 190 KB      | + Shell |

---

## Current Status Summary

```
Phase 0: Foundation               ████████████████████ 100%
Phase 1: Core CPU & Memory        ████████████████████ 100%
Phase 2: Virtual Memory           ████████████████████ 100%
Phase 3: Timer & Interrupts       ████████████████████ 100%
Phase 4: Multitasking             ░░░░░░░░░░░░░░░░░░░░   0%
Phase 5: System Calls             ░░░░░░░░░░░░░░░░░░░░   0%
Phase 6: User Mode                ░░░░░░░░░░░░░░░░░░░░   0%
Phase 7: ELF Loader               ░░░░░░░░░░░░░░░░░░░░   0%
Phase 8: Device Drivers           ░░░░░░░░░░░░░░░░░░░░   0%
Phase 9: VFS                      ░░░░░░░░░░░░░░░░░░░░   0%
Phase 10: Filesystem              ░░░░░░░░░░░░░░░░░░░░   0%
Phase 11: Shell                   ░░░░░░░░░░░░░░░░░░░░   0%

Overall Progress: ███████░░░░░░░░░░░░░ 33.3%
```

**Next Up:** Phase 4 - Multitasking & Scheduling

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

## Phase 4: Multitasking & Scheduling ✅ COMPLETED

**Status:** ✅ Done
**Completion:** 100%

### Implemented:
- [x] Task/Process structure definition
- [x] Context switching (save/restore registers)
- [x] Kernel stacks for each task (16KB per task)
- [x] Task state management (ready, running, blocked, terminated)
- [x] Simple round-robin scheduler
- [x] Priority support (0 = highest priority)
- [x] Idle task (runs when no other tasks ready)
- [x] Task creation and termination
- [x] Scheduler integration with timer interrupts
- [x] Comprehensive test suite (10+ test cases)
- [x] Voluntary yielding support

### Key Structures Implemented:
```c
typedef struct task {
    uint64_t pid;
    uint64_t *kernel_stack;
    cpu_state_t cpu_state;
    address_space_t *address_space;
    task_state_t state;
    uint64_t priority;
    uint64_t time_slice;
    uint64_t total_runtime;
    struct task *next;
} task_t;
```

### Key Functions Implemented:
```c
void task_init(void);
task_t *task_create(void (*entry)(void), uint64_t priority);
void task_destroy(task_t *task);
void sched_init(void);
void sched_add_task(task_t *task);
void schedule(void);  // Called from timer interrupt
void sched_yield(void);
void task_exit(int exit_code);
```

**Files Created:**
- `src/kernel/sched/scheduler.{c,h}` - Round-robin scheduler (~260 LOC)
- `src/kernel/sched/task.{c,h}` - Task management (~150 LOC)
- `src/arch/x86_64/context_switch.S` - Context switch assembly (~120 LOC)
- `src/tests/test_sched.c` - Comprehensive test suite (~410 LOC)

**Tests:**
- Task subsystem initialization
- Task creation and destruction
- Scheduler initialization
- Add/remove tasks from ready queue
- Enable/disable scheduler
- Task state transitions
- Multiple task creation
- Task priority handling
- CPU state initialization
- Scheduler task count tracking

**Metrics:**
- ~530 LOC (production code)
- ~410 LOC (test code)
- 10 test cases covering all functionality
- Binary size increased by ~10 KB

**Integration:**
- Timer interrupt calls `schedule()` every tick
- Tasks can voluntarily yield with `sched_yield()`
- Idle task runs when no other tasks are ready
- Full integration with kernel initialization sequence

---

## Phase 5: System Calls ✅ COMPLETED

**Status:** ✅ Done
**Completion:** 100%

### Implemented:
- [x] syscall/sysret instruction setup (MSR configuration)
- [x] System call table and dispatcher
- [x] System call entry point in assembly
- [x] Basic syscalls:
  - `sys_write` - Write to stdout/stderr (serial console)
  - `sys_exit` - Terminate process
  - `sys_yield` - Voluntary CPU yielding
  - `sys_getpid` - Get process ID
  - Stubs for: `sys_read`, `sys_open`, `sys_close`, `sys_fork`, `sys_exec`, `sys_wait`, `sys_mmap`, `sys_munmap`
- [x] Enhanced kprintf with full format string support (%d, %u, %x, %p, %s, %c)
- [x] Comprehensive test suite (15+ test cases)

### Key Functions Implemented:
```c
void syscall_init(void);
int64_t syscall_dispatch(uint64_t syscall_num, uint64_t arg1,
                         uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5);
void kprintf(const char *fmt, ...);
```

**Files Created:**
- `src/kernel/syscall/syscall.{c,h}` - Syscall infrastructure (~210 LOC)
- `src/kernel/kprintf.{c,h}` - Enhanced printf (~100 LOC)
- `src/arch/x86_64/syscall_entry.S` - Syscall entry point (~70 LOC)
- `src/tests/test_syscall.c` - Comprehensive test suite (~260 LOC)

**Tests:**
- Invalid syscall number handling
- sys_write to stdout and stderr
- sys_write to invalid file descriptors
- All unimplemented syscalls return proper errors
- sys_yield and sys_getpid functionality
- Multiple syscalls in sequence
- Parameter passing and return values

**Metrics:**
- ~360 LOC (production code)
- ~260 LOC (test code)
- 15 test cases covering all functionality
- Binary size increased by ~5 KB

**Note:** Full user mode support is deferred to Phase 6. Current syscalls can be tested via direct dispatcher calls from kernel mode.

---

## Phase 6: User Mode ✅ COMPLETED

**Status:** ✅ Done
**Completion:** 100%

### Implemented:
- [x] User mode (ring 3) transitions via IRET
- [x] User stack allocation and mapping
- [x] Kernel/user memory separation
- [x] User page table setup with proper flags
- [x] User mode entry point (assembly)
- [x] User mode task creation
- [x] Address space validation functions
- [x] Memory protection and isolation
- [x] Integration with task/scheduler system
- [x] Comprehensive test suite (10+ test cases)

### Key Functions Implemented:
```c
void usermode_init(void);
void enter_usermode(uint64_t entry, uint64_t user_stack_top);
bool setup_user_memory(void *address_space, uint64_t code_start,
                       uint64_t code_size, uint64_t stack_top);
bool is_usermode_address(uint64_t addr);
bool is_kernelmode_address(uint64_t addr);
task_t *task_create_user(uint64_t entry, uint64_t priority);
```

**Files Created:**
- `src/kernel/user/usermode.{c,h}` - User mode infrastructure (~120 LOC)
- `src/arch/x86_64/usermode_entry.S` - Ring 3 transition (~60 LOC)
- `src/tests/test_usermode.c` - Comprehensive test suite (~200 LOC)
- Updated `src/kernel/sched/task.{c,h}` - User mode task support (~100 LOC)

**Tests:**
- User mode initialization
- User and kernel address validation
- Create user address spaces
- Setup user memory mapping
- Multiple isolated address spaces
- Different memory sizes
- Boundary address checks
- Address space isolation

**Metrics:**
- ~280 LOC (production code)
- ~200 LOC (test code)
- 10 test cases covering all functionality
- Binary size increased by ~99 KB (from 95 KB to 194 KB)

**Note:** User mode tasks can be created and isolated. Full program execution requires ELF loader (Phase 7).

---

## Phase 7: ELF Loader ✅ COMPLETED

**Status:** ✅ Done
**Completion:** 100%

### Implemented:
- [x] ELF64 header structures and constants
- [x] ELF validation (magic, class, endianness, architecture)
- [x] Program header parsing
- [x] Loadable segment processing (PT_LOAD)
- [x] Memory allocation and mapping for segments
- [x] Segment data copying from ELF file
- [x] BSS section zeroing (p_memsz > p_filesz)
- [x] Entry point detection
- [x] Page permissions (read, write, execute, no-execute)
- [x] Address space creation for loaded programs
- [x] HHDM usage for physical memory access
- [x] Error handling and reporting
- [x] Comprehensive test suite (12+ test cases)
- [ ] Dynamic linking (deferred to later phase)
- [ ] Integration with task creation (deferred - requires Phase 8+)

### Key Functions Implemented:
```c
void elf_init(void);
bool elf_validate(const void *elf_data, size_t size);
void *elf_load(const void *elf_data, size_t size, uint64_t *entry_point);
const char *elf_strerror(int error);
```

**Files Created:**
- `src/kernel/loader/elf.{c,h}` - ELF64 loader (~300 LOC)
- `src/tests/test_elf.c` - Comprehensive test suite (~380 LOC)

**Tests:**
- ELF validation (magic, class, endianness, architecture)
- Invalid ELF rejection (wrong magic, 32-bit, big-endian, etc.)
- Load simple ELF binary
- Load ELF with BSS section
- Load ELF with multiple segments
- Error string formatting
- Null/invalid data handling

**Metrics:**
- ~300 LOC (production code)
- ~380 LOC (test code)
- 12 test cases covering all functionality
- Binary size increased by ~25 KB (from 194 KB to 219 KB)

**Note:** Full program execution (loading from disk, creating tasks from ELF) will be implemented in later phases once filesystem and device drivers are available. Current implementation provides the core ELF loading infrastructure.

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
| 4     | ~3,100        | 90 KB       | + Multitasking ✅ |
| 5     | ~3,720        | 95 KB       | + Syscalls ✅ |
| 6     | ~5,780        | 194 KB      | + User Mode ✅ |
| 7     | ~6,460        | 219 KB      | + ELF Loader ✅ |
| 8     | ~7,200        | 250 KB      | + Drivers |
| 9-10  | ~8,000        | 260 KB      | + Filesystem |
| 11    | ~9,000        | 290 KB      | + Shell |

---

## Current Status Summary

```
Phase 0: Foundation               ████████████████████ 100%
Phase 1: Core CPU & Memory        ████████████████████ 100%
Phase 2: Virtual Memory           ████████████████████ 100%
Phase 3: Timer & Interrupts       ████████████████████ 100%
Phase 4: Multitasking             ████████████████████ 100%
Phase 5: System Calls             ████████████████████ 100%
Phase 6: User Mode                ████████████████████ 100%
Phase 7: ELF Loader               ████████████████████ 100%
Phase 8: Device Drivers           ░░░░░░░░░░░░░░░░░░░░   0%
Phase 9: VFS                      ░░░░░░░░░░░░░░░░░░░░   0%
Phase 10: Filesystem              ░░░░░░░░░░░░░░░░░░░░   0%
Phase 11: Shell                   ░░░░░░░░░░░░░░░░░░░░   0%

Overall Progress: █████████████░░░░░░░ 66.7%
```

**Next Up:** Phase 8 - Device Drivers (Keyboard & Disk I/O)

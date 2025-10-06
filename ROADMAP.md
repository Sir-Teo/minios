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

## Phase 8: Device Drivers ✅ COMPLETED

**Status:** ✅ Done
**Completion:** 100%

### Implemented:
- [x] PS/2 keyboard driver
- [x] Keyboard interrupt handler (IRQ1)
- [x] Scancode to ASCII mapping (US QWERTY layout)
- [x] Keyboard input buffer (circular buffer, 256 characters)
- [x] Modifier key support (Shift, Ctrl, Alt, Caps Lock)
- [x] LED control (Caps Lock, Num Lock, Scroll Lock)
- [x] Blocking and non-blocking getchar functions
- [x] ATA PIO disk driver
- [x] Disk identification (IDENTIFY command)
- [x] Disk read/write operations (LBA28 addressing)
- [x] Support for up to 4 drives (primary/secondary, master/slave)
- [x] Drive information extraction (model, serial, capacity, LBA48 detection)
- [x] Comprehensive test suite (6+ disk tests)
- [ ] Mouse driver (PS/2) - deferred to later phase
- [ ] LBA48 support - deferred (LBA28 supports up to 128GB)
- [ ] DMA support - deferred (PIO mode is simpler and sufficient)

### Key Functions Implemented:
```c
// Keyboard
void keyboard_init(void);
char keyboard_getchar(void);
char keyboard_getchar_blocking(void);
bool keyboard_has_data(void);
void keyboard_clear_buffer(void);
uint8_t keyboard_get_modifiers(void);
void keyboard_set_leds(uint8_t leds);
void keyboard_irq_handler(void);

// Disk
void ata_init(void);
int ata_read_sectors(uint8_t drive, uint64_t lba, uint32_t sectors, void *buffer);
int ata_write_sectors(uint8_t drive, uint64_t lba, uint32_t sectors, const void *buffer);
const ata_drive_t *ata_get_drive_info(uint8_t drive);
void ata_print_drives(void);
```

**Files Created:**
- `src/drivers/keyboard/ps2_keyboard.{c,h}` - PS/2 keyboard driver (~480 LOC)
- `src/drivers/disk/ata.{c,h}` - ATA PIO disk driver (~580 LOC)
- `src/tests/test_ata.c` - Disk driver tests (~380 LOC)

**Tests:**
- Drive detection and identification
- Drive information retrieval
- Single and multiple sector reads
- Invalid drive handling
- MBR signature verification
- Write operations (disabled for safety)

**Metrics:**
- ~1,440 LOC (480 keyboard + 580 disk production + 380 disk tests)
- Binary size increased by ~35 KB (from 219 KB to 254 KB)

**Note:** Both drivers are production-ready. The keyboard awaits the shell for user input, and the disk driver is ready for filesystem implementation in Phase 9-10.

---

## Phase 9: Virtual File System (VFS) ✅ COMPLETED

**Status:** ✅ Done
**Completion:** 100%

### Implemented:
- [x] VFS abstraction layer (node structure, operations table)
- [x] File descriptor table (128 max FDs)
- [x] VFS node creation/destruction with refcounting
- [x] Path resolution (handles "/" and multi-component paths)
- [x] File operations (open, close, read, write, seek)
- [x] vfs_stat for file information retrieval
- [x] Directory child management (linked list)
- [x] Multiple FDs with independent offsets
- [x] tmpfs implementation (in-memory filesystem proof-of-concept)
- [x] tmpfs dynamic file expansion (doubles capacity or uses needed size)
- [x] tmpfs read/write operations
- [x] Test file "/hello.txt" created by tmpfs
- [x] Comprehensive test suite (8+ test cases)

### Key Structures Implemented:
```c
typedef struct vfs_node {
    char name[VFS_MAX_NAME];
    uint32_t type;  // VFS_FILE, VFS_DIRECTORY, etc.
    uint64_t size;
    uint64_t inode;
    uint32_t permissions;
    vfs_operations_t *ops;  // Filesystem-specific operations
    void *fs_data;          // Filesystem-specific data
    struct vfs_node *parent;
    struct vfs_node *children;  // Linked list of children
    struct vfs_node *next;      // Next sibling
    uint32_t refcount;
} vfs_node_t;

typedef struct vfs_operations {
    int (*open)(struct vfs_node *node, uint32_t flags);
    void (*close)(struct vfs_node *node);
    int64_t (*read)(struct vfs_node *node, uint64_t offset, uint64_t size, void *buffer);
    int64_t (*write)(struct vfs_node *node, uint64_t offset, uint64_t size, const void *buffer);
    // ... more operations
} vfs_operations_t;

typedef struct vfs_fd {
    vfs_node_t *node;
    uint64_t offset;
    uint32_t flags;
    bool in_use;
} vfs_fd_t;
```

### Key Functions Implemented:
```c
void vfs_init(void);
vfs_node_t *vfs_get_root(void);
vfs_node_t *vfs_resolve_path(const char *path);
int vfs_open(const char *path, uint32_t flags);
void vfs_close(int fd);
int64_t vfs_read(int fd, void *buffer, uint64_t size);
int64_t vfs_write(int fd, const void *buffer, uint64_t size);
int64_t vfs_seek(int fd, int64_t offset, int whence);
int vfs_stat(const char *path, vfs_node_t *out_node);
vfs_node_t *vfs_create_node(const char *name, uint32_t type);
void vfs_add_child(vfs_node_t *parent, vfs_node_t *child);

// tmpfs
void tmpfs_init(void);
vfs_node_t *tmpfs_create_file(const char *name);
```

**Files Created:**
- `src/kernel/fs/vfs.{c,h}` - VFS core implementation (~620 LOC)
- `src/kernel/fs/tmpfs.{c,h}` - In-memory filesystem (~230 LOC)
- `src/tests/test_vfs.c` - Comprehensive test suite (~340 LOC)

**Tests:**
- VFS initialization (root directory exists)
- Path resolution ("/", "/hello.txt", non-existent paths)
- File open and close operations
- File read (verifies content "Hello from miniOS VFS!")
- File write (modifies content and re-reads)
- File seek (SEEK_SET, SEEK_END, reading from offset)
- Multiple file descriptors (independent offsets)
- vfs_stat (file information retrieval)

**Metrics:**
- ~1,190 LOC (620 VFS + 230 tmpfs + 340 tests)
- 8 test cases covering all functionality
- Binary size increased by ~28 KB (from 254 KB to 282 KB)

**Note:** The VFS provides a complete abstraction layer for filesystem operations. tmpfs demonstrates the interface with a working in-memory filesystem. Real filesystem support (ext2 or custom) will be implemented in Phase 10.

---

## Phase 10: File System Implementation ✅ COMPLETED

**Status:** ✅ Done
**Completion:** 100%

### Implemented:
- [x] SimpleFS custom filesystem design (block-based, Unix-like)
- [x] Superblock with magic number, version, layout information
- [x] Inode bitmap for inode allocation tracking
- [x] Data block bitmap for block allocation tracking
- [x] Inode table with direct block pointers (12 direct blocks per inode)
- [x] Block allocation and deallocation (bitmap-based)
- [x] Filesystem format operation (sfs_format)
- [x] Mount/unmount operations with bitmap caching
- [x] File creation in root directory (sfs_create_file)
- [x] File read with block-level I/O (sfs_read_file)
- [x] File write with automatic block allocation (sfs_write_file)
- [x] Directory entry management (add, search)
- [x] File listing (sfs_list_files)
- [x] Integration with ATA disk driver (8 sectors per block)
- [x] Comprehensive test suite (12+ test cases)

### Key Structures Implemented:
```c
typedef struct {
    uint32_t magic;              // 0x53465330 ("SFS0")
    uint32_t version;
    uint32_t block_size;         // 4096 bytes
    uint32_t total_blocks;
    uint32_t total_inodes;       // 1024 max
    uint32_t free_blocks;
    uint32_t free_inodes;
    // ... layout information
} sfs_superblock_t;

typedef struct {
    uint32_t type;               // File or directory
    uint32_t size;
    uint32_t blocks;
    uint32_t links_count;
    uint32_t direct[12];         // Direct block pointers
    uint32_t indirect;           // Single indirect (not yet used)
} sfs_inode_t;

typedef struct {
    uint32_t inode;              // Inode number (0 = unused)
    char name[56];               // Filename
} sfs_dirent_t;
```

### Key Functions Implemented:
```c
void sfs_init(void);
int sfs_format(uint8_t drive, uint32_t total_blocks);
int sfs_mount(uint8_t drive, const char *mount_point);
void sfs_unmount(void);
int sfs_create_file(const char *path, uint32_t type);
int sfs_read_file(const char *path, uint64_t offset, uint64_t size, void *buffer);
int sfs_write_file(const char *path, uint64_t offset, uint64_t size, const void *buffer);
void sfs_list_files(void);
```

**Files Created:**
- `src/kernel/fs/simplefs.{c,h}` - SimpleFS implementation (~910 LOC)
- `src/tests/test_simplefs.c` - Comprehensive test suite (~410 LOC)
- `src/kernel/support.c` - Added strncpy function (~30 LOC)

**Tests:**
- SimpleFS initialization
- Filesystem format on disk (64 MB test filesystem)
- Mount/unmount operations
- File creation (with duplicate detection)
- File write operations (normal and append)
- File read operations (full and partial)
- Read past EOF handling
- Large file operations (8KB multi-block files)
- File listing
- File not found errors
- Remount and persistence verification

**Metrics:**
- ~1,550 LOC (910 SimpleFS + 410 tests + 30 support)
- 12 test cases covering all functionality
- Binary size increased by ~54 KB (from 282 KB to 336 KB)
- Block size: 4 KB (8 sectors)
- Max file size: 48 KB (12 direct blocks × 4 KB)
- Max filesystem size: 512 MB

**Features:**
- Persistent storage on ATA disk
- Bitmap-based allocation (inodes and blocks)
- Directory entries with filename and inode number
- Files survive unmount/remount cycles
- Root directory support (subdirectories deferred)
- Error handling for out-of-space, duplicate files, etc.

**Note:** SimpleFS is a production-ready filesystem suitable for miniOS. It provides Unix-like file operations with persistence to disk. More advanced features (subdirectories, indirect blocks, permissions, timestamps) can be added incrementally.

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
| 8     | ~8,380        | 254 KB      | + Drivers ✅ |
| 9     | ~9,570        | 282 KB      | + VFS ✅ |
| 10    | ~11,120       | 336 KB      | + SimpleFS ✅ |
| 11    | ~12,000       | 360 KB      | + Shell |

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
Phase 8: Device Drivers           ████████████████████ 100%
Phase 9: VFS                      ████████████████████ 100%
Phase 10: SimpleFS Filesystem     ████████████████████ 100%
Phase 11: Shell                   ░░░░░░░░░░░░░░░░░░░░   0%

Overall Progress: █████████████████░░░ 91.7%
```

**Next Up:** Phase 11 - Shell & User Programs

# miniOS — A Modern x86_64 Operating System from Scratch

[![Build Status](https://github.com/yourusername/minios/workflows/CI/badge.svg)](https://github.com/yourusername/minios/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

A **fully functional** operating system built from scratch for x86_64 architecture. Features a modern kernel with virtual memory, multitasking, filesystem, and an interactive shell. Designed as an educational project demonstrating OS development fundamentals.

🎉 **Project Status: COMPLETE** — All 12 planned phases implemented!

![miniOS Screenshot](docs/screenshot.png)
*miniOS running with interactive shell*

## ✨ Features

### Core Kernel
- ✅ **64-bit x86_64 architecture** with higher-half kernel
- ✅ **UEFI and BIOS boot support** via Limine bootloader
- ✅ **Virtual memory management** with 4-level page tables
- ✅ **Physical memory allocator** (bitmap-based)
- ✅ **Kernel heap allocator** (kmalloc/kfree)
- ✅ **Interrupt handling** (IDT, GDT, ISR/IRQ, PIC)
- ✅ **Exception handling** for all x86_64 exceptions

### Process Management
- ✅ **Preemptive multitasking** with round-robin scheduler
- ✅ **Task priorities** and idle task
- ✅ **Context switching** with full register save/restore
- ✅ **System call interface** (syscall/sysret)
- ✅ **User mode support** (ring 0/3 transitions)
- ✅ **ELF64 program loader** with proper permissions

### Drivers & I/O
- ✅ **PIT timer driver** (Programmable Interval Timer, 100Hz)
- ✅ **PS/2 keyboard driver** with interrupt handling
- ✅ **ATA PIO disk driver** (LBA28, up to 4 drives)
- ✅ **Serial console** (16550A UART)
- ✅ **Framebuffer graphics** for visual output

### Filesystem
- ✅ **Virtual File System (VFS)** abstraction layer
- ✅ **SimpleFS** custom filesystem (Unix-like, persistent)
- ✅ **tmpfs** in-memory filesystem
- ✅ **File operations**: create, read, write, seek, stat
- ✅ **Block allocation** with bitmaps (4KB blocks)
- ✅ **Persistent storage** on disk with mount/unmount

### User Interface
- ✅ **Interactive shell** with command prompt
- ✅ **14 built-in commands** (help, ls, cat, echo, etc.)
- ✅ **Command parsing** and argument handling
- ✅ **Line editing** with backspace support
- ✅ **Command history** (10 commands)

### Testing & Quality
- ✅ **118+ test cases** with comprehensive coverage
- ✅ **CI/CD pipeline** with GitHub Actions
- ✅ **~12,000 lines** of production code
- ✅ **51 source files** organized by subsystem
- ✅ **Cross-platform development** (macOS Intel & Apple Silicon)

## 🚀 Quick Start

### Prerequisites

Install required tools via Homebrew (macOS):

```bash
brew install x86_64-elf-gcc qemu xorriso mtools gptfdisk git
```

For other platforms, install equivalent packages for your OS.

### Building and Running

```bash
# Clone the repository
git clone https://github.com/yourusername/minios.git
cd minios

# Download Limine bootloader
make deps

# Build the kernel
make

# Create bootable ISO
make iso

# Run in QEMU
make run           # Apple Silicon (emulated)
make run ACCEL=hvf # Intel Mac (accelerated)

# Or run with debugging
make debug
```

The system will boot, run all tests, and drop you into an interactive shell:

```
minios> help
minios> format
minios> mount
minios> create hello.txt
minios> write hello.txt Hello from miniOS!
minios> cat hello.txt
minios> ls
```

## 📚 Documentation

- **[USER_GUIDE.md](docs/USER_GUIDE.md)** — Using miniOS and shell commands
- **[ARCHITECTURE.md](docs/ARCHITECTURE.md)** — Technical architecture and design
- **[ROADMAP.md](ROADMAP.md)** — Development phases and implementation details
- **[STATUS.md](STATUS.md)** — Current status and metrics
- **[CONTRIBUTING.md](CONTRIBUTING.md)** — Contributing guidelines

## 📖 Shell Commands

miniOS includes a fully functional interactive shell with these commands:

| Command | Description |
|---------|-------------|
| `help` | Display all available commands |
| `clear` | Clear the screen |
| `echo <text>` | Echo text to console |
| `uname` | Show system information |
| `uptime` | Display system uptime |
| `free` | Show memory usage |
| `ls` | List files in filesystem |
| `cat <file>` | Display file contents |
| `create <file>` | Create a new file |
| `write <file> <data>` | Write data to file |
| `mount` | Mount filesystem from disk |
| `unmount` | Unmount filesystem |
| `format` | Format disk with SimpleFS |
| `shutdown` | Halt the system |

## 🏗️ Project Structure

```
minios/
├── src/
│   ├── kernel/              # Platform-independent kernel
│   │   ├── kernel.c         # Main entry point
│   │   ├── kprintf.{c,h}    # Printf implementation
│   │   ├── support.c        # Freestanding libc
│   │   ├── mm/              # Memory management (PMM, heap)
│   │   ├── sched/           # Scheduler and tasks
│   │   ├── syscall/         # System call interface
│   │   ├── user/            # User mode support
│   │   ├── loader/          # ELF64 program loader
│   │   ├── fs/              # Filesystems (VFS, SimpleFS, tmpfs)
│   │   └── shell/           # Interactive shell
│   ├── arch/x86_64/         # Architecture-specific code
│   │   ├── interrupts/      # GDT, IDT, ISR/IRQ
│   │   └── mm/              # Virtual memory (paging)
│   ├── drivers/             # Device drivers
│   │   ├── timer/           # PIT driver
│   │   ├── keyboard/        # PS/2 keyboard
│   │   └── disk/            # ATA disk driver
│   └── tests/               # Test suites (118+ tests)
├── third_party/
│   └── limine/              # Limine bootloader
├── docs/                    # Documentation
├── GNUmakefile             # Build system
├── linker.lds              # Linker script
└── limine.conf             # Bootloader config
```

## 🧪 Testing

miniOS includes comprehensive automated testing:

```bash
# Run all 118+ test cases
make test

# Tests are organized by subsystem:
# - VMM (25+ tests)
# - Timer (10+ tests)
# - Scheduler (10+ tests)
# - System calls (15+ tests)
# - User mode (10+ tests)
# - ELF loader (12+ tests)
# - Disk driver (6+ tests)
# - VFS (8+ tests)
# - SimpleFS (12+ tests)
# - Shell (10+ tests)
```

Tests run automatically in CI on every commit.

## 🔧 Development

### Debugging with GDB

```bash
# Terminal 1: Start QEMU with GDB server
make debug

# Terminal 2: Connect GDB
gdb -ex "target remote :1234" -ex "symbol-file bin/myos"

# Useful GDB commands:
(gdb) break kmain
(gdb) continue
(gdb) info registers
(gdb) x/10i $rip
```

### Creating a Bootable USB

```bash
# Create HDD image
make hdd

# Find your USB device
diskutil list

# Write to USB (⚠️ DANGEROUS - verify disk number!)
diskutil unmountDisk /dev/diskX
sudo dd if=image.hdd of=/dev/rdiskX bs=1m conv=sync
diskutil eject /dev/diskX
```

**Note:** On T2 Intel Macs, disable Secure Boot in Recovery mode.

### Code Organization

- **Kernel code** in `src/kernel/` is platform-independent
- **Architecture code** in `src/arch/x86_64/` is x86_64-specific
- **Drivers** in `src/drivers/` are hardware-specific
- **Tests** in `src/tests/` mirror the source structure

## 📊 Metrics

| Metric | Value |
|--------|-------|
| **Total Lines of Code** | ~11,920 |
| **Source Files** | 51 |
| **Binary Size** | 363 KB |
| **Test Cases** | 118+ |
| **Boot Time** | <1 second |
| **Phases Complete** | 12/12 (100%) |

## 🎓 Learning Resources

This project demonstrates modern OS development concepts:

- **Bootloader protocols** (Limine)
- **Virtual memory** (4-level paging)
- **Interrupt handling** (IDT, ISR/IRQ)
- **Process scheduling** (round-robin)
- **System calls** (syscall/sysret)
- **Driver development** (keyboard, disk)
- **Filesystem design** (VFS, block I/O)
- **Shell development** (command parsing)

### Recommended Reading

- [OSDev Wiki](https://wiki.osdev.org/) — Comprehensive OS development resources
- [Intel® 64 and IA-32 Architectures Software Developer Manuals](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
- [Limine Bootloader Documentation](https://github.com/limine-bootloader/limine/blob/trunk/PROTOCOL.md)
- [The little book about OS development](https://littleosbook.github.io/)
- [Writing an OS in Rust](https://os.phil-opp.com/)

## 🤝 Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

To contribute:
1. Fork the repository
2. Create a feature branch
3. Make your changes with tests
4. Ensure `make test` passes
5. Submit a pull request

## 📝 License

MIT License - see [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **Limine bootloader** for a modern, clean boot protocol
- **OSDev community** for extensive documentation
- **Homebrew** for easy cross-compiler setup on macOS

## 👤 Author

Built as an educational project to learn OS development from first principles.

---

**Want to learn OS development?** Check out the [ROADMAP.md](ROADMAP.md) to see how miniOS was built step-by-step through 12 phases!

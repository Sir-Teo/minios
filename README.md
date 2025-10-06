# miniOS — A Modern x86_64 Hobby Operating System

[![CI](https://github.com/yourusername/minios/workflows/CI/badge.svg)](https://github.com/yourusername/minios/actions)

A minimal, modern operating system built from scratch using the Limine bootloader. Designed for learning and experimentation with OS development on macOS.

**Current Status:** Basic kernel with memory management, interrupts, and hardware initialization complete.

## Features

### ✅ Implemented
- **64-bit x86_64 architecture** with higher-half kernel
- **UEFI and BIOS boot support** via Limine bootloader
- **Framebuffer graphics** with serial console output
- **Physical memory allocator** (bitmap-based page frame allocator)
- **Kernel heap** (simple bump allocator, expandable to slab allocator)
- **Interrupt handling** (IDT, GDT, ISR/IRQ handlers, exceptions)
- **Global Descriptor Table (GDT)** with TSS for ring transitions
- **CI/CD pipeline** with GitHub Actions for automated builds
- **Cross-platform development** on macOS (Intel and Apple Silicon)
- **Comprehensive logging** via serial port

### 🚧 In Progress / Planned
- Virtual memory management (page tables, vmm)
- APIC timer for scheduling
- Preemptive multitasking and scheduler
- System call interface (syscall/sysret)
- User mode support (ring 3)
- Keyboard and mouse drivers
- VFS and filesystem support
- Network stack

## Quick Start

### Prerequisites

Install the required tools via Homebrew:

```bash
brew install x86_64-elf-gcc qemu xorriso mtools gptfdisk git
```

### Building

```bash
# Clone Limine bootloader and dependencies
make deps

# Build the kernel
make

# Create bootable ISO
make iso

# Run in QEMU
# On Intel Mac (with hardware acceleration):
make run ACCEL=hvf

# On Apple Silicon (x86_64 emulation):
make run
```

## Makefile Targets

- `make deps` — Clone Limine into `third_party/limine` and copy `limine.h`
- `make` — Build the kernel ELF to `bin/myos`
- `make iso` — Create a BIOS+UEFI hybrid ISO image
- `make hdd` — Create a bootable HDD/USB image
- `make run` — Run the ISO in QEMU (use `ACCEL=hvf` on Intel Macs)
- `make debug` — Run with GDB server enabled
- `make test` — Run automated tests
- `make clean` — Remove all build artifacts

## Project Structure

```
minios/
├── .github/
│   └── workflows/
│       └── ci.yml           # CI/CD pipeline
├── src/
│   ├── kernel/
│   │   ├── kernel.c         # Main kernel entry point
│   │   ├── support.c        # Freestanding libc functions
│   │   └── limine.h         # Limine protocol header
│   ├── arch/
│   │   └── x86_64/
│   │       ├── interrupts/  # IDT, GDT, exceptions
│   │       ├── mm/          # Paging and memory management
│   │       └── boot/        # Architecture-specific boot code
│   ├── kernel/
│   │   ├── mm/              # Physical allocator, heap
│   │   ├── sched/           # Scheduler and task management
│   │   ├── syscall/         # System call interface
│   │   └── drivers/         # Device drivers
│   └── tests/               # Kernel tests
├── third_party/
│   └── limine/              # Limine bootloader (auto-cloned)
├── GNUmakefile              # Build system
├── linker.lds               # Linker script
├── limine.conf              # Bootloader configuration
└── README.md
```

## Development

### Running with GDB

```bash
# Terminal 1: Start QEMU with GDB server
make debug

# Terminal 2: Connect GDB
gdb -ex "target remote :1234" -ex "symbol-file bin/myos"
```

### Creating a Bootable USB (Intel Macs)

```bash
# Create HDD image
make hdd

# Find your USB device (e.g., /dev/disk3)
diskutil list

# Unmount and write (⚠️ DANGEROUS - verify disk number!)
diskutil unmountDisk /dev/disk3
sudo dd if=image.hdd of=/dev/rdisk3 bs=1m conv=sync
diskutil eject /dev/disk3
```

**Note:** On T2 Intel Macs, you must allow external boot in Recovery's Startup Security Utility.

### Cross-Compilation

The project uses `x86_64-elf-gcc` as a cross-compiler to produce freestanding ELF binaries. This is necessary because macOS's default compiler produces Mach-O binaries incompatible with bootloaders.

## Architecture

### Boot Process

1. **Firmware** (UEFI/BIOS) loads Limine bootloader
2. **Limine** parses `limine.conf` and loads the kernel
3. **Kernel** receives boot information via Limine protocol
4. **Initialization** sets up framebuffer, serial, memory, interrupts
5. **User mode** transitions to ring 3 and executes init process

### Memory Layout

```
0xFFFFFFFF80000000  ┌─────────────────┐
                    │  Kernel Code    │
                    ├─────────────────┤
                    │  Kernel Data    │
                    ├─────────────────┤
                    │  Kernel Heap    │
                    ├─────────────────┤
                    │  Page Tables    │
                    └─────────────────┘
...
0x0000000000000000  ┌─────────────────┐
                    │  User Space     │
                    └─────────────────┘
```

## Testing

The project includes automated tests that run in QEMU using the `isa-debug-exit` device for clean shutdown with exit codes.

```bash
# Run all tests
make test
```

Tests are automatically run in CI on every push.

## Contributing

This is a learning project, but contributions are welcome! Please ensure:

- Code follows the existing style
- Tests pass (`make test`)
- CI pipeline succeeds
- Commits are atomic and well-documented

## Resources

- [Limine Bootloader](https://github.com/limine-bootloader/limine)
- [OSDev Wiki](https://wiki.osdev.org/)
- [The little book about OS development](https://littleosbook.github.io/)
- [Writing an OS in Rust](https://os.phil-opp.com/)
- [Operating Systems: From 0 to 1](https://github.com/tuhdo/os01)

## License

MIT License - see LICENSE file for details.

## Author

Built as a learning project following modern OS development best practices.

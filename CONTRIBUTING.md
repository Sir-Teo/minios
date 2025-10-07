# Contributing to miniOS

Thank you for your interest in contributing to miniOS! This document provides guidelines for contributing to the project.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Workflow](#development-workflow)
- [Coding Standards](#coding-standards)
- [Testing Requirements](#testing-requirements)
- [Submitting Changes](#submitting-changes)
- [Project Structure](#project-structure)
- [Areas for Contribution](#areas-for-contribution)

---

## Code of Conduct

### Our Standards

- **Be respectful**: Treat all contributors with respect and professionalism
- **Be constructive**: Provide helpful feedback and suggestions
- **Be collaborative**: Work together to improve the project
- **Be patient**: Remember that this is an educational project

### Unacceptable Behavior

- Harassment, discrimination, or offensive comments
- Trolling, insulting, or derogatory remarks
- Personal or political attacks
- Publishing others' private information

---

## Getting Started

### Prerequisites

Before contributing, ensure you have the required tools installed:

```bash
# macOS (via Homebrew)
brew install x86_64-elf-gcc qemu xorriso mtools gptfdisk git

# Linux (Ubuntu/Debian)
sudo apt install build-essential qemu-system-x86 xorriso mtools gdisk git

# Build cross-compiler from source if package not available
# See: https://wiki.osdev.org/GCC_Cross-Compiler
```

### Fork and Clone

1. Fork the repository on GitHub
2. Clone your fork locally:

```bash
git clone https://github.com/YOUR_USERNAME/minios.git
cd minios
```

3. Add upstream remote:

```bash
git remote add upstream https://github.com/ORIGINAL_OWNER/minios.git
```

### Build and Test

```bash
# Download dependencies (Limine bootloader)
make deps

# Build the kernel
make

# Create bootable ISO
make iso

# Run in QEMU
make run

# Run with debugging enabled
make debug

# Clean build artifacts
make clean
```

---

## Development Workflow

### 1. Create a Feature Branch

```bash
git checkout -b feature/your-feature-name
```

Use descriptive branch names:
- `feature/network-stack` - New features
- `bugfix/timer-overflow` - Bug fixes
- `docs/architecture-guide` - Documentation
- `test/vfs-edge-cases` - Test improvements

### 2. Make Your Changes

- Write clean, well-documented code
- Follow coding standards (see below)
- Add tests for new functionality
- Update documentation as needed

### 3. Test Your Changes

```bash
# Build and run
make run

# Check that all tests pass
# (Tests run automatically on boot)

# Test specific scenarios manually
# Use the interactive shell to verify functionality
```

### 4. Commit Your Changes

```bash
git add .
git commit -m "Add feature: brief description"
```

**Commit Message Guidelines**:
- Use present tense ("Add feature" not "Added feature")
- Use imperative mood ("Move cursor to..." not "Moves cursor to...")
- First line: brief summary (50 chars or less)
- Blank line, then detailed description if needed
- Reference issues: "Fixes #123" or "Related to #456"

**Examples**:
```
Add network stack with TCP/IP support

Implements a basic TCP/IP stack with:
- Ethernet driver for E1000
- ARP, IP, TCP protocol handlers
- Socket API for user programs

Fixes #45
```

```
Fix timer overflow in PIT driver

The PIT tick counter was overflowing after ~500 days.
Changed to 64-bit counter to handle longer uptimes.

Related to #78
```

### 5. Push to Your Fork

```bash
git push origin feature/your-feature-name
```

### 6. Create Pull Request

1. Go to your fork on GitHub
2. Click "New Pull Request"
3. Select your feature branch
4. Fill out the PR template (see below)
5. Submit the PR

---

## Coding Standards

### C Code Style

**Indentation**:
- Use **4 spaces** (not tabs)
- Indent function bodies, loops, conditionals

**Braces**:
```c
// K&R style for functions
void function_name(int arg) {
    // function body
}

// Same-line braces for control structures
if (condition) {
    // code
} else {
    // code
}

for (int i = 0; i < n; i++) {
    // code
}
```

**Naming Conventions**:
```c
// Functions: lowercase with underscores
void vmm_map_page(addr_space_t *as, uint64_t virt, uint64_t phys);

// Variables: lowercase with underscores
uint64_t page_table_entry;
int file_descriptor;

// Constants: uppercase with underscores
#define MAX_TASKS 256
#define PAGE_SIZE 4096

// Struct types: lowercase_t suffix
typedef struct task task_t;
typedef struct addr_space addr_space_t;

// Enums: lowercase with underscores
typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED
} task_state_t;
```

**Comments**:
```c
/**
 * Brief description of function
 *
 * Detailed description if needed. Explain parameters,
 * return values, side effects, and preconditions.
 *
 * @param as    Address space to modify
 * @param virt  Virtual address to map
 * @param phys  Physical address to map to
 * @param flags Page flags (PTE_WRITABLE, PTE_USER, etc.)
 * @return 0 on success, -1 on error
 */
int vmm_map_page(addr_space_t *as, uint64_t virt, uint64_t phys, uint64_t flags);

// Single-line comments for clarification
counter++;  // Increment tick counter
```

**Function Length**:
- Keep functions focused and small (<100 lines preferred)
- Extract complex logic into helper functions
- One function, one purpose

**Error Handling**:
```c
// Return -1 on error, 0 on success (or positive value)
int function_that_can_fail(void) {
    if (error_condition) {
        return -1;
    }
    return 0;
}

// Check return values
if (vmm_map_page(as, virt, phys, flags) != 0) {
    kprintf("Error: failed to map page\n");
    return -1;
}
```

### Assembly Code Style

**File Extension**: `.S` (uppercase, for C preprocessor support)

**Syntax**: AT&T syntax (default for GCC)

**Labels**:
```asm
.global function_name
function_name:
    ; function body
    ret
```

**Comments**:
```asm
; Single-line comments for assembly
mov rax, rbx        ; Copy RBX to RAX
```

### Header Files

**Include Guards**:
```c
#ifndef SUBSYSTEM_MODULE_H
#define SUBSYSTEM_MODULE_H

// header contents

#endif  // SUBSYSTEM_MODULE_H
```

**Organization**:
```c
#ifndef VMM_H
#define VMM_H

#include <stdint.h>

// Constants
#define PAGE_SIZE 4096

// Type definitions
typedef struct addr_space addr_space_t;

// Function declarations
addr_space_t *vmm_create_address_space(void);
void vmm_destroy_address_space(addr_space_t *as);

#endif  // VMM_H
```

---

## Testing Requirements

### Test Coverage

**All new features must include tests**. Add tests to the appropriate file in `src/tests/`:

```c
// In src/tests/test_myfeature.c
void test_myfeature_basic(void) {
    kprintf("[TEST] Testing basic functionality...\n");

    // Setup
    int result = my_function(arg);

    // Verify
    if (result != expected) {
        kprintf("[FAIL] Expected %d, got %d\n", expected, result);
        return;
    }

    kprintf("[PASS] Basic functionality works\n");
}

void run_myfeature_tests(void) {
    kprintf("\n========================================\n");
    kprintf("Running My Feature Tests\n");
    kprintf("========================================\n");

    test_myfeature_basic();
    test_myfeature_edge_cases();
    test_myfeature_error_handling();

    kprintf("[TEST] My Feature: All tests passed!\n");
}
```

### Test Types

1. **Unit Tests**: Test individual functions in isolation
2. **Integration Tests**: Test interactions between subsystems
3. **Regression Tests**: Prevent previously-fixed bugs from returning

### Test Execution

- Tests run automatically on boot (see `src/kernel/kernel.c`)
- All tests must pass before submitting PR
- Add your test runner to `kernel.c` if creating new test file

```c
// In src/kernel/kernel.c
extern void run_myfeature_tests(void);

void kmain(void) {
    // ... other initialization

    run_myfeature_tests();  // Add your test runner

    // ... continue initialization
}
```

---

## Submitting Changes

### Pull Request Template

When creating a PR, include:

```markdown
## Description
Brief description of what this PR does.

## Type of Change
- [ ] Bug fix (non-breaking change which fixes an issue)
- [ ] New feature (non-breaking change which adds functionality)
- [ ] Breaking change (fix or feature that would cause existing functionality to not work as expected)
- [ ] Documentation update

## Changes Made
- Bullet point list of changes
- Be specific about what was modified

## Testing
- [ ] All existing tests pass
- [ ] Added tests for new functionality
- [ ] Tested manually in QEMU

## Test Results
Paste relevant test output showing tests pass.

## Checklist
- [ ] Code follows project style guidelines
- [ ] Self-reviewed code
- [ ] Commented code, particularly in hard-to-understand areas
- [ ] Updated documentation (if needed)
- [ ] No new compiler warnings
- [ ] Added tests that prove fix/feature works
- [ ] New and existing tests pass

## Related Issues
Fixes #(issue number)
```

### Review Process

1. **Automated Checks**: CI/CD will build and test your PR automatically
2. **Code Review**: Maintainers will review your code for:
   - Correctness
   - Style compliance
   - Test coverage
   - Documentation
3. **Revisions**: Address any requested changes
4. **Approval**: Once approved, your PR will be merged

### Merge Strategy

- **Squash and merge**: Multiple commits will be squashed into one
- **Commit message**: Will use PR title and description
- **Branch cleanup**: Your branch can be deleted after merge

---

## Project Structure

### Key Directories

```
src/
├── kernel/           # Platform-independent kernel code
│   ├── mm/           # Memory management
│   ├── sched/        # Scheduler
│   ├── syscall/      # System calls
│   ├── user/         # User mode support
│   ├── loader/       # ELF loader
│   ├── fs/           # Filesystems
│   └── shell/        # Shell
├── arch/x86_64/      # Architecture-specific code
│   ├── interrupts/   # GDT, IDT
│   └── mm/           # VMM (paging)
├── drivers/          # Device drivers
│   ├── timer/        # PIT driver
│   ├── keyboard/     # PS/2 keyboard
│   └── disk/         # ATA driver
└── tests/            # Test suites
```

### File Naming

- **C source**: `module.c` (lowercase)
- **C header**: `module.h` (lowercase)
- **Assembly**: `module.S` (uppercase .S)
- **Tests**: `test_module.c` (lowercase with test_ prefix)

---

## Areas for Contribution

### Easy Tasks (Good First Issues)

- **Documentation**: Improve comments, fix typos, add examples
- **Tests**: Add edge case tests for existing functionality
- **Shell commands**: Add new built-in commands
- **Error messages**: Improve error reporting and messages

### Medium Tasks

- **Filesystem improvements**: Add subdirectory support to SimpleFS
- **Driver development**: Add support for more hardware (RTC, CMOS, etc.)
- **Shell features**: Command history with arrow keys, tab completion
- **Memory optimization**: Improve heap allocator efficiency

### Advanced Tasks

- **Multicore support (SMP)**: APIC, per-CPU data structures, spinlocks
- **Networking**: E1000 driver, TCP/IP stack, socket API
- **GUI**: Framebuffer console, window manager, graphics library
- **Advanced filesystems**: ext2 read/write support, FAT32 compatibility

### Bug Fixes

Check the [Issues](https://github.com/ORIGINAL_OWNER/minios/issues) page for:
- Bug reports
- Feature requests
- Enhancement ideas

---

## Building on Different Platforms

### macOS

```bash
# Install dependencies
brew install x86_64-elf-gcc qemu xorriso mtools gptfdisk

# Build
make

# Run (Apple Silicon)
make run

# Run (Intel Mac with acceleration)
make run ACCEL=hvf
```

### Linux

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install build-essential qemu-system-x86 xorriso mtools gdisk

# Build cross-compiler if needed
# See: https://wiki.osdev.org/GCC_Cross-Compiler

# Build
make

# Run
make run
```

### Windows (WSL)

```bash
# Install WSL2 with Ubuntu
# Then follow Linux instructions above

# Note: GUI may require X server (VcXsrv, Xming)
```

---

## Debugging Tips

### Using GDB

```bash
# Terminal 1: Start QEMU with GDB server
make debug

# Terminal 2: Connect GDB
gdb -ex "target remote :1234" -ex "symbol-file bin/myos"

# Useful commands
(gdb) break kmain
(gdb) continue
(gdb) info registers
(gdb) x/10i $rip
(gdb) backtrace
```

### Serial Output

All kernel messages go to serial console (COM1):

```bash
# QEMU redirects serial to stdio by default
make run

# Or save to file
make run 2>&1 | tee boot.log
```

### Common Issues

**Build fails with "x86_64-elf-gcc not found"**:
- Install cross-compiler or add to PATH
- On macOS: `brew install x86_64-elf-gcc`

**QEMU hangs at boot**:
- Check serial output for kernel panic
- Try with `-serial stdio -no-reboot` flags

**Tests fail unexpectedly**:
- Check for memory corruption (use VMM tests)
- Verify hardware initialization order
- Look for race conditions in multitasking code

---

## Resources

### Learning

- [OSDev Wiki](https://wiki.osdev.org/) - Comprehensive OS development guide
- [Intel SDM](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) - x86_64 reference manuals
- [Limine Protocol](https://github.com/limine-bootloader/limine/blob/trunk/PROTOCOL.md) - Bootloader documentation

### Project Documentation

- [README.md](README.md) - Project overview
- [ROADMAP.md](ROADMAP.md) - Development phases
- [docs/USER_GUIDE.md](docs/USER_GUIDE.md) - User manual
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) - Technical architecture

---

## Questions?

If you have questions about contributing:

1. Check existing documentation (README, ROADMAP, USER_GUIDE, ARCHITECTURE)
2. Search [Issues](https://github.com/ORIGINAL_OWNER/minios/issues) for similar questions
3. Open a new issue with the "question" label
4. Be specific and include context

---

## License

By contributing to miniOS, you agree that your contributions will be licensed under the MIT License (see [LICENSE](LICENSE) file).

---

Thank you for contributing to miniOS! Your contributions help make this project a valuable learning resource for OS development.

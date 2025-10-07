# miniOS User Guide

Welcome to miniOS! This guide will help you get started using the operating system and its interactive shell.

## Table of Contents

- [Getting Started](#getting-started)
- [Booting miniOS](#booting-minios)
- [Using the Shell](#using-the-shell)
- [Filesystem Operations](#filesystem-operations)
- [Command Reference](#command-reference)
- [Troubleshooting](#troubleshooting)

## Getting Started

### System Requirements

- **Emulator**: QEMU (recommended for testing)
- **Real Hardware**: x86_64 CPU with UEFI or BIOS support
- **Memory**: Minimum 64 MB RAM
- **Disk**: Optional (for persistent storage)

### Running miniOS

The easiest way to run miniOS is in QEMU:

```bash
# Build and run
make run

# Or with hardware acceleration (Intel Mac)
make run ACCEL=hvf
```

miniOS will boot, run automated tests, and present you with a shell prompt:

```
========================================
       Welcome to miniOS Shell!
========================================

Type 'help' for a list of commands

minios>
```

## Booting miniOS

### Boot Process

1. **Bootloader** (Limine) loads the kernel from disk
2. **Kernel initialization**:
   - Sets up memory management
   - Initializes interrupt handlers
   - Loads device drivers
   - Runs automated tests
3. **Shell** starts automatically

You'll see detailed boot messages on the serial console showing each initialization step.

### Boot Options

miniOS uses Limine bootloader with the following default configuration:

- **Timeout**: 0 seconds (boots immediately)
- **Protocol**: Limine protocol
- **Resolution**: Auto-detected framebuffer

To modify boot options, edit `limine.conf` before building the ISO.

## Using the Shell

### Shell Basics

The shell is your primary interface to miniOS. It features:

- **Command prompt**: `minios> `
- **Command parsing**: Space-separated arguments
- **Line editing**: Backspace to delete characters
- **Command history**: Previous commands stored (10 max)

### Entering Commands

Type a command and press **Enter** to execute:

```
minios> echo Hello World
Hello World

minios> uname
miniOS x86_64 v0.11.0
A modern operating system from scratch
```

### Command Syntax

```
command [argument1] [argument2] ...
```

- **Commands** are case-sensitive
- **Arguments** are separated by spaces
- **Multiple spaces** are ignored
- **Tab completion**: Not yet implemented

### Special Keys

| Key | Action |
|-----|--------|
| **Enter** | Execute command |
| **Backspace** | Delete last character |
| **Arrow keys** | Not yet supported |
| **Ctrl+C** | Not yet supported |

## Filesystem Operations

miniOS includes a custom filesystem called **SimpleFS** with persistent storage.

### Setting Up the Filesystem

Before using filesystem commands, you need to format and mount a disk:

```bash
# 1. Format the disk (creates SimpleFS)
minios> format

# 2. Mount the filesystem
minios> mount
```

**Warning**: `format` erases all data on drive 0!

### Creating Files

```bash
# Create a new file
minios> create myfile.txt

# Create with a different name
minios> create data.bin
```

### Writing to Files

```bash
# Write data to a file
minios> write myfile.txt Hello from miniOS!

# Write multiple words
minios> write notes.txt This is a test file
```

**Note**: `write` replaces the entire file contents (no append yet).

### Reading Files

```bash
# Display file contents
minios> cat myfile.txt
Hello from miniOS!

# Read another file
minios> cat notes.txt
This is a test file
```

### Listing Files

```bash
# List all files in the filesystem
minios> ls
[SIMPLEFS] Files in root directory:
[SIMPLEFS]   FILE         23 bytes  myfile.txt
[SIMPLEFS]   FILE         19 bytes  notes.txt
```

### Unmounting

```bash
# Safely unmount the filesystem
minios> unmount
```

After unmounting, you need to `mount` again to access files.

## Command Reference

### System Information

#### `help`
Display all available commands with descriptions.

```
minios> help
miniOS Shell - Built-in Commands:
  help              - Display this help message
  clear             - Clear the screen
  ...
```

#### `uname`
Show system information.

```
minios> uname
miniOS x86_64 v0.11.0
A modern operating system from scratch
```

#### `uptime`
Display system uptime.

```
minios> uptime
Uptime: 0:02:15 (13500 ticks)
```

#### `free`
Show memory usage statistics.

```
minios> free
Memory:
  Total: 512 MiB
  Used:  48 MiB
  Free:  464 MiB
```

### Utility Commands

#### `echo <text>`
Echo arguments to the console.

```
minios> echo Hello World
Hello World

minios> echo Testing 1 2 3
Testing 1 2 3
```

#### `clear`
Clear the screen using ANSI escape codes.

```
minios> clear
```

**Note**: May not work on all terminals.

### Filesystem Commands

#### `format`
Format drive 0 with SimpleFS (64 MB filesystem).

```
minios> format
WARNING: This will erase all data on drive 0!
Formatting drive 0 with SimpleFS...
Format complete!
Use 'mount' to mount the filesystem
```

**Warning**: Destructive operation! All data will be lost.

#### `mount`
Mount the SimpleFS filesystem from drive 0.

```
minios> mount
[SIMPLEFS] Mounting drive 0 at /disk...
[SIMPLEFS] Filesystem info:
[SIMPLEFS]   Version: 1
[SIMPLEFS]   Block size: 4096 bytes
[SIMPLEFS]   Total blocks: 16384
[SIMPLEFS]   Free blocks: 16368
Filesystem mounted successfully
```

#### `unmount`
Unmount the currently mounted filesystem.

```
minios> unmount
Filesystem unmounted
```

#### `ls`
List all files in the root directory.

```
minios> ls
[SIMPLEFS] Files in root directory:
[SIMPLEFS]   FILE          23 bytes  hello.txt
[SIMPLEFS]   FILE         100 bytes  data.bin
```

Requires a mounted filesystem.

#### `cat <file>`
Display the contents of a file.

```
minios> cat hello.txt
Hello from miniOS!
```

- File paths starting with `/` are used as-is
- Paths without `/` get `/` prepended automatically
- Requires a mounted filesystem

#### `create <file>`
Create a new empty file.

```
minios> create test.txt
Created file: /test.txt
```

Fails if the file already exists.

#### `write <file> <data>`
Write data to a file (replaces existing contents).

```
minios> write test.txt This is my data
Wrote 15 bytes to /test.txt
```

All arguments after the filename are concatenated with spaces.

### System Control

#### `shutdown`
Halt the system.

```
minios> shutdown
Shutting down miniOS...
Goodbye!
```

The system will halt (infinite loop with `hlt` instruction).

## Troubleshooting

### Common Issues

#### "No filesystem mounted"

**Problem**: Trying to use `ls`, `cat`, etc. without a mounted filesystem.

**Solution**:
```
minios> mount
```

If mount fails, you may need to format first:
```
minios> format
minios> mount
```

#### "Cannot read file"

**Problem**: File doesn't exist or filesystem not mounted.

**Solutions**:
- Check if filesystem is mounted: `mount`
- List files to verify the file exists: `ls`
- Check filename spelling

#### "Cannot create file"

**Problem**: File already exists or filesystem is full.

**Solutions**:
- Use a different filename
- Check available space: `ls` shows file count
- Maximum 1024 inodes (files)

#### Keyboard Not Working

**Problem**: No response to keyboard input.

**Solution**: Ensure you're using QEMU with proper settings:
```
make run
```

Real hardware should work with PS/2 keyboards.

### Filesystem Limits

SimpleFS has the following limitations:

| Limit | Value |
|-------|-------|
| **Max file size** | 48 KB (12 blocks × 4 KB) |
| **Max files** | 1024 inodes |
| **Block size** | 4 KB (8 sectors) |
| **Max filesystem size** | 512 MB |
| **Directory depth** | 1 (root only, no subdirectories yet) |

### Tips & Tricks

1. **Create a test workflow**:
   ```
   format
   mount
   create test.txt
   write test.txt Hello World
   cat test.txt
   ls
   ```

2. **Check system resources**:
   ```
   uptime
   free
   ```

3. **Test file persistence**:
   ```
   # Create and write
   write data.txt persistent data
   unmount

   # Remount and read
   mount
   cat data.txt
   ```

4. **Use descriptive filenames**: SimpleFS supports up to 55 characters

## Examples

### Example Session 1: Basic File Operations

```
minios> format
Format complete!

minios> mount
Filesystem mounted successfully

minios> create hello.txt
Created file: /hello.txt

minios> write hello.txt Hello from miniOS!
Wrote 18 bytes to /hello.txt

minios> cat hello.txt
Hello from miniOS!

minios> ls
[SIMPLEFS] Files in root directory:
[SIMPLEFS]   FILE          18 bytes  hello.txt
```

### Example Session 2: Multiple Files

```
minios> create file1.txt
Created file: /file1.txt

minios> create file2.txt
Created file: /file2.txt

minios> write file1.txt First file
Wrote 10 bytes to /file1.txt

minios> write file2.txt Second file
Wrote 11 bytes to /file2.txt

minios> ls
[SIMPLEFS] Files in root directory:
[SIMPLEFS]   FILE          10 bytes  file1.txt
[SIMPLEFS]   FILE          11 bytes  file2.txt

minios> cat file1.txt
First file

minios> cat file2.txt
Second file
```

### Example Session 3: System Information

```
minios> uname
miniOS x86_64 v0.11.0
A modern operating system from scratch

minios> uptime
Uptime: 0:01:30 (9000 ticks)

minios> free
Memory:
  Total: 512 MiB
  Used:  45 MiB
  Free:  467 MiB

minios> echo System check complete
System check complete
```

## Next Steps

- Explore the [ARCHITECTURE.md](ARCHITECTURE.md) to understand how miniOS works internally
- Check [ROADMAP.md](../ROADMAP.md) to see how the system was built
- Read the source code in `src/kernel/shell/` to see the shell implementation

Enjoy using miniOS!

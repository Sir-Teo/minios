# GNUmakefile — build a freestanding x86_64 kernel with Limine boot
.SUFFIXES:

# Toolchain (Homebrew provides x86_64-elf-gcc)
TOOLCHAIN_PREFIX ?= x86_64-elf-
CC      := $(TOOLCHAIN_PREFIX)gcc
LD      := $(TOOLCHAIN_PREFIX)ld
OBJCOPY := $(TOOLCHAIN_PREFIX)objcopy
AR      := $(TOOLCHAIN_PREFIX)ar

# Output
OUTPUT := myos
BIN    := bin/$(OUTPUT)

# Sources (organized by subsystem)
KERNEL_SRC := $(wildcard src/kernel/*.c) $(wildcard src/kernel/*/*.c)
ARCH_SRC   := $(wildcard src/arch/x86_64/*.c) $(wildcard src/arch/x86_64/*.S) \
              $(wildcard src/arch/x86_64/*/*.c) $(wildcard src/arch/x86_64/*/*.S)
DRIVERS_SRC := $(wildcard src/drivers/*.c)
TEST_SRC    := $(wildcard src/tests/*.c)

SRC := $(KERNEL_SRC) $(ARCH_SRC) $(DRIVERS_SRC)

# Objects
OBJ := $(patsubst src/%.c,obj/%.o,$(filter %.c,$(SRC))) \
       $(patsubst src/%.S,obj/%.o,$(filter %.S,$(SRC)))
DEPS := $(OBJ:.o=.d)

# Flags for freestanding kernel build
CFLAGS := -g -O2 -pipe \
  -Wall -Wextra -Wshadow -Wundef -Wwrite-strings -Wredundant-decls -Wdisabled-optimization \
  -Wdouble-promotion -Wformat=2 -Winit-self -Wmissing-include-dirs \
  -Wstrict-overflow=5 -Wswitch-default -Wundef -Wno-unused \
  -std=gnu11 -ffreestanding -fno-stack-protector -fno-stack-check -fno-common \
  -m64 -march=x86-64 -mno-80387 -mno-mmx -mno-sse -mno-sse2 \
  -mno-red-zone -mcmodel=kernel -fno-pic -fno-pie \
  -ffunction-sections -fdata-sections

CPPFLAGS := -Isrc -Isrc/kernel -DLIMINE_API_REVISION=3 -MMD -MP

LDFLAGS := -nostdlib -static -z max-page-size=0x1000 --gc-sections \
  -T linker.lds -m elf_x86_64

# QEMU settings
QEMU       := qemu-system-x86_64
QEMU_FLAGS := -m 512 -serial stdio -no-reboot -no-shutdown
QEMU_DEBUG_FLAGS := -s -S

# Add hardware acceleration if ACCEL is set (e.g., ACCEL=hvf on Intel Mac)
ifneq ($(ACCEL),)
QEMU_FLAGS += -accel $(ACCEL)
endif

# Limine paths
LIMINE_DIR := third_party/limine
ISO_ROOT   := iso_root
ISO        := image.iso
HDD        := image.hdd

# Phonies
.PHONY: all clean deps iso run debug hdd test test-run check

all: $(BIN)

# Build rules
$(BIN): $(OBJ) linker.lds
	@mkdir -p $(@D)
	$(LD) $(LDFLAGS) $(OBJ) -o $@
	@echo "Kernel built successfully: $@"

obj/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

obj/%.o: src/%.S
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Pull Limine (binary branch), build host tool, copy header to src/
deps:
	@echo "Fetching Limine bootloader..."
	@mkdir -p $(LIMINE_DIR) src/kernel
	@if [ ! -d $(LIMINE_DIR)/.git ]; then \
		git clone --depth=1 --branch=v10.x-binary https://codeberg.org/Limine/Limine.git $(LIMINE_DIR); \
	fi
	@$(MAKE) -C $(LIMINE_DIR)
	@cp -f $(LIMINE_DIR)/limine.h src/kernel/limine.h
	@echo "Limine ready!"

# Hybrid BIOS+UEFI ISO using Limine
iso: $(BIN) deps
	@echo "Creating bootable ISO..."
	@rm -rf $(ISO_ROOT)
	@mkdir -p $(ISO_ROOT)/boot/limine $(ISO_ROOT)/EFI/BOOT
	@cp -v $(BIN) $(ISO_ROOT)/boot/$(OUTPUT)
	@cp -v limine.conf $(LIMINE_DIR)/limine-bios.sys \
	      $(LIMINE_DIR)/limine-bios-cd.bin \
	      $(LIMINE_DIR)/limine-uefi-cd.bin \
	      $(ISO_ROOT)/boot/limine/
	@cp -v $(LIMINE_DIR)/BOOTX64.EFI $(ISO_ROOT)/EFI/BOOT/
	@xorriso -as mkisofs -R -r -J \
	  -b boot/limine/limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table \
	  --efi-boot boot/limine/limine-uefi-cd.bin -efi-boot-part --efi-boot-image \
	  -hfsplus -apm-block-size 2048 --protective-msdos-label \
	  $(ISO_ROOT) -o $(ISO) 2>/dev/null
	@$(LIMINE_DIR)/limine bios-install $(ISO) 2>/dev/null || true
	@echo "ISO created: $(ISO)"

# Run in QEMU
run: iso
	@echo "Starting QEMU..."
	@$(QEMU) $(QEMU_FLAGS) -cdrom $(ISO)

# Run with GDB support
debug: iso
	@echo "Starting QEMU with GDB server (port 1234)..."
	@echo "In another terminal, run: gdb -ex 'target remote :1234' -ex 'symbol-file $(BIN)'"
	@$(QEMU) $(QEMU_FLAGS) $(QEMU_DEBUG_FLAGS) -cdrom $(ISO)

# Optional: create a GPT HDD image with a FAT32 ESP (for USB boot)
hdd: $(BIN) deps
	@echo "Creating bootable HDD image..."
	@rm -f $(HDD)
	@dd if=/dev/zero of=$(HDD) bs=1M count=0 seek=64 2>/dev/null
	@sgdisk $(HDD) -n 1:2048 -t 1:ef00 -m 1 2>/dev/null
	@$(LIMINE_DIR)/limine bios-install $(HDD) 2>/dev/null || true
	@mformat -i $(HDD)@@1M
	@mmd     -i $(HDD)@@1M ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine
	@mcopy   -i $(HDD)@@1M $(BIN) ::/boot/$(OUTPUT)
	@mcopy   -i $(HDD)@@1M limine.conf $(LIMINE_DIR)/limine-bios.sys ::/boot/limine
	@mcopy   -i $(HDD)@@1M $(LIMINE_DIR)/BOOTX64.EFI ::/EFI/BOOT
	@echo "HDD image ready: $(HDD)"
	@echo "To write to USB (⚠️ DANGEROUS): sudo dd if=$(HDD) of=/dev/rdiskX bs=1m"

# Testing target
test: iso
	@echo "Running automated tests..."
	@$(QEMU) $(QEMU_FLAGS) -device isa-debug-exit,iobase=0xf4,iosize=0x04 -cdrom $(ISO) -display none || \
		(EXIT_CODE=$$?; if [ $$EXIT_CODE -eq 33 ]; then echo "✓ Tests passed"; exit 0; else echo "✗ Tests failed (exit code $$EXIT_CODE)"; exit 1; fi)

check: test

clean:
	rm -rf obj bin $(ISO_ROOT) $(ISO) $(HDD)
	@echo "Clean complete!"

-include $(DEPS)

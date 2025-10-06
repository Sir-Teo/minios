Awesome — here’s a **copy‑paste starter repo** you can drop onto your Mac and build right away. It boots a **64‑bit, UEFI/BIOS**‑friendly kernel via **Limine**, draws a pixel line to the framebuffer, and logs to **serial** (so you see output in your terminal). It’s set up to scale: clean build system, higher‑half layout, and placeholders to grow memory, interrupts, processes, etc.

---

## 1) Quick start (one‑time)

```bash
# 1) Create a new folder and enter it
mkdir myos && cd myos

# 2) Install tools (Homebrew)
brew install x86_64-elf-gcc qemu xorriso mtools gptfdisk git

# 3) Add the files from this message (keep paths/filenames the same)

# 4) Fetch Limine (bootloader) and its header
make deps

# 5) Build a BIOS+UEFI hybrid ISO and run it
make iso
# Intel Mac (faster): add -accel hvf
make run ACCEL=hvf
# Apple Silicon: omit ACCEL (x86_64 emulation; slower)
make run
```

You’ll see serial logs in the terminal and a window with a white diagonal line (framebuffer test).

---

## 2) Repository tree

```
myos/
├─ README.md
├─ GNUmakefile
├─ linker.lds
├─ limine.conf
├─ .gitignore
├─ third_party/        # (auto-populated by `make deps`)
└─ src/
   ├─ kernel.c
   └─ support.c
   # `limine.h` will be auto-copied into src/ by `make deps`
```

---

## 3) Files (copy exactly)

### `README.md`

````markdown
# myos — minimal Limine-based x86_64 hobby OS (macOS-friendly)

## Quick start
```bash
brew install x86_64-elf-gcc qemu xorriso mtools gptfdisk git
make deps
make iso
make run ACCEL=hvf   # on Intel; omit ACCEL on Apple Silicon
````

## Targets

* `make deps`  – clones Limine into `third_party/limine` and copies `limine.h` into `src/`
* `make`       – builds the kernel ELF to `bin/myos`
* `make iso`   – creates a BIOS+UEFI hybrid `image.iso`
* `make run`   – runs the ISO in QEMU (use `ACCEL=hvf` on Intel Macs)
* `make clean` – removes build artifacts

## Next steps (scaling)

* Add paging + physical allocator under `kernel/mm/` (create when ready).
* Add IDT/exceptions/timer in `arch/x86_64/interrupts/`.
* Add a simple scheduler + syscalls, and split arch-specific vs. generic code.
* Create a user-mode ring3 task and load a tiny ELF from an initramfs.

````

### `GNUmakefile`
```make
# GNUmakefile — build a freestanding x86_64 kernel with Limine boot
.SUFFIXES:

# Toolchain (Homebrew provides x86_64-elf-gcc)
TOOLCHAIN_PREFIX ?= x86_64-elf-
CC      := $(TOOLCHAIN_PREFIX)gcc
LD      := $(TOOLCHAIN_PREFIX)ld
OBJCOPY := $(TOOLCHAIN_PREFIX)objcopy

# Output
OUTPUT := myos
BIN    := bin/$(OUTPUT)

# Sources
SRC := $(wildcard src/*.c) $(wildcard src/*.S)
OBJ := $(patsubst src/%.c,obj/%.o,$(filter %.c,$(SRC))) \
       $(patsubst src/%.S,obj/%.o,$(filter %.S,$(SRC)))
DEPS := $(OBJ:.o=.d)

# Flags
CFLAGS := -g -O2 -pipe \
  -Wall -Wextra -Wshadow -Wundef \
  -ffreestanding -fno-stack-protector -fno-stack-check -fno-common \
  -m64 -march=x86-64 -mno-80387 -mno-mmx -mno-sse -mno-sse2 \
  -mno-red-zone -mcmodel=kernel -fno-pic \
  -ffunction-sections -fdata-sections

CPPFLAGS := -Isrc -DLIMINE_API_REVISION=3 -MMD -MP

LDFLAGS := -nostdlib -static -z max-page-size=0x1000 --gc-sections \
  -T linker.lds -m elf_x86_64

# QEMU
QEMU       := qemu-system-x86_64
QEMU_FLAGS := -m 512 -serial stdio -no-reboot
ifneq ($(ACCEL),)
QEMU_FLAGS += -accel $(ACCEL)
endif

# Limine paths
LIMINE_DIR := third_party/limine
ISO_ROOT   := iso_root
ISO        := image.iso
HDD        := image.hdd

# Phonies
.PHONY: all clean deps iso run hdd

all: $(BIN)

# Build rules
$(BIN): $(OBJ) linker.lds
	mkdir -p bin
	$(LD) $(LDFLAGS) $(OBJ) -o $@

obj/%.o: src/%.c
	mkdir -p obj
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

obj/%.o: src/%.S
	mkdir -p obj
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Pull Limine (binary branch), build host tool, copy header to src/
deps:
	mkdir -p $(LIMINE_DIR) src
	test -d $(LIMINE_DIR)/.git || git clone --depth=1 --branch=v10.x-binary https://codeberg.org/Limine/Limine.git $(LIMINE_DIR)
	$(MAKE) -C $(LIMINE_DIR)
	cp -f $(LIMINE_DIR)/limine.h src/limine.h

# Hybrid BIOS+UEFI ISO using Limine
iso: $(BIN) deps
	rm -rf $(ISO_ROOT)
	mkdir -p $(ISO_ROOT)/boot/limine $(ISO_ROOT)/EFI/BOOT
	cp -v $(BIN) $(ISO_ROOT)/boot/$(OUTPUT)
	cp -v limine.conf $(LIMINE_DIR)/limine-bios.sys \
	      $(LIMINE_DIR)/limine-bios-cd.bin \
	      $(LIMINE_DIR)/limine-uefi-cd.bin \
	      $(ISO_ROOT)/boot/limine/
	cp -v $(LIMINE_DIR)/BOOTX64.EFI $(ISO_ROOT)/EFI/BOOT/

	xorriso -as mkisofs -R -r -J \
	  -b boot/limine/limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table \
	  --efi-boot boot/limine/limine-uefi-cd.bin -efi-boot-part --efi-boot-image \
	  -hfsplus -apm-block-size 2048 --protective-msdos-label \
	  $(ISO_ROOT) -o $(ISO)

	$(LIMINE_DIR)/limine bios-install $(ISO)

run: iso
	$(QEMU) $(QEMU_FLAGS) -cdrom $(ISO)

# Optional: create a GPT HDD image with a FAT32 ESP (for USB boot)
hdd: $(BIN) deps
	rm -f $(HDD)
	dd if=/dev/zero of=$(HDD) bs=1M count=0 seek=64
	sgdisk $(HDD) -n 1:2048 -t 1:ef00 -m 1
	$(LIMINE_DIR)/limine bios-install $(HDD)
	mformat -i $(HDD)@@1M
	mmd     -i $(HDD)@@1M ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine
	mcopy   -i $(HDD)@@1M $(BIN) ::/boot/$(OUTPUT)
	mcopy   -i $(HDD)@@1M limine.conf $(LIMINE_DIR)/limine-bios.sys ::/boot/limine
	mcopy   -i $(HDD)@@1M $(LIMINE_DIR)/BOOTX64.EFI ::/EFI/BOOT
	@echo "HDD image ready at $(HDD). Use 'dd' to write to a USB device (careful!)."

clean:
	rm -rf obj bin $(ISO_ROOT) $(ISO) $(HDD)

-include $(DEPS)
````

### `linker.lds`

```ld
OUTPUT_FORMAT(elf64-x86-64)
ENTRY(kmain)

/* Program headers (segments) */
PHDRS {
  text   PT_LOAD;
  rodata PT_LOAD;
  data   PT_LOAD;
}

SECTIONS {
  /* Higher-half kernel base (top 2GiB) */
  . = 0xffffffff80000000;

  /* Keep Limine request blocks together early in the image */
  .limine_reqs : {
    KEEP(*(.limine_reqs_start))
    KEEP(*(.limine_reqs))
    KEEP(*(.limine_reqs.*))
    KEEP(*(.limine_reqs_end))
    /* also accept possible alternate section names */
    KEEP(*(.limine_requests_start))
    KEEP(*(.limine_requests))
    KEEP(*(.limine_requests.*))
    KEEP(*(.limine_requests_end))
  } :text

  . = ALIGN(0x1000);
  .text : { *(.text .text.*) } :text

  . = ALIGN(0x1000);
  .rodata : { *(.rodata .rodata.*) *(.eh_frame) } :rodata

  . = ALIGN(0x1000);
  .data : { *(.data .data.*) } :data

  .bss : { *(.bss .bss.*) *(COMMON) }
}
```

### `limine.conf`

```conf
# Limine bootloader configuration
timeout: 3

:myOS
    protocol: limine
    kernel_path: boot:///boot/myos
    # kernel_cmdline: loglevel=debug
    # terminal: limine
```

### `.gitignore`

```gitignore
.DS_Store
bin/
obj/
iso_root/
image.iso
image.hdd
third_party/limine/build
```

### `src/kernel.c`

```c
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "limine.h"

/* ---------- Limine boot protocol requests (API rev 3) ---------- */
LIMINE_REQUESTS_START_MARKER;
LIMINE_BASE_REVISION(3);

static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

LIMINE_REQUESTS_END_MARKER;

/* ---------- I/O port helpers ---------- */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0,%1" :: "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t v; __asm__ volatile ("inb %1,%0" : "=a"(v) : "Nd"(port));
    return v;
}

/* ---------- 16550A serial on QEMU (COM1 @ 0x3F8) ---------- */
static int serial_tx_ready(void) { return (inb(0x3F8 + 5) & 0x20) != 0; }

static void serial_init(void) {
    outb(0x3F8 + 1, 0x00); // disable interrupts
    outb(0x3F8 + 3, 0x80); // DLAB on
    outb(0x3F8 + 0, 0x03); // divisor low (3) -> 38400 baud
    outb(0x3F8 + 1, 0x00); // divisor high
    outb(0x3F8 + 3, 0x03); // 8n1, DLAB off
    outb(0x3F8 + 2, 0xC7); // enable FIFO
    outb(0x3F8 + 4, 0x0B); // RTS/DSR + OUT2
}

static void serial_putc(char c) {
    while (!serial_tx_ready()) { }
    outb(0x3F8, (uint8_t)c);
}

static void serial_write(const char *s) {
    for (; *s; s++) {
        if (*s == '\n') serial_putc('\r');
        serial_putc(*s);
    }
}

/* ---------- halt helper ---------- */
static void hcf(void) {
    for (;;) __asm__ volatile ("hlt");
}

/* ---------- kernel entry ---------- */
void kmain(void) {
    if (!LIMINE_BASE_REVISION_SUPPORTED) {
        hcf();
    }

    serial_init();
    serial_write("[myos] booted via Limine. Hello!\n");

    if (!framebuffer_request.response ||
        framebuffer_request.response->framebuffer_count < 1) {
        serial_write("No framebuffer available; halting.\n");
        hcf();
    }

    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];

    volatile uint32_t *pix = (volatile uint32_t *)fb->address;
    size_t w = fb->width, h = fb->height, pitch32 = fb->pitch / 4;
    size_t limit = (w < h ? w : h);

    /* Draw a simple diagonal white line to prove graphics ownership */
    for (size_t i = 0; i < limit && i < 300; i++) {
        pix[i * pitch32 + i] = 0x00FFFFFFu;
    }

    serial_write("Framebuffer OK. Diagonal line drawn. Halting.\n");
    hcf();
}
```

### `src/support.c`

```c
/* Minimal freestanding libc bits some compilers expect */
#include <stddef.h>
#include <stdint.h>

void *memcpy(void *restrict d, const void *restrict s, size_t n) {
    uint8_t *dd = d; const uint8_t *ss = s;
    for (size_t i = 0; i < n; i++) dd[i] = ss[i];
    return d;
}
void *memset(void *d, int c, size_t n) {
    uint8_t *p = d; for (size_t i = 0; i < n; i++) p[i] = (uint8_t)c;
    return d;
}
void *memmove(void *d, const void *s, size_t n) {
    uint8_t *dd = d; const uint8_t *ss = s;
    if (ss > dd) for (size_t i = 0; i < n; i++) dd[i] = ss[i];
    else if (ss < dd) for (size_t i = n; i > 0; i--) dd[i - 1] = ss[i - 1];
    return d;
}
int memcmp(const void *a, const void *b, size_t n) {
    const uint8_t *x = a, *y = b;
    for (size_t i = 0; i < n; i++) {
        if (x[i] != y[i]) return x[i] < y[i] ? -1 : 1;
    }
    return 0;
}
```

---

## 4) Using it

* **Build & run**

  ```bash
  make deps          # clone Limine + host tools, copy limine.h
  make               # build kernel ELF -> bin/myos
  make iso           # build BIOS+UEFI hybrid ISO -> image.iso
  make run ACCEL=hvf # Intel Mac (faster); omit ACCEL on Apple Silicon
  ```

* **Serial output** appears in your terminal (because we passed `-serial stdio`).

* **Graphics**: a white diagonal line shows in the QEMU window (simple framebuffer test).

> When you’re ready to scale: add `arch/` and `kernel/` subdirs, paging, IDT, timer, scheduler, syscalls, then user mode + ELF loader. The Makefile and layout are set up to grow cleanly.

If you want me to add **starter stubs** for `mm/`, `interrupts/`, and a preemptive **timer tick** in a follow‑up iteration, say the word and I’ll extend this scaffold.

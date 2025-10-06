/**
 * kernel.c - Main kernel entry point for miniOS
 *
 * This is the heart of the operating system. After the Limine bootloader
 * loads the kernel, it jumps to kmain() with the system in 64-bit long mode,
 * paging enabled, and boot information passed via the Limine protocol.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "limine.h"

/* Forward declarations */
void serial_init(void);
void serial_write(const char *s);
void serial_write_dec(uint64_t value);
void serial_write_hex(uint64_t value);
void kprintf(const char *fmt, ...);
static void hcf(void);

// Architecture-specific initialization
extern void gdt_init(void);
extern void idt_init(void);

// Memory management
extern void pmm_init(void);
extern void kmalloc_init(void);
extern uint64_t pmm_get_free_memory(void);
extern void vmm_init(void);

// Timer
extern void pit_init(uint32_t frequency);

// Scheduler
extern void task_init(void);
extern void sched_init(void);
extern void sched_set_enabled(bool enabled);

// System calls
extern void syscall_init(void);

// User mode
extern void usermode_init(void);

// ELF loader
extern void elf_init(void);

// Keyboard driver
extern void keyboard_init(void);

// Tests
extern void run_vmm_tests(void);
extern void run_pit_tests(void);
extern void run_sched_tests(void);
extern void run_syscall_tests(void);
extern void test_elf_run_all(void);
extern void run_usermode_tests(void);

/* ---------- Limine boot protocol requests (API revision 3) ---------- */

/* Request markers - these must be in their own sections */
LIMINE_REQUESTS_START_MARKER;

/* Request base revision support */
LIMINE_BASE_REVISION(3);

/* Request framebuffer information */
volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

/* Request memory map */
volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

/* Request executable/kernel file */
volatile struct limine_executable_file_request executable_file_request = {
    .id = LIMINE_EXECUTABLE_FILE_REQUEST,
    .revision = 0
};

/* Request HHDM (Higher Half Direct Map) offset */
volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

/* End of requests */
LIMINE_REQUESTS_END_MARKER;

/* ---------- I/O port helpers ---------- */

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0,%1" :: "a"(val), "Nd"(port) : "memory");
}

static inline uint8_t inb(uint16_t port) {
    uint8_t v;
    __asm__ volatile ("inb %1,%0" : "=a"(v) : "Nd"(port) : "memory");
    return v;
}

static inline void io_wait(void) {
    outb(0x80, 0);  // Write to unused port for small delay
}

/* ---------- 16550A serial UART on QEMU (COM1 @ 0x3F8) ---------- */

#define SERIAL_PORT_COM1 0x3F8

static int serial_tx_ready(void) {
    return (inb(SERIAL_PORT_COM1 + 5) & 0x20) != 0;
}

void serial_init(void) {
    outb(SERIAL_PORT_COM1 + 1, 0x00);  // Disable interrupts
    outb(SERIAL_PORT_COM1 + 3, 0x80);  // Enable DLAB (set baud rate divisor)
    outb(SERIAL_PORT_COM1 + 0, 0x03);  // Divisor low byte (3 = 38400 baud)
    outb(SERIAL_PORT_COM1 + 1, 0x00);  // Divisor high byte
    outb(SERIAL_PORT_COM1 + 3, 0x03);  // 8 bits, no parity, one stop bit
    outb(SERIAL_PORT_COM1 + 2, 0xC7);  // Enable FIFO, clear, 14-byte threshold
    outb(SERIAL_PORT_COM1 + 4, 0x0B);  // IRQs enabled, RTS/DSR set
    outb(SERIAL_PORT_COM1 + 1, 0x01);  // Enable interrupts
}

void serial_putc(char c) {
    // Wait for transmit buffer to be empty
    while (!serial_tx_ready()) {
        __asm__ volatile ("pause");
    }
    outb(SERIAL_PORT_COM1, (uint8_t)c);
}

void serial_write(const char *s) {
    for (; *s; s++) {
        if (*s == '\n') {
            serial_putc('\r');  // CRLF for terminals
        }
        serial_putc(*s);
    }
}

/* ---------- Halt and catch fire ---------- */

static void hcf(void) {
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}

/* ---------- Framebuffer helpers ---------- */

static void draw_test_pattern(struct limine_framebuffer *fb) {
    volatile uint32_t *pix = (volatile uint32_t *)fb->address;
    size_t width = fb->width;
    size_t height = fb->height;
    size_t pitch32 = fb->pitch / 4;

    // Clear screen to black
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            pix[y * pitch32 + x] = 0x00000000;
        }
    }

    // Draw white diagonal line
    size_t limit = (width < height ? width : height);
    for (size_t i = 0; i < limit && i < 400; i++) {
        pix[i * pitch32 + i] = 0x00FFFFFF;
    }

    // Draw colored corners (RGB test)
    for (size_t i = 0; i < 50; i++) {
        // Top-left: Red
        pix[i * pitch32 + i] = 0x00FF0000;
        // Top-right: Green
        if (width - 1 - i < width) {
            pix[i * pitch32 + (width - 1 - i)] = 0x0000FF00;
        }
        // Bottom-left: Blue
        if (height - 1 - i < height) {
            pix[(height - 1 - i) * pitch32 + i] = 0x000000FF;
        }
        // Bottom-right: White
        if (height - 1 - i < height && width - 1 - i < width) {
            pix[(height - 1 - i) * pitch32 + (width - 1 - i)] = 0x00FFFFFF;
        }
    }
}

/* ---------- Memory info display ---------- */

static void print_memory_map(void) {
    if (!memmap_request.response) {
        serial_write("[MEMMAP] No memory map available\n");
        return;
    }

    struct limine_memmap_response *memmap = memmap_request.response;
    serial_write("[MEMMAP] Memory map entries:\n");

    const char *type_names[] = {
        "USABLE",
        "RESERVED",
        "ACPI_RECLAIMABLE",
        "ACPI_NVS",
        "BAD_MEMORY",
        "BOOTLOADER_RECLAIMABLE",
        "KERNEL_AND_MODULES",
        "FRAMEBUFFER"
    };

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        const char *type_name = (entry->type < 8) ? type_names[entry->type] : "UNKNOWN";

        // Simple hex printing (TODO: implement proper formatting)
        serial_write("[MEMMAP]   ");
        serial_write(type_name);
        serial_write("\n");
    }
}

/* ---------- Kernel entry point ---------- */

void kmain(void) {
    // Check if the bootloader supports our revision
    if (!LIMINE_BASE_REVISION_SUPPORTED) {
        hcf();
    }

    // Initialize serial console first for debugging
    serial_init();

    serial_write("\n");
    serial_write("========================================\n");
    serial_write("       miniOS - x86_64 Kernel          \n");
    serial_write("========================================\n");
    serial_write("[BOOT] Kernel started via Limine bootloader\n");
    serial_write("[BOOT] CPU in 64-bit long mode\n");

    // Check executable/kernel file
    if (executable_file_request.response) {
        serial_write("[BOOT] Kernel executable loaded\n");
        serial_write("[BOOT] Kernel virtual base: 0xFFFFFFFF80000000\n");
    }

    // Check HHDM offset
    if (hhdm_request.response) {
        serial_write("[BOOT] Higher Half Direct Map offset obtained\n");
    }

    // Print memory map
    print_memory_map();

    // Initialize GDT
    serial_write("[CPU] Initializing GDT...\n");
    gdt_init();
    serial_write("[CPU] GDT initialized\n");

    // Initialize IDT
    serial_write("[CPU] Initializing IDT...\n");
    idt_init();
    serial_write("[CPU] IDT initialized\n");

    // Enable interrupts
    __asm__ volatile("sti");
    serial_write("[CPU] Interrupts enabled\n");

    // Initialize physical memory manager
    pmm_init();

    // Initialize kernel heap
    kmalloc_init();

    // Display free memory
    serial_write("[MEM] Free memory: ");
    serial_write_dec(pmm_get_free_memory() / 1024 / 1024);
    serial_write(" MiB\n");

    // Initialize virtual memory manager
    vmm_init();

    // Run VMM tests
    serial_write("\n");
    run_vmm_tests();

    // Run PIT tests (they initialize the timer internally)
    serial_write("\n");
    run_pit_tests();

    // Initialize task subsystem
    serial_write("\n");
    task_init();

    // Initialize scheduler
    sched_init();

    // Run scheduler tests
    serial_write("\n");
    run_sched_tests();

    // Initialize system calls
    serial_write("\n");
    syscall_init();

    // Run syscall tests
    serial_write("\n");
    run_syscall_tests();

    // Initialize user mode
    serial_write("\n");
    usermode_init();

    // Run user mode tests
    serial_write("\n");
    run_usermode_tests();

    // Initialize ELF loader
    serial_write("\n");
    elf_init();

    // Run ELF loader tests
    serial_write("\n");
    test_elf_run_all();

    // Initialize keyboard
    serial_write("\n");
    keyboard_init();

    // Enable scheduler (will start on next timer tick)
    serial_write("\n");
    serial_write("[KERNEL] Enabling multitasking...\n");
    sched_set_enabled(true);

    // Initialize framebuffer
    serial_write("[VIDEO] Initializing framebuffer...\n");

    if (!framebuffer_request.response ||
        framebuffer_request.response->framebuffer_count < 1) {
        serial_write("[VIDEO] ERROR: No framebuffer available\n");
        serial_write("[BOOT] Continuing in text mode only...\n");
    } else {
        struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];

        serial_write("[VIDEO] Framebuffer initialized:\n");
        serial_write("[VIDEO]   Resolution: ");
        // TODO: Print actual numbers when we have proper formatting
        serial_write("(width x height)\n");
        serial_write("[VIDEO]   Bits per pixel: 32\n");

        // Draw test pattern
        draw_test_pattern(fb);
        serial_write("[VIDEO] Test pattern drawn\n");
    }

    // Initialization complete
    serial_write("\n");
    serial_write("========================================\n");
    serial_write("[BOOT] Kernel initialization complete!\n");
    serial_write("[BOOT] System ready. Halting...\n");
    serial_write("========================================\n");
    serial_write("\n");
    serial_write("Next steps:\n");
    serial_write("  - Implement memory management (paging, allocator)\n");
    serial_write("  - Set up interrupt handling (IDT, GDT)\n");
    serial_write("  - Add APIC timer for scheduling\n");
    serial_write("  - Create process scheduler\n");
    serial_write("  - Implement syscalls and user mode\n");
    serial_write("\n");

    // Halt the CPU
    hcf();
}

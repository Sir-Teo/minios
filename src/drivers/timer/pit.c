#include "pit.h"
#include <stdint.h>
#include <stddef.h>

// Declare serial_write for debugging
extern void serial_write(const char *s);

// PIC (Programmable Interrupt Controller) ports for remapping IRQs
#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

// Global variables
static volatile uint64_t pit_ticks = 0;
static uint32_t pit_frequency = 0;
static pit_callback_t user_callback = NULL;

/**
 * I/O port access functions (inline assembly).
 */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" :: "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * Remap the PIC (if not already done) to ensure IRQ0 is at vector 32.
 * This is typically done during interrupt initialization, but we do it
 * here to be safe.
 */
static void pit_remap_pic(void) {
    uint8_t mask1, mask2;

    // Save masks
    mask1 = inb(PIC1_DATA);
    mask2 = inb(PIC2_DATA);

    // Start initialization sequence (ICW1)
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);

    // ICW2: Set vector offsets (master at 32, slave at 40)
    outb(PIC1_DATA, 32);
    outb(PIC2_DATA, 40);

    // ICW3: Tell master PIC there's a slave at IRQ2
    outb(PIC1_DATA, 0x04);
    // Tell slave PIC its cascade identity
    outb(PIC2_DATA, 0x02);

    // ICW4: Set 8086 mode
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    // Restore masks
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

/**
 * Unmask IRQ0 (timer) on the PIC.
 */
static void pit_enable_irq0(void) {
    uint8_t mask = inb(PIC1_DATA);
    mask &= ~0x01;  // Clear bit 0 (IRQ0)
    outb(PIC1_DATA, mask);
}

void pit_init(uint32_t frequency) {
    serial_write("[PIT] Initializing Programmable Interval Timer...\n");

    if (frequency == 0 || frequency > PIT_BASE_FREQ) {
        serial_write("[PIT] ERROR: Invalid frequency\n");
        return;
    }

    pit_frequency = frequency;
    pit_ticks = 0;

    // Calculate divisor for desired frequency
    // divisor = PIT_BASE_FREQ / frequency
    uint32_t divisor = PIT_BASE_FREQ / frequency;

    // Sanity check
    if (divisor > 65535) {
        serial_write("[PIT] WARNING: Frequency too low, clamping divisor\n");
        divisor = 65535;
    }
    if (divisor == 0) {
        divisor = 1;
    }

    // Remap PIC (ensure IRQs are at the correct vectors)
    pit_remap_pic();

    // Send command byte: Channel 0, access mode lobyte/hibyte, mode 2 (rate generator), binary
    uint8_t command = PIT_CMD_CHANNEL_0 | PIT_CMD_RW_BOTH | PIT_CMD_MODE_2 | PIT_CMD_BINARY;
    outb(PIT_COMMAND, command);

    // Send divisor (low byte, then high byte)
    outb(PIT_CHANNEL_0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL_0, (uint8_t)((divisor >> 8) & 0xFF));

    // Unmask IRQ0 on the PIC
    pit_enable_irq0();

    serial_write("[PIT] Timer initialized at ");
    // Simple integer to string conversion for debugging
    char freq_str[16];
    int i = 0;
    uint32_t temp = frequency;
    if (temp == 0) {
        freq_str[i++] = '0';
    } else {
        char digits[16];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int j = digit_count - 1; j >= 0; j--) {
            freq_str[i++] = digits[j];
        }
    }
    freq_str[i++] = ' ';
    freq_str[i++] = 'H';
    freq_str[i++] = 'z';
    freq_str[i++] = '\n';
    freq_str[i] = '\0';
    serial_write(freq_str);
}

uint64_t pit_get_ticks(void) {
    return pit_ticks;
}

void pit_sleep(uint64_t ticks) {
    uint64_t target = pit_ticks + ticks;
    while (pit_ticks < target) {
        __asm__ volatile("hlt");  // Halt until next interrupt
    }
}

void pit_set_callback(pit_callback_t callback) {
    user_callback = callback;
}

/**
 * PIT IRQ handler (called from the IRQ0 handler in idt.c).
 * This function is called on every timer tick.
 */
void pit_irq_handler(void) {
    pit_ticks++;

    // Call user callback if set
    if (user_callback != NULL) {
        user_callback();
    }

    // Note: EOI (End of Interrupt) is sent by the common irq_handler in idt.c
}

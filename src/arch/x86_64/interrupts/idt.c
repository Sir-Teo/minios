#include "idt.h"
#include <stdint.h>
#include <stddef.h>

// Declare serial_write from kernel
extern void serial_write(const char *s);

#define IDT_ENTRIES 256

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr idtp;

// External assembly function to load IDT
extern void idt_flush(uint64_t idt_ptr);

// ISR stub declarations (defined in isr_asm.S)
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

// IRQ handlers (IRQ0-15 map to ISR 32-47)
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

void idt_set_gate(uint8_t num, uint64_t handler, uint16_t selector, uint8_t type_attr, uint8_t ist) {
    idt[num].offset_low = handler & 0xFFFF;
    idt[num].selector = selector;
    idt[num].ist = ist;
    idt[num].type_attr = type_attr;
    idt[num].offset_mid = (handler >> 16) & 0xFFFF;
    idt[num].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[num].zero = 0;
}

void idt_init(void) {
    idtp.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idtp.base = (uint64_t)&idt;

    // Clear IDT
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt[i].offset_low = 0;
        idt[i].selector = 0;
        idt[i].ist = 0;
        idt[i].type_attr = 0;
        idt[i].offset_mid = 0;
        idt[i].offset_high = 0;
        idt[i].zero = 0;
    }

    // Install exception handlers (ISR 0-31)
    // Type: 0x8E = Present, Ring 0, 64-bit Interrupt Gate
    idt_set_gate(0, (uint64_t)isr0, 0x08, 0x8E, 0);
    idt_set_gate(1, (uint64_t)isr1, 0x08, 0x8E, 0);
    idt_set_gate(2, (uint64_t)isr2, 0x08, 0x8E, 0);
    idt_set_gate(3, (uint64_t)isr3, 0x08, 0x8E, 0);
    idt_set_gate(4, (uint64_t)isr4, 0x08, 0x8E, 0);
    idt_set_gate(5, (uint64_t)isr5, 0x08, 0x8E, 0);
    idt_set_gate(6, (uint64_t)isr6, 0x08, 0x8E, 0);
    idt_set_gate(7, (uint64_t)isr7, 0x08, 0x8E, 0);
    idt_set_gate(8, (uint64_t)isr8, 0x08, 0x8E, 0);
    idt_set_gate(9, (uint64_t)isr9, 0x08, 0x8E, 0);
    idt_set_gate(10, (uint64_t)isr10, 0x08, 0x8E, 0);
    idt_set_gate(11, (uint64_t)isr11, 0x08, 0x8E, 0);
    idt_set_gate(12, (uint64_t)isr12, 0x08, 0x8E, 0);
    idt_set_gate(13, (uint64_t)isr13, 0x08, 0x8E, 0);
    idt_set_gate(14, (uint64_t)isr14, 0x08, 0x8E, 0);
    idt_set_gate(15, (uint64_t)isr15, 0x08, 0x8E, 0);
    idt_set_gate(16, (uint64_t)isr16, 0x08, 0x8E, 0);
    idt_set_gate(17, (uint64_t)isr17, 0x08, 0x8E, 0);
    idt_set_gate(18, (uint64_t)isr18, 0x08, 0x8E, 0);
    idt_set_gate(19, (uint64_t)isr19, 0x08, 0x8E, 0);
    idt_set_gate(20, (uint64_t)isr20, 0x08, 0x8E, 0);
    idt_set_gate(21, (uint64_t)isr21, 0x08, 0x8E, 0);
    idt_set_gate(22, (uint64_t)isr22, 0x08, 0x8E, 0);
    idt_set_gate(23, (uint64_t)isr23, 0x08, 0x8E, 0);
    idt_set_gate(24, (uint64_t)isr24, 0x08, 0x8E, 0);
    idt_set_gate(25, (uint64_t)isr25, 0x08, 0x8E, 0);
    idt_set_gate(26, (uint64_t)isr26, 0x08, 0x8E, 0);
    idt_set_gate(27, (uint64_t)isr27, 0x08, 0x8E, 0);
    idt_set_gate(28, (uint64_t)isr28, 0x08, 0x8E, 0);
    idt_set_gate(29, (uint64_t)isr29, 0x08, 0x8E, 0);
    idt_set_gate(30, (uint64_t)isr30, 0x08, 0x8E, 0);
    idt_set_gate(31, (uint64_t)isr31, 0x08, 0x8E, 0);

    // Install IRQ handlers (IRQ 0-15 map to ISR 32-47)
    idt_set_gate(32, (uint64_t)irq0, 0x08, 0x8E, 0);
    idt_set_gate(33, (uint64_t)irq1, 0x08, 0x8E, 0);
    idt_set_gate(34, (uint64_t)irq2, 0x08, 0x8E, 0);
    idt_set_gate(35, (uint64_t)irq3, 0x08, 0x8E, 0);
    idt_set_gate(36, (uint64_t)irq4, 0x08, 0x8E, 0);
    idt_set_gate(37, (uint64_t)irq5, 0x08, 0x8E, 0);
    idt_set_gate(38, (uint64_t)irq6, 0x08, 0x8E, 0);
    idt_set_gate(39, (uint64_t)irq7, 0x08, 0x8E, 0);
    idt_set_gate(40, (uint64_t)irq8, 0x08, 0x8E, 0);
    idt_set_gate(41, (uint64_t)irq9, 0x08, 0x8E, 0);
    idt_set_gate(42, (uint64_t)irq10, 0x08, 0x8E, 0);
    idt_set_gate(43, (uint64_t)irq11, 0x08, 0x8E, 0);
    idt_set_gate(44, (uint64_t)irq12, 0x08, 0x8E, 0);
    idt_set_gate(45, (uint64_t)irq13, 0x08, 0x8E, 0);
    idt_set_gate(46, (uint64_t)irq14, 0x08, 0x8E, 0);
    idt_set_gate(47, (uint64_t)irq15, 0x08, 0x8E, 0);

    // Load the new IDT
    idt_flush((uint64_t)&idtp);
}

// Common exception handler names
static const char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    "Reserved"
};

// Common ISR handler (called from assembly stubs)
void isr_handler(struct registers *regs) {
    serial_write("\n!!! EXCEPTION: ");
    if (regs->int_no < 32) {
        serial_write(exception_messages[regs->int_no]);
    } else {
        serial_write("Unknown");
    }
    serial_write(" !!!\n");
    
    // Halt on exception
    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}

// Common IRQ handler
void irq_handler(struct registers *regs) {
    // Send EOI to PIC
    if (regs->int_no >= 40) {
        // Send to slave PIC
        __asm__ volatile("outb %0, %1" :: "a"((uint8_t)0x20), "Nd"((uint16_t)0xA0));
    }
    // Always send to master PIC
    __asm__ volatile("outb %0, %1" :: "a"((uint8_t)0x20), "Nd"((uint16_t)0x20));
}

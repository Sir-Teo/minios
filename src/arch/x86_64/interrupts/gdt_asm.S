.section .text
.global gdt_flush
.global tss_flush

# gdt_flush - Load new GDT
# Parameter: RDI = pointer to GDT pointer structure
gdt_flush:
    lgdt (%rdi)         # Load GDT
    
    # Reload segments
    mov $0x10, %ax      # 0x10 = kernel data segment
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    
    # Far return to reload CS
    popq %rdi           # Pop return address
    pushq $0x08         # Push kernel code segment
    pushq %rdi          # Push return address back
    lretq               # Far return (reloads CS)

# tss_flush - Load TSS
# Parameter: RDI = TSS segment selector
tss_flush:
    ltr %di             # Load Task Register
    ret

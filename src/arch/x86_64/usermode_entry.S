.section .text
.global usermode_entry

# Enter user mode (ring 3)
# Arguments:
#   rdi = entry point (user RIP)
#   rsi = user stack pointer (user RSP)
#   rdx = user code segment (CS with RPL=3)
#   rcx = user data segment (SS with RPL=3)

usermode_entry:
    # We need to use IRET to transition from ring 0 to ring 3
    # IRET pops: RIP, CS, RFLAGS, RSP, SS

    # Disable interrupts during transition
    cli

    # Setup user data segments (DS, ES, FS, GS)
    movq %rcx, %rax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # Build IRET frame on kernel stack
    # Stack layout (bottom to top):
    #   SS      (user stack segment)
    #   RSP     (user stack pointer)
    #   RFLAGS  (flags with IF set)
    #   CS      (user code segment)
    #   RIP     (entry point)

    # Push SS (user stack segment)
    pushq %rcx

    # Push RSP (user stack pointer)
    pushq %rsi

    # Push RFLAGS (enable interrupts in user mode)
    pushfq
    popq %rax
    orq $0x200, %rax    # Set IF (interrupt flag)
    pushq %rax

    # Push CS (user code segment)
    pushq %rdx

    # Push RIP (entry point)
    pushq %rdi

    # Clear general purpose registers for security
    xorq %rax, %rax
    xorq %rbx, %rbx
    xorq %rcx, %rcx
    xorq %rdx, %rdx
    xorq %rsi, %rsi
    xorq %rdi, %rdi
    xorq %rbp, %rbp
    xorq %r8, %r8
    xorq %r9, %r9
    xorq %r10, %r10
    xorq %r11, %r11
    xorq %r12, %r12
    xorq %r13, %r13
    xorq %r14, %r14
    xorq %r15, %r15

    # Return to user mode
    iretq

    # Should never reach here
    ud2

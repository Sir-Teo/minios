.section .text
.global context_switch

# context_switch(cpu_state_t *old_state, cpu_state_t *new_state)
#
# Saves the current CPU state to old_state and restores new_state.
# This function performs a complete context switch between two tasks.
#
# Parameters:
#   rdi = pointer to old task's cpu_state_t (to save current state)
#   rsi = pointer to new task's cpu_state_t (to load new state)
#
# The cpu_state_t structure layout (must match task.h):
# Offset | Register
# -------|---------
#   0    | rax
#   8    | rbx
#   16   | rcx
#   24   | rdx
#   32   | rsi
#   40   | rdi
#   48   | rbp
#   56   | rsp
#   64   | r8
#   72   | r9
#   80   | r10
#   88   | r11
#   96   | r12
#   104  | r13
#   112  | r14
#   120  | r15
#   128  | rip
#   136  | rflags
#   144  | cs
#   152  | ss

context_switch:
    # Save the current task's state (if old_state != NULL)
    test %rdi, %rdi
    jz load_new_state

save_old_state:
    # Save general purpose registers
    movq %rax, 0(%rdi)
    movq %rbx, 8(%rdi)
    movq %rcx, 16(%rdi)
    movq %rdx, 24(%rdi)

    # Save rsi and rdi (note: rdi points to old_state, rsi points to new_state)
    # We need to save the original values before they were used as parameters
    movq %rsi, 32(%rdi)
    # rdi is saved differently since it's being used
    lea 8(%rsp), %rax        # Get return address location
    movq (%rsp), %rbx        # Get return address
    movq %rbx, 128(%rdi)     # Save as RIP

    movq %rbp, 48(%rdi)
    movq %rsp, 56(%rdi)

    movq %r8, 64(%rdi)
    movq %r9, 72(%rdi)
    movq %r10, 80(%rdi)
    movq %r11, 88(%rdi)
    movq %r12, 96(%rdi)
    movq %r13, 104(%rdi)
    movq %r14, 112(%rdi)
    movq %r15, 120(%rdi)

    # Save flags
    pushfq
    popq %rax
    movq %rax, 136(%rdi)

    # Save segment selectors
    movq $0x08, 144(%rdi)    # CS (kernel code segment)
    movq $0x10, 152(%rdi)    # SS (kernel data segment)

load_new_state:
    # Restore the new task's state from new_state (rsi)

    # Restore segment selectors (if needed, for now we stay in kernel mode)
    # movq 144(%rsi), %rax
    # movq %rax, %cs           # Can't directly move to cs
    # movq 152(%rsi), %rax
    # movq %rax, %ss           # Can't directly move to ss

    # Restore flags
    movq 136(%rsi), %rax
    pushq %rax
    popfq

    # Restore general purpose registers
    movq 0(%rsi), %rax
    movq 8(%rsi), %rbx
    movq 16(%rsi), %rcx
    movq 24(%rsi), %rdx

    movq 48(%rsi), %rbp
    movq 56(%rsi), %rsp      # Switch stack

    movq 64(%rsi), %r8
    movq 72(%rsi), %r9
    movq 80(%rsi), %r10
    movq 88(%rsi), %r11
    movq 96(%rsi), %r12
    movq 104(%rsi), %r13
    movq 112(%rsi), %r14
    movq 120(%rsi), %r15

    # Get the new RIP (return address)
    movq 128(%rsi), %rdi     # Temporarily use rdi

    # Restore rsi and rdi last
    pushq %rdi               # Push new RIP onto stack
    movq 40(%rsi), %rdi      # Restore rdi
    movq 32(%rsi), %rsi      # Restore rsi

    # Return to the new task's RIP
    ret

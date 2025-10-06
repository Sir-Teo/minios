.section .text
.global syscall_entry
.extern syscall_dispatch

# System call entry point
# Called via SYSCALL instruction from user mode
# Arguments are passed in: rdi, rsi, rdx, r10, r8, r9
# System call number is in rax
# Return value goes in rax

syscall_entry:
    # Save user stack pointer and switch to kernel stack
    # rcx contains user RIP (return address)
    # r11 contains user RFLAGS

    # We need to preserve these user-mode registers
    pushq %rcx                # Save user RIP
    pushq %r11                # Save user RFLAGS

    # Save user registers that might be clobbered
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # Save original argument registers before we modify them
    pushq %rdi                # Original arg1
    pushq %rsi                # Original arg2
    pushq %rdx                # Original arg3
    pushq %r10                # Original arg4 (note: r10, not rcx)
    pushq %r8                 # Original arg5
    pushq %r9                 # Original arg6 (if needed)

    # Now prepare arguments for syscall_dispatch(syscall_num, arg1-5)
    # C calling convention: rdi, rsi, rdx, rcx, r8, r9
    movq %rax, %rdi           # 1st arg: syscall_num
    movq 40(%rsp), %rsi       # 2nd arg: arg1 (original rdi)
    movq 32(%rsp), %rdx       # 3rd arg: arg2 (original rsi)
    movq 24(%rsp), %rcx       # 4th arg: arg3 (original rdx)
    movq 16(%rsp), %r8        # 5th arg: arg4 (original r10)
    movq 8(%rsp), %r9         # 6th arg: arg5 (original r8)

    # Call the dispatcher
    call syscall_dispatch

    # Clean up argument saves
    addq $48, %rsp            # Remove 6 pushed arguments

    # Restore callee-saved registers
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx

    # Restore user context
    popq %r11                 # Restore user RFLAGS
    popq %rcx                 # Restore user RIP

    # Return value is already in rax
    # SYSRET will:
    # - Load RIP from rcx
    # - Load RFLAGS from r11
    # - Set CPL to 3
    # - Return to user mode
    sysretq

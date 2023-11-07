.extern start_kernel

    .section .text.init    # 将 _start 放入.text.init section
    .globl _start
_start:
    # ------------------
    # - your code here -
    # ------------------

    # set stvec = _traps
    la t0, _traps
    csrw stvec, t0

    # enable supervisor time interrupt
    csrr t0, sie
    ori t1, t0, 0x00000020
    csrw sie, t1

    # set first time interrupt
    rdtime t0
    li a0, 10000000
    add a0, t0, a0
    li a1, 0 
    li a2, 0 
    li a3, 0 
    li a4, 0 
    li a5, 0 
    li a6, 0 
    li a7, 0 
    ecall

    # set sstatus[SIE] = 1
    csrr t0, sstatus
    ori t1, t0, 0x00000002
    csrw sstatus, t1

    la sp, boot_stack_top
    call task_init
    jal start_kernel  # jump to start_kernel

    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # <-- change to your stack size

    .globl boot_stack_top
boot_stack_top:
.extern start_kernel

    .section .text.init    # 将 _start 放入.text.init section
    .globl _start
_start:
    li t0, -0xffffffdf80000000 # PA2VA_OFFSET
    la sp, boot_stack_top
    add sp, sp, t0
    call setup_vm
    call relocate
    call mm_init
    call setup_vm_final
    call task_init
    
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

    jal start_kernel  # jump to start_kernel

relocate:
    # set ra = ra + PA2VA_OFFSET
    # set sp = sp + PA2VA_OFFSET (If you have set the sp before)
    li t0, 0xffffffdf80000000 # PA2VA_OFFSET
    add ra, ra, t0
    add sp, sp, t0

    # set satp with early_pgtbl
    la t0, early_pgtbl
    li t1, 8
    slli t1, t1, 60     # mode 部分设置为 8
    srli t0, t0, 12     # PPN 部分设置为页表物理地址右移 12 位
    or t0, t0, t1
    csrw satp, t0

    # flush tlb
    sfence.vma zero, zero
		
    # flush icache
    fence.i
		
    ret

    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # <-- change to your stack size

    .globl boot_stack_top
boot_stack_top:
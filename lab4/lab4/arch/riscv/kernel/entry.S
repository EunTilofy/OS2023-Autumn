.section .text.entry

#define context_size 512
    .align 2
    .globl _traps 
_traps:

    # swap sp and sscratch
    csrrw sp, sscratch, sp
    beqz sp, _ignore_switch

    csrrw t0, sscratch, t0
    addi sp, sp, -context_size
    sd t0, 16(sp)
    csrrw t0, sscratch, t0

    j _start_save_context

_ignore_switch:
    
    # just read back current sp
    csrr sp, sscratch
    addi sp, sp, -context_size
    sd x0, 16(sp)

_start_save_context:

    csrw sscratch, x0
    
    sd x1, 8(sp)
    sd x3, 24(sp)
    sd x4, 32(sp)
    sd x5, 40(sp)
    sd x6, 48(sp)
    sd x7, 56(sp)
    sd x8, 64(sp)
    sd x9, 72(sp)
    sd x10, 80(sp)
    sd x11, 88(sp)
    sd x12, 96(sp)
    sd x13, 104(sp)
    sd x14, 112(sp)
    sd x15, 120(sp)
    sd x16, 128(sp)
    sd x17, 136(sp)
    sd x18, 144(sp)
    sd x19, 152(sp)
    sd x20, 160(sp)
    sd x21, 168(sp)
    sd x22, 176(sp)
    sd x23, 184(sp)
    sd x24, 192(sp)
    sd x25, 200(sp)
    sd x26, 208(sp)
    sd x27, 216(sp)
    sd x28, 224(sp)
    sd x29, 232(sp)
    sd x30, 240(sp)
    sd x31, 248(sp)

    # save sepc
    csrr t0, sepc
    sd t0, 0(sp)
    sd t0, 256(sp)

    csrr t0, scause
    mv a0, t0
    csrr t0, sepc
    mv a1, t0
    mv a2, sp
    call trap_handler

    ld t0, 0(sp)

    # temporarily add 4 to sepc if it's not clock trap
    li t1, 0x8000000000000005
    csrr a0, scause
    beq a0, t1, _csrwrite
    addi t0, t0, 4

_csrwrite:
    csrw sepc, t0

    ld t0, 16(sp)
    addi t1, sp, context_size
    sd t1, 16(sp)
    beqz t0, _restore_cont
    
    # restore sscratch
    addi sp, sp, context_size
    csrw sscratch, sp
    addi sp, sp, -context_size
    sd t0, 16(sp)

_restore_cont:

    ld x1, 8(sp)
    ld x3, 24(sp)
    ld x4, 32(sp)
    ld x5, 40(sp)
    ld x6, 48(sp)
    ld x7, 56(sp)
    ld x8, 64(sp)
    ld x9, 72(sp)
    ld x10, 80(sp)
    ld x11, 88(sp)
    ld x12, 96(sp)
    ld x13, 104(sp)
    ld x14, 112(sp)
    ld x15, 120(sp)
    ld x16, 128(sp)
    ld x17, 136(sp)
    ld x18, 144(sp)
    ld x19, 152(sp)
    ld x20, 160(sp)
    ld x21, 168(sp)
    ld x22, 176(sp)
    ld x23, 184(sp)
    ld x24, 192(sp)
    ld x25, 200(sp)
    ld x26, 208(sp)
    ld x27, 216(sp)
    ld x28, 224(sp)
    ld x29, 232(sp)
    ld x30, 240(sp)
    ld x31, 248(sp)

_traps_sret:

    # resume sp
    ld sp, 16(sp)
    sret

    .globl __dummy
__dummy:
    csrr t0, sscratch
    csrw sscratch, sp
    mv sp, t0;
    csrwi sepc, 0
    sret

    .globl __switch_to
__switch_to:
    mv t0, a0
    sd ra, 48(t0)
    sd sp, 56(t0)
    sd s0, 64(t0)
    sd s1, 72(t0)
    sd s2, 80(t0)
    sd s3, 88(t0)
    sd s4, 96(t0)
    sd s5, 104(t0)
    sd s6, 112(t0)
    sd s7, 120(t0)
    sd s8, 128(t0)
    sd s9, 136(t0)
    sd s10, 144(t0)
    sd s11, 152(t0)
    csrr t1, sepc
    sd t1,160(a0)
    csrr t1, sstatus
    sd t1,168(a0)
    csrr t1, sscratch
    sd t1,176(a0)
    csrr t1, satp
    sd t1,184(a0)

    mv t1, a1
    ld ra, 48(t1)
    ld sp, 56(t1)
    ld s0, 64(t1)
    ld s1, 72(t1)
    ld s2, 80(t1)
    ld s3, 88(t1)
    ld s4, 96(t1)
    ld s5, 104(t1)
    ld s6, 112(t1)
    ld s7, 120(t1)
    ld s8, 128(t1)
    ld s9, 136(t1)
    ld s10, 144(t1)
    ld s11, 152(t1)
    ld t1,160(a1)
    csrw sepc, t1
    ld t1,168(a1)
    csrw sstatus, t1
    ld t1,176(a1)
    csrw sscratch, t1
    ld t1,184(a1)
    csrw satp, t1

    sfence.vma zero, zero
    fence.i

    ret
.section .text.entry

#define context_size 256
    .align 2
    .globl _traps 
_traps:

    # allocate context struct
    addi sp, sp, -context_size
    
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

    csrr t0, scause
    mv a0, t0
    csrr t0, sepc
    mv a1, t0
    call trap_handler

    ld t0, 0(sp)
    csrw sepc, t0

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
    
    addi sp, sp, context_size
    sret

    .globl __dummy
__dummy:
    la t0, dummy
    csrw sepc, t0
    sret

    .globl __switch_to
__switch_to:
    # save state to prev process
    mv t0, a0
    mv t1, a1
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

    # restore state from next process
    ld ra, 48(t0)
    ld sp, 56(t0)
    ld s0, 64(t0)
    ld s1, 72(t0)
    ld s2, 80(t0)
    ld s3, 88(t0)
    ld s4, 96(t0)
    ld s5, 104(t0)
    ld s6, 112(t0)
    ld s7, 120(t0)
    ld s8, 128(t0)
    ld s9, 136(t0)
    ld s10, 144(t0)
    ld s11, 152(t0)

    ret
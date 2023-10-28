#include "sbi.h"


struct sbiret sbi_ecall(int ext, int fid, uint64 arg0,
			            uint64 arg1, uint64 arg2,
			            uint64 arg3, uint64 arg4,
			            uint64 arg5) 
{
	struct sbiret result;
    __asm__ volatile (
        "lw a7, %2\n"
        "lw a6, %3\n"
        "ld a0, %4\n"
        "ld a1, %5\n"
        "ld a2, %6\n"
        "ld a3, %7\n"
        "ld a4, %8\n"
        "ld a5, %9\n"
        "ecall\n"
        "mv %0, a0\n"
        "mv %1, a1\n"
        : "=r" (result.error), "=r" (result.value)
        : "m" (ext), "m" (fid), 
          "m" (arg0), "m" (arg1), "m" (arg2), "m" (arg3), "m" (arg4), "m" (arg5)
        : "memory"
    );
    return result;
}





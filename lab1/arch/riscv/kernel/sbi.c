#include "sbi.h"


struct sbiret sbi_ecall(int ext, int fid, uint64 arg0,
			            uint64 arg1, uint64 arg2,
			            uint64 arg3, uint64 arg4,
			            uint64 arg5) 
{
	struct sbiret result;
    __asm__ volatile (
        "\tmv a7, %[ext]\n"
        "\tmv a6, %[fid]\n"
        "\tmv a0, %[arg0]\n"
        "\tmv a1, %[arg1]\n"
        "\tmv a2, %[arg2]\n"
        "\tmv a3, %[arg3]\n"
        "\tmv a4, %[arg4]\n"
        "\tmv a5, %[arg5]\n"
        "\tecall\n"
        "\tmv %[err], a0\n"
        "\tmv %[res], a1\n"
        : [err] "=r" (result.error), [res] "=r" (result.value)
        : [ext] "r" (ext), [fid] "r" (fid), 
          [arg0] "r" (arg0), [arg1] "r" (arg1), [arg2] "r" (arg2), [arg3] "r" (arg3), [arg4] "r" (arg4), [arg5] "r" (arg5)
    );
    return result;
}





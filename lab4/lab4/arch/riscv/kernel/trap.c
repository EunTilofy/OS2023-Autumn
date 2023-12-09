#include "printk.h"
#include "proc.h"
#include "defs.h"
#include "clock.h"

#define SYS_WRITE   64
#define SYS_GETPID  172

struct pt_regs {
    uint64 x[32];
    uint64 sepc;
};

extern struct task_struct* current;

void syscall(struct pt_regs* regs) {
    if (regs->x[17] == SYS_WRITE) {
        if (regs->x[10] == 1) {
            char* buf = (char*)regs->x[11];
            for (int i = 0; i < regs->x[12]; i++) {
                printk("%c", buf[i]);
            }
            regs->x[10] = regs->x[12];
        } else {
            printk("not support fd = %d\n", regs->x[10]);
            regs->x[10] = -1;
        }
    } else if (regs->x[17] == SYS_GETPID) {
        regs->x[10] = current->pid;
    } else {
        printk("not support syscall id = %d\n", regs->x[17]);
    }
    regs->sepc += 4;
}

void trap_handler(uint64 scause, uint64 sepc, struct pt_regs* regs) {
    if ((long) scause < 0 && (scause & ((1ul << 63) - 1)) == 5) {
        // printk("%s", "Get STI!\n");
        clock_set_next_event();
        do_timer();
        return ;
        // printk("[S] Supervisor Mode Timer Interrupt!\n");
    } else if (scause== 8) {
        syscall(regs);
        return;
    }
    uint64 stval = csr_read(stval);
    printk("Unhandled trap: scause = %lx, sepc = %llx, stval = %llx\n", scause, sepc, stval);
}

#ifndef _TRAP_H
#define _TRAP_H

void trap_handler(uint64 scause, uint64 sepc, struct pt_regs *regs);

#endif
#include "printk.h"
#include "proc.h"
#include "defs.h"
#include "clock.h"
#include "mm.h"
#include "vm.h"

#define SYS_WRITE   64
#define SYS_GETPID  172

extern char _sramdisk[];

struct pt_regs {
    uint64 x[32];
    uint64 sepc;
};

extern struct task_struct* current;

void syscall(struct pt_regs* regs) {
    if(regs->x[17]==SYS_WRITE) {
        if(regs->x[10]) {
            for(int i = 0; i < regs->x[12]; ++i)
                printk("%c", ((char*)(regs->x[11]))[i]);
            regs->x[10] = regs->x[12];
        }
    } else if(regs->x[17] == SYS_GETPID) {
        regs->x[10] = current->pid;
    }
    regs->sepc += 4;
}

enum {
    PGF_R = 1,
    PGF_W = 2,
    PGF_X = 3
};

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CEIL(sza, szb) ((((sza) + ((szb) - 1)) & (~((szb) - 1))) / PGSIZE)
#define ROUNDUP(a, sz)      ((((uint64)a) + (sz) - 1) & ~((sz) - 1))
#define ROUNDDOWN(a, sz)    ((((uint64)a)) & ~((sz) - 1))

void do_page_fault(uint64 addr, int type) {
    struct vm_area_struct *find_ret = find_vma(current, addr);
    if (!find_ret) {
        printk("SEGMENT FAULT at %llx no segment type %d\n", addr, type);
        return ;
    }
    uint64 pg_start = ROUNDDOWN(addr, PGSIZE);
    uint64 vm_flags = 0x10;
    if (find_ret->vm_flags & VM_R_MASK)
        vm_flags |= 0x2;
    else if (type == PGF_R) {
        printk("SEGMENT FAULT at %llx at read\n", addr);
        return ;
    }
    if (find_ret->vm_flags & VM_W_MASK)
        vm_flags |= 0x4;
    else if (type == PGF_W) {
        printk("SEGMENT FAULT at %llx at write\n", addr);
        return ;
    }
    if (find_ret->vm_flags & VM_X_MASK)
        vm_flags |= 0x8;
    else if (type == PGF_X) {
        printk("SEGMENT FAULT at %llx at inst\n", addr);
        return ;
    }
    uint64_t sa = kalloc();
    create_mapping(current->pgd, pg_start, sa - PA2VA_OFFSET, PGSIZE, vm_flags);
    memset((void *) sa, 0, PGSIZE);
    if (find_ret->vm_flags & VM_ANONYM)
        return ;
    if (pg_start <= find_ret->vm_start) {
        uint64_t realstart = find_ret->vm_start - pg_start + sa;
        uint64_t bytes_to_copy = MIN(find_ret->vm_content_size_in_file, PGSIZE - (find_ret->vm_start - pg_start));
        // realstart + bytes_to_copy <= PGSIZE + sa
        memcpy((void *) realstart, _sramdisk + find_ret->vm_content_offset_in_file, bytes_to_copy);
    } else {
        uint64_t bytes_rest = find_ret->vm_content_size_in_file - (pg_start - find_ret->vm_start);
        uint64_t real_offset = find_ret->vm_content_offset_in_file + (pg_start - find_ret->vm_start);
        uint64_t bytes_to_copy = MIN(bytes_rest, PGSIZE);
        
        memcpy((void *) pg_start, _sramdisk + real_offset, bytes_to_copy);
    }
}

void trap_handler(uint64 scause, uint64 sepc, struct pt_regs* regs) {
    uint64 stval = csr_read(stval);
    if ((long) scause < 0 && (scause & ((1ul << 63) - 1)) == 5) {
        // printk("%s", "Get STI!\n");
        clock_set_next_event();
        do_timer();
        regs->sepc += 4;
        return ;
        // printk("[S] Supervisor Mode Timer Interrupt!\n");
    } else if (scause== 8) {
        syscall(regs);
        return;
    } else if (scause == 15) { // store
        do_page_fault(stval, PGF_W);
        return ;
    } else if (scause == 13) { // load
        do_page_fault(stval, PGF_R);
        return ;
    } else if (scause == 12) { // inst
        do_page_fault(stval, PGF_X);
        return ;
    }
    printk("Unhandled trap: scause = %lx, sepc = %llx, stval = %llx\n", scause, sepc, stval);
}

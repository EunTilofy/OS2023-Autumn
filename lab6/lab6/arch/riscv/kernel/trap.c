#include "printk.h"
#include "proc.h"
#include "defs.h"
#include "clock.h"
#include "mm.h"
#include "vm.h"

#define SYS_WRITE   64
#define SYS_CLONE   220
#define SYS_GETPID  172

void __ret_from_fork_debug();
extern char _sramdisk[];

struct pt_regs {
    uint64 x[32];
    uint64 sepc;
};

extern uint64 task_test_priority[]; // test_init 后, 用于初始化 task[i].priority 的数组
extern uint64 task_test_counter[];  // test_init 后, 用于初始化 task[i].counter  的数组

extern unsigned long swapper_pg_dir[];
extern struct task_struct* task[];
extern struct task_struct* current;

void clone(struct pt_regs *regs) {
    int new_pid;
    for (new_pid = 1; new_pid < NR_TASKS; new_pid++) {
        if (task[new_pid] == NULL) break;
    }
    if (new_pid == NR_TASKS) {
        printk("TASK FULL");
        return ;
    }
    task[new_pid] = (struct task_struct *) kalloc();
    memcpy(task[new_pid], current, PGSIZE);
    task[new_pid]->counter = task_test_counter[new_pid];
    task[new_pid]->priority = task_test_priority[new_pid];
    task[new_pid]->thread.ra = (uint64_t) __ret_from_fork_debug;
    task[new_pid]->thread.sp = ((uint64_t) regs) - ((uint64_t) current) + ((uint64_t) task[new_pid]);
    struct pt_regs *nregs = (struct pt_regs *) (((uint64_t) regs) - ((uint64_t) current) + ((uint64_t) task[new_pid]));
    nregs->x[10] = 0;
    // no need to adjust context sp (user sp)
    // nregs->x[2] = 
    nregs->sepc = regs->sepc + 4;
    task[new_pid]->thread.sscratch = 0;
    // status? no much problem

    #define NXTPGT(pde) (pagetable_t) ((((pde) >> 10) << 12) + PA2VA_OFFSET)
    #define MKPDE(page_va) (((((page_va) - PA2VA_OFFSET) >> 12) << 10) | 0x1)
    #define GETPERM(pte) ((pte) & 0x3fe)
    #define MKPTE(page_va, perm) (((((page_va) - PA2VA_OFFSET) >> 12) << 10) | perm | 0x1)
    #define GETPGVA(pte) ((((pte >> 10) << 12)) + PA2VA_OFFSET)

    
    // create root pgt
    task[new_pid]->pgd = (pagetable_t) kalloc();
    task[new_pid]->pid = new_pid;
    task[new_pid]->state = TASK_RUNNING;
    uint64 satp = csr_read(satp);
    satp = (satp >> 44) << 44;
    satp |= ((uint64)(task[new_pid]->pgd) - PA2VA_OFFSET) >> 12;
    task[new_pid]->satp = satp;
    memcpy(task[new_pid]->pgd, swapper_pg_dir, PGSIZE);

    for (int i = VPN2(USER_START); i < VPN2(USER_END); i++) {
        // need to copy
        if (current->pgd[i] & 0x1) {

            uint64_t n_pgpa = kalloc();
            pagetable_t sec_pgtbl = NXTPGT(current->pgd[i]);

            // make this pde
            task[new_pid]->pgd[i] = MKPDE(n_pgpa);
            pagetable_t sec_pgtbl_n = (pagetable_t) n_pgpa;
            

            for (int j = 0; j < PGSIZE / sizeof(pagetable_t); j++) {
                if (sec_pgtbl[j] & 0x1) {
                    n_pgpa = kalloc();
                    pagetable_t thi_pgtbl = NXTPGT(sec_pgtbl[j]);
                    
                    sec_pgtbl_n[j] = MKPDE(n_pgpa);
                    pagetable_t thi_pgtbl_n = (pagetable_t) n_pgpa;

                    for (int k = 0; k < PGSIZE / sizeof(pagetable_t); k++) {
                        if (thi_pgtbl[k] & 0x1) {
                            n_pgpa = kalloc();
                            thi_pgtbl_n[k] = MKPTE(n_pgpa, GETPERM(thi_pgtbl[k]));
                            memcpy((void *) n_pgpa, (void *) GETPGVA(thi_pgtbl[k]), PGSIZE);
                        }
                    }
                }
            }
        }
    }

    // task[new_pid]->thread.sepc
    regs->x[10] = new_pid;
    return ;
}

void syscall(struct pt_regs* regs) {
    if(regs->x[17]==SYS_WRITE) {
        if(regs->x[10]) {
            for(int i = 0; i < regs->x[12]; ++i)
                printk("%c", ((char*)(regs->x[11]))[i]);
            regs->x[10] = regs->x[12];
        }
    } else if(regs->x[17] == SYS_GETPID) {
        regs->x[10] = current->pid;
    } else if (regs->x[17] == SYS_CLONE) {
        clone(regs);
    } else {
        printk("UNHANDLED SYSCALL %llu\n", regs->x[17]);
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
        printk("[S] PAGE FAULT stval %llx scause %llx sepc %llx\n", stval, scause, sepc);
        do_page_fault(stval, PGF_W);
        return ;
    } else if (scause == 13) { // load
        printk("[S] PAGE FAULT stval %llx scause %llx sepc %llx\n", stval, scause, sepc);
        do_page_fault(stval, PGF_R);
        return ;
    } else if (scause == 12) { // inst
        printk("[S] PAGE FAULT stval %llx scause %llx sepc %llx\n", stval, scause, sepc);
        do_page_fault(stval, PGF_X);
        return ;
    }
    printk("Unhandled trap: scause = %lx, sepc = %llx, stval = %llx\n", scause, sepc, stval);
}

//arch/riscv/kernel/proc.c
#include "proc.h"
#include "mm.h"
#include "defs.h"
#include "rand.h"
#include "printk.h"
#include "test.h"
#include "vm.h"
#include <elf.h>

# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
# define Elf_Half Elf64_Half
# define Elf_Off Elf64_Off

//arch/riscv/kernel/proc.c

extern void __dummy();

struct task_struct* idle;           // idle process
struct task_struct* current;        // 指向当前运行线程的 `task_struct`
struct task_struct* task[NR_TASKS]; // 线程数组, 所有的线程都保存在此

/**
 * new content for unit test of 2023 OS lab2
*/

extern uint64 task_test_priority[]; // test_init 后, 用于初始化 task[i].priority 的数组
extern uint64 task_test_counter[];  // test_init 后, 用于初始化 task[i].counter  的数组

extern char _sramdisk[];
extern char _eramdisk[];
extern unsigned long swapper_pg_dir[];

void task_init() {
    printk("Entering task init\n");
    test_init(NR_TASKS);
    // 1. 调用 kalloc() 为 idle 分配一个物理页
    // 2. 设置 state 为 TASK_RUNNING;
    // 3. 由于 idle 不参与调度 可以将其 counter / priority 设置为 0
    // 4. 设置 idle 的 pid 为 0
    // 5. 将 current 和 task[0] 指向 idle

    /* YOUR CODE HERE */
    // mm_init();
    idle = (struct task_struct *)kalloc();
    if(!idle) return;
    current = task[0] = idle;
    idle->state = TASK_RUNNING;
    idle->counter = 0;
    idle->priority = 0;
    idle->pid = 0;

    // 1. 参考 idle 的设置, 为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    // 2. 其中每个线程的 state 为 TASK_RUNNING, 此外，为了单元测试的需要，counter 和 priority 进行如下赋值：
    //      task[i].counter  = task_test_counter[i];
    //      task[i].priority = task_test_priority[i];
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 `thread_struct` 中的 `ra` 和 `sp`,
    // 4. 其中 `ra` 设置为 __dummy （见 4.3.2）的地址,  `sp` 设置为 该线程申请的物理页的高地址

    /* YOUR CODE HERE */
    for (int i = 2; i < NR_TASKS; ++i)
        task[i] = NULL;
    for(int i = 1; i < 2; ++i) {
        task[i] = (struct task_struct *)kalloc();
        if(!task[i]) return;
        /***** addition ******/
        task[i]->vma_cnt = 0;
        task[i]->thread.sepc = USER_START;
        task[i]->thread.sstatus &= ~(1 << 8);
        task[i]->thread.sstatus |= (1 << 5);
        task[i]->thread.sstatus |= (1 << 18);
        task[i]->thread.sscratch = USER_END;
        task[i]->pgd = (unsigned long*)kalloc();
        memcpy(task[i]->pgd, swapper_pg_dir, PGSIZE);
        task[i]->thread.sp = (uint64_t)task[i] + PGSIZE;
        // task[i]->thread_info = (struct thread_info *) kalloc();
        task[i]->thread_info.kernel_sp = (uint64_t)task[i] + PGSIZE;
        task[i]->thread_info.user_sp = (uint64_t)kalloc();

        #define STACK_SIZE (PGSIZE << 4)
        #define UAPP_SIZE (1 << 24) // 16 MiB

        uint64_t va = USER_END - STACK_SIZE;
        // uint64_t pa = (uint64)(task[i]->thread_info.user_sp) - PA2VA_OFFSET;
        // create_mapping(task[i]->pgd, va, pa, STACK_SIZE, 0x17);
        do_mmap(task[i], va, STACK_SIZE, VM_R_MASK | VM_W_MASK | VM_ANONYM, 0, 0);

        Elf_Ehdr *header = (void *) _sramdisk;
        //  fs_lseek(target_fd, 0, SEEK_SET);
        Elf_Half phnum = header->e_phnum;
        Elf_Off phoff = header->e_phoff;
        printk("get %u segments\n", phnum);
        while (phnum--) {
            Elf_Phdr *segment = (void *) _sramdisk + phoff;
            phoff += header->e_phentsize;
            printk("Segment at 0x%08x with memory size 0x%06x\n", segment->p_vaddr, segment->p_memsz);
            if (!(segment->p_type == PT_LOAD)) {
                printk("Not a PT_LOAD segment, ignoring...\n");
                continue;
            }
            
            void *file_pt = _sramdisk + segment->p_offset;
            #define MIN(a, b) ((a) < (b) ? (a) : (b))
            #define CEIL(sza, szb) ((((sza) + ((szb) - 1)) & (~((szb) - 1))) / PGSIZE)
            #define ROUNDUP(a, sz)      ((((uint64_t)a) + (sz) - 1) & ~((sz) - 1))
            #define ROUNDDOWN(a, sz)    ((((uint64_t)a)) & ~((sz) - 1))

            uint64_t seg_flag_vma = 0;
            if (segment->p_flags & PF_R)
                seg_flag_vma |= VM_R_MASK;
            if (segment->p_flags & PF_W)
                seg_flag_vma |= VM_W_MASK;
            if (segment->p_flags & PF_X)
                seg_flag_vma |= VM_X_MASK;

            do_mmap(task[i], segment->p_vaddr, segment->p_memsz, seg_flag_vma, segment->p_offset, segment->p_filesz);

            printk("Load finished\n");
        }

        uint64 satp = csr_read(satp);
        satp = (satp >> 44) << 44;
        satp |= ((uint64)(task[i]->pgd) - PA2VA_OFFSET) >> 12;
        task[i]->satp = satp;

        /*********************/
        task[i]->state = TASK_RUNNING;
        task[i]->counter = task_test_counter[i];
        task[i]->priority = task_test_priority[i];
        task[i]->pid = i;
        task[i]->thread.ra = (uint64)__dummy;
        task[i]->thread.sp = PGSIZE + (long)task[i];
        // printk("task[%d] address = %x\n", i, task[i]);
    }

    printk("...proc_init done!\n");
}

// arch/riscv/kernel/proc.c
void dummy() {
    schedule_test();
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while(1) {
        if ((last_counter == -1 || current->counter != last_counter) && current->counter > 0) {
            //printk("dummy : %d %d %d %d\n", task[1]->counter, task[2]->counter, task[3]->counter, task[4]->counter);
            if(current->counter == 1){
                --(current->counter);   // forced the counter to be zero if this thread is going to be scheduled
            }                           // in case that the new counter is also 1, leading the information not printed.
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            printk("[PID = %d] is running. auto_inc_local_var = %d\n", current->pid, auto_inc_local_var);
        }
    }
}

extern void __switch_to(struct task_struct* prev, struct task_struct* next);

// void __switch_to(struct task_struct* prev, struct task_struct* next) {
//     __asm__ volatile (
//         "\tsd sp, %[prev_sp]\n"
//         "\tsd ra, %[prev_ra]\n"
//         "\tsd s0, %[prev_s0]\n"
//         "\tsd s1, %[prev_s1]\n"
//         "\tsd s2, %[prev_s2]\n"
//         "\tsd s3, %[prev_s3]\n"
//         "\tsd s4, %[prev_s4]\n"
//         "\tsd s5, %[prev_s5]\n"
//         "\tsd s6, %[prev_s6]\n"
//         "\tsd s7, %[prev_s7]\n"
//         "\tsd s8, %[prev_s8]\n"
//         "\tsd s9, %[prev_s9]\n"
//         "\tsd s10, %[prev_s10]\n"
//         "\tsd s11, %[prev_s11]\n"
//     :   [prev_sp] "=m" (prev->thread.sp),     [prev_ra] "=m" (prev->thread.ra),     [prev_s0] "=m" (prev->thread.s[0]),
//         [prev_s1] "=m" (prev->thread.s[1]),   [prev_s2] "=m" (prev->thread.s[2]),   [prev_s3] "=m" (prev->thread.s[3]),
//         [prev_s4] "=m" (prev->thread.s[4]),   [prev_s5] "=m" (prev->thread.s[5]),   [prev_s6] "=m" (prev->thread.s[6]),
//         [prev_s7] "=m" (prev->thread.s[7]),   [prev_s8] "=m" (prev->thread.s[8]),   [prev_s9] "=m" (prev->thread.s[9]),
//         [prev_s10] "=m" (prev->thread.s[10]),  [prev_s11] "=m" (prev->thread.s[11])
//     :
//     );
//     __asm__ volatile (
//         "\tld ra, %[next_ra]\n"
//         "\tld sp, %[next_sp]\n"
//         "\tld s0, %[next_s0]\n"
//         "\tld s1, %[next_s1]\n"
//         "\tld s2, %[next_s2]\n"
//         "\tld s3, %[next_s3]\n"
//         "\tld s4, %[next_s4]\n"
//         "\tld s5, %[next_s5]\n"
//         "\tld s6, %[next_s6]\n"
//         "\tld s7, %[next_s7]\n"
//         "\tld s8, %[next_s8]\n"
//         "\tld s9, %[next_s9]\n"
//         "\tld s10, %[next_s10]\n"
//         "\tld s11, %[next_s11]\n"
//     ::  [next_sp] "m" (next->thread.sp),     [next_ra] "m" (next->thread.ra),     [next_s0] "m" (next->thread.s[0]),
//         [next_s1] "m" (next->thread.s[1]),   [next_s2] "m" (next->thread.s[2]),   [next_s3] "m" (next->thread.s[3]),
//         [next_s4] "m" (next->thread.s[4]),   [next_s5] "m" (next->thread.s[5]),   [next_s6] "m" (next->thread.s[6]),
//         [next_s7] "m" (next->thread.s[7]),   [next_s8] "m" (next->thread.s[8]),   [next_s9] "m" (next->thread.s[9]),
//         [next_s10] "m" (next->thread.s[10]),  [next_s11] "m" (next->thread.s[11])
//     );
// }

void switch_to(struct task_struct* next) {
    /* YOUR CODE HERE */
    // printk("Now exec switch_to!\n");
    if(!current) return;
    if(!next) return;
    // printk("next pid = %d, address = %x", next->pid, next);
    if(current->pid != next->pid) {
        struct task_struct *prev = current;
        current = next;
        // printk("go __switch_to\n");
        // printk("before __switch-to / counter : %d %d %d %d\n", task[1]->counter, task[2]->counter, task[3]->counter, task[4]->counter);
        __switch_to(prev, next);
    }
}

void do_timer(void) {
    // 1. 如果当前线程是 idle 线程 直接进行调度
    // 2. 如果当前线程不是 idle 对当前线程的运行剩余时间减1 若剩余时间仍然大于0 则直接返回 否则进行调度

    /* YOUR CODE HERE */
    if(!current) return;
    // printk("do-timer / counter : %d %d %d %d\n", task[1]->counter, task[2]->counter, task[3]->counter, task[4]->counter);
    //printk("Enter do_timer!\n current->pid = %d\n", current->pid);
    if(current->pid == 0) {
        // printk("Now run idle~\n");
        schedule();
    }
    else {
        if(current->counter > 0) --current->counter;
        //printk("Current pid : %d, Current->counter : %d, Current->state : %d\n", current->pid, current->counter, current->state);
        if(current->counter == 0) {
            current->state = !TASK_RUNNING;
            //printk("call schedule! current->counter = %d %d\n", current->counter, task[1]->counter);
            schedule();
        }
    }
}

void do_mmap(struct task_struct *task, uint64_t addr, uint64_t length, uint64_t flags,
    uint64_t vm_content_offset_in_file, uint64_t vm_content_size_in_file) {
    
    #define nowvma task->vmas[task->vma_cnt]
    if (task->vma_cnt >= 49) {
        printk("Warning: too many vm areas!");
    }
    
    nowvma.vm_start = addr;
    nowvma.vm_end = addr + length;
    nowvma.vm_content_offset_in_file = vm_content_offset_in_file;
    nowvma.vm_content_size_in_file = vm_content_size_in_file;
    nowvma.vm_flags = flags;

    #undef nowvma
    task->vma_cnt++;
}

struct vm_area_struct *find_vma(struct task_struct *task, uint64_t addr) {
    int vma_idx;
    #define nowvma task->vmas[vma_idx]
    // more semantic information can help compile-time checks
    #define in(addr, begin, end) (((addr) >= (begin)) && ((addr) < (end)))

    for (vma_idx = 0; vma_idx < task->vma_cnt; vma_idx++)
        if (in(addr, nowvma.vm_start, nowvma.vm_end))
            break;

    if (vma_idx == task->vma_cnt) return NULL;
    else return (task->vmas) + vma_idx;

    #undef nowvma
    #undef in
}

#ifdef DSJF
void schedule(void) {
    /* YOUR CODE HERE */
    int next_id = -1;

    // printk("schedule / counter : %d %d %d %d\n", task[1]->counter, task[2]->counter, task[3]->counter, task[4]->counter);

    for(int i = 1; i < NR_TASKS; ++i) {
        if(task[i] && task[i]->counter > 0 && task[i]->state == TASK_RUNNING &&
            (next_id==-1 || task[i]->counter < task[next_id]->counter)) {
            next_id = i;
        }
    }
    if(~next_id) {
        printk("switch to [PID = %d COUNTER = %d]\n", next_id, task[next_id]->counter);
        switch_to(task[next_id]);
    } else {
        for(int i = 1; i < NR_TASKS; ++i) if(task[i]) {
            task[i]->state = TASK_RUNNING;
            task[i]->counter = rand();
            printk("SET [PID = %d COUNTER = %d]\n", i, task[i]->counter);
        }
        schedule();
    }
}
#endif

#ifdef DPRIORITY
void schedule(void) {
    /* YOUR CODE HERE */
    int next_id = -1;

    // printk("schedule / counter : %d %d %d %d\n", task[1]->counter, task[2]->counter, task[3]->counter, task[4]->counter);

    for(int i = 1; i < NR_TASKS; ++i) {
        if(task[i] && task[i]->counter > 0 && task[i]->state == TASK_RUNNING &&
            (next_id==-1 || task[i]->counter >= task[next_id]->counter)) {
            next_id = i;
        }
    }
    if(~next_id) {
        // for(int i = 1; i < NR_TASKS; ++i) {
        //     task[i]->counter = (task[i]->counter>>1) + task[i]->priority;
        // }
        printk("switch to [PID = %d PRIORITY = %d COUNTER = %d]\n", next_id, task[next_id]->priority, task[next_id]->counter);
        switch_to(task[next_id]);
    } else {
        for(int i = 1; i < NR_TASKS; ++i) if (task[i]) {
            task[i]->state = TASK_RUNNING;
            task[i]->counter = task[i]->priority + 1;
            printk("SET [PID = %d PRIORITY = %d COUNTER = %d]\n", i, task[i]->priority, task[i]->counter);
        }
        schedule();
    }
}
#endif

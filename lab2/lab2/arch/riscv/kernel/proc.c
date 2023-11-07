//arch/riscv/kernel/proc.c
#include "proc.h"
#include "mm.h"
#include "defs.h"
#include "rand.h"
#include "printk.h"
#include "test.h"

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

void task_init() {
    test_init(NR_TASKS);
    // 1. 调用 kalloc() 为 idle 分配一个物理页
    // 2. 设置 state 为 TASK_RUNNING;
    // 3. 由于 idle 不参与调度 可以将其 counter / priority 设置为 0
    // 4. 设置 idle 的 pid 为 0
    // 5. 将 current 和 task[0] 指向 idle

    /* YOUR CODE HERE */
    mm_init();
    idle = (struct task_struct *)kalloc();
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
    for(int i = 1; i < NR_TASKS; ++i) {
        task[i] = (struct task_struct *)kalloc();
        if(!task[i]) return;
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
        printk("before __switch-to / counter : %d %d %d %d\n", task[1]->counter, task[2]->counter, task[3]->counter, task[4]->counter);
        __switch_to(prev, next);
    }
}

void do_timer(void) {
    // 1. 如果当前线程是 idle 线程 直接进行调度
    // 2. 如果当前线程不是 idle 对当前线程的运行剩余时间减1 若剩余时间仍然大于0 则直接返回 否则进行调度

    /* YOUR CODE HERE */
    if(!current) return;
    printk("do-timer / counter : %d %d %d %d\n", task[1]->counter, task[2]->counter, task[3]->counter, task[4]->counter);
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

#ifdef DSJF
void schedule(void) {
    /* YOUR CODE HERE */
    int next_id = -1;
    uint64 min_time = (1ull << 50);

    printk("schedule / counter : %d %d %d %d\n", task[1]->counter, task[2]->counter, task[3]->counter, task[4]->counter);

    for(int i = 1; i < NR_TASKS; ++i) {
        if(task[i]->counter > 0 && task[i]->state == TASK_RUNNING &&
            (next_id==-1 || task[i]->counter < task[next_id]->counter)) {
            next_id = i;
        }
    }
    if(~next_id) {
        printk("switch to [PID = %d COUNTER = %d]\n", next_id, task[next_id]->counter);
        switch_to(task[next_id]);
    } else {
        for(int i = 1; i < NR_TASKS; ++i) {
            task[i]->state = TASK_RUNNING;
            task[i]->counter = rand();
            printk("SET [PID = %d COUNTER = %d]\n", i, task[i]->counter);
        }
    }
}
#endif

#ifdef DPRIORITY
void schedule(void) {
    /* YOUR CODE HERE */
    int next_id = -1;
    uint64 min_time = (1ull << 50);

    printk("schedule / counter : %d %d %d %d\n", task[1]->counter, task[2]->counter, task[3]->counter, task[4]->counter);

    for(int i = 1; i < NR_TASKS; ++i) {
        if(task[i]->counter > 0 && task[i]->state == TASK_RUNNING &&
            (next_id==-1 || task[i]->priority > task[next_id]->priority)) {
            next_id = i;
        }
    }
    if(~next_id) {
        printk("switch to [PID = %d PRIORITY = %d COUNTER = %d]\n", next_id, max_priority, task[next_id]->counter);
        switch_to(task[next_id]);
    } else {
        for(int i = 1; i < NR_TASKS; ++i) {
            task[i]->state = TASK_RUNNING;
            task[i]->counter = rand();
            printk("SET [PID = %d PRIORITY = %d COUNTER = %d]\n", i, task[i]->priority, task[i]->counter);
        }
    }
}
#endif

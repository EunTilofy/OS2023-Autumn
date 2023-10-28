# Lab1：RV64内核引导与时钟中断处理

<br/>

<center><font size=2.5>张志心 <code>3210106357</code></font></center>

<br/>

## 实验目的

- 学习 RISC-V 汇编， 编写 head.S 实现跳转到内核运行的第一个 C 函数。
- 学习 OpenSBI，理解 OpenSBI 在实验中所起到的作用，并调用 OpenSBI 提供的接口完成字符的输出。
- 学习 Makefile 相关知识， 补充项目中的 Makefile 文件， 来完成对整个工程的管理。
- 学习 RISC-V 的 trap 处理相关寄存器与指令，完成对 trap 处理的初始化。
- 理解 CPU 上下文切换机制，并正确实现上下文切换功能。
- 编写 trap 处理函数，完成对特定 trap 的处理。
- 调用 OpenSBI 提供的接口，完成对时钟中断事件的设置。

## 实验环境

- 本次实验环境为 Mac 系统下 Ubuntu22.04 VM。（不同于 Lab0）。

  环境配置过程与 Lab0 类似。

## 实验步骤

### RV64 内核引导

#### 编写 head.S

为即将运行的第一个 C 函数设置程序栈，大小为 4kb，将栈放置在 `.bss.stack` 段。

然后通过跳转指令，跳转至 `main.c` 中的 `start_kernel`。

```assembly
# head.S
.extern start_kernel

    .section .text.entry
    .globl _start
_start:
    # ------------------
    # - your code here -
    # ------------------
    la sp, boot_stack_top
    jal start_kernel

    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # <-- change to your stack size

    .globl boot_stack_top
boot_stack_top:
```

#### 完善 Makefile 脚本

补充 `lib/Makefile` 如下：

```makefile
SRCS = $(shell find . -name *"*.S") $(shell find . -name "*.c")
OBJS = $(addsuffix .o, $(basename $(SRCS)))

all: $(OBJS)

%.o: %.S
	@echo CC $< $@
	@$(GCC) $(CFLAG) -c $< -o $@
	

%.o: %.c
	@echo CC $< $@
	@$(GCC) $(CFLAG) -c $< -o $@
	

clean:
	-@rm *.o 2>/dev/null
```

#### 补充 sbi.c

1. 将 ext (Extension ID) 放入寄存器 a7 中，fid (Function ID) 放入寄存器 a6 中，将 arg0 ~ arg5 放入寄存器 a0 ~ a5 中。
2. 使用 `ecall` 指令。`ecall` 之后系统会进入 M 模式，之后 OpenSBI 会完成相关操作。
3. OpenSBI 的返回结果会存放在寄存器 a0 ， a1 中，其中 a0 为 error code， a1 为返回值， 我们用 sbiret 来接受这两个返回值。

```c++
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
```

#### 修改 defs

```c
#define csr_read(csr)                               \
({                                                  \
    uint64 __v;                                     \
    asm volatile("csrr %[v]," #csr                  \
                : [v] "=r" (__v) : : "memory")      \
    __v;                                            \
})
```

#### qemu 运行 `make` 得到内核

在 `/lab1` 目录下进行 `make`

<img src="/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab1/assets/image-20231028220348957.png" alt="image-20231028220348957" style="zoom:50%;" />

### RV64 时钟中断处理

+ 准备工作

```assembly
# vmlinux.lds
.text : ALIGN(0x1000){
    _stext = .;

    *(.text.init)      <- 加入了 .text.init
    *(.text.entry)     <- 之后我们实现 中断处理逻辑 会放置在 .text.entry
    *(.text .text.*)

    _etext = .;
}
# head.S
.extern start_kernel

    # .section .text.entry
    .section .text.init         <- 将 _start 放入.text.init section 
    .globl _start
```

#### 开启 trap 处理

1. 设置 `stvec`， 将 `_traps` 所表示的地址写入 `stvec`，这里我们采用 `Direct 模式`, 而 `_traps` 则是 trap 处理入口函数的基地址。
2. 开启时钟中断，将 `sie[STIE]` 置 1。
3. 设置第一次时钟中断，参考 `clock_set_next_event()` ( `clock_set_next_event()` 在 4.3.4 中介绍 ) 中的逻辑用汇编实现。
4. 开启 S 态下的中断响应， 将 `sstatus[SIE]` 置 1。

```assembly
    # set stvec = _traps
    la t0, _traps
    csrw stvec, t0

    # enable supervisor time interrupt
    csrr t0, sie
    ori t1, t0, 0x00000020
    csrw sie, t1

    # set first time interrupt
    rdtime t0
    li a0, 10000000
    add a0, t0, a0
    li a1, 0 
    li a2, 0 
    li a3, 0 
    li a4, 0 
    li a5, 0 
    li a6, 0 
    li a7, 0 
    ecall

    # set sstatus[SIE] = 1
    csrr t0, sstatus
    ori t1, t0, 0x00000002
    csrw sstatus, t1
```



#### 实现上下文切换

使用汇编实现上下文切换机制， 包含以下几个步骤：

1. 在 `arch/riscv/kernel/` 目录下添加 `entry.S` 文件。
2. 保存 CPU 的寄存器（上下文）到内存中（栈上）。
3. 将 `scause` 和 `sepc` 中的值传入 trap 处理函数 `trap_handler` ( `trap_handler` 在 4.3.3 中介绍 ) ，我们将会在 `trap_handler` 中实现对 trap 的处理。
4. 在完成对 trap 的处理之后， 我们从内存中（栈上）恢复CPU的寄存器（上下文）。
5. 从 trap 中返回。

```assembly
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
```



#### 实现 trap 处理函数

在 `trap.c` 中实现 trap 处理函数 `trap_handler()`, 其接收的两个参数分别是 `scause` 和 `sepc` 两个寄存器中的值。

```c++
void trap_handler(unsigned long scause, unsigned long sepc) {
    if ((long) scause < 0 && (scause & ((1l << 63) - 1)) == 5) {
        printk("%s", "Get STI!\n");
        clock_set_next_event();
    }
}
```



#### 实现时钟中断相关函数

1. 在 `clock.c` 中实现 get_cycles ( ) : 使用 `rdtime` 汇编指令获得当前 `time` 寄存器中的值。
2. 在 `clock.c` 中实现 clock_set_next_event ( ) : 调用 `sbi_ecall`，设置下一个时钟中断事件。

```c++
unsigned long get_cycles() {
    unsigned long m_time;
    __asm__ volatile (
        "rdtime t0\n"
        "mv %[m_time], t0\n"
        : [m_time] "=r" (m_time)
    );
    return m_time;
}

void clock_set_next_event() {
    unsigned long next = get_cycles() + TIMECLOCK;

    sbi_ecall(0x0, 0x0, next, 0, 0, 0, 0, 0);
} 
```



#### 编译及测试

```c++
// test.c
void test() {
    for(int i = 1; i <= 120000000; ++i) {
        if(i == 120000000) {
            printk("%s", "kernel is running!\n");
            i = 0;
        }
    }
}
```

<img src="/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab1/assets/image-20231028223257525.png" alt="image-20231028223257525" style="zoom:50%;" />

## 思考题

1. 请总结一下 RISC-V 的 calling convention，并解释 Caller / Callee Saved Register 有什么区别？

   > （1）calling convention 即调用规约：
   >
   > + 将参数放到寄存器或栈上；
   > + 按需将调用者保存寄存器的值压到栈上；
   > + 使用 `jal` 或 `jalr` 指令，调用函数；
   > + 被调用者按需保存被调用者保存寄存器；
   > + 运行被调用函数代码；
   > + 恢复被调用者保存寄存器；
   > + 执行 `ret` 返回；
   > + 恢复调用者保存寄存器。
   >
   > （2）假设 A 调用 B，调用者保存寄存器（Caller）是 A 在调用 B 之前，需要将其值压到栈上保存，并在 B 返回后恢复的寄存器，B 可以对其任意修改而不用恢复；被调用者保存寄存器（Callee Saved Register）是 B 在被调用之后需要第一时间压到栈上保存的寄存器，并在退出前恢复。

2. 编译之后，通过 System.map 查看 vmlinux.lds 中自定义符号的值。

   > ![image-20231028191109027](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab1/assets/image-20231028191109027.png)

3. 用 `csr_read` 宏读取 `sstatus` 寄存器的值，对照 RISC-V 手册解释其含义（截图）。

4. 用 `csr_write` 宏向 `sscratch` 寄存器写入数据，并验证是否写入成功（截图）。

5. Detail your steps about how to get `arch/arm64/kernel/sys.i`

   > 在`/linux-6.6-rc2`目录下执行：
   >
   > ```bash
   > sudo apt install g++-aarch64-linux-gnu binutils-aarch64-linux-gnu
   > make ARCH=arm64 defconfig
   > make arch/arm64/kernel/sys.i ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
   > ```
   >
   > ![image-20231028194041744](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab1/assets/image-20231028194041744.png)

6. Find system call table of Linux v6.0 for `ARM32`, `RISC-V(32 bit)`, `RISC-V(64 bit)`, `x86(32 bit)`, `x86_64`
   List source code file, the whole system call table with macro expanded, screenshot every step.

   > + `ARM32`：`arch/arm/kernel/entry-common.S`
   >
   >   ```assembly
   >   	syscall_table_start sys_call_table
   >   #ifdef CONFIG_AEABI
   >   #include <calls-eabi.S>
   >   #else
   >   #include <calls-oabi.S>
   >   #endif
   >   	syscall_table_end sys_call_table
   >   ```
   >
   >    `arch/arm/include/generated/calls-eabi.S`
   >
   > <img src="/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab1/assets/f9388cce8650ab964a592b770b2126a2.png" alt="f9388cce8650ab964a592b770b2126a2" style="zoom:50%;" />
   >
   > ​	由于 AS 没有预处理选项，所以无法进行宏展开。
   >
   > + `RISC-V(32 bit)`
   >
   >   ```bash
   >   make ARCH=riscv 32-bit.config
   >   make arch/riscv/kernel/syscall_table.i ARCH=riscv CROSS_COMPILE
   >   =riscv64-linux-gnu-
   >   ```
   >
   >   `arch/riscv/kernel/syscall_table.c`
   >
   >   <img src="/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab1/assets/8d488dff53c5eb79575b188df20f0008.png" alt="8d488dff53c5eb79575b188df20f0008" style="zoom: 67%;" />
   >
   >   `arch/riscv/kernel/syscall_table.i`
   >
   >   <img src="/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab1/assets/9f67ab9bc361c1d43f46515a85cd6c37.png" alt="9f67ab9bc361c1d43f46515a85cd6c37"  />
   >
   > + `RISC-V(64 bit)`
   >
   >   ```bash
   >   make ARCH=riscv 64-bit.config
   >   make arch/riscv/kernel/syscall_table.i ARCH=riscv CROSS_COMPILE
   >   =riscv64-linux-gnu-
   >   ```
   >
   >   `arch/riscv/kernel/syscall_table.i`
   >
   >   <img src="/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab1/assets/0126309b41e98c5ce3d0593f64b9eda7.png" alt="0126309b41e98c5ce3d0593f64b9eda7"  />
   >
   > + `x86 (32 bit)`
   >
   >   ```bash
   >   make ARCH=x86 i386_defconfig
   >   make arch/x86/um/sys_call_table_32.i CROSS_COMPILE= ARCH=x86
   >   ```
   >
   >   `arch/x86/um/sys_call_table_32.c`
   >
   >   <img src="/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab1/assets/6b01067cafd456749d3ec601e04d1b0f.png" alt="6b01067cafd456749d3ec601e04d1b0f" style="zoom:67%;" />
   >
   >   `arch/x86/um/sys_call_table_32.i`
   >
   >   <img src="/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab1/assets/c6d8011ab9f268b0a97b597452419371.png" alt="c6d8011ab9f268b0a97b597452419371" style="zoom:67%;" />
   >
   > + `x86_64`
   >
   >   ```bash
   >   make ARCH=x86 x86_64_defconfig
   >   make arch/x86/um/sys_call_table_64.i CROSS_COMPILE= ARCH=x86
   >   ```
   >
   >   `arch/x86/um/sys_call_table_64.c`
   >
   >   <img src="/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab1/assets/6b01067cafd456749d3ec601e04d1b0f.png" alt="6b01067cafd456749d3ec601e04d1b0f" style="zoom:67%;" />
   >
   >   `arch/x86/um/sys_call_table_64.i`
   >
   >   <img src="/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab1/assets/04aa79a3970129ade2b38643fa11c687.png" alt="04aa79a3970129ade2b38643fa11c687" style="zoom: 67%;" />

7. Explain what is ELF file? Try readelf and objdump command on an ELF file, give screenshot of the output.
   Run an ELF file and cat `/proc/PID/maps` to give its memory layout.

   > ELF 包含将序加载到内存中所必要的程序内存布局的数据结构（如程序头表、符号表、节头表）和各个段的具体数据。
   >
   > 如下为读取 ELF 头的截图：
   >
   > <img src="/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab1/assets/5c9f723d0937159c078b7ca28fe84381.png" alt="5c9f723d0937159c078b7ca28fe84381" style="zoom:50%;" />
   >
   > 如下为读取 ELF 符号表的截图：
   >
   > <img src="/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab1/assets/22c28eeb27e283da45c629a4f18de5a7_720.png" alt="22c28eeb27e283da45c629a4f18de5a7_720" style="zoom: 50%;" />
   >
   > 如下为读取 ELF 程序头表（各段的信息）的截图：
   >
   > <img src="/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab1/assets/08c620e591f5b586fe854cc656f77f28.png" alt="08c620e591f5b586fe854cc656f77f28" style="zoom: 33%;" />
   >
   > 如下为读取 ELF 各节的截图：
   >
   > <img src="/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab1/assets/350334f720786995684fb7a5892583ee.png" alt="350334f720786995684fb7a5892583ee" style="zoom: 50%;" />
   >
   > 对 `zzx.o` 进行反汇编得到的汇编代码：
   >
   > <img src="/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab1/assets/b191d3936a84f176ddae2e378f4923fc.png" alt="b191d3936a84f176ddae2e378f4923fc" style="zoom: 33%;" />
   >
   > 对 `zzx.o` 的各节进行解析得到的二进制 dump：
   >
   > <img src="/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab1/assets/f8026e8c42e18fdbbbfaa46f6a742b8c.png" alt="f8026e8c42e18fdbbbfaa46f6a742b8c" style="zoom:33%;" />
   >
   > 执行 ELF 文件，并输出其内存布局：
   >
   > <img src="/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab1/assets/7470455c051e760edde0ad7fc2a5801e_720.png" alt="7470455c051e760edde0ad7fc2a5801e_720" style="zoom: 50%;" />

8. 通过查看 `RISC-V Privileged Spec` 中的 `medeleg` 和 `mideleg` ，解释上面 `MIDELEG` 值的含义。

   > ![image-20231028210348086](/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab1/assets/image-20231028210348086.png)
   >
   > <img src="/Users/pac/Documents/2023-f/OS/OS2023-Autumn/Lab1/assets/7ff198ba25f5ded695166ba3f70645b0.png" alt="7ff198ba25f5ded695166ba3f70645b0" style="zoom:50%;" />
   >
   > `MIDELEG` 值 0222：
   >
   > + `MIDELEG[1] = 1`  表示把 Supervisor software interrupt 委托给 S-mode
   > + `MIDELEG[5] = 1`  表示把 Supervisor timer interrupt 委托给 S-mode
   > + `MIDELEG[9] = 1`  表示把 Supervisor external interrupt 委托给 S-mode

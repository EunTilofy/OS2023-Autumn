#include "printk.h"
#include "sbi.h"

extern void test();

extern char _stext[];
extern char _srodata[];

int start_kernel() {
    printk("2022");
    printk(" Hello RISC-V\n");

    // asm volatile("jal _srodata");

    // printk("_stext = %x\n", *_stext);
    // printk("_srodata = %x\n", *_srodata);
    // *_stext = 0;
    // *_srodata = 0;
    // printk("_stext = %x\n", *_stext);
    // printk("_srodata = %x\n", *_srodata);

    test(); // DO NOT DELETE !!!

	return 0;
}

#include <defs.h>
#include <printk.h>

struct sstatus {
    uint64 wpri_0 : 1;
    uint64 sie : 1;
    uint64 wpri_1 : 3;
    uint64 spie : 1;
    uint64 ube : 1;
    uint64 wpri_2 : 1;
    uint64 spp : 1;
    uint64 vs : 2;
    uint64 wpri_3 : 2;
    uint64 fs : 2;
    uint64 xs_l : 1;
    uint64 xs_h : 1;
    uint64 wpri_4 : 1;
    uint64 sum : 1;
    uint64 mxr : 1;
    uint64 wpri_5 : 4;
    uint64 wpri_6 : 8;
    uint64 uxl : 2;
    uint64 wpri_7 : 6;
    uint64 wpri_8 : 8;
    uint64 wpri_9 : 8;
    uint64 wpri_10 : 7;
    uint64 sd : 1;
}__attribute__((packed));

void test() {
    // uint64 sstatus_v = csr_read(sstatus);
    // struct sstatus *sstatus_0 = &sstatus_v;
    // printk("sstatus: %llx \n", sstatus_v);
    // printk("sie %lld\n", sstatus_0->sie);
    // printk("spie %lld\n", sstatus_0->spie);
    // printk("ube %lld\n", sstatus_0->ube);
    // printk("spp %lld\n", sstatus_0->spp);
    // printk("vs %lld\n", sstatus_0->vs);
    // printk("fs %lld\n", sstatus_0->fs);
    // printk("xs %lld\n", sstatus_0->xs_l | (sstatus_0->xs_h << 1));
    // printk("sum %lld\n", sstatus_0->sum);
    // printk("mxr %lld\n", sstatus_0->mxr);
    // printk("uxl %lld\n", sstatus_0->uxl);
    // printk("sd %lld\n", sstatus_0->sd);
    // csr_write(sscratch, 0x57);
    // printk("sscratch: 0x%x\n", csr_read(sscratch));
    for(int i = 1; i <= 120000000; ++i) {
        if(i == 120000000) {
            printk("kernel is running!\n");
            i = 0;
        }
    }
}

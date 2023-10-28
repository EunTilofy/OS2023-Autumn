#include "printk.h"

// Please do not modify

void test() {
    for(int i = 1; i <= 120000000; ++i) {
        if(i == 120000000) {
            printk("%s", "kernel is running!\n");
            i = 0;
        }
    }
}

#ifndef _VM_H
#define _VM_H

#include "mm.h"
#include "string.h"
#include "printk.h"
#include "defs.h"

void setup_vm(void);
void setup_vm_final(void);
void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm);

#endif
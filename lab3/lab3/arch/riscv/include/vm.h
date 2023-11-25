#ifndef _VM_H
#define _VM_H

#include "mm.h"
#include "string.h"
#include "printk.h"
#include "defs.h"

void setup_vm();

void setup_vm_final();

void create_mapping(uint64*, uint64, uint64, uint64, int);

#endif
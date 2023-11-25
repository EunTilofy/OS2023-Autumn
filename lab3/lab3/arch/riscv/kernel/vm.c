#include "vm.h"

/* early_pgtbl: 用于 setup_vm 进行 1GB 的 映射。 */
unsigned long early_pgtbl[512] __attribute__((__aligned__(0x1000)));


void setup_vm(void) {
    /* 
    1. 由于是进行 1GB 的映射 这里不需要使用多级页表 
    2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
        high bit 可以忽略
        中间9 bit 作为 early_pgtbl 的 index
        低 30 bit 作为 页内偏移 这里注意到 30 = 9 + 9 + 12， 即我们只使用根页表， 根页表的每个 entry 都对应 1GB 的区域。 
    3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    */
    memset(early_pgtbl, 0x0, PGSIZE);
    uint64 PA = PHY_START;
    uint64 VA = VM_START;
    early_pgtbl[VPN2(PA)] = ((PA >> 12) << 10) | 0xf;
    early_pgtbl[VPN2(VA)] = ((PA >> 12) << 10) | 0xf;
    printk("...setup_vm done!\n");
}

/* swapper_pg_dir: kernel pagetable 根目录， 在 setup_vm_final 进行映射。 */
unsigned long  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

extern char _stext[];
extern char _srodata[];
extern char _sdata[];

void setup_vm_final(void) {
    memset(swapper_pg_dir, 0x0, PGSIZE);

    // No OpenSBI mapping required

    // mapping kernel text X|-|R|V
    create_mapping(swapper_pg_dir, (uint64)_stext, (uint64)_stext - PA2VA_OFFSET, (_srodata - _stext), 0xb);

    // mapping kernel rodata -|-|R|V
    create_mapping(swapper_pg_dir, (uint64)_srodata, (uint64)_srodata - PA2VA_OFFSET, (_sdata - _srodata), 0x3);

    // mapping other memory -|W|R|V
    create_mapping(swapper_pg_dir, (uint64)_sdata, (uint64)_sdata - PA2VA_OFFSET, PHY_SIZE - (_sdata - _stext), 0x7);

    // set satp with swapper_pg_dir
    uint64 _satp = (((uint64)swapper_pg_dir - PA2VA_OFFSET) >> 12) | (0x8000000000000000);
    csr_write(satp, _satp);
    
    // flush TLB
    asm volatile("sfence.vma zero, zero");

    // flush icache
    asm volatile("fence.i");
    return;
}


/**** 创建多级页表映射关系 ****/
void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm) {
    /*
    pgtbl 为根页表的基地址
    va, pa 为需要映射的虚拟地址、物理地址
    sz 为映射的大小
    perm 为映射的读写权限

    创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
    可以使用 V bit 来判断页表项是否存在
    */
    uint64 *now_pgtbl, *nex_pgtbl;
    for(int i = 0, num = sz / PGSIZE; i < num; ++i, va += PGSIZE, pa += PGSIZE) {
        uint64 vpn2 = VPN2(va), vpn1 = VPN1(va), vpn0 = VPN0(va);
        now_pgtbl = pgtbl;
        // first level
        if(now_pgtbl[vpn2] & 0x1) {
            now_pgtbl = (uint64 *)((((uint64)now_pgtbl[vpn2] >> 10) << 12) + PA2VA_OFFSET);
        } else {
            nex_pgtbl = (uint64 *)kalloc();
            now_pgtbl[vpn2] = ((((uint64)nex_pgtbl - PA2VA_OFFSET) >> 12) << 10) | 0x1;
            now_pgtbl = nex_pgtbl;
        }
        // second level
        if(now_pgtbl[vpn1] & 0x1) {
            now_pgtbl = (uint64 *)((((uint64)now_pgtbl[vpn1] >> 10) << 12) + PA2VA_OFFSET);
        } else {
            nex_pgtbl = (uint64 *)kalloc();
            now_pgtbl[vpn1] = ((((uint64)nex_pgtbl - PA2VA_OFFSET) >> 12) << 10) | 0x1;
            now_pgtbl = nex_pgtbl;
        }
        // third level
        now_pgtbl[vpn0] = ((pa >> 12) << 10) | perm | 0x1;
    }
}

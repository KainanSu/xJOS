#include <inc/memlayout.h>
#include <inc/mmu.h>

l1e_t entry_pgdir[L1NENTRIES] __attribute__((aligned(16 * 1024))) = {
    // map for init: make sure imx pin init macro lib can match the real address.
    [PHY_GICBASE >> 20] = PHY_GICBASE | L1_TYPE_SECT | L1_SECT_NSDEVICE | L1_SECT_AP_UNO_SRW,
    [PHY_AIPS1BASE >> 20] = PHY_AIPS1BASE | L1_TYPE_SECT | L1_SECT_NSDEVICE | L1_SECT_AP_UNO_SRW,
    [PHY_AIPS2BASE >> 20] = PHY_AIPS2BASE | L1_TYPE_SECT | L1_SECT_NSDEVICE | L1_SECT_AP_UNO_SRW,

    // map for cprintf
    [AIPS1BASE >> 20] = PHY_AIPS1BASE | L1_TYPE_SECT | L1_SECT_NSDEVICE | L1_SECT_AP_UNO_SRW,

    // map for start: va [DDRBASE, DDRBASE + 16M] to pa [DDRBASE, DDRBASE + 16M]
    [(DDRBASE >> 20) + 0] = (DDRBASE + 0x100000 * 0) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(DDRBASE >> 20) + 1] = (DDRBASE + 0x100000 * 1) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(DDRBASE >> 20) + 2] = (DDRBASE + 0x100000 * 2) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(DDRBASE >> 20) + 3] = (DDRBASE + 0x100000 * 3) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(DDRBASE >> 20) + 4] = (DDRBASE + 0x100000 * 4) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(DDRBASE >> 20) + 5] = (DDRBASE + 0x100000 * 5) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(DDRBASE >> 20) + 6] = (DDRBASE + 0x100000 * 6) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(DDRBASE >> 20) + 7] = (DDRBASE + 0x100000 * 7) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(DDRBASE >> 20) + 8] = (DDRBASE + 0x100000 * 8) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(DDRBASE >> 20) + 9] = (DDRBASE + 0x100000 * 9) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(DDRBASE >> 20) + 10] = (DDRBASE + 0x100000 * 10) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(DDRBASE >> 20) + 11] = (DDRBASE + 0x100000 * 11) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(DDRBASE >> 20) + 12] = (DDRBASE + 0x100000 * 12) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(DDRBASE >> 20) + 13] = (DDRBASE + 0x100000 * 13) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(DDRBASE >> 20) + 14] = (DDRBASE + 0x100000 * 14) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(DDRBASE >> 20) + 15] = (DDRBASE + 0x100000 * 15) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,

    // map for enable mmu: va [KERNBASE, KERNBASE + 16M]([3840M, 3856M]) to pa [DDRBASE, DDRBASE + 16M]
    [(KERNBASE >> 20) + 0] = (DDRBASE + 0x100000 * 0) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(KERNBASE >> 20) + 1] = (DDRBASE + 0x100000 * 1) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(KERNBASE >> 20) + 2] = (DDRBASE + 0x100000 * 2) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(KERNBASE >> 20) + 3] = (DDRBASE + 0x100000 * 3) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(KERNBASE >> 20) + 4] = (DDRBASE + 0x100000 * 4) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(KERNBASE >> 20) + 5] = (DDRBASE + 0x100000 * 5) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(KERNBASE >> 20) + 6] = (DDRBASE + 0x100000 * 6) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(KERNBASE >> 20) + 7] = (DDRBASE + 0x100000 * 7) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(KERNBASE >> 20) + 8] = (DDRBASE + 0x100000 * 8) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(KERNBASE >> 20) + 9] = (DDRBASE + 0x100000 * 9) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(KERNBASE >> 20) + 10] = (DDRBASE + 0x100000 * 10) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(KERNBASE >> 20) + 11] = (DDRBASE + 0x100000 * 11) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(KERNBASE >> 20) + 12] = (DDRBASE + 0x100000 * 12) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(KERNBASE >> 20) + 13] = (DDRBASE + 0x100000 * 13) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(KERNBASE >> 20) + 14] = (DDRBASE + 0x100000 * 14) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
    [(KERNBASE >> 20) + 15] = (DDRBASE + 0x100000 * 15) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW,
};

#ifndef JOS_INC_MMU_H
#define JOS_INC_MMU_H

/*
 *
 *	Part 1.  Paging data structures and constants.
 *
 */

// A linear address 'la' has a three-part structure as follows:
//
// +--------12------+-------8--------+---------12----------+
// |        L1      |        L2      | Offset within Page  |
// |      Index     |      Index     |                     |
// +----------------+----------------+---------------------+
//  \--- L2X(la) --/ \--- L1X(la) --/ \---- PGOFF(la) ----/
//  \---------- PGNUM(la) ----------/
//
// The PDX, PTX, PGOFF, and PGNUM macros decompose linear addresses as shown.
// To construct a linear address la from PDX(la), PTX(la), and PGOFF(la),
// use L2VADDR(PDX(la), PTX(la), PGOFF(la)).

// page number field of address
#define PGSHIFT     12
#define PGNUM(pa)	(((uintptr_t) (pa) - DDRBASE) >> PGSHIFT)

// page directory index
#define L1X(la)		(((uintptr_t) (la)) >> L1SHIFT)

// page table index
#define L2X(la)		((((uintptr_t) (la)) >> L2SHIFT) & 0xFF)

// offset in page
#define PGOFF(la)	(((uintptr_t) (la)) & 0xFFF)

// Page directory and page table constants.
#define L1NENTRIES	4096		// page directory entries per page directory
#define L2NENTRIES	256		    // page table entries per page table = 1M

#define PGSIZE		4096		// bytes mapped by a page
#define PGSHIFT		12		    // log2(PGSIZE)

#define SECSIZE     1024 * 1024

#define PTSIZE		(PGSIZE*L2NENTRIES) // bytes mapped by a page directory entry
#define PTSHIFT		20		// log2(PTSIZE)

#define NORMALSIZE  (4 * PTSIZE) // define a normal size for virtual address map, should align to PGSIZE

#define L2SHIFT	12		// offset of PTX in a linear address
#define L1SHIFT	20		// offset of PDX in a linear address

// construct linear address from indexes and offset
#define LADDR(l1, l2, o)	((void*) ((l1) << L1SHIFT | (l2) << L2SHIFT | (o)))

#define L1TABLE(l1)	    ((physaddr_t) (l1) & ~0x3FF)    // pointer to l2
#define L1PADDR(l1)	    ((physaddr_t) (l1) & ~0xFFFFF)  // PADDR in l1 entry (section)
#define L2PADDR(l2)     ((physaddr_t) (l2) & ~0xFFF)    // PADDR in l2 entry

#define L1_TYPE_MASK		(3 << 0)
#define L1_TYPE_FAULT		(0 << 0)
#define L1_TYPE_TABLE		(1 << 0)
#define L1_TYPE_SECT		(2 << 0)
#define L1_DOMAIN(x)		((x) << 5)
#define L1_TABLE_P		    (1 << 9) // protect
#define L1_SECT_XN		    (1 << 4)
#define L1_SECT_AP_AP0	    (1 << 10)
#define L1_SECT_AP_AP1	    (1 << 11)
#define L1_SECT_TEX(x)		((x) << 12)
#define L1_SECT_APX		    (1 << 15)
#define L1_SECT_AF		    (0)
#define L1_SECT_CACHEABLE	(1 << 3)
#define L2_TYPE_MASK		(3 << 0)
#define L2_TYPE_FAULT		(0 << 0)
#define L2_TYPE_LARGE		(1 << 0)
#define L2_TYPE_SMALL		(2 << 0)
#define L2_SMALL_XN		    (1 << 0)
#define L2_SMALL_AP0		(1 << 4)
#define L2_SMALL_AP1		(2 << 4)
#define L2_SMALL_TEX(x)		((x) << 6)
#define L2_SMALL_APX		(1 << 9)
#define L2_SMALL_SHARED		(1 << 10)
#define L2_SMALL_NG		    (1 << 11)
#define L2_SMALL_BUFFERABLE	(1 << 2)
#define L2_SMALL_CACHEABLE	(1 << 3)

#define L1_EXIST		    (L1_TYPE_MASK) // exist
#define L1_SECT_AP_UNO_SRW  (L1_SECT_AP_AP0)
#define L1_SECT_AP_URW_SRW	(L1_SECT_AP_AP1 | L1_SECT_AP_AP0)
#define L1_SECT_WT          (L1_SECT_CACHEABLE)
#define L1_SECT_NSDEVICE    (L1_SECT_TEX(0b010))
#define L1_TABLE_COW        (L1_TABLE_P)   // use P bit as cow flag
#define L2_EXIST		    (L2_TYPE_MASK)
#define L2_SMALL_AP_UNO_SRO	(L2_SMALL_APX | L2_SMALL_AP0)
#define L2_SMALL_AP_UNO_SRW	(L2_SMALL_AP0)
#define L2_SMALL_AP_URO_SRW	(L2_SMALL_AP1)
#define L2_SMALL_AP_URW_SRW	(L2_SMALL_AP1 | L2_SMALL_AP0)
#define L2_SMALL_NSDEVICE	(L2_SMALL_TEX(0b010))
#define L2_SMALL_WT		    (L2_SMALL_CACHEABLE) // TODO: 改为device mem
#define L2_SMALL_WB		    (L2_SMALL_CACHEABLE | L2_SMALL_BUFFERABLE)
#define L2_SMALL_FOR_USER   (L2_SMALL_APX | L2_SMALL_AP0 | L2_SMALL_AP1 | L2_EXIST | L2_SMALL_SHARED) // for user call check

#define DOMAIN_NONE 0x0
#define DOMAIN_CLIENT 0x1
#define DOMAIN_MANAGER 0x3

#endif

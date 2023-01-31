#ifndef JOS_INC_MEMLAYOUT_H
#define JOS_INC_MEMLAYOUT_H

#ifndef __ASSEMBLER__
#include <inc/types.h>
#include <inc/mmu.h>
#endif /* not __ASSEMBLER__ */

/*
 * This file contains definitions for memory management in our OS,
 * which are relevant to both the kernel and user-mode software.
 */

/*
 * Virtual memory map:                                Permissions
 *                                                    kernel/user
 *
 *    4 Gig -------->  +------------------------------+
 *                     |                              | RW/--
 *                     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *                     :              .               :
 *                     :              .               :				 ........ L1 page, RW/--, XN
 *                     :              .               :
 *                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~| RW/--
 *                     |                              | RW/--
 *                     |   Remapped Physical Memory   | RW/--		 ........ < __bss_end__: XN
 *                     |                              | RW/--
 *    KERNBASE, ---->  +------------------------------+ 0xf0000000      --+
 *    KSTACKTOP        |     CPU0's Kernel Stack      | RW/--  KSTKSIZE   |
 *                     | - - - - - - - - - - - - - - -|                   |
 *                     |      Invalid Memory (*)      | --/--  KSTKGAP    |
 *                     +------------------------------+                   |
 *                     |     CPU1's Kernel Stack      | RW/--  KSTKSIZE   |
 *                     | - - - - - - - - - - - - - - -|                 NORMALSIZE = 4M
 *                     |      Invalid Memory (*)      | --/--  KSTKGAP    |
 *                     +------------------------------+                   |
 *                     :              .               :                   |
 *                     :              .               :                   |
 *    MMIOLIM ------>  +------------------------------+ 0xefc00000      --+
 *                     |       Memory-mapped I/O      | RW/--  NORMALSIZE ........ L2 page, WT, RW/--
 * ULIM, MMIOBASE -->  +------------------------------+ 0xef800000
 *                     |  Cur. Page Table (User R-)   | R-/R-  NORMALSIZE
 *    UVPT      ---->  +------------------------------+ show
 *                     |          RO PAGES            | R-/R-  NORMALSIZE
 *    UPAGES    ---->  +------------------------------+ 0xef000000
 *                     |           RO ENVS            | R-/R-  NORMALSIZE
 * UTOP,UENVS ------>  +------------------------------+ 0xeec00000
 * UXSTACKTOP -/       |     User Exception Stack     | RW/RW  PGSIZE
 *                     +------------------------------+ 0xeebff000
 *                     |       Empty Memory (*)       | --/--  PGSIZE
 *    USTACKTOP  --->  +------------------------------+ 0xeebfe000->0xee7fe000
 *                     |      Normal User Stack       | RW/RW  PGSIZE
 *                     +------------------------------+ 0xeebfd000
 *                     |                              |
 *                     |                              |
 *                     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *                     .                              .
 *                     .                              .
 *                     .                              .
 *                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
 *                     |     Program Data & Heap      |
 *    UTEXT -------->  +------------------------------+ 0x00800000
 *    PFTEMP ------->  |       Empty Memory (*)       |        PTSIZE
 *                     |                              |
 *    UTEMP -------->  +------------------------------+ 0x00400000      --+
 *                     |       Empty Memory (*)       |                   |
 *                     | - - - - - - - - - - - - - - -|                   |
 *                     |  User STAB Data (optional)   |                 PTSIZE
 *    USTABDATA ---->  +------------------------------+ 0x00200000   2M   |
 *                     |       Empty Memory (*)       |                   |
 *    0 ------------>  +------------------------------+                 --+
 *
 * (*) Note: The kernel ensures that "Invalid Memory" is *never* mapped.
 *     "Empty Memory" is normally unmapped, but user programs may map pages
 *     there if desired.  JOS user programs map pages temporarily at UTEMP.
 */

#define	DDRBASE	0x80000000 				// 芯片DDR的0地址
#define DDRSIZE (256 * 1024 * 1024) 	// 256MB
//					    (0x00100000)  	// is 1M
#define PHY_GICBASE     (0x00A00000)   	// at 10M
#define GICSIZE			(32 * 1024)
#define PHY_AIPS1BASE    (0x02000000) 	// in kern_pgdir, map to MMIO_BASE. contain uart1, epit1, ccm
#define AIPS1SIZE 		(1 * 1024 * 1024)
#define PHY_AIPS2BASE    (0x02100000)
#define AIPS2SIZE 		(1 * 1024 * 1024)
// TODO: simplify
#define PHY_EPIT1_BASE  (0x020D0000)
#define PHY_CCM_BASE    (0x020C4000)
#define UART1_OFFSET 	(0x00020000) 	// to APIPS_BASE
#define EPIT1_OFFSET	(PHY_EPIT1_BASE - PHY_AIPS1BASE)

// All physical memory mapped at this address
#define	KERNBASE	0xF0000000

// Kernel stack.
#define KSTACKTOP	KERNBASE
#define KSTKSIZE	(8*PGSIZE)   		// size of a kernel stack
#define KSTKGAP		(8*PGSIZE)   		// size of a kernel stack guard

// irq stack.
#define IRQSTACKTOP (KSTACKTOP - NORMALSIZE)
#define IRQSTKSIZE	(8*PGSIZE)   		// size of a kernel stack
#define IRQSTKGAP	(8*PGSIZE)   		// size of a kernel stack guard

// Memory-mapped IO.
#define MMIOLIM		(IRQSTACKTOP - NORMALSIZE)
#define MMIOBASE	(MMIOLIM - NORMALSIZE)
#define GICBASE		(AIPS2BASE + AIPS2SIZE)			// 32K
#define AIPS2BASE	(AIPS1BASE + AIPS1SIZE)			// 1M
#define AIPS1BASE	(MMIOBASE) 						// 1M

#define ULIM		(AIPS1BASE)

/*
 * User read-only mappings! Anything below here til UTOP are readonly to user.
 * They are global pages mapped in at env allocation time.
 */

// User read-only virtual page table (see 'uvpt' below)
#define UVPT		(ULIM - NORMALSIZE)
// Read-only copies of the Page structures
#define UPAGES		(UVPT - NORMALSIZE)
// Read-only copies of the global env structures
#define UENVS		(UPAGES - NORMALSIZE)

/*
 * Top of user VM. User can manipulate VA from UTOP-1 and down!
 */

// Top of user-accessible VM
#define UTOP		UENVS
// Top of one-page user exception stack
#define UXSTACKTOP	UTOP
// Next page left invalid to guard against exception stack overflow; then:
// Top of normal user stack
#define USTACKTOP	(UTOP - 2*PGSIZE)

// Where user programs generally begin
#define UTEXT		(2*NORMALSIZE)

#define FS_SD_BASE (UTEMP + NORMALSIZE / 2)

// Used for temporary page mappings.  Typed 'void*' for convenience
#define UTEMP		((void*) NORMALSIZE)
// Used for temporary page mappings for the user page-fault handler
// (should not conflict with other temporary page mappings)
#define PFTEMP		(UTEMP + NORMALSIZE - PGSIZE)
// The location of the user-level STABS data structure
#define USTABDATA	(NORMALSIZE / 2)

#define UBASE ISPTOP

// region for isp
#define ISPTOP 1 * 1024 * 1024

#ifndef __ASSEMBLER__

typedef uint32_t l2e_t;
typedef uint32_t l1e_t;

#if JOS_USER
/*
 * The page directory entry corresponding to the virtual address range
 * [UVPT, UVPT + PTSIZE) points to the page directory itself.  Thus, the page
 * directory is treated as a page table as well as a page directory.
 *
 * One result of treating the page directory as a page table is that all PTEs
 * can be accessed through a "virtual page table" at virtual address UVPT (to
 * which uvpt is set in lib/entry.S).  The PTE for page number N is stored in
 * uvpt[N].  (It's worth drawing a diagram of this!)
 *
 * A second consequence is that the contents of the current page directory
 * will always be available at virtual address (UVPT + (UVPT >> PGSHIFT)), to
 * which uvpd is set in lib/entry.S.
 */
extern volatile l2e_t uvpt[];     // VA of "virtual page table"
extern volatile l1e_t uvpd[];     // VA of current page directory
#endif

/*
 * Page descriptor structures, mapped at UPAGES.
 * Read/write to the kernel, read-only to user programs.
 *
 * Each struct PageInfo stores metadata for one physical page.
 * Is it NOT the physical page itself, but there is a one-to-one
 * correspondence between physical pages and struct PageInfo's.
 * You can map a struct PageInfo * to the corresponding physical address
 * with page2pa() in kern/pmap.h.
 */
struct PageInfo {
	// Next page on the free list.
	struct PageInfo *pp_link;

	// pp_ref is the count of pointers (usually in page table entries)
	// to this page, for pages allocated using page_alloc.
	// Pages allocated at boot time using pmap.c's
	// boot_alloc do not have valid reference count fields.

	uint16_t pp_ref;
};

#endif /* !__ASSEMBLER__ */
#endif /* !JOS_INC_MEMLAYOUT_H */

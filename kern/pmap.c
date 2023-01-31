#include <inc/types.h>
#include <inc/arm.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/memlayout.h>
#include <inc/log.h>

#include <kern/pmap.h>

#define NPAGES (DDRSIZE / PGSIZE)
struct PageInfo pages[NPAGES];
size_t npages = NPAGES;
struct PageInfo *page_free_list = NULL;
l1e_t kern_pgdir[L1NENTRIES] __attribute__((aligned(16 * 1024)));

static void page_init();
void boot_map_region(l1e_t *pgdir, uintptr_t va, size_t size, physaddr_t pa, int perm);
static void check_page_free_list(void);
static void check_page_alloc(void);
static void check_kern_pgdir(void);
static physaddr_t check_va2pa(l1e_t *pgdir, uintptr_t va);
static void check_page(void);
static void check_page_installed_pgdir(void);

struct PageInfo *page_alloc(int alloc_flags);
void page_free(struct PageInfo *pp);
int	page_insert(l1e_t *pgdir, struct PageInfo *pp, void *va, int perm);
void page_remove(l1e_t *pgdir, void *va);
struct PageInfo *page_lookup(l1e_t *pgdir, void *va, l2e_t **l2_store);
void page_decref(struct PageInfo *pp);

void mem_init()
{
    page_init();

	// map physical memory
    for (uintptr_t addr = KERNBASE; addr != 0; addr += SECSIZE) {
        kern_pgdir[L1X(addr)] = (PADDR((void*)addr)) 
                | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW;
    }
    kern_pgdir[0] = (DDRBASE) | L1_TYPE_SECT | L1_SECT_AP_UNO_SRW;

	// map kernel stack
	extern char kernstack[];
	boot_map_region(kern_pgdir, (uintptr_t)KSTACKTOP - KSTKSIZE, 
		KSTKSIZE, PADDR(kernstack), L2_SMALL_AP_UNO_SRW);

    // map gpio memory-map
	boot_map_region(kern_pgdir, (uintptr_t)AIPS1BASE, 
		AIPS1SIZE, PHY_AIPS1BASE, L2_SMALL_AP_UNO_SRW | L2_SMALL_NSDEVICE);
    boot_map_region(kern_pgdir, (uintptr_t)AIPS2BASE, 
		AIPS2SIZE, PHY_AIPS2BASE, L2_SMALL_AP_UNO_SRW | L2_SMALL_NSDEVICE);
	boot_map_region(kern_pgdir, (uintptr_t)GICBASE, 
		GICSIZE,   PHY_GICBASE,   L2_SMALL_AP_URO_SRW | L2_SMALL_NSDEVICE);


	boot_map_region(kern_pgdir, (uintptr_t)UPAGES, 
		ROUNDUP((sizeof(struct PageInfo)*npages), PGSIZE), PADDR(pages), L2_SMALL_AP_URO_SRW);

	boot_map_region(kern_pgdir, (uintptr_t)UENVS, 
			ROUNDUP(NENV * sizeof(struct Env), PGSIZE), PADDR(envs), L2_SMALL_AP_URO_SRW);

    load_pgdir(PADDR(kern_pgdir));

    log_trace("memory init success.\n");
}

static void page_init(){
    // [DDRBASE, DDRBASE + (__bss_end__ - KERNBASE)]: used
    // else to page_free_list 
    extern char __bss_end__[];
    struct PageInfo *pi;
    physaddr_t KERNUSED = ROUNDUP(DDRBASE + ((physaddr_t)__bss_end__ - KERNBASE), PGSIZE);

    assert((DDRBASE % PGSIZE) == 0);

    for(physaddr_t addr = DDRBASE; addr < KERNUSED; addr += PGSIZE){
        pi = pa2page(addr);
        pi->pp_ref = 1;
    }

	// scan from high to low
    for(physaddr_t addr = DDRBASE + DDRSIZE - PGSIZE; addr >= KERNUSED; addr -= PGSIZE){
        pi = pa2page(addr);
        pi->pp_ref = 0;
        pi->pp_link = page_free_list;
        page_free_list = pi;
    }

    log_info("total mem: %dK, kern used: %dK, free mem: %dK\n", DDRSIZE / 1024, (KERNUSED - DDRBASE) / 1024, (DDRBASE + DDRSIZE - KERNUSED) / 1024);
}

//
// Allocates a physical page.  If (alloc_flags & ALLOC_ZERO), fills the entire
// returned physical page with '\0' bytes.  Does NOT increment the reference
// count of the page - the caller must do these if necessary (either explicitly
// or via page_insert).
//
// Be sure to set the pp_link field of the allocated page to NULL so
// page_free can check for double-free bugs.
//
// Returns NULL if out of free memory.
//
// Hint: use page2kva and memset
struct PageInfo *
page_alloc(int alloc_flags)
{
	// Fill this function in
	if(page_free_list == NULL)
		return NULL;
	struct PageInfo *result;

	result = page_free_list;
	page_free_list = page_free_list->pp_link;
	result->pp_link = NULL;
	if(alloc_flags & ALLOC_ZERO){
		memset(page2kva(result), 0, PGSIZE);
	}

	return result;
}

//
// Return a page to the free list.
// (This function should only be called when pp->pp_ref reaches 0.)
//
void
page_free(struct PageInfo *pp)
{
	// Fill this function in
	// Hint: You may want to panic if pp->pp_ref is nonzero or
	// pp->pp_link is not NULL.
	if(pp->pp_ref || pp->pp_link)
		panic("page_free: pp error");
	pp->pp_link = page_free_list;
	page_free_list = pp;
}

//
// Decrement the reference count on a page,
// freeing it if there are no more refs.
//
void
page_decref(struct PageInfo* pp)
{
	if (--pp->pp_ref == 0)
		page_free(pp);
}

int update_l1entry(l1e_t *pgdir, void *va, int perm)
{
    // should exist and is table
    assert((pgdir[L1X(va)] & L1_TYPE_TABLE) == L1_TYPE_TABLE);
    assert(L1TABLE(pgdir[L1X(va)]) < KERNBASE);
	pgdir[L1X(va)] = L1TABLE(pgdir[L1X(va)]) | L1_TYPE_TABLE | perm;
	tlb_invalidate(va);

	return 0;
}

// Given 'pgdir', a pointer to a page directory, pgdir_walk returns
// a pointer to the page table entry (PTE) for linear address 'va'.
// This requires walking the two-level page table structure.
//
// The relevant page table page might not exist yet.
// If this is true, and create == false, then pgdir_walk returns NULL.
// Otherwise, pgdir_walk allocates a new page table page with page_alloc.
//    - If the allocation fails, pgdir_walk returns NULL.
//    - Otherwise, the new page's reference count is incremented,
//	the page is cleared,
//	and pgdir_walk returns a pointer into the new page table page.
//
// Hint 1: you can turn a PageInfo * into the physical address of the
// page it refers to with page2pa() from kern/pmap.h.
//
// Hint 2: the x86 MMU checks permission bits in both the page directory
// and the page table, so it's safe to leave permissions in the page
// directory more permissive than strictly necessary.
//
// Hint 3: look at inc/mmu.h for useful macros that manipulate page
// table and page directory entries.
//
l2e_t *
pgdir_walk(l1e_t *pgdir, const void *va, int create){
    l1e_t *pde, *pt, *pte;
	uintptr_t ret_va;
    assert(L1X(va) < L1NENTRIES);
    if (!(pgdir[L1X(va)] & L1_EXIST)) {
        if (!create) 
            return NULL;
        struct PageInfo * new_page = page_alloc(ALLOC_ZERO);
        if(new_page == NULL) 
            return NULL;
        new_page->pp_ref++;
        pgdir[L1X(va)] = page2pa(new_page) | L1_TYPE_TABLE;
    }

    assert((pgdir[L1X(va)] & L1_TYPE_TABLE) == L1_TYPE_TABLE);
    l2e_t *pgtbl = (l2e_t*)KADDR(L1TABLE(pgdir[L1X(va)]));
    return &pgtbl[L2X(va)];
}

//
// Map [va, va+size) of virtual address space to physical [pa, pa+size)
// in the page table rooted at pgdir.  Size is a multiple of PGSIZE, and
// va and pa are both page-aligned.
// Use permission bits perm|L2_P for the entries.
//
// This function is only intended to set up the ``static'' mappings
// above UTOP. As such, it should *not* change the pp_ref field on the
// mapped pages.
//
// Hint: the TA solution uses pgdir_walk
void boot_map_region(l1e_t *pgdir, uintptr_t va, size_t size, physaddr_t pa, int perm)
{
    assert(va % PGSIZE == 0);
    assert(pa % PGSIZE == 0);

    // cprintf("[%p, %p] to [%p, %p]\n", va, va + size, pa, pa + size);
    
    for (int i = 0; i < size; i += PGSIZE) {
	    l2e_t *pte = pgdir_walk(pgdir, (void*)(va + i), 1);
        if (pte == NULL) 
            panic("boot_map_region out of memory\n");
        else
            *pte = (pa + i) | L2_TYPE_SMALL | perm;
    }
}

//
// Map the physical page 'pp' at virtual address 'va'.
// The permissions (the low 12 bits) of the page table entry
// should be set to 'perm|L2_P'.
//
// Requirements
//   - If there is already a page mapped at 'va', it should be page_remove()d.
//   - If necessary, on demand, a page table should be allocated and inserted
//     into 'pgdir'.
//   - pp->pp_ref should be incremented if the insertion succeeds.
//   - The TLB must be invalidated if a page was formerly present at 'va'.
//
// Corner-case hint: Make sure to consider what happens when the same
// pp is re-inserted at the same virtual address in the same pgdir.
// However, try not to distinguish this case in your code, as this
// frequently leads to subtle bugs; there's an elegant way to handle
// everything in one code path.
//
// RETURNS:
//   0 on success
//   -E_NO_MEM, if page table couldn't be allocated
//
// Hint: The TA solution is implemented using pgdir_walk, page_remove,
// and page2pa.
//
int
page_insert(l1e_t *pgdir, struct PageInfo *pp, void *va, int perm)
{
	// Fill this function in
	l2e_t *pte;
	pte = pgdir_walk(pgdir, va, 1);
	if(pte == NULL)
		return -E_NO_MEM;
	pp->pp_ref++;
	if(*pte & L2_EXIST)
		page_remove(pgdir, va);
	*pte = page2pa(pp) | L2_TYPE_SMALL | perm;
	tlb_invalidate(va);

	return 0;
}

//
// Return the page mapped at virtual address 'va'.
// If l2_store is not zero, then we store in it the address
// of the pte for this page.  This is used by page_remove and
// can be used to verify page permissions for syscall arguments,
// but should not be used by most callers.
//
// Return NULL if there is no page mapped at va.
//
// Hint: the TA solution uses pgdir_walk and pa2page.
//
struct PageInfo *
page_lookup(l1e_t *pgdir, void *va, l2e_t **l2_store)
{
	// Fill this function in
	l2e_t * pte = pgdir_walk(pgdir, va, 0);
	if(pte == NULL)
		return NULL;
	if(l2_store != NULL)
		*l2_store = pte;
	if(!(*pte & L2_EXIST))
		return NULL;
	return pa2page(L2PADDR(*pte));
}

//
// Unmaps the physical page at virtual address 'va'.
// If there is no physical page at that address, silently does nothing.
//
// Details:
//   - The ref count on the physical page should decrement.
//   - The physical page should be freed if the refcount reaches 0.
//   - The pg table entry corresponding to 'va' should be set to 0.
//     (if such a PTE exists)
//   - The TLB must be invalidated if you remove an entry from
//     the page table.
//
// Hint: The TA solution is implemented using page_lookup,
// 	tlb_invalidate, and page_decref.
//
void
page_remove(l1e_t *pgdir, void *va)
{
	// Fill this function in
	struct PageInfo *pi;
	l2e_t *pte;
	pi = page_lookup(pgdir, va, &pte);
	if(pi == NULL)
		return;
	*pte = 0;
	tlb_invalidate(va);
	page_decref(pi);
}

static uintptr_t user_mem_check_addr;

// perm: l1e_t or l2e_t or perm only
bool perm_have(int perm, int _have){
    return (perm & _have) == _have;
}

bool perm_have_user(int perm){
    return perm_have(perm, L2_SMALL_AP_URO_SRW) || perm_have(perm, L2_SMALL_AP_URW_SRW);
}

int is_user_mem(const void *va){
    return ((uintptr_t)va > UBASE && (uintptr_t)va < UTOP);
}

int user_mem_check(struct Env *env, const void *va, size_t len, int perm)
{
	uintptr_t start = ROUNDDOWN((uintptr_t)va, PGSIZE);
	uintptr_t end = ROUNDUP((uintptr_t)va + len, PGSIZE);

	for(uintptr_t addr = start; addr < end; addr += PGSIZE){
        if(env->env_pgdir[L1X(addr)] & L1_TABLE_COW)
			return -E_FAULT;
		l2e_t *pte = pgdir_walk(env->env_pgdir, (void *)addr, 0);

		if(addr < ISPTOP || addr > ULIM || pte == NULL || !((uint32_t)*pte & L2_EXIST) || ((uint32_t)*pte & perm) != perm){
			if(addr == ROUNDDOWN((uintptr_t)va, PGSIZE)) {
				user_mem_check_addr = (uintptr_t)va;
			}else {
				user_mem_check_addr = (uintptr_t)addr;
			}
			return -E_FAULT;
		}
	}

	return 0;
}

void user_mem_assert(struct Env *env, const void *va, size_t len, int perm)
{
	if (user_mem_check(env, va, len, perm) < 0) {
		cprintf("[%08x] user_mem_check assertion failure for "
			"va %08x\n", env->env_id, user_mem_check_addr);
		env_destroy(env);	// may not return
	}
}

static void check_page_free_list(void)
{
	int count = 0;

	if (!page_free_list)
		panic("'page_free_list' is a null pointer!");

	for (struct PageInfo* pg = page_free_list; pg != NULL; pg = pg->pp_link) {
		assert(pg->pp_ref == 0);
		count++;
	}
	assert(count > 0);
	cprintf("check_page_free_list() succeeded!\n");
}

static void check_page_alloc(void)
{
    struct PageInfo *pp, *pp0, *pp1, *pp2;
    int nfree;
    struct PageInfo *fl;
    char *c;
    int i;

    // check number of free pages
    for (pp = page_free_list, nfree = 0; pp; pp = pp->pp_link)
		++nfree;

    // should be able to allocate three pages
    pp0 = pp1 = pp2 = 0;
    assert((pp0 = page_alloc(0)));
    assert((pp1 = page_alloc(0)));
    assert((pp2 = page_alloc(0)));

    assert(pp0);
    assert(pp1 && pp1 != pp0);
    assert(pp2 && pp2 != pp1 && pp2 != pp0);
    assert(page2pa(pp0) < npages*PGSIZE + DDRBASE);
    assert(page2pa(pp1) < npages*PGSIZE + DDRBASE);
    assert(page2pa(pp2) < npages*PGSIZE + DDRBASE);

    // temporarily steal the rest of the free pages
    fl = page_free_list;
    page_free_list = 0;

    // should be no free memory
    assert(!page_alloc(0));

    // free and re-allocate?
    page_free(pp0);
    page_free(pp1);
    page_free(pp2);
    pp0 = pp1 = pp2 = 0;
    assert((pp0 = page_alloc(0)));
    assert((pp1 = page_alloc(0)));
    assert((pp2 = page_alloc(0)));
    assert(pp0);
    assert(pp1 && pp1 != pp0);
    assert(pp2 && pp2 != pp1 && pp2 != pp0);
    assert(!page_alloc(0));

    // test flags
    memset(page2kva(pp0), 1, PGSIZE);

    page_free(pp0);
    assert((pp = page_alloc(ALLOC_ZERO)));
    assert(pp && pp0 == pp);
    c = page2kva(pp);
    for (i = 0; i < PGSIZE; i++)
		assert(c[i] == 0);

    // give free list back
    page_free_list = fl;

    // free the pages we took
    page_free(pp0);
    page_free(pp1);
    page_free(pp2);

    // number of free pages should be the same
    for (pp = page_free_list; pp; pp = pp->pp_link)
		--nfree;
    assert(nfree == 0);

    cprintf("check_page_alloc() succeeded!\n");
}

// check page_insert, page_remove, &c
static void check_page(void)
{
	struct PageInfo *pp, *pp0, *pp1, *pp2;
	struct PageInfo *fl;
	l2e_t *ptep, *ptep1;
	void *va;
	int i;
	extern l1e_t entry_pgdir[];

    // should be able to allocate three pages
    pp0 = pp1 = pp2 = 0;
    assert((pp0 = page_alloc(0)));
    assert((pp1 = page_alloc(0)));
    assert((pp2 = page_alloc(0)));

    assert(pp0);
    assert(pp1 && pp1 != pp0);
    assert(pp2 && pp2 != pp1 && pp2 != pp0);

    // temporarily steal the rest of the free pages
    fl = page_free_list;
    page_free_list = 0;

    // should be no free memory
    assert(!page_alloc(0));

    // there is no page allocated at address 0
    assert(page_lookup(kern_pgdir, (void *) 0x0, &ptep) == NULL);

    // there is no free memory, so we can't allocate a page table
    assert(page_insert(kern_pgdir, pp1, 0x0, L2_SMALL_AP_UNO_SRW) < 0);

    // free pp0 and try again: pp0 should be used for page table
    page_free(pp0);
    assert(page_insert(kern_pgdir, pp1, 0x0, L2_SMALL_AP_UNO_SRW) == 0);
    assert(L2PADDR(kern_pgdir[0]) == page2pa(pp0));

    assert(check_va2pa(kern_pgdir, 0x0) == page2pa(pp1));
    assert(pp1->pp_ref == 1);
    assert(pp0->pp_ref == 1);

    // should be able to map pp2 at PGSIZE because pp0 is already allocated for page table
    assert(page_insert(kern_pgdir, pp2, (void*) PGSIZE, L2_SMALL_AP_UNO_SRW) == 0);
    assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp2));
    assert(pp2->pp_ref == 1);

    // should be no free memory
    assert(!page_alloc(0));

    // should be able to map pp2 at PGSIZE because it's already there
    assert(page_insert(kern_pgdir, pp2, (void*) PGSIZE, L2_SMALL_AP_UNO_SRW) == 0);
    assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp2));
    assert(pp2->pp_ref == 1);

    // pp2 should NOT be on the free list
    // could happen in ref counts are handled sloppily in page_insert
    assert(!page_alloc(0));

    // check that pgdir_walk returns a pointer to the pte
    ptep = (l2e_t *) KADDR(L2PADDR(kern_pgdir[L1X(PGSIZE)]));
    assert(pgdir_walk(kern_pgdir, (void*)PGSIZE, 0) == ptep+L2X(PGSIZE));

    // should be able to change permissions too.
    assert(page_insert(kern_pgdir, pp2, (void*) PGSIZE, L2_SMALL_AP_URW_SRW) == 0);
    assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp2));
    assert(pp2->pp_ref == 1);
    assert(*pgdir_walk(kern_pgdir, (void*) PGSIZE, 0) & L2_SMALL_AP_URW_SRW);
    //assert(kern_pgdir[0] & L2_U);

    // should be able to remap with fewer permissions
    assert(page_insert(kern_pgdir, pp2, (void*) PGSIZE, L2_SMALL_AP_UNO_SRW) == 0);
    assert(*pgdir_walk(kern_pgdir, (void*) PGSIZE, 0) & L2_SMALL_AP_UNO_SRW);
    assert((*pgdir_walk(kern_pgdir, (void*) PGSIZE, 0) & L2_SMALL_AP_URW_SRW) != L2_SMALL_AP_URW_SRW);

    // should not be able to map at NORMALSIZE because need free page for page table
    assert(page_insert(kern_pgdir, pp0, (void*) NORMALSIZE, L2_SMALL_AP_UNO_SRW) < 0);

    // insert pp1 at PGSIZE (replacing pp2)
    assert(page_insert(kern_pgdir, pp1, (void*) PGSIZE, L2_SMALL_AP_UNO_SRW) == 0);
    assert((*pgdir_walk(kern_pgdir, (void*) PGSIZE, 0) & L2_SMALL_AP_URW_SRW) != L2_SMALL_AP_URW_SRW);

    // should have pp1 at both 0 and PGSIZE, pp2 nowhere, ...
    assert(check_va2pa(kern_pgdir, 0) == page2pa(pp1));
    assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp1));
    // ... and ref counts should reflect this
    assert(pp1->pp_ref == 2);
    assert(pp2->pp_ref == 0);

    // pp2 should be returned by page_alloc
    assert((pp = page_alloc(0)) && pp == pp2);

    // unmapping pp1 at 0 should keep pp1 at PGSIZE
    page_remove(kern_pgdir, 0x0);
    assert(check_va2pa(kern_pgdir, 0x0) == ~0);
    assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp1));
    assert(pp1->pp_ref == 1);
    assert(pp2->pp_ref == 0);

    // test re-inserting pp1 at PGSIZE
    assert(page_insert(kern_pgdir, pp1, (void*) PGSIZE, 0) == 0);
    assert(pp1->pp_ref);
    assert(pp1->pp_link == NULL);

    // unmapping pp1 at PGSIZE should free it
    page_remove(kern_pgdir, (void*) PGSIZE);
    assert(check_va2pa(kern_pgdir, 0x0) == ~0);
    assert(check_va2pa(kern_pgdir, PGSIZE) == ~0);
    assert(pp1->pp_ref == 0);
    assert(pp2->pp_ref == 0);

    // so it should be returned by page_alloc
    assert((pp = page_alloc(0)) && pp == pp1);

    // should be no free memory
    assert(!page_alloc(0));

    // forcibly take pp0 back
    assert(L2PADDR(kern_pgdir[0]) == page2pa(pp0));
    kern_pgdir[0] = 0;
    assert(pp0->pp_ref == 1);
    pp0->pp_ref = 0;

    // check pointer arithmetic in pgdir_walk
    page_free(pp0);
    va = (void*)(PGSIZE * L1NENTRIES + PGSIZE);
    ptep = pgdir_walk(kern_pgdir, va, 1);
    ptep1 = (l2e_t *) KADDR(L2PADDR(kern_pgdir[L1X(va)]));
    assert(ptep == ptep1 + L2X(va));
    kern_pgdir[L1X(va)] = 0;
    pp0->pp_ref = 0;

    // check that new page tables get cleared
    memset(page2kva(pp0), 0xFF, PGSIZE);
    page_free(pp0);
    pgdir_walk(kern_pgdir, 0x0, 1);
    ptep = (l2e_t *) page2kva(pp0);
    for(i=0; i<L2NENTRIES; i++)
	    assert((ptep[i] & L1_EXIST) == 0);
    kern_pgdir[0] = 0;
    pp0->pp_ref = 0;

    // give free list back
    page_free_list = fl;

    // free the pages we took
    page_free(pp0);
    page_free(pp1);
    page_free(pp2);

    cprintf("check_page() succeeded!\n");
}


//
// Checks that the kernel part of virtual address space
// has been setup roughly correctly (by mem_init()).
static void check_kern_pgdir(void)
{
    uint32_t i, n;
    l1e_t *pgdir;
	extern char kernstack[];

    pgdir = kern_pgdir;

    // check phys mem
    for (i = 0; i < npages * PGSIZE; i += PGSIZE)
		assert(check_va2pa(pgdir, KERNBASE + i) == i + DDRBASE);

    // check pages array
    n = ROUNDUP(npages*sizeof(struct PageInfo), PGSIZE);
    for (i = 0; i < n; i += PGSIZE)
    	assert(check_va2pa(pgdir, UPAGES + i) == PADDR(pages) + i);

    // check kernel stack
    for (i = 0; i < KSTKSIZE; i += PGSIZE)
    	assert(check_va2pa(pgdir, KSTACKTOP - KSTKSIZE + i) == PADDR(kernstack) + i);
    assert(check_va2pa(pgdir, KSTACKTOP - NORMALSIZE) == ~0);

    // check PDE permissions
	// for (i = 0; i < L1NENTRIES; i++) {
	// 	switch (i) {
	// 	case L1X(KSTACKTOP - KSTKSIZE):
    //     	assert(pgdir[i] & L1_EXIST);
	// 		break;
	// 	case L1X(UPAGES):
    //     	assert(pgdir[i] & L1_EXIST);
	// 		break;
	// 	case L1X(AIPS1BASE):
	// 		assert(pgdir[i] & L1_EXIST);
	// 		break;
	// 	default:
	// 		if (i >= L1X(KERNBASE)) {
	// 			assert(pgdir[i] & L1_EXIST);
	// 			assert(pgdir[i] & L1_SECT_AP_UNO_SRW);
	// 		} else
	// 			assert(pgdir[i] == 0);
	// 		break;
	// 	}
	// }
	cprintf("check_kern_pgdir() succeeded!\n");
}

// This function returns the physical address of the page containing 'va',
// defined by the page directory 'pgdir'.  The hardware normally performs
// this functionality for us!  We define our own version to help check
// the check_kern_pgdir() function; it shouldn't be used elsewhere.
static physaddr_t check_va2pa(l1e_t *pgdir, uintptr_t va)
{
    l2e_t *p;

    pgdir = &pgdir[L1X(va)];
    if (!(*pgdir & L1_EXIST))
		return ~0;

    if ((*pgdir & L1_TYPE_SECT) == L1_TYPE_SECT) {
		return (physaddr_t) (((*pgdir) & 0xFFF00000) + (va & 0xFFFFF));
    } else if((*pgdir & L1_TYPE_TABLE) == L1_TYPE_TABLE) {
		p = (l2e_t*) KADDR(L1TABLE(*pgdir));
		if (!(p[L2X(va)] & L2_EXIST))
			return ~0;
		l2e_t pte = p[L2X(va)];
		if ((pte & L2_TYPE_SMALL) == L2_TYPE_SMALL) {
			return L2PADDR(p[L2X(va)]) + (va & 0xFFF);
		} else {
            panic("unreachable area.\n");
            return ~0;
			// return L2_LARGE_ADDR(p[L2X(va)]) + (va & 0xFFFF);
		}
    }
    panic("unreachable area.\n");
    return ~0;
}

// check page_insert, page_remove, &c, with an installed kern_pgdir
static void check_page_installed_pgdir(void)
{
    struct PageInfo *pp, *pp0, *pp1, *pp2;
    struct PageInfo *fl;
    l2e_t *ptep, *ptep1;
    uintptr_t va;
    int i;

    // check that we can read and write installed pages
    pp1 = pp2 = 0;
    assert((pp0 = page_alloc(0)));
    assert((pp1 = page_alloc(0)));
    assert((pp2 = page_alloc(0)));

    page_free(pp0);
    memset(page2kva(pp1), 1, PGSIZE);
    memset(page2kva(pp2), 2, PGSIZE);

    page_insert(kern_pgdir, pp1, (void*) PGSIZE, L2_SMALL_AP_UNO_SRW);
    assert(pp1->pp_ref == 1);

    assert(*(uint32_t *)PGSIZE == 0x01010101U);
    page_insert(kern_pgdir, pp2, (void*) PGSIZE, L2_SMALL_AP_UNO_SRW);


    assert(*(uint32_t *)PGSIZE == 0x02020202U);
    assert(pp2->pp_ref == 1);
    assert(pp1->pp_ref == 0);
    *(uint32_t *)PGSIZE = 0x03030303U;
    assert(*(uint32_t *)page2kva(pp2) == 0x03030303U);
    page_remove(kern_pgdir, (void*) PGSIZE);
    assert(pp2->pp_ref == 0);

    // forcibly take pp0 back
    assert(L2PADDR(kern_pgdir[0]) == page2pa(pp0));
    kern_pgdir[0] = 0;
    assert(pp0->pp_ref == 1);
    pp0->pp_ref = 0;

    // free the pages we took
    page_free(pp0);

    cprintf("check_page_installed_pgdir() succeeded!\n");
}

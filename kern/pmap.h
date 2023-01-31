#ifndef JOS_KERN_PMAP_H
#define JOS_KERN_PMAP_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/memlayout.h>
#include <inc/assert.h>
#include <kern/env.h>

enum {
	// For page_alloc, zero the returned physical page.
	ALLOC_ZERO = 1<<0,
};

extern struct PageInfo pages[];
extern size_t npages;
extern l1e_t kern_pgdir[];

#define PADDR(kva) _paddr(__FILE__, __LINE__, kva)

static inline physaddr_t
_paddr(const char *file, int line, void *kva)
{
	if ((uint32_t)kva < KERNBASE)
		_panic(file, line, "PADDR called with invalid kva %08lx", kva);
	return (physaddr_t)kva - KERNBASE + DDRBASE;
}

#define KADDR(pa) _kaddr(__FILE__, __LINE__, pa)

static inline void*
_kaddr(const char *file, int line, physaddr_t pa)
{
	if (PGNUM(pa) >= npages)
		_panic(file, line, "KADDR called with invalid pa %08lx", pa);
	return (void *)(pa - DDRBASE + KERNBASE);
}

static inline physaddr_t
page2pa(struct PageInfo *pp)
{
	return ((pp - pages) << PGSHIFT) + DDRBASE;
}

static inline struct PageInfo*
pa2page(physaddr_t pa)
{
	if (PGNUM(pa) >= npages)
		panic("pa2page called with invalid pa: %p", pa);
	return &pages[PGNUM(pa)];
}

static inline void*
page2kva(struct PageInfo *pp)
{
	return KADDR(page2pa(pp));
}

void mem_init();
struct PageInfo *page_alloc(int alloc_flags);
void page_free(struct PageInfo *pp);
int	page_insert(l1e_t *pgdir, struct PageInfo *pp, void *va, int perm);
void page_remove(l1e_t *pgdir, void *va);
struct PageInfo *page_lookup(l1e_t *pgdir, void *va, l2e_t **l2_store);
void page_decref(struct PageInfo *pp);

l2e_t *pgdir_walk(l1e_t *pgdir, const void *va, int create);
int update_l1entry(l1e_t *l1e, void *va, int perm);

int user_mem_check(struct Env *env, const void *va, size_t len, int perm);
void user_mem_assert(struct Env *env, const void *va, size_t len, int perm);
int is_user_mem(const void *va);
bool perm_have(int perm, int _have);
bool perm_have_user(int perm);
void boot_map_region(l1e_t *pgdir, uintptr_t va, size_t size, physaddr_t pa, int perm);

#endif /* !JOS_KERN_PMAP_H */

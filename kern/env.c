/* See COPYRIGHT for copyright information. */

#include <inc/arm.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/elf.h>
#include <inc/log.h>

#include <kern/env.h>
#include <kern/pmap.h>
// #include <kern/trap.h>
#include <kern/monitor.h>
#include <kern/sched.h>
// #include <kern/cpu.h>
// #include <kern/spinlock.h>

// struct Env *envs = NULL;		// All environments
struct Env envs[NENV];		// All environments
struct Env *curenv = NULL;		// The current env
static struct Env *env_free_list;	// Free environment list
					// (linked by Env->env_link)

#define ENVGENSHIFT	12		// >= LOGNENV

//
// Converts an envid to an env pointer.
// If checkperm is set, the specified environment must be either the
// current environment or an immediate child of the current environment.
//
// RETURNS
//   0 on success, -E_BAD_ENV on error.
//   On success, sets *env_store to the environment.
//   On error, sets *env_store to NULL.
//
int
envid2env(envid_t envid, struct Env **env_store, bool checkperm)
{
	struct Env *e;

	// If envid is zero, return the current environment.
	if (envid == 0) {
		*env_store = curenv;
		return 0;
	}

	// Look up the Env structure via the index part of the envid,
	// then check the env_id field in that struct Env
	// to ensure that the envid is not stale
	// (i.e., does not refer to a _previous_ environment
	// that used the same slot in the envs[] array).
	e = &envs[ENVX(envid)];
	assert(ENVX(envid) < NENV);
	assert((e->env_status != ENV_FREE) && "hint: is fork out of envs, and you send to this env?");
	assert(e->env_id == envid);

	if (ENVX(envid) >= NENV || e->env_status == ENV_FREE || e->env_id != envid) {
		*env_store = 0;
		return -E_BAD_ENV;
	}

	// Check that the calling environment has legitimate permission
	// to manipulate the specified environment.
	// If checkperm is set, the specified environment
	// must be either the current environment
	// or an immediate child of the current environment.
	if (checkperm && e != curenv && e->env_parent_id != curenv->env_id) {
		*env_store = 0;
		return -E_BAD_ENV;
	}

	*env_store = e;
	return 0;
}

// Mark all environments in 'envs' as free, set their env_ids to 0,
// and insert them into the env_free_list.
// Make sure the environments are in the free list in the same order
// they are in the envs array (i.e., so that the first call to
// env_alloc() returns envs[0]).
//
void
env_init(void)
{
	// Set up envs array
	// LAB 3: Your code here.
	struct Env *env;
	for(env = envs + NENV - 1; env >= envs; env--){
		env->env_status = ENV_FREE;
		env->env_id = 0;
		env->env_link = env_free_list;
		env_free_list = env;
	}

	log_info("envs init success.\n");
}

//
// Initialize the kernel virtual memory layout for environment e.
// Allocate a page directory, set e->env_pgdir accordingly,
// and initialize the kernel portion of the new environment's address space.
// Do NOT (yet) map anything into the user portion
// of the environment's virtual address space.
//
// Returns 0 on success, < 0 on error.  Errors include:
//	-E_NO_MEM if page directory or table could not be allocated.
//
static int
env_setup_vm(struct Env *e)
{
	int i;
	struct PageInfo *p = NULL;

	// // Allocate a page for the page directory
	// if (!(p = page_alloc(ALLOC_ZERO)))
	// 	return -E_NO_MEM;

	// Now, set e->env_pgdir and initialize the page directory.
	//
	// Hint:
	//    - The VA space of all envs is identical above UTOP
	//	(except at UVPT, which we've set below).
	//	See inc/memlayout.h for permissions and layout.
	//	Can you use kern_pgdir as a template?  Hint: Yes.
	//	(Make sure you got the permissions right in Lab 2.)
	//    - The initial VA below UTOP is empty.
	//    - You do not need to make any more calls to page_alloc.
	//    - Note: In general, pp_ref is not maintained for
	//	physical pages mapped only above UTOP, but env_pgdir
	//	is an exception -- you need to increment env_pgdir's
	//	pp_ref for env_free to work correctly.
	//    - The functions in kern/pmap.h are handy.

	// LAB 3: Your code here.
	// copy from kern_pgdir to p, between [UTOP, UVPT], [ULIM, 4G]
	// e->env_pgdir = (pde_t *)page2kva(p);
	// p->pp_ref++;

	for(int i = L1X(UTOP); i < L1NENTRIES; i++)
		e->env_pgdir[i] = kern_pgdir[i];
	e->env_pgdir[0] = kern_pgdir[0]; // interrupt handlers

	// UVPT maps the env's own page table read-only.
	// Permissions: kernel R, user R
	// e->env_pgdir[PDX(UVPT)] = PADDR(e->env_pgdir) | PTE_P | PTE_U;

	return 0;
}

//
// Allocates and initializes a new environment.
// On success, the new environment is stored in *newenv_store.
//
// Returns 0 on success, < 0 on failure.  Errors include:
//	-E_NO_FREE_ENV if all NENV environments are allocated
//	-E_NO_MEM on memory exhaustion
//
int
env_alloc(struct Env **newenv_store, envid_t parent_id)
{
	int32_t generation;
	int r;
	struct Env *e;

	if (!(e = env_free_list))
		return -E_NO_FREE_ENV;

	// Allocate and set up the page directory for this environment.
	if ((r = env_setup_vm(e)) < 0)
		return r;

	// Generate an env_id for this environment.-> generation = 0,unique,idx
	generation = (e->env_id + (1 << ENVGENSHIFT)) & ~(NENV - 1);
	if (generation <= 0)	// Don't create a negative env_id.
		generation = 1 << ENVGENSHIFT;
	e->env_id = generation | (e - envs);

	// Set the basic status variables.
	e->env_parent_id = parent_id;
	e->env_type = ENV_TYPE_USER;
	e->env_status = ENV_RUNNABLE;
	e->env_runs = 0;

	// Clear out all the saved register state,
	// to prevent the register values
	// of a prior environment inhabiting this Env structure
	// from "leaking" into our new environment.
	memset(&e->env_tf, 0, sizeof(e->env_tf));

	// Set up appropriate initial values for the segment registers.
	// GD_UD is the user data segment selector in the GDT, and
	// GD_UT is the user text segment selector (see inc/memlayout.h).
	// The low 2 bits of each segment register contains the
	// Requestor Privilege Level (RPL); 3 means user mode.  When
	// we switch privilege levels, the hardware does various
	// checks involving the RPL and the Descriptor Privilege Level
	// (DPL) stored in the descriptors themselves.
	// e->env_tf.tf_ds = GD_UD | 3;
	// e->env_tf.tf_es = GD_UD | 3;
	// e->env_tf.tf_ss = GD_UD | 3;
	// e->env_tf.tf_esp = USTACKTOP;
	// e->env_tf.tf_cs = GD_UT | 3;
	// You will set e->env_tf.tf_eip later.
	e->env_tf.user_flags = 0x60000100 | CPSR_M_USR;
	e->env_tf.user_sp = USTACKTOP;
	e->env_tf.user_lr = 0;

	// Enable interrupts while in user mode.
	// LAB 4: Your code here.
	// e->env_tf.tf_eflags |= FL_IF;

	// Clear the page fault handler until user installs one.
	// e->env_pgfault_upcall = 0;

	// Also clear the IPC receiving flag.
	// e->env_ipc_recving = 0;

	// commit the allocation
	env_free_list = e->env_link;
	*newenv_store = e;

	log_info("[%08x] create new env %08x\n", curenv ? curenv->env_id : 0, e->env_id);
	return 0;
}

int env_page_map(envid_t srcenvid, void *srcva, envid_t dstenvid, void *dstva, int perm)
{
	int r;
	struct Env *srcenv, *dstenv;
	if((r = envid2env(srcenvid, &srcenv, 1)) < 0)
		return r;
	if((r = envid2env(dstenvid, &dstenv, 1)) < 0)
		return r;

	if(!is_user_mem(srcva) || (uintptr_t)srcva % PGSIZE != 0 
		|| !is_user_mem(dstva) || (uintptr_t)dstva % PGSIZE != 0)
		return -E_INVAL;

	l2e_t *srcpte, *dstpte;
	struct PageInfo *pi;
	// (no pde map) or !(pte & PTE) => NULL
	if((pi = page_lookup(srcenv->env_pgdir, srcva, &srcpte)) == NULL)
		return -E_INVAL;
	
	if(perm & ~L2_SMALL_FOR_USER)
		return -E_INVAL;
	// ???????????????????????????????????????????????????
	if(!(perm & L2_EXIST) || (!perm_have(perm, L2_SMALL_AP_URO_SRW) && !perm_have(perm, L2_SMALL_AP_URW_SRW)))
		return -E_INVAL;
	if(perm_have(perm, L2_SMALL_AP_URW_SRW) && !perm_have(*srcpte, L2_SMALL_AP_URW_SRW))
		return -E_INVAL;

	if((r = page_insert(dstenv->env_pgdir, pi, dstva, perm)) < 0)
		return r;

	return 0;
	panic("env_page_map not implemented");
}

int env_page_unmap(envid_t envid, void *va)
{
	int r;
	struct Env *e;
	if((r = envid2env(envid, &e, 1)) < 0)
		return r;
	if(!is_user_mem(va) || (uintptr_t)va % PGSIZE != 0)
		return -E_INVAL;
	page_remove(e->env_pgdir, va);

	return 0;
	panic("sys_page_unmap not implemented");
}

int env_page_alloc(envid_t envid, void *va, int perm)
{
	struct Env *e;
	int r;
	if((r = envid2env(envid, &e, 1)) < 0)
		return r;
	if(!is_user_mem(va) || (uintptr_t)va % PGSIZE != 0)
		return -E_INVAL;
	if(perm & ~L2_SMALL_FOR_USER)
		return -E_INVAL;
	if(!perm_have(perm, L2_TYPE_SMALL) | (!perm_have(perm, L2_SMALL_AP_URO_SRW) && !perm_have(perm, L2_SMALL_AP_URW_SRW)))
		return -E_INVAL;

	struct PageInfo *pi =  page_alloc(ALLOC_ZERO);
	if((pi = page_alloc(ALLOC_ZERO)) == NULL)
		return -E_NO_MEM;
	if((r = page_insert(e->env_pgdir, pi, va, perm)) < 0){
		page_free(pi);
		return r;
	}

	return 0;
	panic("sys_page_alloc not implemented");
}


//
// Allocate len bytes of physical memory for environment env,
// and map it at virtual address va in the environment's address space.
// Does not zero or otherwise initialize the mapped pages in any way.
// Pages should be writable by user and kernel.
// Panic if any allocation attempt fails.
//
static void
region_alloc(struct Env *e, void *va, size_t len)
{
	// LAB 3: Your code here.
	// (But only if you need it for load_icode.)
	//
	// Hint: It is easier to use region_alloc if the caller can pass
	//   'va' and 'len' values that are not page-aligned.
	//   You should round va down, and round (va + len) up.
	//   (Watch out for corner-cases!)
	uintptr_t start = ROUNDDOWN((uint32_t)va, PGSIZE);
	uintptr_t end = ROUNDUP((uint32_t)va + len, PGSIZE);
	for(uintptr_t addr = start; addr < end; addr += PGSIZE){
		struct PageInfo *pi = page_alloc(0);
		if(pi == NULL)
			panic("region_alloc, page_alloc: %e", E_NO_MEM);
		int ret = page_insert(e->env_pgdir, pi, (void*)addr, L2_SMALL_AP_URW_SRW);
		if(ret < 0)
			panic("region_alloc, page_insert: %e", E_NO_MEM);
	}
}

//
// Set up the initial program binary, stack, and processor flags
// for a user process.
// This function is ONLY called during kernel initialization,
// before running the first user-mode environment.
//
// This function loads all loadable segments from the ELF binary image
// into the environment's user memory, starting at the appropriate
// virtual addresses indicated in the ELF program header.
// At the same time it clears to zero any portions of these segments
// that are marked in the program header as being mapped
// but not actually present in the ELF file - i.e., the program's bss section.
//
// All this is very similar to what our boot loader does, except the boot
// loader also needs to read the code from disk.  Take a look at
// boot/main.c to get ideas.
//
// Finally, this function maps one page for the program's initial stack.
//
// load_icode panics if it encounters problems.
//  - How might load_icode fail?  What might be wrong with the given input?
// -> elf err, i.e. magic number
static void
load_icode(struct Env *e, uint8_t *binary)
{
	// Hints:
	//  Load each program segment into virtual memory
	//  at the address specified in the ELF segment header.
	//  You should only load segments with ph->p_type == ELF_PROG_LOAD.
	//  Each segment's virtual address can be found in ph->p_va
	//  and its size in memory can be found in ph->p_memsz.
	//  The ph->p_filesz bytes from the ELF binary, starting at
	//  'binary + ph->p_offset', should be copied to virtual address
	//  ph->p_va.  Any remaining memory bytes should be cleared to zero.
	//  (The ELF header should have ph->p_filesz <= ph->p_memsz.)
	//  Use functions from the previous lab to allocate and map pages.
	//
	//  All page protection bits should be user read/write for now.
	//  ELF segments are not necessarily page-aligned, but you can
	//  assume for this function that no two segments will touch
	//  the same virtual page.
	//
	//  You may find a function like region_alloc useful.
	//
	//  Loading the segments is much simpler if you can move data
	//  directly into the virtual addresses stored in the ELF binary.
	//  So which page directory should be in force during
	//  this function?
	//
	//  You must also do something with the program's entry point,
	//  to make sure that the environment starts executing there.
	//  What?  (See env_run() and env_pop_tf() below.)

	// LAB 3: Your code here.
	struct Elf *elfhdr = (struct Elf *)binary;
	struct Proghdr *ph, *eph;
    if (elfhdr->e_magic != ELF_MAGIC)
        panic("elf header's magic is not correct\n");

	// for copy from kernel to user space, use user's pgdir
	load_pgdir(PADDR(e->env_pgdir));

	ph = (struct Proghdr *) ((uint8_t *) elfhdr + elfhdr->e_phoff);
	eph = ph + elfhdr->e_phnum;

	for (; ph < eph; ph++){
		if(ph->p_type != ELF_PROG_LOAD)
			continue;
		if (ph->p_filesz > ph->p_memsz)
            panic("file size is great than memory size\n");
		region_alloc(e, (void *)ph->p_va, ph->p_memsz);
		log_trace("load_icode region_alloc\n");
		memcpy((void *)ph->p_va, binary + ph->p_offset, ph->p_filesz);
		memset((void *)(ph->p_va + ph->p_filesz), 0, (ph->p_memsz - ph->p_filesz));
	}

	e->env_tf.user_ip = elfhdr->e_entry;

	// Now map one page for the program's initial stack
	// at virtual address USTACKTOP - PGSIZE.

	// LAB 3: Your code here.
	region_alloc(e, (void *)USTACKTOP - PGSIZE, PGSIZE);

	load_pgdir(PADDR(kern_pgdir));
}

//
// Allocates a new env with env_alloc, loads the named elf
// binary into it with load_icode, and sets its env_type.
// This function is ONLY called during kernel initialization,
// before running the first user-mode environment.
// The new env's parent ID is set to 0.
//
void
env_create(uint8_t *binary, enum EnvType type)
{
	// LAB 3: Your code here.
	struct Env *env = NULL;
	int ret;
	ret = env_alloc(&env, 0);
	if(ret < 0)
		panic("env_create: %e\n", ret);
	env->env_type = type;

	if(type == ENV_TYPE_FS){
		boot_map_region(env->env_pgdir, (uintptr_t)FS_SD_BASE, 4 * PGSIZE,  PHY_AIPS2BASE + 0x90000, L2_SMALL_AP_URW_SRW);
	}

	load_icode(env, binary);
	// If this is the file server (type == ENV_TYPE_FS) give it I/O privileges.
	// LAB 5: Your code here.
	// if(type == ENV_TYPE_FS)
	// 	env->env_tf.tf_eflags |= FL_IOPL_3;
}

//
// Frees env e and all memory it uses.
//
static void env_free(struct Env *e)
{
	l2e_t *pt;
	uint32_t pdeno, pteno;
	physaddr_t pa;

	// If freeing the current environment, switch to kern_pgdir
	// before freeing the page directory, just in case the page
	// gets reused.
	if (e == curenv)
		load_pgdir(PADDR(kern_pgdir));

	// Note the environment's demise.
	// cprintf("[%08x] free env %08x\n", curenv ? curenv->env_id : 0, e->env_id);

	// Flush all mapped pages in the user portion of the address space
	static_assert(UTOP % PTSIZE == 0);
	for (pdeno = L1X(ISPTOP); pdeno < L1X(UTOP); pdeno++) {
		// only look at mapped page tables
		if (!(e->env_pgdir[pdeno] & L1_EXIST))
			continue;

		// find the pa and va of the page table
		pa = L1PADDR(e->env_pgdir[pdeno]);
		pt = (l2e_t*) KADDR(pa);
		// unmap all PTEs in this page table
		for (pteno = 0; pteno < L2NENTRIES; pteno++) {
			if (pt[pteno] & L2_EXIST)
				page_remove(e->env_pgdir, LADDR(pdeno, pteno, 0));
		}

		// free the page table itself
		e->env_pgdir[pdeno] = 0;
		page_decref(pa2page(pa));
	}

	// free the page directory
	// pa = PADDR(e->env_pgdir);
	// e->env_pgdir = 0;
	// page_decref(pa2page(pa));

	// return the environment to the free list
	e->env_status = ENV_FREE;
	e->env_link = env_free_list;
	env_free_list = e;
}

//
// Frees environment e.
// If e was the current env, then runs a new environment (and does not return
// to the caller).
//
void env_destroy(struct Env *e)
{
	// If e is currently running on other CPUs, we change its state to
	// ENV_DYING. A zombie environment will be freed the next time
	// it traps to the kernel.
	if (e->env_status == ENV_RUNNING && curenv != e) {
		e->env_status = ENV_DYING;
		return;
	}

	log_info("[%08x] destroy env %08x\n", curenv ? curenv->env_id : 0, e->env_id);
	env_free(e);

	if (curenv == e) {
		curenv = NULL;
		sched_yield();
	}
}


//
// Restores the register values in the Trapframe with the 'iret' instruction.
// This exits the kernel and starts executing some environment's code.
//
// This function does not return.
//
void
env_pop_tf(struct Trapframe *tf)
{
	// Record the CPU we are running on for user-space debugging
	// curenv->env_cpunum = cpunum();

	asm volatile(
		"\tmov sp, r0\n"
		"\tldmfd sp!, {r0-r2}\n"
		"\tmsr spsr, r0\n"
		"\tmsr sp_usr, r1\n"
		"\tmsr lr_usr, r2\n"
		"\tldmfd sp!, {r0-r12,lr}\n"
		"\tldr sp, =kernstacktop\n"
		"\tsubs pc, lr, #0\n"
		::: "memory");

	panic("env_pop_tf failed");  /* mostly to placate the compiler */
}

//
// Context switch from curenv to env e.
// Note: if this is the first call to env_run, curenv is NULL.
//
// This function does not return.
//
void
env_run(struct Env *e)
{
	// Step 1: If this is a context switch (a new environment is running):
	//	   1. Set the current environment (if any) back to
	//	      ENV_RUNNABLE if it is ENV_RUNNING (think about
	//	      what other states it can be in),
	//	   2. Set 'curenv' to the new environment,
	//	   3. Set its status to ENV_RUNNING,
	//	   4. Update its 'env_runs' counter,
	//	   5. Use lcr3() to switch to its address space.
	// Step 2: Use env_pop_tf() to restore the environment's
	//	   registers and drop into user mode in the
	//	   environment.

	// Hint: This function loads the new environment's state from
	//	e->env_tf.  Go back through the code you wrote above
	//	and make sure you have set the relevant parts of
	//	e->env_tf to sensible values.

	// LAB 3: Your code here.
	if(curenv){
		if(curenv->env_status == ENV_RUNNING)
			curenv->env_status = ENV_RUNNABLE;
		else if(curenv->env_status == ENV_NOT_RUNNABLE)
			;
		else
			panic("env_run: curenv unknown status: %d", curenv->env_status);
	}
	curenv = e;
	curenv->env_status = ENV_RUNNING;
	curenv->env_runs++;
	load_pgdir(PADDR(curenv->env_pgdir));

	env_pop_tf(&curenv->env_tf);

	panic("env_run not yet implemented");
}


#include <kern/syscall.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>
#include <kern/sched.h>
#include <inc/string.h>
#include <inc/error.h>
#include <inc/log.h>
#include <inc/arm.h>

extern void print_trapframe(struct Trapframe *tf);

int syscall(uint32_t num, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5);
static void sys_cputs(const char *s, size_t len);
static void sys_panic(int info);
static void sys_yield(void);
static int sys_fork(void);
static envid_t sys_getenvid(void);
static int sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm);
static int sys_ipc_recv(void *dstva);
static int sys_page_map(envid_t srcenvid, void *srcva, envid_t dstenvid, void *dstva, int perm);
static int sys_page_unmap(envid_t envid, void *va);
static int sys_page_alloc(envid_t envid, void *va, int perm);
static int sys_env_set_pgfault_upcall(envid_t envid, void *func);
static int sys_env_destroy(envid_t envid);
static int sys_get_l1(void *va);
static int sys_get_l2(void *va);
static envid_t sys_exofork(void);
static int sys_env_set_trapframe(envid_t envid, struct Trapframe *tf);
static int sys_env_set_status(envid_t envid, int status);
static int sys_cgetc(void);

int syscall(uint32_t num, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5){
	log_trace("comming syscall: %d\n", num);
	switch(num){
		case SYS_cputs:
			sys_cputs((char *)a1, a2);
			return 0;
		break;

		case SYS_panic:
			sys_panic(a1);
			return 0;
		break;

		case SYS_yield:
			sys_yield(); // no return
			panic("sys_yield return");
			return 0;
		break;

		case SYS_fork:
			return sys_fork();
		break;

		case SYS_getenvid:
			return sys_getenvid();
		break;

		case SYS_ipc_try_send:
			return sys_ipc_try_send(a1, a2, (void *)a3, a4);
		break;

		case SYS_ipc_recv:
			return sys_ipc_recv((void*)a1);
		break;

		case SYS_page_alloc:
			return sys_page_alloc(a1, (void *)a2, a3);

		case SYS_page_map:
			return sys_page_map(a1, (void *)a2, a3, (void *)a4, a5);

		case SYS_page_unmap:
			return sys_page_unmap(a1, (void *)a2);

		case SYS_env_set_pgfault_upcall:
			return sys_env_set_pgfault_upcall(a1, (void *)a2);
		break;

		case SYS_env_destroy:
			return sys_env_destroy(a1);
		break;

		case SYS_get_l1:
			return sys_get_l1((void *)a1);
		break;

		case SYS_get_l2:
			return sys_get_l2((void *)a1);
		break;

		case SYS_exofork:
			return sys_exofork();
		break;

		case SYS_env_set_trapframe:
			return sys_env_set_trapframe(a1, (struct Trapframe *)a2);
		break;

		case SYS_env_set_status:
			return sys_env_set_status(a1, a2);
		break;

		case SYS_cgetc:
			return sys_cgetc();
		break;

		default:		
		break;
	}

	log_err("unknown syscall: %d\n", num);
    
	return 0;
}

static void sys_cputs(const char *s, size_t len)
{
	// Check that the user has permission to read memory [s, s+len).
	// Destroy the environment if not.

	// LAB 3: Your code here.
	user_mem_assert(curenv, s, len, L2_SMALL_AP_URW_SRW);

	// Print the string supplied by the user.
	cprintf("%.*s", len, s);
}

static void sys_panic(int info)
{
	mon_backtrace(0, 0, &curenv->env_tf);
	env_destroy(curenv);
	// panic("user env panic: %d\n", info);
}

static void sys_yield(void)
{
	sched_yield();
}

// 1.copy parent trapframe
// 2.(implement copy on write)
// scan L2, for every entry, 
// if is read only, map directly to child;
// if is read & write, map to child and remap parent with read permission.
// 3.set L1 entry with COW if one or more write page in L2 table.
static int sys_fork(void){
	int r;
	struct Env *e;
	// cprintf("sys_fork start\n");

	if((r = env_alloc(&e, curenv->env_id)) < 0)
		return r;
	e->env_status = ENV_NOT_RUNNABLE;
	memcpy(&e->env_tf, &curenv->env_tf, sizeof(e->env_tf));
	e->env_tf.r[0] = 0;
	
	for(int i = L1X(UTEXT); i < L1X(UTOP); i++){
		if(curenv->env_pgdir[i] & L1_EXIST){

			assert(perm_have(curenv->env_pgdir[i], L1_TYPE_TABLE));
			int l2_have_write = 0;
			l2e_t* l2e = (l2e_t*)KADDR(L1TABLE(curenv->env_pgdir[i]));

			for(int j = 0; j < L2NENTRIES; j++){
				void* addr = LADDR(i, j, 0);
				if(l2e[j] & L2_EXIST){
					assert(perm_have(l2e[j], L2_TYPE_SMALL));
					if(l2e[j] & L2_SMALL_SHARED) {
						if ((r = sys_page_map(curenv->env_id, addr, e->env_id, addr, L2_TYPE_SMALL | L2_SMALL_AP_URW_SRW | L2_SMALL_SHARED)) < 0) 
							panic("duppage: page mapping failed %e", r);
					}else if(perm_have(l2e[j], L2_SMALL_AP_URW_SRW) || (curenv->env_pgdir[i] & L1_TABLE_COW)){
						if ((r = env_page_map(curenv->env_id, addr, e->env_id, addr, L2_SMALL_AP_URO_SRW | L2_TYPE_SMALL)) < 0)
							panic("sys_fork map(COW) to child: %e", r);
						if ((r = env_page_map(curenv->env_id, addr, curenv->env_id, addr, L2_SMALL_AP_URO_SRW | L2_TYPE_SMALL)) < 0)
							panic("sys_fork remap(COW)  parent: %e", r);
						l2_have_write = 1;
					}else{
						if ((r = env_page_map(curenv->env_id, addr, e->env_id, addr, L2_SMALL_AP_URO_SRW | L2_TYPE_SMALL)) < 0)
							panic("sys_fork map(RO) to child: %e", r);
					}
				}
			}
			if(l2_have_write){
				void* addr = LADDR(i, 0, 0);
				if ((r = update_l1entry(curenv->env_pgdir, addr, L1_TYPE_TABLE | L1_TABLE_COW)) < 0)
					panic("sys_fork update_l1entry %e", r);
				if ((r = update_l1entry(e->env_pgdir, addr, L1_TYPE_TABLE | L1_TABLE_COW)) < 0)
					panic("sys_fork update_l1entry %e", r);
			}
		}
	}

	e->env_status = ENV_RUNNABLE;

	// cprintf("sys_fork end\n");
	return e->env_id;
}

static envid_t sys_getenvid(void)
{
	return curenv->env_id;
}

static int sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
{
	int r;
	struct Env *e;
	if((r = envid2env(envid, &e, 0)) < 0)
		return -E_BAD_ENV;
	// 需要对方在等待ipc
	if(e->env_ipc_recving != true || e->env_ipc_from != 0)
		return -E_IPC_NOT_RECV;
	
	if(is_user_mem(srcva)){
		if((uint32_t)srcva % PGSIZE != 0)
			return -E_INVAL;
		if ((perm & L2_EXIST) == 0 || !perm_have_user(perm))
			return -E_INVAL;
		if ((perm & ~L2_SMALL_FOR_USER) != 0)
			return -E_INVAL;
		if(e->env_ipc_dstva == 0)
			return -E_INVAL;
		l2e_t *pte;
		struct PageInfo *pi;
		if((pi = page_lookup(curenv->env_pgdir, srcva, &pte)) == NULL)
			return -E_INVAL;
		if(perm_have(perm, L2_SMALL_AP_URW_SRW) && !perm_have(*pte, L2_SMALL_AP_URW_SRW))
			return -E_INVAL;
		if((r = page_insert(e->env_pgdir, pi, e->env_ipc_dstva, perm)) < 0)
			return r;

		e->env_ipc_perm = perm;
	}
	e->env_ipc_from = curenv->env_id;
	e->env_ipc_value = value;
	e->env_ipc_recving = false;
	e->env_status = ENV_RUNNABLE;

	e->env_tf.r[0] = 0; // 因为recv中调用了sched_yield()，syscall不会返回，所以需要在这里设置为0
	return 0;
	panic("sys_ipc_try_send not implemented");
}


static int sys_ipc_recv(void *dstva)
{
	if(is_user_mem(dstva)){
		if((uint32_t)dstva % PGSIZE != 0)
			return -E_INVAL;
		curenv->env_ipc_dstva = dstva;
	}
	curenv->env_status = ENV_NOT_RUNNABLE;
	curenv->env_ipc_from = 0;
	curenv->env_ipc_recving = true;
	sched_yield();

	return 0;
}

static int sys_page_map(envid_t srcenvid, void *srcva, envid_t dstenvid, void *dstva, int perm)
{
	return env_page_map(srcenvid, srcva, dstenvid, dstva, perm);
}

static int sys_page_unmap(envid_t envid, void *va)
{
	return env_page_unmap(envid, va);
}

static int sys_page_alloc(envid_t envid, void *va, int perm)
{
	return env_page_alloc(envid, va, perm);
}

static int sys_env_set_pgfault_upcall(envid_t envid, void *func)
{
	// LAB 4: Your code here.
	struct Env *e;
	int r;

	if((r = envid2env(envid, &e, 1)) < 0)
		return r;
	e->env_pgfault_upcall = func;

	return 0;
	panic("sys_env_set_pgfault_upcall not implemented");
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int sys_env_destroy(envid_t envid)
{
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	env_destroy(e);
	return 0;
}

static int sys_get_l1(void *va)
{
	return curenv->env_pgdir[L1X(va)];
}

static int sys_get_l2(void *va)
{
	return (int)*pgdir_walk(curenv->env_pgdir, va, 0);
}

// Allocate a new environment.
// Returns envid of new environment, or < 0 on error.  Errors are:
//	-E_NO_FREE_ENV if no free environment is available.
//	-E_NO_MEM on memory exhaustion.
static envid_t sys_exofork(void)
{
	// Create the new environment with env_alloc(), from kern/env.c.
	// It should be left as env_alloc created it, except that
	// status is set to ENV_NOT_RUNNABLE, and the register set is copied
	// from the current environment -- but tweaked so sys_exofork
	// will appear to return 0.

	// LAB 4: Your code here.
	int r;
	struct Env *e;
	if((r = env_alloc(&e, curenv->env_id)) < 0)
		return r;
	e->env_status = ENV_NOT_RUNNABLE;
	memcpy(&e->env_tf, &curenv->env_tf, sizeof(e->env_tf));
	e->env_tf.r[0] = 0;

	return e->env_id;
	// panic("sys_exofork not implemented");
}

// Set envid's trap frame to 'tf'.
// tf is modified to make sure that user environments always run at code
// protection level 3 (CPL 3), interrupts enabled, and IOPL of 0.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int sys_env_set_trapframe(envid_t envid, struct Trapframe *tf)
{
	// LAB 5: Your code here.
	// Remember to check whether the user has supplied us with a good
	// address!
	struct Env *e;
	int r;
	
	if((r = envid2env(envid, &e, 1)) < 0)
		return r;
	user_mem_assert(curenv, tf, sizeof(tf), 0);
	e->env_tf = *tf;
	cprintf("sys_env_set_trapframe env %p, ip %p\n", e->env_id, e->env_tf.user_ip);
	e->env_tf.user_flags = 0x60000100 | CPSR_M_USR;
	e->env_tf.user_lr = 0;

	return 0;
	// panic("sys_env_set_trapframe not implemented");
}

// Set envid's env_status to status, which must be ENV_RUNNABLE
// or ENV_NOT_RUNNABLE.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if status is not a valid status for an environment.
static int
sys_env_set_status(envid_t envid, int status)
{
	// Hint: Use the 'envid2env' function from kern/env.c to translate an
	// envid to a struct Env.
	// You should set envid2env's third argument to 1, which will
	// check whether the current environment has permission to set
	// envid's status.

	// LAB 4: Your code here.
	int r;
	struct Env *e;

	if(status != ENV_RUNNABLE && status != ENV_NOT_RUNNABLE)
		return -E_INVAL;
	if((r = envid2env(envid, &e, 1)) < 0)
		return r;

	e->env_status = status;
	return 0;
	// panic("sys_env_set_status not implemented");
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int sys_cgetc(void)
{
	return cons_getc();
}
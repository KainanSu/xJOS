#include <kern/syscall.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>
#include <inc/arm.h>
#include <kern/sched.h>
#include <inc/string.h>
#include <imx6/imx6ul.h>
#include <inc/log.h>

void print_trapframe(struct Trapframe *tf);
static void page_fault_handler(struct Trapframe *tf);

void svc_handler(struct Trapframe* tf){
	// cprintf("Incoming TRAP frame at %p\n", tf);
    // cprintf("svc_handler\n");

	// some syscall may not return, so save tf first
	curenv->env_tf = *tf;
	// tf = &curenv->env_tf;
    // cprintf("env %s svc_handler\n", curenv->env_id == 4096 ? "A" : "B");

    curenv->env_tf.r[0] = syscall(curenv->env_tf.r[7], curenv->env_tf.r[0], 
		curenv->env_tf.r[1], curenv->env_tf.r[2], curenv->env_tf.r[3], curenv->env_tf.r[4]);

	if (curenv && curenv->env_status == ENV_RUNNING)
		env_run(curenv);
	else
		sched_yield();

    print_trapframe(&curenv->env_tf);
    panic("svc_handler:should not reach here\n");
}

void data_abort_handler(struct Trapframe *tf)
{
    page_fault_handler(tf);

	if (curenv && curenv->env_status == ENV_RUNNING)
		env_run(curenv);
	else
		sched_yield();

    print_trapframe(&curenv->env_tf);
    panic("svc_handler:should not reach here\n");
}

void irq_handler(struct Trapframe *tf)
{
	uint32_t irq_num = read_irq_num();
	if(curenv){
		curenv->env_tf = *tf;
		// tf = &curenv->env_tf;
		curenv->env_tf.user_ip -= 4;
	}
    // cprintf("irq_handler\n");

	switch(irq_num){
		case 88:
			write_irq_eoir(irq_num);
			((EPIT_Type *)(AIPS1BASE+EPIT1_OFFSET))->SR |= 1<<0;
			sched_yield();
		break;

		default:
		break;
	}

	panic("known irq num: %d\n", irq_num);
}

void print_trapframe(struct Trapframe *tf)
{
	cprintf("TRAP frame at %08x\n", tf);
    for(int i = 0; i < 13; i++)
	    cprintf("  r[%d]     -------%04d\n", i, tf->r[i]);
	cprintf("  cpsr      0x----%08x\n", tf->user_flags);
	cprintf("  user_sp   0x----%08x\n", tf->user_sp);
	cprintf("  user_lr   0x----%08x\n", tf->user_lr);
	cprintf("  user_ip   0x----%08x\n", tf->user_ip);

	// If this trap was a page fault that just happened
	// (so %cr2 is meaningful), print the faulting linear address.
	// if (tf == last_tf && tf->tf_trapno == T_PGFLT)
	// 	cprintf("  cr2  0x%08x\n", rcr2());
	// For page faults, print decoded fault error code:
	// U/K=fault occurred in user/kernel mode
	// W/R=a write/read caused the fault
	// PR=a protection violation caused the fault (NP=page not present).
	// if (tf->tf_trapno == T_PGFLT)
	// 	cprintf(" [%s, %s, %s]\n",
	// 		tf->tf_err & 4 ? "user" : "kernel",
	// 		tf->tf_err & 2 ? "write" : "read",
	// 		tf->tf_err & 1 ? "protection" : "not-present");
	// else
	// 	cprintf("\n");
	// cprintf("  eip  0x%08x\n", tf->tf_eip);
	// cprintf("  cs   0x----%04x\n", tf->tf_cs);
	// cprintf("  flag 0x%08x\n", tf->tf_eflags);
	// if ((tf->tf_cs & 3) != 0) {
	// 	cprintf("  esp  0x%08x\n", tf->tf_esp);
	// 	cprintf("  ss   0x----%04x\n", tf->tf_ss);
	// }
}

// Travers L2，copy every page in L2 to new page.
// map new page to L2, with RW flag
// unmap origin page
static int copy_on_write_handler(void* addr){
	int r;
	envid_t envid = curenv->env_id;
	// cprintf("copy_on_write_handler start\n");

	addr = ROUNDDOWN(addr, PGSIZE * L2NENTRIES); // round down to l1 addr
	void *l1addr = addr;

	l2e_t* l2table = (l2e_t*)(KADDR(L1TABLE(curenv->env_pgdir[L1X(addr)])));

	for(int i = 0; i < L2NENTRIES; i++) {
		if(l2table[i] & L2_EXIST) {
			assert(perm_have(l2table[i], L2_TYPE_SMALL));
			if ((r = env_page_alloc(envid, (void *)PFTEMP, L2_TYPE_SMALL | L2_SMALL_AP_URW_SRW)) < 0)
				panic("pgfault: page allocation failed %e", r);
			memmove(PFTEMP, addr, PGSIZE);
			if ((r = env_page_unmap(envid, addr)) < 0)
				panic("pgfault: page unmap failed %e", r);
			if ((r = env_page_map(envid, PFTEMP, envid, addr, L2_TYPE_SMALL | L2_SMALL_AP_URW_SRW)) < 0)
				panic("pgfault: page map failed %e", r);
			if ((r = env_page_unmap(envid, PFTEMP)) < 0)
				panic("pgfault: page unmap failed %e", r);
		}
		addr += PGSIZE;
	}
	update_l1entry(curenv->env_pgdir, l1addr, L1_TYPE_TABLE);

	return 0;
}

// Travers L2，copy every page in L2 to new page.
// map new page to L2, with RW flag
// unmap origin page
static void page_fault_handler(struct Trapframe *tf)
{
	uint32_t fault_va, err_info;
	int r;

	fault_va = read_far();
	err_info = read_dfsr();
	bool in_user_mode = !(read_spsr() & 0xf);
	bool err_write = false, err_violation = false, err_nexist = false;

	// refer to Cortex-A7 TRM page 128: Data Fault Status Register(DFSR)
	if (((err_info >> 11) & 0x1) == 0x1){
		err_write = true;
	}
	if ((err_info & 0b1101) == 0b0101){
		err_nexist = true;
	}
	if ((err_info & 0b1111) == 0b1111){
		err_violation = true;
	}

	if (!in_user_mode) {
		cprintf("[%08x] kernel fault va %08x ip %08x\n",
		curenv->env_id, fault_va, tf->user_ip);
		if(fault_va > ISPTOP && fault_va < KERNBASE)
			cprintf("kernel read or write user mem fault. Please whether check user have permission, using user_mem_assert().\n");
		print_trapframe(tf);
		panic("kernel-mode page faults");
	}

	if(curenv->env_type == ENV_TYPE_FS){

		log_trace("comming ENV_TYPE_FS pgfault\n");

		struct UTrapframe *utf;
		if(curenv->env_pgfault_upcall == NULL){
			log_err("fs didn't registered env_pgfault_upcall\n");
			return;
		}

		if(tf->user_sp >= UXSTACKTOP - PGSIZE && tf->user_sp <= UXSTACKTOP - 1)
			utf = (struct UTrapframe *)(tf->user_sp - sizeof(struct UTrapframe) - 4);
		else
			utf =  (struct UTrapframe *)(UXSTACKTOP - sizeof(struct UTrapframe));
		
		// 检查异常栈是否存在、是否溢出
		user_mem_assert(curenv, (const void*)utf, sizeof(struct UTrapframe), L2_SMALL_AP_URW_SRW);

		utf->utf_fault_va = fault_va;
		utf->utf_err = err_info;
		for(int i = 0; i < 13; i++)
			utf->r[i] = tf->r[i];
		utf->utf_eip = tf->user_ip - 8;
		utf->utf_lr = tf->user_lr;
		utf->utf_flags = tf->user_flags;
		utf->utf_sp = tf->user_sp;

		curenv->env_tf.user_ip = (uint32_t)curenv->env_pgfault_upcall;
		curenv->env_tf.user_sp = (uint32_t)utf;
		log_trace("curenv->env_tf.user_sp: %p\n", curenv->env_tf.user_sp);
		env_run(curenv);

	}else{
		if(!is_user_mem((void*)fault_va))
			goto user_page_fault_hander_err;

		tf->user_ip = (uint32_t)tf->user_ip - 8;
		if(err_write && (curenv->env_pgdir[L1X(fault_va)] & L1_TABLE_COW)){
			r = copy_on_write_handler((void*)fault_va);
			if(r < 0)
				panic("copy_on_write_handler err");
			return;
		}
	}

user_page_fault_hander_err:
	cprintf("[%08x] user fault va %08x ip %08x. ", curenv->env_id, fault_va, tf->user_ip);
	cprintf("err: %s %s %s\n", err_write ? "write" : "read", err_nexist ? "not exist" : "", err_violation ? "violation" : "");
	mon_backtrace(0, 0, tf);
	env_destroy(curenv);
}


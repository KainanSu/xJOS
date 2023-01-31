// test user-level fault handler -- alloc pages to fix faults

#include <inc/lib.h>

void
handler(struct UTrapframe *utf)
{
	log_trace("user pf handler start\n");

	int r;
	void *addr = (void*)utf->utf_fault_va;

	log_trace("fault %x\n", addr);

	if ((r = sys_page_alloc(0, ROUNDDOWN(addr, PGSIZE), L2_TYPE_SMALL | L2_SMALL_AP_URW_SRW)) < 0)
		panic("allocating at %x in page fault handler: %e", addr, r);
	snprintf((char*) addr, 100, "this string was faulted in at %x", addr);

	log_trace("user pf handler end\n");
}

void
umain(int argc, char **argv)
{
	set_pgfault_handler(handler);
	cprintf("%s\n", (char*)0xDeadBeef);
	cprintf("%s\n", (char*)0xCafeBffe);
}

#include <inc/lib.h>

int
pageref(void *v)
{
	l2e_t pte;

	if(!(sys_get_l1(v) & L1_EXIST))
		return 0;
	assert((sys_get_l1(v) & L1_EXIST) == L1_TYPE_TABLE);

	pte = sys_get_l2(v);
	if (!(pte & L2_EXIST))
		return 0;
	assert((sys_get_l2(v) & L2_EXIST) == L2_TYPE_SMALL);

	return pages[PGNUM(pte)].pp_ref;
}

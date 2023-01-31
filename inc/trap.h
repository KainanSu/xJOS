#ifndef JOS_INC_TRAP_H
#define JOS_INC_TRAP_H

#ifndef __ASSEMBLER__

#include <inc/types.h>

struct Trapframe {
    uint32_t user_flags;
    uint32_t user_sp;
    uint32_t user_lr;
    uint32_t r[13];
    uint32_t user_ip; // TODO: rename to trap ip
} __attribute__((packed));

struct UTrapframe {
	/* information about the fault */
	uint32_t utf_fault_va;	/* va for T_PGFLT, 0 otherwise */
	uint32_t utf_err;
	/* trap-time return state */
    uint32_t r[13];
    uint32_t utf_lr;
    uint32_t utf_eip;
    uint32_t utf_flags;
    uint32_t utf_sp;
} __attribute__((packed));

#endif /* !__ASSEMBLER__ */

#endif /* !JOS_INC_TRAP_H */

#include <inc/mmu.h>
#include <inc/memlayout.h>

.text
.extern _pgfault_handler
.globl _pgfault_upcall
_pgfault_upcall:
	// Call the C page fault handler.
	ldr     r0, =_pgfault_handler
    ldr     r1, [r0]
    mov     r0, sp // function argument: pointer to UTF
    blx     r1
	
	// Now the C page fault handler has returned and you must return
	// to the trap time state.
	// Push trap-time %eip onto the trap-time stack.
	//
	// Explanation:
	//   We must prepare the trap-time stack for our eventual return to
	//   re-execute the instruction that faulted.
	//   Unfortunately, we can't return directly from the exception stack:
	//   We can't call 'jmp', since that requires that we load the address
	//   into a register, and all registers must have their trap-time
	//   values after the return.
	//   We can't call 'ret' from the exception stack either, since if we
	//   did, %esp would have the wrong value.
	//   So instead, we push the trap-time %eip onto the *trap-time* stack!
	//   Below we'll switch to that stack and call 'ret', which will
	//   restore %eip to its pre-fault value.
	// 我们必须准备好trap-time堆栈，以便最终返回来重新执行出错的指令。
	// 不幸的是，我们不能直接从异常堆栈返回:我们不能调用'jmp'，
	// 因为这要求我们将地址加载到寄存器中，并且所有寄存器在返回后都必须有它们的陷阱时间值。
	// 我们也不能从异常堆栈中调用'ret'，因为如果我们这样做，%esp会有错误的值。
	// 因此，我们将trap-time %eip推入*trap-time*堆栈!下面我们将切换到该堆栈并调用'ret'，这将恢复%eip到故障前的值。
	//
	//   In the case of a recursive fault on the exception stack,
	//   note that the word we're pushing now will fit in the
	//   blank word that the kernel reserved for us.
	//  在异常堆栈上出现递归错误的情况下，请注意我们现在push的字将适合内核为我们保留的空白字。
	//
	// Throughout the remaining code, think carefully about what
	// registers are available for intermediate calculations.  You
	// may find that you have to rearrange your code in non-obvious
	// ways as registers become unavailable as scratch space.
	// 在剩下的代码中，仔细考虑哪些寄存器可用于中间计算。
	// 您可能会发现必须以不明显的方式重新排列代码，因为寄存器作为临时空间不可用。
	// LAB 4: Your code here.
	// 在trap-time stack（可以是异常栈或者用户栈）存入eip
	@ movl 48(%esp), %ebp
	@ subl $4, %ebp
	@ movl %ebp, 48(%esp)
	@ movl 40(%esp), %eax // push eip to trap-time stack top
	@ movl %eax, (%ebp)

	# utf->sp = utf->sp - 4
	ldr r0, [sp, #72]
	sub	r0, r0, #4
	str r0, [sp, #72]

	# eip => trapsp
	ldr r1, [sp, #64]
	str r1, [r0]

	// Restore the trap-time registers.  After you do this, you
	// can no longer modify any general-purpose registers.
	@ addl $8, %esp
	@ popal
	# skip fault_va and err
	add	sp, sp, #8
	ldmfd sp!, {r0-r12,lr}

	// Restore eflags from the stack.  After you do this, you can
	// no longer use arithmetic operations or anything else that
	// modifies eflags.
	# skip eip and flags
	add	sp, sp, #8

	// Switch back to the adjusted trap-time stack.
	pop {sp}

	// Return to re-execute the instruction that faulted.
	pop {pc}

#include <inc/mmu.h>
#include <inc/memlayout.h>

.extern svc_handler
.extern data_abort_handler
.extern irq_handler
// To keep this in the first portion of the binary.
.section ".text.boot"

@ .section ".interrupt_vector_table"
vector_table:
    b       reset_handler
    b       undefined_handler_isp
    b       svc_isp
	b       undefined_prefetch_abort_isp @ prefetch_abort_isp
    b       data_abort_isp
    nop
    b       irq_isp

undefined_handler_isp:
	b .
undefined_prefetch_abort_isp:
    b .

svc_isp:
    clrex
    stmfd   sp!, {r0-r12,lr}
    mrs     r0, spsr
    mrs     r1, sp_usr
    mrs     r2, lr_usr
    stmfd   sp!, {r0-r2}

    ldr     r0, =.svc_handler_addr
    ldr     r1, [r0]
    mov     r0, sp
    blx     r1

data_abort_isp:
    clrex
    @ subs    lr, lr, #8 @ set in handler
    stmfd   sp!, {r0-r12,lr}
    mrs     r0, spsr
    mrs     r1, sp_usr
    mrs     r2, lr_usr
    stmfd   sp!, {r0-r2}

    ldr     r0, =.data_abort_handler_addr
    ldr     r1, [r0]
    mov     r0, sp
    blx     r1

@ now in irq, cspr's 'i' bit is set, irq now disable
irq_isp:
    clrex
    stmfd   sp!, {r0-r12,lr}
    mrs     r0, spsr
    mrs     r1, sp_usr
    mrs     r2, lr_usr
    stmfd   sp!, {r0-r2}

    ldr     r0, =.irq_handler_addr
    ldr     r1, [r0]
    mov     r0, sp
    blx     r1

// Make _start global.
.globl _start
_start:
.globl entry
entry:
reset_handler:
	/* Initialize .bss */
    mov r0, #0
    ldr r1, =(__bss_start__ - KERNBASE + DDRBASE)
    ldr r2, =(__bss_end__ - KERNBASE + DDRBASE)
bss_loop:
    cmp r1, r2
    strlt r0, [r1], #4
    blt bss_loop

	// Turn on the MMU
	// Ref. http://www.embedded-bits.co.uk/2011/mmucode/
	ldr r0, =(entry_pgdir - KERNBASE + DDRBASE)
	mcr p15, 0, r0, c2, c0, 0

	ldr r0, =0x55555555
	mcr p15, 0, r0, c3, c0, 0

	mrc p15, 0, r0, c1, c0, 0
	orr r0, r0, #0x1
	mcr p15, 0, r0, c1, c0, 0

	// Jump up above KERNBASE before entering C code
	// 这里为什么要用bx?
	ldr lr, =relocated
	bx lr
relocated:
    @ abort
	mrs r0, cpsr
    bic r0, r0, #0x1f
    orr r0, r0, #0x17
	msr cpsr, r0
    ldr sp, =kernstacktop

    @ irq
	mrs r0, cpsr
    bic r0, r0, #0x1f
    orr r0, r0, #0x12
	msr cpsr, r0
    ldr sp, =kernstacktop

    @ svc
    mrs r0, cpsr
    bic r0, r0, #0x1f
    orr r0, r0, #0x13
	msr cpsr, r0
	mov r11, #0
	ldr sp, =kernstacktop  // Setup the stack.

	bl arm_init

	// halt, shoud not reach here
halt:
	wfe
	b halt

.data
.svc_handler_addr:
    .word svc_handler
.data_abort_handler_addr:
    .word data_abort_handler
.irq_handler_addr:
    .word irq_handler
// kernel stack: svc/irq/abort use the same one stack
.p2align 20         // force page alignment to 1M
.globl kernstack
kernstack:
	.space          KSTKSIZE
.globl kernstacktop
kernstacktop:

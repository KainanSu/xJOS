#ifndef JOS_INC_ARM_H
#define JOS_INC_ARM_H

#include "inc/types.h"
#include "inc/memlayout.h"

#define CPSR_M_USR 0x10U
#define CPSR_M_FIQ 0x11U
#define CPSR_M_IRQ 0x12U
#define CPSR_M_SVC 0x13U
#define CPSR_M_MON 0x16U
#define CPSR_M_ABT 0x17U
#define CPSR_M_HYP 0x1AU
#define CPSR_M_UND 0x1BU
#define CPSR_M_SYS 0x1FU

static inline __attribute__((always_inline)) uint32_t read_r11(void)
{
	uint32_t r11;
	asm volatile("mov %0, r11" : "=r" (r11));
	return r11;
}

static inline __attribute__((always_inline)) uint32_t read_far()
{
    uint32_t val;
    asm volatile("mrc p15, 0, %0, c6, c0, 0": "=r"(val) :);
    return val;
}

static inline __attribute__((always_inline)) uint32_t read_dfsr()
{
    uint32_t val;
    asm volatile("mrc p15, 0, %0, c5, c0, 0" : "=r"(val) :);
    return val;
}

static inline __attribute__((always_inline)) uint32_t read_spsr()
{
    uint32_t spsr;
    asm volatile("mrs %0, spsr" : "=r"(spsr) :);
    return spsr;
}

inline static __attribute__((always_inline)) void tlb_invalidate(void *va)
{
    asm("mcr p15, 0, %0, c8, c7, 1":: "r"(va):);
}

inline static __attribute__((always_inline)) void flush_whole_tlb()
{
    asm volatile("mcr p15, 0, %0, c8, c7, 0" : : "r"(0) : "memory");
}

static inline __attribute__((always_inline)) void load_pgdir(uint32_t value) {
	asm volatile ("mcr p15, 0, %0, c2, c0, 0" : : "r"(value));
	flush_whole_tlb();
}

static inline __attribute__((always_inline)) void system_disable_interrupts() { 
    asm volatile("cpsid i"); 
}

static inline __attribute__((always_inline)) void system_enable_interrupts() { 
    asm volatile("cpsie i"); 
}

static inline __attribute__((always_inline)) uint32_t read_irq_num() { 
    uint32_t num;
	asm volatile("ldr %0, [%1]": "=r" (num):"r"(GICBASE+0x2000+0xC));
	return num;
}

static inline __attribute__((always_inline)) void write_irq_eoir(uint32_t irq_num) {
	asm volatile ("str %0, [%1]" : : "r"(irq_num), "r"(GICBASE+0x2000+0x10));
}
#endif

#include <inc/stdio.h>
#include <inc/assert.h>
#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/pmap.h>
#include <kern/env.h>
#include <inc/arm.h>

extern void clk_enable(void);
extern void imx6u_clkinit(void);
extern void gic_init(void);
extern void epit1_init(unsigned int ms);


// Test the stack backtrace function (lab 1 only)
void
test_backtrace(int x)
{
	cprintf("entering test_backtrace %d\n", x);
	if (x > 0)
		test_backtrace(x-1);
	else
		mon_backtrace(0, 0, 0);
	cprintf("leaving test_backtrace %d\n", x);
}

void arm_init()
{
    system_disable_interrupts();
    gic_init();
    imx6u_clkinit();
    clk_enable();
    epit1_init(50);

    cons_init();
    cprintf("xJOS: Hello World\n");
    mem_init();
    env_init();

    // ENV_CREATE(user_hello, ENV_TYPE_USER);
    ENV_CREATE(user_procA, ENV_TYPE_USER);
    ENV_CREATE(user_procB, ENV_TYPE_USER);
    // ENV_CREATE(user_testfork, ENV_TYPE_USER);
    // ENV_CREATE(user_primes, ENV_TYPE_USER);
    // ENV_CREATE(user_faultalloc, ENV_TYPE_FS);
    // ENV_CREATE(fs_fs, ENV_TYPE_FS);
    // ENV_CREATE(user_init, ENV_TYPE_USER);

    system_enable_interrupts();

    // wait timer irq to yield
    while(1);

    panic("stop");
}

/*
 * Variable panicstr contains argument to first call to panic; used as flag
 * to indicate that the kernel has already called panic.
 */
const char *panicstr;

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", and then enters the kernel monitor.
 */
    void
_panic(const char *file, int line, const char *fmt,...)
{
    va_list ap;

    if (panicstr)
	goto dead;
    panicstr = fmt;

    // Be extra sure that the machine is in as reasonable state
    // __asm __volatile("cli; cld");

    va_start(ap, fmt);
    cprintf("kernel panic on CPU at %s:%d: ", file, line);
    vcprintf(fmt, ap);
    cprintf("\n");
    va_end(ap);

dead:
    /* break into the kernel monitor */
	monitor(NULL);
}

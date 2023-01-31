//copyright@Yiru Chen

#include <inc/memlayout.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/console.h>

#include <imx6/imx6ul.h>

// Ref. http://wiki.osdev.org/ARM_RaspberryPi_Tutorial_C

static void cons_intr(int (*proc)(void));
static void cons_putc(int c);


static inline void mmio_write(volatile uint32_t* reg, uint32_t data)
{
	*(volatile uint32_t *)reg = data;
}
 
static inline uint32_t mmio_read(volatile uint32_t* reg)
{
	return *(volatile uint32_t *)reg;
}

static UART_Type *uart1 = (UART_Type *)(AIPS1BASE + UART1_OFFSET); // 0202_0000 0202_3FFF

void uart_init(void)
{
	// !first: make sure the map address is the same as the physical address
	IOMUXC_SetPinMux(IOMUXC_UART1_TX_DATA_UART1_TX, 0);
	IOMUXC_SetPinMux(IOMUXC_UART1_RX_DATA_UART1_RX, 0);
	IOMUXC_SetPinConfig(IOMUXC_UART1_TX_DATA_UART1_TX, 0x10B0);
	IOMUXC_SetPinConfig(IOMUXC_UART1_RX_DATA_UART1_RX, 0x10B0);

	uart1->UCR1 &= ~(1<<0); // disable
	
	uart1->UCR2 &= ~(1<<0); // soft reset
	while((uart1->UCR2 & 0x1) == 0);

	uart1->UCR1 = 0;		// function
	uart1->UCR1 &= ~(1<<14);
	uart1->UCR2 |= (1<<14) | (1<<5) | (1<<2) | (1<<1);
	uart1->UCR3 |= 1<<2; 

	// uart的时钟源为PLL3/6=80M，下面是波特率的计算公式
	uart1->UFCR = 5<<7; // baudrate
	uart1->UBIR = 71;
	uart1->UBMR = 3124;

	uart1->UCR1 |= (1<<0); // enable
}

void uart_putc(unsigned char byte)
{
	// Wait for UART to become ready to transmit.
	while (!(mmio_read(&uart1->USR2) & (1 << 3))) { }
	mmio_write(&uart1->UTXD, byte);
}

int uart_proc_data()
{
    if (!(mmio_read(&uart1->USR2) & (1 << 0)))
    	return -1;
    return mmio_read(&uart1->URXD);
}

void uart_intr(void)
{
	cons_intr(uart_proc_data);
}

/***** General device-independent console code *****/
// Here we manage the console input buffer,
// where we stash characters received from the keyboard or serial port
// whenever the corresponding interrupt occurs.

#define CONSBUFSIZE 512

static struct {
	uint8_t buf[CONSBUFSIZE];
	uint32_t rpos;
	uint32_t wpos;
} cons;

// called by device interrupt routines to feed input characters
// into the circular console input buffer.
static void
cons_intr(int (*proc)(void))
{
	int c;

	while ((c = (*proc)()) != -1) {
		if (c == 0)
			continue;
		cons.buf[cons.wpos++] = c;
		if (cons.wpos == CONSBUFSIZE)
			cons.wpos = 0;
	}
}

// return the next input character from the console, or 0 if none waiting
int
cons_getc(void)
{
	unsigned char c;

	// poll for any pending input characters,
	// so that this function works even when interrupts are disabled
	// (e.g., when called from the kernel monitor).
	uart_intr();

	// grab the next character from the input buffer.
	if (cons.rpos != cons.wpos) {
		c = cons.buf[cons.rpos++];
		if (cons.rpos == CONSBUFSIZE)
			cons.rpos = 0;
		return c;
	}
	return 0;
}

// output a character to the console
static void
cons_putc(int c)
{
	uart_putc(c);
}

// initialize the console devices
void
cons_init(void)
{
	uart_init();
}


// `High'-level console I/O.  Used by readline and cprintf.

void
cputchar(int c)
{
	cons_putc(c);
}

int
getchar(void)
{
	int c;

	while ((c = cons_getc()) == 0)
		/* do nothing */;
	return c;
}

int
iscons(int fdnum)
{
	// used by readline
	return 1;
}

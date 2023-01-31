/* Simple linker script for the JOS kernel.
   See the GNU ld 'info' manual ("info ld") to learn the syntax. */
OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(ARM)
ENTRY(_start)

SECTIONS
{
	/* 0xF0100000和0x80100000需要一起改，因为AT是运行地址，也应该是链接地址 */ 
	/* Link the kernel at this address: "." means the current address */
	. = 0xF0000000;
	PROVIDE(start = .);

	/* AT(...) gives the load address of this section, which tells
	   the boot loader where to load the kernel in physical memory */
	/* 需要设置为PHYBASE  */
	.text : AT(0x80000000) {
		*(.text .stub .text.* .gnu.linkonce.t.*)
	}

	PROVIDE(etext = .);	/* Define the 'etext' symbol to this value */

	.rodata : {
		*(.rodata .rodata.* .gnu.linkonce.r.*)
	}

	/* Include debugging information in kernel memory */
	.stab : {
		PROVIDE(__STAB_BEGIN__ = .);
		*(.stab);
		PROVIDE(__STAB_END__ = .);
		BYTE(0)		/* Force the linker to allocate space
				   for this section */
	}

	.stabstr : {
		PROVIDE(__STABSTR_BEGIN__ = .);
		*(.stabstr);
		PROVIDE(__STABSTR_END__ = .);
		BYTE(0)		/* Force the linker to allocate space
				   for this section */
	}

	/* Adjust the address for the data segment to the next page */
	. = ALIGN(0x1000);

	/* The data segment */
	.data : {
		*(.data)
	}

	.bss : {
		__bss_start__ = .;
		*(.bss)
	}
	__bss_end__ = .;

	/DISCARD/ : {
		*(.eh_frame .note.GNU-stack)
	}
}

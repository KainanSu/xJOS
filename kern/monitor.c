// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/arm.h>
#include <inc/trap.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display the call stack backtrace", mon_backtrace }, 
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char start[], entry[], etext[], __bss_start__[],__bss_end__[];
	//unsigned int BASE = 0x8000;
	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", __bss_start__, __bss_start__ - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", __bss_end__, __bss_end__ - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(__bss_end__ - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	uint32_t tmp_fp, tmp_lr, first_pc;
	struct Eipdebuginfo info;
	int frame_cnt = 0;

	if(tf == NULL){
		tmp_fp = read_r11();
		first_pc = 0;
	}else{
		tmp_fp = tf->r[11];
		first_pc = tf->user_lr - 8;
	}

	cprintf("Stack backtrace:\n");
	cprintf("--frame %d------\n", frame_cnt);
	if (debuginfo_eip(first_pc, &info) == 0){
		int fn_offset = first_pc - info.eip_fn_addr;
		cprintf("     %s:%d: %.*s+%d\n", info.eip_file, info.eip_line, info.eip_fn_namelen, info.eip_fn_name, fn_offset);
	}
	frame_cnt++;
	// in arm, r11 points to ret addr, and old r11 is just below ret addr

	while (tmp_fp != 0x0) {
		// ret addr stays just above fp
		tmp_lr = *((uint32_t*)tmp_fp);

		cprintf("--frame %d------\n", frame_cnt);
		cprintf("  eip: %08x", tmp_lr);
		// cprintf("     fp: %08x; eip: %08x", tmp_fp, tmp_lr);
		
		// find the debuginfo of the instruction which ret addr indicates
		// and check whether the debug info is valid
		if (debuginfo_eip(tmp_lr, &info) == 0) {
			cprintf("  %s:%d: %.*s+%d\n", 
					info.eip_file, 
					info.eip_line,
					info.eip_fn_namelen, info.eip_fn_name,
					(int)tmp_lr - (int)info.eip_fn_addr);
		}
		else {
			cprintf("  debuginfo not available\n");
		}

		tmp_fp = *((uint32_t*)tmp_fp - 1);
		frame_cnt++;
	}
	return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}

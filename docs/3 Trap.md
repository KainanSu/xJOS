# 3. Trap

要实现CPU运行多个任务，自然要完成内核与用户进程的切换。切换过程包括运行空间（pc、sp寄存器、页表），上下文（CPU所有寄存器），CPU运行模式（cpsr寄存器）的切换。内核调度用户进程，将CPU设置为用户上下文和运行空间，然后修改cpsr，对应内核中的env_run()函数；而用户进程主动进入内核，一般是通过系统调用，系统调用在ARM上使用软中断swi指令实现，对应lib中的syscall()函数。

本节先讲在ARM上如何从内核切换为用户进程运行，接着讲ARM系统调用的实现机制，看看如何从用户进程进入内核，以及如何从内核返回到用户进程，最后讲如何编写xJOS的用户程序并在xJOS中运行。

# Envs

运行用户进程（env）涉及的变量及之间的关系如图所示。env[0]是描述进程独立的变量，主要包括上下文（trapframe）和页表（pgdir），trapframe结构如tf所示。用户程序编译后的elf将被链接到内核镜像中的data段，一起加载到物理内存（physical）中。在加载用户程序时，使用region_alloc为用户虚拟空间（user virtual）分配物理页面，并将elf段复制到其指定的加载位置。

![Untitled](3%20Trap%203b934bc433bd47f580408f70f16e7a88/Untitled.png)

为了让user跑起来，需要实现以下方法：（1）内核管理envs[]（2）创建一个envs[n]，并初始化pgdir和trapframe（3）为user pgdir分配所需空间，然后将elf加载到user的pgdir（4）实现kernel到user的切换

| env_init、env_alloc、env_free | 管理env_free_list。 |
| --- | --- |
| load_icode | 将elf内容加载到用户进程 |
| env_create、env_destroy | env_create调用env_alloc分配一个设置好vm和tf的env，然后调用load_icode加载elf到vm |
| env_run、env_pop_tf | env_run实现curenv的更新，以及调用env_pop_tf实现从trapframe恢复CPU状态 |

env_alloc需要复制内核页表的共享部分到用户页表，以及设置trapframe为CPU可以运行的模样。共享部分页表主要包括UTOP以上的空间和[0, 1M]地址空间，UTOP以上的空间中有pages和envs等user可读的信息，低1M空间是ARM中断机制所需要的，如swi中断发生时，cpu会设置pc=0x8运行中断服务程序。tf主要设置sp、lr和spsr寄存器，而user_pc在加载elf时才设置。核心代码如下：

```c
int env_alloc(struct Env **newenv_store, envid_t parent_id){

	// Setup vitrual memory
	for(int i = L1X(UTOP); i < L1NENTRIES; i++)
		e->env_pgdir[i] = kern_pgdir[i];
	e->env_pgdir[0] = kern_pgdir[0]; // interrupt handlers
	
	// Setup trapframe
	memset(&e->env_tf, 0, sizeof(e->env_tf));
	e->env_tf.user_flags = 0x60000100 | CPSR_M_USR;
	e->env_tf.user_sp = USTACKTOP;
	e->env_tf.user_lr = 0;

	// Set the basic status variables.
	...
	
  // commit
	env_free_list = e->env_link;
	*newenv_store = e;

	return 0;
}
```

load_icode从elf中加载段到用户进程空间（下文会提到用户程序elf会被链接到kernel的data段中）。为了将复制数据到用户进程，先切换页表为用户页表。接着从elf中复制每一个ELF_PROG_LOAD类型的段到用户空间。值得注意的是，p_memsz和p_filesz之间的空间需要设为0，如bss段。最后将user_ip设置为entry，当用户进程运行时，从entry开始运行。核心代码如下：

```c
static void load_icode(struct Env *e, uint8_t *binary){

	// for copy from kernel to user space, use user's pgdir
	load_pgdir(PADDR(e->env_pgdir));
  
	struct Elf *elfhdr = (struct Elf *)binary;
	...
	for (; ph < eph; ph++){
		if(ph->p_type != ELF_PROG_LOAD)
			continue;
		region_alloc(e, (void *)ph->p_va, ph->p_memsz);
		memcpy((void *)ph->p_va, binary + ph->p_offset, ph->p_filesz);
		memset((void *)(ph->p_va + ph->p_filesz), 0, (ph->p_memsz - ph->p_filesz));
	}

	e->env_tf.user_ip = elfhdr->e_entry;
	region_alloc(e, (void *)USTACKTOP - PGSIZE, PGSIZE);

	load_pgdir(PADDR(kern_pgdir));
}
```

env_pop_tf(*tf)负责从trapframe中恢复上下文到CPU。先从r0获取参数tf并设置为sp，然后从sp从弹出寄存器的值，最后使用subs指令将lr赋值给pc，subs末尾的s表示将spsr赋值给cpsr，实现运行模式的切换。TODO：新增trapframe值的意义，指向何处

![Untitled](3%20Trap%203b934bc433bd47f580408f70f16e7a88/Untitled%201.png)

```nasm
env_pop_tf:
    mov     sp, r0
    ldmfd   sp!, {r0-r2}
    msr     spsr, r0
    msr     sp_usr, r1
    msr     lr_usr, r2
    
    ldmfd   sp!, {r0-r12,lr}
    ldr     sp, =kernstacktop
    subs    pc, lr, #0
```

# Syscall

系统调用是用户进程主动进入内核的机制，其本质就是一段软件中断代码，在arm中使用swi指令实现。

当swi执行时，cpu会完成以下事情：

- 将swi的**下一条指令**放到`lr_svc`
- 将cpsr保存到spsr_svc
- 更改cpu模式为svc（sp等寄存器也会一起切换）
- 切换到ARM状态，并禁止IRQ
- 设置pc=0x8，为swi服务程序

xJOS中的系统调用流程如图所示，具体的实现下文详述。

![Untitled](3%20Trap%203b934bc433bd47f580408f70f16e7a88/Untitled%202.png)

为了方便设计，抽象一个系统调用接口，具体的系统调用将被封装成调用该抽象接口。该抽象接口固定使用4个参数以及一个调用号，使用r7存调用号，r0~r3存参数，调用swi进入内核，并使用r0存返回值。

```c
static inline int32_t syscall(int num, int check, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4){
	int32_t ret;

	asm volatile(
		"mov r7, %1;\
		mov r0, %2;\
		mov r1, %3;\
		mov r2, %4;\
		mov r3, %5;\
		mov r4, %6;\
		swi 1;\
		mov %0, r0;"
		: "=r"(ret)
		: "r"(num), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(0)
		: "memory", "r0", "r1", "r2", "r3", "r4", "r7");
	
	if(check && ret > 0)
		panic("syscall %d returned %d (> 0)", num, ret);
	return ret;
}
```

中断发生时，cpu将pc置为对应中断处理函数，在[0, 1M]空间。各种中断的处理函数放在中断向量（vector_table），vector_table放到内核链接地址开头。之后将虚拟地址[0, 1M]空间映射到vector_table地址，cpu即可访问到正确的处理函数。

偏移量0x8处为swi处理程序（svc_isp）。svc_isp实现将cpu上下文保存到栈中（arm没有cpu自动压栈的内容，cpu只是更换一套寄存器，压栈由软件实现），保存之后，栈顶sp即可认为trapframe内容，将sp作为参数调用svc_handler，跳转C语言处理。

```nasm
.extern svc_handler

.section ".text.boot"
vector_table:
  b       reset_handler
  b       undefined_handler_isp
  b       svc_isp
	b       undefined_handler_isp

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
```

svc_handler根据swi中断号调用对应的处理函数，现在只支持syscall，所以直接调用syscall。syscall根据系统调用号调用具体的sys_xxx服务函数。

```nasm
void svc_handler(struct Trapframe* tf){
	tf->r[0] = syscall(tf->r[7], tf->r[0], tf->r[1], tf->r[2], tf->r[3]);
	curenv->env_tf = *tf;
 
	if (curenv && curenv->env_status == ENV_RUNNING)
		env_run(curenv);
	else
		panic("no env to run");
    panic("svc_handler:should not reach here\n");
}
```

值得注意的是，在具体的syscall服务函数中，内核应对用户传入的参数进行检查。如在sys_puts中，应该检查用户是否具有该区域的读取权限，防止恶意用户打印内核区域的信息。

从内核返回到用户进程，xJOS直接使用env_run()来实现内核返回。（xJOS采用了与linux0.11不同的方案，我认为xJOS的思路会更清晰，不需要从多个文件的函数之间跳转。不过这种方案下的用户进程没有内核态）。

# User test program

首先要解决的问题是，xJOS如何获取用户程序。xJOS暂时不支持gcc编译器，也不支持文件系统，所以我将用户测试程序在linux中编译成elf之后，将整个elf链接到kernel镜像中。使用`arm-none-eabi-ld <kernel_file> -o kernel -b binary <elf_file>`工具实现，该命令将elf_file放到kernel的data段，并提供_binary_obj_xxx_start和_binary_obj_xxx_end指定elf的范围。

![Untitled](3%20Trap%203b934bc433bd47f580408f70f16e7a88/Untitled%203.png)

用户程序的结构如下：用户程序用到的程序入口和syscall是相同的，我把这部分编译为.a静态库库文件，在链接时使用。lib/包含entry.S、libmain.c、syscall.c等程序，其中，entry.S设置栈，libmain.c设置一些变量然后进入用户程序入口umain。user/编写测试程序，程序入口为umain。user.ld指定链接链接到0x800200，入口为entry.S

print和backtrace是调试用户程序的利器。

我们之前已经实现了print库函数给内核使用，同时也进行了一定的抽象，所以用户如果要使用该库，只需要提供一个打印字符串的接口putch给该库。如下所示，putch函数通过调用sys_cputs系统调用来实现字符串的打印。

```c
static void putch(int ch, struct printbuf *b){
	...
		sys_cputs(b->buf, b->idx);
	...
}
```

backtrace我们之前实现了根据ip和stab搜索函数描述信息。现在只需要加上如果是用户程序，则在用户程序的stab段搜索即可。

创建env时，将_binary_obj_xxx_start作为elf结构体起始地址，调用load_icode解析整个elf，并把ELF_PROG_LOAD类型的段加载到用户进程的虚拟空间。具体的思路和代码在上面的Env小节有给出。

```c
env_create(_binary_obj_xxx_start, type);
```

# Summary

本节实现了让user跑起来，总的来说，实现了以下步骤，

（1）内核管理envs[]：先用静态env和pgdir

（2）分配一个envs[n]，并初始化pgdir和tf

（3）为user pgdir分配所需空间，然后将elf加载到user的pgdir

（4）实现切换

              user

 env_run⬆⬇syscall

              kernel

（5）保护措施：地址访问权限
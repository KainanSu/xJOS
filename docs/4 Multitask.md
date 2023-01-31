# 4. Multitask

上一节讲了内核如何启动用户程序，以及从用户程序如何主动陷入内核。为了实现多任务，本节实现以下功能：1️⃣设计带有写时复制特性（copy on write）的fork系统调用，copy on write基于arm的data_abort异常机制。同时，编写round-robin策略的sched_yield()，用于进程调度，并提供sys_yield()系统调用给用户程序主动让出CPU。2️⃣实现从用户程序被动陷入内核（抢占式调度）。如果没有抢占式调度，用户将持续运行至自己退出，才进入内核，造成其他程序无法同时运行。抢占式调度基于arm定时器中断实现，在定时器中断处理中进行进程调度。3️⃣设计基于共享内存的进程间通信机制。

# Page fault and copy on write

ARM处理器有多种类型的异常，如

- data abort：访问非法内存
- prefetch abort：执行一个为空的函数指针、给 pc 赋非指令地址
- undefine instruction：除零、给 pc 赋未定义指令

在程序以不被允许的权限访问内存时，将触发data abort异常，如写一个没有写权限的页面，或者读一个没有读权限的页面。所以在data abort处理程序中需要实现page fault处理。

在page fault处理时，需要判断具体是由于什么原因触发，如是访问非法地址，还是写cow页面等。如果是前者，那应该将程序panic，如果是后者，那就复制页面并并让程序回到出错指令，重新运行。

思路框架如下所示：

```c
vector_table:
	...
	b data_abort_isp
	...

data_abort_isp:
	@ make trap frame
	@ jump C handler(data_abort_handler)

void data_abort_handler(tf){
	// 1.fault in kernel mode. panic
	
	// fault in user mode
	// 2.if error is write and the addr page have cow flag
	// alloc a new page and copy from the fault page
	// then unmap the origin page 
	// map the new page with read and write flags
	// 3.rerun the user fault instruction
	
}
```

为了实现以上框架，需要考虑以下技术点的实现方法：

**1、用户还是内核异常**

当发生data_abort时，处理器进入abort模式，spsr寄存器保存进入abort时cpsr。spsr寄存器中带有处理器模式标志，所以检查spsr寄存器的mode位，就可以判断是用户还是内核访问异常。

**2、访问哪个地址出错，是不是写错误，该地址所在页有没有带COW标志**

访问地址从协处理器cp15的c6获取，出错信息从cp15的c5获取。

![Untitled](4%20Multitask%20bf64846161704a0cb6f70fa3285df059/Untitled.png)

用户空间都使用二级页表（4K），如图所示，与x86不一样，armv7二级页表表项没有提供多余的bit可以用于标记COW。而一级页表表项中的P（Protect）可以用来作为COW标志。所以copy on write不以二级页表项为单位，而是以整个二级页表为单位。当发生cow异常时，复制二级页表的每一个页面。

![Untitled](4%20Multitask%20bf64846161704a0cb6f70fa3285df059/Untitled%201.png)

**3、应该返回程序的哪个指令位置**

陷入abort时，lr就存储了user模式下的pc。而armv7架构具有三级流水线（取指，译指，执行），可认为`返回地址return_pc（执行） = user_pc（取指）-8`。

![Untitled](4%20Multitask%20bf64846161704a0cb6f70fa3285df059/Untitled%202.png)

根据以上方案，copy on write核心实现如下：

```c
static void page_fault_handler(struct Trapframe *tf)
{
	in_user_mode = !(read_spsr() & 0xf);
	fault_va = read_far();
	err_info = read_dfsr();
	
	// refer to Cortex-A7 TRM page 128: Data Fault Status Register(DFSR)
	// parse err info to set err_write...

	if (!in_user_mode) {
		// ... print err info and panic.
	}

	if(!is_user_mem((void*)fault_va))
		goto user_page_fault_hander_err;

	tf->user_ip = (uint32_t)tf->user_ip - 8;

	if(err_write && (curenv->env_pgdir[L1X(fault_va)] & L1_TABLE_COW)){
		r = copy_on_write_handler((void*)fault_va);
	}

user_page_fault_hander_err:
	// ... print err info and destroy env.
}

// Travers L2，copy every page in L2 to new page.
// map new page to L2, with RW flag
// unmap origin page
static int copy_on_write_handler(void* addr)
{
	l2e_t* l2table = (l2e_t*)(KADDR(L1TABLE(curenv->env_pgdir[L1X(addr)])));
	
	for(int i = 0; i < L2NENTRIES; i++) {
		if(l2table[i] & L2_EXIST) {
			assert(perm_have(l2table[i], L2_TYPE_SMALL));
			r = env_page_alloc(envid, (void *)PFTEMP, L2_SMALL_AP_URW_SRW);
			addr = ROUNDDOWN(addr, PGSIZE);
			memmove(PFTEMP, addr, PGSIZE);
			r = env_page_unmap(envid, addr);
			r = env_page_map(envid, PFTEMP, envid, addr, L2_SMALL_AP_URW_SRW);
			r = env_page_unmap(envid, PFTEMP);
		}
	}

	update_l1entry(curenv->env_pgdir, addr, L1_TYPE_TABLE);

	return 0;
}
```

# Fork and yield

创建多进程并开始调度。

sched_yield()调度使用了轮询策略，轮询所有进程，找到状态为ENV_RUNNABLE的进程并运行。

```c
void sched_yield(void)
{
	envid_t i, nowid = curenv ? curenv->env_id : 0;

	for(i = 0; i < NENV; i++) {
		struct Env *nextenv = envs + ((nowid + i) % NENV);
		if(nextenv->env_status == ENV_RUNNABLE){
			env_run(nextenv); // no return
		}
	}
	if(curenv && curenv->env_status == ENV_RUNNING)
		env_run(curenv);

	panic("no environment to run.");
}
```

同时，封装sched_yield()为sys_yield()系统调用，提供进程主动让出CPU的接口。

fork系统调用实现的功能与Unix一致，复制父进程的信息（Trapframe与内存），子进程返回0，父进程返回子进程id。

![Untitled](4%20Multitask%20bf64846161704a0cb6f70fa3285df059/Untitled%203.png)

```c
// 1.copy parent trapframe
// 2.(implement copy on write)
// scan L2, for every entry, 
// if is read only, map directly to child;
// if is read & write, map to child and remap parent with read permission.
// 3.set L1 entry with COW if one or more write page in L2 table.
static int sys_fork(void){
	if((r = env_alloc(&e, curenv->env_id)) < 0)
		return r;
	e->env_status = ENV_NOT_RUNNABLE;
	memcpy(&e->env_tf, &curenv->env_tf, sizeof(e->env_tf));
	e->env_tf.r[0] = 0;
	
	for(int i = L1X(UTEXT); i < L1X(UTOP); i++){
		if(curenv->env_pgdir[i] & L1_EXIST){

			assert(perm_have(curenv->env_pgdir[i], L1_TYPE_TABLE));
			int l2_have_write = 0;
			l2e_t* l2e = (l2e_t*)KADDR(L1TABLE(curenv->env_pgdir[i]));

			for(int j = 0; j < L2NENTRIES; j++){
				void* addr = LADDR(i, j, 0);
				if(l2e[j] & L2_EXIST){
					assert(perm_have(l2e[j], L2_TYPE_SMALL));
					if(perm_have(l2e[j], L2_SMALL_AP_URW_SRW)){
						r = env_page_map(curenv->env_id, addr, e->env_id, addr, L2_SMALL_AP_URO_SRW);
						r = env_page_map(curenv->env_id, addr, curenv->env_id, addr, L2_SMALL_AP_URO_SRW);
						l2_have_write = 1;
					}else{
						r = env_page_map(curenv->env_id, addr, e->env_id, addr, L2_SMALL_AP_URO_SRW);
					}
				}
			}
			if(l2_have_write){
				void* addr = LADDR(i, 0, 0);
				r = update_l1entry(curenv->env_pgdir, addr, L1_TYPE_TABLE | L1_TABLE_COW);
				r = update_l1entry(e->env_pgdir, addr, L1_TYPE_TABLE | L1_TABLE_COW);
			}
		}
	}

	e->env_status = ENV_RUNNABLE;

	return e->env_id;
}
```

# Preemptive

抢占式调度基于epit定时器实现，需要配置epit中断和epit时钟。

imx6的中断由Generic Interrupt Controller（GIC）管理，GIC包含Distributor和CPU Interface两个组件，前者管理所有的中断组（SPI、PPI、SGI）的所有中断号（ID=0~1019），后者与CPU通信完成GIC的读写。SPI是共享中断，包含来自GPIO、EPIT等外设的中断；PPI是每个CPU独立的中断；SGI是软件中断，用于多核通信。

![Untitled](4%20Multitask%20bf64846161704a0cb6f70fa3285df059/Untitled%204.png)

查阅imx6的时钟树，可知epit的时钟源为PLL3产生的ipg_clk，为66M，设置epit的分频与计数，即可产生周期中断。

学习linux简化中断处理程序，比如epit irq调用了no return的sched_yield()，不会返回原来的位置（xJOS仅使用一个内核栈，而不是每个进程一个内核栈），所以禁止了中断嵌套的情况。防止irq嵌套的实现其实很简单，设置只有在user模式才会进入irq：打开user模式cpsr的irq，（自动）关闭irq模式cpsr的irq，（自动）关闭svc模式的irq。

```c
void irq_handler(struct Trapframe *tf)
{
	uint32_t irq_num = read_irq_num();
	if(curenv){
		curenv->env_tf = *tf;
		tf = &curenv->env_tf;
		curenv->env_tf.user_ip -= 4;
	}

	switch(irq_num){
		case 88:
			write_irq_eoir(irq_num);
			((EPIT_Type *)(AIPS1BASE+EPIT1_OFFSET))->SR |= 1<<0;
			sched_yield();
		break;

		default:
		break;
	}

	panic("known irq num: %d\n", irq_num);
}
```

# Ipc

![Untitled](4%20Multitask%20bf64846161704a0cb6f70fa3285df059/Untitled%205.png)

进程间通信通过sys_ipc_send()和sys_ipc_recv()系统调用，通信消息包含form、to、value、data（page），其中value用于传递一个变量，data用于传递多个变量。data传递基于共享内存机制，可以提高数据通信效率。

sys_ipc_recv()是阻塞的，只有接收到消息之后才会返回。sys_ipc_send()只有在接收端等待接收时（即调用sys_ipc_recv），才能发送成功。

```c
static int sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
{
	int r;
	struct Env *e;
	if((r = envid2env(envid, &e, 0)) < 0)
		return -E_BAD_ENV;

	if(e->env_ipc_recving != true || e->env_ipc_from != 0)
		return -E_IPC_NOT_RECV;
	
	if(is_user_mem(srcva)){
		if((uint32_t)srcva % PGSIZE != 0)
			return -E_INVAL;
		if ((perm & L2_EXIST) == 0 || !perm_have_user(perm))
			return -E_INVAL;
		if ((perm & ~L2_SMALL_FOR_USER) != 0)
			return -E_INVAL;
		if(e->env_ipc_dstva == 0)
			return -E_INVAL;
		l2e_t *pte;
		struct PageInfo *pi;
		if((pi = page_lookup(curenv->env_pgdir, srcva, &pte)) == NULL)
			return -E_INVAL;
		if(perm_have(perm, L2_SMALL_AP_URW_SRW) && !perm_have(*pte, L2_SMALL_AP_URW_SRW))
			return -E_INVAL;
		if((r = page_insert(e->env_pgdir, pi, e->env_ipc_dstva, perm)) < 0)
			return r;

		e->env_ipc_perm = perm;
	}
	e->env_ipc_from = curenv->env_id;
	e->env_ipc_value = value;
	e->env_ipc_recving = false;
	e->env_status = ENV_RUNNABLE;
	e->env_tf.r[0] = 0;

	return 0;
}
```
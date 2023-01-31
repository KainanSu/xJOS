# 1. Boot imx6 in qemu

# QEMU

仿真环境提供了很多方便和调试工具，所以我选择先用qemu搭建实验环境，之后再放到物理机上运行。考虑到虚拟机和物理机之间程序入口的兼容性，需要先搞清楚开发板的启动方式和qemu的启动方式有什么不一样。

物理机有多种启动方式。比如当选择从SD卡启动时，imx6u内部的boot rom程序，会从SD卡中拷贝镜像到**链接地址处**，然后从链接地址处开始执行。链接地址可以是imx内部128KB RAM（0X90 0000~0X91 FFFF）或者外部DDR（0X8000 0000）中。程序放到SD卡的哪个位置也是有规定的，每个芯片不一样，所以要用专门的工具把程序放到SD中。在存放到SD前，需要在bin文件的基础上加上一些规范：如Boot data，包含了bin需要拷贝到哪个地址、长度是多少。

qemu相当于不需要指定imx的启动方式，跳过了imx从SD卡加载img的过程，image会直接存放到DDR中（**编译时指定的加载地址**）。如果用真机做实验，还需要将image集成到SD卡。因为真机实验还需要在前面加入头部数据，所以在用qemu做实验时，image的链接地址最好别是RAM的0地址，否则前面没位置存放头部数据了，放到真机实验时还得修改链接地址。我选择链接地址为0x8100 0000（RAM的16M处）。

由于imx的boot rom相当于BootLoader，所以我们的os代码不需要实现多级启动（实现BootLoader放到磁盘第一个块。启动时rom读入BootLoader，BootLoader从磁盘搬运os代码到内存），而可以直接实现os部分。

![Untitled](1%20Boot%20imx6%20in%20qemu%202ed0d47530dc4446b4740f0207da3cd6/Untitled.png)

# Boot

在开启MMU之前的代码必须是地址无关的，所在在跳转C函数前，需要开启做好内存映射，将os的地址映射为物理RAM地址，这部分工作放在entry.S中实现。

我们设置entry.S中的entry作为程序的入口，当image加载到DDR后，将从entry开始执行。entry.S中主要完成以下三件事：把自身bss段清零；加载页表，使能MMU；设置堆栈并进入C环境。

其他两个比较简单，这里说一下如何构建和加载页表。

内核镜像的链接地址（运行地址）为0xF010 0000，在芯片上电后被加载到0x8010 0000。加载地址与RAM起始地址（DDRBASE）中间还有1M的空间，是预留给物理机imx加载格式的区域。为了使内核能够在（虚拟空间的）链接地址运行，需要映射虚拟地址中KERNBASE以上16M的空间到物理RAM的起始+16M（允许jump到链接地址），同时也需要映射虚拟地址物理RAM的起始+16M到该物理地址（让当前的指令地址还是能翻译到正确位置）。当然，后者只是暂时的映射，在后面正式的页表中不会出现（在jump到链接地址后就可以去掉了）。

![Untitled](1%20Boot%20imx6%20in%20qemu%202ed0d47530dc4446b4740f0207da3cd6/Untitled%201.png)

与x86使用页目录表和页表组成的二级翻译形式不同，ARM分为一级页表和二级页表，两级页表都可以映射到页框。一级页表映射1M页框，二级页表可以映射4K和16K页框。

启动时的页表entry_pgdir手动构造为一级页表（映射1M页框）。构建entry_pgdir就是在数组中，按照格式写入页表项。一级页表的直接映射表项格式如下所示，我们暂时不需要太多功能，只需要设置Section Base Address为物理基地址，以及末两位为10表示这是一级直接映射表项。

![Untitled](1%20Boot%20imx6%20in%20qemu%202ed0d47530dc4446b4740f0207da3cd6/Untitled%202.png)

```c
pde_t entry_pgdir[NPDENTRIES] __attribute__((aligned(16 * 1024))) = {
    // va [PHYBASE, PHYBASE + 16M] to pa [PHYBASE, PHYBASE + 16M]
    [(PHYBASE >> 20) + 0] = (PHYBASE + 0x100000 * 0) | 2,
	...
    [(PHYBASE >> 20) + 15] = (PHYBASE + 0x100000 * 15) | 2,

    // va [KERNBASE, KERNBASE + 16M] to pa [PHYBASE, PHYBASE + 16M]
    [(KERNBASE >> 20) + 0] = (PHYBASE + 0x100000 * 0) | 2, 
	...
    [(KERNBASE >> 20) + 15] = (PHYBASE + 0x100000 * 15) | 2,
};
```

ARM内存管理主要由协处理器CP15完成，主要用到C1、C2、C3寄存器。

- C1（Control Register）：bit0是MMU的enable位
- C2（Translation Table Base Register）（TTBR）：用于存储一级页表的基址，与x86的CR3功能类似。由于一级页表是16KB（4*4096项），所以基址需要16KB对齐。
- C3（Domain Access Control Register）：保存16个域状态，每个域的权限不一样。每个虚拟页都属于一个域。00不可访问，01访问需检查权限，11（Manager）无需检查权限

在启动代码中，将C2寄存器设置为entry_pgdir；再设置C3，将域都设置为Manager；最后设置C1，使能MMU。

```c
ldr r0, =(entry_pgdir - KERNBASE + PHYBASE)
mcr p15, 0, r0, c2, c0, 0

mov r0, #0xFFFFFFFF
mcr p15, 0, r0, c3, c0, 0

mrc p15, 0, r0, c1, c0, 0
orr r0, r0, #0x1
mcr p15, 0, r0, c1, c0, 0
```

# Print

print对内核调试非常有帮助，所以进入C语言后，首先要做的就是使能print。

我希望print库能支持串口、键盘和VGA等IO设备作为控制台，所以定义了三层结构来封装不同层级的输入输出，保证软件的复用率。首先最上层提供cprintf和readline接口，中间层是与设备无关的控制台缓冲区管理，最底层是硬件设备的单字符输入输出。

![Untitled](1%20Boot%20imx6%20in%20qemu%202ed0d47530dc4446b4740f0207da3cd6/Untitled%203.png)

目前的print库设置了串口作为底层IO接口。输出的流程如下：cprintf()将fmt和args格式化为buf，然后调用cputchar输出每个字符；cputchar只是作为连接两层的接口，内部是直接调用cons_putc；cons_putc调用uart_putc输出。输入流程如下：当uart接收到数据时，调用cons_getc输入到控制台缓冲区，上层通过轮询调用readline来获取缓冲区中的每一行。

底层串口设备的初始化按照裸机的配置方式，即对设备寄存器的读写进行配置。通过查找《i.MX 6ULL Applications Processor Reference Manual》的Chapter 2 Memory Maps和Chapter 55 UART，获取uart的映射地址和寄存器配置信息。

我想要使用imx提供的寄存器宏定义库来初始化串口，由于这些宏定义使用的是寄存器的物理地址，所以在entry_pgdir中需要将这些物理地址映射到与本身相同的虚拟地址。而在初始化完成后，不需要再使用这些宏定义，同时也方便管理虚拟内存（使用处于高地址的APISBASE来访问设备，低地址是用户进程区域），所以将APISBASE映射到PHY_APISBASE。更新页表如下：

```c
pde_t entry_pgdir[NPDENTRIES] __attribute__((aligned(16 * 1024))) = {
  [APISBASE >> 20] = PHY_APISBASE | 2,
	...
	[PHY_APISBASE >> 20] = PHY_APISBASE | 2,
}
```

具体的映射初始化过程可以参考裸机的配置过程。

# Backtrace

在内核或测试程序出错时，我们常希望能够backtrace，打印函数的调用过程。打印的原理是利用栈帧中存放的返回地址信息，根据这个地址在调试信息（.stab段）查找该返回地址是在哪个函数的第几行；然后解析栈帧链即可获取整个调用过程。

编译生成的调试信息主要包括stab段和stabstr段。如图所示，stab段是一个结构体数组，包含一个地址（n_value）对应的描述信息（n_strx），描述信息是该信息在stabstr段中的偏移量。stab段结构体数组的排列是多个src file按（地址）顺序排列，每个src file又按顺序排列func。具体的调试信息可以通过`arm-none-eabi-objdump -G kernel > kernel.stab`导出查看。

![Untitled](1%20Boot%20imx6%20in%20qemu%202ed0d47530dc4446b4740f0207da3cd6/Untitled%204.png)

为了解析一个地址所处的函数位置，通过二分搜索stab段的n_value（addr），接着搜索左右边界得到所在的src file的界限，继续在界限之内内部搜索func、line。最终得到详细的描述信息。

![Untitled](1%20Boot%20imx6%20in%20qemu%202ed0d47530dc4446b4740f0207da3cd6/Untitled%205.png)

arm的函数调用过程和栈帧结构

每个函数的汇编代码，都会对栈帧进行以下操作：1️⃣压入lr和fp。在bl、call调用函数时，返回地址会被设置到lr寄存器（所以如果不再调用子函数，则可以不用压入lr），fp是上一个栈帧的起始位置。2️⃣开辟位置存储局部变量，包括将r0~r3参数压入栈（没有开优化时）3️⃣代码运行到return时，返回值将放入r0，然后关闭所使用的局部变量空间，最后弹出fp和lr，通过`bl lr`指令返回父函数。

所以通过解析fp链即可得到整个调用过程。

通过inline内嵌汇编来获取r11的值。值得注意的是，该inline函数的编译属性需要配置为`always_inline`，否则在没有打开优化编译时，该函数可能不会展开为内联函数，而是作为普通函数调用，此时r11并非期望的值。

```c
static inline __attribute__((always_inline)) uint32_t read_r11(void)
{
	uint32_t r11;
	asm volatile("mov %0, r11" : "=r" (r11));
	return r11;
}
```

# Material

[An ebook about bare-metal programming for ARM](https://github.com/umanovskis/baremetal-arm)：这本书讲了如何从零开始编译链接代码，并在qemu上运行。

[Ireneruru/JOS-on-Arm](https://github.com/Ireneruru/JOS-on-Arm)：这个是北大的作业，将MIT x86的实验迁移到arm上，里面有个pdf总结，写得挺好的。

《i.MX 6ULL Applications Processor Reference Manual》
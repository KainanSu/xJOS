# 5. Device & File System

device

serv（opentab共享fd（其实可以不共享的））文件系统，FS等数据结构（indirect），bc（user-level），读写SD卡，

exec：spawn，console getchar，dup，share。

shell=fork+spawn+传参args从栈中取值

## Device

学习linux处理设备驱动的思路，xJOS也将设备视为文件，为控制台、文件服务和网络服务等设备都提供同一套文件操作接口（read、write和seek等），增删设备就简单很多。

![Untitled](5%20Device%20&%20File%20System%204d38ce31789d4f2a9af3257ef8429281/Untitled.png)

设备驱动代码框架如下：1️⃣实现lib/fd.c，提供统一read接口，并转发到具体dev（dev_lookup）；2️⃣为每一个设备写一个c文件，提供操作函数（devfile_read）并定义Dev（devfile）；3️⃣将Dev添加到devtab中。

以读取文件为例，用户层使用文件描述符fd调用库函数read(fd)，read函数将使用dev_lookup匹配fd→dev_id和设备表devtab中每一项的dev_id，找到对应devfile之后，调用devfile→read()接口，实现文件读取。

```c
// lib/fd.c
ssize_t read(int fdnum, void *buf, size_t n){
	...
	if ((r = fd_lookup(fdnum, &fd)) < 0
	    || (r = dev_lookup(fd->fd_dev_id, &dev)) < 0)
		return r;
	if ((fd->fd_omode & O_ACCMODE) == O_WRONLY) {
		cprintf("[%08x] read %d -- bad mode\n", thisenv->env_id, fdnum);
		return -E_INVAL;
	}
	if (!dev->dev_read)
		return -E_NOT_SUPP;
	return (*dev->dev_read)(fd, buf, n);
}

int dev_lookup(int dev_id, struct Dev **dev){
	for (int i = 0; devtab[i]; i++)
		if (devtab[i]->dev_id == dev_id) {
			*dev = devtab[i];
			return 0;
		}
	log_err("[%08x] unknown device type %d\n", thisenv->env_id, dev_id);
	*dev = 0;
	return -E_INVAL;
}

// lib/file.c
struct Dev devfile =
{
	.dev_id =	'f',
	.dev_name =	"file",
	.dev_read =	devfile_read,
	.dev_close =	devfile_flush,
	.dev_stat =	devfile_stat,
	.dev_write =	devfile_write,
};

// lib/fd.c
static struct Dev *devtab[] ={
	&devcons,
	&devfile,
	0
};
```

# File Server

前面设备框架已经将read转发到devfile_read，下面就讲讲xJOS中devfile_read的设计框架。与linux不同，xJOS采用微内核策略，故将文件系统作为用户进程（以下将该进程成为称为文件服务File Server）运行。

将FS作为用户进程和作为内核的区别主要有以下两点：1️⃣ 常规进程在访问文件系统时，不是进入到内核，而是跨进程调用（RPC）；2️⃣需要内核开放存储设备（sd卡）读取权限给FS。

File Server的设计框架如图所示。实现1️⃣，可以基于前面设计的IPC通信机制，设计FS和进程的通信机制（**RPC**）。实现2️⃣，需要将外设地址映射到FS虚拟空间，并写一套SD卡读写程序（**usdhc总线驱动和sd卡驱动**）。3️⃣进一步地，访问sd卡速度远低于访问内存，所以在FS中应该实现缓存策略（**Block CaChe**）。

![Untitled](5%20Device%20&%20File%20System%204d38ce31789d4f2a9af3257ef8429281/Untitled%201.png)

## SD

![Untitled](5%20Device%20&%20File%20System%204d38ce31789d4f2a9af3257ef8429281/Untitled%202.png)

qemu中模拟的imx开发板，TF卡使用usdhc协议通信，插入TF卡使用`-sd fs.img`选项。

![Untitled](5%20Device%20&%20File%20System%204d38ce31789d4f2a9af3257ef8429281/Untitled%203.png)

imx通过usdhc（Ultra Secured Digital Host Controller）访问TF卡。其中CMD线用于发送命令，主从机通信的所有命令在CMD线上，SDIO协议定义了63个命令，一个命令跟着一个响应；而数据都走DATA[0,7]数据线。

imx的usdhc协议支持ADMA通信，ADMA和SDMA的区别是ADMA支持描述符队列，而不是只能传输一次。xJOS就使用了ADMA机制读写TF卡，一次为512字节。

为了让File Server能够操作usdhc总线，将usdhc映射到FS的FS_SD_BASE虚拟地址处。

## File System

sd卡文件系统如下所示，super block存了文件系统的分区信息，如free bitmap占了多少block，data blocks从哪里开始。xJOS中没有inode索引的概念，data block中直接存储了File信息（即linux中的inode），或者文件数据data，为了查找文件夹中的文件，只需要遍历文件夹对应的block中的direct[10]和indirect，比对name，即可获取文件夹中的文件。

sd卡sector为512字节，block为4096字节（与PGSIZE相同），File结构体为512字节。

![Untitled](5%20Device%20&%20File%20System%204d38ce31789d4f2a9af3257ef8429281/Untitled%204.png)

为了访问文件系统上的文件，有两个重要的函数：dir_lookup、block_walk和file_getblock。walk_path给定一个dir和file，找到dir中file对应的FIle结构体；file_block_walk后者给定文件偏移量，获取磁盘上的block；file_getblock将文件偏移量转换为缓冲区（block cache）的位置，从而实现在内存中读写文件，读写完毕之后再flush进sd卡。

其中，file_block_walk实现的思路与内存管理中的pgdir_walk相似（根据va获取二级页表项）。如果偏移量在direc[10]中，则返回对应数组项；如果是indirect，则再计算一层，如果block num为0，说明还未分配block，则进行分配。

![Untitled](5%20Device%20&%20File%20System%204d38ce31789d4f2a9af3257ef8429281/Untitled%205.png)

```c
static int dir_lookup(struct File *dir, const char *name, struct File **file){
	...
	nblock = dir->f_size / BLKSIZE;
	for (i = 0; i < nblock; i++) {
		r = file_get_block(dir, i, &blk)) < 0
		f = (struct File*) blk;
		for (j = 0; j < BLKFILES; j++)
			if (strcmp(f[j].f_name, name) == 0) {
				*file = &f[j];
				return 0;
			}
	}
	return -E_NOT_FOUND;
}

int file_get_block(struct File *f, uint32_t filebno, char **blk){
	...
	if(filebno >= NDIRECT + NINDIRECT)
		return -E_INVAL;

	if((r = file_block_walk(f, filebno, &entry, true)) < 0)
		return r;
	if(*entry == 0){
		r = alloc_block()) < 0
		*entry = r;
		memset(diskaddr(*entry), 0, BLKSIZE);
		flush_block(diskaddr(*entry));
	}
	if(blk)
		*blk = diskaddr(*entry);

	return 0;
}

int file_block_walk(struct File *f, uint32_t filebno, uint32_t **ppdiskbno, bool alloc){
	int r;
	if(filebno >= NDIRECT + NINDIRECT)
		return -E_INVAL;
	
	if(filebno < NDIRECT){
		if(ppdiskbno)
			*ppdiskbno = &f->f_direct[filebno];
		return 0;
	}
	filebno -= NDIRECT;
	if(filebno < NINDIRECT){
		if(f->f_indirect == 0 && alloc == false)
			return -E_NOT_FOUND;
		if(f->f_indirect == 0){
			if((r = alloc_block()) < 0)
				return r;
			f->f_indirect = r;
			memset(diskaddr(r), 0, BLKSIZE);
			flush_block(diskaddr(r));
		}
		void *addr = diskaddr(f->f_indirect);
		if(ppdiskbno)
			*ppdiskbno = &((uint32_t*)addr)[filebno];
		return 0;
	}

	return 0;
}

```

## File Server RPC

FS RPC框架如图所示。RPC机制基于进程间通信（IPC），xJOS的IPC是基于共享内存实现的，收发请求、传输数据都通过共享的页面实现。

![Untitled](5%20Device%20&%20File%20System%204d38ce31789d4f2a9af3257ef8429281/Untitled%206.png)

每次请求时，用户将请求的相关信息（如是open/read/write，以及write的数据等）放到共享页面中，然后通过ipc_send发送到FS。发送完请求之后，调用对应的recv原地等待FS返回。当FS接收到请求后，分析请求并调用对应的服务函数，如需回传数据（read）则读取block cache，然后将数据放到共享的页面中回传。

![Untitled](5%20Device%20&%20File%20System%204d38ce31789d4f2a9af3257ef8429281/Untitled%207.png)

通过Union和Struct来封装不同请求，使代码更整洁。union Fsipc即为每次RPC传输的页面。

```c
union Fsipc {
	struct Fsreq_read {
		int req_fileid;
		size_t req_n;
	} read;
	struct Fsret_read {
		char ret_buf[PGSIZE];
	} readRet;
	struct Fsreq_write {
		int req_fileid;
		size_t req_n;
		char req_buf[PGSIZE - (sizeof(int) + sizeof(size_t))];
	} write;
	...
	// Ensure Fsipc is one page
	char _pad[PGSIZE];
};
```

## Block Cache

File server实现block cache的策略是，将sd卡空间映射到虚拟空间[DISKMAP, DISKMAP + DISKSIZE]共3G空间，这样线性映射使数据位置转换、读写都很方便。

Block Cache读入SD卡数据通过user-level的页面错误处理进行。当读取一个不存在于内存中的block时，将触发page fault，在user-level的handler中将sd卡数据读入到对应内存空间。

flush内存时，将该区域sys_unmap_page，从而释放缓冲区。

[https://www.notion.so](https://www.notion.so)

## User-level page fault handler

如何提供用户层的页错误处理。简单点想，kernel提供一个注册handler的接口，当发生页错误中断时，kernel让user去调用handler。这涉及到三个关键问题，1️⃣kernel如何让user暂停当前的执行序列，转而先执行handler；2️⃣user执行完handler后，如何恢复原来的执行序列；3️⃣如何恢复原来的状态，即如何保存和恢复CPU寄存器。

1️⃣2️⃣与hw6 cpu alarm那个作业的做法是一样的。把tf→eip压栈，然后把tf→eip赋值为handler，接着run_env()。这样执行完handler，ret时弹出tf→eip执行，原来的执行序列得以恢复。

3️⃣这也是hw6中的Challenge：在handler前后保存和恢复CPU状态。不过这个lab还要求我们将新的异常栈作为错误处理时使用栈，使用原来的栈也是可以实现的，只不过用异常栈可以使栈的功能单一，方便调试。

那如何保存恢复CPU状态呢，这就要求在handler执行前后插入一段代码，可以通过提供一个统一的页错误处理函数（_pgfault_upcall），注册到kernel的是_pgfault_upcall，而户注册的处理函数handler是放到_pgfault_upcall中运行。

我们要使用异常栈来保存CPU状态，所以我们在kernel页错误处理时，应该把CPU信息复制到异常栈中（UTrapframe），此时utf的eip和esp指向了原来的状态。为了切换用户的栈和执行程序，修改tf的esp和eip并run_env。为了在执行完handler之后，恢复执行位置，应该保证在ret时，pop的是原来的执行位置，需要通过在栈顶压入utf→eip。

那么，栈顶是运行栈的栈顶吗？其实不一定，考虑异常递归的情况，此时是在异常栈之中。还好，不论是其中哪种情况，utf→esp指向的就是那个栈。所以我们只要在utf→esp位置压入一个utf→eip就可以了。不过，当异常递归时，我们使用的utf就是放在异常栈中的，也就是utf是在utf→esp下方的，如果在utf→esp压入，那就覆盖了我们的异常栈了。所以，这就需要特殊处理了，我们在kernel页错误处理函数中，如果是异常递归，那就在utf
# 2. Memory management

内存管理分为虚拟地址映射和物理内存管理两部分。在内核设计中，利用了arm提供的多级页表：使用了一级页表的section（1M）映射KERNBASE以上的虚拟空间，来提高TLB的命中率，而剩下的虚拟空间均使用二级页表的small page（4K）映射地址，同时将物理内存划分为4K页进行管理。

为了实现这个目的，本节主要完成三件事：物理页的管理，二级页表映射管理，内核页表管理。1️⃣使用pages结构体数组管理RAM内存，将内存划分成4K页进行管理，每个pages[n]对应一个4K物理页，且是线性一一对应关系。同时提供一组分配与释放pages的方法。2️⃣二级页表映射管理提供了一组修改kern_pgdir**二级页表**的方法，主要包括将根据va查找二级页表项的地址（pgdir_walk），将va映射到pages[n]（page_insert）、取消映射（page_remove），获取va映射的page（page_lookup）。3️⃣内核页表KERNBASE以上的部分与entry_pgdir一致，使用一级页表以1M为单位，线性映射DDR内存。[UTOP, KERNBASE]区域提供方法boot_map_region，使用二级页表以4K单位进行映射之后的UENVS等变量。

# Pages

pages数组管理RAM内存，归宿是被映射到二级页表的表项。

pages数组的类型是是PageInfo结构体，如下所示。PageInfo中的pp_link指针用于连接free_page_list；pp_ref为物理页被映射到va的次数，当映射次数为0时，该物理页将放回free_page_list管理。

```c
struct PageInfo {
	struct PageInfo *pp_link;
	uint16_t pp_ref;
};
```

物理内存被划分成4K大小的物理页，pages[N]数组与物理页线性一一对应。free_page_list将空闲的page串成链表，在分配page时从头部分配，删除时放回头部。

分配page时，一般都需要将page映射到二级页表（使用二级页表管理函数）。在管理函数中需要先将page转换为物理地址，使用page2pa()函数。该函数利用物理页与page线性对应关系，将page转换成物理地址，与permision拼接之后生成二级页表表项。

![Untitled](2%20Memory%20management%20a3ede1f30ae54e24a314d622334ccdd8/Untitled.png)

同时提供了一组分配与释放pages的方法，这些方法只是简单的链表增删操作，下面就只给出page_alloc函数的实现，其他函数大同小异。

```c
struct PageInfo *page_alloc(int alloc_flags){
	if(page_free_list == NULL)
		return NULL;
	struct PageInfo *result;

	result = page_free_list;
	page_free_list = page_free_list->pp_link;
	result->pp_link = NULL;
	if(alloc_flags & ALLOC_ZERO){
		memset(page2kva(result), 0, PGSIZE);
	}

	return result;
}
```

# Level2 page table

ARM的一级页表可以映射1M区域，二级页表映射4K区域。除了KERNBASE以上的虚拟地址使用一级页表映射，其他区域都将使用二级页表映射。

二级页表映射管理提供了一组修改kern_pgdir**二级页表**的方法，主要包括将根据va查找二级页表项地址（pgdir_walk），将va映射到pages[n]（page_insert）、取消映射（page_remove），获取va映射的page（page_lookup）。

pgdir_walk是实现其他几个方法的基础，完成va到二级页表项地址的转换函数，需要理解一级、二级页表的翻译过程才能实现。

![Untitled](2%20Memory%20management%20a3ede1f30ae54e24a314d622334ccdd8/Untitled%201.png)

给定一个虚拟地址va，L1X(va)获取对应一级页表L1中页表项的index。当该页表项指向二级页表时，页表项的内容如下所示：

![Untitled](2%20Memory%20management%20a3ede1f30ae54e24a314d622334ccdd8/Untitled%202.png)

使用L1ENTRY(*l1e)取出页表项中指向二级页表的物理地址（Level 2 Descriptor Base Address），结合va中二级页号的index（使用L2X(va)获取），就可以得到二级页表项的地址。

```c
l2e_t *pgdir_walk(l1e_t *pgdir, const void *va, int create){
    l1e_t *pde, *pt, *pte;
		uintptr_t ret_va;
    assert(L1X(va) < L1NENTRIES);
    if (!(pgdir[L1X(va)] & L1_EXIST)) {
        if (!create) 
            return NULL;
        struct PageInfo * new_page = page_alloc(ALLOC_ZERO);
        if(new_page == NULL) 
            return NULL;
        new_page->pp_ref++;
        pgdir[L1X(va)] = page2pa(new_page) | L1_TYPE_TABLE;
    }

    l2e_t *pgtbl = (l2e_t*)KADDR(L1ENTRY(pgdir[L1X(va)]));
    return &pgtbl[L2X(va)];
}
```

增删查改方法的实现其实都是使用二级页表地址进行操作，这里给出page_insert的实现，其他方法大同小异。

```c
int page_insert(l1e_t *pgdir, struct PageInfo *pp, void *va, int perm){
	l2e_t *pte;
	pte = pgdir_walk(pgdir, va, 1);
	if(pte == NULL)
		return -E_NO_MEM;
	pp->pp_ref++;
	if(*pte & L2_EXIST)
		page_remove(pgdir, va);
	*pte = page2pa(pp) | L2_TYPE_SMALL | perm;
	tlb_invalidate(pgdir, va);

	return 0;
}
```

page_insert和page_remove操作都涉及到页表的更新，由于Translation Lookaside Buffer（TLB）的存在，当更新页表项时，应使映射的虚拟地址对应的TLB项失效

```c
static void tlb_invalidate(void *va){
    asm("mcr p15, 0, %0, c8, c7, 1":: "r"(va):);
}
```

# Kern_pgdir

期望的虚拟空间分布如下所示

为了实现这个空间映射，内核页表KERNBASE以上的部分与entry_pgdir一致，使用一级页表以1M为单位，线性映射DDR内存。[UTOP, KERNBASE]区域提供方法boot_map_region（该函数封装映射多个4K页面的操作），使用二级页表以4K单位映射之后的UPAGES、UENVS等空间。

![Untitled](2%20Memory%20management%20a3ede1f30ae54e24a314d622334ccdd8/Untitled%203.png)

将DDR内存（256M）线性映射至[KERNBASE, 4G]区域，方便之后page、pa和kva的互相转换。

```c
KSTKSIZE = 8 * 4K
KSTKGAP  = 8 * 4K
NORMALSIZE = 4M
```

# Convert

内存管理包含了多个变量：physical address（pa），kernel virtual address（kva），pages（pg），page num（pages数组的下标）（pgn），它们之间的转换关系如图所示：

![Untitled](2%20Memory%20management%20a3ede1f30ae54e24a314d622334ccdd8/Untitled%204.png)

下面给出部分代码，具体说明它们之间线性映射及线性映射提供的方便。

当我们获取一个page（地址为kva），想将它作为二级页表页面，并插入一级页表表项（填入pa），此时就需要kva转pa。内核将[DDRBASE, DDRBASE+256M]的RAM线性映射到虚拟空间[KERNBASE, 4G]。在kva转pa时，只需要简单的偏移就可以得到。可以设想一下，如果不是线性映射关系，还需要多一张表来存放kva和pa的转换关系。

```c
kva转pa
static inline physaddr_t _paddr(const char *file, int line, void *kva){
	if ((uint32_t)kva < KERNBASE)
		_panic(file, line, "PADDR called with invalid kva %08lx", kva);
	return (physaddr_t)kva - KERNBASE + DDRBASE;
}
```

一个pages[n]映射一个4K物理页面，所以想从pages[n]转物理地址，只需要将下标*4K再加上RAM的偏移即可。

```c
page转pa
static inline physaddr_t page2pa(struct PageInfo *pp){
	return ((pp - pages) << PGSHIFT) + DDRBASE;
}
```

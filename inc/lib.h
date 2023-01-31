// Main public header file for our user-land support library,
// whose code lives in the lib directory.
// This library is roughly our OS's version of a standard C library,
// and is intended to be linked into all user-mode applications
// (NOT the kernel or boot loader).

#ifndef JOS_INC_LIB_H
#define JOS_INC_LIB_H 1

#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/stdarg.h>
#include <inc/string.h>
#include <inc/error.h>
#include <inc/assert.h>
#include <inc/env.h>
#include <inc/log.h>
#include <inc/fd.h>
#include <inc/syscall.h>
#include <inc/args.h>

extern const char *binaryname;
extern const volatile struct Env *thisenv;
extern const volatile struct Env envs[NENV];
extern const volatile struct PageInfo pages[];

void sys_cputs(const char *string, size_t len);
void sys_panic(int info);
void sys_yield(void);
int sys_fork(void);
int sys_getenvid(void);
int sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, int perm);
int sys_ipc_recv(void *dstva);
int	sys_page_alloc(envid_t env, void *va, int perm);
int	sys_page_map(envid_t src_env, void *src_pg,
		     envid_t dst_env, void *dst_pg, int perm);
int	sys_page_unmap(envid_t env, void *pg);
int sys_env_set_pgfault_upcall(envid_t envid, void *upcall);
int sys_env_destroy(envid_t envid);
l1e_t sys_get_l1(void *va);
l2e_t sys_get_l2(void *va);
int sys_env_set_trapframe(envid_t envid, struct Trapframe *tf);
int sys_env_set_status(envid_t envid, int status);
int sys_cgetc(void);

static inline envid_t __attribute__((always_inline)) sys_exofork(void)
{
	envid_t ret;
	asm volatile(
        "mov r7, %1;\
        swi 1;\
        mov %0, r0;"
		: "=r" (ret)
		: "r" (SYS_exofork): "memory", "r0", "r7");
	return ret;
}

static void delay(int n){
    for(int k = 0; k < n; k++)
        for(int i = 0; i < 1000; i++)
            for(int j = 0; j < 5000; j++)
                ;
}

static uintptr_t user_get_pa(void *va){
    assert(((uintptr_t)va > UBASE && (uintptr_t)va < UTOP));

    l2e_t l2e = sys_get_l2(va);
    uintptr_t res = ((uint32_t)l2e & ~0xFFF) | ((uint32_t)va & 0xFFF);

    log_trace("user_get_pa: convert [%p] to [%p]\n", va, res);
    return res;
}

int cprintf(const char *fmt, ...);

int fork(void);

// pgfault.c
void	set_pgfault_handler(void (*handler)(struct UTrapframe *utf));

// ipc.c
void	ipc_send(envid_t to_env, uint32_t value, void *pg, int perm);
int32_t ipc_recv(envid_t *from_env_store, void *pg, int *perm_store);
envid_t	ipc_find_env(enum EnvType type);

// fd.c
int	close(int fd);
ssize_t	read(int fd, void *buf, size_t nbytes);
ssize_t	write(int fd, const void *buf, size_t nbytes);
int	seek(int fd, off_t offset);
void	close_all(void);
ssize_t	readn(int fd, void *buf, size_t nbytes);
int	dup(int oldfd, int newfd);
int	fstat(int fd, struct Stat *statbuf);
int	stat(const char *path, struct Stat *statbuf);

// file.c
int	open(const char *path, int mode);
int	ftruncate(int fd, off_t size);
int	remove(const char *path);
int	sync(void);

/* File open modes */
#define	O_RDONLY	0x0000		/* open for reading only */
#define	O_WRONLY	0x0001		/* open for writing only */
#define	O_RDWR		0x0002		/* open for reading and writing */
#define	O_ACCMODE	0x0003		/* mask for above modes */

#define	O_CREAT		0x0100		/* create if nonexistent */
#define	O_TRUNC		0x0200		/* truncate to zero length */
#define	O_EXCL		0x0400		/* error if already exists */
#define O_MKDIR		0x0800		/* create directory, not regular file */

// pageref.c
int	pageref(void *addr);

// spawn.c
envid_t	spawn(const char *program, const char **argv);
envid_t	spawnl(const char *program, const char *arg0, ...);

// console.c
void	cputchar(int c);
int	getchar(void);
int	iscons(int fd);
int	opencons(void);

// wait.c
void	wait(envid_t env);

void exit(void);

#endif	// !JOS_INC_LIB_H

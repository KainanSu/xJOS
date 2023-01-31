#include <inc/lib.h>
#include <inc/env.h>

static int a = 1;

void
umain(int argc, char **argv)
{
    envid_t id;
    if((id = sys_fork()) < 0){
        panic("fork error");
    }
    if(id == 0){ // child
        for(int i = 0; i < 5; i++){
            cprintf("from child: id = %d\n", sys_getenvid());
            sys_yield();
        }
    }else{
        for(int i = 0; i < 5; i++){
            cprintf("from parent: id = %d\n", sys_getenvid());
            sys_yield();
        }
    }
    panic("A: to panic myself");
}

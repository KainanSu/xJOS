#include <inc/lib.h>

int fork(){
    int r = sys_fork();
    if(r == 0){
        envid_t eid = sys_getenvid();
	    thisenv = &((struct Env *)UENVS)[ENVX(eid)];
    }
    return r;
}
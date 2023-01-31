#include <inc/lib.h>

static int a = 1;

void
umain(int argc, char **argv)
{
    // delay();
    while(1){
        log_info("AAA\n");
        delay(1);
        delay(1);
    }
    for(int i = 0; i < 5; i++){
	    cprintf("A: i am A.\n");
        sys_yield();
    }
    panic("A: to panic myself");
}

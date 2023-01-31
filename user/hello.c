// hello, world
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	while(1){
		cprintf("test user cprintf\n");
		*(int*)(0) = 1;			// write error
		int x = *((int *)0x0);  // read error
		cprintf("x = %p\n", x);
		panic("fuck you\n");
	}
}

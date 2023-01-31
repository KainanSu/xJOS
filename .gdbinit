echo + target remote localhost:26000\n
target remote localhost:26000

echo + symbol-file obj/kern/kernel\n
symbol-file obj/kern/kernel

echo + breakpoint-file debug.bp\n
source debug.bp
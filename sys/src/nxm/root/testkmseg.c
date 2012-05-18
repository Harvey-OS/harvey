#include <u.h>
#include <libc.h>
#define KMBASE		(0x0008000000000000ull)


void
main(int argc, char *argv[])
{
	u8int *c;
	c = segbrk((void *)KMBASE, 0);

	*c = 0;
	while(1);
}

/* 6c big.c; 6l -o big big.6 */

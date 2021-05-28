/* virtex 4 dependencies */
#include "include.h"

uvlong myhz = 300000000;	/* fixed 300MHz */
uchar mymac[Eaddrlen] = { 0x00, 0x0A, 0x35, 0x01, 0x8B, 0xB1 };

void
clrmchk(void)
{
	putesr(0);			/* clears machine check */
}

/*
 * virtex 4 systems always have 128MB.
 */
uintptr
memsize(void)
{
	uintptr sz = 128*MB;

	return securemem? MEMTOP(sz): sz;
}

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

/*
 * wait (spin) a little while for a bit pattern to appear in a register,
 * but don't wait forever.
 */
int
awaitbitpatms(void *vint, ulong mask, ulong wantpat, int ms)
{
	ulong *reg;

	reg = (ulong *)vint;		/* accept (u?(int|long) *) */
	coherence();
	delay(1);	/* sometimes is important to delay before testing */
	while (--ms >= 0 && (*reg & mask) != wantpat)
		delay(1);
	return (*reg & mask) == wantpat? 0: -1;
}

int
awaitbitpat(void *vint, ulong mask, ulong wantpat)
{
	return awaitbitpatms(vint, mask, wantpat, 1000);
}

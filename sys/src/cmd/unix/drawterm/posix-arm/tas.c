#include "u.h"
#include "libc.h"

int
tas(long *x)
{
	int     v, t, i = 1;

#if ARMv5
	__asm__(
		"swp  %0, %1, [%2]"
		: "=&r" (v)
		: "r" (1), "r" (x)
		: "memory"
	);
#else
	__asm__ (
		"1:	ldrex	%0, [%2]\n"
		"	strex	%1, %3, [%2]\n"
		"	teq	%1, #0\n"
		"	bne	1b"
		: "=&r" (v), "=&r" (t)
		: "r" (x), "r" (i)
		: "cc");
#endif
	switch(v) {
	case 0:
	case 1:
		return v;
	default:
		print("canlock: corrupted 0x%lux\n", v);
		return 1;
	}
}


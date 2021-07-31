/* arm pre-v7 architecture */
#include "u.h"
#include "libc.h"

int
tas(long *x)
{
	int     v;

	__asm__(
		"swp  %0, %1, [%2]"
		: "=&r" (v)
		: "r" (1), "r" (x)
		: "memory"
	);
	switch(v) {
	case 0:
	case 1:
		return v;
	default:
		print("canlock: corrupted 0x%lux\n", v);
		return 1;
	}
}

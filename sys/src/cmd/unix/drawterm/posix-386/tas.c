#include "u.h"
#include "libc.h"

int
tas(long *x)
{
	int     v;

	__asm__(	"movl   $1, %%eax\n\t"
			"xchgl  %%eax,(%%ecx)"
			: "=a" (v)
			: "c" (x)
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


#include "lib9.h"
#include "sys.h"
#include "error.h"

int
canlock(Lock *l)
{
	int     v;

	__asm__(	"movl   $1, %%eax\n\t"
			"xchgl  %%eax,(%%ebx)"
			: "=a" (v)
			: "ebx" (&l->val)
	);
	switch(v) {
	case 0:	 return 1;
	case 1:	 return 0;
	default:	print("canlock: corrupted 0x%lux\n", v);
	}
	return 0;
}


#include <u.h>
#include <libc.h>
#include <ureg.h>

int	__noterestore(void);

void
notejmp(void *vr, jmp_buf j, int ret)
{
	struct Ureg *r = vr;

	/*
	 * song and dance to get around the kernel smashing r7 in noted
	 */
	r->r8 = ret;
	if(ret == 0)
		r->r8 = 1;
	r->r9 = j[JMPBUFPC] - JMPBUFDPC;
	r->pc = (ulong)__noterestore;
	r->npc = (ulong)__noterestore + 4;
	r->sp = j[JMPBUFSP];
	noted(NCONT);
}

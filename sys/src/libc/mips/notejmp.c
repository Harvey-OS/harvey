#include <u.h>
#include <libc.h>
#include <ureg.h>

int	__noterestore(void);

void
notejmp(void *vr, jmp_buf j, int ret)
{
	struct Ureg *r = vr;

	/*
	 * song and dance to get around the kernel smashing r1 in noted
	 */
	r->r2 = ret;
	if(ret == 0)
		r->r2 = 1;
	r->r3 = j[JMPBUFPC];
	r->pc = (ulong)__noterestore;
	r->sp = j[JMPBUFSP];
	noted(NCONT);
}

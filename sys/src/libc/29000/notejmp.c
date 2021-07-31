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
	r->sp = ret;
	if(ret == 0)
		r->sp = 1;
	r->pc = j[JMPBUFPC];
	r->sp = j[JMPBUFSP];
	noted(NCONT);
}

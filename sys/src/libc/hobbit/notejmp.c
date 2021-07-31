#include <u.h>
#include <libc.h>
#include <ureg.h>

void
notejmp(void *vr, jmp_buf j, int ret)
{
	struct Ureg *r = vr;

	if(ret == 0)
		ret = 1;
	r->pc = j[JMPBUFPC];
	r->sp = j[JMPBUFSP];
	*((int *)r->sp+1) = ret;
	noted(NCONT);
}

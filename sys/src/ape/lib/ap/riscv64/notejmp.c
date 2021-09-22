#include <u.h>
#include <libc.h>
#include <ureg.h>

void
notejmp(void *vr, jmp_buf j, int ret)
{
	struct Ureg *r = vr;

	r->ret = ret;
	if(ret == 0)
		r->ret = 1;
	r->pc = j[JMPBUFPC];
	r->sp = j[JMPBUFSP];
	noted(NCONT);
}

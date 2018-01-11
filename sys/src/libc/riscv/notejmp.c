#include <u.h>
#include <libc.h>
#include <ureg.h>

void
notejmp(void *vr, jmp_buf j, int ret)
{
	struct Ureg *r = vr;

	r->a0 = ret;
	if(ret == 0)
		r->a0 = 1;
	r->ip = j[JMPBUFPC];
	r->sp = j[JMPBUFSP] + 8;
	noted(NCONT);
}

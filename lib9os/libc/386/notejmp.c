#include <u.h>
#include <libc.h>
#include <ureg.h>

void
notejmp(void *vr, jmp_buf j, int ret)
{
	struct Ureg *r = vr;

	r->ax = ret;
	if(ret == 0)
		r->ax = 1;
	r->pc = j[JMPBUFPC];
	r->sp = j[JMPBUFSP] + 4;
	noted(NCONT);
}

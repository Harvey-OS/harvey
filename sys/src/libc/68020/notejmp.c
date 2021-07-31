#include <u.h>
#include <libc.h>
#define UREGVARSZ 4	/* not right but doesn't matter */
#include <ureg.h>

void
notejmp(void *vr, jmp_buf j, int ret)
{
	struct Ureg *r = vr;

	r->r0 = ret;
	if(ret == 0)
		r->r0 = 1;
	r->pc = j[JMPBUFPC];
	r->usp = j[JMPBUFSP] + 4;
	noted(NCONT);
}

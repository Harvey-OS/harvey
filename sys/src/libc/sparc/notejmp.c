#include <u.h>
#include <libc.h>
#include <ureg.h>

void
notejmp(void *vr, jmp_buf j, int ret)
{
	struct Ureg *r = vr;

	r->r7 = ret;
	if(ret == 0)
		r->r7 = 1;
	r->pc = j[JMPBUFPC] - JMPBUFDPC;
	r->npc = j[JMPBUFPC] - JMPBUFDPC + 4;
	r->sp = j[JMPBUFSP] + 4;
	noted(NCONT);
}

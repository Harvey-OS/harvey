#include	"l.h"

void
sched(Prog *p0, Prog *p1)
{
#ifdef	CODSCHED
	Prog *p, *q, *t;
	int r, a;

	if(debug['X'])
		Bprint(&bso, "\n");
#endif
	if (p0 == P || p0 == p1)
		return;
	if(debug['X'])
		Bprint(&bso, "sched from %P to %P\n", p0, p1);
}


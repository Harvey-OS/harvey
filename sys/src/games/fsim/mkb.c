#include	"type.h"

void
mkbinit(void)
{
}

void
mkbupdat(void)
{
	Nav *np;
	long d;

	if(plane.time % 5)
		return;
	np = plane.bk;
	if(np == NNULL)
		return;
	d = fdist(np->x, np->y);
	if(d > 2000)
		return;
	if(np->flag & OM)
		if(plane.time % 15)
			return;
	if(np->flag & MM)
		if(plane.time % 10)
			return;
	ringbell();
}

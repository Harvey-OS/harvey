#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>

int
cformat(void *c, int f1, int f2, int f3, int chr)
{
	char buf[64];

	sprint(buf, "%s.%d", ((Coord *) c)->chip, ((Coord *) c)->pin);
	strconv(buf, f1, f2, f3);
	return(sizeof(Coord));
}

int
rformat(Rectangle *r, int f1, int f2, int f3, int chr)
{
	char buf[64];

	sprint(buf, "%p %p", r->min, r->max);
	strconv(buf, f1, f2, f3);
	return(sizeof(Rectangle));
}

int
pformat(Point *p, int f1, int f2, int f3, int chr)
{
	char buf[64];

	sprint(buf, "%d/%d", p->x, p->y);
	strconv(buf, f1, f2, f3);
	return(sizeof(Point));
}

void
initformat(void)
{
	fmtinstall('r', rformat);
	fmtinstall('p', pformat);
	fmtinstall('C', cformat);
}

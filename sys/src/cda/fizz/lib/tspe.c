#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>

extern oneok;
void sortset(int (*)(void*, void*), short *, Coord *, Pin *, int);
long tsp1(Coord *c, Pin *p, short *o, int n, int head);
long exhaust(Coord *, Pin *, short *, int, int);
void ttspe(Coord *, Pin *, int, int);
extern int ycmp(short *, short *), xcmp(short * ,short *);

void
tspe(Signal *s)
{
	register Coord *c, *cc;

	if(s->type == VSIG)
		return;
	if(s->n == 1){
		if(!oneok)
			fprint(2, "signal %s has only one point!\n", s->name);
		return;
	}
	for(cc = s->routes, c = s->coords; ; c++)
		if(c->pin == cc->pin)
			break;
	ttspe(s->coords, s->pins, s->n, c-s->coords);
}

void
ttspe(Coord *cb, Pin *pb, int n, int head)
{
	short o[MAXNET], best[MAXNET];
	long bestl, l;

#define	TRY(fn)	{ l = fn(cb, pb, o, n, head);\
	if(l < bestl){\
		memcpy((char *)best, (char *)o, n*sizeof(short));\
		bestl = l;\
	} }

	bestl = 9999999L;
	sortset(xcmp, o, cb, pb, n); TRY(tsp1);
	sortset(ycmp, o, cb, pb, n); TRY(tsp1);
	if(bestl);	/* shut up cyntax */
	reorder(cb, pb, best, n);
}

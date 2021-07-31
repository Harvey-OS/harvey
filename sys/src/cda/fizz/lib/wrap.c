#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>

void f_major(char *, ...);

static int defroute = RTSP;
long wwraplen, wwires;

void tsp(Signal *);
void tspe(Signal *s);
void hand(Signal *s);
void prseq(Signal *s);
void prmst(Signal *s);
void mst(Signal *s);
void mst3(Signal *s);

void
calclen(Signal *s)
{
	register Pin *p;
	int n;
	register d;
	register long len;

	if((s->type & VSIG) || (s->n == 1))
		return;
	wwraplen -= s->length;
	len = 0;
	p = s->pins;
	n = s->n;
	for(p++, n -= 2; n >= 0; n--, p++){
		d = abs(p[-1].p.x-p->p.x) + abs(p[-1].p.y-p->p.y);
		len += d;
		wwires++;
	}
	wwraplen += s->length = len;
}

void
wrap(Signal *s)
{
	if(s->alg == 0)
		s->alg = defroute;
	switch(s->alg)
	{
	case RTSP:	tsp(s); s->prfn = prseq; break;
	case RTSPE:	tspe(s); s->prfn = prseq; break;
	case RHAND:	hand(s); s->prfn = prseq; break;
	case RMST:	mst(s); s->prfn = prmst; break;
	case RMST3:	mst3(s); s->prfn = prmst; break;
	default:
		f_major("Unknown routing algorithm %d", s->alg);
		abort();
		return;
	}
	calclen(s);
}

void
routedefault(int n)
{
	defroute = n;
}

void
prwrap(Signal *s)
{
	(*s->prfn)(s);
}

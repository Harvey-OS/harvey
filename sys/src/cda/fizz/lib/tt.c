#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>
void f_major(char *, ...);


void
ttadd(int sig, char *chip, int pin)
{
	register Nsig *n, *nn;
	char *s;

	if((s = f_b->v[sig].name) == 0){
		f_major("chip %s needs (tt) special signal %d which has no name",
			chip, sig);
		s = f_b->v[sig].name = f_strdup("??");
	}
	n = NEW(Nsig);
	n->c.chip = chip;
	n->c.pin = pin;
	n->sig = s;
	n->next = 0;
	if((nn = (Nsig *)symlook(s, S_TT, (void *)0)) == 0)
		(void)symlook(n->sig, S_TT, (void *)n);
	else {
		while(nn->next){
			nn = nn->next;
		}
		nn->next = n;
	}
}

void
ttclean(Nsig *nn)
{
	register Nsig *n;
	register Coord *c;
	int i;
	register Signal *s;

	for(n = nn, i = 0; n; i++)
		n = n->next;
	if((s = (Signal *)symlook(nn->sig, S_SIGNAL, (void *)0)) == 0){
		s = NEW(Signal);
		s->name = nn->sig;
		(void)symlook(s->name, S_SIGNAL, (void *)s);
		s->type = 0;	/* it will get set later if need be */
	}
	c = (Coord *)lmalloc((s->n+i)*(long)sizeof(Coord));
	memcpy((char *)c, (char *)s->coords, s->n*(long)sizeof(Coord));
	s->coords = c;
	for(c = &s->coords[s->n]; nn; nn = nn->next)
		*c++ = nn->c;
	s->n += i;
}

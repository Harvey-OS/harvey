#include	<u.h>
#include	<libc.h>
#include	<cda/fizz.h>
void f_minor(char *, ...);
void f_major(char *, ...);

void
hand(Signal *s)
{
	short ord[MAXNET];
	register Coord *c, *cc;
	int i, j;
	/*
		unfortunately quadratic because i am lazy
	*/
	for(cc = s->routes, i = 0; i < s->n; i++, cc++){
		for(c = s->coords, j = 0; j < s->n; j++, c++)
			if(c->pin == cc->pin)
				break;
		if(j < s->n)
			ord[i] = j;
		else {
			f_major("route for %s refers to bogus coord %C\n",
				s->name, *cc);
			return;
		}
	}
	reorder(s->coords, s->pins, ord, s->n);
}

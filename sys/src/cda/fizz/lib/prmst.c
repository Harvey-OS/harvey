#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>

extern long wwraplen, wwires;
char ptype(char *);
void cbprseq(Pin *, int, char *, int);

int cbflag, netno, numterms;
void
prmst(Signal *s)
{
	char type;
	Coord *co;
	Pin *p;
	int level, first, i, n;
	register d;

	if((s->type & VSIG) || (s->n == 1))
		return;
	co = s->coords;
	p = s->pins;
	n = s->n;
	type = s->type? 's' : 'w';
	level = 2;
	first = 1;
	if(cbflag) {
		if(s->type) return;
		cbprseq(p, n, s->name, type);
	}
	else {
	   for(n = 0; n < s->n; n += 2, co += 2, p += 2){
		   i = level = 3-level;
		   if(first) i += 4, first = 0;
		   if(n == 0) i += 8;
		   d = abs(p[1].p.x-p->p.x) + abs(p[1].p.y-p->p.y);
		   if(d == 0){
			   fprint(2, "zero length wire: %s.%d %s.%d\n",
				   co[0].chip, co[0].pin, co[1].chip, co[1].pin);
			   return;
		   }
		   fprint(1, "%c %d %s %d %p %s %c.%c %d %p %s %c.%c %d\n",
			   type, i, s->name, d/scale, SC(p[0].p), co[0].chip,
			   ptype(co[0].chip), p[0].drill, co[0].pin,
			   SC(p[1].p), co[1].chip, ptype(co[1].chip),
			   p[1].drill, co[1].pin);
		   wwraplen += d;
		   wwires++;
	   }
	}
	++netno;
}

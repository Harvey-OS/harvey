#include <u.h>
#include	<libc.h>
#include	<cda/fizz.h>

long wwraplen, wwires;
int cbflag, netno, numterms, cbfd;
int scale = 1;
void cbprseq(Pin *, int, char *, int );
char ptype(char *);
Point SC(Point);

void
prseq(Signal *s)
{
	char type;
	Coord *co;
	Pin *p;
	int level, first, i, n;
	register d;
	
	if(cbflag) {
		if(s->n == 1) return;
		cbprseq(s->pins, s->n, s->name, s->type);
		++netno;
		return;
	}
	if((s->type == VSIG) || (s->n == 1))
		return;
	co = s->coords;
	p = s->pins;
	n = s->n;
	type = s->type? 's' : 'w';
	level = 2;
	first = 1;
	for(co++, p++, n -= 2; n >= 0; n--, co++, p++){
		i = level = 3-level;
		if(first) i += 4, first = 0;
		if(n == 0) i += 8;
		d = abs(p[-1].p.x-p->p.x) + abs(p[-1].p.y-p->p.y);
		if(d == 0){
			fprint(2, "zero length wire: %s.%d %s.%d\n",
				co[-1].chip, co[-1].pin, co->chip, co->pin);
			return;
		}
		fprint(1, "%c %d %s %d %p %s %c.%c %d %p %s %c.%c %d\n",
			type, i, s->name, d/scale, SC(p[-1].p), co[-1].chip,
			ptype(co[-1].chip), p[-1].drill, co[-1].pin,
			SC(p->p), co->chip, ptype(co->chip),
			p->drill, co->pin);
		wwraplen += d;
		wwires++;
	}
	++netno;

}

void
cbprseq(Pin *p, int n, char *s, int type)
{
	int i;
	i = 0;
	if((type & VSIG) != VSIG)
		fprint(cbfd, "%s%4.4d	%s\n", "NN", netno, s); 
	while(n-- > 0){
		if(i%5 == 0) fprint(1,"\n%5d", netno);
		fprint(1, "%6d%6d", p->p.x, p->p.y);
		++p;
		++numterms;
		if((++i)%5 == 0) {
			if(type & VSIG)
				fprint(1, "  %s", s); 
			else
				fprint(1, "  %s%4.4d", "NN", netno);
		}
	}
	if(i%5) {
		while((i++)%5) fprint(1, "            ");
		if(type & VSIG)
			fprint(1, "  %s", s); 
		else
			fprint(1, "  %s%4.4d", "NN", netno);
	}
}

char
ptype(char *s)
{
	register i;

	for(i = 0; i < 6; i++)
		if(f_b->v[i].name && (strcmp(f_b->v[i].name, s) == 0))
			return('0'+i);
	return('_');
}

Point
SC(Point p)
{
	p.x /= scale;
	p.y /= scale;
	return(p);
}

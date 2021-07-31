#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>

void f_minor(char *, ...);
void f_major(char *, ...);
static void put(Pinspec *, int, Pin *, int );

Pin *
f_pins(char *s, int zero, int npins)
{
	register loop, nm;
	Pinspec p;
	Pin *pins;
	register Pin *pp;

	pins = (Pin *)f_malloc(npins*(long)sizeof(Pin));
	if(loop = *s == '{')
		s = f_inline();
	do {
		BLANK(s);
		if(*s == '}') break;
		NUM(s, p.p1);
		COORD(s, p.pt1.x, p.pt1.y)
		if(*s == '-'){
			s++;
			if(isdigit(*s))
				NUM(s, p.pd)
			else {
				BLANK(s)
				p.pd = 1;
			}
			NUM(s, p.p2);
			COORD(s, p.pt2.x, p.pt2.y)
		} else
			p.p2 = p.p1, p.pd = 1;
		if(*s){
			p.drill = *s++;
			BLANK(s);
		} else
			p.drill = 'A';	/* ??? */
		if(*s) EOLN(s)
		put(&p, zero, pins, npins);
	} while(loop && (s = f_inline()));
	for(pp = pins, npins -= zero; npins >= 0; npins--, pp++)
		if(pp->drill == 0)
			f_minor("pin %d not defined", zero+(int)(pp-pins));
	return(pins);
}

static
void
put(Pinspec *ps, int zero, Pin *pins, int npins)
{
	register long n, xd, yd;
	register short p;
	register Pin *pp, *pend = pins+npins;

	n = ps->p2-ps->p1;
	xd = ps->pt2.x-ps->pt1.x;
	yd = ps->pt2.y-ps->pt1.y;
	p = ps->p1;
	{
		pp = &pins[p-zero];
		if(pp >= pend){
			f_major("pin %d out of range", p);
			pp = pins;
		}
		pp->p.x = ps->pt1.x;
		pp->p.y = ps->pt1.y;
		pp->drill = ps->drill;
	}
	for(p = ps->p1+ps->pd; p <= ps->p2; p += ps->pd){
		pp = &pins[p-zero];
		if(pp >= pend){
			f_major("pin %d out of range", p);
			pp = pins;
		}
		pp->p.x = ps->pt1.x+(p-ps->p1)*(xd)/n;
		pp->p.y = ps->pt1.y+(p-ps->p1)*(yd)/n;
		pp->drill = ps->drill;
		pp->used = 0;
	}
}

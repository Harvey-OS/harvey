#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>
void f_minor(char *, ...);
void f_major(char *, ...);

static void pinhadd(Pinhole *);
long nph;

void
f_pinholes(char *s)
{
	register nm, loop;
	register Pinhole *p;

	BLANK(s);
	if(loop = *s == '{')
		s = f_inline();
	do {
		p = NEW(Pinhole);
		BLANK(s);
		if(*s == '}') break;
		COORD(s, p->o.x, p->o.y)
		NUM(s, p->len.x)
		NUM(s, p->len.y)
		NUM(s, p->sp.x)
		if(*s == '/'){
			s++;
			BLANK(s);
			NUM(s, p->sp.y);
		} else
			p->sp.y = p->sp.x;
		if(*s){
			p->drill = *s++;
			BLANK(s);
		} else
			p->drill = 0;
		if(*s) EOLN(s)
		pinhadd(p);
		p->next = f_b->pinholes;
		f_b->pinholes = p;
	} while(loop && (s = f_inline()));
}

static void
pinhadd(Pinhole *p)
{
	register x, y;
	Point l;

	l.x = p->len.x + p->o.x;
	l.y = p->len.y + p->o.y;
	blim(p->o);
	for(x = p->o.x; x < l.x; x += p->sp.x)
		for(y = p->o.y; y < l.y; y += p->sp.y){
			nph++;
			pinlook(XY(x, y), (long) p->drill);
		}
	l.x = x-p->sp.x;
	l.y = y-p->sp.y;
	blim(l);
}

void
blim(Point p)
{
	if(p.x < f_b->prect.min.x) f_b->prect.min.x = p.x;
	if(p.y < f_b->prect.min.y) f_b->prect.min.y = p.y;
	if(p.x > f_b->prect.max.x) f_b->prect.max.x = p.x;
	if(p.y > f_b->prect.max.y) f_b->prect.max.y = p.y;
	if((p.x + p.y) > (f_b->ne.x + f_b->ne.y)) f_b->ne = p;
	if((-p.x + p.y) > (-f_b->nw.x + f_b->nw.y)) f_b->nw = p;
	if((-p.x + -p.y) > (-f_b->sw.x + -f_b->sw.y)) f_b->sw = p;
	if((p.x + -p.y) > (f_b->se.x + -f_b->se.y)) f_b->se = p;
}

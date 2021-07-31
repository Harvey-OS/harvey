#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>
void f_minor(char *, ...);

#define		NPLANES		1000		/* more than when reading */
typedef struct Planeset
{
	int np;
	unsigned short layer;
	Plane p[NPLANES];
} Planeset;

static void
add(Planeset *p, Rectangle r, char *sig)
{
	register Plane *pp;
	register i;

	for(i = 0, pp = p->p; i < p->np; i++, pp++){
		if(strcmp(sig, pp->sig)) continue;
		if((pp->r.min.x == r.min.x) && (pp->r.max.x == r.max.x)){
			if(r.min.y == pp->r.max.y){
				pp->r.max.y = r.max.y;
				return;
			}
			if(r.max.y == pp->r.min.y){
				pp->r.min.y = r.min.y;
				return;
			}
		}
		if((pp->r.min.y == r.min.y) && (pp->r.max.y == r.max.y)){
			if(r.min.x == pp->r.max.x){
				pp->r.max.x = r.max.x;
				return;
			}
			if(r.max.x == pp->r.min.x){
				pp->r.min.x = r.min.x;
				return;
			}
		}
	}
	pp->sense = 1;
	pp->r = r;
	pp->layer = p->layer;
	pp->sig = sig;
	p->np++;
}

static void
sub(Planeset *p, Rectangle r, int miss)
{
	Rectangle rr[4];
	int n, i, np;

	if(p->np == 0){
		if(!miss)
			f_minor("layer %d: nothing to subtract %d,%d %d,%d from",
				p->layer, r.min.x, r.min.y, r.max.x, r.max.y);
		return;
	}
	for(i = 0, np = p->np; i < np; i++){
		if((n = fg_rectXrect(r, p->p[i].r, rr)) > 0){
			p->p[i].r = rr[0];
			while(--n >= 1){
				p->p[p->np].sense = 1;
				p->p[p->np].sig = p->p[i].sig;
				p->p[p->np].layer = p->layer;
				p->p[p->np++].r = rr[n];
			}
		}
	}
}

void
fizzplane(Board *b)
{
	Planeset p[MAXLAYER];
	register Plane *pp;
	register i, n;
	int layer;
	char buf[512];

	for(i = 0; i < MAXLAYER; i++)
		p[i].np = 0, p[i].layer = i;
	layer = 0;
	for(i = 0, pp = b->planes; i < b->nplanes; i++, pp++)
		if((pp->layer < 0) || (pp->layer >= MAXLAYER))
			f_minor("bad layer %d in plane defn", pp->layer);
		else if(pp->sense > 0){
			layer |= pp->layer;
			add(&p[pp->layer], pp->r, pp->sig);
		}
	b->layers = layer;
	if(f_nerrs)
		return;
	for(i = 0, pp = b->planes; i < b->nplanes; i++, pp++)
		if(pp->sense < 0)
			sub(&p[pp->layer], pp->r, pp->sig == 0);
	for(i = 0; i < MAXLAYER; i++)
		if(b->layer[i])
			layer &= ~LAYER(i);
	if(layer){
		n = 0;
		for(i = 0; i < MAXLAYER; i++)
			if(layer&LAYER(i)){
				sprint(&buf[n], ", %d", i);
				while(buf[n]) n++;
			}
		f_minor("undefined layers %s", buf+2);
	}
	for(n = i = 0; i < MAXLAYER; i++)
		n += p[i].np;
	b->nplanes = n;
	b->planes = (Plane *)f_malloc(b->nplanes*(long)sizeof(Plane));
	for(n = i = 0; i < MAXLAYER; i++)
		if(p[i].np){
			memcpy((char *)&b->planes[n], (char *)p[i].p, p[i].np*sizeof(Plane));
			n += p[i].np;
		}
}

int
layerof(unsigned short u)
{
	register i;

	for(i = 0; i < MAXLAYER; i++)
		if(u&LAYER(i))
			return(i);
	return(0);
}

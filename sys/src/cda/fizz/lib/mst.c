#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>

int dist(int,int);
void swap(int, int);

typedef struct Wrap
{
	Pin *p;
	Coord *c;
} Wrap;
static Wrap *base;
/* Points not yet in fragment */
extern int	name [];		/* name of point */
extern short	nndist [];	/* dist to nearest neighbor in mst */
extern int	nninfrag [];	/* nearest neighbor in fragment */

extern int oneok;

static Wrap *
domst(Wrap *wb, Wrap *w, int count)
{
	int thispt, thisnn, i, top, minelt;
	float tdist;

	thispt = top = count-1;
	base = wb;
	for(i = 0; i < top; i++){
		name[i] = i;
		nndist[i] = dist(i,thispt);
		nninfrag[i] = thispt;
	}
	while (top > 0) {
		minelt = 0;
		for (i = 1; i < top; i++)
			if (nndist[i] <= nndist[minelt]) minelt = i;
		top--;
		swap(minelt, top);
		thispt = name[top];
		thisnn = nninfrag[top];
		*w++ = base[thispt];
		*w++ = base[thisnn];
		for (i = 0; i < top; i++) {
			tdist = dist(name[i], thispt);
			/* || clause: if dists equal, tend towards less */
			if (tdist <  nndist[i]) {
				nndist[i] = tdist;
				nninfrag[i] = thispt;
			}
		}
	}
	return(w);
}

void
mst(Signal *s)
{
	Wrap ww[2*MAXNET], res[2*MAXNET];
	register Wrap *wend, *w;
	register i;

	if(s->type & VSIG)
		return;
	if(s->n == 1){
		if(!oneok)
			fprint(2, "signal %s has only one point!\n", s->name);
		return;
	}
	for(i = 0, w = ww; i < s->n; i++, w++){
		w->p = &s->pins[i];
		w->c = &s->coords[i];
	}
	wend = domst(ww, res, s->n);
	s->n = wend-res;
	s->pins = (Pin *) lmalloc((long) (s->n*sizeof(Pin)));
	s->coords = (Coord *) lmalloc((long) (s->n*sizeof(Coord)));
	for(i = 0, w = res; i < s->n; i++, w++){
		s->pins[i] = *w->p;
		s->coords[i] = *w->c;
	}
}

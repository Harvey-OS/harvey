#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>

typedef struct Wrap
{
	Pin *p;
	Coord *c;
	short degree;
} Wrap;

static Wrap *base;
/* Points not yet in fragment */
int	name [MAXNET];		/* name of point */
short	nndist [MAXNET];	/* dist to nearest neighbor in mst */
int	nninfrag [MAXNET];	/* nearest neighbor in fragment */

extern int oneok;

int
dist(int a, int b)
{
	return(abs(base[a].p->p.x-base[b].p->p.x) + abs(base[a].p->p.y-base[b].p->p.y));
}

void
swap(int i, int j) /* swap nonfragment points i and j */
{
	int t;
	register short tt;

	t=name[i];     name[i]=name[j];         name[j]=t;
	tt=nndist[i];  nndist[i]=nndist[j];     nndist[j]=tt;
	t=nninfrag[i]; nninfrag[i]=nninfrag[j]; nninfrag[j]=t;
}

static Wrap *
real_mst(Wrap *wb, Wrap *w, int count, int deg3)
{
	int thispt, thisnn, i, top, minelt;
	float tdist, mindist;

	thispt = top = count-1;
	base = wb;
	for(i = 0; i < count; i++)
		wb[i].degree = 0;
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
		/* Begin: avoid degree-4 vertices */
		if (deg3 && (base[thisnn].degree == 3)) {
			mindist = 32767;
			for (i = 0; i < count; i++)
				if ((base[i].degree==1 || base[i].degree==2) &&
				     dist(i,thispt) < mindist) {
					mindist = dist(i,thispt);
					minelt = i;
				}
			thisnn = minelt;
		}
		/* End:   avoid degree-4 vertices */
		*w = base[thisnn]; w->p->used = thisnn; w++;
		*w = base[thispt]; w->p->used = thispt; w++;
		base[thispt].degree++;
		base[thisnn].degree++;
		for (i = 0; i < top; i++) {
			tdist = dist(name[i], thispt);
			/* || clause: if dists equal, tend towards less */
			if ((tdist <  nndist[i]) ||
			    (tdist == nndist[i] &&
			      base[thispt].degree < base[nninfrag[i]].degree)) {
				nndist[i] = tdist;
				nninfrag[i] = thispt;
			}
		}
	}
	return(w);
}

static void
do_mst(Signal *s, int deg3)
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
	wend = real_mst(ww, res, s->n, deg3);
	s->n = wend-res;
	s->pins = (Pin *) lmalloc((long) (s->n*sizeof(Pin)));
	s->coords = (Coord *) lmalloc((long) (s->n*sizeof(Coord)));
	for(i = 0, w = res; i < s->n; i++, w++){
		s->pins[i] = *w->p;
		s->coords[i] = *w->c;
	}
}

void
mst(Signal *s)
{
	do_mst(s, 0);
}

void
mst3(Signal *s)
{
	do_mst(s, 1);
}

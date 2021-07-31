#include <u.h>
#include <libc.h>
#include <libg.h>
#include <bio.h>
#include "dat.h"
#include "fns.h"

static void	borderfn(void*, void*);
static void	borderpt(Sym*, Point);
static void	prbfunc(Sym*, void*);

void
xyborder(char *name)
{
	execlump(name, (Point){0,0}, 0, borderfn);
}

void
prborder(Biobuf *bp)
{
	symtraverse(prbfunc, bp);
}

void
comborder(Sym *s1, Sym *s2)
{
	Rect *r2;

	if(s1 == 0 || s2 == 0)
		return;
	if(s1->type != Layer || s2->type != Layer)
		return;
	r2 = s2->ptr;
	if(r2 == 0)
		return;
	borderpt(s1, r2->min);
	borderpt(s1, r2->max);
}

static void
borderfn(void *o, void *args)
{
	Path *pp; Rect *rp; Text *tp; Ring *kp;
	int i;

	switch(((Clump *)o)->type){
	case PATH:
	case BLOB:
		pp = o;
		for(i=0; i<pp->n; i++)
			borderpt(pp->layer, pp->edge[i]);
		break;
	case RECT:
		rp = o;
		borderpt(rp->layer, rp->min);
		borderpt(rp->layer, rp->max);
		break;
	case RING:
		kp = o;
		i = kp->diam/2;
		borderpt(kp->layer, Pt(kp->pt.x+i, kp->pt.y+i));
		borderpt(kp->layer, Pt(kp->pt.x-i, kp->pt.y-i));
		break;
	case TEXT:
		tp = o;
		borderpt(tp->layer, tp->start);	/* BUG */
		break;
	default:
		error("xyborder: bad type %d", ((Clump *)o)->type);
		break;
	}
}

static void
borderpt(Sym *layer, Point p)
{
	Rect *r;

	r = layer->ptr;
	if(r == 0){
		r = new(RECT);
		layer->ptr = r;
		r->layer = layer;
		r->min = p;
		r->max = p;
		return;
	}
	if(p.x < r->min.x)
		r->min.x = p.x;
	if(p.x > r->max.x)
		r->max.x = p.x;
	if(p.y < r->min.y)
		r->min.y = p.y;
	if(p.y > r->max.y)
		r->max.y = p.y;
}

static void
prbfunc(Sym *x, void *arg)
{
	Biobuf *bp; Rect *r;

	if(x->type != Layer)
		return;
	bp = *(Biobuf **)arg;
	r = x->ptr;
	Bprint(bp, "%s\t", x->name);
	if(r)
		Bprint(bp, "%d,%d %d,%d\n",
			r->min.x, r->min.y, r->max.x, r->max.y);
	else
		Bprint(bp, "empty\n");
}

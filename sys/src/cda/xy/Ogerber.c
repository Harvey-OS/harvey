#include <u.h>
#include <libc.h>
#include <libg.h>
#include <bio.h>
#include "dat.h"
#include "fns.h"

static void	displayfn(void*, void*);
static int	xsidecompare(Side**, Side**);
static int	ysidecompare(Side**, Side**);

static Sym *	layer;
static int	sense;

static Clump *	goo;

void
gerberin(char *name, int fcode)
{
	layer = symfind(name);
	if(layer == 0 || layer->type != Layer){
		fprint(2, "gerberin: layer \"%s\" not found\n", name);
		return;
	}
	if(layer->ptr == 0){
		fprint(2, "gerberin: layer \"%s\" is empty\n", name);
		return;
	}
	if(goo == 0){
		goo = new(CLUMP);
	}
	sense = fcode;
	execlump(master->name, (Point){0,0}, 0, displayfn);
}

static void
displayfn(void *o, void *args)
{
	Path *pp; Rect *rp; Text *tp; Ring *kp;
	Rectangle r; Side *sp;

	rp = o;
	if(rp->layer != layer)
		return;
	switch(rp->type){
	case PATH:
		pp = o;
		if(sense < 0)
			error("sorry, PATH's can only be added");
		gerberpath(pp);
		break;
	case BLOB:
		pp = o;
		error("sorry, BLOB's are too messy");
		break;
	case RING:
		kp = o;
		error("sorry, RING's are too messy");
		break;
	case RECT:
		r = rcanon(*rp);
		sp = new(SIDE);
		sp->sense = sense ? 1 : -1;
		sp->x = r.min.x;
		sp->ymin = r.min.y;
		sp->ymax = r.max.y;
		wrclump(goo, sp);
		sp = new(SIDE);
		sp->sense = sense ? -1 : 1;
		sp->x = r.max.x;
		sp->ymin = r.min.y;
		sp->ymax = r.max.y;
		wrclump(goo, sp);
		break;
	case TEXT:
		tp = o;
		if(sense < 0)
			error("sorry, TEXT can only be added");
		gerbertext(tp);
		break;
	default:
		error("gerberin: bad type %d", rp->type);
		break;
	}
}

void
gerberout(void)
{
	Side *s, *sprev, *head; Clump *xp;
	int xesize, *xedge, nxe;
	int i, y0, y1, ymin, ymax;
	int owind, wind = 0;

	if(goo == 0 || goo->n == 0){
		fprint(2, "gerberout: no goo to mash\n");
		return;
	}

	xesize = 32;
	xedge = malloc(xesize*sizeof(int));

	qsort(goo->o, goo->n, sizeof(Side *), ysidecompare);

	head = goo->o[0];
	ymin = head->ymin;
	ymax = head->ymax;
	for(sprev=head, i=1; i<goo->n; sprev=s, i++){
		s = goo->o[i];
		sprev->next = s;
		if(s->ymax > ymax)
			ymax = s->ymax;
	}
	sprev->next = 0;
	if(debug)
		Bprint(&out, "ymin=%d, ymax=%d\n", ymin, ymax);
	if(debug>1)for(i=0; i<goo->n; i++){
		s = goo->o[i];
		Bprint(&out, "%d %c %d [%d %d)\n",
			i, (s->sense>0?'+':'-'), s->x, s->ymin, s->ymax);
	}
	xp = new(CLUMP);
	for(y0 = ymin; y0 < ymax; y0 = y1){
		xp->n = 0;
		for(sprev=0, s=head; s; s=s->next){
			if(s->ymin > y0)
				break;
			if(y0 >= s->ymax){
				if(sprev)
					sprev->next = s->next;
				else
					head = s->next;
				continue;
			}
			wrclump(xp, s);
			sprev = s;
		}
		y1 = s ? s->ymin : ymax;
		for(i=0; i<xp->n; i++){
			s = xp->o[i];
			if(y0 < s->ymin && s->ymin < y1){
				y1 = s->ymin;
				break;
			}
			if(y0 < s->ymax && s->ymax < y1)
				y1 = s->ymax;
		}
		if(debug>1)
			Bprint(&out, "[%d, %d)\n", y0, y1);
		qsort(xp->o, xp->n, sizeof(Side *), xsidecompare);
		if(xp->size > xesize){
			xesize = xp->size;
			free(xedge);
			xedge = malloc(xesize*sizeof(int));
		}
		nxe = 0;
		for(i=0; i<xp->n; i++){
			s = xp->o[i];
			if(debug>1)
				Bprint(&out, "\t%d %c %d [%d %d)\n", i,
					(s->sense>0?'+':'-'), s->x,
					s->ymin, s->ymax);
			owind = wind;
			wind += s->sense;
			if((owind <= 0) == (wind <= 0))
				continue;
			if(nxe == 0 || s->x != xedge[nxe-1]){
				xedge[nxe++] = s->x;
				continue;
			}
			if(((nxe&1) == 0) != (wind <= 0))
				--nxe;
		}
		if(wind != 0)
			error("winding number botch");
		for(i=0; i<nxe; i+=2){
			gerberrect(Rxy(xedge[i], y0, xedge[i+1], y1));
		}
	}
}

static int
ysidecompare(Side **sp0, Side **sp1)
{
	Side *s0 = *sp0, *s1 = *sp1;
	int t;

	if(t = s0->ymin - s1->ymin)	/* assign = */
		return t;
	return s0->x - s1->x;
}

static int
xsidecompare(Side **sp0, Side **sp1)
{
	Side *s0 = *sp0, *s1 = *sp1;

	return s0->x - s1->x;
}

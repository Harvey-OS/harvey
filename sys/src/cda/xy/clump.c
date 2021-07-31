#include <u.h>
#include <libc.h>
#include <libg.h>
#include <bio.h>
#include "dat.h"
#include "fns.h"

#define NCLUMP	64
#define NPATH	16

static Point	xform(Point, Point, int);

static int lengths[] = {
	[BLOB]	sizeof(Path),
	[CLUMP]	sizeof(Clump),
	[INC]	sizeof(Inc),
	[PATH]	sizeof(Path),
	[RCOMP]	sizeof(Rcomp),
	[RECTI]	sizeof(Rect),
	[RECT]	sizeof(Rect),
	[REPEAT] sizeof(Repeat),
	[RING]	sizeof(Ring),
	[SIDE]	sizeof(Side),
	[TEXT]	sizeof(Text),
};

void *
new(int type)
{
	Clump *c; Path *pp;
	int len;

	if(0 <= type && type < sizeof lengths/sizeof lengths[0])
		len = lengths[type];
	else
		len = 0;
	if(len == 0){
		error("new: bad type %d", type);
		return 0;
	}
	c = malloc(len);
	c->type = type;
	switch(type){
	case CLUMP:
		c->o = malloc(NCLUMP*sizeof(void *));
		c->n = 0;
		c->size = NCLUMP;
		break;
	case PATH:
		pp = (Path *)c;
		pp->edge = malloc(NPATH*sizeof(Point));
		pp->n = 0;
		pp->size = NPATH;
		break;
	}
	return c;
}

void
wrclump(Clump *c, void *o)
{
	if(c->n >= c->size){
		c->size += NCLUMP;
		c->o = realloc(c->o, c->size*sizeof(void *));
		if(c->o == 0)
			error("realloc failed in wrclump");
	}
	c->o[c->n++] = o;
}

void
endclump(Clump *c)
{
	if(c->n != c->size){
		c->size = c->n;
		c->o = realloc(c->o, c->size*sizeof(void *));
		if(c->o == 0)
			error("realloc failed in endclump");
	}
}

void
wrpath(Path *pp, Point p)
{
	if(pp->n >= pp->size){
		pp->size += NPATH;
		pp->edge = realloc(pp->edge, pp->size*sizeof(Point));
		if(pp->edge == 0)
			error("realloc failed in wrpath");
	}
	pp->edge[pp->n++] = p;
}

void
endpath(Path *pp)
{
	if(pp->n != pp->size){
		pp->size = pp->n;
		pp->edge = realloc(pp->edge, pp->size*sizeof(Point));
		if(pp->edge == 0)
			error("realloc failed in endpath");
	}
}

void
prclump(Biobuf *bp, char *name)
{
	Sym *s;
	Clump *c;
	int i;

	s = symfind(name);
	if(s == 0 || s->type != Clname){
		fprint(2, "prlump: \"%s\" not found\n", name);
		return;
	}
	Bprint(bp, "%s CLUMP\n", name);
	c = s->ptr;
	for(i=0; i<c->n; i++)
		probject(bp, c->o[i]);
	Bprint(bp, " END CLUMP\n");
}


void
probject(Biobuf *bp, void *o)
{
	Inc *ip; Path *pp; Rect *rp; Repeat *np; Text *tp; Ring *kp;
	int j;

	switch(((Clump *)o)->type){
	case INC:
		ip = o;
		Bprint(bp, " INC %s %d,%d,,%d\n",
			ip->clump->name, ip->pt.x, ip->pt.y, ip->angle);
		break;
	case PATH:
		pp = o;
		Bprint(bp, " PATH %s,%d", pp->layer->name, pp->width);
		for(j=0; j<pp->n; j++)
			Bprint(bp, " %d,%d",
				pp->edge[j].x, pp->edge[j].y);
		Bprint(bp, "\n");
		break;
	case BLOB:
		pp = o;
		Bprint(bp, " BLOB %s", pp->layer->name);
		for(j=0; j<pp->n; j++)
			Bprint(bp, " %d,%d",
				pp->edge[j].x, pp->edge[j].y);
		Bprint(bp, "\n");
		break;
	case RECT:
		rp = o;
		Bprint(bp, " RECT %s %d,%d %d,%d\n",
			rp->layer->name,
			rp->min.x, rp->min.y, rp->max.x, rp->max.y);
		break;
	case RECTI:
		rp = o;
		Bprint(bp, " RECTI %s %d,%d %d,%d\n",
			rp->layer->name, rp->min.x, rp->min.y,
			rp->max.x-rp->min.x, rp->max.y-rp->min.y);
		break;
	case REPEAT:
		np = o;
		Bprint(bp, " REPEAT %s %d,%d, %d %d %d\n",
			np->clump->name, np->pt.x, np->pt.y,
			np->count, np->inc.x, np->inc.y);
		break;
	case RING:
		kp = o;
		Bprint(bp, " RING %s %d,%d, %d\n",
			kp->layer->name, kp->pt.x, kp->pt.y, kp->diam);
		break;
	case TEXT:
		tp = o;
		Bprint(bp, " TEXT %s %d %d,%d,,%d, (%s)\n",
			tp->layer->name, tp->size,
			tp->start.x, tp->start.y, tp->angle,
			tp->text->name);
		break;
	default:
		error("probject: bad type %d", ((Clump *)o)->type);
		break;
	}
}

void
execlump(char *name, Point org, int angle, void (*pfn)(void*, void*), ...)
{
	Sym *s;

	s = symfind(name);
	if(s == 0 || s->type != Clname){
		fprint(2, "execlump: \"%s\" not found\n", name);
		return;
	}
	clumploop(s->ptr, org, angle, pfn, (&pfn+1));
}

void
clumploop(Clump *c, Point org, int angle, void (*pfn)(void*, void*), void *args)
{
	void *o; Inc *ip;
	Path *pp; Rect *rp; Repeat *np; Text *tp; Ring *kp;
	Path tpp[1]; Rect trp[1]; Text ttp[1]; Ring tkp[1];
	Point p;
	int i, j;

	for(i=0; i<c->n; i++){
		o = c->o[i];
		switch(((Clump *)o)->type){
		case INC:
			ip = o;
			clumploop(ip->clump->ptr, xform(ip->pt, org, angle),
				angle+ip->angle, pfn, args);
			break;
		case PATH:
		case BLOB:
			pp = o;
			*tpp = *pp;
			tpp->edge = malloc(tpp->size*sizeof(Point));
			for(j=0; j<tpp->n; j++)
				tpp->edge[j] = xform(pp->edge[j], org, angle);
			(*pfn)(tpp, args);
			free(tpp->edge);
			break;
		case RECT:
		case RECTI:
			rp = o;
			trp->type = RECT;
			trp->layer = rp->layer;
			trp->min = xform(rp->min, org, angle);
			trp->max = xform(rp->max, org, angle);
			(*pfn)(trp, args);
			break;
		case REPEAT:
			np = o;
			p = np->pt;
			for(j = 0; j<np->count; j++){
				clumploop(np->clump->ptr, xform(p, org, angle),
					angle, pfn, args);
				p = add(p, np->inc);
			}
			break;
		case RING:
			kp = o;
			*tkp = *kp;
			tkp->pt = xform(kp->pt, org, angle);
			(*pfn)(tkp, args);
			break;
		case TEXT:
			tp = o;
			*ttp = *tp;
			ttp->start = xform(tp->start, org, angle);
			ttp->angle = tp->angle + angle;
			(*pfn)(ttp, args);
			break;
		default:
			error("clumploop: bad type %d", ((Clump *)o)->type);
			break;
		}
	}
}

static Point
xform(Point p, Point shift, int angle)
{
	static int cangle;
	static double ccos, csin;
	double theta;

	switch(angle){
	case 0:
		return (Point){shift.x+p.x, shift.y+p.y};
	case 90:
		return (Point){shift.x-p.y, shift.y+p.x};
	case 180:
		return (Point){shift.x-p.x, shift.y-p.y};
	case -90:
	case 270:
		return (Point){shift.x+p.y, shift.y-p.x};
	}
	if(angle != cangle){
		cangle = angle;
		theta = PI*angle/180.0;
		ccos = cos(theta);
		csin = sin(theta);
	}
	return (Point){ shift.x+(int)(p.x*ccos - p.y*csin + 0.5),
			shift.y+(int)(p.x*csin + p.y*ccos + 0.5)};
}

#include <u.h>
#include <libc.h>
#include <libg.h>
#include <bio.h>
#include "dat.h"
#include "fns.h"

static void	displayfn(void*, void*);
static Point	xform(Point);

static int	inited;

static Rectangle	srect;
static Sym *		layer;
static Rectangle	lrect;
static double		scale;
static int		sense;

void
displayer(char *name, int fcode)
{
	Sym *s; Rect *lr;
	double xscale, yscale;

	layer = symfind(name);
	if(layer == 0 || layer->type != Layer){
		fprint(2, "display: layer \"%s\" not found\n", name);
		return;
	}
	lr = layer->ptr;
	if(lr == 0){
		fprint(2, "display: layer \"%s\" is empty\n", name);
		return;
	}
	if(!inited){
		inited = 1;
		binit(0, 0, "xydraw");
		srect = inset(bscreenrect(0), 5);
		srect.min.x += 16;
		bitblt(&screen, srect.min, &screen, srect, Zero);
		srect = inset(srect, font->height);
	}
	lrect.min = lr->min;
	lrect.max = lr->max;
	xscale = ((double)Dx(srect))/Dx(lrect);
	yscale = ((double)Dy(srect))/Dy(lrect);
	if(xscale < yscale)
		scale = xscale;
	else
		scale = yscale;
	sense = fcode;
	execlump(master->name, (Point){0,0}, 0, displayfn);
	bflush();
}

static void
displayfn(void *o, void *args)
{
	Path *pp; Rect *rp; Text *tp; Ring *kp;
	Rectangle r; Point p, q;
	int i, tl, rad;

	rp = o;
	if(rp->layer != layer)
		return;
	switch(rp->type){
	case PATH:
		pp = o;
		p = xform(pp->edge[0]);
		for(i=1; i<pp->n; i++){
			q = xform(pp->edge[i]);
			if(debug)
				Bprint(&out, "%d %s %P %P\n",
					sense, layer->name, p, q);
			segment(&screen, p, q, colors[colori], sense);
			p = q;
		}
		break;
	case RECT:
		r.min = xform(rp->min);
		r.max = xform(rp->max);
		r = rcanon(r);
		if(debug)
			Bprint(&out, "%d %s %R\n", sense, layer->name, r);
		rect(&screen, r, colors[colori], sense);
		break;
	case RING:
		kp = o;
		p = xform(kp->pt);
		rad = (int)(0.5*scale*kp->diam + 0.5);
		if(rad < 1)
			rad = 1;
		if(debug)
			Bprint(&out, "C%d %s %P %d\n",
				sense, layer->name, p, rad);
		disc(&screen, p, rad, colors[colori], sense);
		break;
	case TEXT:
		tp = o;
		tl = (4*tp->size*strlen(tp->text->name))/5;
		r.min = tp->start;
		switch(tp->angle){
		case 0:
			r.min = add(r.min, Pt(-2*tp->size/5, -tp->size/2));
			r.max = add(r.min, Pt(tl, tp->size));
			break;
		case 90:
			r.min = add(r.min, Pt(-tp->size/2, -2*tp->size/5));
			r.max = add(r.min, Pt(tp->size, tl));
			break;
		case 180:
			r.min = add(r.min, Pt(2*tp->size/5, -tp->size/2));
			r.max = add(r.min, Pt(-tl, tp->size));
			break;
		case 270:
		case -90:
			r.min = add(r.min, Pt(-tp->size/2, 2*tp->size/5));
			r.max = add(r.min, Pt(tp->size, -tl));
			break;
		}
		r.min = xform(r.min);
		r.max = xform(r.max);
		r = rcanon(r);
		if(debug)
			Bprint(&out, "T%d %s %R\n", sense, layer->name, r);
		rect(&screen, r, colors[colori], sense);
		break;
	default:
		error("display bad type %d", rp->type);
		break;
	}
}

static Point
xform(Point p)
{
	p.x = srect.min.x + (int)(scale*(p.x - lrect.min.x) + 0.5);
	p.y = srect.max.y - (int)(scale*(p.y - lrect.min.y) + 0.5);
	return p;
}

void
disptexture(void)
{
	Bitmap *t;

	t = balloc(Rxy(0, 0, 2, 2), screen.ldepth);

	point(t, (Point){0, 0}, ~0, F);
	point(t, (Point){1, 1}, ~0, F);
	texture(&screen, srect, t, DandS);
	bfree(t);
}

void
rect(Bitmap *b, Rectangle r, int col, Fcode fc)
{
	static int prev;
	static Bitmap *one, *many;

	if(one == 0){
		one = balloc(Rxy(0,0,1,1), screen.ldepth);
		many = balloc(Rxy(0,0,128,128), screen.ldepth);
	}
	if(col != prev){
		point(one, Pt(0,0), col, S);
		texture(many, many->r, one, S);
		prev = col;
	}
	texture(b, r, many, fc);
}

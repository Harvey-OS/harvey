#include <u.h>
#include <libg.h>
#include <layer.h>

#define	RECUR(a,b,c,d)	_layerop(fn, cl->next, Rect(a.x, b.y, c.x, d.y), l, etc);

void
_layerop(
	void (*fn)(Layer *, Rectangle, Layer *, void *),
	Layer *cl,
	Rectangle r,
	Layer *l,
	void *etc)
{
    Top:
	if(cl == l){
		(*fn)(l->cover->layer, r, l, etc);
		return;
	}
	if(rectXrect(r, cl->r) == 0){
		cl = cl->next;
		goto Top;
	}
	if(cl->r.max.x < r.max.x){
		RECUR(cl->r.max, r.min, r.max, r.max);
		r.max.x = cl->r.max.x;
	}
	if(cl->r.max.y < r.max.y){
		RECUR(r.min, cl->r.max, r.max, r.max);
		r.max.y = cl->r.max.y;
	}
	if(r.min.x < cl->r.min.x){
		RECUR(r.min, r.min, cl->r.min, r.max);
		r.min.x = cl->r.min.x;
	}
	if(r.min.y < cl->r.min.y){
		RECUR(r.min, r.min, r.max, cl->r.min);
		r.min.y = cl->r.min.y;
	}
	(*fn)((Layer *)l->cache, r, l, etc);
}

void
layerop(
	void (*fn)(Layer *, Rectangle, Layer *, void *),
	Rectangle r,
	Layer *l,
	void *etc)
{
	if(rectclip(&r, l->r)){
		if(l->vis == Visible)
			(*fn)(l->cover->layer, r, l, etc);
		else
			_layerop(fn, l->cover->front, r, l, etc);
	}
}

#include <u.h>
#include <libg.h>
#include <layer.h>

static void
_ltofront(Layer *dl, Rectangle r, Layer *l, void *ignore)
{
	USED(ignore);
	if(dl == (Layer *)l->cache)
		bitblt(l->cover->layer, r.min, l->cache, r, S);
}

void
ltofront(Layer *l)
{
	Layer *tl, *cl;

	cl = l->cover->front;
	if(cl == l)
		cl = 0;
	else
		while(cl->next != l)
			cl = cl->next;
	if(cl)
		for(tl=l->cover->front; tl!=l; tl=tl->next)
			if(rectXrect(l->r, tl->r) && tl->vis==Visible)
				bitblt(tl->cache, tl->r.min,
					l->cover->layer, tl->r, S);
	layerop(_ltofront, l->r, l, (void *)0);
	_ldelete(l);
	_linsertfront(l);
	if(cl)
		for(tl=l->cover->front;; tl=tl->next){
			if(rectXrect(l->r, tl->r))
				_lvis(tl);
			if(tl == cl)
				break;
		}
	_lvis(l);	/* could be offscreen, so not Visible */
}

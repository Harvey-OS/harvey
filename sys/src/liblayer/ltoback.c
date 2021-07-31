#include <u.h>
#include <libg.h>
#include <layer.h>

void
ltoback(Layer *l)
{
	Layer *tl;
	int ovis;

	ovis = l->vis;
	tl = l->next;
	_ldelete(l);
	_linsertback(l);
	_lvis(l);
	if(ovis==Visible && l->vis!=Visible)
		bitblt(l->cache, l->r.min, l->cover->layer, l->r, S);
	if(tl)
		for(; tl!=l; tl=tl->next)
			if(rectXrect(l->r, tl->r)){
				layerop(lupdate, l->r, tl, (void *)0);
				_lvis(tl);
			}
}

#include <u.h>
#include <libg.h>
#include <layer.h>
#include <libc.h>

Layer*
lalloc(Cover *c, Rectangle r)
{
	Layer *l, *cl;

	cl = c->layer;
	l = malloc(sizeof(Layer));
	if(l == 0)
		return 0;
	l->id = cl->id;
	l->ldepth = cl->ldepth;
	l->r = r;
	l->clipr = r;
	l->cover = c;
	l->cache = balloc(r, l->ldepth);	/* clears the cache, too */
	if(l->cache == 0){
		free(l);
		return 0;
	}
	_linsertback(l);
	l->vis = Invisible;
	/* pull to the front and clear the new layer */
	ltofront(l);
	bitblt(l, r.min, l, r, 0);
	return l;
}

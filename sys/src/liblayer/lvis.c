#include <u.h>
#include <libg.h>
#include <layer.h>

struct lvisarg{
	int	obs;
	int	vis;
};

static void
__lvis(Layer *dl, Rectangle r, Layer *l, void *etc)
{
	struct lvisarg *lv;

	USED(r);
	lv = etc;
	if(dl == (Layer *)l->cache)
		lv->obs = 1;
	else
		lv->vis = 1;
}

void
_lvis(Layer *l)
{
	struct lvisarg lv;
	Rectangle rc, rl;

	lv.obs = 0;
	lv.vis = 0;
	l->vis = Invisible;	/* assume the worst */
	layerop(__lvis, l->r, l, &lv);
	if(lv.vis == 0)
		l->vis = Invisible;
	else if(lv.obs)
		l->vis = Obscured;
	else{
		l->vis = Visible;
		rl = l->r;
		rc = l->cover->layer->clipr;
		if(rl.min.x<rc.min.x || rl.max.x>rc.max.x
		|| rl.min.y<rc.min.y || rl.max.y>rc.max.y)
			l->vis = Obscured;
	}
}

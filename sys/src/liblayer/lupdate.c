#include <u.h>
#include <libg.h>
#include <layer.h>

void
lupdate(Layer *dl, Rectangle r, Layer *l, void *ignore)
{
	USED(ignore);
	if(dl != (Layer *)l->cache)
		bitblt((Bitmap *)l->cover->layer, r.min, l->cache, r, S);
}

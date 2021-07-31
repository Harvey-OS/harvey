#include <u.h>
#include <libc.h>
#include <libg.h>
#include <layer.h>

void
lfree(Layer *l)
{
	ltoback(l);
	if(l->vis != Invisible){
		l->clipr = l->r;
		texture(l, l->r, l->cover->ground, S);
	}
	_ldelete(l);
	bfree(l->cache);
	free(l);
}

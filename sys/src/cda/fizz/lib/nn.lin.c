#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>

static Pin *pb;
static np;

void
nnprep(Pin *pins, int npins)
{
	pb = pins;
	np = npins;
}

void
nn(Point p, long *bdist, short *bpt)
{
	register Pin *pp;
	register j, k;
	long dist, best;

	best = 999999L;
	for(pp = pb, j = 0; j < np; j++, pp++){
		dist = p.x-pp->p.x;
		if(dist < 0) dist = -dist;
		if(dist >= best) continue;
		k = p.y-pp->p.y;
		dist += (k < 0)? -k:k;
		if(dist < best)
			best = dist, *bpt = j;
	}
	*bdist = best;
}

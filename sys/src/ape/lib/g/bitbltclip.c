#include <libg.h>

int
bitbltclip(void *vp)
{
	int dx, dy;
	int i;
	struct{
		Bitmap *dm;
		Point p;
		Bitmap *sm;
		Rectangle r;
		Fcode f;
	}*bp;

	bp = vp;
	dx = Dx(bp->r);
	dy = Dy(bp->r);
	if(bp->p.x < bp->dm->clipr.min.x){
		i = bp->dm->clipr.min.x-bp->p.x;
		bp->r.min.x += i;
		bp->p.x += i;
		dx -= i;
	}
	if(bp->p.y < bp->dm->clipr.min.y){
		i = bp->dm->clipr.min.y-bp->p.y;
		bp->r.min.y += i;
		bp->p.y += i;
		dy -= i;
	}
	if(bp->p.x+dx > bp->dm->clipr.max.x){
		i = bp->p.x+dx-bp->dm->clipr.max.x;
		bp->r.max.x -= i;
		dx -= i;
	}
	if(bp->p.y+dy > bp->dm->clipr.max.y){
		i = bp->p.y+dy-bp->dm->clipr.max.y;
		bp->r.max.y -= i;
		dy -= i;
	}
	if(bp->r.min.x < bp->sm->clipr.min.x){
		i = bp->sm->clipr.min.x-bp->r.min.x;
		bp->p.x += i;
		bp->r.min.x += i;
		dx -= i;
	}
	if(bp->r.min.y < bp->sm->clipr.min.y){
		i = bp->sm->clipr.min.y-bp->r.min.y;
		bp->p.y += i;
		bp->r.min.y += i;
		dy -= i;
	}
	if(bp->r.max.x > bp->sm->clipr.max.x){
		i = bp->r.max.x-bp->sm->clipr.max.x;
		bp->r.max.x -= i;
		dx -= i;
	}
	if(bp->r.max.y > bp->sm->clipr.max.y){
		i = bp->r.max.y-bp->sm->clipr.max.y;
		bp->r.max.y -= i;
		dy -= i;
	}
	return dx>0 && dy>0;
}

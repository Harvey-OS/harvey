#include <libg.h>

void
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
	}*bp=vp;
	dx=Dx(bp->r);
	dy=Dy(bp->r);
	if(bp->p.x < bp->dm->r.min.x){
		i=bp->dm->r.min.x-bp->p.x;
		bp->r.min.x+=i;
		bp->p.x+=i;
		dx-=i;
	}
	if(bp->p.y < bp->dm->r.min.y){
		i=bp->dm->r.min.y-bp->p.y;
		bp->r.min.y+=i;
		bp->p.y+=i;
		dy-=i;
	}
	if(bp->p.x+dx > bp->dm->r.max.x)
		bp->r.max.x-=bp->p.x+dx-bp->dm->r.max.x;
	if(bp->p.y+dy > bp->dm->r.max.y)
		bp->r.max.y-=bp->p.y+dy-bp->dm->r.max.y;
	if(bp->r.min.x < bp->sm->r.min.x){
		i=bp->sm->r.min.x-bp->r.min.x;
		bp->p.x+=i;
		bp->r.min.x+=i;
	}
	if(bp->r.min.y < bp->sm->r.min.y){
		i=bp->sm->r.min.y-bp->r.min.y;
		bp->p.y+=i;
		bp->r.min.y+=i;
	}
	if(bp->r.max.x > bp->sm->r.max.x)
		bp->r.max.x=bp->sm->r.max.x;
	if(bp->r.max.y > bp->sm->r.max.y)
		bp->r.max.y=bp->sm->r.max.y;
}

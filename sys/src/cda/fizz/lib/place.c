#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>
int quiet;
void rotate(Chip *);
void putpkg(Chip *);
void rectdel(Chip *c);

void
place(Chip *c)
{
	register Pin *p;
	Chip *cp;
	int i;

	putpkg(c);
	if(!(c->flags&IGRECT)){
		if(cp = rectadd(c)){
			if(!quiet)
				fprint(2, "chip %s: rect collision with %s\n",
					c->name, cp->name);
			unplace(c);
			return;
		}
	}
	if(!(c->flags&IGPINS)){
		for(i = 0, p = c->pins; i < c->npins; i++, p++)
			if(pinlook(XY(p->p.x, p->p.y), 0) == 0){
				if(!quiet)
					fprint(2, "pin %s.%d[%p]; no pinhole\n",
						c->name, i+c->pmin, p->p);
				unplace(c);
				return;
			}
	}
}

void
unplace(Chip *c)
{
	register Pin *p;
	register int i;

	rectdel(c);
	c->flags |= UNPLACED;
	c->r = c->type->pkg->r;
	c->pt.x = c->pt.y = 0;
	if(c->npins)
		c->pins[0].p = c->pt;	/* don't ask me why; ask rotate */
	rotate(c);
	for(i = 0, p = c->pins; i < c->npins; i++, p++)
		p->p.x = p->p.y = -1;
}

void
putpkg(Chip *c)
{
	register i;
	register Package *p = c->type->pkg;

	memcpy((char *)c->pins, (char *)p->pins, c->npins*(long)sizeof(Pin));
	memcpy((char *)c->drills, (char *)p->drills, c->ndrills*(long)sizeof(Pin));
	c->r = p->r;
	rotate(c);
	c->r = fg_raddp(c->r, c->pt);
	for(i = 0; i < c->npins; i++)
		fg_padd(&c->pins[i].p, c->pt);
	for(i = 0; i < c->ndrills; i++)
		fg_padd(&c->drills[i].p, c->pt);
}

static rot[4][4] =
{
	1, 0, 0, 1,	/* rot 0 */
	0, -1, 1, 0,	/* rot 90 */
	-1, 0, 0, -1,	/* rot 180 */
	0, 1, -1, 0,	/* rot 270 */
};

#define	ROTXY(X,Y)	{\
		int x = X*rot[r][0]+Y*rot[r][1];\
		Y = X*rot[r][2]+Y*rot[r][3];\
		X = x; }

void
xyrot(Point *off, int r)
{
	ROTXY(off->x, off->y);
}

void
rotate(Chip *c)
{
	int r = c->rotation;
	register i;
	Point zero;

	if(c->npins)
		zero = c->pins[0].p;
	else
		zero.x = zero.y = 0;
	ROTXY(c->r.min.x, c->r.min.y)
	ROTXY(c->r.max.x, c->r.max.y)
	fg_rcanon(&c->r);
	for(i = 0; i < c->npins; i++)
		ROTXY(c->pins[i].p.x, c->pins[i].p.y)
	for(i = 0; i < c->npins; i++)
		fg_psub(&c->pins[i].p, zero);
	for(i = 0; i < c->ndrills; i++)
		ROTXY(c->drills[i].p.x, c->drills[i].p.y)
	for(i = 0; i < c->ndrills; i++)
		fg_psub(&c->drills[i].p, zero);
	fg_psub(&c->r.min, zero);
	fg_psub(&c->r.max, zero);
}
void
planerot(Rectangle *rr, Chip *c)
{
	int r = c->rotation;
	Point zero;
	if(c->type->pkg->npins)
		zero = c->type->pkg->pins[0].p;
	else
		zero.x = zero.y = 0;

	ROTXY(rr->min.x, rr->min.y);
	ROTXY(rr->max.x, rr->max.y);
	fg_rcanon(rr);
	fg_psub(&rr->min, zero);
	fg_psub(&rr->max, zero);	
}

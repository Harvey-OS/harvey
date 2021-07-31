#include	"type.h"

typedef	struct	Pln	Pln;
struct	Pln
{
	Rectangle	I;
	Rectangle	J;
	int		state;
	long		dorgx;
	long		dorgy;
	long		dlim;
	Fract		dsin;
	Fract		dcos;
	Bitmap*		dmap;
	Fract		dscale;
	Point		voff;
	Bitmap*		vor;
	Bitmap*		dme;
	Bitmap*		ndb;
	Bitmap*		mkb;
	Bitmap*		apt;
};
static	Pln	pln;

enum
{
	MSIZE	= 50,
	DSIZE	= 800,
	DOFF	= (DSIZE-MSIZE)/2,
};

void
plinit(void)
{
	int t;

	plane.dx = 104 * KNOT;
	plane.dz = 0;
	plane.ap = DEG(0);
	plane.ab = DEG(10);
	plane.rb = DEG(0);
	plane.pwr = 790;
	plane.tz = 0;
	plane.ah = DEG(0);

	plane.obs1 = 227;
	plane.vor1 = 1030;
	plane.obs2 = 200;
	plane.vor2 = 960;
	plane.adf = 392;

	plane.wlo = DEG(180);
	plane.whi = DEG(90);
	plane.mlo = 5 * KNOT;
	plane.mhi = 20 * KNOT;

	pln.I = Drect;
	pln.I.max.y -= 3*plane.side;
	pln.dlim = 36*MILE;
	pln.dscale = ONE / 380;
	pln.dmap = balloc(Rect(0,0, DSIZE,DSIZE), 0);
	if(!pln.dmap)
		exits("1");
	pln.vor = balloc(Rect(0,0, MSIZE,MSIZE), 0);
	pln.dme = balloc(Rect(0,0, MSIZE,MSIZE), 0);
	pln.ndb = balloc(Rect(0,0, MSIZE,MSIZE), 0);
	pln.mkb = balloc(Rect(0,0, MSIZE,MSIZE), 0);
	pln.apt = balloc(Rect(0,0, MSIZE,MSIZE), 0);
	if(!pln.vor || !pln.dme || !pln.ndb || !pln.mkb || !pln.apt)
		exits("1");
	vorlogo(pln.vor);
	dmelogo(pln.dme);
	ndblogo(pln.ndb);
	mbklogo(pln.mkb);
	aptlogo(pln.apt);
	pln.voff.x = MSIZE/2 + W + W/2;
	pln.voff.y = MSIZE/2 - H/2 + 2;
	setorg();
	t = (pln.I.max.x - pln.I.min.x) / 2;
	pln.J.min.x = 400 - t;
	pln.J.max.x = 400 + t;
	t = (pln.I.max.y - pln.I.min.y) / 2;
	pln.J.min.y = 400 - t;
	pln.J.max.y = 400 + t;
}

void
plupdat(void)
{
	short vsq;
	Angle aa;
	int x, y;
	long dx, dy;

	plane.ab += plane.rb;
	plane.ab &= 2*PI - 1;
	if(plane.ab > DEG(60) && plane.ab < DEG(300)) {
		if(plane.ab > DEG(180))
			plane.ab = DEG(360-60);
		else
			plane.ab = DEG(60);
	}
	vsq = muldiv(plane.dx/10, plane.dx/10, 1000);
	aa = plane.ap;
	aa -= iatan((long)plane.dz, (long)plane.dx);
	aa += DEG(5);
	aa &= 2*PI - 1;

	x = (plane.pwr + 7) / 14;
	x -= (vsq + 28) / 56;
	x -= (plane.dz + 25) / 50;
	plane.dx += x;

	x = fmul((long)muldiv(vsq, 12, 10), isin(aa));
	x = fmul((long)x, icos(plane.ab));
	x -= 320;
	plane.dz += x;

	aa = plane.ah - plane.magvar;
	plane.x += fmul((long)plane.dx+500, isin(aa)/1000);
	plane.y += fmul((long)plane.dx+500, icos(aa)/1000);

	aa = plane.wlo +  muldiv(plane.whi-plane.wlo, plane.z, 10000)
			- plane.magvar;
	x = plane.mlo +  muldiv(plane.mhi-plane.mlo, plane.z, 10000);
	plane.x += fmul((long)x+500, isin(aa)/1000);
	plane.y += fmul((long)x+500, icos(aa)/1000);

	plane.tz += plane.dz;
	while(plane.tz >= 1000) {
		plane.z++;
		plane.tz -= 1000;
	}
	while(plane.tz <= -1000) {
		plane.z--;
		plane.tz += 1000;
	}
	if(plane.z < 0)
		plane.z = 0;
	if(plane.z > 10000)
		plane.z = 10000;
	if(plane.ab < PI)
		plane.ah += plane.ab / 40;
	else
		plane.ah -= (PI - plane.ab + PI) / 40;

	dx = plane.x - pln.dorgx;
	if(dx >= -pln.dlim && dx < pln.dlim) {
		dy = plane.y - pln.dorgy;
		if(dy >= -pln.dlim && dy < pln.dlim) {
			x = fmul(dx, pln.dcos) + fmul(dy, pln.dsin);
			y = fmul(dx, pln.dsin) - fmul(dy, pln.dcos);
			x = fmul(x, pln.dscale) + DSIZE/2;
			y = fmul(y, pln.dscale) + DSIZE/2;
			point(pln.dmap, Pt(x, y), LEVEL, F_OR);
		}
	}
	if(plane.nbut && !plane.obut)
	if(plane.buty < pln.I.max.y) {
		if(plane.butx > pln.I.max.x-plane.side)
			rectf(D, pln.I, F_CLR);
		else
		if(plane.butx < pln.I.min.x+plane.side)
			setorg();
		else
		bitblt(D, pln.I.min,
			 pln.dmap, pln.J,F_STORE);
	}
}

void
setorg(void)
{
	Nav *np;
	Apt *ap;
	Point p;
	char s[10];
	long dx, dy;
	long x, y;

	rectf(pln.dmap, pln.dmap->r, F_CLR);
	pln.dorgx = plane.x;
	pln.dorgy = plane.y;
	pln.dsin = isin(plane.magvar);
	pln.dcos = icos(plane.magvar);
	for(np = &nav[0]; np->z; np++) {
		dx = np->x - pln.dorgx;
		if(dx < -pln.dlim || dx >= pln.dlim)
			continue;
		dy = np->y - pln.dorgy;
		if(dy < -pln.dlim || dy >= pln.dlim)
			continue;
		x = fmul(dx, pln.dcos) + fmul(dy, pln.dsin);
		y = fmul(dx, pln.dsin) - fmul(dy, pln.dcos);
		p.x = fmul(x, pln.dscale) + DOFF;
		p.y = fmul(y, pln.dscale) + DOFF;
		if(np->flag & VOR) {
			bitblt(pln.dmap, p, pln.vor, pln.vor->r, F_OR);
			rad50((ushort)np->obs, s);
			string(pln.dmap, add(p, pln.voff), font, s, F_OR);
		}
		if(np->flag & DME)
			bitblt(pln.dmap, p, pln.dme, pln.dme->r, F_OR);
		if(np->flag & NDB)
			bitblt(pln.dmap, p, pln.ndb, pln.ndb->r, F_OR);
		if(np->flag & (OM|IM|MM))
			bitblt(pln.dmap, p, pln.mkb, pln.mkb->r, F_OR);
	}
	for(ap = &apt[0]; ap->z; ap++) {
		dx = ap->x - pln.dorgx;
		if(dx < -pln.dlim || dx >= pln.dlim)
			continue;
		dy = ap->y - pln.dorgy;
		if(dy < -pln.dlim || dy >= pln.dlim)
			continue;
		x = fmul(dx, pln.dcos) + fmul(dy, pln.dsin);
		y = fmul(dx, pln.dsin) - fmul(dy, pln.dcos);
		p.x = fmul(x, pln.dscale) + DOFF;
		p.y = fmul(y, pln.dscale) + DOFF;
		bitblt(pln.dmap, p, pln.apt, pln.apt->r, F_OR);
	}
}

void
vorlogo(Bitmap *b)
{
	int t;

	t = MSIZE/2;
	rectf(b, b->r, F_CLR);
	circle(b, add(b->r.min, Pt(t,t)), 8, LEVEL, F_OR);
	circle(b, add(b->r.min, Pt(t,t)), 10, LEVEL, F_OR);
	segment(b, Pt(t,t+8), Pt(t,t+10), LEVEL, F_OR);
	segment(b, Pt(t,t-8), Pt(t,t-10), LEVEL, F_OR);
	segment(b, Pt(t+8,t), Pt(t+10,t), LEVEL, F_OR);
	segment(b, Pt(t-8,t), Pt(t-10,t), LEVEL, F_OR);
}

void
dmelogo(Bitmap *b)
{
	int t;

	t = MSIZE/2;
	rectf(b, b->r, F_CLR);
	circle(b, add(b->r.min, Pt(t,t)), 6, LEVEL, F_OR);
}

uchar	ndbtex[] =
{
	 0x22, 0x22, 0x88, 0x88, 0x22, 0x22, 0x88, 0x88,
	 0x22, 0x22, 0x88, 0x88, 0x22, 0x22, 0x88, 0x88,
	 0x22, 0x22, 0x88, 0x88, 0x22, 0x22, 0x88, 0x88,
	 0x22, 0x22, 0x88, 0x88, 0x22, 0x22, 0x88, 0x88,
};

void
ndblogo(Bitmap *b)
{
	int i, j, t;
	Bitmap *tex;

	tex = balloc(Rect(0, 0, 16, 16), 0);
	wrbitmap(tex, 0, 16, ndbtex);
	t = MSIZE/2;
	rectf(b, b->r, F_CLR);
	texture(b, b->r, tex, F_OR);
	for(i=0; i<MSIZE; i++)
	for(j=0; j<MSIZE; j++)
		if((i-t)*(i-t)+(j-t)*(j-t) > 100)
			point(b, Pt(i,j), LEVEL, F_CLR);
	bfree(tex);
}

void
mbklogo(Bitmap *b)
{
	int t, i;

	t = MSIZE/2;
	rectf(b, b->r, F_CLR);
	for(i=1; i<3; i++)
		circle(b, add(b->r.min, Pt(t,t)), i, LEVEL, F_OR);
}

void
aptlogo(Bitmap *b)
{
	int t;

	t = MSIZE/2;
	rectf(b, b->r, F_CLR);
	circle(b, add(b->r.min, Pt(t,t)), 4, LEVEL, F_OR);
}

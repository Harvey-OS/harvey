#include	"type.h"

typedef	struct	Pln	Pln;
struct	Pln
{
	Rectangle	I;
	int		state;
	ulong		dorglat;
	ulong		dorglng;
	long		dlim;
	Fract		dsin;
	Fract		dcos;
	Image*		dmap;
	Fract		dscale;
	Image*		vor;
	Image*		dme;
	Image*		ndb;
	Image*		mkb;
	Image*		apt;
	Point		dspo;
};
static	Pln	pln;

enum
{
	MSIZE	= 50,
	DSIZE	= 800,
	DOFF	= (DSIZE-MSIZE)/2,
};

void
plstart(void)
{
	plane.lat = 484173095;
	plane.lng = 891708464;
	plane.coslat = cos(plane.lat * C1);
	plane.z = 5000;
	setxyz("N51");

	plane.dx = KNOT(104);
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
	plane.mlo = KNOT(5);
	plane.mhi = KNOT(20);
}

void
plinit(void)
{
	Rectangle r;
	Image *tmp;

	d_bfree(pln.dmap);
	d_bfree(pln.vor);
	d_bfree(pln.dme);
	d_bfree(pln.ndb);
	d_bfree(pln.mkb);
	d_bfree(pln.apt);

	r = Rect(0,0, DSIZE,DSIZE);
	pln.dmap = d_balloc(r, 0);

	r = Rect(0,0, MSIZE,MSIZE);
	pln.vor = d_balloc(r, 0);
	pln.dme = d_balloc(r, 0);
	pln.ndb = d_balloc(r, 0);
	pln.mkb = d_balloc(r, 0);
	pln.apt = d_balloc(r, 0);
	tmp = d_balloc(r, 0);

	pln.I = Drect;
	pln.I.max.y -= 3*plane.side;
	pln.dlim = MILE(36);
	pln.dscale = ONE / 380;

	/*
	 * In order to ``or'' these onto the screen, we need their negatives.
	 */
	vorlogo(tmp);
	draw(pln.vor, r, display->black, tmp, r.min);

	d_clr(tmp, tmp->r);
	dmelogo(tmp);
	draw(pln.dme, r, display->black, tmp, r.min);

	d_clr(tmp, tmp->r);
	ndblogo(tmp);
	draw(pln.ndb, r, display->black, tmp, r.min);

	d_clr(tmp, tmp->r);
	mbklogo(tmp);
	draw(pln.mkb, r, display->black, tmp, r.min);

	d_clr(tmp, tmp->r);
	aptlogo(tmp);
	draw(pln.apt, r, display->black, tmp, r.min);

	setorg();
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

	aa = plane.ah + plane.magvar;
	x = plane.dx+500;
	dx = fmul(x, isin(aa));
	dy = fmul(x, icos(aa));

	/* wind correction */
	aa = plane.wlo + muldiv(plane.whi-plane.wlo, plane.z, 10000) - plane.magvar;
	x =  plane.mlo + muldiv(plane.mhi-plane.mlo, plane.z, 10000) + 500;
	dx += fmul(x, isin(aa));
	dy += fmul(x, icos(aa));

	convertll(dx, dy);

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

	dy = plane.lat - pln.dorglat;
	dy = dy * C2;
	if(dy >= -pln.dlim && dy < pln.dlim) {
		dx = pln.dorglng - plane.lng;
		dx = dx * C2 * plane.coslat;
		if(dx >= -pln.dlim && dx < pln.dlim) {
			x = fmul(dx, pln.dcos) + fmul(dy, pln.dsin);
			y = fmul(dx, pln.dsin) - fmul(dy, pln.dcos);
			x = fmul(x, pln.dscale) + DSIZE/2;
			y = fmul(y, pln.dscale) + DSIZE/2;
			d_point(pln.dmap, Pt(x, y), LEVEL);
		}
	}
	if(plane.nbut && !plane.obut)
	if(plane.buty < pln.I.max.y) {
		if(plane.butx > pln.I.max.x-plane.side)
			d_clr(D, pln.I);		// clear the map display
		else
		if(plane.butx < pln.I.min.x+plane.side)
			setorg();			// place plane on center of map
		else
			draw(D, pln.I, pln.dmap, nil, pln.dspo);
//			d_copy(D, pln.I, pln.dmap);	// display the hidden map

	}

	if(plane.trace != nil) {
		if(plane.otime == 0)
			plane.otime = plane.time;
		Bprint(plane.trace, "%ld %.6f %.6f %d\n",
			plane.time-plane.otime,
			plane.lat * (360./4294967296.),
			plane.lng * (360./4294967296.),
			plane.z);
	}
}

int
empty(Image *i, Rectangle r)
{
	int j, n;
	uchar data[1000];

	n = unloadimage(i, r, data, sizeof(data));
	for(j=0; j<n; j++)
		if(data[j] != 0xff)
			return 0;
	return 1;
}

static	schar	off[] = { 0, +1, -1, +2, -2, 0 };

void
placestr(Point p, char *s)
{
	Rectangle r;
	int i;

	// find a vacant place to place the string
	r.min.x = p.x;
	r.max.x = r.min.x + 4*W;
	for(i=0; i<nelem(off); i++) {
		r.min.y = p.y + off[i]*H;
		r.max.y = r.min.y + H;
		if(empty(pln.dmap, r))
			break;
	}
	d_string(pln.dmap, r.min, font, s);
}

void
setorg(void)
{
	Nav *np;
	Rectangle r;
	Point pt;
	char s[10], *p, *q;
	int c;
	long dx, dy;
	long x, y;


	pln.dspo.x = 0;
	pln.dspo.y = plane.side;
//	pln.dspo.x -= (pln.I.min.x + pln.I.max.x)/2;
//	pln.dspo.y -= (pln.I.min.y + pln.I.max.y)/2;
//	pln.dspo.x += DOFF;
//	pln.dspo.y += DOFF;

	d_clr(pln.dmap, pln.dmap->r);	// clear the hidden map
	pln.dorglat = plane.lat;
	pln.dorglng = plane.lng;
	pln.dsin = isin(plane.magvar);
	pln.dcos = icos(plane.magvar);

	/* pass 1 - put on the logos */
	for(np = &nav[0]; np->name != nil; np++) {
		dy = np->lat - pln.dorglat;
		dy = dy * C2;
		if(dy < -pln.dlim || dy >= pln.dlim)
			continue;
		dx = pln.dorglng - np->lng;
		dx = dx * C2 * plane.coslat;
		if(dx < -pln.dlim || dx >= pln.dlim)
			continue;
		x = fmul(dx, pln.dcos) + fmul(dy, pln.dsin);
		y = fmul(dx, pln.dsin) - fmul(dy, pln.dcos);
		r.min.x = fmul(x, pln.dscale) + DOFF;
		r.min.y = fmul(y, pln.dscale) + DOFF;
		r.max.x = r.min.x + MSIZE;
		r.max.y = r.min.y + MSIZE;
		if(np->flag & DME)
			d_or(pln.dmap, r, pln.dme);
		if(np->flag & NDB)
			d_or(pln.dmap, r, pln.ndb);
		if(np->flag & (OM|IM|MM))
			d_or(pln.dmap, r, pln.mkb);
		if(np->flag & VOR)
			d_or(pln.dmap, r, pln.vor);
		if(np->flag & APT)
			d_or(pln.dmap, r, pln.apt);
	}

	/* pass 2 - put on the names */
	for(np = &nav[0]; np->name != nil; np++) {
		if(!(np->flag & (APT|VOR)))
			continue;
		dy = np->lat - pln.dorglat;
		dy = dy * C2;
		if(dy < -pln.dlim || dy >= pln.dlim)
			continue;
		dx = pln.dorglng - np->lng;
		dx = dx * C2 * plane.coslat;
		if(dx < -pln.dlim || dx >= pln.dlim)
			continue;
		x = fmul(dx, pln.dcos) + fmul(dy, pln.dsin);
		y = fmul(dx, pln.dsin) - fmul(dy, pln.dcos);
		pt.x = fmul(x, pln.dscale) + DOFF + MSIZE/2 + W + W/2;
		pt.y = fmul(y, pln.dscale) + DOFF + MSIZE/2 - H/2 + 2;
		if(np->flag & APT) {
			q = s;
			for(p=np->name; c=*p; p++) {
				if(c < 'A' || c > 'Z') {
					q = nil;
					break;
				}
				*q++ = c + 'a' - 'A';
			}
			if(q != nil) {
				*q = 0;
				placestr(pt, s);
			}
		}
		if(np->flag & VOR)
			placestr(pt, np->name);
	}
}

void
vorlogo(Image *b)
{
	int t;

	t = MSIZE/2;
	d_circle(b, addpt(b->r.min, Pt(t,t)), 8, LEVEL);
	d_circle(b, addpt(b->r.min, Pt(t,t)), 10, LEVEL);
	d_segment(b, Pt(t,t+8), Pt(t,t+10), LEVEL);
	d_segment(b, Pt(t,t-8), Pt(t,t-10), LEVEL);
	d_segment(b, Pt(t+8,t), Pt(t+10,t), LEVEL);
	d_segment(b, Pt(t-8,t), Pt(t-10,t), LEVEL);
}

void
dmelogo(Image *b)
{
	int t;

	t = MSIZE/2;
	d_circle(b, addpt(b->r.min, Pt(t,t)), 6, LEVEL);
}

void
ndblogo(Image *b)
{
	int i, j, t;

	t = MSIZE/2;
	for(i=0; i<MSIZE; i++)
	for(j=0; j<MSIZE; j++)
		if((i-t)*(i-t)+(j-t)*(j-t) <= 100)
			if((j%2==0 && i%4==0) || (j%2==1 && i%4==2))
				d_point(b, Pt(i,j), LEVEL);
}

void
mbklogo(Image *b)
{
	int t, i;

	t = MSIZE/2;
	for(i=1; i<3; i++)
		d_circle(b, addpt(b->r.min, Pt(t,t)), i, LEVEL);
}

void
aptlogo(Image *b)
{
	int t;

	t = MSIZE/2;
	d_circle(b, addpt(b->r.min, Pt(t,t)), 4, LEVEL);
}

#include	"type.h"

typedef	struct	Vor	Vor;
struct	Vor
{
	Rectangle	I;
	char		s1[10];
	char		s2[10];
	char		s3[10];
	char		s4[10];
	Point		obs;
	Point		freq;
	Point		dme;
	Point		from;
	Nav**		np;
	Nav**		dm;
	Nav**		gs;
	ushort*		ob;
	ushort*		q;
	int		nx;
	int		ny;
	Image*		i;
	Image*		t;
};
static	Vor	v1, v2;

extern	void	vupdate(Vor*);
extern	void	vinit(Vor*, int, int, Nav**, Nav**, Nav**, ushort*, ushort*);

void
vorinit(void)
{

	vinit(&v1, 3, 0, &plane.v1, &plane.d1, &plane.g1, &plane.obs1, &plane.vor1);
	vinit(&v2, 3, 1, &plane.v2, &plane.d2, &plane.g2, &plane.obs2, &plane.vor2);
}

void
vorupdat(void)
{

	vupdate(&v1);
	vupdate(&v2);
}

void
vinit(Vor *vp, int x, int y, Nav **np, Nav **dm, Nav **gs, ushort *ob, ushort *q)
{

	vp->I.min.x = plane.origx + x*plane.side;
	vp->I.min.y = plane.origy + y*plane.side;
	vp->I.max.x = vp->I.min.x + plane.side;
	vp->I.max.y = vp->I.min.y + plane.side;

	d_bfree(vp->i);
	d_bfree(vp->t);
	vp->i = d_balloc(vp->I, 0);
	vp->t = d_balloc(vp->I, 0);

	d_circle(vp->i, addpt(vp->I.min, Pt(plane.side24, plane.side24)),
		plane.side24, LEVEL);
	d_circle(vp->i, addpt(vp->I.min, Pt(plane.side24, plane.side24)),
		plane.side14, LEVEL);
	vp->obs.x = vp->I.min.x + plane.side24 - (3*W)/2;
	vp->obs.y = vp->I.min.y + H/2;
	vp->from.x = vp->I.min.x + plane.side24 - W/2;
	vp->from.y = vp->I.min.y + (3*H)/2;
	vp->freq.x = vp->I.min.x + plane.side24 - 3*W;
	vp->freq.y = vp->I.min.y + plane.side - (3*H)/2;
	vp->dme.x = vp->I.min.x + plane.side24 - (5*W)/2;
	vp->dme.y = vp->I.min.y + plane.side - (5*H)/2;
	vp->np = np;
	vp->dm = dm;
	vp->gs = gs;
	vp->ob = ob;
	vp->q = q;
	vp->nx = NONE;
	vp->ny = NONE;
	strcpy(vp->s1, "");
	strcpy(vp->s2, "");
	strcpy(vp->s3, "");
	strcpy(vp->s4, "");
}

void
vupdate(Vor *vp)
{
	Nav *np;
	int t1, t2;
	int nx, ny, ob, q;
	long a, d, z;

	np = *(vp->np);
	if(plane.nbut && !plane.obut) {
		t1 = hit(vp->obs.x, vp->obs.y, 3);
		if(t1 >= 0) {
			*(vp->ob) = mod3(*(vp->ob), t1);
			while((*vp->ob) >= 400)
				(*vp->ob) -= 400;
		}
		t1 = hit(vp->freq.x, vp->freq.y, 6);
		if(t1 >= 0)
			*(vp->q) = mod6(*(vp->q), t1);
		t1 = hit(vp->from.x, vp->from.y, 1);
		if(t1 >= 0)
		if(np) {
			if(!(np->flag & ILS)) {
				a = bearto(np);
				if(vp->s3[0] == 'T')
					a += DEG(180);
				*(vp->ob) = deg(a);
			} else
				*(vp->ob) = np->obs;
		}
	}
	ob = *(vp->ob);
	q = *(vp->q);

	d_copy(vp->t, vp->I, vp->i);
	strcpy(vp->s1, "XXX");
	dconv(ob, vp->s1, 3);
	d_string(vp->t, vp->obs, font, vp->s1);

	strcpy(vp->s2, "1XX.XX");
	dconv(q/100, vp->s2+1, 2);
	dconv(q%100, vp->s2+4, 2);
	d_string(vp->t, vp->freq, font, vp->s2);

	nx = NONE;
	ny = NONE;

	t2 = 'X';
	if(np) {
		t1 = 0;
		t2 = 'T';
		if(np->flag & ILS)
			ob = np->obs;
		a = bearto(np) - DEG(ob);
		a &= 2*PI - 1;
		if(a >= DEG(180)) {
			t1 = 1;
			a = DEG(360) - a;
		}
		if(a > DEG(90)) {
			t2 = 'F';
			a = DEG(180) - a;
		}
		if(np->flag & ILS)
			a *= 4;
		if(a > DEG(10))
			a = DEG(10);
		if(t1)
			a = -a;
		a = muldiv(a, plane.side14, DEG(10));
		nx = plane.side24 + a;
	}
	vp->s3[0] = t2;
	vp->s3[1] = 0;
	d_string(vp->t, vp->from, font, vp->s3);

	np = *(vp->dm);
	strcpy(vp->s4, "XXX.X");
	if(np && np->flag & DME) {
		d = fdist(np);
		z = plane.z - np->z;
		t1 = ihyp(d, z)*10 / MILE(1);
		if(t1 > 9999)
			t1 = 9999;
		dconv(t1/10, vp->s4, 3);
		dconv(t1%10, vp->s4+4, 1);
	}

	np = *(vp->gs);
	if(np && np->flag & GS) {
		d = fdist(np);
		z = plane.z - np->z;
		if(np->flag & DME) {
			t1 = ihyp(d, z)*10 / MILE(1);
			if(t1 > 9999)
				t1 = 9999;
			dconv(t1/10, vp->s4, 3);
			dconv(t1%10, vp->s4+4, 1);
		}
		t1 = 0;
		a = iatan(z, d) - DEG(3);
		a &= 2*PI - 1;
		if(a >= DEG(180)) {
			t1 = 1;
			a = DEG(360) - a;
		}
		if(a > DEG(1))
			a = DEG(1);
		a = muldiv(a, plane.side14, DEG(1));
		if(t1)
			a = -a;
		ny = plane.side24 + a;
	}
	d_string(vp->t, vp->dme, font, vp->s4);

	vp->nx = nx;
	vp->ny = ny;
	cross(vp->t, vp->nx, vp->ny, vp->I.min.x, vp->I.min.y);

	d_copy(D, vp->I, vp->t);
}

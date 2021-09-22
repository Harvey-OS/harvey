#include <u.h>
#include <libc.h>
#include "obj.h"
#include "sphere.h"

#define INCR    (20/RADE)
#define MDIS    (200/RADE)

extern	Ft	ft[];
extern	int     nft;

double	rad;
Obj     obj[1000];
char	*fargv[] =
{
	"here", 0
};

extern	Obj*	look(char*);
extern	void	doft(Obj*, Obj*);
extern	double	fdist(double, double, double, double);
extern	double	xdist(double, double, double, double);

void
main(int argc, char *argv[])
{
	Obj *p, *p1;
	int i;

	rad = 0;

	ARGBEGIN {
	default:
		fprint(2, "usage: a.out\n");
		exits(0);
	case 'd':
	case 'r':
		rad = atof(ARGF())/RADE;
		break;
	} ARGEND

	if(argc == 0) {
		argc = 1;
		argv = fargv;
	}
	for(i=0; i<nft; i++) {
		ft[i].lat *= RADIAN;
		ft[i].lng *= RADIAN;
	}
	p = ONULL;
	load(argc+1, argv-1, obj);
	for(i=0; i<argc; i++) {
		p1 = look(argv[i]);
		if(p1 == ONULL)
			fprint(2, "cannot find %s\n", argv[i]);
		if(p != ONULL && p1 != ONULL)
			doft(p, p1);
		p = p1;
	}
	if(argc < 2 && p != ONULL)
		doft(p, p);
	exits(0);
}

Obj*
look(char *s)
{
	Obj *p;

	for(p=obj; p->name[0]; p++)
		if(ocmp(s, p))
			return p;
	return ONULL;
}

void
probj(Ft *b)
{
	Ft *p, *q;
	int i, j;

	// print all objects at this place
	// never print same name twice
	for(i=0, p=ft; i<nft; i++, p++)
	if(b->lat == p->lat && b->lng == p->lng) {
		for(j=0, q=ft; j<nft; j++, q++)
		if(strcmp(p->name, q->name) == 0) {
			if(q->hit == 0) {
				print("%s\n", q->name);
				q->hit = 1;
			}
			break;
		}
	}
}

void
doft(Obj *o1, Obj *o2)
{
	Ft *p, *b;
	double d, bd;
	double lat3, lng3, dis, tdis, ang;
	int i;

	b = 0;
	dis = 1/RADE;
	tdis = xdist(o1->lat, o1->lng, o2->lat, o2->lng);
	if(tdis < dis)
		tdis = dis;
	ang = sphere(MYPI/2-o1->lat, tdis, MYPI/2.-o2->lat,
		MISSING, MISSING, UNKNOWN);

loop:
	lat3 = sphere(MYPI/2-o1->lat, UNKNOWN, dis,
		MISSING, ang, MISSING);
	lat3 = MYPI/2 - lat3;
	lng3 = sphere(MYPI/2-o1->lat, MYPI/2-lat3, dis,
		MISSING, ang, UNKNOWN);
	if(o1->lng > o2->lng)
		lng3 = o1->lng - lng3;
	else
		lng3 = o1->lng + lng3;
	bd = MYPI;
	for(i=0, p=ft; i<nft; i++, p++) {
		d = fdist(lat3, lng3, p->lat, p->lng);
		if(d < bd) {
			bd = d;
			b = p;
		}
		if(rad > 0 && d < rad)
			probj(p);
	}

	probj(b);

	if(dis < tdis) {
		dis += INCR;
		if(dis > tdis)
			dis = tdis + 1/RADE;
		goto loop;
	}
}

double
xdist(double lat1, double lng1, double lat2, double lng2)
{
	double a;
	static double l1, sl1, cl1;

	if(lat1 != l1) {
		l1 = lat1;
		sl1 = sin(lat1);
		cl1 = cos(lat1);
	}
	a = sl1 * sin(lat2) +
		cl1 * cos(lat2) *
		cos(lng1 - lng2);
	a = atan2(xsqrt(1 - a*a), a);
	if(a < 0.0)
		a = -a;
	return a;
}

double
fdist(double lat1, double lng1, double lat2, double lng2)
{
	double a, b, c, d;
	static double l1, cl1;

	if(lat1 != l1) {
		l1 = lat1;
		cl1 = cos(lat1);
	}
	a = lat1 - lat2;
	if(a < 0.)
		a = -a;
	b = (lng1 - lng2) * cl1;
	if(b < 0)
		b = -b;
	if(a > b)
		d = a + b/2;
	else
		d = b + a/2;
	if(d > MDIS)
		return d;
	c = a*a + b*b;
	return d + (c - d*d)/(2*d);
}

#include <u.h>
#include <libc.h>
#include "sphere.h"
#include "obj.h"

double	MAXD1	= 0.0;
#define	NXOBJ	10000

typedef
struct
{
	Obj	o;
	double	clat;
	double	slat;
	double	c12;
	double	s12;
	double	dist1;
	double	dist2;
} Xobj;
Xobj	obj[NXOBJ];

extern	Mag	mag[];
#define	XNULL	((Xobj*)0)
double	dist1;
double	dist2;

extern	void	load1(int, char*[]);
extern	void	load2(int, char*[]);
extern	void	path(Xobj*, Xobj*, Obj*);
extern	int	cmp(void*, void*);

void
main(int argc, char *argv[])
{
	int i, f;

	MAXD1 = 10.0;
	fmtinstall('O', Ofmt);
	if(argc > 1 && argv[1][0] == '-') {
		MAXD1 = atof(&argv[1][1]);
		argv++;
		argc--;
	}
print("maxd1 = %.1f\n", MAXD1);
	load1(argc, argv);
	f = 0;
	for(i=1; i<argc; i++) {
		if(!ocmp(argv[i], &obj[i-1].o)) {
			print("cannot find %s\n", argv[i]);
			f++;
		}
	}
	if(f)
		exits("error");
	load2(argc, argv);
	exits(0);
}

void
load1(int argc, char *argv[])
{
	File o[300], *fo;
	Obj *op;
	int c, i, j, f;

	f = open(oname, 0);
	if(f < 0) {
		print("cannot open %s\n", oname);
		exits("open");
	}

	if(mkhere(o)) {
		c = 1;
		goto srch;
	}

loop:
	c = read(f, o, sizeof(o));
	c /= sizeof(o[0]);
	if(c <= 0) {
		close(f);
		return;
	}

srch:
	for(i=1; i<argc; i++) {
		fo = o;
		for(j=0; j<c; j++) {
			if(focmp(argv[i], &o[j])) {
				op = &obj[i-1].o;
				memcpy(op->name, fo->name, sizeof(op->name));
				op->type = fo->type;
				op->flag = fo->flag;
				op->lat = atof(fo->lat);
				op->lng = atof(fo->lng);
				op->freq = atof(fo->freq);
			}
			fo++;
		}
	}
	goto loop;
}

int
cmp(void *a1, void *a2)
{
	Xobj *p1, *p2;
	double d1, d2;

	p1 = a1;
	p2 = a2;
	if(p1->dist1 < p2->dist1)
		return -1;
	if(p1->dist1 > p2->dist1)
		return 1;
	d1 = p1->dist2;
	if(d1 < 0)
		d1 = -d1;
	d2 = p2->dist2;
	if(d2 < 0.)
		d2 = -d2;
	if(p1->dist1 == 0) {
		if(d1 > d2)
			return -1;
		if(d1 < d2)
			return 1;
		return 0;
	}
	if(d1 < d2)
		return -1;
	if(d1 > d2)
		return 1;
	return 0;
}

void
load2(int argc, char *argv[])
{
	File o[300], *fp;
	Obj oj;
	Xobj *op, *np, *p, *bp;
	int c, j, f;
	double d, t;

	USED(argc);
	USED(argv);
	bp = 0;
	f = open(oname, 0);
	if(f < 0) {
		print("cannot open %s\n", oname);
		exits("open");
	}
	t = 0.;
	for(op = obj; op->o.name[0]; op++) {
		if(op >= obj + nelem(obj)) {
			fprint(2, "too many objects\n");
			exits("size");
		}
		d = sin(op->o.lat);
		op->clat = d;
		op->slat = xsqrt(1 - d*d);
		if(op > obj) {
			d = (op-1)->clat*op->clat +
				(op-1)->slat*op->slat*
				cos((op-1)->o.lng - op->o.lng);
			(op-1)->c12 = d;
			(op-1)->s12 = xsqrt(1 - d*d);
			t += dist(&(op-1)->o, &op->o);
		}
		op->dist1 = t;
	}
	np = op-1;

	if(mkhere(o)) {
		c = 1;
		goto srch;
	}

loop:
	c = read(f, o, sizeof(o));
	c /= sizeof(o[0]);
	if(c <= 0) {
		close(f);
		np++;
		qsort(np, (int)(op-np), sizeof(*np), cmp);
		for(p=np; p<op; p++) {
			print("%O %8.2f %8.2f", &p->o, p->dist1, p->dist2);
			if(p->dist2 == 0)
			for(bp=p+1; bp<op; bp++) {
				if(bp->dist2)
					continue;
				print("    %O", &bp->o);
				print(" %3.0f/%.2f",
					angl(&(p->o), &(bp->o)) + magvar(&(p->o)),
					dist(&(p->o), &(bp->o)));
				break;
			}
			print("\n");
		}
		return;
	}

srch:
	for(j=0; j<c; j++) {
				fp = o+j;
				memcpy(oj.name, fp->name, sizeof(oj.name));
				oj.type = fp->type;
				oj.flag = fp->flag;
				oj.lat = atof(fp->lat);
				oj.lng = atof(fp->lng);
				oj.freq = atof(fp->freq);
		t = 0;
		d = MAXD1+1.;
		for(p = obj; p < np; p++) {
			path(p, p+1, &oj);
			if(dist2 < d) {
				t = dist1;
				d = dist2;
				bp = p;
				if(p->o.lat == oj.lat)
				if(p->o.lng == oj.lng)
					d = 0;
				if((p+1)->o.lat == oj.lat)
				if((p+1)->o.lng == oj.lng)
					d = 0;
			}
		}
		if(d >= MAXD1)
			continue;
		if(d)
		if("10001100"[o[j].type] == '1')
			continue;
		op->o = oj;
		op->dist1 = t;
		t = angl(&(bp->o), &oj) -
		    angl(&(bp->o), &((bp+1)->o));
		while(t < 0.)
			t += 360.;
		if(t >= 180.)
			d = -d;
		op->dist2 = d;
		op++;
	}
	goto loop;
}

void
path(Xobj *o1, Xobj *o2, Obj *o3)
{
	double c1, c2, c3;
	double s1, s2, s3;
	double c12, c13, c23;
	double s13, s12, s23;
	double a1, a2;

	c1 = o1->clat;
	c2 = o2->clat;
	c3 = sin(o3->lat);
	s1 = o1->slat;
	s2 = o2->slat;
	s3 = xsqrt(1 - c3*c3);

	c12 = o1->c12;
	c13 = c1*c3 + s1*s3*cos(o1->o.lng - o3->lng);
	c23 = c2*c3 + s2*s3*cos(o2->o.lng - o3->lng);

	if(c13 < c12*c23) {
		s23 = xsqrt(1 - c23*c23);
		dist1 = o2->dist1;
		dist2 = atan2(s23, c23)*RADE;
		return;
	}
	a1 = c23 - c12*c13;
	s13 = xsqrt(1 - c13*c13);
	if(a1 < 0.) {
		dist1 = o1->dist1;
		dist2 = atan2(s13, c13)*RADE;
		return;
	}
	s12 = o1->s12;
	a2 = s12 * s13;
	if(a2 > 1e-10)
		a1 /= a2;
	a1 = xsqrt(1 - a1*a1) * s13;
	a2 = xsqrt(1 - a1*a1);
	a1 = atan2(a1, a2);
	if(a2 > 1e-10)
		a2 = c13/a2;
	dist1 = atan2(xsqrt(1 - a2*a2), a2)*RADE;
	dist1 += o1->dist1;
	dist2 = a1*RADE;
}

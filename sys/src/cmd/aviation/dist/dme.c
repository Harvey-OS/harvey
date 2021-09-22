#include <u.h>
#include <libc.h>
#include "obj.h"
#include "sphere.h"

Obj	obj[100];

extern	Obj*	look(char*);

void
main(int argc, char *argv[])
{
	Obj *p;
	double d, r, t, u;

	if(argc < 4) {
		fprint(2, "dme place radial dist [name]\n");
		exits("argc");
	}
	load(2, argv, obj);
	p = look(argv[1]);
	if(p == ONULL) {
		fprint(2, "cannot find %s\n", argv[1]);
		exits("args");
	}

	r = anorm(atof(argv[2]) - magvar(p)) * RADIAN;
	d = atof(argv[3]);
	if(d < 1.0e-3)
		d = 1.0e-3;
	d = d/RADE;

	t = sphere(UNKNOWN, d, MYPI/2. - p->lat,
		r, MISSING, MISSING);
	u = anorm((MYPI/2. - t)/RADIAN);

	t = sphere(t, d, MYPI/2. - p->lat,
		r, UNKNOWN, MISSING);
	if(r <= MYPI)
		t = p->lng - t;
	else
		t = p->lng + t;
	t = anorm(t/RADIAN);
	if(argc > 4)
		print("%s,4,0,0,", argv[4]);
	else
		print("x,4,0,0,");
	print("	%.6f, %.6f,\n", u, t);
	exits(0);
}

Obj*
look(char *s)
{
	Obj *p;

	for(p=obj; p->name[0]; p++)
		if(ocmp(s, p))
			return(p);
	return ONULL;
}

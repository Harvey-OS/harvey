#include <u.h>
#include <libc.h>
#include "obj.h"
#include "sphere.h"

double
xsqrt(double a)
{

	if(a < 0)
		return 0;
	return sqrt(a);
}

double
dist(Obj *o1, Obj *o2)
{
	double a;

	a = sin(o1->lat) * sin(o2->lat) +
		cos(o1->lat) * cos(o2->lat) *
		cos(o1->lng - o2->lng);
	a = atan2(xsqrt(1 - a*a), a);
	if(a < 0)
		a = -a;
	return a * RADE;
}

double
angl(Obj *o1, Obj *o2)
{
	double a;

	a = sphere(MYPI/2.-o2->lat, MYPI/2.-o1->lat, MISSING,
		UNKNOWN, MISSING, fabs(o1->lng-o2->lng));
	if(o1->lng < o2->lng)
		a = 2.0*MYPI - a;
	return a/RADIAN;
}

double
anorm(double a)
{

	while(a < 0)
		a += 360;
	while(a >= 360)
		a -= 360;
	return a;
}

int
Ofmt(Fmt *f)
{
	char s[100];
	Obj *o;

	o = va_arg(f->args, Obj*);
	sprint(s, "%s.%c", o->name, "-vnaistV"[o->type]);
	switch(o->type)
	{
	default:
		sprint(s, "%5.5s.?(%d)", o->name, o->type);
		break; 
	case 1:
	case 2:
	case 3:
	case 6:
	case 7:
		sprint(s, "%4.4s.%c/%5.1f", o->name,
			"-vnaistV"[o->type], o->freq);
		if(s[7] == ' ')
			s[7] = '0';
		if(s[8] == ' ')
			s[8] = '0';
		break;

	case 4:
	case 5:
		sprint(s, "%5.5s.%c    ", o->name, "-vnaistV"[o->type]);
		break;
	}
	return fmtstrcpy(f, s);
}

int
ocmp(char *p1, Obj *o)
{
	char *p2;
	int i;

	p2 = o->name;
	for(i=0; i<NNAME; i++) {
		if(*p1++ == *p2) {
			if(*p2++ == 0)
				return 1;
			continue;
		}
		break;
	}
	if(*p2 != 0)
		return 0;
	if(p1[-1] != '.')
		return 0;
	i = o->type;
	if(*p1 == 'v' && (i == 1 || i == 7))
		return 1;
	if(*p1 == 't' && i == 6)
		return 1;
	if(*p1 == 'n' && i == 2)
		return 1;
	if(*p1 == 'a' && i == 3)
		return 1;
	if(*p1 == 'i' && i == 4)
		return 1;
	if(*p1 == 's' && i == 5)
		return 1;
	return 0;
}

int
focmp(char *p1, File *o)
{
	char *p2;
	int i;

	p2 = o->name;
	for(i=0; i<NNAME; i++) {
		if(*p1++ == *p2) {
			if(*p2++ == 0)
				return 1;
			continue;
		}
		break;
	}
	if(*p2 != 0)
		return 0;
	if(p1[-1] != '.')
		return 0;
	i = o->type;
	if(*p1 == 'v' && (i == 1 || i == 7))
		return 1;
	if(*p1 == 't' && i == 6)
		return 1;
	if(*p1 == 'n' && i == 2)
		return 1;
	if(*p1 == 'a' && i == 3)
		return 1;
	if(*p1 == 'i' && i == 4)
		return 1;
	if(*p1 == 's' && i == 5)
		return 1;
	return 0;
}

char	here[100];
#define	hname		"/lib/sky/here"

int
mkhere(File *o)
{
	int f, n, i;
	double d;

	f = open(hname, 0);
	if(f < 0)
		return 0;
	n = read(f, here, sizeof(here));
	close(f);

	if(n < 0 || n >= sizeof(here))
		return 0;

	memset(o, 0, sizeof(*o));
	strcpy(o->name, "here");
	o->type = 4;
	o->flag = 0;
	sprint(o->freq, "%.2f", 0.0);

	for(i=0;; i++) {
		if(here[i] == 0)
			return 0;
		if(here[i] != ' ')
			break;
	}
	d = atof(here+i);
	sprint(o->lat, "%.6f", d*RADIAN);

	for(;; i++) {
		if(here[i] == 0)
			return 0;
		if(here[i] == ' ')
			break;
	}
	for(;; i++) {
		if(here[i] == 0)
			return 0;
		if(here[i] != ' ')
			break;
	}

	d = atof(here+i);
	sprint(o->lng, "%.6f", d*RADIAN);
	return 1;
}

void
load(int argc, char *argv[], Obj *obj)
{
	int i, j, c, f;
	File o[300], *fo;
	Obj *op;


	f = open(oname, 0);
	if(f < 0) {
		print("cannot open %s\n", oname);
		exits("open");
	}
	op = obj;

	if(mkhere(o)) {
		c = 1;
		goto srch;
	}

loop:
	c = read(f, o, sizeof(o));
	c /= sizeof(o[0]);
	if(c <= 0) {
		close(f);
		op->name[0] = 0;
		return;
	}

srch:
	for(i=1; i<argc; i++) {
		fo = o;
		for(j=0; j<c; j++) {
			if(focmp(argv[i], fo)) {
				memcpy(op->name, fo->name, sizeof(op->name));
				op->type = fo->type;
				op->flag = fo->flag;
				op->lat = atof(fo->lat);
				op->lng = atof(fo->lng);
				op->freq = atof(fo->freq);
				op++;
			}
			fo++;
		}
	}
	goto loop;
}

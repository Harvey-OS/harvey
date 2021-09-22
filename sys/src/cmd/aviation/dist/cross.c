#include <u.h>
#include <libc.h>
#include "obj.h"

double	MAXD1	= 0.0;
Obj	obj[300];
Obj	litobj;

extern	Obj*	look(char*);
extern	void	load1(int, char*[]);
extern	void	load2(int, char*[]);
extern	Obj*	literal(char*, char*);
extern	int	fpoint(char*);

void
main(int argc, char *argv[])
{
	int i;
	Obj *p, *q;
	double a1, a2, a3;

	fmtinstall('O', Ofmt);
	while(argc > 1 && argv[1][0] == '-') {
		a1 = atof(&argv[1][1]);
		if(MAXD1 == 0.0)
			MAXD1 = a1; else
			print("unknown argument %s\n", argv[1]);
		argv++;
		argc--;
	}
	if(MAXD1 == 0.0)
		MAXD1 = 50.0;
	load1(argc, argv);
	load2(argc, argv);
	for(i=1; i<argc; i++) {
		q = ONULL;
		if(i+1 < argc)
			q = literal(argv[i], argv[i+1]);
		if(q == ONULL)
			q = look(argv[i]); else
			i++;
		if(q == ONULL) {
			print("cannot find %s\n", argv[i]);
			continue;
		}
		for(p=obj; p->name[0]; p++) {
			a1 = dist(p, q);
			if(a1 <= 0.0)
				continue;
			if(a1 > MAXD1)
				continue;
			a2 = anorm(angl(p, q));
			a3 = anorm(a2 + magvar(p));
			print("%O %O %3.0f/%.1f\n",
				p, q, a3, a1);
		}
	}
	exits(0);
}

Obj*
look(char *s)
{
	Obj *p;

	for(p=obj; p->name[0]; p++) {
		if(p->name[0] != s[0])
			goto no;
		if(p->name[1] != s[1])
			goto no;
		if(p->name[2] != s[2])
			goto no;
		if(p->name[3] == s[3])
			return(p);
	no:;
	}
	return(ONULL);
}

void
load1(int argc, char *argv[])
{
	File o[300], *fo;
	Obj *op, *q;
	char *p1, *p2;
	int c, i, j, f;

	f = open(oname, 0);
	if(f < 0) {
		print("cannot open %s\n", oname);
		exits("open");
	}
	op = &obj[0];
	for(i=1; i<argc-1; i++) {
		q = literal(argv[i], argv[i+1]);
		if(q != ONULL) {
			*op = *q;
			op++;
			i++;
		}
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
		op->name[0] = 0;
		return;
	}

srch:
	for(i=1; i<argc; i++)
	for(j=0; j<c; j++) {
		p1 = argv[i];
		p2 = o[j].name;
		if(*p1++ != *p2++)
			continue;
		if(*p1++ != *p2++)
			continue;
		if(*p1++ != *p2++)
			continue;
		if(*p1 != *p2)
			continue;

		fo = o+j;
		memcpy(op->name, fo->name, sizeof(op->name));
		op->type = fo->type;
		op->flag = fo->flag;
		op->lat = atof(fo->lat);
		op->lng = atof(fo->lng);
		op->freq = atof(fo->freq);
		op++;
	}
	goto loop;
}

void
load2(int argc, char *argv[])
{
	File o[300], *fo;
	Obj *op, *np, *p;
	int c, j, f, no, yes;
	double d;

	USED(argc);
	USED(argv);
	f = open(oname, 0);
	if(f < 0) {
		print("cannot open %s\n", oname);
		exits("open");
	}
	for(op = obj; op->name[0]; op++)
		;
	np = op;

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
	for(j=0; j<c; j++) {
		no = 0;
		yes = 0;
		fo = o+j;
		memcpy(op->name, fo->name, sizeof(op->name));
		op->type = fo->type;
		op->flag = fo->flag;
		op->lat = atof(fo->lat);
		op->lng = atof(fo->lng);
		op->freq = atof(fo->freq);
		for(p = obj; p < np; p++) {
			d = dist(p, op);
			if(d <= 0.0)
				no = 1;
			if(d <= MAXD1)
				yes = 1;
		}
		if(no)
			continue;
		if(!yes)
			continue;
		op++;
	}
	goto loop;
}

Obj*
literal(char *s1, char *s2)
{

	if(!fpoint(s1))
		return ONULL;
	if(!fpoint(s2))
		return ONULL;
	litobj.lat = atof(s1) * RADIAN;
	litobj.lng = atof(s2) * RADIAN;
	litobj.name[0] = '#';
	litobj.name[1] = '#';
	litobj.name[2] = '#';
	litobj.name[3] = '\0';
	return &litobj;
}

fpoint(char *s)
{

	for(; *s; s++) {
		if(*s == '.')
			continue;
		if(*s >= '0' && *s <= '9')
			continue;
		return 0;
	}
	return 1;
}

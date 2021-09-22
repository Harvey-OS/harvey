#include <u.h>
#include <libc.h>
#include "obj.h"
#include "sphere.h"

Obj	obj[100];
double	conv1	= 1.15078;	/* miles */
double	conv	= 1.0;		/* nautical miles */

extern	Obj*	look(char*);
extern	void	pname(Obj*);

void
main(int argc, char *argv[])
{
	Obj *p, *p1, *pf, *pl;
	double a1, a2, a3, mv, tot;
	int i, mflag;

	mflag = 0;
	ARGBEGIN {
	default:
		fprint(2, "usage: a.out\n");
		exits(0);
	case 'm':
	case 't':
		mflag = 1;
		break;
	} ARGEND

	fmtinstall('O', Ofmt);
	if(argc < 2) {
		print("dist place1 place2 ...\n");
		exits("argc");
	}
	pf = ONULL;
	pl = ONULL;
	tot = 0.0;
	p = ONULL;
	load(argc+1, argv-1, obj);
	for(i=0; i<argc; i++) {
		p1 = look(argv[i]);
		if(p1 == ONULL)
			print("cannot find %s\n", argv[i]);
		if(p != ONULL && p1 != ONULL) {
			if(pf == ONULL)
				pf = p;
			else
				pl = p1;
			mv = magvar(p);
			a1 = anorm(angl(p, p1));
			a2 = anorm(a1 + mv);
			a3 = dist(p, p1);
			if(mflag) {
				if(mv >= 0)
					print("%O %O %3.1fT/%.3f (+%.1fw)\n",
						p, p1, a1, a3*conv, mv);
				else
					print("%O %O %3.1fT/%.3f (-%.1fe)\n",
						p, p1, a1, a3*conv, -mv);
			} else
				print("%O %O %3.0f/%.1f\n", p, p1, a2, a3*conv);
			tot += a3;
		}
		p = p1;
	}
	if(pf != ONULL && pl != ONULL)
		print("total = %.2f; direct = %.2f\n", tot*conv, dist(pf, pl)*conv);
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

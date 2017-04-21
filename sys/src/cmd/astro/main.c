/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "astro.h"

char*	herefile = "/lib/sky/here";

void
main(int argc, char *argv[])
{
	int i, j;
	double d;

	pi = atan(1.0)*4;
	pipi = pi*2;
	radian = pi/180;
	radsec = radian/3600;
	converge = 1.0e-14;

	fmtinstall('R', Rconv);
	fmtinstall('D', Dconv);

	per = PER;
	deld = PER/NPTS;
	init();
	args(argc, argv);
	init();

loop:
	d = day;
	pdate(d);
	if(flags['p'] || flags['e']) {
		print(" ");
		ptime(d);
		pstime(d);
	}
	print("\n");
	for(i=0; i<=NPTS+1; i++) {
		setime(d);

		for(j=0; objlst[j]; j++) {
			(*objlst[j]->obj)();
			setobj(&objlst[j]->point[i]);
			if(flags['p']) {
				if(flags['m'])
					if(strcmp(objlst[j]->name, "Comet"))
						continue;
				output(objlst[j]->name, &objlst[j]->point[i]);
			}
		}
		if(flags['e']) {
			d = dist(&eobj1->point[i], &eobj2->point[i]);
			print("dist %s to %s = %.4f\n", eobj1->name, eobj2->name, d);
		}
//		if(flags['p']) {
//			pdate(d);
//			print(" ");
//			ptime(d);
//			print("\n");
//		}
		if(flags['p'] || flags['e'])
			break;
		d += deld;
	}
	if(!(flags['p'] || flags['e']))
		search();
	day += per;
	nperiods -= 1;
	if(nperiods > 0)
		goto loop;
	exits(0);
}

void
args(int argc, char *argv[])
{
	char *p;
	long t;
	int f, i;
	Obj2 *q;

	memset(flags, 0, sizeof(flags));
	ARGBEGIN {
	default:
		fprint(2, "astro [-adeklmopst] [-c nperiod] [-C tperiod]\n");
		exits(0);

	case 'c':
		nperiods = 1;
		p = ARGF();
		if(p)
			nperiods = atol(p);
		flags['c']++;
		break;
	case 'C':
		p = ARGF();
		if(p)
			per = atof(p);
		break;
	case 'e':
		eobj1 = nil;
		eobj2 = nil;
		p = ARGF();
		if(p) {
			for(i=0; (q=objlst[i]) != nil; i++) {
				if(strcmp(q->name, p) == 0)
					eobj1 = q;
				if(strcmp(q->name1, p) == 0)
					eobj1 = q;
			}
			p = ARGF();
			if(p) {
				for(i=0; (q=objlst[i]) != nil; i++) {
					if(strcmp(q->name, p) == 0)
						eobj2 = q;
					if(strcmp(q->name1, p) == 0)
						eobj2 = q;
				}
			}
		}
		if(eobj1 && eobj2) {
			flags['e']++;
			break;
		}
		fprint(2, "cant recognize eclipse objects\n");
		exits("eflag");

	case 'a':
	case 'd':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'o':
	case 'p':
	case 's':
	case 't':
		flags[ARGC()]++;
		break;
	} ARGEND
	if(*argv){
		fprint(2, "usage: astro [-dlpsatokm] [-c nday] [-e obj1 obj2]\n");
		exits("usage");
	}

	t = time(0);
	day = t/86400. + 25567.5;
	if(flags['d'])
		day = readate();
	if(flags['j'])
		print("jday = %.4f\n", day);
	deltat = day * .001704;
	if(deltat > 32.184)		// assume date is utc1
		deltat = 32.184;	// correct by leap sec
	if(flags['t'])
		deltat = readdt();

	if(flags['l']) {
		fprint(2, "nlat wlong elev\n");
		readlat(0);
	} else {
		f = open(herefile, OREAD);
		if(f < 0) {
			fprint(2, "%s?\n", herefile);
			/* btl mh */
			nlat = (40 + 41.06/60)*radian;
			awlong = (74 + 23.98/60)*radian;
			elev = 150 * 3.28084;
		} else {
			readlat(f);
			close(f);
		}
	}
}

double
readate(void)
{
	int i;
	Tim t;

	fprint(2, "year mo da hr min\n");
	rline(0);
	for(i=0; i<5; i++)
		t.ifa[i] = atof(skip(i));
	return convdate(&t);
}

double
readdt(void)
{

	fprint(2, "ΔT (sec) (%.3f)\n", deltat);
	rline(0);
	return atof(skip(0));
}

double
etdate(int32_t year, int mo, double day)
{
	Tim t;

	t.ifa[0] = year;
	t.ifa[1] = mo;
	t.ifa[2] = day;
	t.ifa[3] = 0;
	t.ifa[4] = 0;
	return convdate(&t) + 2415020;
}

void
readlat(int f)
{

	rline(f);
	nlat = atof(skip(0)) * radian;
	awlong = atof(skip(1)) * radian;
	elev = atof(skip(2)) * 3.28084;
}

double
fmod(double a, double b)
{
	return a - floor(a/b)*b;
}

#include "astro.h"

char*	herefile = "/lib/sky/here";

void
main(int argc, char *argv[])
{
	int i, j;
	double d;

	pi = atan(1.0)*4;
	pipi = 2*pi;
	radian = pi/180;
	radsec = radian/3600;

	fmtinstall('R', Rconv);
	fmtinstall('D', Dconv);

	args(argc, argv);
	init();
	deld = PER/NPTS;

loop:
	d = day;
	pdate(d);
	if(flags['p']) {
		print(" ");
		ptime(d);
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

		if(flags['p']) {
			if(flags['e']) {
				d = dist(&osun.point[0], &omoon.point[0]);
				print(" dist = %.4f\n", d);
			}
			break;
		}
		d += deld;
	}
	if(!(flags['p']))
		search();
	day += PER;
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
	int f;

	memset(flags, 0, sizeof(flags));
	ARGBEGIN {
	default:
		fprint(2, "unknown option '%c'\n", ARGC());
		break;

	case 'c':
		nperiods = 1;
		p = ARGF();
		if(p)
			nperiods = atol(p);
		break;
	case 'd':
	case 'l':
	case 'e':
	case 'p':
	case 's':
	case 'a':
	case 't':
	case 'o':
	case 'k':
	case 'm':
		flags[ARGC()]++;
		break;
	} ARGEND
	if(argc);
	if(*argv)
		fprint(2, "sorry, because of ARGS, flags need '-'\n");

	t = time(0);
	day = t/86400. + 25567.5;
	if(flags['d'])
		day = readate();
	deltat = day * .001704;
	if(flags['t'])
		deltat = readdt();

	if(flags['l']) {
		print("nlat wlong elev\n");
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

	print("year mo da hr min\n");
	rline(0);
	for(i=0; i<5; i++)
		t.ifa[i] = atof(skip(i));
	return convdate(&t);
}

double
readdt(void)
{

	print("ΔT (sec)\n");
	rline(0);
	return atof(skip(0));
}

double
etdate(long year, int mo, double day)
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

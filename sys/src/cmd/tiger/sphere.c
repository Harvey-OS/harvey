#include <all.h>

#define	SIDEA	001
#define	SIDEB	002
#define	SIDEC	004
#define	ANGLEA	010
#define	ANGLEB	020
#define	ANGLEC	040
#define	KNOWN(x)	(kf&(x))==(x)

static	void	exchg(double*, double*);
static	double	sph1(double, double, double);
static	double	sph2(double, double, double);
static	double	sph3(double, double, double, double*);
static	double	sph4(double, double, double, double*);
static	double	sph5(double, double, double);
static	double	sph6(double, double, double);

double
sphere(double sa, double sb, double sc, double aa, double ab, double ac)
{
	int uf, kf, i;
	double j;

again:
	uf = 0;
	kf = 0;
	for(i=0; i<6; i++) {
		if((&sa)[i] == UNKNOWN)
			uf |= 1<<i;
		else
		if((&sa)[i] != MISSING)
			kf |= 1<<i;
	}
	switch(uf)
	{
	default:
		j = UNKNOWN;
		break;

	case SIDEA:
		if(KNOWN(ANGLEA|ANGLEB|ANGLEC))
			j = sph1(aa, ab, ac);
		else
		if(KNOWN(ANGLEA|ANGLEB|SIDEC))
			j = sph3(aa, ab, sc, &j);
		else
		if(KNOWN(ANGLEA|ANGLEC|SIDEB))
			j = sph3(aa, ac, sb, &j);
		else
		if(KNOWN(ANGLEA|SIDEB|SIDEC))
			j = sph5(aa, sb, sc);
		else
			j = UNKNOWN;
		break;

	case SIDEB:
		exchg(&sa, &sb);
		exchg(&aa, &ab);
		goto again;

	case SIDEC:
		exchg(&sa, &sc);
		exchg(&aa, &ac);
		goto again;

	case ANGLEA:
		if(KNOWN(SIDEA|SIDEB|SIDEC))
			j = sph2(sa, sb, sc);
		else
		if(KNOWN(SIDEA|SIDEB|ANGLEC))
			j = sph4(sa, sb, ac, &j);
		else
		if(KNOWN(SIDEA|SIDEC|ANGLEB))
			j = sph4(sa, sc, ab, &j);
		else
		if(KNOWN(SIDEA|ANGLEB|ANGLEC))
			j = sph6(sa, ab, ac);
		else
			j = UNKNOWN;
		break;

	case ANGLEB:
		exchg(&sa, &sb);
		exchg(&aa, &ab);
		goto again;

	case ANGLEC:
		exchg(&sa, &sc);
		exchg(&aa, &ac);
		goto again;

	}
	return j;
}

static void
exchg(double *aa, double *ab)
{
	double t;

	t = *aa;
	*aa = *ab;
	*ab = t;
}

static double
sph1(double a, double b, double c)
{
	double e, x;

	if(a == 0.)
		goto bad;
	e = (a+b+c-PI) / 2;
	if(e == b || e == c)
		goto bad;
	x = (sin(e) * sin(e-a)) /
		(sin(e-b) * sin(e-c));
	if(x > 0)
		goto bad;
	x = atan2(sqrt(-x), 1) * 2;
	return x;

bad:
	return 0;
}

static double
sph2(double a, double b, double c)
{
	double p, x;

	p = (a+b+c)/2;
	if(p == 0 || p == c)
		goto bad;
	x = (sin(p-b) * sin(p-c)) /
		(sin(p) * sin(p-a));
	if(x < 0)
		goto bad;
	x = atan2(sqrt(x), 1) * 2;
	return x;

bad:
	return 0;
}

static double
sph3(double a, double b, double c, double *d)
{
	double x1, x2, x3;

/* has a singularity at 2.pi and 0 */
	x1 = tan(c/2);
	x2 = sin((a+b)/2);
	x2 = atan2(x1 * sin((a-b)/2), x2);
	x3 = cos((a+b)/2);
	x3 = atan2(x1 * cos((a-b)/2), x3);
	*d = x3 - x2;
	return x3 + x2;
}

static double
sph4(double a, double b, double c, double *d)
{
	double x1, x2, x3;

/* has a singularity at 2.pi and 0 */
	x1 = tan(c/2);
	x2 = x1 * sin((a+b)/2);
	x2 = atan2(sin((a-b)/2), x2);
	x3 = x1 * cos((a+b)/2);
	x3 = atan2(cos((a-b)/2), x3);
	*d = x3 - x2;
	return x3 + x2;
}

static double
sph5(double a, double b, double c)
{
	double ob, oc;

	ob = sph4(b, c, a, &oc);
	return sph1(a, ob, oc);
}

static double
sph6(double a, double b, double c)
{
	double ob, oc;

	ob = sph3(b, c, a, &oc);
	return sph2(a, ob, oc);
}

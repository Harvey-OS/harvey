#include <u.h>
#include <libc.h>

/*
 * gamma(x) computes the log of the absolute
 * value of the gamma function.
 * The sign of the gamma function is returned in the
 * external quantity signgam.
 *
 * The coefficients for expansion around zero
 * are #5243 from Hart & Cheney; for expansion
 * around infinity they are #5404.
 *
 * Calls log and sin.
 */
int	signgam;
static	double	goobie	= 0.9189385332046727417803297;
static	double	asym(double);
static	double	pos(double);
static	double	neg(double);

#define M 6
#define N 8
static double p1[] =
{
	0.83333333333333101837e-1,
	-.277777777735865004e-2,
	0.793650576493454e-3,
	-.5951896861197e-3,
	0.83645878922e-3,
	-.1633436431e-2,
};
static double p2[] =
{
	-.42353689509744089647e5,
	-.20886861789269887364e5,
	-.87627102978521489560e4,
	-.20085274013072791214e4,
	-.43933044406002567613e3,
	-.50108693752970953015e2,
	-.67449507245925289918e1,
	0.0,
};
static double q2[] =
{
	-.42353689509744090010e5,
	-.29803853309256649932e4,
	0.99403074150827709015e4,
	-.15286072737795220248e4,
	-.49902852662143904834e3,
	0.18949823415702801641e3,
	-.23081551524580124562e2,
	0.10000000000000000000e1,
};

static
double
asym(double arg)
{
	double n, argsq;
	int i;

	argsq = 1 / (arg*arg);
	n = 0;
	for(i=M-1; i>=0; i--)
		n = n*argsq + p1[i];
	return (arg-.5)*log(arg) - arg + goobie + n/arg;
}

static
double
pos(double arg)
{
	double n, d, s;
	int i;

	if(arg < 2)
		return pos(arg+1)/arg;
	if(arg > 3)
		return (arg-1)*pos(arg-1);

	s = arg - 2;
	n = 0;
	d = 0;
	for(i=N-1; i>=0; i--){
		n = n*s + p2[i];
		d = d*s + q2[i];
	}
	return n/d;
}

static double
neg(double arg)
{
	double temp;

	arg = -arg;
	temp = sin(PI*arg);
	if(temp == 0)
		return NaN();
	if(temp < 0)
		temp = -temp;
	else
		signgam = -1;
	return -log(arg*pos(arg)*temp/PI);
}

double
gamma(double arg)
{

	signgam = 1;
	if(arg <= 0)
		return neg(arg);
	if(arg > 8)
		return asym(arg);
	return log(pos(arg));
}

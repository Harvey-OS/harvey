#include <u.h>
#include <libc.h>
#include "sky.h"

double PI_180=0.0174532925199432957692369;
double TWOPI=6.2831853071795864769252867665590057683943387987502;

int
rint(char *p, int n)
{
	int i=0;

	while(*p==' ' && n)
		p++, --n;
	while(n--)
		i=i*10+*p++-'0';
	return i;
}

static double angledangle=(180./PI)*60.*60.*1000.;
DAngle
dangle(Angle angle)
{
	return angle*angledangle;
}

Angle
angle(DAngle dangle)
{
	return dangle/angledangle;
}

double
rfloat(char *p, int n)
{
	double i, d=0;

	while(*p==' ' && n)
		p++, --n;
	if(*p == '+')
		return rfloat(p+1, n-1);
	if(*p == '-')
		return -rfloat(p+1, n-1);
	while(*p == ' ' && n)
		p++, --n;
	if(n == 0)
		return 0.0;
	while(n-- && *p!='.')
		d = d*10+*p++-'0';
	if(n <= 0)
		return d;
	p++;
	i = 1;
	while(n--)
		d+=(*p++-'0')/(i*=10.);
	return d;
}

int
sign(int c)
{
	if(c=='-')
		return -1;
	return 1;
}

char*
hms(Angle a)
{
	static char buf[20];
	double x=DEG(a)/15;
	int h, m, s, ts;

	h = floor(x);
	x -= h;
	x *= 60;
	m = floor(x);
	x -= m;
	x *= 60;
	s = floor(x);
	x -= s;
	ts = 10*x;
	sprint(buf, "%dh%.2dm%.2d.%ds", h, m, s, ts);
	return buf;
}

char*
dms(Angle a)
{
	static char buf[20];
	double x=DEG(a);
	int sign, d, m, s, ts;

	sign='+';
	if(a<0){
		sign='-';
		x=-x;
	}
	d = floor(x);
	x -= d;
	x *= 60;
	m = floor(x);
	x -= m;
	x *= 60;
	s = floor(x);
	x -= s;
	ts = floor(10*x);
	sprint(buf, "%c%d°%.2d'%.2d.%d\"", sign, d, m, s, ts);
	return buf;
}

char*
hm(Angle a)
{
	static char buf[20];
	double x=DEG(a)/15;
	int h, m;

	h = floor(x);
	x -= h;
	x *= 60;
	m = floor(x);
	sprint(buf, "%dh%2dm", h, m);
	return buf;
}

char*
dm(Angle a)
{
	static char buf[20];
	double x=DEG(a);
	int sign, d, m, n;

	sign='+';
	if(a<0){
		sign='-';
		x=-x;
	}
	d = floor(x);
	x -= d;
	x *= 60;
	m = floor(x);
	x -= m;
	x *= 10;
	n = floor(x);
	sprint(buf, "%c%d°%.2d.%.1d'", sign, d, m, n);
	return buf;
}

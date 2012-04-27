#include "u.h"
#include "lib.h"
#include "dat.h"
#include "fns.h"
#include "error.h"

void
mallocsummary(void)
{
}

void
pagersummary(void)
{
}

int
iseve(void)
{
	return 1;
}

void
setswapchan(Chan *c)
{
	USED(c);
}

void
splx(int x)
{
	USED(x);
}

int
splhi(void)
{
	return 0;
}

int
spllo(void)
{
	return 0;
}

void
procdump(void)
{
}

void
scheddump(void)
{
}

void
killbig(void)
{
}

void
dumpstack(void)
{
}

void
xsummary(void)
{
}

void
rebootcmd(int argc, char **argv)
{
	USED(argc);
	USED(argv);
}

void
kickpager(void)
{
}

int
userwrite(char *a, int n)
{
	error(Eperm);
	return 0;
}

vlong
todget(vlong *p)
{
	if(p)
		*p = 0;
	return 0;
}

void
todset(vlong a, vlong b, int c)
{
	USED(a);
	USED(b);
	USED(c);
}

void
todsetfreq(vlong a)
{
	USED(a);
}

long
hostdomainwrite(char *a, int n)
{
	USED(a);
	USED(n);
	error(Eperm);
	return 0;
}

long
hostownerwrite(char *a, int n)
{
	USED(a);
	USED(n);
	error(Eperm);
	return 0;
}

void
todinit(void)
{
}

void
rdb(void)
{
}

void
setmalloctag(void *v, uintptr tag)
{
	USED(v);
	USED(tag);
}

int
postnote(Proc *p, int x, char *msg, int flag)
{
	USED(p);
	USED(x);
	USED(msg);
	USED(flag);
	return 0;
}

void
exhausted(char *s)
{
	panic("out of %s", s);
}

uvlong
fastticks(uvlong *v)
{
	if(v)
		*v = 1;
	return 0;
}


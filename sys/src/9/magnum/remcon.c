#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"devtab.h"

void
screeninit(int colormono)
{
	USED(colormono);
}

void
screenputs(char *s, int n)
{
	USED(s, n);
}

int
screenbits(void)
{
	return 1;
}

/*
 * fake keyboard and mouse
 */
int
mouseputc(IOQ *q, int c)
{
	USED(q, c);
	return 0;
}

void
mouseclock(void)
{
}

/*
 * return of 0 means use eia1
 */
int
kbdinit(void)
{
	return 0;
}

int
kbdintr(void)
{
	return 0;
}

void
buzz(int f, int d)
{
	USED(f, d);
}

void
lights(int l)
{
	USED(l);
}

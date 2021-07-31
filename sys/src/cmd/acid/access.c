#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <mach.h>
#define Extern extern
#include "acid.h"

#define	min(x, y)	((x) < (y) ? (x) : (y))

static	char	*relocmsg[] =
{
	"can't find text or data address",
	"can't find text address",
	"can't find data address",
};
static char	*readmsg[] =
{
	"can't read text or data",
	"can't read text",
	"can't read data",
};
static char	*writemsg[] =
{
	"can't write text or data",
	"can't write text",
	"can't write data",
};

static	int dbget(Map *, int, ulong, char *, int);
static	int dbput(Map *, int, ulong, char *, int);

/*
 * routines to get/put various types
 */

int
get4(Map *map, ulong addr, int space, long *x)
{
	if (dbget(map, space, addr, (char *)x, 4) == 0)
		return 0;
	*x = machdata->swal(*x);
	return (1);
}

int
get2(Map *map, ulong addr, int space, ushort *x)
{
	if (dbget(map, space, addr, (char *)x, 2) == 0)
		return 0;
	*x = machdata->swab(*x);
	return (1);
}

int
get1(Map *map, ulong addr, int space, uchar *x, int size)
{
	if (dbget(map, space, addr, (char *)x, size) == 0)
		return 0;
	return 1;
}

int
put4(Map *map, ulong addr, int space, long v)
{
	v = machdata->swal(v);
	return dbput(map, space, addr, (char *)&v, 4);
}

int
put2(Map *map, ulong addr, int space, ushort v)
{
	v = machdata->swab(v);
	return dbput(map, space, addr, (char *)&v, 2);
}

int
put1(Map *map, ulong addr, int space, uchar *v, int size)
{
	return dbput(map, space, addr, (char *)v, size);
}

static
dbget(Map *map, int s, ulong addr, char *buf, int size)
{
	switch (mget(map, s, addr, buf, size)) {
	case -1:
		if (++s < 0)
			s = 0;
		error(relocmsg[s]);
		return 0;
	case 0:
		if (++s < 0)
			s = 0;
		error("%s: %r", readmsg[s]);
		return 0;
	default:
		return 1;
	}
}

static
dbput(Map *map, int s, ulong addr, char *buf, int size)
{
	if(map != cormap && wtflag == 0)
		error("not in write mode");
	switch(mput(map, s, addr, buf, size)) {
	case -1:
		if (s < 0)
			s = 0;
		error(relocmsg[s]);
		return 0;
	case 0:
		if (s < 0)
			s = 0;
		error("%s: %r", writemsg[s]);
		return 0;
	default:
		return 1;
	}
}

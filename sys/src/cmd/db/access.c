/*
 * access the files:
 * read or write data from some address in some space
 */

#include "defs.h"
#include "fns.h"

#define	min(x, y)	((x) < (y) ? (x) : (y))

static	char	*relocmsg[] =
{
	"can't find text or data address",
	"can't find text address",
	"can't find data address",
	"can't find ublock address",
	"can't find register",
};
static char	*readmsg[] =
{
	"can't read text or data",
	"can't read text",
	"can't read data",
	"can't read ublock",
	"can't read register",
};
static char	*writemsg[] =
{
	"can't write text or data",
	"can't write text",
	"can't write data",
	"can't write ublock",
	"can't write register",
};

static	int dbget(Map *, int, ulong, char *, int);
static	int dbput(Map *, int, ulong, char *, int);

/*
 * routines to get/put various types
 */

int
get4(Map *map, ulong addr, int space, long *x)
{
	if (space == SEGNONE) {
		*x = dot;
		return 1;
	}
	if (dbget(map, space, addr, (char *)x, 4) == 0)
		return 0;
	*x = machdata->swal(*x);
	return (1);
}

int
get2(Map *map, ulong addr, int space, ushort *x)
{
	if (space == SEGNONE) {
		*x = (ushort) dot;
		return 1;
	}
	if (dbget(map, space, addr, (char *)x, 2) == 0)
		return 0;
	*x = machdata->swab(*x);
	return (1);
}

int
get1(Map *map, ulong addr, int space, uchar *x, int size)
{
	if (space == SEGNONE) {
		*x = (uchar) dot;
		return 1;
	}
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
put1(Map *map, ADDR addr, int space, uchar *v, int size)
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
		errflg = relocmsg[s];
		return 0;
	case 0:
		if (++s < 0)
			s = 0;
		errflg = readmsg[s];
		return 0;
	default:
		return 1;
	}
}

static
dbput(Map *map, int s, ulong addr, char *buf, int size)
{
	if (wtflag == 0 &&  !(map == cormap && pid))
		error("not in write mode");
	switch (mput(map, s, addr, buf, size)) {
	case -1:
		if (s < 0)
			s = 0;
		errflg = relocmsg[s];
		return 0;
	case 0:
		if (s < 0)
			s = 0;
		errflg = writemsg[s];
		return 0;
	default:
		return 1;
	}
}

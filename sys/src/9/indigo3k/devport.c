#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

/*
 *	This device can be used to make byte, short, and word
 *	references to addresses in I/O space.  It is intended
 *	for debugging GIO cards.
 */

#include	"devtab.h"

#define	G0ADDR	(KSEG1+GIO0_ADDR)
#define	G1ADDR	(KSEG1+GIO1_ADDR)
#define	H0ADDR	(KSEG1+HPC_0_ID_ADDR)
#define	GSIZE	0x200000

#define	ROMADDR	(KSEG1+PROM)

enum
{
	Qdir,
	Qgio0,
	Qgio1,
	Qhpc0,
	Qprobe,
	Qcount,
	Qrom
};

Dirtab portdir[]={
	"gio0",		{Qgio0},	0,	0666,
	"gio1",		{Qgio1},	0,	0666,
	"hpc0",		{Qhpc0},	0,	0666,
	"probe",	{Qprobe},	0,	0444,
	"count",	{Qcount}, NUMSIZE,	0444,
	"rom",		{Qrom},  PROMSIZE,	0444,
};

#define	NPORT	(sizeof portdir/sizeof(Dirtab))

void
portreset(void)
{}

void
portinit(void)
{}

Chan *
portattach(char *param)
{
	return devattach('x', param);
}

Chan *
portclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
portwalk(Chan *c, char *name)
{
	return devwalk(c, name, portdir, NPORT, devgen);
}

void
portstat(Chan *c, char *db)
{
	devstat(c, db, portdir, NPORT, devgen);
}

Chan *
portopen(Chan *c, int omode)
{
	return devopen(c, omode, portdir, NPORT, devgen);
}

void
portcreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
portclose(Chan *c)
{
	USED(c);
}

long
portread(Chan *c, void *a, long n, ulong offset)
{
	ulong addr = 0;

	if(n <= 0)
		return 0;
	switch(c->qid.path & ~CHDIR){
	case Qdir:
		return devdirread(c, a, n, portdir, NPORT, devgen);
	case Qgio0:
		addr = G0ADDR;
		break;
	case Qgio1:
		addr = G1ADDR;
		break;
	case Qhpc0:
		addr = H0ADDR;
		break;
	case Qrom:
		if(offset >= PROMSIZE)
			return 0;
		if(offset+n > PROMSIZE)
			n = PROMSIZE - offset;
		memmove(a, ((char*)ROMADDR)+offset, n);
		return n;
	case Qprobe:
		if(offset % 4)
			error(Ebadarg);
		if(busprobe(KSEG1+offset) < 0)
			error("bus error");
		return n;
		break;
	case Qcount:
		return readnum(offset, a, n, rdcount(), NUMSIZE);
	default:
		panic("portread");
	}
	if(offset+n >= GSIZE)
		error(Ebadarg);
	addr += offset;
	switch(n){
	case 1:
		*(uchar *)a = *(uchar *)addr;
		break;
	case 2:
		if(addr % 2)
			error(Ebadarg);
		*(ushort *)a = *(ushort *)addr;
		break;
	case 4:
		if(addr % 4)
			error(Ebadarg);
		*(ulong *)a = *(ulong *)addr;
		break;
	default:
		error(Ebadarg);
	}
	return n;
}

long
portwrite(Chan *c, void *a, long n, ulong offset)
{
	ulong addr = 0;

	if(n <= 0)
		return 0;
	switch(c->qid.path){
	case Qgio0:
		addr = G0ADDR;
		break;
	case Qgio1:
		addr = G1ADDR;
		break;
	case Qhpc0:
		addr = H0ADDR;
		break;
	default:
		panic("portwrite");
	}
	if(offset+n >= GSIZE)
		error(Ebadarg);
	addr += offset;
	switch(n){
	case 1:
		*(uchar *)addr = *(uchar *)a;
		break;
	case 2:
		if(addr % 2)
			error(Ebadarg);
		*(ushort *)addr = *(ushort *)a;
		break;
	case 4:
		if(addr % 4)
			error(Ebadarg);
		*(ulong *)addr = *(ulong *)a;
		break;
	default:
		error(Ebadarg);
	}
	return n;
}

void
portremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
portwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

#define	Nportservice	8
static	int (*portservice[Nportservice])(void);
static	Lock intrlock;

void
addportintr(int (*f)(void))
{
	int s = splhi();
	int (**p)(void);

	lock(&intrlock);
	for(p=portservice; *p; p++)
		if (*p == f)
			goto out;
	if(p >= &portservice[Nportservice-1])
		panic("addportintr");
	*IO(uchar, LIO_0_MASK) |= LIO_GIO_1;
	*p = f;
out:
	unlock(&intrlock);
	splx(s);
}

void
devportintr(void)
{
	int (**p)(void);
	int i = 0;

	for(p=portservice; *p; p++)
		i |= (**p)();
	USED(i);
}

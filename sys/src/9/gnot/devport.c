#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"devtab.h"

#define	ROMADDR	0x40000000
#define	ROMSIZE	((256*1024)/8)

#define	P_qlock(sel)	if(sel >= 0){\
				qlock(&portpage);\
				PORTSELECT = portpage.select = sel;\
			}else

#define	P_qunlock(sel)	if(sel >= 0){\
				qunlock(&portpage);\
			}else

#define	P_oper(sel, inst)		P_qlock(sel); inst; P_qunlock(sel)
#define	P_read(sel, addr, val, type)	P_oper(sel, val = *(type *)(PORT+addr))
#define	P_write(sel, addr, val, type)	P_oper(sel, *(type *)(PORT+addr) = val)

enum
{
	Qdir,
	Qdata,
	Qrom
};

Dirtab portdir[]={
	"data",		{Qdata},	0,	0666,
	"rom",		{Qrom},	ROMSIZE,	0444,
};

#define	NPORT	(sizeof portdir/sizeof(Dirtab))

#define	Nportservice	8
static	int (*portservice[Nportservice])(void);
	Portpage portpage;
static	Lock intrlock;

int
portprobe(char *what, int select, int addr, int rw, long val)
{
	int time;
	if (!conf.portispaged)
		return 0;
	P_qlock(select);
	switch (rw) {
	case -1:
		val = *(uchar *)(PORT+addr);
		break;
	case -2:
		val = *(ushort *)(PORT+addr);
		break;
	case -4:
		val = *(long *)(PORT+addr);
		break;
	case 1:
		*(uchar *)(PORT+addr) = val;
		break;
	case 2:
		*(ushort *)(PORT+addr) = val;
		break;
	case 4:
		*(long *)(PORT+addr) = val;
		break;
	default:
		panic("portprobe");
	}
	time = PORTSELECT & 0x1f;
	P_qunlock(select);
	if (what) {
		print("%s at %d, %d", what, select, addr);
		print("%s", time & 0x10 ? " -- NOT FOUND\n" : "\n");
	}
	if (time & 0x10)
		return -1;
	return val;
}

void
portreset(void)
{
	portpage.select = -1;
	lock(&portpage);
	unlock(&portpage);
}

void
portinit(void)
{
}

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
portread(Chan *c, char *a, long n, ulong offset)
{
	long s, k;
	if (n == 0)
		return 0;
	switch (c->qid.path & ~CHDIR) {
	case Qdir:
		return devdirread(c, a, n, portdir, NPORT, devgen);
	case Qdata:
		if (!conf.portispaged || (s = offset >> PORTSHIFT) > 0xff)
			s = -1;
		k = offset % PORTSIZE;
		P_qlock(s);
		switch ((int)n) {
		case 1:
			*a = PORT[k]; break;
		case 2:
			*(short *)a = *(short *)(PORT+k); break;
		case 4:
			*(long *)a = *(long *)(PORT+k); break;
		default:
			P_qunlock(s);
			error(Ebadarg);
		}
		P_qunlock(s);
		break;
	case Qrom:
		if(offset >= ROMSIZE)
			return 0;
		if(offset+n > ROMSIZE)
			n = ROMSIZE - offset;
		memmove(a, ((char*)ROMADDR)+offset, n);
		return n;
		break;
	default:
		panic("portread");
	}
	return n;
}

long
portwrite(Chan *c, char *a, long n, ulong offset)
{
	long s, k;
	if (n == 0)
		return 0;
	switch ((int)c->qid.path) {
	case Qdata:
		if (!conf.portispaged || (s = offset >> PORTSHIFT) > 0xff)
			s = -1;
		k = offset % PORTSIZE;
		P_qlock(s);
		switch ((int)n) {
		case 1:
			PORT[k] = *a; break;
		case 2:
			*(short *)(PORT+k) = *(short *)a;
			break;
		case 4:
			*(long *)(PORT+k) = *(long *)a;
			break;
		default:
			P_qunlock(s);
			error(Ebadarg);
		}
		P_qunlock(s);
		break;
	default:
		panic("portwrite");
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

void
addportintr(int (*f)(void))
{
	int s = splhi();
	int (**p)(void);

	lock(&intrlock);
	for (p=portservice; *p; p++)
		if (*p == f)
			goto out;
	if (p >= &portservice[Nportservice-1])
		panic("addportintr");
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

	for (p=portservice; *p; p++)
		i |= (**p)();

	if (!i)
		/*putstring("spurious portintr\n");*/
		panic("portintr");
	if (portpage.select >= 0)
		PORTSELECT = portpage.select;
}

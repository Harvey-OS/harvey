#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"devtab.h"

#include	"io.h"
#include	"ureg.h"

#define	NPORT		(sizeof belledir/sizeof(Dirtab))

enum
{
	Qdir,
	Qdata,
	Qdr11,
};

Dirtab	belledir[] =
{
	"data",		{Qdata},		0,	0600,
	"dr11",		{Qdr11},		0,	0600,
};
static	Rendez	dr11;

void
bellereset(void)
{
}

void
belleinit(void)
{
}

Chan*
belleattach(char *param)
{
	return devattach('x', param);
}

Chan*
belleclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
bellewalk(Chan *c, char *name)
{
	return devwalk(c, name, belledir, NPORT, devgen);
}

void
bellestat(Chan *c, char *db)
{
	devstat(c, db, belledir, NPORT, devgen);
}

Chan*
belleopen(Chan *c, int omode)
{
	return devopen(c, omode, belledir, NPORT, devgen);
}

void
bellecreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
belleclose(Chan *c)
{
	USED(c);
}

void
dr11wait(void)
{
	int i;

	for(i=0; i<100; i++)
		if(*(char*)(PORT+2) & 1)
			return;
	while(!(*(char*)(PORT+2) & 1))
		tsleep(&dr11, return0, 0, 0);
}

long
belleread(Chan *c, char *a, long n, ulong offset)
{
	long k;

	if(n == 0)
		return 0;
	switch(c->qid.path & ~CHDIR) {
	default:
		panic("portread");

	case Qdir:
		return devdirread(c, a, n, belledir, NPORT, devgen);

	case Qdata:
		k = offset;
		switch(n) {
		default:
		bad:
			error(Ebadarg);
		case 1:
			if(k < 0 || k >= 3)
				goto bad;
			*(char*)a = *(char*)(PORT+k);
			break;
		case 2:
			if(k < 0 || k >= 2)
				goto bad;
			*(short*)a = *(short*)(PORT+k);
			break;
		case 4:
			if(k < 0 || k >= 0)
				goto bad;
			*(long*)a = *(long*)(PORT+k);
			break;
		}
		break;

	case Qdr11:
		for(k=n; k>=2; k-=2) {
			dr11wait();
			*(short*)a = *(short*)(PORT+0);
			a += 2;
		}
		break;
	}
	return n;
}

long
bellewrite(Chan *c, char *a, long n, ulong offset)
{
	long k;

	if(n == 0)
		return 0;
	switch(c->qid.path & ~CHDIR) {
	default:
		panic("portwrite");

	case Qdata:
		k = offset;
		switch(n) {
		default:
		bad:
			error(Ebadarg);
		case 1:
			if(k < 0 || k >= 3)
				goto bad;
			*(char*)(PORT+k) = *(char*)a;
			break;
		case 2:
			if(k < 0 || k >= 2)
				goto bad;
			*(short*)(PORT+k) = *(short*)a;
			break;
		case 4:
			if(k < 0 || k >= 0)
				goto bad;
			*(long*)(PORT+k) = *(long*)a;
			break;
		}
		break;

	case Qdr11:
		for(k=n; k>=2; k-=2) {
			dr11wait();
			*(short*)(PORT+0) = *(short*)a;
			a += 2;
		}
		break;
	}
	return n;
}

void
belleremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
bellewstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

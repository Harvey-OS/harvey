/*
 *  reboot stream module and device
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"devtab.h"

enum{
	Dirqid,
	Deltatqid,
	Zotqid,
};

Dirtab Reboottab[]={
	"deltat",	{Deltatqid, 0},	NUMSIZE,	0660,
	"zot",		{Zotqid, 0},	0,		0220,
};

#define NReboottab (sizeof(Reboottab)/sizeof(Dirtab))

/*
 *  stream module definition
 */
static void rebootstopen(Queue*, Stream*);
static void rebootiput(Queue*, Block*);
static void rebootoput(Queue*, Block*);
static Qinfo rebootinfo =
{
	rebootiput,
	rebootoput,
	rebootstopen,
	0,
	"reboot",
	0
};

static Rendez	timer;
static int	deltat;

void
rebootreset(void)
{
	newqinfo(&rebootinfo);
}

void
rebootinit(void)
{
}

Chan *
rebootattach(char *spec)
{
	return devattach(L'↑', spec);
}

Chan *
rebootclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
rebootwalk(Chan *c, char *name)
{
	return devwalk(c, name, Reboottab, NReboottab, devgen);
}

void
rebootstat(Chan *c, char *db)
{
	devstat(c, db, Reboottab, NReboottab, devgen);
}

Chan *
rebootopen(Chan *c, int omode)
{
	return devopen(c, omode, Reboottab, NReboottab, devgen);
}

void
rebootcreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
rebootremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
rebootwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

void
rebootclose(Chan *c)
{
	USED(c);
}

long
rebootread(Chan *c, void *a, long n, ulong offset)
{
	switch(c->qid.path & ~CHDIR){
	case Dirqid:
		return devdirread(c, a, n, Reboottab, NReboottab, devgen);
	case Deltatqid:
		return readnum(offset, a, n, deltat, NUMSIZE);
	default:
		n=0;
		break;
	}
	return n;
}

long
rebootwrite(Chan *c, char *a, long n, ulong offset)
{
	char buf[NUMSIZE];
	Block *bp;

	USED(offset);
	switch(c->qid.path & ~CHDIR){
	case Deltatqid:
		if(n > NUMSIZE-1)
			n = NUMSIZE-1;
		memmove(buf, a, n);
		buf[n] = 0;
		deltat = strtoul(buf, 0, 0);
		break;
	case Zotqid:
		bp = allocb(0);
		bp->type = M_HANGUP;
		rebootiput(0, bp);
		break;
	default:
		error(Ebadusefd);
	}
	return n;
}

/*
 *  stream routines
 */

static void
rebootstopen(Queue *q, Stream *s)
{
	USED(q, s);
	if(strcmp(u->p->user, eve) != 0)
		error(Eperm);
}

void
rebootoput(Queue *q, Block *bp)
{
	PUTNEXT(q, bp);
}

static void
rebootiput(Queue *q, Block *bp)
{
	if(bp->type == M_HANGUP){
		tsleep(&timer, return0, 0, deltat);
		exit(0);
	}
	PUTNEXT(q, bp);
}

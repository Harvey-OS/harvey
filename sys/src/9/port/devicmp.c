#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include 	"arp.h"
#include 	"../port/ipdat.h"

#include	"devtab.h"

static Streamput	icmpoput;
static Streamopen	icmpstopen;
static Streamclose	icmpstclose;
static void		icmprcvmsg(Ipifc*, Block*);

Qinfo icmpinfo  = { 0,    icmpoput,    icmpstopen,    icmpstclose,    "icmp"  };

static struct {
	QLock;
	Queue	*rq;
	Ipifc	*ifc;
} icmp;

Ipifc	*ipifc[];

/*
 *  device interface
 */
Dirtab icmptab[]={
	"icmp",		{Sdataqid},		0,	0666,
};
#define Nicmptab (sizeof(icmptab)/sizeof(Dirtab))

void
icmpreset(void)
{
}

void
icmpinit(void)
{
	Ipifc **p;

	/* add into protocol list */
	for(p = ipifc; *p; p++)
		;
	icmp.ifc = *p = xalloc(sizeof(Ipifc));
	icmp.ifc->name = "icmp";
}

Chan *
icmpattach(char *spec)
{
	return devattach('Q', spec);
}

Chan *
icmpclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
icmpwalk(Chan *c, char *name)
{
	return devwalk(c, name, icmptab, (long)Nicmptab, devgen);
}

void
icmpstat(Chan *c, char *db)
{
	devstat(c, db, icmptab, (long)Nicmptab, devgen);
}

Chan *
icmpopen(Chan *c, int omode)
{
	if(c->qid.path == CHDIR){
		if(omode != OREAD)
			error(Eperm);
	} else
		streamopen(c, &icmpinfo);
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

void
icmpcreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
icmpremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
icmpwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

void
icmpclose(Chan *c)
{
	if((c->qid.path&CHDIR) == 0)
		streamclose(c);
}

long
icmpread(Chan *c, void *a, long n, ulong offset)
{
	USED(offset);

	if(c->qid.path&CHDIR)
		return devdirread(c, a, n, icmptab, Nicmptab, devgen);
	return streamread(c, a, n);
}

long
icmpwrite(Chan *c, char *a, long n, ulong offset)
{
	USED(offset);

	return streamwrite(c, a, n, 1);
}

static void
icmpstclose(Queue *q)
{
	USED(q);
	qlock(&icmp);
	icmp.rq = 0;
	qunlock(&icmp);
}

static void
icmpstopen(Queue *q, Stream *s)
{
	USED(s);
	initipifc(icmp.ifc, IP_ICMPPROTO, icmprcvmsg);
	icmp.rq = q;
}

static void
icmprcvmsg(Ipifc *ifc, Block *bp)
{
	USED(ifc);

	qlock(&icmp);
	if(icmp.rq == 0 || QFULL(icmp.rq->next))
		freeb(bp);
	else
		PUTNEXT(icmp.rq, bp);
	qunlock(&icmp);
}

static void
icmpoput(Queue *q, Block *bp)
{
	USED(q);
	ipmuxoput(0, bp);
}

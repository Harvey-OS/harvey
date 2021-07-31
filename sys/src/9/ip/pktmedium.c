#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#include "ip.h"


static void	pktbind(Ipifc *ifc, int argc, char **argv);
static void	pktunbind(Ipifc *ifc);
static void	pktbwrite(Ipifc *ifc, Block *bp, int version, uchar *ip);
static void	pktin(Fs*, Ipifc *ifc, Block *bp);

Medium pktmedium =
{
.name=		"pkt",
.hsize=		14,
.minmtu=	40,
.maxmtu=	4*1024,
.maclen=	6,
.bind=		pktbind,
.unbind=	pktunbind,
.bwrite=	pktbwrite,
.pktin=		pktin,
.unbindonclose=	1,
};

/*
 *  called to bind an IP ifc to an ethernet device
 *  called with ifc wlock'd
 */
static void
pktbind(Ipifc*, int, char**)
{
}

/*
 *  called with ifc wlock'd
 */
static void
pktunbind(Ipifc*)
{
}

/*
 *  called by ipoput with a single packet to write
 */
static void
pktbwrite(Ipifc *ifc, Block *bp, int, uchar*)
{
	/* enqueue onto the conversation's rq */
	bp = concatblock(bp);
	qpass(ifc->conv->rq, bp);
}

/*
 *  called with ifc rlocked when someone write's to 'data'
 */
static void
pktin(Fs *f, Ipifc *ifc, Block *bp)
{
	if(ifc->lifc == nil)
		freeb(bp);
	else
		ipiput(f, ifc, bp);
}

void
pktmediumlink(void)
{
	addipmedium(&pktmedium);
}

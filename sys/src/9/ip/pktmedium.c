/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#include "ip.h"


static void	pktbind(Ipifc*, int, char**);
static void	pktunbind(Ipifc*);
static void	pktbwrite(Ipifc*, Block*, int, uchar*);
static void	pktin(Fs*, Ipifc*, Block*);

Medium pktmedium =
{
.name=		"pkt",
.hsize=		14,
.mintu=		40,
.maxtu=		4*1024,
.maclen=	6,
.bind=		pktbind,
.unbind=	pktunbind,
.bwrite=	pktbwrite,
.pktin=		pktin,
};

/*
 *  called to bind an IP ifc to an ethernet device
 *  called with ifc wlock'd
 */
static void
pktbind(Ipifc*, int argc, char **argv)
{
	USED(argc, argv);
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
	if(ifc->conv->snoopers.ref > 0)
		qpass(ifc->conv->sq, copyblock(bp, BLEN(bp)));
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
	else {
		if(ifc->conv->snoopers.ref > 0)
			qpass(ifc->conv->sq, copyblock(bp, BLEN(bp)));
		ipiput4(f, ifc, bp);
	}
}

void
pktmediumlink(void)
{
	addipmedium(&pktmedium);
}

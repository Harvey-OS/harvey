#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#include "ip.h"

enum
{
	Maxtu=	16*1024,
};

typedef struct LB LB;
struct LB
{
	Proc	*readp;
	Queue	*q;
	Fs	*f;
};

static void loopbackread(void *a);

static void
loopbackbind(Ipifc *ifc, int, char**)
{
	LB *lb;

	lb = smalloc(sizeof(*lb));
	lb->f = ifc->conv->p->f;
	lb->q = qopen(1024*1024, Qmsg, nil, nil);
	ifc->arg = lb;
	ifc->mbps = 1000;

	kproc("loopbackread", loopbackread, ifc);

}

static void
loopbackunbind(Ipifc *ifc)
{
	LB *lb = ifc->arg;

	if(lb->readp)
		postnote(lb->readp, 1, "unbind", 0);

	/* wait for reader to die */
	while(lb->readp != 0)
		tsleep(&up->sleep, return0, 0, 300);

	/* clean up */
	qfree(lb->q);
	free(lb);
}

static void
loopbackbwrite(Ipifc *ifc, Block *bp, int, uchar*)
{
	LB *lb;

	lb = ifc->arg;
	if(qpass(lb->q, bp) < 0)
		ifc->outerr++;
	ifc->out++;
}

static void
loopbackread(void *a)
{
	Ipifc *ifc;
	Block *bp;
	LB *lb;

	ifc = a;
	lb = ifc->arg;
	lb->readp = up;	/* hide identity under a rock for unbind */
	if(waserror()){
		lb->readp = 0;
		pexit("hangup", 1);
	}
	for(;;){
		bp = qbread(lb->q, Maxtu);
		if(bp == nil)
			continue;
		ifc->in++;
		if(!canrlock(ifc)){
			freeb(bp);
			continue;
		}
		if(waserror()){
			runlock(ifc);
			nexterror();
		}
		if(ifc->lifc == nil)
			freeb(bp);
		else
			ipiput4(lb->f, ifc, bp);
		runlock(ifc);
		poperror();
	}
}

Medium loopbackmedium =
{
.hsize=		0,
.mintu=		0,
.maxtu=		Maxtu,
.maclen=	0,
.name=		"loopback",
.bind=		loopbackbind,
.unbind=	loopbackunbind,
.bwrite=	loopbackbwrite,
};

void
loopbackmediumlink(void)
{
	addipmedium(&loopbackmedium);
}

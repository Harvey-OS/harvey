#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#include "ip.h"
#include "trip.h"

static void	tripread(void *a);
static void	tripbind(Ipifc *ifc, int argc, char **argv);
static void	tripunbind(Ipifc *ifc);
static void	tripbwrite(Ipifc *ifc, Block *bp, int version, uchar *ip);
static void	tripaddmulti(Ipifc *ifc, uchar*, uchar*);
static void	tripremmulti(Ipifc *ifc, uchar*, uchar*);
static void	tripaddroute(Ipifc *ifc, int, uchar*, uchar*, uchar*, int);
static void	tripremroute(Ipifc *ifc, int, uchar*, uchar*);
static void	tripares(Fs*, int, uchar*, uchar*, int, int);

Medium tripmedium =
{
.name=		"trip",
.minmtu=	20,
.maxmtu=	64*1024,
.maclen=	LCIMACSIZE,
.bind=		tripbind,
.unbind=	tripunbind,
.bwrite=	tripbwrite,
.addmulti=	tripaddmulti,
.remmulti=	tripremmulti,
.addroute=	tripaddroute,
.remroute=	tripremroute,
.ares=		tripares,
};

typedef struct	Tripinfo Tripinfo;
struct Tripinfo
{
	Fs*	fs;		/* my instance of the IP stack */
	Ipifc*	ifc;		/* IP interface */
	Card*	dev;
	Proc*	readp;		/* reading process */
	Chan*	mchan;		/* Data channel */
};

/*
 *  called to bind an IP ifc to an ethernet device
 *  called with ifc qlock'd
 */
static void
tripbind(Ipifc *ifc, int argc, char **argv)
{
	int fd;
	Chan *mchan;
	Tripinfo *er;

	if(argc < 2)
		error(Ebadarg);

	fd = kopen(argv[2], ORDWR);
	if(fd < 0)
		error("trip open failed");

	mchan = fdtochan(up->env->fgrp, fd, ORDWR, 0, 1);
	kclose(fd);

	if(devtab[mchan->type]->dc != 'T') {
		cclose(mchan);
		error(Enoport);
	}

	er = smalloc(sizeof(*er));
	er->mchan = mchan;
	er->ifc = ifc;
	er->dev = tripsetifc(mchan, ifc);
	er->fs = ifc->conv->p->f;

	ifc->arg = er;

	kproc("tripread", tripread, ifc);
}

/*
 *  called with ifc qlock'd
 */
static void
tripunbind(Ipifc *ifc)
{
	Tripinfo *er = ifc->arg;
/*
	if(er->readp)
		postnote(er->readp, 1, "unbind", 0);
*/
	tsleep(&up->sleep, return0, 0, 300);

	if(er->mchan != nil)
		cclose(er->mchan);

	free(er);
}

/*
 *  called by ipoput with a single block to write
 */
static void
tripbwrite(Ipifc *ifc, Block *bp, int version, uchar *ip)
{
	Tripinfo *er = ifc->arg;

	/*
	 * Packet is rerouted at linecard
	 * so the gateway is ignored
	 */
	USED(ip);
	USED(version);

	if(waserror()) {
		print("tripwrite failed\n");
		return;
	}

	devtab[er->mchan->type]->bwrite(er->mchan, bp, 0);
	poperror();
	ifc->out++;
}

/*
 *  process to read from the trip interface
 */
static void
tripread(void *a)
{
	Ipifc *ifc;
	Block *bp;
	Tripinfo *er;

	ifc = a;
	er = ifc->arg;
	er->readp = up;	/* hide identity under a rock for unbind */

	for(;;) {
		bp = devtab[er->mchan->type]->bread(er->mchan, ifc->maxmtu, 0);
		ifc->in++;
		ipiput(er->fs, ifc, bp);
	}

	pexit("hangup", 1);
}

static void
tripaddroute(Ipifc *ifc, int v, uchar *addr, uchar *mask, uchar *gate, int t)
{
	int alen;
	MTroute mtr;
	Tripinfo *tinfo;

	tinfo = ifc->arg;
	if(!tinfo->dev->routing)
		return;

	/*
	 * Multicast addresses are handled on the linecard by
	 * the multicast port driver, so the route load is dumped.
	 *	loaded by addmulti/remmulti for SBC routes
	 *		  joinmulti/leavemulti for inter LC
	 */
	if(ipismulticast(addr))
		return;

	mtr.type = T_ROUTEADMIN;
	if(v & Rv4) {
		mtr.op = RTADD4;
		alen = IPv4addrlen;
	}
	else {
		mtr.op = RTADD6;
		alen = IPaddrlen;
	}
	mtr.rtype = t;
	memmove(mtr.addr, addr, alen);
	memmove(mtr.mask, mask, alen);
	memmove(mtr.gate, gate, alen);

	i2osend(tinfo->dev, &mtr, sizeof(mtr));
}

static void
tripremroute(Ipifc *ifc, int v, uchar *addr, uchar *mask)
{
	int alen;
	MTroute mtr;
	Tripinfo *tinfo;

	tinfo = ifc->arg;
	if(!tinfo->dev->routing)
		return;

	if(ipismulticast(addr))
		return;

	mtr.type = T_ROUTEADMIN;
	if(v & Rv4) {
		mtr.op = RTDEL4;
		alen = IPv4addrlen;
	}
	else {
		mtr.op = RTDEL6;
		alen = IPaddrlen;
	}
	memmove(mtr.addr, addr, alen);
	memmove(mtr.mask, mask, alen);

	i2osend(tinfo->dev, &mtr, sizeof(mtr));
}

static void
tripxmitroute(Route *r, Routewalk *rw)
{
	int nifc;
	char t[5];
	uchar a[IPaddrlen], m[IPaddrlen], g[IPaddrlen];

	convroute(r, a, m, g, t, &nifc);
	if(!(r->type & Rv4)) {
		tripaddroute(rw->state, 0, a, m, g, r->type);
		return;
	}

	tripaddroute(rw->state, Rv4, a+IPv4off, m+IPv4off, g+IPv4off, r->type);
}

static void
sendifcinfo(Ipifc *dest)
{
	Conv **cp, **e;
	Iplifc *l;
	Ipifc *ifc;
	MTifctl mtc;
	Tripinfo *tinfo, *oinfo;
	Proto *p;

	tinfo = dest->arg;

	/* Install interfaces */
	p = tinfo->fs->ipifc;
	e = &p->conv[p->nc];
	for(cp = p->conv; cp < e; cp++) {

		if(*cp == nil)
			continue;

		ifc = (Ipifc*)(*cp)->ptcl;
		if(dest == ifc)
			continue;

		mtc.type = T_CTLIFADMIN;
		mtc.maxtu = ifc->maxmtu;
		mtc.mintu = ifc->minmtu;

		mtc.port = 0;
		if(ifc->m == &tripmedium) {
			oinfo = ifc->arg;
			mtc.port = oinfo->dev->bar[0].bar;
		}

		for(l = ifc->lifc; l != nil; l = l->next) {
			if(isv4(l->local)) {
				mtc.op = IFADD4;
				memmove(mtc.addr, l->local+IPv4off, IPv4addrlen);
				memmove(mtc.mask, l->mask+IPv4off, IPv4addrlen);
			}
			else {
				mtc.op = IFADD6;
				memmove(mtc.addr, l->local, sizeof(mtc.addr));
				memmove(mtc.mask, l->mask, sizeof(mtc.mask));
			}

			i2osend(tinfo->dev, &mtc, sizeof(mtc));
		}
	}
}

void
tripsync(Ipifc *ifc)
{
	Routewalk rw;

	if(ifc == nil) {
		print("tripsync: interface not bound\n");
		return;
	}

	/* Mirror the route table into the lincard */
	rw.o = 0;
	rw.n = (1<<22);
	rw.state = ifc;
	rw.walk = tripxmitroute;

	ipwalkroutes(ifc->conv->p->f, &rw);

	/*
	 * Tell the linecard about interfaces that already
	 * exist elsewhere
	 */
	sendifcinfo(ifc);
}

/* Tell a line card the SBC is interested in listening
 * to a multicast address
 */
static void
tripaddmulti(Ipifc *ifc, uchar *addr, uchar *ifca)
{
	MTmultiears mt;
	Tripinfo *tinfo;

	/* print("tripaddmulti %I %I\n", addr, ifca); /**/

	tinfo = ifc->arg;
	if(!tinfo->dev->routing)
		return;

	mt.type = T_MULTIEAR;
	mt.op = ADDMULTI;
	memmove(mt.addr, addr, sizeof(mt.addr));
	memmove(mt.ifca, ifca, sizeof(mt.ifca));

	i2osend(tinfo->dev, &mt, sizeof(mt));
}

/* Tell a line card the SBC is no longer interested in listening
 * to a multicast address
 */
static void
tripremmulti(Ipifc *ifc, uchar *addr, uchar *ifca)
{
	MTmultiears mt;
	Tripinfo *tinfo;

	tinfo = ifc->arg;
	if(!tinfo->dev->routing)
		return;

	mt.type = T_MULTIEAR;
	mt.op = REMMULTI;
	memmove(mt.addr, addr, sizeof(mt.addr));
	memmove(mt.ifca, ifca, sizeof(mt.ifca));

	i2osend(tinfo->dev, &mt, sizeof(mt));
}

static void
tripares(Fs *fs, int vers, uchar *ip, uchar *mac, int l, int)
{
	Route *r;
	Ipifc *ifc;
	MTaresenter ta;
	Tripinfo *tinfo;
	uchar v6ip[IPaddrlen];

	if(vers == V4) {
		r = v4lookup(fs, ip);
		v4tov6(v6ip, ip);
		ip = v6ip;
	}
	else
		r = v6lookup(fs, ip);

	if(r == nil) {
		print("tripares: no route for entry\n");
		return;
	}

	ifc = r->ifc;

	tinfo = ifc->arg;
	if(!tinfo->dev->routing)
		return;

	if(vers == V4) {
		v4tov6(v6ip, ip);
		ip = v6ip;
	}

	ta.type = T_ARESENTER;
	ta.maclen = l;
	memmove(ta.addr, ip, IPaddrlen);
	memmove(ta.amac, mac, l);

	i2osend(tinfo->dev, &ta, sizeof(ta));
}

void
tripmediumlink(void)
{
	addipmedium(&tripmedium);
}

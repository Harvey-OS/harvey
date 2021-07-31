#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#include "../port/netif.h"
#include "ip.h"
#include "ipv6.h"

typedef struct Etherhdr Etherhdr;
struct Etherhdr
{
	uchar	d[6];
	uchar	s[6];
	uchar	t[2];
};

static uchar ipbroadcast[IPaddrlen] = {
	0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,
};

static uchar etherbroadcast[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

static void	etherread4(void *a);
static void	etherread6(void *a);
static void	etherbind(Ipifc *ifc, int argc, char **argv);
static void	etherunbind(Ipifc *ifc);
static void	etherbwrite(Ipifc *ifc, Block *bp, int version, uchar *ip);
static void	etheraddmulti(Ipifc *ifc, uchar *a, uchar *ia);
static void	etherremmulti(Ipifc *ifc, uchar *a, uchar *ia);
static Block*	multicastarp(Fs *f, Arpent *a, Medium*, uchar *mac);
static void	sendarp(Ipifc *ifc, Arpent *a);
static void	sendgarp(Ipifc *ifc, uchar*);
static int	multicastea(uchar *ea, uchar *ip);
static void	recvarpproc(void*);
static void	resolveaddr6(Ipifc *ifc, Arpent *a);
static void	etherpref2addr(uchar *pref, uchar *ea);

Medium ethermedium =
{
.name=		"ether",
.hsize=		14,
.mintu=		60,
.maxtu=		1514,
.maclen=	6,
.bind=		etherbind,
.unbind=	etherunbind,
.bwrite=	etherbwrite,
.addmulti=	etheraddmulti,
.remmulti=	etherremmulti,
.ares=		arpenter,
.areg=		sendgarp,
.pref2addr=	etherpref2addr,
};

Medium gbemedium =
{
.name=		"gbe",
.hsize=		14,
.mintu=		60,
.maxtu=		9014,
.maclen=	6,
.bind=		etherbind,
.unbind=	etherunbind,
.bwrite=	etherbwrite,
.addmulti=	etheraddmulti,
.remmulti=	etherremmulti,
.ares=		arpenter,
.areg=		sendgarp,
.pref2addr=	etherpref2addr,
};

typedef struct	Etherrock Etherrock;
struct Etherrock
{
	Fs	*f;		/* file system we belong to */
	Proc	*arpp;		/* arp process */
	Proc	*read4p;	/* reading process (v4)*/
	Proc	*read6p;	/* reading process (v6)*/
	Chan	*mchan4;	/* Data channel for v4 */
	Chan	*achan;		/* Arp channel */
	Chan	*cchan4;	/* Control channel for v4 */
	Chan	*mchan6;	/* Data channel for v6 */
	Chan	*cchan6;	/* Control channel for v6 */
};

/*
 *  ethernet arp request
 */
enum
{
	ARPREQUEST	= 1,
	ARPREPLY	= 2,
};

typedef struct Etherarp Etherarp;
struct Etherarp
{
	uchar	d[6];
	uchar	s[6];
	uchar	type[2];
	uchar	hrd[2];
	uchar	pro[2];
	uchar	hln;
	uchar	pln;
	uchar	op[2];
	uchar	sha[6];
	uchar	spa[4];
	uchar	tha[6];
	uchar	tpa[4];
};

static char *nbmsg = "nonblocking";

/*
 *  called to bind an IP ifc to an ethernet device
 *  called with ifc wlock'd
 */
static void
etherbind(Ipifc *ifc, int argc, char **argv)
{
	Chan *mchan4, *cchan4, *achan, *mchan6, *cchan6, *schan;
	char addr[Maxpath];	//char addr[2*KNAMELEN];
	char dir[Maxpath];	//char dir[2*KNAMELEN];
	char *buf;
	int n;
	char *ptr;
	Etherrock *er;

	if(argc < 2)
		error(Ebadarg);

	mchan4 = cchan4 = achan = mchan6 = cchan6 = nil;
	buf = nil;
	if(waserror()){
		if(mchan4 != nil)
			cclose(mchan4);
		if(cchan4 != nil)
			cclose(cchan4);
		if(achan != nil)
			cclose(achan);
		if(mchan6 != nil)
			cclose(mchan6);
		if(cchan6 != nil)
			cclose(cchan6);
		if(buf != nil)
			free(buf);
		nexterror();
	}

	/*
	 *  open ipv4 conversation
	 *
	 *  the dial will fail if the type is already open on
	 *  this device.
	 */
	snprint(addr, sizeof(addr), "%s!0x800", argv[2]);	/* ETIP4 */
	mchan4 = chandial(addr, nil, dir, &cchan4);

	/*
	 *  make it non-blocking
	 */
	devtab[cchan4->type]->write(cchan4, nbmsg, strlen(nbmsg), 0);

	/*
	 *  get mac address and speed
	 */
	snprint(addr, sizeof(addr), "%s/stats", argv[2]);
	buf = smalloc(512);
	schan = namec(addr, Aopen, OREAD, 0);
	if(waserror()){
		cclose(schan);
		nexterror();
	}
	n = devtab[schan->type]->read(schan, buf, 511, 0);
	cclose(schan);
	poperror();
	buf[n] = 0;

	ptr = strstr(buf, "addr: ");
	if(!ptr)
		error(Eio);
	ptr += 6;
	parsemac(ifc->mac, ptr, 6);

	ptr = strstr(buf, "mbps: ");
	if(ptr){
		ptr += 6;
		ifc->mbps = atoi(ptr);
	} else
		ifc->mbps = 100;

	/*
 	 *  open arp conversation
	 */
	snprint(addr, sizeof(addr), "%s!0x806", argv[2]);	/* ETARP */
	achan = chandial(addr, nil, nil, nil);

	/*
	 *  open ipv6 conversation
	 *
	 *  the dial will fail if the type is already open on
	 *  this device.
	 */
	snprint(addr, sizeof(addr), "%s!0x86DD", argv[2]);	/* ETIP6 */
	mchan6 = chandial(addr, nil, dir, &cchan6);

	/*
	 *  make it non-blocking
	 */
	devtab[cchan6->type]->write(cchan6, nbmsg, strlen(nbmsg), 0);

	er = smalloc(sizeof(*er));
	er->mchan4 = mchan4;
	er->cchan4 = cchan4;
	er->achan = achan;
	er->mchan6 = mchan6;
	er->cchan6 = cchan6;
	er->f = ifc->conv->p->f;
	ifc->arg = er;

	free(buf);
	poperror();

	kproc("etherread4", etherread4, ifc);
	kproc("recvarpproc", recvarpproc, ifc);
	kproc("etherread6", etherread6, ifc);
}

/*
 *  called with ifc wlock'd
 */
static void
etherunbind(Ipifc *ifc)
{
	Etherrock *er = ifc->arg;

	if(er->read4p)
		postnote(er->read4p, 1, "unbind", 0);
	if(er->read6p)
		postnote(er->read6p, 1, "unbind", 0);
	if(er->arpp)
		postnote(er->arpp, 1, "unbind", 0);

	/* wait for readers to die */
	while(er->arpp != 0 || er->read4p != 0 || er->read6p != 0)
		tsleep(&up->sleep, return0, 0, 300);

	if(er->mchan4 != nil)
		cclose(er->mchan4);
	if(er->achan != nil)
		cclose(er->achan);
	if(er->cchan4 != nil)
		cclose(er->cchan4);
	if(er->mchan6 != nil)
		cclose(er->mchan6);
	if(er->cchan6 != nil)
		cclose(er->cchan6);

	free(er);
}

/*
 *  called by ipoput with a single block to write with ifc rlock'd
 */
static void
etherbwrite(Ipifc *ifc, Block *bp, int version, uchar *ip)
{
	Etherhdr *eh;
	Arpent *a;
	uchar mac[6];
	Etherrock *er = ifc->arg;

	/* get mac address of destination */
	a = arpget(er->f->arp, bp, version, ifc, ip, mac);
	if(a){
		/* check for broadcast or multicast */
		bp = multicastarp(er->f, a, ifc->m, mac);
		if(bp==nil){
			switch(version){
			case V4:
				sendarp(ifc, a);
				break;
			case V6:
				resolveaddr6(ifc, a);
				break;
			default:
				panic("etherbwrite: version %d", version);
			}
			return;
		}
	}

	/* make it a single block with space for the ether header */
	bp = padblock(bp, ifc->m->hsize);
	if(bp->next)
		bp = concatblock(bp);
	if(BLEN(bp) < ifc->mintu)
		bp = adjustblock(bp, ifc->mintu);
	eh = (Etherhdr*)bp->rp;

	/* copy in mac addresses and ether type */
	memmove(eh->s, ifc->mac, sizeof(eh->s));
	memmove(eh->d, mac, sizeof(eh->d));

 	switch(version){
	case V4:
		eh->t[0] = 0x08;
		eh->t[1] = 0x00;
		devtab[er->mchan4->type]->bwrite(er->mchan4, bp, 0);
		break;
	case V6:
		eh->t[0] = 0x86;
		eh->t[1] = 0xDD;
		devtab[er->mchan6->type]->bwrite(er->mchan6, bp, 0);
		break;
	default:
		panic("etherbwrite2: version %d", version);
	}
	ifc->out++;
}


/*
 *  process to read from the ethernet
 */
static void
etherread4(void *a)
{
	Ipifc *ifc;
	Block *bp;
	Etherrock *er;

	ifc = a;
	er = ifc->arg;
	er->read4p = up;	/* hide identity under a rock for unbind */
	if(waserror()){
		er->read4p = 0;
		pexit("hangup", 1);
	}
	for(;;){
		bp = devtab[er->mchan4->type]->bread(er->mchan4, ifc->maxtu, 0);
		if(!canrlock(ifc)){
			freeb(bp);
			continue;
		}
		if(waserror()){
			runlock(ifc);
			nexterror();
		}
		ifc->in++;
		bp->rp += ifc->m->hsize;
		if(ifc->lifc == nil)
			freeb(bp);
		else
			ipiput4(er->f, ifc, bp);
		runlock(ifc);
		poperror();
	}
}


/*
 *  process to read from the ethernet, IPv6
 */
static void
etherread6(void *a)
{
	Ipifc *ifc;
	Block *bp;
	Etherrock *er;

	ifc = a;
	er = ifc->arg;
	er->read6p = up;	/* hide identity under a rock for unbind */
	if(waserror()){
		er->read6p = 0;
		pexit("hangup", 1);
	}
	for(;;){
		bp = devtab[er->mchan6->type]->bread(er->mchan6, ifc->maxtu, 0);
		if(!canrlock(ifc)){
			freeb(bp);
			continue;
		}
		if(waserror()){
			runlock(ifc);
			nexterror();
		}
		ifc->in++;
		bp->rp += ifc->m->hsize;
		if(ifc->lifc == nil)
			freeb(bp);
		else
			ipiput6(er->f, ifc, bp);
		runlock(ifc);
		poperror();
	}
}

static void
etheraddmulti(Ipifc *ifc, uchar *a, uchar *)
{
	uchar mac[6];
	char buf[64];
	Etherrock *er = ifc->arg;
	int version;

	version = multicastea(mac, a);
	sprint(buf, "addmulti %E", mac);
	switch(version){
	case V4:
		devtab[er->cchan4->type]->write(er->cchan4, buf, strlen(buf), 0);
		break;
	case V6:
		devtab[er->cchan6->type]->write(er->cchan6, buf, strlen(buf), 0);
		break;
	default:
		panic("etheraddmulti: version %d", version);
	}
}

static void
etherremmulti(Ipifc *ifc, uchar *a, uchar *)
{
	uchar mac[6];
	char buf[64];
	Etherrock *er = ifc->arg;
	int version;

	version = multicastea(mac, a);
	sprint(buf, "remmulti %E", mac);
	switch(version){
	case V4:
		devtab[er->cchan4->type]->write(er->cchan4, buf, strlen(buf), 0);
		break;
	case V6:
		devtab[er->cchan6->type]->write(er->cchan6, buf, strlen(buf), 0);
		break;
	default:
		panic("etherremmulti: version %d", version);
	}
}

/*
 *  send an ethernet arp
 *  (only v4, v6 uses the neighbor discovery, rfc1970)
 */
static void
sendarp(Ipifc *ifc, Arpent *a)
{
	int n;
	Block *bp;
	Etherarp *e;
	Etherrock *er = ifc->arg;

	/* don't do anything if it's been less than a second since the last */
	if(NOW - a->ctime < 1000){
		arprelease(er->f->arp, a);
		return;
	}

	/* remove all but the last message */
	while((bp = a->hold) != nil){
		if(bp == a->last)
			break;
		a->hold = bp->list;
		freeblist(bp);
	}

	/* try to keep it around for a second more */
	a->ctime = NOW;
	arprelease(er->f->arp, a);

	n = sizeof(Etherarp);
	if(n < a->type->mintu)
		n = a->type->mintu;
	bp = allocb(n);
	memset(bp->rp, 0, n);
	e = (Etherarp*)bp->rp;
	memmove(e->tpa, a->ip+IPv4off, sizeof(e->tpa));
	ipv4local(ifc, e->spa);
	memmove(e->sha, ifc->mac, sizeof(e->sha));
	memset(e->d, 0xff, sizeof(e->d));		/* ethernet broadcast */
	memmove(e->s, ifc->mac, sizeof(e->s));

	hnputs(e->type, ETARP);
	hnputs(e->hrd, 1);
	hnputs(e->pro, ETIP4);
	e->hln = sizeof(e->sha);
	e->pln = sizeof(e->spa);
	hnputs(e->op, ARPREQUEST);
	bp->wp += n;

	devtab[er->achan->type]->bwrite(er->achan, bp, 0);
}

static void
resolveaddr6(Ipifc *ifc, Arpent *a)
{
	int sflag;
	Block *bp;
	Etherrock *er = ifc->arg;
	uchar ipsrc[IPaddrlen];

	/* don't do anything if it's been less than a second since the last */
	if(NOW - a->ctime < ReTransTimer){
		arprelease(er->f->arp, a);
		return;
	}

	/* remove all but the last message */
	while((bp = a->hold) != nil){
		if(bp == a->last)
			break;
		a->hold = bp->list;
		freeblist(bp);
	}

	/* try to keep it around for a second more */
	a->ctime = NOW;
	a->rtime = NOW + ReTransTimer;
	if(a->rxtsrem <= 0) {
		arprelease(er->f->arp, a);
		return;
	}

	a->rxtsrem--;
	arprelease(er->f->arp, a);

	if(sflag = ipv6anylocal(ifc, ipsrc))
		icmpns(er->f, ipsrc, sflag, a->ip, TARG_MULTI, ifc->mac);
}

/*
 *  send a gratuitous arp to refresh arp caches
 */
static void
sendgarp(Ipifc *ifc, uchar *ip)
{
	int n;
	Block *bp;
	Etherarp *e;
	Etherrock *er = ifc->arg;

	/* don't arp for our initial non address */
	if(ipcmp(ip, IPnoaddr) == 0)
		return;

	n = sizeof(Etherarp);
	if(n < ifc->m->mintu)
		n = ifc->m->mintu;
	bp = allocb(n);
	memset(bp->rp, 0, n);
	e = (Etherarp*)bp->rp;
	memmove(e->tpa, ip+IPv4off, sizeof(e->tpa));
	memmove(e->spa, ip+IPv4off, sizeof(e->spa));
	memmove(e->sha, ifc->mac, sizeof(e->sha));
	memset(e->d, 0xff, sizeof(e->d));		/* ethernet broadcast */
	memmove(e->s, ifc->mac, sizeof(e->s));

	hnputs(e->type, ETARP);
	hnputs(e->hrd, 1);
	hnputs(e->pro, ETIP4);
	e->hln = sizeof(e->sha);
	e->pln = sizeof(e->spa);
	hnputs(e->op, ARPREQUEST);
	bp->wp += n;

	devtab[er->achan->type]->bwrite(er->achan, bp, 0);
}

static void
recvarp(Ipifc *ifc)
{
	int n;
	Block *ebp, *rbp;
	Etherarp *e, *r;
	uchar ip[IPaddrlen];
	static uchar eprinted[4];
	Etherrock *er = ifc->arg;

	ebp = devtab[er->achan->type]->bread(er->achan, ifc->maxtu, 0);
	if(ebp == nil)
		return;

	e = (Etherarp*)ebp->rp;
	switch(nhgets(e->op)) {
	default:
		break;

	case ARPREPLY:
		/* check for machine using my ip address */
		v4tov6(ip, e->spa);
		if(iplocalonifc(ifc, ip) || ipproxyifc(er->f, ifc, ip)){
			if(memcmp(e->sha, ifc->mac, sizeof(e->sha)) != 0){
				print("arprep: 0x%E/0x%E also has ip addr %V\n",
					e->s, e->sha, e->spa);
				break;
			}
		}

		/* make sure we're not entering broadcast addresses */
		if(ipcmp(ip, ipbroadcast) == 0 ||
			!memcmp(e->sha, etherbroadcast, sizeof(e->sha))){
			print("arprep: 0x%E/0x%E cannot register broadcast address %I\n",
				e->s, e->sha, e->spa);
			break;
		}

		arpenter(er->f, V4, e->spa, e->sha, sizeof(e->sha), 0);
		break;

	case ARPREQUEST:
		/* don't answer arps till we know who we are */
		if(ifc->lifc == 0)
			break;

		/* check for machine using my ip or ether address */
		v4tov6(ip, e->spa);
		if(iplocalonifc(ifc, ip) || ipproxyifc(er->f, ifc, ip)){
			if(memcmp(e->sha, ifc->mac, sizeof(e->sha)) != 0){
				if (memcmp(eprinted, e->spa, sizeof(e->spa))){
					/* print only once */
					print("arpreq: 0x%E also has ip addr %V\n", e->sha, e->spa);
					memmove(eprinted, e->spa, sizeof(e->spa));
				}
			}
		} else {
			if(memcmp(e->sha, ifc->mac, sizeof(e->sha)) == 0){
				print("arpreq: %V also has ether addr %E\n", e->spa, e->sha);
				break;
			}
		}

		/* refresh what we know about sender */
		arpenter(er->f, V4, e->spa, e->sha, sizeof(e->sha), 1);

		/* answer only requests for our address or systems we're proxying for */
		v4tov6(ip, e->tpa);
		if(!iplocalonifc(ifc, ip))
		if(!ipproxyifc(er->f, ifc, ip))
			break;

		n = sizeof(Etherarp);
		if(n < ifc->mintu)
			n = ifc->mintu;
		rbp = allocb(n);
		r = (Etherarp*)rbp->rp;
		memset(r, 0, sizeof(Etherarp));
		hnputs(r->type, ETARP);
		hnputs(r->hrd, 1);
		hnputs(r->pro, ETIP4);
		r->hln = sizeof(r->sha);
		r->pln = sizeof(r->spa);
		hnputs(r->op, ARPREPLY);
		memmove(r->tha, e->sha, sizeof(r->tha));
		memmove(r->tpa, e->spa, sizeof(r->tpa));
		memmove(r->sha, ifc->mac, sizeof(r->sha));
		memmove(r->spa, e->tpa, sizeof(r->spa));
		memmove(r->d, e->sha, sizeof(r->d));
		memmove(r->s, ifc->mac, sizeof(r->s));
		rbp->wp += n;

		devtab[er->achan->type]->bwrite(er->achan, rbp, 0);
	}
	freeb(ebp);
}

static void
recvarpproc(void *v)
{
	Ipifc *ifc = v;
	Etherrock *er = ifc->arg;

	er->arpp = up;
	if(waserror()){
		er->arpp = 0;
		pexit("hangup", 1);
	}
	for(;;)
		recvarp(ifc);
}

static int
multicastea(uchar *ea, uchar *ip)
{
	int x;

	switch(x = ipismulticast(ip)){
	case V4:
		ea[0] = 0x01;
		ea[1] = 0x00;
		ea[2] = 0x5e;
		ea[3] = ip[13] & 0x7f;
		ea[4] = ip[14];
		ea[5] = ip[15];
		break;
 	case V6:
 		ea[0] = 0x33;
 		ea[1] = 0x33;
 		ea[2] = ip[12];
		ea[3] = ip[13];
 		ea[4] = ip[14];
 		ea[5] = ip[15];
 		break;
	}
	return x;
}

/*
 *  fill in an arp entry for broadcast or multicast
 *  addresses.  Return the first queued packet for the
 *  IP address.
 */
static Block*
multicastarp(Fs *f, Arpent *a, Medium *medium, uchar *mac)
{
	/* is it broadcast? */
	switch(ipforme(f, a->ip)){
	case Runi:
		return nil;
	case Rbcast:
		memset(mac, 0xff, 6);
		return arpresolve(f->arp, a, medium, mac);
	default:
		break;
	}

	/* if multicast, fill in mac */
	switch(multicastea(mac, a->ip)){
	case V4:
	case V6:
		return arpresolve(f->arp, a, medium, mac);
	}

	/* let arp take care of it */
	return nil;
}

void
ethermediumlink(void)
{
	addipmedium(&ethermedium);
	addipmedium(&gbemedium);
}


static void
etherpref2addr(uchar *pref, uchar *ea)
{
	pref[8] = ea[0] | 0x2;
	pref[9] = ea[1];
	pref[10] = ea[2];
	pref[11] = 0xFF;
	pref[12] = 0xFE;
	pref[13] = ea[3];
	pref[14] = ea[4];
	pref[15] = ea[5];
}

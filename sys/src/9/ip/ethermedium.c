#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#include "ip.h"

typedef struct Etherhdr Etherhdr;
struct Etherhdr
{
	uchar	d[6];
	uchar	s[6];
	uchar	t[2];
};

static void	etherread(void *a);
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

Medium ethermedium =
{
.name=		"ether",
.hsize=		14,
.minmtu=	60,
.maxmtu=	1514,
.maclen=	6,
.bind=		etherbind,
.unbind=	etherunbind,
.bwrite=	etherbwrite,
.addmulti=	etheraddmulti,
.remmulti=	etherremmulti,
.ares=		arpenter,
.areg=		sendgarp,
};

Medium gbemedium =
{
.name=		"gbe",
.hsize=		14,
.minmtu=	60,
.maxmtu=	9014,
.maclen=	6,
.bind=		etherbind,
.unbind=	etherunbind,
.bwrite=	etherbwrite,
.addmulti=	etheraddmulti,
.remmulti=	etherremmulti,
.ares=		arpenter,
.areg=		sendgarp,
};

typedef struct	Etherrock Etherrock;
struct Etherrock
{
	Fs	*f;		/* file system we belong to */
	Proc	*arpp;		/* arp process */
	Proc	*readp;		/* reading process */
	Chan	*mchan;		/* Data channel */
	Chan	*achan;		/* Arp channel */
	Chan	*cchan;		/* Control channel */
};

/*
 *  ethernet arp request
 */
enum
{
	ETARP		= 0x0806,
	ETIP		= 0x0800,
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


/*
 *  called to bind an IP ifc to an ethernet device
 *  called with ifc wlock'd
 */
static void
etherbind(Ipifc *ifc, int argc, char **argv)
{
	Chan *mchan, *cchan, *achan;
	char addr[Maxpath];
	char dir[Maxpath];
	char *buf;
	int n;
	char *ptr;
	Etherrock *er;

	if(argc < 2)
		error(Ebadarg);

	mchan = cchan = achan = nil;
	buf = nil;
	if(waserror()){
		if(mchan != nil)
			cclose(mchan);
		if(cchan != nil)
			cclose(cchan);
		if(achan != nil)
			cclose(achan);
		if(buf != nil)
			free(buf);
		nexterror(); 
	}

	/*
	 *  open ip conversation
	 *
	 *  the dial will fail if the type is already open on
	 *  this device.
	 */
	snprint(addr, sizeof(addr), "%s!0x800", argv[2]);
	mchan = chandial(addr, nil, dir, &cchan);

	/*
	 *  get mac address
	 */
	snprint(addr, sizeof(addr), "%s/stats", dir);
	buf = smalloc(512);
	achan = namec(addr, Aopen, OREAD, 0);
	n = devtab[achan->type]->read(achan, buf, 511, 0);
	cclose(achan);
	buf[n] = 0;

	ptr = strstr(buf, "addr: ");
	if(!ptr)
		error(Eio);
	ptr += 6;

	parsemac(ifc->mac, ptr, 6);

	/*
 	 *  open arp conversation
	 */
	snprint(addr, sizeof(addr), "%s!0x806", argv[2]);
	achan = chandial(addr, nil, nil, nil);

	er = smalloc(sizeof(*er));
	er->mchan = mchan;
	er->cchan = cchan;
	er->achan = achan;
	er->f = ifc->conv->p->f;
	ifc->arg = er;

	free(buf);
	poperror();

	kproc("etherread", etherread, ifc);
	kproc("recvarpproc", recvarpproc, ifc);
}

/*
 *  called with ifc wlock'd
 */
static void
etherunbind(Ipifc *ifc)
{
	Etherrock *er = ifc->arg;

	if(er->readp)
		postnote(er->readp, 1, "unbind", 0);
	if(er->arpp)
		postnote(er->arpp, 1, "unbind", 0);

	/* wait for readers to die */
	while(er->arpp != 0 || er->readp != 0)
		tsleep(&up->sleep, return0, 0, 300);

	if(er->mchan != nil)
		cclose(er->mchan);
	if(er->achan != nil)
		cclose(er->achan);
	if(er->cchan != nil)
		cclose(er->cchan);

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
	a = arpget(er->f->arp, bp, version, ifc->m, ip, mac);
	if(a){
		/* check for broadcast or multicast */
		bp = multicastarp(er->f, a, ifc->m, mac);
		if(bp == nil){
			sendarp(ifc, a);
			return;
		}
	}

	/* make it a single block with space for the ether header */
	bp = padblock(bp, ifc->m->hsize);
	if(bp->next)
		bp = concatblock(bp);
	if(BLEN(bp) < ifc->minmtu)
		bp = adjustblock(bp, ifc->minmtu);
	eh = (Etherhdr*)bp->rp;

	/* copy in mac addresses and ether type */
	memmove(eh->s, ifc->mac, sizeof(eh->s));
	memmove(eh->d, mac, sizeof(eh->d));
	switch(version){
	case V4:
		eh->t[0] = 0x08;
		eh->t[1] = 0x00;
		break;
	case V6:
		eh->t[0] = 0x86;
		eh->t[1] = 0xDD;
		break;
	}

	devtab[er->mchan->type]->bwrite(er->mchan, bp, 0);
	ifc->out++;
}

/*
 *  process to read from the ethernet
 */
static void
etherread(void *a)
{
	Ipifc *ifc;
	Block *bp;
	Etherrock *er;

	ifc = a;
	er = ifc->arg;
	er->readp = up;	/* hide identity under a rock for unbind */
	if(waserror()){
		er->readp = 0;
		pexit("hangup", 1);
	}
	for(;;){
		bp = devtab[er->mchan->type]->bread(er->mchan, ifc->maxmtu, 0);
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
			ipiput(er->f, ifc, bp);
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

	multicastea(mac, a);
	sprint(buf, "addmulti %E", mac);
	devtab[er->cchan->type]->write(er->cchan, buf, strlen(buf), 0);
}

static void
etherremmulti(Ipifc *ifc, uchar *a, uchar *)
{
	uchar mac[6];
	char buf[64];
	Etherrock *er = ifc->arg;

	multicastea(mac, a);
	sprint(buf, "remmulti %E", mac);
	devtab[er->cchan->type]->write(er->cchan, buf, strlen(buf), 0);
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
	if(msec - a->time < 1000){
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
	a->time = msec;
	arprelease(er->f->arp, a);

	n = sizeof(Etherarp);
	if(n < a->type->minmtu)
		n = a->type->minmtu;
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
	hnputs(e->pro, ETIP);
	e->hln = sizeof(e->sha);
	e->pln = sizeof(e->spa);
	hnputs(e->op, ARPREQUEST);
	bp->wp += n;

	n = devtab[er->achan->type]->bwrite(er->achan, bp, 0);
	if(n < 0)
		print("arp: send: %r\n");
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
	if(n < ifc->m->minmtu)
		n = ifc->m->minmtu;
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
	hnputs(e->pro, ETIP);
	e->hln = sizeof(e->sha);
	e->pln = sizeof(e->spa);
	hnputs(e->op, ARPREQUEST);
	bp->wp += n;

	n = devtab[er->achan->type]->bwrite(er->achan, bp, 0);
	if(n < 0)
		print("garp: send: %r\n");
}

static void
recvarp(Ipifc *ifc)
{
	int n;
	Block *ebp, *rbp;
	Etherarp *e, *r;
	uchar ip[IPaddrlen];
	Etherrock *er = ifc->arg;

	ebp = devtab[er->achan->type]->bread(er->achan, ifc->maxmtu, 0);
	if(ebp == nil) {
		print("arp: rcv: %r\n");
		return;
	}

	e = (Etherarp*)ebp->rp;
	switch(nhgets(e->op)) {
	default:
		break;

	case ARPREPLY:
		if(iplocalonifc(ifc, ip) || ipproxyifc(er->f, ifc, ip)){
			if(memcmp(e->sha, ifc->mac, sizeof(e->sha)) != 0){
				print("arprep: 0x%E/0x%E also has ip addr %V\n",
					e->s, e->sha, e->spa);
				break;
			}
		}
		arpenter(er->f, V4, e->spa, e->sha, sizeof(e->sha), 0);
		break;

	case ARPREQUEST:
		/* don't answer arps till we know who we are */
		if(ifc->lifc == 0)
			break;

		/* check for someone that think's they're me */
		v4tov6(ip, e->spa);
		if(iplocalonifc(ifc, ip) || ipproxyifc(er->f, ifc, ip)){
			if(memcmp(e->sha, ifc->mac, sizeof(e->sha)) != 0){
				print("arpreq: 0x%E/0x%E also has ip addr %V\n",
					e->s, e->sha, e->spa);
				break;
			}
		} else {
			if(memcmp(e->sha, ifc->mac, sizeof(e->sha)) == 0){
				print("arp: %V also has ether addr %E\n", e->spa, e->sha);
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

/* print("arp: rem %I %E (for %I)\n", e->spa, e->sha, e->tpa); /**/

		n = sizeof(Etherarp);
		if(n < ifc->minmtu)
			n = ifc->minmtu;
		rbp = allocb(n);
		r = (Etherarp*)rbp->rp;
		memset(r, 0, sizeof(Etherarp));
		hnputs(r->type, ETARP);
		hnputs(r->hrd, 1);
		hnputs(r->pro, ETIP);
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

		n = devtab[er->achan->type]->bwrite(er->achan, rbp, 0);
		if(n < 0)
			print("arp: write: %r\n");
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
 *  addresses
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

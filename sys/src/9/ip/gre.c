/*
 * Generic Routing Encapsulation over IPv4, rfc1702
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#include "ip.h"

enum {
	GRE_IPONLY	= 12,		/* size of ip header */
	GRE_IPPLUSGRE	= 12,		/* minimum size of GRE header */
	IP_GREPROTO	= 47,

	GRErxms		= 200,
	GREtickms	= 100,
	GREmaxxmit	= 10,

	K		= 1024,
	GREqlen		= 256 * K,

	GRE_cksum	= 0x8000,
	GRE_routing	= 0x4000,
	GRE_key		= 0x2000,
	GRE_seq		= 0x1000,

	Nring		= 1 << 10,	/* power of two, please */
	Ringmask	= Nring - 1,

	GREctlraw	= 0,
	GREctlcooked,
	GREctlretunnel,
	GREctlreport,
	GREctldlsuspend,
	GREctlulsuspend,
	GREctldlresume,
	GREctlulresume,
	GREctlforward,
	GREctlulkey,
	Ncmds,
};

typedef struct GREhdr GREhdr;
struct GREhdr{
	/* ip header */
	uchar	vihl;		/* Version and header length */
	uchar	tos;		/* Type of service */
	uchar	len[2];		/* packet length (including headers) */
	uchar	id[2];		/* Identification */
	uchar	frag[2];	/* Fragment information */
	uchar	ttl;
	uchar	proto;		/* Protocol */
	uchar	cksum[2];	/* checksum */
	uchar	src[4];		/* Ip source */
	uchar	dst[4];		/* Ip destination */

	/* gre header */
	uchar	flags[2];
	uchar	eproto[2];	/* encapsulation protocol */
};

typedef struct GREpriv GREpriv;
struct GREpriv{
	/* non-MIB stats */
	ulong	lenerr;			/* short packet */
};

typedef struct Bring	Bring;
struct Bring{
	Block	*ring[Nring];
	long	produced;
	long	consumed;
};

typedef struct GREconv	GREconv;
struct GREconv{
	int	raw;

	/* Retunnelling information.  v4 only */
	uchar	north[4];			/* HA */
	uchar	south[4];			/* Base station */
	uchar	hoa[4];				/* Home address */
	uchar	coa[4];				/* Careof address */
	ulong	seq;				/* Current sequence # */
	int	dlsusp;				/* Downlink suspended? */
	int	ulsusp;				/* Uplink suspended? */
	ulong	ulkey;				/* GRE key */

	QLock	lock;				/* Lock for rings */
	Bring	dlpending;			/* Ring of pending packets */
	Bring	dlbuffered;			/* Received while suspended */
	Bring	ulbuffered;			/* Received while suspended */
};

typedef struct Metablock Metablock;
struct Metablock{
	uchar	*rp;
	ulong	seq;
};

static char *grectlcooked(Conv *, int, char **);
static char *grectldlresume(Conv *, int, char **);
static char *grectldlsuspend(Conv *, int, char **);
static char *grectlforward(Conv *, int, char **);
static char *grectlraw(Conv *, int, char **);
static char *grectlreport(Conv *, int, char **);
static char *grectlretunnel(Conv *, int, char **);
static char *grectlulkey(Conv *, int, char **);
static char *grectlulresume(Conv *, int, char **);
static char *grectlulsuspend(Conv *, int, char **);

static struct{
	char	*cmd;
	int	argc;
	char	*(*f)(Conv *, int, char **);
} grectls[Ncmds] = {
[GREctlraw]	=	{	"raw",		1,	grectlraw,	},
[GREctlcooked]	=	{	"cooked",	1,	grectlcooked,	},
[GREctlretunnel]=	{	"retunnel",	5,	grectlretunnel,	},
[GREctlreport]	=	{	"report",	2,	grectlreport,	},
[GREctldlsuspend]=	{	"dlsuspend",	1,	grectldlsuspend,},
[GREctlulsuspend]=	{	"ulsuspend",	1,	grectlulsuspend,},
[GREctldlresume]=	{	"dlresume",	1,	grectldlresume,	},
[GREctlulresume]=	{	"ulresume",	1,	grectlulresume,	},
[GREctlforward]	=	{	"forward",	2,	grectlforward,	},
[GREctlulkey]	=	{	"ulkey",	2,	grectlulkey,	},
};

static uchar nulladdr[4];
static char *sessend = "session end";

static void grekick(void *x, Block *bp);
static char *gresetup(Conv *, char *, char *, char *);

ulong grepdin, grepdout, grebdin, grebdout;
ulong grepuin, grepuout, grebuin, grebuout;

static Block *
getring(Bring *r)
{
	Block *bp;

	if(r->consumed == r->produced)
		return nil;

	bp = r->ring[r->consumed & Ringmask];
	r->ring[r->consumed & Ringmask] = nil;
	r->consumed++;
	return bp;
}

static void
addring(Bring *r, Block *bp)
{
	Block *tbp;

	if(r->produced - r->consumed > Ringmask){
		/* Full! */
		tbp = r->ring[r->produced & Ringmask];
		assert(tbp);
		freeb(tbp);
		r->consumed++;
	}
	r->ring[r->produced & Ringmask] = bp;
	r->produced++;
}

static char *
greconnect(Conv *c, char **argv, int argc)
{
	Proto *p;
	char *err;
	Conv *tc, **cp, **ecp;

	err = Fsstdconnect(c, argv, argc);
	if(err != nil)
		return err;

	/* make sure noone's already connected to this other sys */
	p = c->p;
	qlock(p);
	ecp = &p->conv[p->nc];
	for(cp = p->conv; cp < ecp; cp++){
		tc = *cp;
		if(tc == nil)
			break;
		if(tc == c)
			continue;
		if(tc->rport == c->rport && ipcmp(tc->raddr, c->raddr) == 0){
			err = "already connected to that addr/proto";
			ipmove(c->laddr, IPnoaddr);
			ipmove(c->raddr, IPnoaddr);
			break;
		}
	}
	qunlock(p);

	if(err != nil)
		return err;
	Fsconnected(c, nil);

	return nil;
}

static void
grecreate(Conv *c)
{
	c->rq = qopen(GREqlen, Qmsg, 0, c);
	c->wq = qbypass(grekick, c);
}

static int
grestate(Conv *c, char *state, int n)
{
	GREconv *grec;
	char *ep, *p;

	grec = c->ptcl;
	p    = state;
	ep   = p + n;
	p    = seprint(p, ep, "%s%s%s%shoa %V north %V south %V seq %ulx "
	 "pending %uld  %uld buffered dl %uld %uld ul %uld %uld ulkey %.8ulx\n",
			c->inuse? "Open ": "Closed ",
			grec->raw? "raw ": "",
			grec->dlsusp? "DL suspended ": "",
			grec->ulsusp? "UL suspended ": "",
			grec->hoa, grec->north, grec->south, grec->seq,
			grec->dlpending.consumed, grec->dlpending.produced,
			grec->dlbuffered.consumed, grec->dlbuffered.produced,
			grec->ulbuffered.consumed, grec->ulbuffered.produced,
			grec->ulkey);
	return p - state;
}

static char*
greannounce(Conv*, char**, int)
{
	return "gre does not support announce";
}

static void
greclose(Conv *c)
{
	GREconv *grec;
	Block *bp;

	grec = c->ptcl;

	/* Make sure we don't forward any more packets */
	memset(grec->hoa, 0, sizeof grec->hoa);
	memset(grec->north, 0, sizeof grec->north);
	memset(grec->south, 0, sizeof grec->south);

	qlock(&grec->lock);
	while((bp = getring(&grec->dlpending)) != nil)
		freeb(bp);

	while((bp = getring(&grec->dlbuffered)) != nil)
		freeb(bp);

	while((bp = getring(&grec->ulbuffered)) != nil)
		freeb(bp);

	grec->dlpending.produced = grec->dlpending.consumed = 0;
	grec->dlbuffered.produced = grec->dlbuffered.consumed = 0;
	grec->ulbuffered.produced = grec->ulbuffered.consumed = 0;
	qunlock(&grec->lock);

	grec->raw = 0;
	grec->seq = 0;
	grec->dlsusp = grec->ulsusp = 1;

	qhangup(c->rq, sessend);
	qhangup(c->wq, sessend);
	qhangup(c->eq, sessend);
	ipmove(c->laddr, IPnoaddr);
	ipmove(c->raddr, IPnoaddr);
	c->lport = c->rport = 0;
}

static void
grekick(void *x, Block *bp)
{
	Conv *c;
	GREconv *grec;
	GREhdr *gre;
	uchar laddr[IPaddrlen], raddr[IPaddrlen];

	if(bp == nil)
		return;

	c    = x;
	grec = c->ptcl;

	/* Make space to fit ip header (gre header already there) */
	bp = padblock(bp, GRE_IPONLY);
	if(bp == nil)
		return;

	/* make sure the message has a GRE header */
	bp = pullupblock(bp, GRE_IPONLY+GRE_IPPLUSGRE);
	if(bp == nil)
		return;

	gre = (GREhdr *)bp->rp;
	gre->vihl = IP_VER4;

	if(grec->raw == 0){
		v4tov6(raddr, gre->dst);
		if(ipcmp(raddr, v4prefix) == 0)
			memmove(gre->dst, c->raddr + IPv4off, IPv4addrlen);
		v4tov6(laddr, gre->src);
		if(ipcmp(laddr, v4prefix) == 0){
			if(ipcmp(c->laddr, IPnoaddr) == 0)
				/* pick interface closest to dest */
				findlocalip(c->p->f, c->laddr, raddr);
			memmove(gre->src, c->laddr + IPv4off, sizeof gre->src);
		}
		hnputs(gre->eproto, c->rport);
	}

	gre->proto = IP_GREPROTO;
	gre->frag[0] = gre->frag[1] = 0;

	grepdout++;
	grebdout += BLEN(bp);
	ipoput4(c->p->f, bp, 0, c->ttl, c->tos, nil);
}

static void
gredownlink(Conv *c, Block *bp)
{
	Metablock *m;
	GREconv *grec;
	GREhdr *gre;
	int hdrlen, suspended, extra;
	ushort flags;
	ulong seq;

	gre = (GREhdr *)bp->rp;
	if(gre->ttl == 1){
		freeb(bp);
		return;
	}

	/*
	 * We've received a packet with a GRE header and we need to
	 * re-adjust the packet header to strip all unwanted parts
	 * but leave room for only a sequence number.
	 */
	grec   = c->ptcl;
	flags  = nhgets(gre->flags);
	hdrlen = 0;
	if(flags & GRE_cksum)
		hdrlen += 2;
	if(flags & GRE_routing){
		print("%V routing info present.  Discarding packet", gre->src);
		freeb(bp);
		return;
	}
	if(flags & (GRE_cksum|GRE_routing))
		hdrlen += 2;			/* Offset field */
	if(flags & GRE_key)
		hdrlen += 4;
	if(flags & GRE_seq)
		hdrlen += 4;

	/*
	 * The outgoing packet only has the sequence number set.  Make room
	 * for the sequence number.
	 */
	if(hdrlen != sizeof(ulong)){
		extra = hdrlen - sizeof(ulong);
		if(extra < 0 && bp->rp - bp->base < -extra){
			print("gredownlink: cannot add sequence number\n");
			freeb(bp);
			return;
		}
		memmove(bp->rp + extra, bp->rp, sizeof(GREhdr));
		bp->rp += extra;
		assert(BLEN(bp) >= sizeof(GREhdr) + sizeof(ulong));
		gre = (GREhdr *)bp->rp;
	}
	seq = grec->seq++;
	hnputs(gre->flags, GRE_seq);
	hnputl(bp->rp + sizeof(GREhdr), seq);

	/*
	 * Keep rp and seq at the base.  ipoput4 consumes rp for
	 * refragmentation.
	 */
	assert(bp->rp - bp->base >= sizeof(Metablock));
	m = (Metablock *)bp->base;
	m->rp  = bp->rp;
	m->seq = seq;

	/*
	 * Here we make a decision what we're doing with the packet.  We're
	 * doing this w/o holding a lock which means that later on in the
	 * process we may discover we've done the wrong thing.  I don't want
	 * to call ipoput with the lock held.
	 */
restart:
	suspended = grec->dlsusp;
	if(suspended){
		if(!canqlock(&grec->lock)){
			/*
			 * just give up.  too bad, we lose a packet.  this
			 * is just too hard and my brain already hurts.
			 */
			freeb(bp);
			return;
		}

		if(!grec->dlsusp){
			/*
			 * suspend race.  We though we were suspended, but
			 * we really weren't.
			 */
			qunlock(&grec->lock);
			goto restart;
		}

		/* Undo the incorrect ref count addition */
		addring(&grec->dlbuffered, bp);
		qunlock(&grec->lock);
		return;
	}

	/*
	 * When we get here, we're not suspended.  Proceed to send the
	 * packet.
	 */
	memmove(gre->src, grec->coa, sizeof gre->dst);
	memmove(gre->dst, grec->south, sizeof gre->dst);

	/*
	 * Make sure the packet does not go away.
	 */
	_xinc(&bp->ref);
	assert(bp->ref == 2);

	ipoput4(c->p->f, bp, 0, gre->ttl - 1, gre->tos, nil);
	grepdout++;
	grebdout += BLEN(bp);

	/*
	 * Now make sure we didn't do the wrong thing.
	 */
	if(!canqlock(&grec->lock)){
		freeb(bp);		/* The packet just goes away */
		return;
	}

	/* We did the right thing */
	addring(&grec->dlpending, bp);
	qunlock(&grec->lock);
}

static void
greuplink(Conv *c, Block *bp)
{
	GREconv *grec;
	GREhdr *gre;
	ushort flags;

	gre = (GREhdr *)bp->rp;
	if(gre->ttl == 1)
		return;

	grec = c->ptcl;
	memmove(gre->src, grec->coa, sizeof gre->src);
	memmove(gre->dst, grec->north, sizeof gre->dst);

	/*
	 * Add a key, if needed.
	 */
	if(grec->ulkey){
		flags = nhgets(gre->flags);
		if(flags & (GRE_cksum|GRE_routing)){
			print("%V routing info present.  Discarding packet\n",
				gre->src);
			freeb(bp);
			return;
		}

		if((flags & GRE_key) == 0){
			/* Make room for the key */
			if(bp->rp - bp->base < sizeof(ulong)){
				print("%V can't add key\n", gre->src);
				freeb(bp);
				return;
			}

			bp->rp -= 4;
			memmove(bp->rp, bp->rp + 4, sizeof(GREhdr));

			gre = (GREhdr *)bp->rp;
			hnputs(gre->flags, flags | GRE_key);
		}

		/* Add the key */
		hnputl(bp->rp + sizeof(GREhdr), grec->ulkey);
	}

	if(!canqlock(&grec->lock)){
		freeb(bp);
		return;
	}

	if(grec->ulsusp)
		addring(&grec->ulbuffered, bp);
	else{
		ipoput4(c->p->f, bp, 0, gre->ttl - 1, gre->tos, nil);
		grepuout++;
		grebuout += BLEN(bp);
	}
	qunlock(&grec->lock);
}

static void
greiput(Proto *proto, Ipifc *, Block *bp)
{
	int len, hdrlen;
	ushort eproto, flags;
	uchar raddr[IPaddrlen];
	Conv *c, **p;
	GREconv *grec;
	GREhdr *gre;
	GREpriv *gpriv;
	Ip4hdr *ip;

	/*
	 * We don't want to deal with block lists.  Ever.  The problem is
	 * that when the block is forwarded, devether.c puts the block into
	 * a queue that also uses ->next.  Just do not use ->next here!
	 */
	if(bp->next){
		len = blocklen(bp);
		bp  = pullupblock(bp, len);
		assert(BLEN(bp) == len && bp->next == nil);
	}

	gre = (GREhdr *)bp->rp;
	if(BLEN(bp) < sizeof(GREhdr) || gre->proto != IP_GREPROTO){
		freeb(bp);
		return;
	}

	v4tov6(raddr, gre->src);
	eproto = nhgets(gre->eproto);
	flags  = nhgets(gre->flags);
	hdrlen = sizeof(GREhdr);

	if(flags & GRE_cksum)
		hdrlen += 2;
	if(flags & GRE_routing){
		print("%I routing info present.  Discarding packet\n", raddr);
		freeb(bp);
		return;
	}
	if(flags & (GRE_cksum|GRE_routing))
		hdrlen += 2;			/* Offset field */
	if(flags & GRE_key)
		hdrlen += 4;
	if(flags & GRE_seq)
		hdrlen += 4;

	if(BLEN(bp) - hdrlen < sizeof(Ip4hdr)){
		print("greretunnel: packet too short (s=%V d=%V)\n",
			gre->src, gre->dst);
		freeb(bp);
		return;
	}
	ip = (Ip4hdr *)(bp->rp + hdrlen);

	qlock(proto);
	/*
	 * Look for a conversation structure for this port and address, or
	 * match the retunnel part, or match on the raw flag.
	 */
	for(p = proto->conv; *p; p++) {
		c = *p;

		if(c->inuse == 0)
			continue;

		/*
		 * Do not stop this session - blocking here
		 * implies that etherread is blocked.
		 */
		grec = c->ptcl;
		if(memcmp(ip->dst, grec->hoa, sizeof ip->dst) == 0){
			grepdin++;
			grebdin += BLEN(bp);
			gredownlink(c, bp);
			qunlock(proto);
			return;
		}

		if(memcmp(ip->src, grec->hoa, sizeof ip->src) == 0){
			grepuin++;
			grebuin += BLEN(bp);
			greuplink(c, bp);
			qunlock(proto);
			return;
		}
	}

	/*
	 * when we get here, none of the forwarding tunnels matched.  now
	 * try to match on raw and conversational sessions.
	 */
	for(c = nil, p = proto->conv; *p; p++) {
		c = *p;

		if(c->inuse == 0)
			continue;

		/*
		 * Do not stop this session - blocking here
		 * implies that etherread is blocked.
		 */
		grec = c->ptcl;
		if(c->rport == eproto &&
		    (grec->raw || ipcmp(c->raddr, raddr) == 0))
			break;
	}

	qunlock(proto);

	if(*p == nil){
		freeb(bp);
		return;
	}

	/*
	 * Trim the packet down to data size
	 */
	len = nhgets(gre->len) - GRE_IPONLY;
	if(len < GRE_IPPLUSGRE){
		freeb(bp);
		return;
	}

	bp = trimblock(bp, GRE_IPONLY, len);
	if(bp == nil){
		gpriv = proto->priv;
		gpriv->lenerr++;
		return;
	}

	/*
	 *  Can't delimit packet so pull it all into one block.
	 */
	if(qlen(c->rq) > GREqlen)
		freeb(bp);
	else{
		bp = concatblock(bp);
		if(bp == 0)
			panic("greiput");
		qpass(c->rq, bp);
	}
}

int
grestats(Proto *gre, char *buf, int len)
{
	GREpriv *gpriv;

	gpriv = gre->priv;
	return snprint(buf, len,
		"gre: %lud %lud %lud %lud %lud %lud %lud %lud, lenerrs %lud\n",
		grepdin, grepdout, grepuin, grepuout,
		grebdin, grebdout, grebuin, grebuout, gpriv->lenerr);
}

static char *
grectlraw(Conv *c, int, char **)
{
	GREconv *grec;

	grec = c->ptcl;
	grec->raw = 1;
	return nil;
}

static char *
grectlcooked(Conv *c, int, char **)
{
	GREconv *grec;

	grec = c->ptcl;
	grec->raw = 0;
	return nil;
}

static char *
grectlretunnel(Conv *c, int, char **argv)
{
	GREconv *grec;
	uchar ipaddr[4];

	grec = c->ptcl;
	if(memcmp(grec->hoa, nulladdr, sizeof grec->hoa))
		return "tunnel already set up";

	v4parseip(ipaddr, argv[1]);
	if(memcmp(ipaddr, nulladdr, sizeof ipaddr) == 0)
		return "bad hoa";
	memmove(grec->hoa, ipaddr, sizeof grec->hoa);
	v4parseip(ipaddr, argv[2]);
	memmove(grec->north, ipaddr, sizeof grec->north);
	v4parseip(ipaddr, argv[3]);
	memmove(grec->south, ipaddr, sizeof grec->south);
	v4parseip(ipaddr, argv[4]);
	memmove(grec->coa, ipaddr, sizeof grec->coa);
	grec->ulsusp = 1;
	grec->dlsusp = 0;

	return nil;
}

static char *
grectlreport(Conv *c, int, char **argv)
{
	ulong seq;
	Block *bp;
	Bring *r;
	GREconv *grec;
	Metablock *m;

	grec = c->ptcl;
	seq  = strtoul(argv[1], nil, 0);

	qlock(&grec->lock);
	r = &grec->dlpending;
	while(r->produced - r->consumed > 0){
		bp = r->ring[r->consumed & Ringmask];

		assert(bp && bp->rp - bp->base >= sizeof(Metablock));
		m = (Metablock *)bp->base;
		if((long)(seq - m->seq) <= 0)
			break;

		r->ring[r->consumed & Ringmask] = nil;
		r->consumed++;

		freeb(bp);
	}
	qunlock(&grec->lock);
	return nil;
}

static char *
grectldlsuspend(Conv *c, int, char **)
{
	GREconv *grec;

	grec = c->ptcl;
	if(grec->dlsusp)
		return "already suspended";

	grec->dlsusp = 1;
	return nil;
}

static char *
grectlulsuspend(Conv *c, int, char **)
{
	GREconv *grec;

	grec = c->ptcl;
	if(grec->ulsusp)
		return "already suspended";

	grec->ulsusp = 1;
	return nil;
}

static char *
grectldlresume(Conv *c, int, char **)
{
	GREconv *grec;
	GREhdr *gre;
	Block *bp;

	grec = c->ptcl;

	qlock(&grec->lock);
	if(!grec->dlsusp){
		qunlock(&grec->lock);
		return "not suspended";
	}

	while((bp = getring(&grec->dlbuffered)) != nil){
		gre = (GREhdr *)bp->rp;
		qunlock(&grec->lock);

		/*
		 * Make sure the packet does not go away.
		 */
		_xinc(&bp->ref);
		assert(bp->ref == 2);

		ipoput4(c->p->f, bp, 0, gre->ttl - 1, gre->tos, nil);

		qlock(&grec->lock);
		addring(&grec->dlpending, bp);
	}
	grec->dlsusp = 0;
	qunlock(&grec->lock);
	return nil;
}

static char *
grectlulresume(Conv *c, int, char **)
{
	GREconv *grec;
	GREhdr *gre;
	Block *bp;

	grec = c->ptcl;

	qlock(&grec->lock);
	while((bp = getring(&grec->ulbuffered)) != nil){
		gre = (GREhdr *)bp->rp;

		qunlock(&grec->lock);
		ipoput4(c->p->f, bp, 0, gre->ttl - 1, gre->tos, nil);
		qlock(&grec->lock);
	}
	grec->ulsusp = 0;
	qunlock(&grec->lock);
	return nil;
}

static char *
grectlforward(Conv *c, int, char **argv)
{
	int len;
	Block *bp, *nbp;
	GREconv *grec;
	GREhdr *gre;
	Metablock *m;

	grec = c->ptcl;

	v4parseip(grec->south, argv[1]);
	memmove(grec->north, grec->south, sizeof grec->north);

	qlock(&grec->lock);
	if(!grec->dlsusp){
		qunlock(&grec->lock);
		return "not suspended";
	}
	grec->dlsusp = 0;
	grec->ulsusp = 0;

	while((bp = getring(&grec->dlpending)) != nil){

		assert(bp->rp - bp->base >= sizeof(Metablock));
		m = (Metablock *)bp->base;
		assert(m->rp >= bp->base && m->rp < bp->lim);

		/*
		 * If the packet is still held inside the IP transmit
		 * system, make a copy of the packet first.
		 */
		if(bp->ref > 1){
			len = bp->wp - m->rp;
			nbp = allocb(len);
			memmove(nbp->wp, m->rp, len);
			nbp->wp += len;
			freeb(bp);
			bp  = nbp;
		}
		else{
			/* Patch up rp */
			bp->rp = m->rp;
		}

		gre = (GREhdr *)bp->rp;
		memmove(gre->src, grec->coa, sizeof gre->dst);
		memmove(gre->dst, grec->south, sizeof gre->dst);

		qunlock(&grec->lock);
		ipoput4(c->p->f, bp, 0, gre->ttl - 1, gre->tos, nil);
		qlock(&grec->lock);
	}

	while((bp = getring(&grec->dlbuffered)) != nil){
		gre = (GREhdr *)bp->rp;
		memmove(gre->src, grec->coa, sizeof gre->dst);
		memmove(gre->dst, grec->south, sizeof gre->dst);

		qunlock(&grec->lock);
		ipoput4(c->p->f, bp, 0, gre->ttl - 1, gre->tos, nil);
		qlock(&grec->lock);
	}

	while((bp = getring(&grec->ulbuffered)) != nil){
		gre = (GREhdr *)bp->rp;

		memmove(gre->src, grec->coa, sizeof gre->dst);
		memmove(gre->dst, grec->south, sizeof gre->dst);

		qunlock(&grec->lock);
		ipoput4(c->p->f, bp, 0, gre->ttl - 1, gre->tos, nil);
		qlock(&grec->lock);
	}
	qunlock(&grec->lock);
	return nil;
}

static char *
grectlulkey(Conv *c, int, char **argv)
{
	GREconv *grec;

	grec = c->ptcl;
	grec->ulkey = strtoul(argv[1], nil, 0);
	return nil;
}

char *
grectl(Conv *c, char **f, int n)
{
	int i;

	if(n < 1)
		return "too few arguments";

	for(i = 0; i < Ncmds; i++)
		if(strcmp(f[0], grectls[i].cmd) == 0)
			break;

	if(i == Ncmds)
		return "no such command";
	if(grectls[i].argc != 0 && grectls[i].argc != n)
		return "incorrect number of arguments";

	return grectls[i].f(c, n, f);
}

void
greinit(Fs *fs)
{
	Proto *gre;

	gre = smalloc(sizeof(Proto));
	gre->priv = smalloc(sizeof(GREpriv));
	gre->name = "gre";
	gre->connect = greconnect;
	gre->announce = greannounce;
	gre->state = grestate;
	gre->create = grecreate;
	gre->close = greclose;
	gre->rcv = greiput;
	gre->ctl = grectl;
	gre->advise = nil;
	gre->stats = grestats;
	gre->ipproto = IP_GREPROTO;
	gre->nc = 64;
	gre->ptclsize = sizeof(GREconv);

	Fsproto(fs, gre);
}

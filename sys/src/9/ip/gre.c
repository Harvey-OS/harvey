#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#include "ip.h"

#define DPRINT if(0)print

enum
{
	GRE_IPONLY	= 12,		/* size of ip header */
	GRE_IPPLUSGRE	= 12,		/* minimum size of GRE header */
	IP_GREPROTO	= 47,

	GRErxms		= 200,
	GREtickms	= 100,
	GREmaxxmit	= 10,
};

typedef struct GREhdr
{
	/* ip header */
	uchar	vihl;		/* Version and header length */
	uchar	tos;		/* Type of service */
	uchar	len[2];		/* packet length (including headers) */
	uchar	id[2];		/* Identification */
	uchar	frag[2];	/* Fragment information */
	uchar	Unused;	
	uchar	proto;		/* Protocol */
	uchar	cksum[2];	/* checksum */
	uchar	src[4];		/* Ip source */
	uchar	dst[4];		/* Ip destination */

	/* gre header */
	uchar	flags[2];
	uchar	eproto[2];	/* encapsulation protocol */
} GREhdr;

typedef struct GREpriv GREpriv;
struct GREpriv
{
	/* non-MIB stats */
	ulong		csumerr;		/* checksum errors */
	ulong		lenerr;			/* short packet */
};

static char*
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

static int
grestate(Conv *c, char *state, int n)
{
	USED(c);
	return snprint(state, n, "%s", "Datagram");
}

static void
grecreate(Conv *c)
{
	c->rq = qopen(64*1024, 1, 0, c);
	c->wq = qopen(64*1024, 0, 0, 0);
}

static char*
greannounce(Conv*, char**, int)
{
	return "pktifc does not support announce";
}

static void
greclose(Conv *c)
{
	qclose(c->rq);
	qclose(c->wq);
	qclose(c->eq);
	ipmove(c->laddr, IPnoaddr);
	ipmove(c->raddr, IPnoaddr);
	c->lport = 0;
	c->rport = 0;
}

int drop;

static void
grekick(Conv *c)
{
	GREhdr *ghp;
	Block *bp;
	uchar laddr[IPaddrlen], raddr[IPaddrlen];

	bp = qget(c->wq);
	if(bp == nil)
		return;

	/* Make space to fit ip header (gre header already there) */
	bp = padblock(bp, GRE_IPONLY);
	if(bp == nil)
		return;

	/* make sure the message has a GRE header */
	bp = pullupblock(bp, GRE_IPONLY+GRE_IPPLUSGRE);
	if(bp == nil)
		return;

	ghp = (GREhdr *)(bp->rp);

	v4tov6(raddr, ghp->dst);
	if(ipcmp(raddr, v4prefix) == 0)
		memmove(ghp->dst, c->raddr + IPv4off, IPv4addrlen);
	v4tov6(laddr, ghp->src);
	if(ipcmp(laddr, v4prefix) == 0){
		if(ipcmp(c->laddr, IPnoaddr) == 0)
			findlocalip(c->p->f, c->laddr, raddr); /* pick interface closest to dest */
		memmove(ghp->src, c->laddr + IPv4off, IPv4addrlen);
	}

	ghp->proto = IP_GREPROTO;
	hnputs(ghp->eproto, c->rport);
	ghp->frag[0] = 0;
	ghp->frag[1] = 0;

	ipoput(c->p->f, bp, 0, c->ttl, c->tos);
}

static void
greiput(Proto *gre, Ipifc*, Block *bp)
{
	int len;
	GREhdr *ghp;
	Conv *c, **p;
	ushort eproto;
	uchar raddr[IPaddrlen];
	GREpriv *gpriv;

	gpriv = gre->priv;
	ghp = (GREhdr*)(bp->rp);

	v4tov6(raddr, ghp->src);
	eproto = nhgets(ghp->eproto);

	qlock(gre);

	/* Look for a conversation structure for this port and address */
	c = nil;
	for(p = gre->conv; *p; p++) {
		c = *p;
		if(c->inuse == 0)
			continue;
		if(c->rport == eproto && ipcmp(c->raddr, raddr) == 0)
			break;
	}

	if(*p == nil) {
		qunlock(gre);
		freeblist(bp);
		return;
	}

	qunlock(gre);

	/*
	 * Trim the packet down to data size
	 */
	len = nhgets(ghp->len) - GRE_IPONLY;
	if(len < GRE_IPPLUSGRE){
		freeblist(bp);
		return;
	}
	bp = trimblock(bp, GRE_IPONLY, len);
	if(bp == nil){
		gpriv->lenerr++;
		return;
	}

	/*
	 *  Can't delimit packet so pull it all into one block.
	 */
	if(qlen(c->rq) > 64*1024)
		freeblist(bp);
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

	return snprint(buf, len, "gre: len %lud\n", gpriv->lenerr);
}

void
greinit(Fs *fs)
{
	Proto *gre;

	gre = smalloc(sizeof(Proto));
	gre->priv = smalloc(sizeof(GREpriv));
	gre->name = "gre";
	gre->kick = grekick;
	gre->connect = greconnect;
	gre->announce = greannounce;
	gre->state = grestate;
	gre->create = grecreate;
	gre->close = greclose;
	gre->rcv = greiput;
	gre->ctl = nil;
	gre->advise = nil;
	gre->stats = grestats;
	gre->ipproto = IP_GREPROTO;
	gre->nc = 64;
	gre->ptclsize = 0;

	Fsproto(fs, gre);
}

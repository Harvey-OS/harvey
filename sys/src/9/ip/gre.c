/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
	uint8_t	vihl;		/* Version and header length */
	uint8_t	tos;		/* Type of service */
	uint8_t	len[2];		/* packet length (including headers) */
	uint8_t	id[2];		/* Identification */
	uint8_t	frag[2];	/* Fragment information */
	uint8_t	Unused;
	uint8_t	proto;		/* Protocol */
	uint8_t	cksum[2];	/* checksum */
	uint8_t	src[4];		/* Ip source */
	uint8_t	dst[4];		/* Ip destination */

	/* gre header */
	uint8_t	flags[2];
	uint8_t	eproto[2];	/* encapsulation protocol */
} GREhdr;

typedef struct GREpriv GREpriv;
struct GREpriv
{
	int		raw;			/* Raw GRE mode */

	/* non-MIB stats */
	uint32_t		csumerr;		/* checksum errors */
	uint32_t		lenerr;			/* short packet */
};

static void grekick(void *x, Block *bp);

static int8_t*
greconnect(Conv *c, int8_t **argv, int argc)
{
	Proto *p;
	int8_t *err;
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
	c->rq = qopen(64*1024, Qmsg, 0, c);
	c->wq = qbypass(grekick, c);
}

static int
grestate(Conv *c, int8_t *state, int n)
{
	USED(c);
	return snprint(state, n, "%s\n", "Datagram");
}

static int8_t*
greannounce(Conv*, int8_t**, int)
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

static void
grekick(void *x, Block *bp)
{
	Conv *c = x;
	GREhdr *ghp;
	uint8_t laddr[IPaddrlen], raddr[IPaddrlen];

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
	ghp->vihl = IP_VER4;

	if(!((GREpriv*)c->p->priv)->raw){
		v4tov6(raddr, ghp->dst);
		if(ipcmp(raddr, v4prefix) == 0)
			memmove(ghp->dst, c->raddr + IPv4off, IPv4addrlen);
		v4tov6(laddr, ghp->src);
		if(ipcmp(laddr, v4prefix) == 0){
			if(ipcmp(c->laddr, IPnoaddr) == 0)
				findlocalip(c->p->f, c->laddr, raddr); /* pick interface closest to dest */
			memmove(ghp->src, c->laddr + IPv4off, IPv4addrlen);
		}
		hnputs(ghp->eproto, c->rport);
	}

	ghp->proto = IP_GREPROTO;
	ghp->frag[0] = 0;
	ghp->frag[1] = 0;

	ipoput4(c->p->f, bp, 0, c->ttl, c->tos, nil);
}

static void
greiput(Proto *gre, Ipifc*, Block *bp)
{
	int len;
	GREhdr *ghp;
	Conv *c, **p;
	uint16_t eproto;
	uint8_t raddr[IPaddrlen];
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
		if(c->rport == eproto &&
			(gpriv->raw || ipcmp(c->raddr, raddr) == 0))
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
grestats(Proto *gre, int8_t *buf, int len)
{
	GREpriv *gpriv;

	gpriv = gre->priv;

	return snprint(buf, len, "gre: len %lud\n", gpriv->lenerr);
}

int8_t*
grectl(Conv *c, int8_t **f, int n)
{
	GREpriv *gpriv;

	gpriv = c->p->priv;
	if(n == 1){
		if(strcmp(f[0], "raw") == 0){
			gpriv->raw = 1;
			return nil;
		}
		else if(strcmp(f[0], "cooked") == 0){
			gpriv->raw = 0;
			return nil;
		}
	}
	return "unknown control request";
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
	gre->ptclsize = 0;

	Fsproto(fs, gre);
}

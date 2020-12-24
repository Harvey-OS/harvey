/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * igmp - internet group management protocol
 * unfinished.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#include "ip.h"

enum {
	IGMP_IPHDRSIZE = 20, /* size of ip header */
	IGMP_HDRSIZE = 8,    /* size of IGMP header */
	IP_IGMPPROTO = 2,

	IGMPquery = 1,
	IGMPreport = 2,

	MSPTICK = 100,
	MAXTIMEOUT = 10000 / MSPTICK, /* at most 10 secs for a response */
};

typedef struct IGMPpkt IGMPpkt;
struct IGMPpkt {
	/* ip header */
	u8 vihl;	 /* Version and header length */
	u8 tos;	 /* Type of service */
	u8 len[2];	 /* packet length (including headers) */
	u8 id[2];	 /* Identification */
	u8 frag[2]; /* Fragment information */
	u8 Unused;
	u8 proto;		/* Protocol */
	u8 cksum[2];	/* checksum of ip portion */
	u8 src[IPaddrlen]; /* Ip source */
	u8 dst[IPaddrlen]; /* Ip destination */

	/* igmp header */
	u8 vertype; /* version and type */
	u8 unused;
	u8 igmpcksum[2];	  /* checksum of igmp portion */
	u8 group[IPaddrlen]; /* multicast group */

	u8 payload[];
};

#define IGMPPKTSZ offsetof(IGMPpkt, payload[0])

/*
 *  lists for group reports
 */
typedef struct IGMPrep IGMPrep;
struct IGMPrep {
	IGMPrep *next;
	Medium *m;
	int ticks;
	Multicast *multi;
};

typedef struct IGMP IGMP;
struct IGMP {
	Lock l;
	Rendez r;
	IGMPrep *reports;
};

IGMP igmpalloc;

Proto igmp;
extern Fs fs;

static struct Stats {
	u32 inqueries;
	u32 outqueries;
	u32 inreports;
	u32 outreports;
} stats;

void
igmpsendreport(Medium *m, u8 *addr)
{
	IGMPpkt *p;
	Block *bp;

	bp = allocb(sizeof(IGMPpkt));
	if(bp == nil)
		return;
	p = (IGMPpkt *)bp->wp;
	p->vihl = IP_VER4;
	bp->wp += IGMPPKTSZ;
	memset(bp->rp, 0, IGMPPKTSZ);
	hnputl(p->src, Mediumgetaddr(m));
	hnputl(p->dst, Ipallsys);
	p->vertype = (1 << 4) | IGMPreport;
	p->proto = IP_IGMPPROTO;
	memmove(p->group, addr, IPaddrlen);
	hnputs(p->igmpcksum, ptclcsum(bp, IGMP_IPHDRSIZE, IGMP_HDRSIZE));
	netlog(Logigmp, "igmpreport %I\n", p->group);
	stats.outreports++;
	ipoput4(bp, 0, 1, DFLTTOS, nil); /* TTL of 1 */
}

static int
isreport(void *a)
{
	USED(a);
	return igmpalloc.reports != 0;
}

void
igmpproc(void *a)
{
	IGMPrep *rp, **lrp;
	Multicast *mp, **lmp;
	u8 ip[IPaddrlen];

	USED(a);

	for(;;){
		sleep(&igmpalloc.r, isreport, 0);
		for(;;){
			lock(&igmpalloc);

			if(igmpalloc.reports == nil)
				break;

			/* look for a single report */
			lrp = &igmpalloc.reports;
			mp = nil;
			for(rp = *lrp; rp; rp = *lrp){
				rp->ticks++;
				lmp = &rp->multi;
				for(mp = *lmp; mp; mp = *lmp){
					if(rp->ticks >= mp->timeout){
						*lmp = mp->next;
						break;
					}
					lmp = &mp->next;
				}
				if(mp != nil)
					break;

				if(rp->multi != nil){
					lrp = &rp->next;
					continue;
				} else {
					*lrp = rp->next;
					free(rp);
				}
			}
			unlock(&igmpalloc);

			if(mp){
				/* do a single report and try again */
				hnputl(ip, mp->addr);
				igmpsendreport(rp->m, ip);
				free(mp);
				continue;
			}

			tsleep(&up->sleep, return0, 0, MSPTICK);
		}
		unlock(&igmpalloc);
	}
}

void
igmpiput(Medium *m, Ipifc *, Block *bp)
{
	int n;
	IGMPpkt *ghp;
	Ipaddr group;
	IGMPrep *rp, **lrp;
	Multicast *mp, **lmp;

	ghp = (IGMPpkt *)(bp->rp);
	netlog(Logigmp, "igmpiput: %d %I\n", ghp->vertype, ghp->group);

	n = blocklen(bp);
	if(n < IGMP_IPHDRSIZE + IGMP_HDRSIZE){
		netlog(Logigmp, "igmpiput: bad len\n");
		goto error;
	}
	if((ghp->vertype >> 4) != 1){
		netlog(Logigmp, "igmpiput: bad igmp type\n");
		goto error;
	}
	if(ptclcsum(bp, IGMP_IPHDRSIZE, IGMP_HDRSIZE)){
		netlog(Logigmp, "igmpiput: checksum error %I\n", ghp->src);
		goto error;
	}

	group = nhgetl(ghp->group);

	lock(&igmpalloc);
	switch(ghp->vertype & 0xf){
	case IGMPquery:
		/*
		 *  start reporting groups that we're a member of.
		 */
		stats.inqueries++;
		for(rp = igmpalloc.reports; rp; rp = rp->next)
			if(rp->m == m)
				break;
		if(rp != nil)
			break; /* already reporting */

		mp = Mediumcopymulti(m);
		if(mp == nil)
			break;

		rp = malloc(sizeof(*rp));
		if(rp == nil)
			break;

		rp->m = m;
		rp->multi = mp;
		rp->ticks = 0;
		for(; mp; mp = mp->next)
			mp->timeout = nrand(MAXTIMEOUT);
		rp->next = igmpalloc.reports;
		igmpalloc.reports = rp;

		wakeup(&igmpalloc.r);

		break;
	case IGMPreport:
		/*
		 *  find report list for this medium
		 */
		stats.inreports++;
		lrp = &igmpalloc.reports;
		for(rp = *lrp; rp; rp = *lrp){
			if(rp->m == m)
				break;
			lrp = &rp->next;
		}
		if(rp == nil)
			break;

		/*
		 *  if someone else has reported a group,
		 *  we don't have to.
		 */
		lmp = &rp->multi;
		for(mp = *lmp; mp; mp = *lmp){
			if(mp->addr == group){
				*lmp = mp->next;
				free(mp);
				break;
			}
			lmp = &mp->next;
		}

		break;
	}
	unlock(&igmpalloc);

error:
	freeb(bp);
}

int
igmpstats(char *buf, int len)
{
	return snprint(buf, len, "\trcvd %d %d\n\tsent %d %d\n",
		       stats.inqueries, stats.inreports,
		       stats.outqueries, stats.outreports);
}

void
igmpinit(Fs *fs)
{
	igmp.name = "igmp";
	igmp.connect = nil;
	igmp.announce = nil;
	igmp.ctl = nil;
	igmp.state = nil;
	igmp.close = nil;
	igmp.rcv = igmpiput;
	igmp.stats = igmpstats;
	igmp.ipproto = IP_IGMPPROTO;
	igmp.nc = 0;
	igmp.ptclsize = 0;

	igmpreportfn = igmpsendreport;
	kproc("igmpproc", igmpproc, 0);

	Fsproto(fs, &igmp);
}

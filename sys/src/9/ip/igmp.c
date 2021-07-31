#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#include "ip.h"

enum
{
	IGMP_IPHDRSIZE	= 20,		/* size of ip header */
	IGMP_HDRSIZE	= 8,		/* size of IGMP header */
	IP_IGMPPROTO	= 2,

	IGMPquery	= 1,
	IGMPreport	= 2,

	MSPTICK		= 100,
	MAXTIMEOUT	= 10000/MSPTICK,	/* at most 10 secs for a response */
};

typedef struct IGMPpkt IGMPpkt;
struct IGMPpkt
{
	/* ip header */
	byte	vihl;		/* Version and header length */
	byte	tos;		/* Type of service */
	byte	len[2];		/* packet length (including headers) */
	byte	id[2];		/* Identification */
	byte	frag[2];	/* Fragment information */
	byte	Unused;	
	byte	proto;		/* Protocol */
	byte	cksum[2];	/* checksum of ip portion */
	byte	src[IPaddrlen];		/* Ip source */
	byte	dst[IPaddrlen];		/* Ip destination */

	/* igmp header */
	byte	vertype;	/* version and type */
	byte	unused;
	byte	igmpcksum[2];		/* checksum of igmp portion */
	byte	group[IPaddrlen];	/* multicast group */
};

/*
 *  lists for group reports
 */
typedef struct IGMPrep IGMPrep;
struct IGMPrep
{
	IGMPrep		*next;
	Media		*m;
	int		ticks;
	Multicast	*multi;
};

typedef struct IGMP IGMP;
struct IGMP
{
	Lock;
	Rendez	r;
	Rendez	r0;
	IGMPrep	*reports;
};

IGMP igmpalloc;

	Proto	igmp;
extern	Fs	fs;

static struct Stats
{
	ulong 	inqueries;
	ulong	outqueries;
	ulong	inreports;
	ulong	outreports;
} stats;

void
igmpsendreport(Media *m, byte *addr)
{
	IGMPpkt *p;
	Block *bp;

	bp = allocb(sizeof(IGMPpkt));
	if(bp == nil)
		return;
	p = (IGMPpkt*)bp->wp;
	bp->wp += sizeof(IGMPpkt);
	memset(bp->rp, 0, sizeof(IGMPpkt));
	hnputl(p->src, Mediagetaddr(m));
	hnputl(p->dst, Ipallsys);
	p->vertype = (1<<4) | IGMPreport;
	p->proto = IP_IGMPPROTO;
	memmove(p->group, addr, IPaddrlen);
	hnputs(p->igmpcksum, ptclcsum(bp, IGMP_IPHDRSIZE, IGMP_HDRSIZE));
	netlog(Logigmp, "igmpreport %I\n", p->group);
	stats.outreports++;
	ipoput(bp, 0, 1, DFLTTOS);	/* TTL of 1 */
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
	byte ip[IPaddrlen];

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

			tsleep(&igmpalloc.r0, return0, 0, MSPTICK);
		}
		unlock(&igmpalloc);
	}

}

void
igmpiput(Media *m, Ipifc*, Block *bp)
{
	int n;
	IGMPpkt *ghp;
	Ipaddr group;
	IGMPrep *rp, **lrp;
	Multicast *mp, **lmp;

	ghp = (IGMPpkt*)(bp->rp);
	netlog(Logigmp, "igmpiput: %d %I\n", ghp->vertype, ghp->group);

	n = blocklen(bp);
	if(n < IGMP_IPHDRSIZE+IGMP_HDRSIZE){
		netlog(Logigmp, "igmpiput: bad len\n");
		goto error;
	}
	if((ghp->vertype>>4) != 1){
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
			break;	/* already reporting */

		mp = Mediacopymulti(m);
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
	igmp.kick = nil;
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

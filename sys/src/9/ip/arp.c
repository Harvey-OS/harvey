#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#include "ip.h"
#include "ipv6.h"

/*
 *  address resolution tables
 */

enum
{
	NHASH		= (1<<6),
	NCACHE		= 256,

	AOK		= 1,
	AWAIT		= 2,
};

char *arpstate[] =
{
	"UNUSED",
	"OK",
	"WAIT",
};

/*
 *  one per Fs
 */
struct Arp
{
	QLock;
	Fs	*f;
	Arpent	*hash[NHASH];
	Arpent	cache[NCACHE];
	Arpent	*rxmt;
	Proc	*rxmitp;	/* neib sol re-transmit proc */
	Rendez	rxmtq;
	Block 	*dropf, *dropl;
};

char *Ebadarp = "bad arp";

#define haship(s) ((s)[IPaddrlen-1]%NHASH)

extern int 	ReTransTimer = RETRANS_TIMER;

static void 	rxmitproc(void *v);

void
arpinit(Fs *f)
{
	f->arp = smalloc(sizeof(Arp));
	f->arp->f = f;
	f->arp->rxmt = nil;
	f->arp->dropf = f->arp->dropl = nil;
	kproc("rxmitproc", rxmitproc, f->arp);
}

/*
 *  create a new arp entry for an ip address.
 */
static Arpent*
newarp6(Arp *arp, uchar *ip, Ipifc *ifc, int addrxt)
{
	uint t;
	Block *next, *xp;
	Arpent *a, *e, *f, **l;
	Medium *m = ifc->m;
	int empty;

	/* find oldest entry */
	e = &arp->cache[NCACHE];
	a = arp->cache;
	t = a->utime;
	for(f = a; f < e; f++){
		if(f->utime < t){
			t = f->utime;
			a = f;
		}
	}

	/* dump waiting packets */
	xp = a->hold;
	a->hold = nil;

	if(isv4(a->ip)){
		while(xp){
			next = xp->list;
			freeblist(xp);
			xp = next;
		}
	}
	else { /* queue icmp unreachable for rxmitproc later on, w/o arp lock */
		if(xp){
			if(arp->dropl == nil)
				arp->dropf = xp;
			else
				arp->dropl->list = xp;

			for(next = xp->list; next; next = next->list)
				xp = next;
			arp->dropl = xp;
			wakeup(&arp->rxmtq);
		}
	}

	/* take out of current chain */
	l = &arp->hash[haship(a->ip)];
	for(f = *l; f; f = f->hash){
		if(f == a){
			*l = a->hash;
			break;
		}
		l = &f->hash;
	}

	/* insert into new chain */
	l = &arp->hash[haship(ip)];
	a->hash = *l;
	*l = a;

	memmove(a->ip, ip, sizeof(a->ip));
	a->utime = NOW;
	a->ctime = 0;
	a->type = m;

	a->rtime = NOW + ReTransTimer;
	a->rxtsrem = MAX_MULTICAST_SOLICIT;
	a->ifc = ifc;
	a->ifcid = ifc->ifcid;

	/* put to the end of re-transmit chain; addrxt is 0 when isv4(a->ip) */
	if(!ipismulticast(a->ip) && addrxt){
		l = &arp->rxmt;
		empty = (*l==nil);

		for(f = *l; f; f = f->nextrxt){
			if(f == a){
				*l = a->nextrxt;
				break;
			}
			l = &f->nextrxt;
		}
		for(f = *l; f; f = f->nextrxt){
			l = &f->nextrxt;
		}
		*l = a;
		if(empty)
			wakeup(&arp->rxmtq);
	}

	a->nextrxt = nil;

	return a;
}

/* called with arp qlocked */

void
cleanarpent(Arp *arp, Arpent *a)
{
	Arpent *f, **l;

	a->utime = 0;
	a->ctime = 0;
	a->type = 0;
	a->state = 0;

	/* take out of current chain */
	l = &arp->hash[haship(a->ip)];
	for(f = *l; f; f = f->hash){
		if(f == a){
			*l = a->hash;
			break;
		}
		l = &f->hash;
	}

	/* take out of re-transmit chain */
	l = &arp->rxmt;
	for(f = *l; f; f = f->nextrxt){
		if(f == a){
			*l = a->nextrxt;
			break;
		}
		l = &f->nextrxt;
	}
	a->nextrxt = nil;
	a->hash = nil;
	a->hold = nil;
	a->last = nil;
	a->ifc = nil;
}

/*
 *  fill in the media address if we have it.  Otherwise return an
 *  Arpent that represents the state of the address resolution FSM
 *  for ip.  Add the packet to be sent onto the list of packets
 *  waiting for ip->mac to be resolved.
 */
Arpent*
arpget(Arp *arp, Block *bp, int version, Ipifc *ifc, uchar *ip, uchar *mac)
{
	int hash;
	Arpent *a;
	Medium *type = ifc->m;
	uchar v6ip[IPaddrlen];

	if(version == V4){
		v4tov6(v6ip, ip);
		ip = v6ip;
	}

	qlock(arp);
	hash = haship(ip);
	for(a = arp->hash[hash]; a; a = a->hash){
		if(memcmp(ip, a->ip, sizeof(a->ip)) == 0)
		if(type == a->type)
			break;
	}

	if(a == nil){
		a = newarp6(arp, ip, ifc, (version != V4));
		a->state = AWAIT;
	}
	a->utime = NOW;
	if(a->state == AWAIT){
		if(bp != nil){
			if(a->hold)
				a->last->list = bp;
			else
				a->hold = bp;
			a->last = bp;
			bp->list = nil;
		}
		return a;		/* return with arp qlocked */
	}

	memmove(mac, a->mac, a->type->maclen);

	/* remove old entries */
	if(NOW - a->ctime > 15*60*1000)
		cleanarpent(arp, a);

	qunlock(arp);
	return nil;
}

/*
 * called with arp locked
 */
void
arprelease(Arp *arp, Arpent*)
{
	qunlock(arp);
}

/*
 * Copy out the mac address from the Arpent.  Return the
 * block waiting to get sent to this mac address.
 *
 * called with arp locked
 */
Block*
arpresolve(Arp *arp, Arpent *a, Medium *type, uchar *mac)
{
	Block *bp;
	Arpent *f, **l;

	if(!isv4(a->ip)){
		l = &arp->rxmt;
		for(f = *l; f; f = f->nextrxt){
			if(f == a){
				*l = a->nextrxt;
				break;
			}
			l = &f->nextrxt;
		}
	}

	memmove(a->mac, mac, type->maclen);
	a->type = type;
	a->state = AOK;
	a->utime = NOW;
	bp = a->hold;
	a->hold = nil;
	qunlock(arp);

	return bp;
}

void
arpenter(Fs *fs, int version, uchar *ip, uchar *mac, int n, int refresh)
{
	Arp *arp;
	Route *r;
	Arpent *a, *f, **l;
	Ipifc *ifc;
	Medium *type;
	Block *bp, *next;
	uchar v6ip[IPaddrlen];

	arp = fs->arp;

	if(n != 6){
//		print("arp: len = %d\n", n);
		return;
	}

	switch(version){
	case V4:
		r = v4lookup(fs, ip, nil);
		v4tov6(v6ip, ip);
		ip = v6ip;
		break;
	case V6:
		r = v6lookup(fs, ip, nil);
		break;
	default:
		panic("arpenter: version %d", version);
		return;	/* to supress warnings */
	}

	if(r == nil){
//		print("arp: no route for entry\n");
		return;
	}

	ifc = r->ifc;
	type = ifc->m;

	qlock(arp);
	for(a = arp->hash[haship(ip)]; a; a = a->hash){
		if(a->type != type || (a->state != AWAIT && a->state != AOK))
			continue;

		if(ipcmp(a->ip, ip) == 0){
			a->state = AOK;
			memmove(a->mac, mac, type->maclen);

			if(version == V6){
				/* take out of re-transmit chain */
				l = &arp->rxmt;
				for(f = *l; f; f = f->nextrxt){
					if(f == a){
						*l = a->nextrxt;
						break;
					}
					l = &f->nextrxt;
				}
			}

			a->ifc = ifc;
			a->ifcid = ifc->ifcid;
			bp = a->hold;
			a->hold = nil;
			if(version == V4)
				ip += IPv4off;
			a->utime = NOW;
			a->ctime = a->utime;
			qunlock(arp);

			while(bp){
				next = bp->list;
				if(ifc != nil){
					if(waserror()){
						runlock(ifc);
						nexterror();
					}
					rlock(ifc);
					if(ifc->m != nil)
						ifc->m->bwrite(ifc, bp, version, ip);
					else
						freeb(bp);
					runlock(ifc);
					poperror();
				} else
					freeb(bp);
				bp = next;
			}
			return;
		}
	}

	if(refresh == 0){
		a = newarp6(arp, ip, ifc, 0);
		a->state = AOK;
		a->type = type;
		a->ctime = NOW;
		memmove(a->mac, mac, type->maclen);
	}

	qunlock(arp);
}

int
arpwrite(Fs *fs, char *s, int len)
{
	int n;
	Route *r;
	Arp *arp;
	Block *bp;
	Arpent *a, *fl, **l;
	Medium *m;
	char *f[4], buf[256];
	uchar ip[IPaddrlen], mac[MAClen];

	arp = fs->arp;

	if(len == 0)
		error(Ebadarp);
	if(len >= sizeof(buf))
		len = sizeof(buf)-1;
	strncpy(buf, s, len);
	buf[len] = 0;
	if(len > 0 && buf[len-1] == '\n')
		buf[len-1] = 0;

	n = getfields(buf, f, 4, 1, " ");
	if(strcmp(f[0], "flush") == 0){
		qlock(arp);
		for(a = arp->cache; a < &arp->cache[NCACHE]; a++){
			memset(a->ip, 0, sizeof(a->ip));
			memset(a->mac, 0, sizeof(a->mac));
			a->hash = nil;
			a->state = 0;
			a->utime = 0;
			while(a->hold != nil){
				bp = a->hold->list;
				freeblist(a->hold);
				a->hold = bp;
			}
		}
		memset(arp->hash, 0, sizeof(arp->hash));
		/* clear all pkts on these lists (rxmt, dropf/l) */
		arp->rxmt = nil;
		arp->dropf = nil;
		arp->dropl = nil;
		qunlock(arp);
	} else if(strcmp(f[0], "add") == 0){
		switch(n){
		default:
			error(Ebadarg);
		case 3:
			if (parseip(ip, f[1]) == -1)
				error(Ebadip);
			if(isv4(ip))
				r = v4lookup(fs, ip+IPv4off, nil);
			else
				r = v6lookup(fs, ip, nil);
			if(r == nil)
				error("Destination unreachable");
			m = r->ifc->m;
			n = parsemac(mac, f[2], m->maclen);
			break;
		case 4:
			m = ipfindmedium(f[1]);
			if(m == nil)
				error(Ebadarp);
			if (parseip(ip, f[2]) == -1)
				error(Ebadip);
			n = parsemac(mac, f[3], m->maclen);
			break;
		}

		if(m->ares == nil)
			error(Ebadarp);

		m->ares(fs, V6, ip, mac, n, 0);
	} else if(strcmp(f[0], "del") == 0){
		if(n != 2)
			error(Ebadarg);

		if (parseip(ip, f[1]) == -1)
			error(Ebadip);
		qlock(arp);

		l = &arp->hash[haship(ip)];
		for(a = *l; a; a = a->hash){
			if(memcmp(ip, a->ip, sizeof(a->ip)) == 0){
				*l = a->hash;
				break;
			}
			l = &a->hash;
		}

		if(a){
			/* take out of re-transmit chain */
			l = &arp->rxmt;
			for(fl = *l; fl; fl = fl->nextrxt){
				if(fl == a){
					*l = a->nextrxt;
					break;
				}
				l = &fl->nextrxt;
			}

			a->nextrxt = nil;
			a->hash = nil;
			a->hold = nil;
			a->last = nil;
			a->ifc = nil;
			memset(a->ip, 0, sizeof(a->ip));
			memset(a->mac, 0, sizeof(a->mac));
		}
		qunlock(arp);
	} else
		error(Ebadarp);

	return len;
}

enum
{
	Alinelen=	90,
};

char *aformat = "%-6.6s %-8.8s %-40.40I %-32.32s\n";

static void
convmac(char *p, char *ep, uchar *mac, int n)
{
	while(n-- > 0)
		p = seprint(p, ep, "%2.2ux", *mac++);
}

int
arpread(Arp *arp, char *p, ulong offset, int len)
{
	Arpent *a;
	int n;
	char mac[2*MAClen+1];

	if(offset % Alinelen)
		return 0;

	offset = offset/Alinelen;
	len = len/Alinelen;

	n = 0;
	for(a = arp->cache; len > 0 && a < &arp->cache[NCACHE]; a++){
		if(a->state == 0)
			continue;
		if(offset > 0){
			offset--;
			continue;
		}
		len--;
		qlock(arp);
		convmac(mac, &mac[sizeof mac], a->mac, a->type->maclen);
		n += snprint(p+n, Alinelen+1, aformat, a->type->name,
			arpstate[a->state], a->ip, mac);	/* +1 for NUL */
		qunlock(arp);
	}

	return n;
}

extern int
rxmitsols(Arp *arp)
{
	uint sflag;
	Block *next, *xp;
	Arpent *a, *b, **l;
	Fs *f;
	uchar ipsrc[IPaddrlen];
	Ipifc *ifc = nil;
	long nrxt;

	qlock(arp);
	f = arp->f;

	a = arp->rxmt;
	if(a==nil){
		nrxt = 0;
		goto dodrops; 		/* return nrxt; */
	}
	nrxt = a->rtime - NOW;
	if(nrxt > 3*ReTransTimer/4)
		goto dodrops; 		/* return nrxt; */

	for(; a; a = a->nextrxt){
		ifc = a->ifc;
		assert(ifc != nil);
		if((a->rxtsrem <= 0) || !(canrlock(ifc)) || (a->ifcid != ifc->ifcid)){
			xp = a->hold;
			a->hold = nil;

			if(xp){
				if(arp->dropl == nil)
					arp->dropf = xp;
				else
					arp->dropl->list = xp;
			}

			cleanarpent(arp, a);
		}
		else
			break;
	}
	if(a == nil)
		goto dodrops;


	qunlock(arp);	/* for icmpns */
	if((sflag = ipv6anylocal(ifc, ipsrc)) != SRC_UNSPEC)
		icmpns(f, ipsrc, sflag, a->ip, TARG_MULTI, ifc->mac);

	runlock(ifc);
	qlock(arp);

	/* put to the end of re-transmit chain */
	l = &arp->rxmt;
	for(b = *l; b; b = b->nextrxt){
		if(b == a){
			*l = a->nextrxt;
			break;
		}
		l = &b->nextrxt;
	}
	for(b = *l; b; b = b->nextrxt){
		l = &b->nextrxt;
	}
	*l = a;
	a->rxtsrem--;
	a->nextrxt = nil;
	a->rtime = NOW + ReTransTimer;

	a = arp->rxmt;
	if(a==nil)
		nrxt = 0;
	else
		nrxt = a->rtime - NOW;

dodrops:
	xp = arp->dropf;
	arp->dropf = nil;
	arp->dropl = nil;
	qunlock(arp);

	for(; xp; xp = next){
		next = xp->list;
		icmphostunr(f, ifc, xp, Icmp6_adr_unreach, 1);
	}

	return nrxt;

}

static int
rxready(void *v)
{
	Arp *arp = (Arp *) v;
	int x;

	x = ((arp->rxmt != nil) || (arp->dropf != nil));

	return x;
}

static void
rxmitproc(void *v)
{
	Arp *arp = v;
	long wakeupat;

	arp->rxmitp = up;
	//print("arp rxmitproc started\n");
	if(waserror()){
		arp->rxmitp = 0;
		pexit("hangup", 1);
	}
	for(;;){
		wakeupat = rxmitsols(arp);
		if(wakeupat == 0)
			sleep(&arp->rxmtq, rxready, v);
		else if(wakeupat > ReTransTimer/4)
			tsleep(&arp->rxmtq, return0, 0, wakeupat);
	}
}


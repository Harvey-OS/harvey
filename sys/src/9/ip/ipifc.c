#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#include "ip.h"

#define DPRINT if(0)print

enum {
	Maxmedia	= 32,
	Nself		= Maxmedia*5,
	NHASH		= (1<<6),
	NCACHE		= 256,
	QMAX		= 64*1024-1,
};

Medium *media[Maxmedia] =
{
	0
};

/*
 *  cache of local addresses (addresses we answer to)
 */
typedef struct Ipself Ipself;
struct Ipself
{
	uchar	a[IPaddrlen];
	Ipself	*hnext;		/* next address in the hash table */
	Iplink	*link;		/* binding twixt Ipself and Ipifc */
	ulong	expire;
	uchar	type;		/* type of address */
	int	ref;
	Ipself	*next;		/* free list */
};

struct Ipselftab
{
	QLock;
	int	inited;
	int	acceptall;	/* true if an interface has the null address */
	Ipself	*hash[NHASH];	/* hash chains */
};

/*
 *  Multicast addresses are chained onto a Chan so that
 *  we can remove them when the Chan is closed.
 */
typedef struct Ipmcast Ipmcast;
struct Ipmcast
{
	Ipmcast	*next;
	uchar	ma[IPaddrlen];	/* multicast address */
	uchar	ia[IPaddrlen];	/* interface address */
};

/* quick hash for ip addresses */
#define hashipa(a) ( ( ((a)[IPaddrlen-2]<<8) | (a)[IPaddrlen-1] )%NHASH )

static char tifc[] = "ifc ";

static void	addselfcache(Fs *f, Ipifc *ifc, Iplifc *lifc, uchar *a, int type);
static void	remselfcache(Fs *f, Ipifc *ifc, Iplifc *lifc, uchar *a);
static char*	ipifcjoinmulti(Ipifc *ifc, char **argv, int argc);
static char*	ipifcleavemulti(Ipifc *ifc, char **argv, int argc);
static void	ipifcregisterproxy(Fs*, Ipifc*, uchar*);

/*
 *  link in a new medium
 */
void
addipmedium(Medium *med)
{
	int i;

	for(i = 0; i < nelem(media)-1; i++)
		if(media[i] == nil){
			media[i] = med;
			break;
		}
}

/*
 *  find the medium with this name
 */
Medium*
ipfindmedium(char *name)
{
	Medium **mp;

	for(mp = media; *mp != nil; mp++)
		if(strcmp((*mp)->name, name) == 0)
			break;
	return *mp;
}

/*
 *  attach a device (or pkt driver) to the interface.
 *  called with c locked
 */
static char*
ipifcbind(Conv *c, char **argv, int argc)
{
	Ipifc *ifc;
	Medium *m;

	if(argc < 2)
		return Ebadarg;

	ifc = (Ipifc*)c->ptcl;

	/* bind the device to the interface */
	m = ipfindmedium(argv[1]);
	if(m == nil)
		return "unknown interface type";

	wlock(ifc);
	if(ifc->m != nil){
		wunlock(ifc);
		return "interface already bound";	
	}
	if(waserror()){
		wunlock(ifc);
		nexterror();
	}

	(*m->bind)(ifc, argc, argv);
	if(argc > 2)
		strncpy(ifc->dev, argv[2], sizeof(ifc->dev));
	else
		sprint(ifc->dev, "%s%d", m->name, c->x);
	ifc->dev[sizeof(ifc->dev)-1] = 0;
	ifc->m = m;
	ifc->minmtu = ifc->m->minmtu;
	ifc->maxmtu = ifc->m->maxmtu;
	if(ifc->m->unbindonclose == 0)
		ifc->conv->inuse++;
	ifc->ifcid++;

	wunlock(ifc);
	poperror();

	return nil;
}

/*
 *  detach a device from an interface, close the interface
 *  called with ifc->conv closed
 */
static char*
ipifcunbind(Ipifc *ifc)
{
	char *av[4];
	char ip[32];
	char mask[32];

	if(waserror()){
		wunlock(ifc);
		nexterror();
	}
	wlock(ifc);

	/* dissociate routes */
	if(ifc->m != nil && ifc->m->unbindonclose == 0)
		ifc->conv->inuse--;
	ifc->ifcid++;

	/* disassociate device */
	if(ifc->m != nil && ifc->m->unbind)
		(*ifc->m->unbind)(ifc);
	memset(ifc->dev, 0, sizeof(ifc->dev));
	ifc->arg = nil;
	ifc->reassemble = 0;

	/* hangup queues to stop queuing of packets */
	qhangup(ifc->conv->rq, "unbind");
	qhangup(ifc->conv->wq, "unbind");

	/* disassociate logical interfaces */
	av[0] = "remove";
	av[1] = ip;
	av[2] = mask;
	av[3] = 0;
	while(ifc->lifc){
		if(ipcmp(ifc->lifc->mask, IPallbits) == 0)
			sprint(ip, "%I", ifc->lifc->remote);
		else
			sprint(ip, "%I", ifc->lifc->local);
		sprint(mask, "%M", ifc->lifc->mask);
		ipifcrem(ifc, av, 3, 0);
	}

	ifc->m = nil;
	wunlock(ifc);
	poperror();
	return nil;
}

static int
ipifcstate(Conv *c, char *state, int n)
{
	Ipifc *ifc;
	Iplifc *lifc;
	int m;

	ifc = (Ipifc*)c->ptcl;

	m = snprint(state, n, "%-12s %-5d", ifc->dev, ifc->maxmtu);

	rlock(ifc);
	for(lifc = ifc->lifc; lifc && n > m; lifc = lifc->next)
		m += snprint(state+m, n - m,
			" %-20.20I %-20.20M %-20.20I %-7lud %-7lud %-7lud %-7lud\n",
				lifc->local, lifc->mask, lifc->remote,
				ifc->in, ifc->out, ifc->inerr, ifc->outerr);
	if(ifc->lifc == nil)
		m += snprint(state+m, n - m, "\n");
	runlock(ifc);
	return m;
}

static int
ipifclocal(Conv *c, char *state, int n)
{
	Ipifc *ifc;
	Iplifc *lifc;
	Iplink *link;
	int m;

	ifc = (Ipifc*)c->ptcl;

	m = 0;

	rlock(ifc);
	for(lifc = ifc->lifc; lifc; lifc = lifc->next){
		m += snprint(state+m, n - m, "%-20.20I ->", lifc->local);
		for(link = lifc->link; link; link = link->lifclink)
			m += snprint(state+m, n - m, " %-20.20I", link->self->a);
		m += snprint(state+m, n - m, "\n");
	}
	runlock(ifc);
	return m;
}

static int
ipifcinuse(Conv *c)
{
	Ipifc *ifc;

	ifc = (Ipifc*)c->ptcl;
	return ifc->m != nil;
}

/*
 *  called when a process writes to an interface's 'data'
 */
static void
ipifckick(Conv *c)
{
	Block *bp;
	Ipifc *ifc;

	bp = qget(c->wq);
	if(bp == nil)
		return;

	ifc = (Ipifc*)c->ptcl;
	if(ifc->m == nil || ifc->m->pktin == nil)
		freeb(bp);
	else
		(*ifc->m->pktin)(c->p->f, ifc, bp);
}

/*
 *  we'll have to have a kick routine at
 *  some point to deal with these
 */
static void
ipifccreate(Conv *c)
{
	Ipifc *ifc;

	c->rq = qopen(QMAX, 0, 0, 0);
	c->wq = qopen(QMAX, 0, 0, 0);
	ifc = (Ipifc*)c->ptcl;
	ifc->conv = c;
	ifc->unbinding = 0;
	ifc->m = nil;
	ifc->reassemble = 0;
}

/* 
 *  called after last close of ipifc data or ctl
 *  called with c locked, we must unlock
 */
static void
ipifcclose(Conv *c)
{
	Ipifc *ifc;
	Medium *m;

	ifc = (Ipifc*)c->ptcl;
	m = ifc->m;
	if(m != nil && m->unbindonclose)
		ipifcunbind(ifc);
}

/*
 *  change an interface's mtu
 */
char*
ipifcsetmtu(Ipifc *ifc, char **argv, int argc)
{
	int mtu;

	if(argc < 2)
		return Ebadarg;
	if(ifc->m == nil)
		return Ebadarg;
	mtu = strtoul(argv[1], 0, 0);
	if(mtu < ifc->m->minmtu || mtu > ifc->m->maxmtu)
		return Ebadarg;
	ifc->maxmtu = mtu;
	return nil;
}

/*
 *  add an address to an interface.
 */
char*
ipifcadd(Ipifc *ifc, char **argv, int argc)
{
	uchar ip[IPaddrlen], mask[IPaddrlen], rem[IPaddrlen];
	uchar bcast[IPaddrlen], net[IPaddrlen];
	Iplifc *lifc, **l;
	int i, type, mtu;
	Fs *f;

	if(ifc->m == nil)
		return "ipifc not yet bound to device";

	f = ifc->conv->p->f;

	type = Rifc;
	memset(ip, 0, IPaddrlen);
	memset(mask, 0, IPaddrlen);
	memset(rem, 0, IPaddrlen);
	switch(argc){
	case 6:
		if(strcmp(argv[5], "proxy") == 0)
			type |= Rproxy;
		/* fall through */
	case 5:
		mtu = strtoul(argv[4], 0, 0);
		if(mtu >= ifc->m->minmtu && mtu <= ifc->m->maxmtu)
			ifc->maxmtu = mtu;
		/* fall through */
	case 4:
		parseip(ip, argv[1]);
		parseipmask(mask, argv[2]);
		parseip(rem, argv[3]);
		maskip(rem, mask, net);
		break;
	case 3:
		parseip(ip, argv[1]);
		parseipmask(mask, argv[2]);
		maskip(ip, mask, rem);
		maskip(rem, mask, net);
		break;
	case 2:
		parseip(ip, argv[1]);
		memmove(mask, defmask(ip), IPaddrlen);
		maskip(ip, mask, rem);
		maskip(rem, mask, net);
		break;
	default:
		return Ebadarg;
		break;
	}

	wlock(ifc);

	/* ignore if this is already a local address for this ifc */
	for(lifc = ifc->lifc; lifc; lifc = lifc->next)
		if(ipcmp(lifc->local, ip) == 0)
			goto out;

	/* add the address to the list of logical ifc's for this ifc */
	lifc = smalloc(sizeof(Iplifc));
	ipmove(lifc->local, ip);
	ipmove(lifc->mask, mask);
	ipmove(lifc->remote, rem);
	ipmove(lifc->net, net);
	lifc->next = nil;
	for(l = &ifc->lifc; *l; l = &(*l)->next)
		;
	*l = lifc;

	/* add a route for the local network */
	if(ipcmp(mask, IPallbits) == 0){
		/* point to point networks are a hack */
		if(ipcmp(ip, rem) == 0)
			findprimaryip(f, lifc->local);
		type |= Rptpt;
	}
	if(isv4(ip))
		v4addroute(f, tifc, rem+IPv4off, mask+IPv4off, rem+IPv4off, type);
	else
		v6addroute(f, tifc, rem, mask, rem, type);

	addselfcache(f, ifc, lifc, ip, Runi);

	if((type & (Rptpt|Rproxy)) == (Rptpt|Rproxy)){
		ipifcregisterproxy(f, ifc, rem);
		goto out;
	}

	/* add subnet directed broadcast addresses to the self cache */
	for(i = 0; i < IPaddrlen; i++)
		bcast[i] = (ip[i] & mask[i]) | ~mask[i];
	addselfcache(f, ifc, lifc, bcast, Rbcast);

	/* add old subnet directed broadcast addresses to the self cache */
	for(i = 0; i < IPaddrlen; i++)
		bcast[i] = (ip[i] & mask[i]) & mask[i];
	addselfcache(f, ifc, lifc, bcast, Rbcast);

	/* add network directed broadcast addresses to the self cache */
	memmove(mask, defmask(ip), IPaddrlen);
	for(i = 0; i < IPaddrlen; i++)
		bcast[i] = (ip[i] & mask[i]) | ~mask[i];
	addselfcache(f, ifc, lifc, bcast, Rbcast);

	/* add old network directed broadcast addresses to the self cache */
	memmove(mask, defmask(ip), IPaddrlen);
	for(i = 0; i < IPaddrlen; i++)
		bcast[i] = (ip[i] & mask[i]) & mask[i];
	addselfcache(f, ifc, lifc, bcast, Rbcast);

	addselfcache(f, ifc, lifc, IPv4bcast, Rbcast);

	/* register the address on this network for address resolution */
	if(ifc->m->areg != nil)
		(*ifc->m->areg)(ifc, ip);

out:
	wunlock(ifc);
	return nil;
}

/*
 *  remove an address from an interface.
 *  called with c->car locked
 */
char*
ipifcrem(Ipifc *ifc, char **argv, int argc, int dolock)
{
	uchar ip[IPaddrlen];
	uchar mask[IPaddrlen];
	Iplifc *lifc, **l;
	Fs *f;
	uchar *addr;
	int type;

	if(argc < 3)
		return Ebadarg;

	f = ifc->conv->p->f;

	parseip(ip, argv[1]);
	parseipmask(mask, argv[2]);

	if(dolock)
		wlock(ifc);

	/* Are we point to point */
	type = 0;
	if(ipcmp(mask, IPallbits) == 0)
		type = Rptpt;

	/*
	 *  find address on this interface and remove from chain.
	 *  for pt to pt we actually specify the remote address at the
	 *  addresss to remove.
	 */
	l = &ifc->lifc;
	for(lifc = *l; lifc != nil; lifc = lifc->next) {
		addr = lifc->local;
		if(type == Rptpt)
			addr = lifc->remote;
		if(memcmp(ip, addr, IPaddrlen) == 0)
		if(memcmp(mask, lifc->mask, IPaddrlen) == 0) {
			*l = lifc->next;
			break;
		}
		l = &lifc->next;
	}

	if(lifc == nil){
		if(dolock)
			wunlock(ifc);
		return "address not on this interface";
	}

	ifc->ifcid++;

	/* disassociate any addresses */
	while(lifc->link)
		remselfcache(f, ifc, lifc, lifc->link->self->a);

	/* remove the route for this logical interface */
	if(isv4(ip))
		v4delroute(f, lifc->remote+IPv4off, lifc->mask+IPv4off, 1);
	else
		v6delroute(f, lifc->remote, lifc->mask, 1);

	free(lifc);

	if(dolock)
		wunlock(ifc);
	return nil;
}

/*
 * distrbute routes to active interfaces like the
 * TRIP linecards
 */
void
ipifcaddroute(Fs *f, int vers, uchar *addr, uchar *mask, uchar *gate, int type)
{
	Medium *m;
	Conv **cp, **e;
	Ipifc *ifc;

	e = &f->ipifc->conv[f->ipifc->nc];
	for(cp = f->ipifc->conv; cp < e; cp++){
		if(*cp != nil) {
			ifc = (Ipifc*)(*cp)->ptcl;
			m = ifc->m;
			if(m == nil)
				continue;
			if(m->addroute != nil)
				m->addroute(ifc, vers, addr, mask, gate, type);
		}
	}
}

void
ipifcremroute(Fs *f, int vers, uchar *addr, uchar *mask)
{
	Medium *m;
	Conv **cp, **e;
	Ipifc *ifc;

	e = &f->ipifc->conv[f->ipifc->nc];
	for(cp = f->ipifc->conv; cp < e; cp++){
		if(*cp != nil) {
			ifc = (Ipifc*)(*cp)->ptcl;
			m = ifc->m;
			if(m == nil)
				continue;
			if(m->remroute != nil)
				m->remroute(ifc, vers, addr, mask);
		}
	}
}

/*
 *  associate an address with the interface.  This wipes out any previous
 *  addresses.  This is a macro that means, remove all the old interfaces
 *  and add a new one.
 */
static char*
ipifcconnect(Conv* c, char **argv, int argc)
{
	char *err;
	Ipifc *ifc;
	char *av[4];
	char ip[80], mask[80];

	ifc = (Ipifc*)c->ptcl;

	if(ifc->m == nil)
		 return "ipifc not yet bound to device";

	av[0] = "remove";
	av[1] = ip;
	av[2] = mask;
	av[3] = 0;
	if(waserror()){
		wunlock(ifc);
		nexterror();
	}
	wlock(ifc);
	while(ifc->lifc){
		if(ipcmp(ifc->lifc->mask, IPallbits) == 0)
			sprint(ip, "%I", ifc->lifc->remote);
		else
			sprint(ip, "%I", ifc->lifc->local);
		sprint(mask, "%M", ifc->lifc->mask);
		ipifcrem(ifc, av, 3, 0);
	}
	wunlock(ifc);
	poperror();

	err = ipifcadd(ifc, argv, argc);
	if(err)
		return err;

	Fsconnected(c, nil);

	return nil;
}

/*
 *  non-standard control messages.
 *  called with c->car locked.
 */
static char*
ipifcctl(Conv* c, char**argv, int argc)
{
	Ipifc *ifc;
	int i;

	ifc = (Ipifc*)c->ptcl;
	if(strcmp(argv[0], "add") == 0)
		return ipifcadd(ifc, argv, argc);
	else if(strcmp(argv[0], "remove") == 0)
		return ipifcrem(ifc, argv, argc, 1);
	else if(strcmp(argv[0], "unbind") == 0)
		return ipifcunbind(ifc);
	else if(strcmp(argv[0], "joinmulti") == 0)
		return ipifcjoinmulti(ifc, argv, argc);
	else if(strcmp(argv[0], "leavemulti") == 0)
		return ipifcleavemulti(ifc, argv, argc);
	else if(strcmp(argv[0], "mtu") == 0)
		return ipifcsetmtu(ifc, argv, argc);
	else if(strcmp(argv[0], "reassemble") == 0){
		ifc->reassemble = 1;
		return nil;
	} else if(strcmp(argv[0], "iprouting") == 0){
		i = 1;
		if(argc > 1)
			i = atoi(argv[1]);
		iprouting(c->p->f, i);
		return nil;
	}
	return "unsupported ctl";
}

ipifcstats(Proto *ipifc, char *buf, int len)
{
	return ipstats(ipifc->f, buf, len);
}

void
ipifcinit(Fs *f)
{
	Proto *ipifc;

	ipifc = smalloc(sizeof(Proto));
	ipifc->name = "ipifc";
	ipifc->kick = ipifckick;
	ipifc->connect = ipifcconnect;
	ipifc->announce = nil;
	ipifc->bind = ipifcbind;
	ipifc->state = ipifcstate;
	ipifc->create = ipifccreate;
	ipifc->close = ipifcclose;
	ipifc->rcv = nil;
	ipifc->ctl = ipifcctl;
	ipifc->advise = nil;
	ipifc->stats = ipifcstats;
	ipifc->inuse = ipifcinuse;
	ipifc->local = ipifclocal;
	ipifc->ipproto = -1;
	ipifc->nc = Maxmedia;
	ipifc->ptclsize = sizeof(Ipifc);

	f->ipifc = ipifc;			/* hack for ipifcremroute, findipifc, ... */
	f->self = smalloc(sizeof(Ipselftab));	/* hack for ipforme */

	Fsproto(f, ipifc);
}

/*
 *  add to self routing cache
 *	called with c->car locked
 */
static void
addselfcache(Fs *f, Ipifc *ifc, Iplifc *lifc, uchar *a, int type)
{
	Ipself *p;
	Iplink *lp;
	int h;

	qlock(f->self);

	/* see if the address already exists */
	h = hashipa(a);
	for(p = f->self->hash[h]; p; p = p->next)
		if(memcmp(a, p->a, IPaddrlen) == 0)
			break;

	/* allocate a local address and add to hash chain */
	if(p == nil){
		p = smalloc(sizeof(*p));
		ipmove(p->a, a);
		p->type = type;
		p->next = f->self->hash[h];
		f->self->hash[h] = p;

		/* if the null address, accept all packets */
		if(ipcmp(a, v4prefix) == 0 || ipcmp(a, IPnoaddr) == 0)
			f->self->acceptall = 1;
	}

	/* look for a link for this lifc */
	for(lp = p->link; lp; lp = lp->selflink)
		if(lp->lifc == lifc)
			break;

	/* allocate a lifc-to-local link and link to both */
	if(lp == nil){
		lp = smalloc(sizeof(*lp));
		lp->ref = 1;
		lp->lifc = lifc;
		lp->self = p;
		lp->selflink = p->link;
		p->link = lp;
		lp->lifclink = lifc->link;
		lifc->link = lp;

		/* add to routing table */
		if(isv4(a))
			v4addroute(f, tifc, a+IPv4off, IPallbits+IPv4off, a+IPv4off, type);
		else
			v6addroute(f, tifc, a, IPallbits, a, type);

		if((type & Rmulti) && ifc->m->addmulti != nil)
			(*ifc->m->addmulti)(ifc, a, lifc->local);
	} else {
		lp->ref++;
	}

	qunlock(f->self);
}

/*
 *  These structures are unlinked from their chains while
 *  other threads may be using them.  To avoid excessive locking,
 *  just put them aside for a while before freeing them.
 *	called with f->self locked
 */
static Iplink *freeiplink;
static Ipself *freeipself;

static void
iplinkfree(Iplink *p)
{
	Iplink **l, *np;
	ulong now = msec;

	l = &freeiplink;
	for(np = *l; np; np = *l){
		if(np->expire > now){
			*l = np->next;
			free(np);
			continue;
		}
		l = &np->next;
	}
	p->expire = now + 5000;		/* give other threads 5 secs to get out */
	p->next = nil;
	*l = p;
}
static void
ipselffree(Ipself *p)
{
	Ipself **l, *np;
	ulong now = msec;

	l = &freeipself;
	for(np = *l; np; np = *l){
		if(np->expire > now){
			*l = np->next;
			free(np);
			continue;
		}
		l = &np->next;
	}
	p->expire = now + 5000;		/* give other threads 5 secs to get out */
	p->next = nil;
	*l = p;
}

/*
 *  Decrement reference for this address on this link.
 *  Unlink from selftab if this is the last ref.
 *	called with c->car locked
 */
static void
remselfcache(Fs *f, Ipifc *ifc, Iplifc *lifc, uchar *a)
{
	Ipself *p, **l;
	Iplink *link, **l_self, **l_lifc;

	qlock(f->self);

	/* find the unique selftab entry */
	l = &f->self->hash[hashipa(a)];
	for(p = *l; p; p = *l){
		if(ipcmp(p->a, a) == 0)
			break;
		l = &p->next;
	}

	if(p == nil)
		goto out;

	/*
	 *  walk down links from an ifc looking for one
	 *  that matches the selftab entry
	 */
	l_lifc = &lifc->link;
	for(link = *l_lifc; link; link = *l_lifc){
		if(link->self == p)
			break;
		l_lifc = &link->lifclink;
	}

	if(link == nil)
		goto out;

	/*
	 *  walk down the links from the selftab looking for
	 *  the one we just found
	 */
	l_self = &p->link;
	for(link = *l_self; link; link = *l_self){
		if(link == *(l_lifc))
			break;
		l_self = &link->selflink;
	}

	if(link == nil)
		panic("remselfcache");

	if(--(link->ref) != 0)
		goto out;

	if((p->type & Rmulti) && ifc->m->remmulti != nil)
		(*ifc->m->remmulti)(ifc, a, lifc->local);

	/* ref == 0, remove from both chains and free the link */
	*l_lifc = link->lifclink;
	*l_self = link->selflink;
	iplinkfree(link);

	if(p->link != nil)
		goto out;

	/* remove from routing table */
	if(isv4(a))
		v4delroute(f, a+IPv4off, IPallbits+IPv4off, 1);
	else
		v6delroute(f, a, IPallbits, 1);
	
	/* no more links, remove from hash and free */
	*l = p->next;
	ipselffree(p);

	/* if IPnoaddr, forget */
	if(ipcmp(a, v4prefix) == 0 || ipcmp(a, IPnoaddr) == 0)
		f->self->acceptall = 0;

out:
	qunlock(f->self);
}

static char *stformat = "%-32.32I %2.2d %4.4s\n";

long
ipselftabread(Fs *f, char *cp, ulong offset, int n)
{
	int i, m, nifc, off;
	Ipself *p;
	Iplink *link;
	char state[8];

	m = 0;
	off = offset;
	qlock(f->self);
	for(i = 0; i < NHASH && m < n; i++){
		for(p = f->self->hash[i]; p != nil && m < n; p = p->next){
			nifc = 0;
			for(link = p->link; link; link = link->selflink)
				nifc++;
			routetype(p->type, state);
			m += snprint(cp + m, n - m, stformat, p->a, nifc, state);
			if(off > 0){
				off -= m;
				m = 0;
			}
		}
	}
	qunlock(f->self);
	return m;
}


/*
 *  returns
 *	0		- no match
 *	Runi
 *	Rbcast
 *	Rmcast
 */
int
ipforme(Fs *f, uchar *addr)
{
	Ipself *p;

	p = f->self->hash[hashipa(addr)];
	for(; p; p = p->next){
		if(ipcmp(addr, p->a) == 0)
			return p->type;
	}

	/* hack to say accept anything */
	if(f->self->acceptall)
		return Runi;

	return 0;
}

/*
 *  find the ifc on same net as the remote system.  If none,
 *  return nil.
 */
Ipifc*
findipifc(Fs *f, uchar *remote, int type)
{
	Ipifc *ifc, *x;
	Iplifc *lifc;
	Conv **cp, **e;
	uchar gnet[IPaddrlen];
	uchar xmask[IPaddrlen];

	x = nil;

	/* find most specific match */
	e = &f->ipifc->conv[f->ipifc->nc];
	for(cp = f->ipifc->conv; cp < e; cp++){
		if(*cp == 0)
			continue;
		ifc = (Ipifc*)(*cp)->ptcl;
		for(lifc = ifc->lifc; lifc; lifc = lifc->next){
			maskip(remote, lifc->mask, gnet);
			if(ipcmp(gnet, lifc->net) == 0){
				if(x == nil || ipcmp(lifc->mask, xmask) > 0){
					x = ifc;
					ipmove(xmask, lifc->mask);
				}
			}
		}
	}
	if(x != nil)
		return x;

	/* for now for broadcast and multicast, just use first interface */
	if(type & (Rbcast|Rmulti)){
		for(cp = f->ipifc->conv; cp < e; cp++){
			if(*cp == 0)
				continue;
			ifc = (Ipifc*)(*cp)->ptcl;
			if(ifc->lifc != nil)
				return ifc;
		}
	}
		
	return nil;
}

/*
 *  returns first ip address configured
 */
void
findprimaryip(Fs *f, uchar *local)
{
	Conv **cp, **e;
	Ipifc *ifc;
	Iplifc *lifc;

	/* find first ifc local address */
	e = &f->ipifc->conv[f->ipifc->nc];
	for(cp = f->ipifc->conv; cp < e; cp++){
		if(*cp == 0)
			continue;
		ifc = (Ipifc*)(*cp)->ptcl;
		for(lifc = ifc->lifc; lifc; lifc = lifc->next){
			ipmove(local, lifc->local);
			return;
		}
	}
}

/*
 *  find the local address 'closest' to the remote system, copy it to
 *  local and return the ifc for that address
 */
void
findlocalip(Fs *f, uchar *local, uchar *remote)
{
	Ipifc *ifc;
	Iplifc *lifc;
	Route *r;
	uchar gate[IPaddrlen];
	uchar gnet[IPaddrlen];

	qlock(f->ipifc);
	r = v6lookup(f, remote);
	
	if(r != nil){
		ifc = r->ifc;
		if(r->type & Rv4)
			v4tov6(gate, r->v4.gate);
		else
			ipmove(gate, r->v6.gate);

		/* find ifc address closest to the gateway to use */
		for(lifc = ifc->lifc; lifc; lifc = lifc->next){
			maskip(gate, lifc->mask, gnet);
			if(ipcmp(gnet, lifc->net) == 0){
				ipmove(local, lifc->local);
				goto out;
			}
		}
	}

	findprimaryip(f, local);

out:
	qunlock(f->ipifc);
}

/*
 *  return first v4 address associated with an interface
 */
int
ipv4local(Ipifc *ifc, uchar *addr)
{
	Iplifc *lifc;

	for(lifc = ifc->lifc; lifc; lifc = lifc->next){
		if(isv4(lifc->local)){
			memmove(addr, lifc->local+IPv4off, IPv4addrlen);
			return 1;
		}
	}
	return 0;
}

/*
 *  return first v6 address associated with an interface
 */
int
ipv6local(Ipifc *ifc, uchar *addr)
{
	Iplifc *lifc;

	for(lifc = ifc->lifc; lifc; lifc = lifc->next){
		if(!isv4(lifc->local)){
			ipmove(addr, lifc->local);
			return 1;
		}
	}
	return 0;
}

/*
 *  see if this address is bound to the interface
 */
Iplifc*
iplocalonifc(Ipifc *ifc, uchar *ip)
{
	Iplifc *lifc;

	for(lifc = ifc->lifc; lifc; lifc = lifc->next)
		if(ipcmp(ip, lifc->local) == 0)
			return lifc;
	return nil;
}


/*
 *  See if we're proxying for this address on this interface
 */
int
ipproxyifc(Fs *f, Ipifc *ifc, uchar *ip)
{
	Route *r;
	uchar net[IPaddrlen];
	Iplifc *lifc;

	/* see if this is a direct connected pt to pt address */
	r = v6lookup(f, ip);
	if(r == nil)
		return 0;
	if((r->type & (Rifc|Rptpt|Rproxy)) != (Rifc|Rptpt|Rproxy))
		return 0;

	/* see if this is on the right interface */
	for(lifc = ifc->lifc; lifc; lifc = lifc->next){
		maskip(ip, lifc->mask, net);
		if(ipcmp(net, lifc->remote) == 0)
			return 1;
	}

	return 0;
}

/*
 *  return multicast version if any
 */
int
ipismulticast(uchar *ip)
{
	if(isv4(ip)){
		if(ip[IPv4off] >= 0xe0 && ip[IPv4off] < 0xf0)
			return V4;
	} else {
		if(ip[0] == 0xff)
			return V6;
	}
	return 0;
}
int
ipisbm(uchar *ip)
{
	if(isv4(ip)){
		if(ip[IPv4off] >= 0xe0 && ip[IPv4off] < 0xf0)
			return V4;
		if(ipcmp(ip, IPv4bcast) == 0)
			return V4;
	} else {
		if(ip[0] == 0xff)
			return V6;
	}
	return 0;
}


/*
 *  add a multicast address to an interface, called with c->car locked
 */
void
ipifcaddmulti(Conv *c, uchar *ma, uchar *ia)
{
	Ipifc *ifc;
	Iplifc *lifc;
	Conv **p;
	Ipmulti *multi, **l;
	Fs *f;

	f = c->p->f;
	
	for(l = &c->multi; *l; l = &(*l)->next)
		if(ipcmp(ma, (*l)->ma) == 0)
		if(ipcmp(ia, (*l)->ia) == 0)
			return;		/* it's already there */

	multi = *l = smalloc(sizeof(*multi));
	ipmove(multi->ma, ma);
	ipmove(multi->ia, ia);
	multi->next = nil;

	for(p = f->ipifc->conv; *p; p++){
		if((*p)->inuse == 0)
			continue;
		ifc = (Ipifc*)(*p)->ptcl;
		if(waserror()){
			wunlock(ifc);
			nexterror();
		}
		wlock(ifc);
		for(lifc = ifc->lifc; lifc; lifc = lifc->next)
			if(ipcmp(ia, lifc->local) == 0)
				addselfcache(f, ifc, lifc, ma, Rmulti);
		wunlock(ifc);
		poperror();
	}
}


/*
 *  remove a multicast address from an interface, called with c->car locked
 */
void
ipifcremmulti(Conv *c, uchar *ma, uchar *ia)
{
	Ipmulti *multi, **l;
	Iplifc *lifc;
	Conv **p;
	Ipifc *ifc;
	Fs *f;

	f = c->p->f;
	
	for(l = &c->multi; *l; l = &(*l)->next)
		if(ipcmp(ma, (*l)->ma) == 0)
		if(ipcmp(ia, (*l)->ia) == 0)
			break;

	multi = *l;
	if(multi == nil)
		return; 	/* we don't have it open */

	*l = multi->next;

	for(p = f->ipifc->conv; *p; p++){
		if((*p)->inuse == 0)
			continue;

		ifc = (Ipifc*)(*p)->ptcl;
		if(waserror()){
			wunlock(ifc);
			nexterror();
		}
		wlock(ifc);
		for(lifc = ifc->lifc; lifc; lifc = lifc->next)
			if(ipcmp(ia, lifc->local) == 0)
				remselfcache(f, ifc, lifc, ma);
		wunlock(ifc);
		poperror();
	}

	free(multi);
}

/*
 *  make lifc's join and leave multicast groups
 */
static char*
ipifcjoinmulti(Ipifc *ifc, char **argv, int argc)
{
	USED(ifc, argv, argc);
	return nil;
}

static char*
ipifcleavemulti(Ipifc *ifc, char **argv, int argc)
{
	USED(ifc, argv, argc);
	return nil;
}

static void
ipifcregisterproxy(Fs *f, Ipifc *ifc, uchar *ip)
{
	Conv **cp, **e;
	Ipifc *nifc;
	Iplifc *lifc;
	Medium *m;
	uchar net[IPaddrlen];

	/* register the address on any network that will proxy for us */
	e = &f->ipifc->conv[f->ipifc->nc];
	for(cp = f->ipifc->conv; cp < e; cp++){
		if(*cp == nil)
			continue;
		nifc = (Ipifc*)(*cp)->ptcl;
		if(nifc == ifc)
			continue;

		rlock(nifc);
		m = nifc->m;
		if(m == nil || m->areg == nil){
			runlock(nifc);
			continue;
		}
		for(lifc = nifc->lifc; lifc; lifc = lifc->next){
			maskip(ip, lifc->mask, net);
			if(ipcmp(net, lifc->remote) == 0){
				(*m->areg)(nifc, ip);
				break;
			}
		}
		runlock(nifc);
	}
}

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"arp.h"
#include 	"../port/ipdat.h"

#include	"devtab.h"

#define ARP_FREE	0
#define ARP_OK		1
#define ARP_ASKED	2
#define ARP_TEMP	0
#define ARP_PERM	1
#define Arphashsize	32
#define ARPHASH(p)	arphash[((p[2]^p[3])%Arphashsize)]

typedef struct Arpcache Arpcache;
struct Arpcache
{
	uchar	status;
	uchar	type;
	uchar	eip[4];
	uchar	et[6];
	Arpcache *hash;
	Arpcache **hashhd;
	Arpcache *frwd;
	Arpcache *prev;
};

Arpstats	arpstats;
Arpcache 	*arplruhead, *arplrutail;
Arpcache 	*arp, **arphash;
Queue		*Servq;
Lock		larphash;

void	arpiput(Queue *, Block *);
void	arpoput(Queue *, Block *);
void	arpopn(Queue *, Stream *);
void	arpcls(Queue *);
void	arpenter(Arpentry*, int);
void	arpflush(void);
int	arpperm(char*);
int	arpdelete(char*);
void	arplinkhead(Arpcache*);
int	arplookup(uchar*, uchar*);

Qinfo arpinfo = { arpiput, arpoput, arpopn, arpcls, "arp" };

#define ARP_ENTRYLEN	50
char *padstr = "                                           ";

enum{
	arpdirqid,
	arpdir2qid,
	arpstatqid,
	arpctlqid,
	arpdataqid,
};

Dirtab arptab[]={
	"stats",	{arpstatqid},		0,	0444,
	"ctl",		{arpctlqid},		0,	0664,
	"data",		{arpdataqid},		0,	0664,
};
#define Narptab (sizeof(arptab)/sizeof(Dirtab))

enum
{
	Narp=	64,	/* size of arp cache */
};

/*
 *  create a 2-level directory
 */
int
arpgen(Chan *c, void *vp, int ntab, int i, Dir *dp)
{
	Qid q;

	USED(vp);
	USED(ntab);

	q.vers = 0;

	/* top level directory contains the directory arp */
	if(c->qid.path == CHDIR){
		if(i)
			return -1;
		q.path = CHDIR | arpdir2qid;
		devdir(c, q, "arp", 0, eve, 0555, dp);
		return 1;
	}

	/* next level uses table */
	return devgen(c, arptab, Narptab, i, dp);
}

void
arpreset(void)
{
	Arpcache *ap, *ep;

	arp = xalloc(sizeof(Arpcache) * Narp);
	arphash = (Arpcache **)xalloc(sizeof(Arpcache *) * Arphashsize);

	ep = &arp[Narp];
	for(ap = arp; ap < ep; ap++) {
		ap->frwd = ap+1;
		ap->prev = ap-1;
		ap->type = ARP_FREE;
		ap->status = ARP_TEMP;
	}

	arp[0].prev = 0;
	arplruhead = arp;
	ap = &arp[Narp-1];
	ap->frwd = 0;
	arplrutail = ap;
	newqinfo(&arpinfo);
}

void
arpinit(void)
{
}

Chan *
arpattach(char *spec)
{
	return devattach('a', spec);
}

Chan *
arpclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
arpwalk(Chan *c, char *name)
{
	return devwalk(c, name, 0, 0, arpgen);
}

void
arpstat(Chan *c, char *db)
{
	devstat(c, db, 0, 0, arpgen);
}

Chan *
arpopen(Chan *c, int omode)
{
	if(c->qid.path&CHDIR){
		if(omode != OREAD)
			error(Eperm);
	}

	switch(STREAMTYPE(c->qid.path)) {
	case arpdataqid:
		break;
	case arpstatqid:
		if(omode != OREAD)
			error(Ebadarg);
		break;
	case arpctlqid:
		break;
	}

	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

void
arpcreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
arpremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
arpwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

void
arpclose(Chan *c)
{
	streamclose(c);
}

long
arpread(Chan *c, void *a, long n, ulong offset)
{
	char	 buf[100];
	Arpcache *ap;
	int	 part, bytes, size;
	char	 *ststr;

	if(c->qid.path&CHDIR)
		return devdirread(c, a, n, arptab, Narptab, arpgen);

	switch((int)(c->qid.path&~CHDIR)){
	case arpdataqid:
		bytes = c->offset;
		while(bytes < Narp*ARP_ENTRYLEN && n) {
			ap = &arp[bytes/ARP_ENTRYLEN];
			part = bytes%ARP_ENTRYLEN;

			if(ap->status != ARP_OK)
				ststr = "invalid";
			else
				ststr = (ap->type == ARP_TEMP ? "temp" : "perm");

			sprint(buf,"%d.%d.%d.%d to %.2x:%.2x:%.2x:%.2x:%.2x:%.2x %s%s",
				ap->eip[0], ap->eip[1], ap->eip[2], ap->eip[3],
				ap->et[0], ap->et[1], ap->et[2], ap->et[3],
				ap->et[4], ap->et[5],
				ststr, padstr); 
			
			buf[ARP_ENTRYLEN-1] = '\n';

			size = ARP_ENTRYLEN - part;
			size = MIN(n, size);
			memmove(a, buf+part, size);

			a = (void *)((int)a + size);
			n -= size;
			bytes += size;
		}
		return bytes - c->offset;
		break;
	case arpstatqid:
		sprint(buf, "hits: %d miss: %d failed: %d\n",
			arpstats.hit, arpstats.miss, arpstats.failed);

		return readstr(offset, a, n, buf);
	default:
		n=0;
		break;
	}
	return n;
}

long
arpwrite(Chan *c, char *a, long n, ulong offset)
{
	int m;
	Arpentry entry;
	char buf[32], *field[5];

	USED(offset);

	switch(STREAMTYPE(c->qid.path)) {
	case arpctlqid:
		strncpy(buf, a, sizeof buf);
		m = getfields(buf, field, 5, " ");

		if(strncmp(field[0], "flush", 5) == 0)
			arpflush();
		else
		if(strcmp(field[0], "delete") == 0) {
			if(m != 2)
				error(Ebadarg);

			if(arpdelete(field[1]) < 0)
				error(Enetaddr);
		}
		else
		if(strcmp(field[0], "perm") == 0) {
			if(m != 2)
				error(Ebadarg);

			if(arpperm(field[1]) < 0)
				error(Enetaddr);
		}
		else
			error(Ebadctl);
		break;

	case arpdataqid:
		if(n != sizeof(Arpentry))
			error(Emsgsize);
		memmove(&entry, a, sizeof(Arpentry));
		arpenter(&entry, ARP_TEMP);
		break;

	default:
		error(Ebadusefd);
	}

	return n;
}

void
arpopn(Queue *q, Stream *s)
{
	USED(q, s);
}

void
arpcls(Queue *q)
{
	if(q == Servq)
		Servq = 0;
}

void
arpiput(Queue *q, Block *bp)
{
	PUTNEXT(q, bp);
}

void
arpoput(Queue *q, Block *bp)
{
	uchar ip[4];
	Ipaddr addr;
	Ipdevice *p;
	Etherhdr *eh;
	static int dropped;


	if(bp->type != M_DATA) {
		if(Servq == 0 && streamparse("arpd", bp)) {
			Servq = RD(q);
			freeb(bp);
		}
		else
			PUTNEXT(q, bp);
		return;
	}

	eh = (Etherhdr *)bp->rptr;
	if(nhgets(eh->type) != ET_IP) {
		PUTNEXT(q, bp);	
		return;
	}

	/*
	 * sleaze - we hid the next hop in the ethernet destination
	 *  and the interface in the first byte of the source
	 */
	memmove(ip, eh->d, sizeof(ip));
	p = &ipd[eh->s[0]];
	if(p >= &ipd[Nipd] || p->q == 0)
		p = ipd;

	/* if ip broadcast, use ether bcast address */
	addr = nhgetl(eh->dst);
	if(p->Myip[Myself] == 0 || addr == p->Myip[Mybcast] || addr == p->Myip[Mynet]
	|| ((addr & p->Mymask) == p->Myip[Mynet+1]
	    && (addr & ~p->Mynetmask) == ~p->Mynetmask)){
		memset(eh->d, 0xff, sizeof(eh->d));
		PUTNEXT(q, bp);
		return;
	}

	/* if a known ip addr, send downstream to the ethernet */
	if(arplookup(ip, eh->d)) {
		PUTNEXT(q, bp);
		return;
	}

	/* Push the packet up to the arp server for address resolution */
	if(!Servq) {
		if((dropped++ % 100) == 99)
			print("arp: No server, packet dropped %d.%d.%d.%d\n",
				eh->dst[0], eh->dst[1], eh->dst[2], eh->dst[3]);
		freeb(bp);
		return;
	}
	memmove(eh->d, ip, sizeof(ip));
	PUTNEXT(Servq, bp);
}

int
arplookup(uchar *ip, uchar *et)
{
	Arpcache *ap;

	lock(&larphash);
	for(ap = ARPHASH(ip); ap; ap = ap->hash) {
		if(ap->status == ARP_OK && memcmp(ap->eip, ip, sizeof(ap->eip)) == 0) {
			memmove(et, ap->et, sizeof(ap->et));
			arplinkhead(ap);
			arpstats.hit++;
			unlock(&larphash);
			return 1;
		}
	}
	arpstats.miss++;
	unlock(&larphash);
	return 0;
}

void
arpflush(void)
{
	Arpcache *ap, *ep;

	ep = &arp[Narp];
	for(ap = arp; ap < ep; ap++)
		ap->status = ARP_FREE;
}

void
arpenter(Arpentry *ape, int type)
{
	Arpcache *ap, **l, *d;


	/* Update an entry if we have one already */
	l = &ARPHASH(ape->ipaddr);
	lock(&larphash);
	for(ap = *l; ap; ap = ap->hash) {
		if(ap->status == ARP_OK && memcmp(ap->eip, ape->ipaddr, sizeof(ap->eip)) == 0) {
			if(ap->type != ARP_PERM) {
				ap->type = type;
				memmove(ap->et, ape->etaddr, sizeof(ap->et));
				ap->status = ARP_OK;
			}
			unlock(&larphash);
			return;
		}
	}

	/* Find an entry to replace */
	ap = arplrutail;
	while(ap) {
		if(ap->type != ARP_PERM || ap->type == ARP_FREE)
			break;
		 ap = ap->prev;
	}
	if(!ap) {
		unlock(&larphash);
		print("arp: too many permanent entries\n");
		return;
	}

	if(ap->hashhd) {
		for(d = *ap->hashhd; d; d = d->hash) {
			if(d == ap) {
				*(ap->hashhd) = ap->hash;
				break;
			}
			ap->hashhd = &d->hash;
		}
	}

	ap->type = type;
	ap->status = ARP_OK;
	memmove(ap->eip, ape->ipaddr, sizeof(ape->ipaddr));
	memmove(ap->et, ape->etaddr, sizeof(ape->etaddr));
	ap->hashhd = l;
	ap->hash = *l;
	*l = ap;
	arplinkhead(ap);
	unlock(&larphash);
}

int
arpperm(char *addr)
{
	Arpcache *ap;
	uchar ip[4];
	Ipaddr i;
	int rv;

	rv = -1;
	i = ipparse(addr);
	hnputl(ip, i);	
	lock(&larphash);
	for(ap = arplruhead; ap; ap = ap->frwd) {
		if(memcmp(ap->eip, ip, sizeof(ap->eip)) == 0) {
			ap->type = ARP_PERM;
			rv = 0;
			break;
		}
	}
	unlock(&larphash);
	return rv;
}

int
arpdelete(char *addr)
{
	Arpcache *ap;
	uchar ip[4];
	Ipaddr i;
	int rv;

	rv = -1;
	i = ipparse(addr);
	hnputl(ip, i);	
	lock(&larphash);
	for(ap = arplruhead; ap; ap = ap->frwd) {
		if(memcmp(ap->eip, ip, sizeof(ap->eip)) == 0) {
			ap->status = ARP_FREE;
			rv = 0;
			break;
		}
	}
	unlock(&larphash);
	return rv;
}

void
arplinkhead(Arpcache *ap)
{
	if(ap == arplruhead)
		return;

	if(ap->prev)
		ap->prev->frwd = ap->frwd;
	else
		arplruhead = ap->frwd;
	
	if(ap->frwd)
		ap->frwd->prev = ap->prev;
	else
		arplrutail = ap->prev;
		
	ap->frwd = arplruhead;
	ap->prev = 0;
	arplruhead->prev = ap;
	arplruhead = ap;
}

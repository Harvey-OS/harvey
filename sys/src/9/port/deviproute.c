#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"arp.h"
#include 	"../port/ipdat.h"

#include	"devtab.h"

/*
 *  All ip numbers and masks are stored as ulongs.
 *  All interfaces to this code uses the standard byte
 *  string representation.
 */

typedef	struct Iproute	Iproute;
typedef	struct Iprtab	Iprtab;

enum
{
	Nroutes=	1024,
};

/*
 *  routes
 */
struct Iproute {
	ulong	dst;
	ulong	gate;
	ulong	mask;
	Iproute	*next;
	int	inuse;
};
struct Iprtab {
	Lock;
	int	n;		/* number of valid routes */
	Iproute *first;		/* list of valid routes */
	Iproute	r[Nroutes];	/* all routes */
};
Iprtab	iprtab;

/*
 *  The chosen route is the one obeys the constraint
 *		r->mask & dst == r->dst
 *
 *  If there are several matches, the one whose mask has the most
 *  leading ones (and hence is the most specific) wins.  This is
 *  forced by storing the routes in decreasing number of ones order
 *  and returning the first match.  The default gateway has no ones
 *  in the mask and is thus the last matched.
 */
Ipdevice*
iproute(uchar *dst, uchar *gate)
{
	Iproute *r;
	Ipdevice *p;
	ulong udst;

	udst = nhgetl(dst);
	for(p = ipd; p < &ipd[Nipd]; p++){
		if(p->q == 0)
			continue;
		if((udst&p->Mynetmask) == p->Remip){
			memmove(gate, dst, 4);
			return p;
		}
	}

	/*
	 *  first check routes
	 */
	for(r = iprtab.first; r; r = r->next){
		if((r->mask&udst) == r->dst){
			hnputl(gate, r->gate);
			for(p = ipd; p < &ipd[Nipd]; p++){
				if(p->q == 0)
					break;
				if((r->gate&p->Mynetmask) == p->Remip)
					return p;
			}
			return ipd;
		}
	}

	/*
	 *  else just return the same address and first interface
	 */	
	memmove(gate, dst, 4);
	return ipd;
}

/*
 *  Add a route, create a mask if the first mask is 0.
 *
 *  All routes are stored sorted by the length of leading
 *  ones in the mask.
 *
 *  NOTE: A default route has an all zeroes mask and dst.
 */
void
ipaddroute(ulong dst, ulong mask, ulong gate)
{
	Iproute *r, *e, *free;

	/*
	 *  filter out impossible requests
	 */
	if((dst&mask) != dst)
		error(Enetaddr);

	/*
	 *  see if we already have a route for
	 *  the destination
	 */
	lock(&iprtab);
	free = 0;
	for(r = iprtab.r; r < &iprtab.r[Nroutes]; r++){
		if(r->inuse == 0){
			free = r;
			continue;
		}
		if(dst==r->dst && mask==r->mask){
			r->gate = gate;
			unlock(&iprtab);
			return;
		}
	}
	if(free == 0){
		unlock(&iprtab);
		exhausted("routes");
	}

	/*
	 *  add the new route in sorted order
	 */
	free->dst = dst;
	free->mask = mask;
	free->gate = gate;
	free->inuse = 1;
	for(r = e = iprtab.first; r; r = r->next){
		if(mask > r->mask)
			break;
		e = r;
	}
	free->next = r;
	if(r == iprtab.first)
		iprtab.first = free;
	else
		e->next = free;
	iprtab.n++;
	unlock(&iprtab);
}

/*
 *  remove a route
 */
void
ipremroute(ulong dst, ulong mask)
{
	Iproute *r, *e;

	lock(&iprtab);
	for(r = e = iprtab.first; r; r = r->next){
		if(dst==r->dst && (mask == 0 || mask==r->mask)){
			if(r == iprtab.first)
				iprtab.first = r->next;
			else
				e->next = r->next;
			r->inuse = 0;
			iprtab.n--;
			break;
		}
		e = r;
	}
	unlock(&iprtab);
}

/*
 *  remove all routes
 */
void
ipflushroute(void)
{
	Iproute *r;

	lock(&iprtab);
	for(r = iprtab.first; r; r = r->next)
		r->inuse = 0;
	iprtab.first = 0;
	iprtab.n = 0;
	unlock(&iprtab);
}

/*
 *  device interface
 */
enum{
	Qdir,
	Qdata,
	Qipifc,
};
Dirtab iproutetab[]={
	"iproute",		{Qdata},		0,	0666,
	"ipifc",		{Qipifc},		0,	0666,
};
#define Niproutetab (sizeof(iproutetab)/sizeof(Dirtab))

void
iproutereset(void)
{
}

void
iprouteinit(void)
{
}

Chan *
iprouteattach(char *spec)
{
	return devattach('P', spec);
}

Chan *
iprouteclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
iproutewalk(Chan *c, char *name)
{
	return devwalk(c, name, iproutetab, (long)Niproutetab, devgen);
}

void
iproutestat(Chan *c, char *db)
{
	devstat(c, db, iproutetab, (long)Niproutetab, devgen);
}

Chan *
iprouteopen(Chan *c, int omode)
{
	if(c->qid.path == CHDIR){
		if(omode != OREAD)
			error(Eperm);
	}
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

void
iproutecreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
iprouteremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
iproutewstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

void
iprouteclose(Chan *c)
{
	USED(c);
}

/* sprint an ip address right justified into a 16 char field */
char*
ipformat(char *to, ulong ip)
{
	uchar hip[4];
	char buf[17];
	int n;

	hnputl(hip, ip);
	n = sprint(buf, "%d.%d.%d.%d", hip[0], hip[1], hip[2], hip[3]);
	memset(to, ' ', 16 - n);
	memmove(to+16-n, buf, n);
	to[16] = 0;
	return to;
}

long
iprouteread(Chan *c, void *a, long n, ulong offset)
{
	Iproute	*r;
	Ipdevice *p;
	char	 *va = a;
	char	 *e;
	int	part, bytes, size;
	char	dst[17], mask[17], gate[17];
	char	buf[128];

	switch((int)(c->qid.path&~CHDIR)){
	case Qdir:
		return devdirread(c, a, n, iproutetab, Niproutetab, devgen);
	case Qdata:
		lock(&iprtab);
		bytes = 0;
		e = va + n;
		for(r = iprtab.first; r && va < e; r = r->next){
			size = sprint(buf, "%s & %s -> %s\n", ipformat(dst, r->dst),
				ipformat(mask, r->mask), ipformat(gate, r->gate));
			if(bytes + size <= offset){
				bytes += size;
				continue;
			}

			if(va == a){
				part = offset - bytes;
				size -= part;
			} else
				part = 0;
			if(size > n)
				size = n;

			memmove(va, buf + part, size);
			va += size;
		}
		unlock(&iprtab);
		n = va - (char*)a;
		break;
	case Qipifc:
		bytes = 0;
		e = va + n;
		for(p = ipd; p < &ipd[Nipd] && va < e; p++){
			if(p->q == 0)
				continue;
			size = sprint(buf, "  #%C%d %5d %s %s %s\n",
				devchar[p->type], p->dev, p->maxmtu,
				ipformat(dst, p->Myip[0]),
				ipformat(mask, p->Mynetmask),
				ipformat(gate, p->Remip));
			if(bytes + size <= offset){
				bytes += size;
				continue;
			}

			if(va == a){
				part = offset - bytes;
				size -= part;
			} else
				part = 0;
			if(size > n)
				size = n;

			memmove(va, buf + part, size);
			va += size;
		}
		n = va - (char*)a;
		break;
	default:
		n=0;
		break;
	}
	return n;
}

long
iproutewrite(Chan *c, char *a, long n, ulong offset)
{
	char buf[128];
	char *field[4];
	Ipaddr mask, dst, gate;
	int m;

	USED(offset);

	switch((int)(c->qid.path&~CHDIR)){
	case Qdata:
		strncpy(buf, a, sizeof buf);
		m = getfields(buf, field, 4, " ");

		if(strncmp(field[0], "flush", 5) == 0)
			ipflushroute();
		else if(strncmp(field[0], "add", 3) == 0){
			switch(m){
			case 4:
				dst = ipparse(field[1]);
				mask = ipparse(field[2]);
				gate = ipparse(field[3]);
				ipaddroute(dst, mask, gate);
				break;
			case 3:
				dst = ipparse(field[1]);
				gate = ipparse(field[2]);
				ipaddroute(dst, classmask[dst>>30], gate);
				break;
			default:
				error(Ebadarg);
			}
		} else if(strncmp(field[0], "delete", 6) == 0){
			switch(m){
			case 3:
				dst = ipparse(field[1]);
				mask = ipparse(field[2]);
				ipremroute(dst, mask);
				break;
			case 2:
				dst = ipparse(field[1]);
				ipremroute(dst, 0);
				break;
			default:
				error(Ebadarg);
			}
		}
		else
			error(Ebadctl);
		break;
	default:
		error(Ebadusefd);
	}
	return n;
}

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"arp.h"
#include	"ipdat.h"

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
	Nroutes=	256,
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
void
iproute(uchar *dst, uchar *gate)
{
	Iproute *r;
	ulong udst;

	udst = nhgetl(dst);
	if((udst&Mynetmask) == (Myip[Myself]&Mynetmask)){
		memmove(gate, dst, 4);
		return;
	}

	/*
	 *  first check routes
	 */
	for(r = iprtab.first; r; r = r->next){
		if((r->mask&udst) == r->dst){
			hnputl(gate, r->gate);
			return;
		}
	}

	/*
	 *  else just return the same address
	 */	
	memmove(gate, dst, 4);
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
		if(dst==r->dst && mask==r->mask){
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
};
Dirtab iproutetab[]={
	"iproute",		{Qdata},		0,	0666,
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

#define IPR_ENTRYLEN 54
#define PAD "                                                                  "

long
iprouteread(Chan *c, void *a, long n, ulong offset)
{
	char	buf[IPR_ENTRYLEN*3];
	Iproute	*r;
	int	part, bytes, size;
	uchar	dst[4], mask[4], gate[4];

	switch((int)(c->qid.path&~CHDIR)){
	case Qdir:
		return devdirread(c, a, n, iproutetab, Niproutetab, devgen);
	case Qdata:
		lock(&iprtab);
		part = offset/IPR_ENTRYLEN;
		for(r = iprtab.first; part && r; r = r->next)
			part--;
		bytes = offset;
		while(r && bytes < iprtab.n*IPR_ENTRYLEN && n){
			part = bytes%IPR_ENTRYLEN;

			hnputl(dst, r->dst);
			hnputl(mask, r->mask);
			hnputl(gate, r->gate);
			sprint(buf,"%d.%d.%d.%d & %d.%d.%d.%d -> %d.%d.%d.%d%s",
				dst[0], dst[1], dst[2], dst[3],
				mask[0], mask[1], mask[2], mask[3],
				gate[0], gate[1], gate[2], gate[3],
				PAD); 
			
			buf[IPR_ENTRYLEN-1] = '\n';

			size = IPR_ENTRYLEN - part;
			if(size > n)
				size = n;
			memmove(a, buf+part, size);

			a = (void *)((int)a + size);
			n -= size;
			bytes += size;
			r = r->next;
		}
		unlock(&iprtab);
		return bytes - offset;
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
	char buf[IPR_ENTRYLEN];
	char *field[4];
	Ipaddr mask, dst, gate;
	int m;

	USED(offset);

	switch((int)(c->qid.path&~CHDIR)){
	case Qdata:
		strncpy(buf, a, sizeof buf);
		m = getfields(buf, field, 4, ' ');

		if(strncmp(field[0], "flush", 5) == 0)
			ipflushroute();
		else if(strcmp(field[0], "add") == 0){
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
		} else if(strcmp(field[0], "delete") == 0){
			switch(m){
			case 3:
				dst = ipparse(field[1]);
				mask = ipparse(field[2]);
				ipremroute(dst, mask);
				break;
			case 2:
				dst = ipparse(field[1]);
				ipremroute(dst, classmask[dst>>30]);
				break;
			default:
				error(Ebadarg);
			}
		}
		break;
	default:
		error(Ebadusefd);
	}
	return n;
}

#include <u.h>
#include <libc.h>
#include <ip.h>
#include "dns.h"

/*
 *  a dictionary of domain names for packing messages
 */
enum
{
	Ndict=	64,
};
typedef struct Dict	Dict;
struct Dict
{
	struct {
		ushort	offset;		/* pointer to packed name in message */
		char	*name;		/* pointer to unpacked name in buf */
	} x[Ndict];
	int n;			/* size of dictionary */
	uchar *start;		/* start of packed message */
	char buf[1024];		/* buffer for unpacked names */
	char *ep;		/* first free char in buf */
};

#define NAME(x)		p = pname(m, p, ep, x, dp)
#define STRING(x)	p = pstring(m, p, ep, x)
#define USHORT(x)	p = pushort(m, p, ep, x)
#define ULONG(x)	p = pulong(m, p, ep, x)
#define ADDR(x)		p = paddr(m, p, ep, x)

static uchar*
pstring(DNSmsg *m, uchar *p, uchar *ep, char *np)
{
	int n;

	n = strlen(np);
	if(n >= Strlen)			/* DNS maximum length string */
		n = Strlen - 1;
	if(ep - p <= n){		/* see if it fits in the buffer */
		m->flags |= Ftrunc;
		return ep;
	}
	*p++ = n;
	memcpy(p, np, n);
	return p + n;
}

static uchar*
pushort(DNSmsg *m, uchar *p, uchar *ep, int val)
{
	if(ep - p < 2){
		m->flags |= Ftrunc;
		return ep;
	}
	*p++ = val>>8;
	*p++ = val;
	return p;
}

static uchar*
pulong(DNSmsg *m, uchar *p, uchar *ep, int val)
{
	if(ep - p < 4){
		m->flags |= Ftrunc;
		return ep;
	}
	*p++ = val>>24;
	*p++ = val>>16;
	*p++ = val>>8;
	*p++ = val;
	return p;
}

static uchar*
paddr(DNSmsg *m, uchar *p, uchar *ep, char *name)
{
	if(ep - p < 4){
		m->flags |= Ftrunc;
		return ep;
	}
	parseip(p, name);
	return p + 4;

}

static uchar*
pname(DNSmsg *m, uchar *p, uchar *ep, char *np, Dict *dp)
{
	char *cp;
	int i;
	char *last;		/* last component packed */

	if(strlen(np) >= Domlen){	/* make sure we don't exceed DNS limits */
		m->flags |= Ftrunc;
		return ep;
	}

	last = 0;
	while(*np){
		/* look through every component in the dictionary for a match */
		for(i = 0; i < dp->n; i++){
			if(strcmp(np, dp->x[i].name) == 0){
				if(ep - p < 2){
					m->flags |= Ftrunc;
					return ep;
				}
				*p++ = (dp->x[i].offset>>8) | 0xc0;
				*p++ = dp->x[i].offset;
				return p;
			}
		}

		/* if there's room, enter this name in dictionary */
		if(dp->n < Ndict){
			if(last){
				/* the whole name is already in dp->buf */
				last = strchr(last, '.') + 1;
				dp->x[dp->n].name = last;
				dp->x[dp->n].offset = p - dp->start;
				dp->n++;
			} else {
				/* add to dp->buf */
				i = strlen(np);
				if(dp->ep + i + 1 < &dp->buf[sizeof(dp->buf)]){
					strcpy(dp->ep, np);
					dp->x[dp->n].name = dp->ep;
					last = dp->ep;
					dp->x[dp->n].offset = p - dp->start;
					dp->ep += i + 1;
					dp->n++;
				}
			}
		}

		/* put next component into message */
		cp = strchr(np, '.');
		if(cp == 0){
			i = strlen(np);
			cp = np + i;	/* point to null terminator */
		} else {
			i = cp - np;
			cp++;		/* point past '.' */
		}
		if(ep-p < i+1){
			m->flags |= Ftrunc;
			return ep;
		}
		*p++ = i;		/* count of chars in label */
		memcpy(p, np, i);
		np = cp;
		p += i;
	}
	*p++ = 0;	/* add top level domain */

	return p;
}

static uchar*
convRR2M(DNSmsg *m, RR *rp, uchar *p, uchar *ep, Dict *dp)
{
	uchar *lp, *data;
	int len;

	NAME(rp->owner->name);
	USHORT(rp->type);
	USHORT(rp->owner->class);
	ULONG(rp->ttl);
	lp = p;			/* leave room for the rdata length */
	p += 2;
	data = p;
	switch(rp->type){
	case Thinfo:
		STRING(rp->cpu->name);
		STRING(rp->os->name);
		break;
	case Tcname:
	case Tmb:
	case Tmd:
	case Tmf:
	case Tns:
		NAME(rp->host->name);
		break;
	case Tmg:
	case Tmr:
		NAME(rp->mb->name);
		break;
	case Tminfo:
		NAME(rp->rmb->name);
		NAME(rp->mb->name);
		break;
	case Tmx:
		USHORT(rp->pref);
		NAME(rp->host->name);
		break;
	case Ta:
		ADDR(rp->ip->name);
		break;
	case Tptr:
		NAME(rp->ptr->name);
		break;
	case Tsoa:
		NAME(rp->host->name);
		NAME(rp->rmb->name);
		ULONG(rp->soa->serial);
		ULONG(rp->soa->refresh);
		ULONG(rp->soa->retry);
		ULONG(rp->soa->expire);
		ULONG(rp->soa->minttl);
		break;
	}
	if(m->flags & Ftrunc)
		return ep+1;

	/* stuff in the rdata section length */
	len = p - data;
	*lp++ = len >> 8;
	*lp = len;

	return p;
}

static uchar*
convQ2M(DNSmsg *m, RR *rp, uchar *p, uchar *ep, Dict *dp)
{
	NAME(rp->owner->name);
	USHORT(rp->type);
	USHORT(rp->owner->class);
	if(m->flags & Ftrunc)
		return 0;
	return p;
}

static uchar*
rrloop(DNSmsg *m, RR *rp, long *countp, uchar *p, uchar *ep, Dict *dp, int quest)
{
	uchar *np;

	if(p > ep)
		return p;
	*countp = 0;
	for(; rp; rp = rp->next){
		if(quest)
			np = convQ2M(m, rp, p, ep, dp);
		else
			np = convRR2M(m, rp, p, ep, dp);
		if(np == 0){
			p = ep+1;
			break;
		}
		p = np;
		(*countp)++;
	}
	return p;
}

/*
 *  convert into a message
 */
int
convDNS2M(DNSmsg *m, uchar *buf, int len)
{
	uchar *p, *ep, *np;
	Dict d;

	d.n = 0;
	d.start = buf;
	d.ep = d.buf;
	memset(buf, 0, len);

	/* first pack in the RR's so we can get real counts */
	p = buf + 12;
	ep = buf + len;
	p = rrloop(m, m->qd, &m->qdcount, p, ep, &d, 1);
	p = rrloop(m, m->an, &m->ancount, p, ep, &d, 0);
	p = rrloop(m, m->ns, &m->nscount, p, ep, &d, 0);
	p = rrloop(m, m->ar, &m->arcount, p, ep, &d, 0);
	if(p > ep)
		return -1;

	/* now pack the rest */
	np = p;
	p = buf;
	ep = buf + len;
	USHORT(m->id);
	USHORT(m->flags);
	USHORT(m->qdcount);
	USHORT(m->ancount);
	USHORT(m->nscount);
	USHORT(m->arcount);
	if(p > ep)
		return -1;

	return np - buf;
}

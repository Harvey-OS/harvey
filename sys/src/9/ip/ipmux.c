/*
 * IP packet filter
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#include "ip.h"
#include "ipv6.h"

typedef struct Ipmuxrock  Ipmuxrock;
typedef struct Ipmux      Ipmux;

typedef struct Myip4hdr Myip4hdr;
struct Myip4hdr
{
	uchar	vihl;		/* Version and header length */
	uchar	tos;		/* Type of service */
	uchar	length[2];	/* packet length */
	uchar	id[2];		/* ip->identification */
	uchar	frag[2];	/* Fragment information */
	uchar	ttl;		/* Time to live */
	uchar	proto;		/* Protocol */
	uchar	cksum[2];	/* Header checksum */
	uchar	src[4];		/* IP source */
	uchar	dst[4];		/* IP destination */

	uchar	data[1];	/* start of data */
};
Myip4hdr *ipoff = 0;

enum
{
	Tproto,
	Tdata,
	Tiph,
	Tdst,
	Tsrc,
	Tifc,

	Cother = 0,
	Cbyte,		/* single byte */
	Cmbyte,		/* single byte with mask */
	Cshort,		/* single short */
	Cmshort,	/* single short with mask */
	Clong,		/* single long */
	Cmlong,		/* single long with mask */
	Cifc,
	Cmifc,
};

char *ftname[] = 
{
[Tproto]	"proto",
[Tdata]		"data",
[Tiph]	 	"iph",
[Tdst]		"dst",
[Tsrc]		"src",
[Tifc]		"ifc",
};

/*
 *  a node in the decision tree
 */
struct Ipmux
{
	Ipmux	*yes;
	Ipmux	*no;
	uchar	type;		/* type of field(Txxxx) */
	uchar	ctype;		/* tupe of comparison(Cxxxx) */
	uchar	len;		/* length in bytes of item to compare */
	uchar	n;		/* number of items val points to */
	short	off;		/* offset of comparison */
	short	eoff;		/* end offset of comparison */
	uchar	skiphdr;	/* should offset start after ipheader */
	uchar	*val;
	uchar	*mask;
	uchar	*e;		/* val+n*len*/

	int	ref;		/* so we can garbage collect */
	Conv	*conv;
};

/*
 *  someplace to hold per conversation data
 */
struct Ipmuxrock
{
	Ipmux	*chain;
};

static int	ipmuxsprint(Ipmux*, int, char*, int);
static void	ipmuxkick(void *x);

static char*
skipwhite(char *p)
{
	while(*p == ' ' || *p == '\t')
		p++;
	return p;
}

static char*
follows(char *p, char c)
{
	char *f;

	f = strchr(p, c);
	if(f == nil)
		return nil;
	*f++ = 0;
	f = skipwhite(f);
	if(*f == 0)
		return nil;
	return f;
}

static Ipmux*
parseop(char **pp)
{
	char *p = *pp;
	int type, off, end, len;
	Ipmux *f;

	p = skipwhite(p);
	if(strncmp(p, "dst", 3) == 0){
		type = Tdst;
		off = (ulong)(ipoff->dst);
		len = IPv4addrlen;
		p += 3;
	}
	else if(strncmp(p, "src", 3) == 0){
		type = Tsrc;
		off = (ulong)(ipoff->src);
		len = IPv4addrlen;
		p += 3;
	}
	else if(strncmp(p, "ifc", 3) == 0){
		type = Tifc;
		off = -IPv4addrlen;
		len = IPv4addrlen;
		p += 3;
	}
	else if(strncmp(p, "proto", 5) == 0){
		type = Tproto;
		off = (ulong)&(ipoff->proto);
		len = 1;
		p += 5;
	}
	else if(strncmp(p, "data", 4) == 0 || strncmp(p, "iph", 3) == 0){
		if(strncmp(p, "data", 4) == 0) {
			type = Tdata;
			p += 4;
		}
		else {
			type = Tiph;
			p += 3;
		}
		p = skipwhite(p);
		if(*p != '[')
			return nil;
		p++;
		off = strtoul(p, &p, 0);
		if(off < 0 || off > (64-IP4HDR))
			return nil;
		p = skipwhite(p);
		if(*p != ':')
			end = off;
		else {
			p++;
			p = skipwhite(p);
			end = strtoul(p, &p, 0);
			if(end < off)
				return nil;
			p = skipwhite(p);
		}
		if(*p != ']')
			return nil;
		p++;
		len = end - off + 1;
	}
	else
		return nil;

	f = smalloc(sizeof(*f));
	f->type = type;
	f->len = len;
	f->off = off;
	f->val = nil;
	f->mask = nil;
	f->n = 1;
	f->ref = 1;
	if(type == Tdata)
		f->skiphdr = 1;
	else
		f->skiphdr = 0;

	return f;	
}

static int
htoi(char x)
{
	if(x >= '0' && x <= '9')
		x -= '0';
	else if(x >= 'a' && x <= 'f')
		x -= 'a' - 10;
	else if(x >= 'A' && x <= 'F')
		x -= 'A' - 10;
	else
		x = 0;
	return x;
}

static int
hextoi(char *p)
{
	return (htoi(p[0])<<4) | htoi(p[1]);
}

static void
parseval(uchar *v, char *p, int len)
{
	while(*p && len-- > 0){
		*v++ = hextoi(p);
		p += 2;
	}
}

static Ipmux*
parsemux(char *p)
{
	int n, nomask;
	Ipmux *f;
	char *val;
	char *mask;
	char *vals[20];
	uchar *v;

	/* parse operand */
	f = parseop(&p);
	if(f == nil)
		return nil;

	/* find value */
	val = follows(p, '=');
	if(val == nil)
		goto parseerror;

	/* parse mask */
	mask = follows(p, '&');
	if(mask != nil){
		switch(f->type){
		case Tsrc:
		case Tdst:
		case Tifc:
			f->mask = smalloc(f->len);
			v4parseip(f->mask, mask);
			break;
		case Tdata:
		case Tiph:
			f->mask = smalloc(f->len);
			parseval(f->mask, mask, f->len);
			break;
		default:
			goto parseerror;
		}
		nomask = 0;
	} else {
		nomask = 1;
		f->mask = smalloc(f->len);
		memset(f->mask, 0xff, f->len);
	}

	/* parse vals */
	f->n = getfields(val, vals, sizeof(vals)/sizeof(char*), 1, "|");
	if(f->n == 0)
		goto parseerror;
	f->val = smalloc(f->n*f->len);
	v = f->val;
	for(n = 0; n < f->n; n++){
		switch(f->type){
		case Tsrc:
		case Tdst:
		case Tifc:
			v4parseip(v, vals[n]);
			break;
		case Tproto:
		case Tdata:
		case Tiph:
			parseval(v, vals[n], f->len);
			break;
		}
		v += f->len;
	}

	f->eoff = f->off + f->len;
	f->e = f->val + f->n*f->len;
	f->ctype = Cother;
	if(f->n == 1){
		switch(f->len){
		case 1:
			f->ctype = nomask ? Cbyte : Cmbyte;
			break;
		case 2:
			f->ctype = nomask ? Cshort : Cmshort;
			break;
		case 4:
			if(f->type == Tifc)
				f->ctype = nomask ? Cifc : Cmifc;
			else
				f->ctype = nomask ? Clong : Cmlong;
			break;
		}
	}
	return f;

parseerror:
	if(f->mask)
		free(f->mask);
	if(f->val)
		free(f->val);
	free(f);
	return nil;
}

/*
 *  Compare relative ordering of two ipmuxs.  This doesn't compare the
 *  values, just the fields being looked at.  
 *
 *  returns:	<0 if a is a more specific match
 *		 0 if a and b are matching on the same fields
 *		>0 if b is a more specific match
 */
static int
ipmuxcmp(Ipmux *a, Ipmux *b)
{
	int n;

	/* compare types, lesser ones are more important */
	n = a->type - b->type;
	if(n != 0)
		return n;

	/* compare offsets, call earlier ones more specific */
	n = (a->off+((int)a->skiphdr)*(ulong)ipoff->data) - 
		(b->off+((int)b->skiphdr)*(ulong)ipoff->data);
	if(n != 0)
		return n;

	/* compare match lengths, longer ones are more specific */
	n = b->len - a->len;
	if(n != 0)
		return n;

	/*
	 *  if we get here we have two entries matching
	 *  the same bytes of the record.  Now check
	 *  the mask for equality.  Longer masks are
	 *  more specific.
	 */
	if(a->mask != nil && b->mask == nil)
		return -1;
	if(a->mask == nil && b->mask != nil)
		return 1;
	if(a->mask != nil && b->mask != nil){
		n = memcmp(b->mask, a->mask, a->len);
		if(n != 0)
			return n;
	}
	return 0;
}

/*
 *  Compare the values of two ipmuxs.  We're assuming that ipmuxcmp
 *  returned 0 comparing them.
 */
static int
ipmuxvalcmp(Ipmux *a, Ipmux *b)
{
	int n;

	n = b->len*b->n - a->len*a->n;
	if(n != 0)
		return n;
	return memcmp(a->val, b->val, a->len*a->n);
} 

/*
 *  add onto an existing ipmux chain in the canonical comparison
 *  order
 */
static void
ipmuxchain(Ipmux **l, Ipmux *f)
{
	for(; *l; l = &(*l)->yes)
		if(ipmuxcmp(f, *l) < 0)
			break;
	f->yes = *l;
	*l = f;
}

/*
 *  copy a tree
 */
static Ipmux*
ipmuxcopy(Ipmux *f)
{
	Ipmux *nf;

	if(f == nil)
		return nil;
	nf = smalloc(sizeof *nf);
	*nf = *f;
	nf->no = ipmuxcopy(f->no);
	nf->yes = ipmuxcopy(f->yes);
	nf->val = smalloc(f->n*f->len);
	nf->e = nf->val + f->len*f->n;
	memmove(nf->val, f->val, f->n*f->len);
	return nf;
}

static void
ipmuxfree(Ipmux *f)
{
	if(f->val != nil)
		free(f->val);
	free(f);
}

static void
ipmuxtreefree(Ipmux *f)
{
	if(f == nil)
		return;
	if(f->no != nil)
		ipmuxfree(f->no);
	if(f->yes != nil)
		ipmuxfree(f->yes);
	ipmuxfree(f);
}

/*
 *  merge two trees
 */
static Ipmux*
ipmuxmerge(Ipmux *a, Ipmux *b)
{
	int n;
	Ipmux *f;

	if(a == nil)
		return b;
	if(b == nil)
		return a;
	n = ipmuxcmp(a, b);
	if(n < 0){
		f = ipmuxcopy(b);
		a->yes = ipmuxmerge(a->yes, b);
		a->no = ipmuxmerge(a->no, f);
		return a;
	}
	if(n > 0){
		f = ipmuxcopy(a);
		b->yes = ipmuxmerge(b->yes, a);
		b->no = ipmuxmerge(b->no, f);
		return b;
	}
	if(ipmuxvalcmp(a, b) == 0){
		a->yes = ipmuxmerge(a->yes, b->yes);
		a->no = ipmuxmerge(a->no, b->no);
		a->ref++;
		ipmuxfree(b);
		return a;
	}
	a->no = ipmuxmerge(a->no, b);
	return a;
}

/*
 *  remove a chain from a demux tree.  This is like merging accept that
 *  we remove instead of insert.
 */
static int
ipmuxremove(Ipmux **l, Ipmux *f)
{
	int n, rv;
	Ipmux *ft;

	if(f == nil)
		return 0;		/* we've removed it all */
	if(*l == nil)
		return -1;

	ft = *l;
	n = ipmuxcmp(ft, f);
	if(n < 0){
		/* *l is maching an earlier field, descend both paths */
		rv = ipmuxremove(&ft->yes, f);
		rv += ipmuxremove(&ft->no, f);
		return rv;
	}
	if(n > 0){
		/* f represents an earlier field than *l, this should be impossible */
		return -1;
	}

	/* if we get here f and *l are comparing the same fields */
	if(ipmuxvalcmp(ft, f) != 0){
		/* different values mean mutually exclusive */
		return ipmuxremove(&ft->no, f);
	}

	/* we found a match */
	if(--(ft->ref) == 0){
		/*
		 *  a dead node implies the whole yes side is also dead.
		 *  since our chain is constrained to be on that side,
		 *  we're done.
		 */
		ipmuxtreefree(ft->yes);
		*l = ft->no;
		ipmuxfree(ft);
		return 0;
	}

	/*
	 *  free the rest of the chain.  it is constrained to match the
	 *  yes side.
	 */
	return ipmuxremove(&ft->yes, f->yes);
}

/*
 *  connection request is a semi separated list of filters
 *  e.g. proto=17;data[0:4]=11aa22bb;ifc=135.104.9.2&255.255.255.0
 *
 *  there's no protection against overlapping specs.
 */
static char*
ipmuxconnect(Conv *c, char **argv, int argc)
{
	int i, n;
	char *field[10];
	Ipmux *mux, *chain;
	Ipmuxrock *r;
	Fs *f;

	f = c->p->f;

	if(argc != 2)
		return Ebadarg;

	n = getfields(argv[1], field, nelem(field), 1, ";");
	if(n <= 0)
		return Ebadarg;

	chain = nil;
	mux = nil;
	for(i = 0; i < n; i++){
		mux = parsemux(field[i]);
		if(mux == nil){
			ipmuxtreefree(chain);
			return Ebadarg;
		}
		ipmuxchain(&chain, mux);
	}
	if(chain == nil)
		return Ebadarg;
	mux->conv = c;

	/* save a copy of the chain so we can later remove it */
	mux = ipmuxcopy(chain);
	r = (Ipmuxrock*)(c->ptcl);
	r->chain = chain;

	/* add the chain to the protocol demultiplexor tree */
	wlock(f);
	f->ipmux->priv = ipmuxmerge(f->ipmux->priv, mux);
	wunlock(f);

	Fsconnected(c, nil);
	return nil;
}

static int
ipmuxstate(Conv *c, char *state, int n)
{
	Ipmuxrock *r;
	
	r = (Ipmuxrock*)(c->ptcl);
	return ipmuxsprint(r->chain, 0, state, n);
}

static void
ipmuxcreate(Conv *c)
{
	Ipmuxrock *r;

	c->rq = qopen(64*1024, Qmsg, 0, c);
	c->wq = qopen(64*1024, Qkick, ipmuxkick, c);
	r = (Ipmuxrock*)(c->ptcl);
	r->chain = nil;
}

static char*
ipmuxannounce(Conv*, char**, int)
{
	return "ipmux does not support announce";
}

static void
ipmuxclose(Conv *c)
{
	Ipmuxrock *r;
	Fs *f = c->p->f;

	r = (Ipmuxrock*)(c->ptcl);

	qclose(c->rq);
	qclose(c->wq);
	qclose(c->eq);
	ipmove(c->laddr, IPnoaddr);
	ipmove(c->raddr, IPnoaddr);
	c->lport = 0;
	c->rport = 0;

	wlock(f);
	ipmuxremove(&(c->p->priv), r->chain);
	wunlock(f);
	ipmuxtreefree(r->chain);
	r->chain = nil;
}

/*
 *  takes a fully formed ip packet and just passes it down
 *  the stack
 */
static void
ipmuxkick(void *x)
{
	Conv *c = x;
	Block *bp;

	bp = qget(c->wq);
	if(bp != nil) {
		Myip4hdr *ih4 = (Myip4hdr*)(bp->rp);

		if((ih4->vihl & 0xF0) != IP_VER6)
			ipoput4(c->p->f, bp, 0, ih4->ttl, ih4->tos, nil);
		else
			ipoput6(c->p->f, bp, 0, ((Ip6hdr*)ih4)->ttl, 0, nil);
	}
}

static void
ipmuxiput(Proto *p, Ipifc *ifc, Block *bp)
{
	int len, hl;
	Fs *f = p->f;
	uchar *m, *h, *v, *e, *ve, *hp;
	Conv *c;
	Ipmux *mux;
	Myip4hdr *ip;
	Ip6hdr *ip6;

	ip = (Myip4hdr*)bp->rp;
	hl = (ip->vihl&0x0F)<<2;

	if(p->priv == nil)
		goto nomatch;

	h = bp->rp;
	len = BLEN(bp);

	/* run the v4 filter */
	rlock(f);
	c = nil;
	mux = f->ipmux->priv;
	while(mux != nil){
		if(mux->eoff > len){
			mux = mux->no;
			continue;
		}
		hp = h + mux->off + ((int)mux->skiphdr)*hl;
		switch(mux->ctype){
		case Cbyte:
			if(*mux->val == *hp)
				goto yes;
			break;
		case Cmbyte:
			if((*hp & *mux->mask) == *mux->val)
				goto yes;
			break;
		case Cshort:
			if(*((ushort*)mux->val) == *(ushort*)hp)
				goto yes;
			break;
		case Cmshort:
			if((*(ushort*)hp & (*((ushort*)mux->mask))) == *((ushort*)mux->val))
				goto yes;
			break;
		case Clong:
			if(*((ulong*)mux->val) == *(ulong*)hp)
				goto yes;
			break;
		case Cmlong:
			if((*(ulong*)hp & (*((ulong*)mux->mask))) == *((ulong*)mux->val))
				goto yes;
			break;
		case Cifc:
			if(*((ulong*)mux->val) == *(ulong*)(ifc->lifc->local + IPv4off))
				goto yes;
			break;
		case Cmifc:
			if((*(ulong*)(ifc->lifc->local + IPv4off) & (*((ulong*)mux->mask))) == *((ulong*)mux->val))
				goto yes;
			break;
		default:
			v = mux->val;
			for(e = mux->e; v < e; v = ve){
				m = mux->mask;
				hp = h + mux->off;
				for(ve = v + mux->len; v < ve; v++){
					if((*hp++ & *m++) != *v)
						break;
				}
				if(v == ve)
					goto yes;
			}
		}
		mux = mux->no;
		continue;
yes:
		if(mux->conv != nil)
			c = mux->conv;
		mux = mux->yes;
	}
	runlock(f);

	if(c != nil){
		/* tack on interface address */
		bp = padblock(bp, IPaddrlen);
		ipmove(bp->rp, ifc->lifc->local);
		bp = concatblock(bp);
		if(bp != nil)
			if(qpass(c->rq, bp) < 0)
				print("Q");
		return;
	}

nomatch:
	/* doesn't match any filter, hand it to the specific protocol handler */
	ip = (Myip4hdr*)bp->rp;
	if((ip->vihl & 0xF0) == IP_VER4) {
		p = f->t2p[ip->proto];
	} else {
		ip6 = (Ip6hdr*)bp->rp;
		p = f->t2p[ip6->proto];
	}
	if(p && p->rcv)
		(*p->rcv)(p, ifc, bp);
	else
		freeblist(bp);
	return;
}

static int
ipmuxsprint(Ipmux *mux, int level, char *buf, int len)
{
	int i, j, n;
	uchar *v;

	n = 0;
	for(i = 0; i < level; i++)
		n += snprint(buf+n, len-n, " ");
	if(mux == nil){
		n += snprint(buf+n, len-n, "\n");
		return n;
	}
	n += snprint(buf+n, len-n, "h[%d:%d]&", 
               mux->off+((int)mux->skiphdr)*((int)ipoff->data), 
               mux->off+(((int)mux->skiphdr)*((int)ipoff->data))+mux->len-1);
	for(i = 0; i < mux->len; i++)
		n += snprint(buf+n, len - n, "%2.2ux", mux->mask[i]);
	n += snprint(buf+n, len-n, "=");
	v = mux->val;
	for(j = 0; j < mux->n; j++){
		for(i = 0; i < mux->len; i++)
			n += snprint(buf+n, len - n, "%2.2ux", *v++);
		n += snprint(buf+n, len-n, "|");
	}
	n += snprint(buf+n, len-n, "\n");
	level++;
	n += ipmuxsprint(mux->no, level, buf+n, len-n);
	n += ipmuxsprint(mux->yes, level, buf+n, len-n);
	return n;
}

static int
ipmuxstats(Proto *p, char *buf, int len)
{
	int n;
	Fs *f = p->f;

	rlock(f);
	n = ipmuxsprint(p->priv, 0, buf, len);
	runlock(f);

	return n;
}

void
ipmuxinit(Fs *f)
{
	Proto *ipmux;

	ipmux = smalloc(sizeof(Proto));
	ipmux->priv = nil;
	ipmux->name = "ipmux";
	ipmux->connect = ipmuxconnect;
	ipmux->announce = ipmuxannounce;
	ipmux->state = ipmuxstate;
	ipmux->create = ipmuxcreate;
	ipmux->close = ipmuxclose;
	ipmux->rcv = ipmuxiput;
	ipmux->ctl = nil;
	ipmux->advise = nil;
	ipmux->stats = ipmuxstats;
	ipmux->ipproto = -1;
	ipmux->nc = 64;
	ipmux->ptclsize = sizeof(Ipmuxrock);

	f->ipmux = ipmux;			/* hack for Fsrcvpcol */

	Fsproto(f, ipmux);
}

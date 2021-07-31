#include	"all.h"

static	char*	abits;
static	long	sizabits;
static	char*	qbits;
static	long	sizqbits;
static	char*	name;
static	long	sizname;
static	long	fstart;
static	long	fsize;
static	long	nfiles;
static	long	maxq;
static	char*	calloc;
static	Device*	dev;
static	long	ndup;
static	long	nused;
static	long	nfdup;
static	long	nqbad;
static	long	nfree;
static	long	nbad;
static	int	mod;
static	int	flags;
static	int	ronly;
static	int	cwflag;
static	long	sbaddr;
static	long	oldblock;
static	int	depth;
static	int	maxdepth;

/* local prototypes */
static	int	fsck(Dentry*);
static	void	ckfreelist(Superb*);
static	void	mkfreelist(Superb*);
static	void	trfreelist(Superb*);
static	void	xaddfree(Device*, long, Superb*, Iobuf*);
static	void	xflush(Device*, Superb*, Iobuf*);
static	Dentry*	maked(long, int, long);
static	void	modd(long, int, Dentry*);
static	void	xread(long, long);
static	int	amark(long);
static	int	fmark(long);
static	int	ftest(long);
static	void	missing(void);
static	void	qmark(long);
static	void*	malloc(ulong);
static	Iobuf*	xtag(long, int, long);

static
void*
malloc(ulong n)
{
	char *p, *q;

	p = calloc;
	while((ulong)p & 3)
		p++;
	q = p+n;
	if(((ulong)q&0x0fffffffL) >= conf.mem)
		panic("check: mem size");
	calloc = q;
	memset(p, 0, n);
	return p;
}

/*
 * check flags
 */
enum
{
	Crdall	= (1<<0),	/* read all files */
	Ctag	= (1<<1),	/* rebuild tags */
	Cpfile	= (1<<2),	/* print files */
	Cpdir	= (1<<3),	/* print directories */
	Cfree	= (1<<4),	/* rebuild free list */
	Csetqid	= (1<<5),	/* resequence qids */
	Cream	= (1<<6),	/* clear all bad tags */
	Cbad	= (1<<7),	/* clear all bad blocks */
	Ctouch	= (1<<8),	/* touch old dir and indir */
};

static
struct
{
	char*	option;
	long	flag;
} ckoption[] =
{
	"rdall",	Crdall,
	"tag",		Ctag,
	"pfile",	Cpfile,
	"pdir",		Cpdir,
	"free",		Cfree,
	"setqid",	Csetqid,
	"ream",		Cream,
	"bad",		Cbad,
	"touch",	Ctouch,
	0,
};

void
cmd_check(int argc, char *argv[])
{
	long f, i;
	Filsys *fs;
	Iobuf *p;
	Superb *sb;
	Dentry *d;
	long raddr, flag;

	flag = 0;
	for(i=1; i<argc; i++) {
		for(f=0; ckoption[f].option; f++)
			if(strcmp(argv[i], ckoption[f].option) == 0)
				goto found;
		print("unknown check option %s\n", argv[i]);
		for(f=0; ckoption[f].option; f++)
			print("	%s\n", ckoption[f].option);
		return;
	found:
		flag |= ckoption[f].flag;
	}
	fs = cons.curfs;

	dev = fs->dev;
	ronly = (dev->type == Devro);
	cwflag = (dev->type == Devcw) | (dev->type == Devro);
	if(!ronly)
		wlock(&mainlock);		/* check */
	calloc = (char*)ialloc(0, 1) + 100000;
	flags = flag;

	sizqbits = ((1<<22) + 7) / 8;		/* botch */
	qbits = malloc(sizqbits);

	sbaddr = superaddr(dev);
	raddr = getraddr(dev);
	p = xtag(sbaddr, Tsuper, QPSUPER);
	if(!p)
		goto out;
	sb = (Superb*)p->iobuf;
	fstart = 2;
	cons.noage = 1;

	fsize = sb->fsize;
	sizabits = (fsize-fstart + 7)/8;
	abits = malloc(sizabits);

	sizname = 4000;
	name = malloc(sizname);
	sizname -= NAMELEN+10;	/* for safety */

	mod = 0;
	nfree = 0;
	nfdup = 0;
	nused = 0;
	nbad = 0;
	ndup = 0;
	nqbad = 0;
	depth = 0;
	maxdepth = 0;

	if(flags & Ctouch) {
		oldblock = fsize/DSIZE;
		oldblock *= DSIZE;
		if(oldblock < 0)
			oldblock = 0;
		print("oldblock = %ld\n", oldblock);
	}
	amark(sbaddr);
	if(cwflag) {
		amark(sb->roraddr);
		amark(sb->next);
	}

	print("checking filsys: %s\n", fs->name);
	nfiles = 0;
	maxq = 0;

	d = maked(raddr, 0, QPROOT);
	if(d) {
		amark(raddr);
		if(fsck(d))
			modd(raddr, 0, d);
		depth--;
		calloc -= sizeof(Dentry);
		if(depth)
			print("depth not zero on return\n");
	}

	if(flags & Cfree) {
		if(cwflag)
			trfreelist(sb);
		else
			mkfreelist(sb);
	}

	if(sb->qidgen < maxq)
		print("qid generator low path=%ld maxq=%ld\n",
			sb->qidgen, maxq);
	if(!(flags & Cfree))
		ckfreelist(sb);
	if(mod) {
		sb->qidgen = maxq;
		print("file system was modified\n");
		settag(p, Tsuper, QPNONE);
	}

	print("nfiles = %ld\n", nfiles);
	print("fsize  = %ld\n", fsize);
	print("nused  = %ld\n", nused);
	print("ndup   = %ld\n", ndup);
	print("nfree  = %ld\n", nfree);
	print("tfree  = %ld\n", sb->tfree);
	print("nfdup  = %ld\n", nfdup);
	print("nmiss  = %ld\n", fsize-fstart-nused-nfree);
	print("nbad   = %ld\n", nbad);
	print("nqbad  = %ld\n", nqbad);
	print("maxq   = %ld\n", maxq);
	if(!cwflag)
		missing();

out:
	cons.noage = 0;
	putbuf(p);
	if(!ronly)
		wunlock(&mainlock);
}

static
int
fsck(Dentry *d)
{
	Dentry *nd;
	Iobuf *p1, *p2, *pd;
	int i, j, k, ns, dmod;
	long a, qpath;

	depth++;
	if(depth >= maxdepth) {
		maxdepth = depth;
		/*
		 * On a 386 each recursion costs 72 bytes or thereabouts,
		 * for some slop bump it up to 100.
		 * Alternatives here might be to give the check process
		 * a much bigger stack or rewrite it without recursion.
		 */
		if(maxdepth >= MAXSTACK/100) {
			print("max depth exceeded: %s\n", name);
			return 0;
		}
	}
	dmod = 0;
	if(!(d->mode & DALLOC))
		goto out;
	nfiles++;

	ns = strlen(name);
	i = strlen(d->name);
	if(i >= NAMELEN) {
		d->name[NAMELEN-1] = 0;
		print("%s->name (%s) not terminated\n", name, d->name);
		return 0;
	}
	ns += i;
	if(ns >= sizname) {
		print("%s->name (%s) name too large\n", name, d->name);
		return 0;
	}
	strcat(name, d->name);

	if(d->mode & DDIR) {
		if(ns > 1) {
			strcat(name, "/");
			ns++;
		}
		if(flags & Cpdir) {
			print("%s\n", name);
			prflush();
		}
	} else
	if(flags & Cpfile) {
		print("%s\n", name);
		prflush();
	}

	qpath = d->qid.path & ~QPDIR;
	qmark(qpath);
	if(qpath > maxq)
		maxq = qpath;
	for(i=0; i<NDBLOCK; i++) {
		a = d->dblock[i];
		if(amark(a)) {
			if(flags & Cbad) {
				d->dblock[i] = 0;
				dmod++;
			}
			a = 0;
		}
		if(!a)
			continue;
		if(d->mode & DDIR) {
			if((flags&Ctouch) && a < oldblock) {
				pd = getbuf(dev, a, Bread|Bmod);
				if(pd)
					putbuf(pd);
				dmod++;
			}
			for(k=0; k<DIRPERBUF; k++) {
				nd = maked(a, k, qpath);
				if(!nd)
					break;
				if(fsck(nd)) {
					modd(a, k, nd);
					dmod++;
				}
				depth--;
				calloc -= sizeof(Dentry);
				name[ns] = 0;
			}
			continue;
		}
		if(flags & Crdall)
			xread(a, qpath);
	}
	a = d->iblock;
	if(amark(a)) {
		if(flags & Cbad) {
			d->iblock = 0;
			dmod++;
		}
		a = 0;
	}
	if(a) {
		if((flags&Ctouch) && a < oldblock) {
			pd = getbuf(dev, a, Bread|Bmod);
			if(pd)
				putbuf(pd);
			dmod++;
		}
		if(p1 = xtag(a, Tind1, qpath))
		for(i=0; i<INDPERBUF; i++) {
			a = ((long*)p1->iobuf)[i];
			if(amark(a)) {
				if(flags & Cbad) {
					((long*)p1->iobuf)[i] = 0;
					p1->flags |= Bmod;
				}
				a = 0;
			}
			if(!a)
				continue;
			if(d->mode & DDIR) {
				if((flags&Ctouch) && a < oldblock) {
					pd = getbuf(dev, a, Bread|Bmod);
					if(pd)
						putbuf(pd);
					dmod++;
				}
				for(k=0; k<DIRPERBUF; k++) {
					nd = maked(a, k, qpath);
					if(!nd)
						break;
					if(fsck(nd)) {
						modd(a, k, nd);
						dmod++;
					}
					depth--;
					calloc -= sizeof(Dentry);
					name[ns] = 0;
				}
				continue;
			}
			if(flags & Crdall)
				xread(a, qpath);
		}
		if(p1)
			putbuf(p1);
	}
	a = d->diblock;
	if(amark(a)) {
		if(flags & Cbad) {
			d->diblock = 0;
			dmod++;
		}
		a = 0;
	}
	if((flags&Ctouch) && a && a < oldblock) {
		pd = getbuf(dev, a, Bread|Bmod);
		if(pd)
			putbuf(pd);
		dmod++;
	}
	if(p2 = xtag(a, Tind2, qpath))
	for(i=0; i<INDPERBUF; i++) {
		a = ((long*)p2->iobuf)[i];
		if(amark(a)) {
			if(flags & Cbad) {
				((long*)p2->iobuf)[i] = 0;
				p2->flags |= Bmod;
			}
			continue;
		}
		if((flags&Ctouch) && a && a < oldblock) {
			pd = getbuf(dev, a, Bread|Bmod);
			if(pd)
				putbuf(pd);
			dmod++;
		}
		if(p1 = xtag(a, Tind1, qpath))
		for(j=0; j<INDPERBUF; j++) {
			a = ((long*)p1->iobuf)[j];
			if(amark(a)) {
				if(flags & Cbad) {
					((long*)p1->iobuf)[j] = 0;
					p1->flags |= Bmod;
				}
				continue;
			}
			if(!a)
				continue;
			if(d->mode & DDIR) {
				if((flags&Ctouch) && a < oldblock) {
					pd = getbuf(dev, a, Bread|Bmod);
					if(pd)
						putbuf(pd);
					dmod++;
				}
				for(k=0; k<DIRPERBUF; k++) {
					nd = maked(a, k, qpath);
					if(!nd)
						break;
					if(fsck(nd)) {
						modd(a, k, nd);
						dmod++;
					}
					depth--;
					calloc -= sizeof(Dentry);
					name[ns] = 0;
				}
				continue;
			}
			if(flags & Crdall)
				xread(a, qpath);
		}
		if(p1)
			putbuf(p1);
	}
	if(p2)
		putbuf(p2);
out:
	return dmod;
}

#define	XFEN	(FEPERBUF+6)
typedef
struct
{
	int	flag;
	int	count;
	int	next;
	long	addr[XFEN];
} Xfree;

static
void
xaddfree(Device *dev, long a, Superb *sb, Iobuf *p)
{
	Xfree *x;

	x = (Xfree*)p->iobuf;
	if(x->count < XFEN) {
		x->addr[x->count] = a;
		x->count++;
		return;
	}
	if(!x->flag) {
		memset(&sb->fbuf, 0, sizeof(sb->fbuf));
		sb->fbuf.free[0] = 0L;
		sb->fbuf.nfree = 1;
		sb->tfree = 0;
		x->flag = 1;
	}
	addfree(dev, a, sb);
}

static
void
xflush(Device *dev, Superb *sb, Iobuf *p)
{
	int i;
	Xfree *x;

	x = (Xfree*)p->iobuf;
	if(!x->flag) {
		memset(&sb->fbuf, 0, sizeof(sb->fbuf));
		sb->fbuf.free[0] = 0L;
		sb->fbuf.nfree = 1;
		sb->tfree = 0;
	}
	for(i=0; i<x->count; i++)
		addfree(dev, x->addr[i], sb);
}

/*
 * make freelist
 * from existing freelist
 * (cw devices)
 */
static
void
trfreelist(Superb *sb)
{
	long a;
	int n, i;
	Iobuf *p, *xp;
	Fbuf *fb;


	xp = getbuf(devnone, Cckbuf, 0);
	memset(xp->iobuf, 0, BUFSIZE);
	fb = &sb->fbuf;
	p = 0;
	for(;;) {
		n = fb->nfree;
		if(n < 0 || n > FEPERBUF)
			break;
		for(i=1; i<n; i++) {
			a = fb->free[i];
			if(a && !ftest(a))
				xaddfree(dev, a, sb, xp);
		}
		a = fb->free[0];
		if(!a)
			break;
		if(ftest(a))
			break;
		xaddfree(dev, a, sb, xp);
		if(p)
			putbuf(p);
		p = xtag(a, Tfree, QPNONE);
		if(!p)
			break;
		fb = (Fbuf*)p->iobuf;
	}
	if(p)
		putbuf(p);
	xflush(dev, sb, xp);
	putbuf(xp);
	mod++;
	print("%ld blocks free\n", sb->tfree);
}

static
void
ckfreelist(Superb *sb)
{
	long a, lo, hi;
	int n, i;
	Iobuf *p;
	Fbuf *fb;


	strcpy(name, "free list");
	print("check %s\n", name);
	fb = &sb->fbuf;
	a = sbaddr;
	p = 0;
	lo = 0;
	hi = 0;
	for(;;) {
		n = fb->nfree;
		if(n < 0 || n > FEPERBUF) {
			print("check: nfree bad %ld\n", a);
			break;
		}
		for(i=1; i<n; i++) {
			a = fb->free[i];
			if(a && !fmark(a)) {
				if(!lo || lo > a)
					lo = a;
				if(!hi || hi < a)
					hi = a;
			}
		}
		a = fb->free[0];
		if(!a)
			break;
		if(fmark(a))
			break;
		if(!lo || lo > a)
			lo = a;
		if(!hi || hi < a)
			hi = a;
		if(p)
			putbuf(p);
		p = xtag(a, Tfree, QPNONE);
		if(!p)
			break;
		fb = (Fbuf*)p->iobuf;
	}
	if(p)
		putbuf(p);
	print("lo = %ld; hi = %ld\n", lo, hi);
}

/*
 * make freelist from scratch
 */
static
void
mkfreelist(Superb *sb)
{
	long a;
	int i, b;

	if(ronly) {
		print("cant make freelist on ronly device\n");
		return;
	}
	strcpy(name, "free list");
	memset(&sb->fbuf, 0, sizeof(sb->fbuf));
	sb->fbuf.free[0] = 0L;
	sb->fbuf.nfree = 1;
	sb->tfree = 0;
	for(a=fsize-fstart-1; a >= 0; a--) {
		i = a/8;
		if(i < 0 || i >= sizabits)
			continue;
		b = 1 << (a&7);
		if(abits[i] & b)
			continue;
		addfree(dev, fstart+a, sb);
	}
	print("%ld blocks free\n", sb->tfree);
	mod++;
}

static
Dentry*
maked(long a, int s, long qpath)
{
	Iobuf *p;
	Dentry *d, *d1;

	p = xtag(a, Tdir, qpath);
	if(!p)
		return 0;
	d = getdir(p, s);
	d1 = malloc(sizeof(Dentry));
	memmove(d1, d, sizeof(Dentry));
	putbuf(p);
	return d1;
}

static
void
modd(long a, int s, Dentry *d1)
{
	Iobuf *p;
	Dentry *d;

	if(!(flags & Cbad))
		return;
	p = getbuf(dev, a, Bread);
	d = getdir(p, s);
	if(!d) {
		if(p)
			putbuf(p);
		return;
	}
	memmove(d, d1, sizeof(Dentry));
	p->flags |= Bmod;
	putbuf(p);
}

static
void
xread(long a, long qpath)
{
	Iobuf *p;

	p = xtag(a, Tfile, qpath);
	if(p)
		putbuf(p);
}

static
Iobuf*
xtag(long a, int tag, long qpath)
{
	Iobuf *p;

	if(a == 0)
		return 0;
	p = getbuf(dev, a, Bread);
	if(!p) {
		print("check: \"%s\": xtag: p null\n", name);
		if(flags & (Cream|Ctag)) {
			p = getbuf(dev, a, Bmod);
			if(p) {
				memset(p->iobuf, 0, RBUFSIZE);
				settag(p, tag, qpath);
				mod++;
				return p;
			}
		}
		return 0;
	}
	if(checktag(p, tag, qpath)) {
		print("check: \"%s\": xtag: checktag\n", name);
		if(flags & (Cream|Ctag)) {
			if(flags & Cream)
				memset(p->iobuf, 0, RBUFSIZE);
			settag(p, tag, qpath);
			mod++;
			return p;
		}
		return p;
	}
	return p;
}

static
int
amark(long a)
{
	long i;
	int b;

	if(a < fstart || a >= fsize) {
		if(a == 0)
			return 0;
		print("check: \"%s\": range %ld\n",
			name, a);
		nbad++;
		return 1;
	}
	a -= fstart;
	i = a/8;
	b = 1 << (a&7);
	if(abits[i] & b) {
		if(!ronly) {
			if(ndup < 10)
				print("check: \"%s\": address dup %ld\n",
					name, fstart+a);
			else
			if(ndup == 10)
				print("...");
		}
		ndup++;
		return 1;
	}
	abits[i] |= b;
	nused++;
	return 0;
}

static
int
fmark(long a)
{
	long i;
	int b;

	if(a < fstart || a >= fsize) {
		print("check: \"%s\": range %ld\n",
			name, a);
		nbad++;
		return 1;
	}
	a -= fstart;
	i = a/8;
	b = 1 << (a&7);
	if(abits[i] & b) {
		print("check: \"%s\": address dup %ld\n",
			name, fstart+a);
		nfdup++;
		return 1;
	}
	abits[i] |= b;
	nfree++;
	return 0;
}

static
int
ftest(long a)
{
	long i;
	int b;

	if(a < fstart || a >= fsize)
		return 1;
	a -= fstart;
	i = a/8;
	b = 1 << (a&7);
	if(abits[i] & b)
		return 1;
	abits[i] |= b;
	return 0;
}

static
void
missing(void)
{
	long a, i;
	int b, n;

	n = 0;
	for(a=fsize-fstart-1; a>=0; a--) {
		i = a/8;
		b = 1 << (a&7);
		if(!(abits[i] & b)) {
			print("missing: %ld\n", fstart+a);
			n++;
		}
		if(n > 10) {
			print(" ...\n");
			break;
		}
	}
}

static
void
qmark(long qpath)
{
	int i, b;

	i = qpath/8;
	b = 1 << (qpath&7);
	if(i < 0 || i >= sizqbits) {
		nqbad++;
		if(nqbad < 20)
			print("check: \"%s\": qid out of range %lux\n",
				name, qpath);
		return;
	}
	if((qbits[i] & b) && !ronly) {
		nqbad++;
		if(nqbad < 20)
			print("check: \"%s\": qid dup %lux\n",
				name, qpath);
	}
	qbits[i] |= b;
}

#include	"all.h"

/* augmented Dentry */
typedef struct {
	Dentry	*d;
	Off	qpath;
	int	ns;
} Extdentry;

static	char*	abits;
static	long	sizabits;
static	char*	qbits;
static	long	sizqbits;

static	char*	name;
static	long	sizname;

static	Off	fstart;
static	Off	fsize;
static	Off	nfiles;
static	Off	maxq;
static	Device*	dev;
static	Off	ndup;
static	Off	nused;
static	Off	nfdup;
static	Off	nqbad;
static	Off	nfree;
static	Off	nbad;
static	int	mod;
static	int	flags;
static	int	ronly;
static	int	cwflag;
static	Devsize	sbaddr;
static	Devsize	oldblock;

static	int	depth;
static	int	maxdepth;
static	uchar	*lowstack, *startstack;

/* local prototypes */
static	int	amark(Off);
static	void*	chkalloc(ulong);
static	void	ckfreelist(Superb*);
static	int	fmark(Off);
static	int	fsck(Dentry*);
static	int	ftest(Off);
static	Dentry*	maked(Off, int, Off);
static	void	missing(void);
static	void	mkfreelist(Superb*);
static	void	modd(Off, int, Dentry*);
static	void	qmark(Off);
static	void	trfreelist(Superb*);
static	void	xaddfree(Device*, Off, Superb*, Iobuf*);
static	void	xflush(Device*, Superb*, Iobuf*);
static	void	xread(Off, Off);
static	Iobuf*	xtag(Off, int, Off);

static void *
chkalloc(ulong n)
{
	char *p = mallocz(n, 1);

	if (p == nil)
		panic("chkalloc: out of memory");
	return p;
}

void
chkfree(void *p)
{
	free(p);
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
//	Csetqid	= (1<<5),	/* resequence qids */
	Cream	= (1<<6),	/* clear all bad tags */
	Cbad	= (1<<7),	/* clear all bad blocks */
	Ctouch	= (1<<8),	/* touch old dir and indir */
	Ctrim	= (1<<9),   /* trim fsize back to fit when checking free list */
};

static struct {
	char*	option;
	long	flag;
} ckoption[] = {
	"rdall",	Crdall,
	"tag",		Ctag,
	"pfile",	Cpfile,
	"pdir",		Cpdir,
	"free",		Cfree,
//	"setqid",	Csetqid,
	"ream",		Cream,
	"bad",		Cbad,
	"touch",	Ctouch,
	"trim",		Ctrim,
	0,
};

void
cmd_check(int argc, char *argv[])
{
	long f, i, flag;
	Off raddr;
	Filsys *fs;
	Iobuf *p;
	Superb *sb;
	Dentry *d;

	flag = 0;
	for(i=1; i<argc; i++) {
		for(f=0; ckoption[f].option; f++)
			if(strcmp(argv[i], ckoption[f].option) == 0)
				goto found;
		print("unknown check option %s\n", argv[i]);
		for(f=0; ckoption[f].option; f++)
			print("\t%s\n", ckoption[f].option);
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
	flags = flag;

	name = abits = qbits = nil;		/* in case of goto */
	sbaddr = superaddr(dev);
	raddr = getraddr(dev);
	p = xtag(sbaddr, Tsuper, QPSUPER);
	if(!p)
		goto out;
	sb = (Superb*)p->iobuf;
	fstart = 2;
	cons.noage = 1;

	/* 200 is slop since qidgen is likely to be a little bit low */
	sizqbits = (sb->qidgen+200 + 7) / 8;
	qbits = chkalloc(sizqbits);

	fsize = sb->fsize;
	sizabits = (fsize-fstart + 7)/8;
	abits = chkalloc(sizabits);

	sizname = 4000;
	name = chkalloc(sizname);
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
		/* round fsize down to start of current side */
		int s;
		Devsize dsize;

		oldblock = 0;
		for (s = 0; dsize = wormsizeside(dev, s),
		     dsize > 0 && oldblock + dsize < fsize; s++)
			oldblock += dsize;
		print("oldblock = %lld\n", (Wideoff)oldblock);
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
		chkfree(d);
		depth--;
		if(depth)
			print("depth not zero on return\n");
	}

	if(flags & Cfree)
		if(cwflag)
			trfreelist(sb);
		else
			mkfreelist(sb);

	if(sb->qidgen < maxq)
		print("qid generator low path=%lld maxq=%lld\n",
			(Wideoff)sb->qidgen, (Wideoff)maxq);
	if(!(flags & Cfree))
		ckfreelist(sb);
	if(mod) {
		sb->qidgen = maxq;
		print("file system was modified\n");
		settag(p, Tsuper, QPNONE);
	}

	print("nfiles = %lld\n", (Wideoff)nfiles);
	print("fsize  = %lld\n", (Wideoff)fsize);
	print("nused  = %lld\n", (Wideoff)nused);
	print("ndup   = %lld\n", (Wideoff)ndup);
	print("nfree  = %lld\n", (Wideoff)nfree);
	print("tfree  = %lld\n", (Wideoff)sb->tfree);
	print("nfdup  = %lld\n", (Wideoff)nfdup);
	print("nmiss  = %lld\n", (Wideoff)fsize-fstart-nused-nfree);
	print("nbad   = %lld\n", (Wideoff)nbad);
	print("nqbad  = %lld\n", (Wideoff)nqbad);
	print("maxq   = %lld\n", (Wideoff)maxq);
	print("base stack=%llud\n", (vlong)startstack);
	/* high-water mark of stack usage */
	print("high stack=%llud\n", (vlong)lowstack);
	print("deepest recursion=%d\n", maxdepth-1);	/* one-origin */
	if(!cwflag)
		missing();

out:
	cons.noage = 0;
	putbuf(p);
	chkfree(name);
	chkfree(abits);
	chkfree(qbits);
	name = abits = qbits = nil;
	if(!ronly)
		wunlock(&mainlock);
}

/*
 * if *blkp is already allocated and Cbad is set, zero it.
 * returns *blkp if it's free, else 0.
 */
static Off
blkck(Off *blkp, int *flgp)
{
	Off a = *blkp;

	if(amark(a)) {
		if(flags & Cbad) {
			*blkp = 0;
			*flgp |= Bmod;
		}
		a = 0;
	}
	return a;
}

/*
 * if a block address within a Dentry, *blkp, is already allocated
 * and Cbad is set, zero it.
 * stores 0 into *resp if already allocated, else stores *blkp.
 * returns dmod count.
 */
static int
daddrck(Off *blkp, Off *resp)
{
	int dmod = 0;

	if(amark(*blkp)) {
		if(flags & Cbad) {
			*blkp = 0;
			dmod++;
		}
		*resp = 0;
	} else
		*resp = *blkp;
	return dmod;
}

/*
 * under Ctouch, read block `a' if it's in range.
 * returns dmod count.
 */
static int
touch(Off a)
{
	if((flags&Ctouch) && a < oldblock) {
		Iobuf *pd = getbuf(dev, a, Brd|Bmod);

		if(pd)
			putbuf(pd);
		return 1;
	}
	return 0;
}

/*
 * if d is a directory, touch it and check all its entries in block a.
 * if not, under Crdall, read a.
 * returns dmod count.
 */
static int
dirck(Extdentry *ed, Off a)
{
	int k, dmod = 0;

	if(ed->d->mode & DDIR) {
		dmod += touch(a);
		for(k=0; k<DIRPERBUF; k++) {
			Dentry *nd = maked(a, k, ed->qpath);

			if(nd == nil)
				break;
			if(fsck(nd)) {
				modd(a, k, nd);
				dmod++;
			}
			chkfree(nd);
			depth--;
			name[ed->ns] = 0;
		}
	} else if(flags & Crdall)
		xread(a, ed->qpath);
	return dmod;
}

/*
 * touch a, check a's tag for Tind1, Tind2, etc.
 * if the tag is right, validate each block number in the indirect block,
 * and check each block (mostly in case we are reading a huge directory).
 */
static int
indirck(Extdentry *ed, Off a, int tag)
{
	int i, dmod = 0;
	Iobuf *p1;

	if (a == 0)
		return dmod;
	dmod = touch(a);
	if (p1 = xtag(a, tag, ed->qpath)) {
		for(i=0; i<INDPERBUF; i++) {
			a = blkck(&((Off *)p1->iobuf)[i], &p1->flags);
			if (a)
				/*
				 * check each block named in this
				 * indirect(^n) block (a).
				 */
				if (tag == Tind1)
					dmod +=   dirck(ed, a);
				else
					dmod += indirck(ed, a, tag-1);
		}
		putbuf(p1);
	}
	return dmod;
}

static int
indiraddrck(Extdentry *ed, Off *indirp, int tag)
{
	int dmod;
	Off a;

	dmod = daddrck(indirp, &a);
	return dmod + indirck(ed, a, tag);
}

/* if result is true, *d was modified */
static int
fsck(Dentry *d)
{
	int i, dmod;
	Extdentry edent;

	depth++;
	if(depth >= maxdepth)
		maxdepth = depth;
	if (lowstack == nil)
		startstack = lowstack = (uchar *)&edent;
	/* more precise check, assumes downward-growing stack */
	if ((uchar *)&edent < lowstack)
		lowstack = (uchar *)&edent;

	/* check that entry is allocated */
	if(!(d->mode & DALLOC))
		return 0;
	nfiles++;

	/* we stash qpath & ns in an Extdentry for eventual use by dirck() */
	memset(&edent, 0, sizeof edent);
	edent.d = d;

	/* check name */
	edent.ns = strlen(name);
	i = strlen(d->name);
	if(i >= NAMELEN) {
		d->name[NAMELEN-1] = 0;
		print("%s->name (%s) not terminated\n", name, d->name);
		return 0;
	}
	edent.ns += i;
	if(edent.ns >= sizname) {
		print("%s->name (%s) name too large\n", name, d->name);
		return 0;
	}
	strcat(name, d->name);

	if(d->mode & DDIR) {
		if(edent.ns > 1) {
			strcat(name, "/");
			edent.ns++;
		}
		if(flags & Cpdir) {
			print("%s\n", name);
			prflush();
		}
	} else if(flags & Cpfile) {
		print("%s\n", name);
		prflush();
	}

	/* check qid */
	edent.qpath = d->qid.path & ~QPDIR;
	qmark(edent.qpath);
	if(edent.qpath > maxq)
		maxq = edent.qpath;

	/* check direct blocks (the common case) */
	dmod = 0;
	{
		Off a;

		for(i=0; i<NDBLOCK; i++) {
			dmod += daddrck(&d->dblock[i], &a);
			if (a)
				dmod += dirck(&edent, a);
		}
	}
	/* check indirect^n blocks, if any */
	for (i = 0; i < NIBLOCK; i++)
		dmod += indiraddrck(&edent, &d->iblocks[i], Tind1+i);
	return dmod;
}

enum { XFEN = FEPERBUF + 6 };

typedef struct {
	int	flag;
	int	count;
	int	next;
	Off	addr[XFEN];
} Xfree;

static void
xaddfree(Device *dev, Off a, Superb *sb, Iobuf *p)
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

static void
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
static void
trfreelist(Superb *sb)
{
	Off a, n;
	int i;
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
	print("%lld blocks free\n", (Wideoff)sb->tfree);
}

static void
ckfreelist(Superb *sb)
{
	Off a, lo, hi;
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
			print("check: nfree bad %lld\n", (Wideoff)a);
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
	if (flags & Ctrim) {
		fsize = hi--;		/* fsize = hi + 1 */
		sb->fsize = fsize;
		mod++;
		print("set fsize to %lld\n", (Wideoff)fsize);
	}
	print("lo = %lld; hi = %lld\n", (Wideoff)lo, (Wideoff)hi);
}

/*
 * make freelist from scratch
 */
static void
mkfreelist(Superb *sb)
{
	Off a;
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
	print("%lld blocks free\n", (Wideoff)sb->tfree);
	mod++;
}

static Dentry*
maked(Off a, int s, Off qpath)
{
	Iobuf *p;
	Dentry *d, *d1;

	p = xtag(a, Tdir, qpath);
	if(!p)
		return 0;
	d = getdir(p, s);
	d1 = chkalloc(sizeof(Dentry));
	memmove(d1, d, sizeof(Dentry));
	putbuf(p);
	return d1;
}

static void
modd(Off a, int s, Dentry *d1)
{
	Iobuf *p;
	Dentry *d;

	if(!(flags & Cbad))
		return;
	p = getbuf(dev, a, Brd);
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

static void
xread(Off a, Off qpath)
{
	Iobuf *p;

	p = xtag(a, Tfile, qpath);
	if(p)
		putbuf(p);
}

static Iobuf*
xtag(Off a, int tag, Off qpath)
{
	Iobuf *p;

	if(a == 0)
		return 0;
	p = getbuf(dev, a, Brd);
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

static int
amark(Off a)
{
	Off i;
	int b;

	if(a < fstart || a >= fsize) {
		if(a == 0)
			return 0;
		print("check: \"%s\": range %lld\n",
			name, (Wideoff)a);
		nbad++;
		return 1;
	}
	a -= fstart;
	i = a/8;
	b = 1 << (a&7);
	if(abits[i] & b) {
		if(!ronly)
			if(ndup < 10)
				print("check: \"%s\": address dup %lld\n",
					name, (Wideoff)fstart+a);
			else if(ndup == 10)
				print("...");
		ndup++;
		return 1;
	}
	abits[i] |= b;
	nused++;
	return 0;
}

static int
fmark(Off a)
{
	Off i;
	int b;

	if(a < fstart || a >= fsize) {
		print("check: \"%s\": range %lld\n",
			name, (Wideoff)a);
		nbad++;
		return 1;
	}
	a -= fstart;
	i = a/8;
	b = 1 << (a&7);
	if(abits[i] & b) {
		print("check: \"%s\": address dup %lld\n",
			name, (Wideoff)fstart+a);
		nfdup++;
		return 1;
	}
	abits[i] |= b;
	nfree++;
	return 0;
}

static int
ftest(Off a)
{
	Off i;
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

static void
missing(void)
{
	Off a, i;
	int b, n;

	n = 0;
	for(a=fsize-fstart-1; a>=0; a--) {
		i = a/8;
		b = 1 << (a&7);
		if(!(abits[i] & b)) {
			print("missing: %lld\n", (Wideoff)fstart+a);
			n++;
		}
		if(n > 10) {
			print(" ...\n");
			break;
		}
	}
}

static void
qmark(Off qpath)
{
	int b;
	Off i;

	i = qpath/8;
	b = 1 << (qpath&7);
	if(i < 0 || i >= sizqbits) {
		nqbad++;
		if(nqbad < 20)
			print("check: \"%s\": qid out of range %llux\n",
				name, (Wideoff)qpath);
		return;
	}
	if((qbits[i] & b) && !ronly) {
		nqbad++;
		if(nqbad < 20)
			print("check: \"%s\": qid dup %llux\n", name,
				(Wideoff)qpath);
	}
	qbits[i] |= b;
}

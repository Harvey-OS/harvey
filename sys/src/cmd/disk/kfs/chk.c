/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"all.h"

#define	DSIZE		546000
#define	MAXDEPTH	100

static	char*	abits;
static	int32_t	sizabits;
static	char*	qbits;
static	int32_t	sizqbits;
static	char*	name;
static	int32_t	sizname;
static	int32_t	fstart;
static	int32_t	fsize;
static	int32_t	nfiles;
static	int32_t	maxq;
static	char*	fence;
static	char*	fencebase;
static	Device	dev;
static	int32_t	ndup;
static	int32_t	nused;
static	int32_t	nfdup;
static	int32_t	nqbad;
static	int32_t	nfree;
static	int32_t	nbad;
static	int	mod;
static	int	flags;
static	int	ronly;
static	int	cwflag;
static	int32_t	sbaddr;
static	int32_t	oldblock;
static	int	depth;
static	int	maxdepth;

/* local prototypes */
static	int	fsck(Dentry*);
static	void	ckfreelist(Superb*);
static	void	mkfreelist(Superb*);
static	Dentry*	maked(int32_t, int, int32_t);
static	void	modd(int32_t, int, Dentry*);
static	void	xread(int32_t, int32_t);
static	int	amark(int32_t);
static	int	fmark(int32_t);
static	void	missing(void);
static	void	qmark(int32_t);
static	void*	zalloc(uint32_t);
static	void*	dalloc(uint32_t);
static	Iobuf*	xtag(int32_t, int, int32_t);

static
void*
zalloc(uint32_t n)
{
	char *p;

	p = malloc(n);
	if(p == 0)
		panic("zalloc: out of memory\n");
	memset(p, '\0', n);
	return p;
}

static
void*
dalloc(uint32_t n)
{
	char *p;

	if(fencebase == 0)
		fence = fencebase = zalloc(MAXDEPTH*sizeof(Dentry));
	p = fence;
	fence += n;
	if(fence > fencebase+MAXDEPTH*sizeof(Dentry))
		panic("dalloc too much memory\n");
	return p;
}

void
check(Filsys *fs, int32_t flag)
{
	Iobuf *p;
	Superb *sb;
	Dentry *d;
	int32_t raddr;
	int32_t nqid;

	wlock(&mainlock);
	dev = fs->dev;
	flags = flag;
	fence = fencebase;

	sizname = 4000;
	name = zalloc(sizname);
	sizname -= NAMELEN+10;	/* for safety */

	sbaddr = superaddr(dev);
	raddr = getraddr(dev);
	p = xtag(sbaddr, Tsuper, QPSUPER);
	if(!p){
		cprint("bad superblock\n");
		goto out;
	}
	sb = (Superb*)p->iobuf;
	fstart = 1;

	fsize = sb->fsize;
	sizabits = (fsize-fstart + 7)/8;
	abits = zalloc(sizabits);

	nqid = sb->qidgen+100;		/* not as much of a botch */
	if(nqid > 1024*1024*8)
		nqid = 1024*1024*8;
	if(nqid < 64*1024)
		nqid = 64*1024;

	sizqbits = (nqid+7)/8;
	qbits = zalloc(sizqbits);

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
		cprint("oldblock = %ld\n", oldblock);
	}
	if(amark(sbaddr))
		{}
	if(cwflag) {
		if(amark(sb->roraddr))
			{}
		if(amark(sb->next))
			{}
	}

	if(!(flags & Cquiet))
		cprint("checking file system: %s\n", fs->name);
	nfiles = 0;
	maxq = 0;

	d = maked(raddr, 0, QPROOT);
	if(d) {
		if(amark(raddr))
			{}
		if(fsck(d))
			modd(raddr, 0, d);
		depth--;
		fence -= sizeof(Dentry);
		if(depth)
			cprint("depth not zero on return\n");
	}

	if(flags & Cfree) {
		mkfreelist(sb);
		sb->qidgen = maxq;
		settag(p, Tsuper, QPNONE);
	}

	if(sb->qidgen < maxq)
		cprint("qid generator low path=%ld maxq=%ld\n",
			sb->qidgen, maxq);
	if(!(flags & Cfree))
		ckfreelist(sb);
	if(mod) {
		cprint("file system was modified\n");
		settag(p, Tsuper, QPNONE);
	}

	if(!(flags & Cquiet)){
		cprint("%8ld files\n", nfiles);
		cprint("%8ld blocks in the file system\n", fsize-fstart);
		cprint("%8ld used blocks\n", nused);
		cprint("%8ld free blocks\n", sb->tfree);
	}
	if(!(flags & Cfree)){
		if(nfree != sb->tfree)
			cprint("%8ld free blocks found\n", nfree);
		if(nfdup)
			cprint("%8ld blocks duplicated in the free list\n", nfdup);
		if(fsize-fstart-nused-nfree)
			cprint("%8ld missing blocks\n", fsize-fstart-nused-nfree);
	}
	if(ndup)
		cprint("%8ld address duplications\n", ndup);
	if(nbad)
		cprint("%8ld bad block addresses\n", nbad);
	if(nqbad)
		cprint("%8ld bad qids\n", nqbad);
	if(!(flags & Cquiet))
		cprint("%8ld maximum qid path\n", maxq);
	missing();

out:
	if(p)
		putbuf(p);
	free(abits);
	free(name);
	free(qbits);
	wunlock(&mainlock);
}

static
int
touch(int32_t a)
{
	Iobuf *p;

	if((flags&Ctouch) && a && a < oldblock){
		p = getbuf(dev, a, Bread|Bmod);
		if(p)
			putbuf(p);
		return 1;
	}
	return 0;
}

static
int
checkdir(int32_t a, int32_t qpath)
{
	Dentry *nd;
	int i, ns, dmod;

	ns = strlen(name);
	dmod = touch(a);
	for(i=0; i<DIRPERBUF; i++) {
		nd = maked(a, i, qpath);
		if(!nd)
			break;
		if(fsck(nd)) {
			modd(a, i, nd);
			dmod++;
		}
		depth--;
		fence -= sizeof(Dentry);
		name[ns] = 0;
	}
	name[ns] = 0;
	return dmod;
}

static
int
checkindir(int32_t a, Dentry *d, int32_t qpath)
{
	Iobuf *p;
	int i, dmod;

	dmod = touch(a);
	p = xtag(a, Tind1, qpath);
	if(!p)
		return dmod;
	for(i=0; i<INDPERBUF; i++) {
		a = ((int32_t*)p->iobuf)[i];
		if(!a)
			continue;
		if(amark(a)) {
			if(flags & Cbad) {
				((int32_t*)p->iobuf)[i] = 0;
				p->flags |= Bmod;
			}
			continue;
		}
		if(d->mode & DDIR)
			dmod += checkdir(a, qpath);
		else if(flags & Crdall)
			xread(a, qpath);
	}
	putbuf(p);
	return dmod;
}

static
int
fsck(Dentry *d)
{
	char *s;
	Rune r;
	Iobuf *p;
	int l, i, ns, dmod;
	int32_t a, qpath;

	depth++;
	if(depth >= maxdepth){
		maxdepth = depth;
		if(maxdepth >= MAXDEPTH){
			cprint("max depth exceeded: %s\n", name);
			return 0;
		}
	}
	dmod = 0;
	if(!(d->mode & DALLOC))
		return 0;
	nfiles++;

	ns = strlen(name);
	i = strlen(d->name);
	if(i >= NAMELEN){
		d->name[NAMELEN-1] = 0;
		cprint("%s->name (%s) not terminated\n", name, d->name);
		return 0;
	}
	ns += i;
	if(ns >= sizname){
		cprint("%s->name (%s) name too large\n", name, d->name);
		return 0;
	}
	for (s = d->name; *s; s += l){
		l = chartorune(&r, s);
		if (r == Runeerror)
			for (i = 0; i < l; i++){
				s[i] = '_';
				cprint("%s->name (%s) bad UTF\n", name, d->name);
				dmod++;
			}
	}
	strcat(name, d->name);

	if(d->mode & DDIR){
		if(ns > 1)
			strcat(name, "/");
		if(flags & Cpdir)
			cprint("%s\n", name);
	} else
	if(flags & Cpfile)
		cprint("%s\n", name);

	qpath = d->qid.path & ~QPDIR;
	qmark(qpath);
	if(qpath > maxq)
		maxq = qpath;
	for(i=0; i<NDBLOCK; i++) {
		a = d->dblock[i];
		if(!a)
			continue;
		if(amark(a)) {
			d->dblock[i] = 0;
			dmod++;
			continue;
		}
		if(d->mode & DDIR)
			dmod += checkdir(a, qpath);
		else if(flags & Crdall)
			xread(a, qpath);
	}
	a = d->iblock;
	if(a && amark(a)) {
		d->iblock = 0;
		dmod++;
	}
	else if(a)
		dmod += checkindir(a, d, qpath);

	a = d->diblock;
	if(a && amark(a)) {
		d->diblock = 0;
		return dmod + 1;
	}
	dmod += touch(a);
	if(p = xtag(a, Tind2, qpath)){
		for(i=0; i<INDPERBUF; i++){
			a = ((int32_t*)p->iobuf)[i];
			if(!a)
				continue;
			if(amark(a)) {
				if(flags & Cbad) {
					((int32_t*)p->iobuf)[i] = 0;
					p->flags |= Bmod;
				}
				continue;
			}
			dmod += checkindir(a, d, qpath);
		}
		putbuf(p);
	}
	return dmod;
}

static
void
ckfreelist(Superb *sb)
{
	int32_t a, lo, hi;
	int n, i;
	Iobuf *p;
	Fbuf *fb;


	strcpy(name, "free list");
	cprint("check %s\n", name);
	fb = &sb->fbuf;
	a = sbaddr;
	p = 0;
	lo = 0;
	hi = 0;
	for(;;) {
		n = fb->nfree;
		if(n < 0 || n > FEPERBUF) {
			cprint("check: nfree bad %ld\n", a);
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
	cprint("lo = %ld; hi = %ld\n", lo, hi);
}

/*
 * make freelist from scratch
 */
static
void
mkfreelist(Superb *sb)
{
	int32_t a;
	int i, b;

	strcpy(name, "free list");
	memset(&sb->fbuf, 0, sizeof(sb->fbuf));
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
		abits[i] |= b;
	}
}

static
Dentry*
maked(int32_t a, int s, int32_t qpath)
{
	Iobuf *p;
	Dentry *d, *d1;

	p = xtag(a, Tdir, qpath);
	if(!p)
		return 0;
	d = getdir(p, s);
	d1 = dalloc(sizeof(Dentry));
	memmove(d1, d, sizeof(Dentry));
	putbuf(p);
	return d1;
}

static
void
modd(int32_t a, int s, Dentry *d1)
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
xread(int32_t a, int32_t qpath)
{
	Iobuf *p;

	p = xtag(a, Tfile, qpath);
	if(p)
		putbuf(p);
}

static
Iobuf*
xtag(int32_t a, int tag, int32_t qpath)
{
	Iobuf *p;

	if(a == 0)
		return 0;
	p = getbuf(dev, a, Bread);
	if(!p) {
		cprint("check: \"%s\": xtag: p null\n", name);
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
		cprint("check: \"%s\": xtag: checktag\n", name);
		if(flags & Cream)
			memset(p->iobuf, 0, RBUFSIZE);
		if(flags & (Cream|Ctag)) {
			settag(p, tag, qpath);
			mod++;
		}
		return p;
	}
	return p;
}

static
int
amark(int32_t a)
{
	int32_t i;
	int b;

	if(a < fstart || a >= fsize) {
		cprint("check: \"%s\": range %ld\n",
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
				cprint("check: \"%s\": address dup %ld\n",
					name, fstart+a);
			else
			if(ndup == 10)
				cprint("...");
		}
		ndup++;
		return 0;	/* really?? */
	}
	abits[i] |= b;
	nused++;
	return 0;
}

static
int
fmark(int32_t a)
{
	int32_t i;
	int b;

	if(a < fstart || a >= fsize) {
		cprint("check: \"%s\": range %ld\n",
			name, a);
		nbad++;
		return 1;
	}
	a -= fstart;
	i = a/8;
	b = 1 << (a&7);
	if(abits[i] & b) {
		cprint("check: \"%s\": address dup %ld\n",
			name, fstart+a);
		nfdup++;
		return 1;
	}
	abits[i] |= b;
	nfree++;
	return 0;
}

static
void
missing(void)
{
	int32_t a, i;
	int b, n;

	n = 0;
	for(a=fsize-fstart-1; a>=0; a--) {
		i = a/8;
		b = 1 << (a&7);
		if(!(abits[i] & b)) {
			cprint("missing: %ld\n", fstart+a);
			n++;
		}
		if(n > 10) {
			cprint(" ...\n");
			break;
		}
	}
}

static
void
qmark(int32_t qpath)
{
	int i, b;

	i = qpath/8;
	b = 1 << (qpath&7);
	if(i < 0 || i >= sizqbits) {
		nqbad++;
		if(nqbad < 20)
			cprint("check: \"%s\": qid out of range %lx\n",
				name, qpath);
		return;
	}
	if((qbits[i] & b) && !ronly) {
		nqbad++;
		if(nqbad < 20)
			cprint("check: \"%s\": qid dup %lx\n",
				name, qpath);
	}
	qbits[i] |= b;
}

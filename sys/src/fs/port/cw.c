#include "all.h"
void	searchtag(Device d, int tag, long a);

#define	DEBUG		0
#define	RECOVCW		0		/* address of root to be recovered */
#define	RECOVRO		0		/* address of root to be recovered */

#define	FIRST		SUPER_ADDR
#define	ADDFREE		(100)
#define	CACHE_ADDR	SUPER_ADDR
#define	DSIZE		546000
#define	MAXAGE		10000
#define	BKPERBLK	10
#define CEPERBK		((BUFSIZE-BKPERBLK*sizeof(long))/\
				(sizeof(Centry)*BKPERBLK))

/* cache state */
enum
{
	/* states -- beware these are recorded on the cache */
				/*    cache    worm	*/
	Cnone = 0,		/*	0	?	*/
	Cdirty,			/*	1	0	*/
	Cdump,			/*	1	0->1	*/
	Cread,			/*	1	1	*/
	Cwrite,			/*	2	1	*/
	Cdump1,			/* inactive form of dump */
	Cerror,

	/* opcodes -- these are not recorded */
	Onone,
	Oread,
	Owrite,
	Ogrow,
	Odump,
	Orele,
	Ofree,
};

typedef	struct	Cache	Cache;
typedef	struct	Centry	Centry;
typedef	struct	Bucket	Bucket;

struct	Cache
{
	long	maddr;		/* cache map addr */
	long	msize;		/* cache map size in buckets */
	long	caddr;		/* cache addr */
	long	csize;		/* cache size */
	long	fsize;		/* current size of worm */
	long	wsize;		/* max size of the worm */
	long	wmax;		/* highwater write */

	long	sbaddr;		/* super block addr */
	long	cwraddr;	/* cw root addr */
	long	roraddr;	/* dump root addr */

	long	toytime;	/* somewhere convienent */
	long	time;
};

struct	Centry
{
	ushort	age;
	short	state;
	long	waddr;		/* worm addr */
};

struct	Bucket
{
	long	agegen;		/* generator for ages in this bkt */
	Centry	entry[CEPERBK];
};

static
struct
{
	Filter	ncwio;
	int	inited;
	int	dbucket;	/* last bucket dumped */
	long	daddr;		/* last block dumped */
	long	ncopy;
	int	nodump;
/*
 * following are cached variables for dumps
 */
	Device	dev;
	Device	cdev;
	Device	wdev;
	Device	rodev;
	long	fsize;
	long	ndump;
	int	depth;
	int	all;		/* local flag to recur on modified directories */
	int	allflag;	/* global flag to recur on modified directories */
	long	falsehits;	/* times recur found modified blocks */
	struct
	{
		char	name[500];
		char	namepad[NAMELEN+10];
	};
} cw;

char*	cwnames[] =
{
	[Cnone]		"none",
	[Cdirty]	"dirty",
	[Cdump]		"dump",
	[Cread]		"read",
	[Cwrite]	"write",
	[Cdump1]	"dump1",
	[Cerror]	"error",

	[Onone]		"none",
	[Oread]		"read",
	[Owrite]	"write",
	[Ogrow]		"grow",
	[Odump]		"dump",
	[Orele]		"rele",
};

Centry*	getcentry(Bucket*, long);
int	cwio(Device, long, void*, int);
void	cmd_cwcmd(int, char*[]);

/*
 * console command
 * initiate a dump
 */
void
cmd_dump(int argc, char *argv[])
{
	Filsys *fs;

	fs = cons.curfs;
	if(argc > 1)
		fs = fsstr(argv[1]);
	if(fs == 0) {
		print("%s: unknown file system\n", argv[1]);
		return;
	}
	cfsdump(fs);
}

/*
 * console command
 * worm stats
 */
static
void
cmd_statw(int argc, char *argv[])
{
	Filsys *fs;
	Iobuf *p;
	Superb *sb;
	Cache *h;
	Bucket *b;
	Centry *c, *ce;
	long m, nw, bw, state[Onone];
	long sbfsize, sbcwraddr, sbroraddr, sblast, sbnext;
	long hmsize, hmaddr;
	Device dev, cdev;
	int s;

	USED(argc);
	USED(argv);

	print("cwstats\n");
	print("	nio   = %F\n", (Filta){&cw.ncwio, 1});

	for(fs=filsys; fs->name; fs++) {
		dev = fs->dev;
		if(dev.type != Devcw)
			continue;

		print("	cache %s\n", fs->name);

		cdev = CDEV(dev);

		p = getbuf(dev, cwsaddr(dev), Bread);
		if(!p || checktag(p, Tsuper, QPSUPER)) {
			print("cwstats: checktag super\n");
			if(p)
				putbuf(p);
			sbfsize = 0;
			sbcwraddr = 0;
			sbroraddr = 0;
			sblast = 0;
			sbnext = 0;
		} else {
			sb = (Superb*)p->iobuf;
			sbfsize = sb->fsize;
			sbcwraddr = sb->cwraddr;
			sbroraddr = sb->roraddr;
			sblast = sb->last;
			sbnext = sb->next;
			putbuf(p);
		}

		p = getbuf(cdev, CACHE_ADDR, Bread|Bres);
		if(!p || checktag(p, Tcache, QPSUPER)) {
			print("cwstats: checktag c bucket\n");
			if(p)
				putbuf(p);
			continue;
		}
		h = (Cache*)p->iobuf;
		hmaddr = h->maddr;
		hmsize = h->msize;

		print("		maddr  = %8ld\n", hmaddr);
		print("		msize  = %8ld\n", hmsize);
		print("		caddr  = %8ld\n", h->caddr);
		print("		csize  = %8ld\n", h->csize);
		print("		sbaddr = %8ld\n", h->sbaddr);
		print("		craddr = %8ld %8ld\n", h->cwraddr, sbcwraddr);
		print("		roaddr = %8ld %8ld\n", h->roraddr, sbroraddr);
		print("		fsize  = %8ld %8ld %2ld+%2ld%%\n", h->fsize, sbfsize,
					h->fsize/DSIZE,
					(h->fsize%DSIZE)/(DSIZE/100));
		print("		slast  =          %8ld\n", sblast);
		print("		snext  =          %8ld\n", sbnext);
		print("		wmax   = %8ld          %2ld+%2ld%%\n", h->wmax,
					h->wmax/DSIZE,
					(h->wmax%DSIZE)/(DSIZE/100));
		print("		wsize  = %8ld          %2ld+%2ld%%\n", h->wsize,
					h->wsize/DSIZE,
					(h->wsize%DSIZE)/(DSIZE/100));
		putbuf(p);

		bw = 0;	/* max filled bucket */
		memset(state, 0, sizeof(state));
		for(m=0; m<hmsize; m++) {
			p = getbuf(cdev, hmaddr + m/BKPERBLK, Bread);
			if(!p || checktag(p, Tbuck, hmaddr + m/BKPERBLK))
				panic("cwstats: checktag c bucket");
			b = (Bucket*)p->iobuf + m%BKPERBLK;
			ce = b->entry + CEPERBK;
			nw = 0;
			for(c=b->entry; c<ce; c++) {
				s = c->state;
				state[s]++;
				if(s != Cnone && s != Cread)
					nw++;
			}
			putbuf(p);
			if(nw > bw)
				bw = nw;
		}
		for(s=Cnone; s<Cerror; s++)
			print("		%6ld %s\n", state[s], cwnames[s]);
		print("		cache %2ld%% full\n", (bw*100)/CEPERBK);
	}
}

int
dumpblock(Device dev)
{
	Device cdev, wdev;
	Iobuf *p, *cb, *p1, *p2;
	Cache *h;
	Centry *c, *ce, *bc;
	Bucket *b;
	long m, a, msize, maddr, wmax, caddr;
	int s1, s2, count;

	if(cw.nodump)
		return 0;

	cdev = CDEV(dev);
	cb = getbuf(cdev, CACHE_ADDR, Bread|Bres);
	h = (Cache*)cb->iobuf;
	msize = h->msize;
	maddr = h->maddr;
	wmax = h->wmax;
	caddr = h->caddr;
	putbuf(cb);

	for(m=msize; m>=0; m--) {
		a = cw.dbucket + 1;
		if(a < 0 || a >= msize)
			a = 0;
		cw.dbucket = a;
		p = getbuf(cdev, maddr + a/BKPERBLK, Bread);
		b = (Bucket*)p->iobuf + a%BKPERBLK;
		ce = b->entry + CEPERBK;
		bc = 0;
		for(c=b->entry; c<ce; c++)
			if(c->state == Cdump) {
				if(bc == 0) {
					bc = c;
					continue;
				}
				if(c->waddr < cw.daddr) {
					if(bc->waddr < cw.daddr && bc->waddr > c->waddr)
						bc = c;
					continue;
				}
				if(bc->waddr < cw.daddr || bc->waddr > c->waddr)
					bc = c;
			}
		if(bc) {
			c = bc;
			goto found;
		}
		putbuf(p);
	}
	if(cw.ncopy) {
		print("%ld blocks copied to worm\n", cw.ncopy);
		cw.ncopy = 0;
	}
	cw.nodump = 1;
	return 0;

found:
	a = a*CEPERBK + (c - b->entry) + caddr;
	p1 = getbuf(devnone, Cwdump1, 0);
	count = 0;

retry:
	count++;
	if(count > 10)
		goto stop;
	if(devread(cdev, a, p1->iobuf))
		goto stop;
	m = c->waddr;
	cw.daddr = m;
	wdev = WDEV(dev);
	s1 = devwrite(wdev, m, p1->iobuf);
	if(s1) {
		p2 = getbuf(devnone, Cwdump2, 0);
		s2 = devread(wdev, m, p2->iobuf);
		if(s2) {
			if(s1 == 0x61 && s2 == 0x60) {
				putbuf(p2);
				goto retry;
			}
			goto stop1;
		}
		if(memcmp(p1->iobuf, p2->iobuf, RBUFSIZE))
			goto stop1;
		putbuf(p2);
	}
	/*
	 * reread and compare
	 */
	p2 = getbuf(devnone, Cwdump2, 0);
	s1 = devread(wdev, m, p2->iobuf);
	if(s1)
		goto stop1;
	if(memcmp(p1->iobuf, p2->iobuf, RBUFSIZE)) {
		print("reread C%ld W%ld didnt compare\n", a, m);
		goto stop1;
	}
	putbuf(p2);

	putbuf(p1);
	c->state = Cread;
	p->flags |= Bmod;
	putbuf(p);

	if(m > wmax) {
		cb = getbuf(cdev, CACHE_ADDR, Bread|Bmod|Bres);
		h = (Cache*)cb->iobuf;
		if(m > h->wmax)
			h->wmax = m;
		putbuf(cb);
	}
	cw.ncopy++;
	return 1;

stop1:
	putbuf(p2);
	putbuf(p1);
	c->state = Cdump1;
	p->flags |= Bmod;
	putbuf(p);
	return 1;

stop:
	putbuf(p1);
	putbuf(p);
	print("stopping dump!!\n");
	cw.nodump = 1;
	return 0;
}

void
cwinit(Device dev)
{
	Cache *h;
	Iobuf *cb, *p;
	long l, m;
	Device cdev;

	if(!cw.inited) {
		cw.inited = 1;
		cw.allflag = 0;
		dofilter(&cw.ncwio);
		cmd_install("dump", "-- make dump backup to worm", cmd_dump);
		cmd_install("statw", "-- cache/worm stats", cmd_statw);
		cmd_install("cwcmd", "subcommand -- cache/worm errata", cmd_cwcmd);

		roflag = flag_install("ro", "-- ro reads and writes");
	}
	cdev = CDEV(dev);
	devinit(cdev);
	devinit(WDEV(dev));
	l = devsize(WDEV(dev));

	cb = getbuf(cdev, CACHE_ADDR, Bread|Bmod|Bres);
	h = (Cache*)cb->iobuf;
	h->toytime = toytime() + SECOND(30);
	h->time = time();
	m = h->wsize;
	if(l != m) {
		print("wdev changed size %ld to %ld\n", m, l);
		h->wsize = l;
		cb->flags |= Bmod;
	}

	for(m=0; m<h->msize; m++) {
		p = getbuf(cdev, h->maddr + m/BKPERBLK, Bread);
		if(!p || checktag(p, Tbuck, h->maddr + m/BKPERBLK))
			panic("cwinit: checktag c bucket");
		putbuf(p);
	}

	putbuf(cb);
}

long
cwsaddr(Device dev)
{
	Iobuf *cb;
	long sa;

	cb = getbuf(CDEV(dev), CACHE_ADDR, Bread|Bres);
	sa = ((Cache*)cb->iobuf)->sbaddr;
	putbuf(cb);
	return sa;
}

long
cwraddr(Device dev, int flag)
{
	Iobuf *cb;
	long ra;

	cb = getbuf(CDEV(dev), CACHE_ADDR, Bread|Bres);
	if(flag)
		ra = ((Cache*)cb->iobuf)->roraddr;
	else
		ra = ((Cache*)cb->iobuf)->cwraddr;
	putbuf(cb);
	return ra;
}

long
cwsize(Device dev)
{
	Iobuf *cb;
	long fs;

	cb = getbuf(CDEV(dev), CACHE_ADDR, Bread|Bres);
	fs = ((Cache*)cb->iobuf)->fsize;
	putbuf(cb);
	return fs;
}

int
cwread(Device a, long b, void *c)
{
	return cwio(a, b, c, Oread) == Cerror;
}

int
cwwrite(Device a, long b, void *c)
{
	return cwio(a, b, c, Owrite) == Cerror;
}

int
roread(Device a, long b, void *c)
{
	Device d;
	int s;

	d = a;
	d.type = Devcw;

	/*
	 * maybe better is to try buffer pool first
	 */
	s = cwio(d, b, 0, Onone);
	if(s == Cdump || s == Cdump1 || s == Cread) {
		s = cwio(d, b, c, Oread);
		if(s == Cdump || s == Cdump1 || s == Cread) {
			if(cons.flags & roflag)
				print("roread: %D %ld -> %D(hit)\n", a, b, d);
			return 0;
		}
	}
	if(cons.flags & roflag)
		print("roread: %D %ld -> %D(miss)\n", a, b, WDEV(d));
	return devread(WDEV(d), b, c);
}

int
cwio(Device dev, long addr, void *buf, int opcode)
{
	Iobuf *p, *p1, *p2, *cb;
	Cache *h;
	Bucket *b;
	Centry *c;
	Device cdev, wdev;
	long bn, a1, a2, max, newmax;
	int state;

	cw.ncwio.count++;
	cdev = CDEV(dev);
	wdev = WDEV(dev);

	cb = getbuf(cdev, CACHE_ADDR, Bread|Bres);
	h = (Cache*)cb->iobuf;
	if(toytime() >= h->toytime) {
		cb->flags |= Bmod;
		h->toytime = toytime() + SECOND(30);
		h->time = time();
	}

	if(addr < 0) {
		putbuf(cb);
		return Cerror;
	}
/*
	if(addr >= h->fsize)
		print("cwio large address %ld\n", addr);
*/
	bn = addr % h->msize;
	a1 = h->maddr + bn/BKPERBLK;
	a2 = bn*CEPERBK + h->caddr;
	max = h->wmax;

	putbuf(cb);
	newmax = 0;

	p = getbuf(cdev, a1, Bread|Bmod);
	if(!p || checktag(p, Tbuck, a1))
		panic("cwio: checktag c bucket");
	b = (Bucket*)p->iobuf + bn%BKPERBLK;

	c = getcentry(b, addr);
	if(c == 0) {
		putbuf(p);
		print("disk cache bucket %ld is full\n", a1);
		return Cerror;
	}
	a2 += c - b->entry;

	state = c->state;
	switch(opcode)
	{
	default:
		goto bad;

	case Onone:
		break;

	case Oread:
		switch(state) {
		default:
			goto bad;

		case Cread:
			if(!devread(cdev, a2, buf))
				break;
			c->state = Cnone;

		case Cnone:
			if(devread(wdev, addr, buf)) {
				state = Cerror;
				break;
			}
			if(addr > max)
				newmax = addr;
			if(!devwrite(cdev, a2, buf))
				c->state = Cread;
			break;

		case Cdirty:
		case Cdump:
		case Cdump1:
		case Cwrite:
			if(devread(cdev, a2, buf))
				state = Cerror;
			break;
		}
		break;

	case Owrite:
		switch(state) {
		default:
			goto bad;

		case Cdump:
		case Cdump1:
			/*
			 * this is hard part -- a dump block must be
			 * sent to the worm if it is rewritten.
			 * if this causes an error, there is no
			 * place to save the dump1 data. the block
			 * is just reclassified as 'dump1' (botch)
			 */
			p1 = getbuf(devnone, Cwio1, 0);
			if(devread(cdev, a2, p1->iobuf)) {
				putbuf(p1);
				print("cwio: write induced dump error - r cache\n");

			casenone:
				if(devwrite(cdev, a2, buf)) {
					state = Cerror;
					break;
				}
				c->state = Cdump1;
				break;
			}
			if(devwrite(wdev, addr, p1->iobuf)) {
				p2 = getbuf(devnone, Cwio2, 0);
				if(devread(wdev, addr, p2->iobuf)) {
					putbuf(p1);
					putbuf(p2);
					print("cwio: write induced dump error - r+w worm\n");
					goto casenone;
				}
				if(memcmp(p1->iobuf, p2->iobuf, RBUFSIZE)) {
					putbuf(p1);
					putbuf(p2);
					print("cwio: write induced dump error - w worm\n");
					goto casenone;
				}
				putbuf(p2);
			}
			putbuf(p1);
			c->state = Cread;
			if(addr > max)
				newmax = addr;
			cw.ncopy++;

		case Cnone:
		case Cread:
			if(devwrite(cdev, a2, buf)) {
				state = Cerror;
				break;
			}
			c->state = Cwrite;
			break;

		case Cdirty:
		case Cwrite:
			if(devwrite(cdev, a2, buf))
				state = Cerror;
			break;
		}
		break;

	case Ogrow:
		if(state != Cnone) {
			print("cwgrow with state = %s\n",
				cwnames[state]);
			break;
		}
		c->state = Cdirty;
		break;

	case Odump:
		if(state != Cdirty) {	/* BOTCH */
			print("cwdump with state = %s\n",
				cwnames[state]);
			break;
		}
		c->state = Cdump;
		cw.ndump++;	/* only called from dump command */
		break;

	case Orele:
		if(state != Cwrite) {
			if(state != Cdump1)
				print("cwrele with state = %s\n",
					cwnames[state]);
			break;
		}
		c->state = Cnone;
		break;

	case Ofree:
		if(state == Cwrite || state == Cread)
			c->state = Cnone;
		break;
	}
	if(DEBUG)
		print("cwio: %ld s=%s o=%s ns=%s\n",
			addr, cwnames[state],
			cwnames[opcode],
			cwnames[c->state]);
	putbuf(p);
	if(newmax) {
		cb = getbuf(cdev, CACHE_ADDR, Bread|Bmod|Bres);
		h = (Cache*)cb->iobuf;
		if(newmax > h->wmax)
			h->wmax = newmax;
		putbuf(cb);
	}
	return state;

bad:
	print("cw state = %s; cw opcode = %s",
		cwnames[state], cwnames[opcode]);
	return Cerror;
}

int
cwgrow(Device dev, Superb *sb, int uid)
{
	Iobuf *cb;
	Cache *h;
	long fs, nfs, ws;
	int state;

	cb = getbuf(CDEV(dev), CACHE_ADDR, Bread|Bmod|Bres);
	h = (Cache*)cb->iobuf;
	ws = h->wsize;
	fs = h->fsize;
	if(fs >= ws)
		return 0;
	nfs = fs + ADDFREE;
	if(nfs >= ws)
		nfs = ws;
	h->fsize = nfs;
	putbuf(cb);

	sb->fsize = nfs;
	print("%D grow from %ld to %ld limit %ld uid=%d\n", dev, fs, nfs, ws, uid);
	for(nfs--; nfs>=fs; nfs--) {
		state = cwio(dev, nfs, 0, Ogrow);
		if(state == Cnone)
			addfree(dev, nfs, sb);
	}
	return 1;
}

int
cwfree(Device dev, long addr)
{
	int state;

	if(dev.type == Devcw) {
		state = cwio(dev, addr, 0, Ofree);
		if(state != Cdirty)
			return 1;	/* do not put in freelist */
	}
	return 0;			/* put in freelist */
}

int
bktcheck(Bucket *b)
{
	Centry *c, *c1, *c2, *ce;
	int err;

	err = 0;
	if(b->agegen < CEPERBK || b->agegen > MAXAGE) {
		print("agegen %ld\n", b->agegen);
		err = 1;
	}

	ce = b->entry + CEPERBK;
	c1 = 0;		/* lowest age last pass */
	for(;;) {
		c2 = 0;		/* lowest age this pass */
		for(c = b->entry; c < ce; c++) {
			if(c1 != 0 && c != c1) {
				if(c->age == c1->age) {
					print("same age %d\n", c->age);
					err = 1;
				}
				if(c1->waddr == c->waddr)
				if(c1->state != Cnone)
				if(c->state != Cnone) {
					print("same waddr %ld\n", c->waddr);
					err = 1;
				}
			}
			if(c1 != 0 && c->age <= c1->age)
				continue;
			if(c2 == 0 || c->age < c2->age)
				c2 = c;
		}
		if(c2 == 0)
			break;
		c1 = c2;
		if(c1->age >= b->agegen) {
			print("age >= generator %d %d\n", c1->age, b->agegen);
			err = 1;
		}
	}
	return err;
}

void
resequence(Bucket *b)
{
	Centry *c, *ce, *cr;
	int age, i;

	ce = b->entry + CEPERBK;
	for(c = b->entry; c < ce; c++) {
		c->age += CEPERBK;
		if(c->age < CEPERBK)
			c->age = MAXAGE;
	}
	b->agegen += CEPERBK;

	age = 0;
	for(i=0;; i++) {
		cr = 0;
		for(c = b->entry; c < ce; c++) {
			if(c->age < i)
				continue;
			if(cr == 0 || c->age < age) {
				cr = c;
				age = c->age;
			}
		}
		if(cr == 0)
			break;
		cr->age = i;
	}
	b->agegen = i;
	cons.nreseq++;
}

Centry*
getcentry(Bucket *b, long addr)
{
	Centry *c, *ce, *cr;
	int s, age;

	/*
	 * search for cache hit
	 * find oldest block as byproduct
	 */
	ce = b->entry + CEPERBK;
	age = 0;
	cr = 0;
	for(c = b->entry; c < ce; c++) {
		s = c->state;
		if(s == Cnone) {
			cr = c;
			age = 0;
			continue;
		}
		if(c->waddr == addr)
			goto found;
		if(s == Cread) {
			if(cr == 0 || c->age < age) {
				cr = c;
				age = c->age;
			}
		}
	}

	/*
	 * remap entry
	 */
	c = cr;
	if(c == 0)
		return 0;	/* bucket is full */

	c->state = Cnone;
	c->waddr = addr;

found:
	/*
	 * update the age to get filo cache.
	 * small number in age means old
	 */
	if(!cons.noage || c->state == Cnone) {
		age = b->agegen;
		c->age = age;
		age++;
		b->agegen = age;
		if(age < 0 || age >= MAXAGE)
			resequence(b);
	}
	return c;
}

/*
 * ream the cache
 * calculate new buckets
 */
Iobuf*
cacheinit(Device dev)
{
	Iobuf *cb, *p;
	Cache *h;
	Device cdev;
	long long m;

	print("cache init %D\n", dev);
	cdev = CDEV(dev);
	devinit(cdev);

	cb = getbuf(cdev, CACHE_ADDR, Bmod|Bres);
	memset(cb->iobuf, 0, RBUFSIZE);
	settag(cb, Tcache, QPSUPER);
	h = (Cache*)cb->iobuf;

	/*
	 * calculate csize such that
	 * tsize = msize/BKPERBLK + csize and
	 * msize = csize/CEPERBK
	 */
	h->maddr = CACHE_ADDR + 1;
	m = devsize(cdev) - h->maddr;
	h->csize = ((m-1) * CEPERBK*BKPERBLK) / (CEPERBK*BKPERBLK+1);
	h->msize = h->csize/CEPERBK;
	while(!prime(h->msize))
		h->msize--;
	h->csize = h->msize*CEPERBK;
	h->caddr = h->maddr + (h->msize+BKPERBLK-1)/BKPERBLK;
	h->wsize = devsize(WDEV(dev));

	if(h->msize <= 0)
		panic("cache too small");
	if(h->caddr + h->csize > m)
		panic("cache size error");

	/*
	 * setup cache map
	 */
	for(m=h->maddr; m<h->caddr; m++) {
		p = getbuf(cdev, m, Bmod);
		memset(p->iobuf, 0, RBUFSIZE);
		settag(p, Tbuck, m);
		putbuf(p);
	}
	print("done cacheinit\n");
	return cb;
}

/*
 * ream the cache
 * calculate new buckets
 * get superblock from
 * last worm dump block.
 */
void
cwrecover(Device dev)
{
	Iobuf *p, *cb;
	Cache *h;
	Superb *s;
	long m, baddr;
	Device wdev;

	wdev = WDEV(dev);

	devinit(wdev);


	p = getbuf(devnone, Cwxx1, 0);
	s = (Superb*)p->iobuf;
	baddr = 0;
	m = FIRST;
	if(startsb)		/* to force a starting superblock */
		m = startsb;
	for(;;) {
		memset(p->iobuf, 0, RBUFSIZE);
		if(devread(wdev, m, p->iobuf) ||
		   checktag(p, Tsuper, QPSUPER))
			break;
		baddr = m;
		m = s->next;
		print("dump %ld is good; %ld next\n", baddr, m);
	}
	putbuf(p);
	if(!baddr)
		panic("recover: no superblock\n");

	p = getbuf(wdev, baddr, Bread);
	s = (Superb*)p->iobuf;

	cb = cacheinit(dev);
	h = (Cache*)cb->iobuf;
	h->sbaddr = baddr;
	h->cwraddr = s->cwraddr;
	h->roraddr = s->roraddr;
	h->fsize = s->fsize;		/* this must be accurate */
	if(RECOVCW)
		h->cwraddr = RECOVCW;
	if(RECOVRO)
		h->roraddr = RECOVRO;

	putbuf(cb);
	putbuf(p);

	p = getbuf(dev, baddr, Bread|Bmod);
	s = (Superb*)p->iobuf;

	memset(&s->fbuf, 0, sizeof(s->fbuf));
	s->fbuf.free[0] = 0;
	s->fbuf.nfree = 1;
	s->tfree = 0;
	if(RECOVCW)
		s->cwraddr = RECOVCW;
	if(RECOVRO)
		s->roraddr = RECOVRO;

	putbuf(p);
	print("done recover\n");
}
/*
 * ream the cache
 * calculate new buckets
 * initialize superblock.
 */
void
cwream(Device dev)
{
	Iobuf *p, *cb;
	Cache *h;
	Superb *s;
	long m, baddr;
	Device cdev;


	print("cwream %D\n", dev);
	cdev = CDEV(dev);
	devinit(cdev);

	baddr = FIRST;	/*	baddr   = super addr
				baddr+1 = cw root
				baddr+2 = ro root
				baddr+3 = reserved next superblock */

	cb = cacheinit(dev);
	h = (Cache*)cb->iobuf;

	h->sbaddr = baddr;
	h->cwraddr = baddr+1;
	h->roraddr = baddr+2;
	h->fsize = 0;	/* prevents superream from freeing */

	putbuf(cb);

	for(m=0; m<3; m++)
		cwio(dev, baddr+m, 0, Ogrow);
	superream(dev, baddr);
	rootream(dev, baddr+1);			/* cw root */
	rootream(dev, baddr+2);			/* ro root */

	cb = getbuf(cdev, CACHE_ADDR, Bread|Bmod|Bres);
	h = (Cache*)cb->iobuf;
	h->fsize = baddr+4;
	putbuf(cb);

	p = getbuf(dev, baddr, Bread|Bmod|Bimm);
	s = (Superb*)p->iobuf;
	s->last = baddr;
	s->cwraddr = baddr+1;
	s->roraddr = baddr+2;
	s->next = baddr+3;
	s->fsize = baddr+4;
	putbuf(p);

	for(m=0; m<3; m++)
		cwio(dev, baddr+m, 0, Odump);
}

long
rewalk1(long addr, int slot, Wpath *up)
{
	Iobuf *p, *p1;
	Dentry *d;

	if(up == 0)
		return cwraddr(cw.dev, 0);
	up->addr = rewalk1(up->addr, up->slot, up->up);
	p = getbuf(cw.dev, up->addr, Bread|Bmod);
	d = getdir(p, up->slot);
	if(!d || !(d->mode & DALLOC)) {
		print("rewalk1 1\n");
		if(p)
			putbuf(p);
		return addr;
	}
	p1 = dnodebuf(p, d, slot/DIRPERBUF, 0, 0);
	if(!p1) {
		print("rewalk1 2\n");
		if(p)
			putbuf(p);
		return addr;
	}
	if(DEBUG)
		print("rewalk1 %ld to %ld \"%s\"\n",
			addr, p1->addr, d->name);
	addr = p1->addr;
	p1->flags |= Bmod;
	putbuf(p1);
	putbuf(p);
	return addr;
}

long
rewalk2(long addr, int slot, Wpath *up)
{
	Iobuf *p, *p1;
	Dentry *d;

	if(up == 0)
		return cwraddr(cw.dev, 1);
	up->addr = rewalk2(up->addr, up->slot, up->up);
	p = getbuf(cw.rodev, up->addr, Bread);
	d = getdir(p, up->slot);
	if(!d || !(d->mode & DALLOC)) {
		print("rewalk2 1\n");
		if(p)
			putbuf(p);
		return addr;
	}
	p1 = dnodebuf(p, d, slot/DIRPERBUF, 0, 0);
	if(!p1) {
		print("rewalk2 2\n");
		if(p)
			putbuf(p);
		return addr;
	}
	if(DEBUG)
		print("rewalk2 %ld to %ld \"%s\"\n",
			addr, p1->addr, d->name);
	addr = p1->addr;
	putbuf(p1);
	putbuf(p);
	return addr;
}

void
rewalk(void)
{
	int h;
	File *f;

	for(h=0; h<nelem(flist); h++) {
		for(f=flist[h]; f; f=f->next) {
			if(!f->fs)
				continue;
			if(!devcmp(cw.dev, f->fs->dev))
				f->addr = rewalk1(f->addr, f->slot, f->wpath);
			else
			if(!devcmp(cw.rodev, f->fs->dev))
				f->addr = rewalk2(f->addr, f->slot, f->wpath);
		}
	}
}

long
split(Iobuf *p, long addr)
{
	long na;
	int state;

	na = 0;
	if(p && (p->flags & Bmod)) {
		p->flags |= Bimm;
		putbuf(p);
		p = 0;
	}
	state = cwio(cw.dev, addr, 0, Onone);	/* read the state (twice?) */
	switch(state)
	{
	default:
		panic("split: unknown state %s", cwnames[state]);

	case Cerror:
	case Cnone:
	case Cdump:
	case Cread:
		break;

	case Cdump1:
	case Cwrite:
		/*
		 * botch.. could be done by relabeling
		 */
		if(!p) {
			p = getbuf(cw.dev, addr, Bread);
			if(!p) {
				print("split: null getbuf\n");
				break;
			}
		}
		na = cw.fsize;
		cw.fsize = na+1;
		cwio(cw.dev, na, 0, Ogrow);
		cwio(cw.dev, na, p->iobuf, Owrite);
		cwio(cw.dev, na, 0, Odump);
		cwio(cw.dev, addr, 0, Orele);
		break;

	case Cdirty:
		cwio(cw.dev, addr, 0, Odump);
		break;
	}
	if(p)
		putbuf(p);
	return na;
}

int
isdirty(Iobuf *p, long addr, int tag)
{
	int s;

	if(p && (p->flags & Bmod))
		return 1;
	s = cwio(cw.dev, addr, 0, Onone);
	if(s == Cdirty || s == Cwrite)
		return 1;
	if(tag == Tind1 || tag == Tind2)	/* botch, get these modified */
		if(s != Cnone)
			return 1;
	return 0;
}

long
cwrecur(long addr, int tag, int tag1, long qp)
{
	Iobuf *p;
	Dentry *d;
	int i, j, shouldstop;
	long na;
	char *np;


	shouldstop = 0;
	p = getbuf(cw.dev, addr, Bprobe);
	if(!isdirty(p, addr, tag)) {
		if(!cw.all) {
			if(DEBUG)
				print("cwrecur: %s %ld t=%s not dirty\n",
					cw.name, addr, tagnames[tag]);
			if(p)
				putbuf(p);
			return 0;
		}
		shouldstop = 1;
	}
	if(DEBUG)
		print("cwrecur: %s %ld t=%s\n",
			cw.name, addr, tagnames[tag]);
	if(cw.depth >= 100) {
		print("dump depth too great\n");
		if(p)
			putbuf(p);
		return 0;
	}
	cw.depth++;

	switch(tag)
	{
	default:
		print("cwrecur: unknown tag %d\n", tag);

	case Tfile:
		break;

	case Tsuper:
	case Tdir:
		if(!p) {
			p = getbuf(cw.dev, addr, Bread);
			if(!p) {
				print("cwrecur: Tdir p null\n");
				break;
			}
		}
		if(tag == Tdir) {
			cw.namepad[0] = 0;	/* force room */
			np = strchr(cw.name, 0);
			*np++ = '/';
		} else {
			np = 0;	/* set */
			cw.name[0] = 0;
		}

		for(i=0; i<DIRPERBUF; i++) {
			d = getdir(p, i);
			if(!(d->mode & DALLOC))
				continue;
			qp = d->qid.path & ~QPDIR;
			if(tag == Tdir)
				strncpy(np, d->name, NAMELEN);
			else
			if(i > 0)
				print("cwrecur: root with >1 directory\n");
			tag1 = Tfile;
			if(d->mode & DDIR)
				tag1 = Tdir;
			for(j=0; j<NDBLOCK; j++) {
				if(na = d->dblock[j]) {
					na = cwrecur(na, tag1, 0, qp);
					if(na) {
						d->dblock[j] = na;
						p->flags |= Bmod;
					}
				}
			}
			if(na = d->iblock) {
				na = cwrecur(na, Tind1, tag1, qp);
				if(na) {
					d->iblock = na;
					p->flags |= Bmod;
				}
			}
			if(na = d->diblock) {
				na = cwrecur(na, Tind2, tag1, qp);
				if(na) {
					d->diblock = na;
					p->flags |= Bmod;
				}
			}
		}
		break;

	case Tind1:
		j = tag1;
		tag1 = 0;
		goto tind;

	case Tind2:
		j = Tind1;

	tind:
		if(!p) {
			p = getbuf(cw.dev, addr, Bread);
			if(!p) {
				print("cwrecur: Tind p null\n");
				break;
			}
		}
		for(i=0; i<INDPERBUF; i++) {
			if(na = ((long*)p->iobuf)[i]) {
				na = cwrecur(na, j, tag1, qp);
				if(na) {
					((long*)p->iobuf)[i] = na;
					p->flags |= Bmod;
				}
			}
		}
		break;
	}
	na = split(p, addr);
	cw.depth--;
	if(na && shouldstop) {
		if(cw.falsehits < 10)
			print("shouldstop %s %ld %ld t=%s\n",
				cw.name, addr, na, tagnames[tag]);
		cw.falsehits++;
	}
	return na;
}

void
cfsdump(Filsys *fs)
{
	Iobuf *pr, *p1, *p;
	Dentry *dr, *d1, *d;
	Cache *h;
	Superb *s;
	long orba, rba, oroa, roa, sba, a, m, n, i, tim;
	char tstr[20];

	if(fs->dev.type != Devcw) {
		print("cant dump this: %D\n", fs->dev);
		return;
	}

	tim = toytime();
	wlock(&mainlock);		/* dump */

	/*
	 * set up static structure
	 * with frequent variables
	 */
	cw.dev = fs->dev;
	cw.rodev = cw.dev;
	cw.rodev.type = Devro;
	cw.wdev = WDEV(cw.dev);
	cw.cdev = CDEV(cw.dev);

	cw.ndump = 0;
	cw.name[0] = 0;
	cw.depth = 0;

	/*
	 * cw root
	 */
	sync("before dump");
	cw.fsize = cwsize(cw.dev);
	orba = cwraddr(cw.dev, 0);
	print("cwroot %ld", orba);
	cons.noage = 1;
	cw.all = cw.allflag;
	rba = cwrecur(orba, Tsuper, 0, QPROOT);
	if(rba == 0)
		rba = orba;
	print("->%ld\n", rba);
	sync("after cw");

	/*
	 * partial super block
	 */
	p = getbuf(cw.dev, cwsaddr(cw.dev), Bread|Bmod|Bimm);
	s = (Superb*)p->iobuf;
	s->fsize = cw.fsize;
	s->cwraddr = rba;
	putbuf(p);

	/*
	 * partial cache block
	 */
	p = getbuf(cw.cdev, CACHE_ADDR, Bread|Bmod|Bimm|Bres);
	h = (Cache*)p->iobuf;
	h->fsize = cw.fsize;
	h->cwraddr = rba;
	putbuf(p);

	/*
	 * ro root
	 */
	oroa = cwraddr(cw.dev, 1);
	pr = getbuf(cw.dev, oroa, Bread|Bmod);
	dr = getdir(pr, 0);

	datestr(tstr, time());	/* tstr = "yyyymmdd" */
	n = 0;
	for(a=0;; a++) {
		p1 = dnodebuf(pr, dr, a, Tdir, 0);
		if(!p1)
			goto bad;
		n++;
		for(i=0; i<DIRPERBUF; i++) {
			d1 = getdir(p1, i);
			if(!d1)
				goto bad;
			if(!(d1->mode & DALLOC))
				goto found1;
			if(!memcmp(d1->name, tstr, 4))
				goto found2;	/* found entry */
		}
		putbuf(p1);
	}

	/*
	 * no year directory, create one
	 */
found1:
	p = getbuf(cw.dev, rba, Bread);
	d = getdir(p, 0);
	d1->qid = d->qid;
	d1->qid.version += n;
	memmove(d1->name, tstr, 4);
	d1->mode = d->mode;
	d1->uid = d->uid;
	d1->gid = d->gid;
	putbuf(p);
	accessdir(p1, d1, FWRITE, 0);

	/*
	 * put mmdd[count] in year directory
	 */
found2:
	accessdir(p1, d1, FREAD, 0);
	putbuf(pr);
	pr = p1;
	dr = d1;

	n = 0;
	m = 0;
	for(a=0;; a++) {
		p1 = dnodebuf(pr, dr, a, Tdir, 0);
		if(!p1)
			goto bad;
		n++;
		for(i=0; i<DIRPERBUF; i++) {
			d1 = getdir(p1, i);
			if(!d1)
				goto bad;
			if(!(d1->mode & DALLOC))
				goto found;
			if(!memcmp(d1->name, tstr+4, 4))
				m++;
		}
		putbuf(p1);
	}

	/*
	 * empty slot put in root
	 */
found:
	if(m)	/* how many dumps this date */
		sprint(tstr+8, "%d", m);

	p = getbuf(cw.dev, rba, Bread);
	d = getdir(p, 0);
	*d1 = *d;				/* qid is QPROOT */
	putbuf(p);
	strcpy(d1->name, tstr+4);
	d1->qid.version += n;
	accessdir(p1, d1, FWRITE, 0);
	putbuf(p1);
	putbuf(pr);

	cw.fsize = cwsize(cw.dev);
	oroa = cwraddr(cw.dev, 1);		/* probably redundant */
	print("roroot %ld", oroa);

	cons.noage = 0;
	cw.all = 0;
	roa = cwrecur(oroa, Tsuper, 0, QPROOT);
	if(roa == 0) {
		print("[same]");
		roa = oroa;
	}
	print("->%ld /%.4s/%s\n", roa, tstr, tstr+4);
	sync("after ro");

	/*
	 * final super block
	 */
	a = cwsaddr(cw.dev);
	print("sblock %ld", a);
	p = getbuf(cw.dev, a, Bread|Bmod|Bimm);
	s = (Superb*)p->iobuf;
	s->last = a;
	sba = s->next;
	s->next = cw.fsize;
	cw.fsize++;
	s->fsize = cw.fsize;
	s->roraddr = roa;

	cwio(cw.dev, sba, 0, Ogrow);
	cwio(cw.dev, sba, p->iobuf, Owrite);
	cwio(cw.dev, sba, 0, Odump);
	print("->%ld (->%ld)\n", sba, s->next);

	putbuf(p);

	/*
	 * final cache block
	 */
	p = getbuf(cw.cdev, CACHE_ADDR, Bread|Bmod|Bimm|Bres);
	h = (Cache*)p->iobuf;
	h->fsize = cw.fsize;
	h->roraddr = roa;
	h->sbaddr = sba;
	putbuf(p);

	rewalk();
	sync("all done");

	print("%ld blocks queued for worm\n", cw.ndump);
	print("%ld falsehits\n", cw.falsehits);
	cw.nodump = 0;

	/*
	 * extend all of the locks
	 */
	tim = toytime() - tim;
	for(i=0; i<NTLOCK; i++)
		if(tlocks[i].time > 0)
			tlocks[i].time += tim;

	wunlock(&mainlock);
	return;

bad:
	panic("dump: bad");
}

void
mvstates(Device dev, int s1, int s2, int drive)
{
	Iobuf *p, *cb;
	Cache *h;
	Bucket *b;
	Centry *c, *ce;
	long m, lo, hi, msize, maddr;
	Device cdev;

	lo = 0;
	hi = lo + 101*DSIZE;
	if(drive >= 0) {
		lo = drive * DSIZE;
		hi = lo + DSIZE;
	}
	cdev = CDEV(dev);
	cb = getbuf(cdev, CACHE_ADDR, Bread|Bres);
	if(!cb || checktag(cb, Tcache, QPSUPER))
		panic("cwstats: checktag c bucket");
	h = (Cache*)cb->iobuf;
	msize = h->msize;
	maddr = h->maddr;
	putbuf(cb);

	for(m=0; m<msize; m++) {
		p = getbuf(cdev, maddr + m/BKPERBLK, Bread|Bmod);
		if(!p || checktag(p, Tbuck, maddr + m/BKPERBLK))
			panic("cwtest: checktag c bucket");
		b = (Bucket*)p->iobuf + m%BKPERBLK;
		ce = b->entry + CEPERBK;
		for(c=b->entry; c<ce; c++)
			if(c->state == s1 && c->waddr >= lo && c->waddr < hi)
				c->state = s2;
		putbuf(p);
	}
}

void
prchain(Device dev, long m, int flg)
{
	Iobuf *p;
	Superb *s;

	if(m == 0) {
		if(flg)
			m = cwsaddr(dev);
		else
		if(startsb)
			m = startsb;
		else
			m = FIRST;
	}
	p = getbuf(devnone, Cwxx2, 0);
	s = (Superb*)p->iobuf;
	for(;;) {
		memset(p->iobuf, 0, RBUFSIZE);
		if(devread(WDEV(dev), m, p->iobuf) ||
		   checktag(p, Tsuper, QPSUPER))
			break;
		if(flg) {
			print("dump %ld is good; %ld prev\n", m, s->last);
			print("\t%ld cwroot; %ld roroot\n", s->cwraddr, s->roraddr);
			if(m <= s->last)
				break;
			m = s->last;
		} else {
			print("dump %ld is good; %ld next\n", m, s->next);
			print("\t%ld cwroot; %ld roroot\n", s->cwraddr, s->roraddr);
			if(m >= s->next)
				break;
			m = s->next;
		}
	}
	putbuf(p);
}

void
touchsb(Device dev)
{
	Iobuf *p;
	long m;

	m = cwsaddr(dev);
	p = getbuf(devnone, Cwxx2, 0);

	memset(p->iobuf, 0, RBUFSIZE);
	if(devread(WDEV(dev), m, p->iobuf) ||
	   checktag(p, Tsuper, QPSUPER))
		print("WORM SUPER BLOCK READ FAILED\n");
	else
		print("touch superblock %ld\n", m);
	putbuf(p);
}

void
savecache(Device dev, int pcnt)
{
	Iobuf *p, *cb;
	Cache *h;
	Bucket *b;
	Centry *c, *ce;
	long m, n, maddr, msize, left, *longp, nbyte;
	int age;
	Device cdev;

	if(walkto("/adm/cache"))
		goto bad;
	if(con_open(FID2, MWRITE|MTRUNC))
		goto bad;


	cdev = CDEV(dev);
	cb = getbuf(cdev, CACHE_ADDR, Bread|Bres);
	if(!cb || checktag(cb, Tcache, QPSUPER))
		panic("savecache: checktag c bucket");
	h = (Cache*)cb->iobuf;
	msize = h->msize;
	maddr = h->maddr;
	putbuf(cb);

	n = BUFSIZE;			/* calculate write size */
	if(n > MAXDAT)
		n = MAXDAT;

	cb = getbuf(devnone, Cwxx4, 0);
	longp = (long*)cb->iobuf;
	left = n/sizeof(long);
	cons.offset = 0;

	for(m=0; m<msize; m++) {
		if(left < BKPERBLK) {
			nbyte = (n/sizeof(long) - left) * sizeof(long);
			con_write(FID2, cb->iobuf, cons.offset, nbyte);
			cons.offset += nbyte;
			longp = (long*)cb->iobuf;
			left = n/sizeof(long);
		}
		p = getbuf(cdev, maddr + m/BKPERBLK, Bread);
		if(!p || checktag(p, Tbuck, maddr + m/BKPERBLK))
			panic("cwtest: checktag c bucket");
		b = (Bucket*)p->iobuf + m%BKPERBLK;
		resequence(b);
		age = b->agegen - CEPERBK*(100-pcnt)/100;
		if(age < 0)
			age = 0;
		ce = b->entry + CEPERBK;
		for(c=b->entry; c<ce; c++)
			if(c->state == Cread && c->age >= age) {
				*longp++ = c->waddr;
				left--;
			}
		putbuf(p);
	}
	nbyte = (n/sizeof(long) - left) * sizeof(long);
	con_write(FID2, cb->iobuf, cons.offset, nbyte);
	putbuf(cb);
	return;

bad:
	print("cant open /adm/cache\n");
}

void
loadcache(Device dev, int dskno)
{
	Iobuf *p, *cb;
	long m, nbyte, *longp, count;

	if(walkto("/adm/cache"))
		goto bad;
	if(con_open(FID2, MREAD))
		goto bad;

	cb = getbuf(devnone, Cwxx4, 0);
	cons.offset = 0;
	count = 0;

	for(;;) {
		memset(cb->iobuf, 0, BUFSIZE);
		nbyte = con_read(FID2, cb->iobuf, cons.offset, 100) / sizeof(long);
		if(nbyte <= 0)
			break;
		cons.offset += nbyte * sizeof(long);
		longp = (long*)cb->iobuf;
		while(nbyte > 0) {
			m = *longp++;
			nbyte--;
			if(m == 0)
				continue;
			if(dskno < 0 ||
			   m >= dskno*DSIZE && m < (dskno+1)*DSIZE) {
				p = getbuf(dev, m, Bread);
				if(p)
					putbuf(p);
				count++;
			}
		}
	}
	putbuf(cb);
	print("%ld blocks loaded from worm %d\n", count, dskno);
	return;

bad:
	print("cant open /adm/cache\n");
}

void
blockcmp(Device dev, long wa, long ca)
{
	Iobuf *p1, *p2;
	int i, c;

	p1 = getbuf(WDEV(dev), wa, Bread);
	if(!p1) {
		print("dowcmp: wdev error\n");
		return;
	}

	p2 = getbuf(CDEV(dev), ca, Bread);
	if(!p2) {
		print("dowcmp: cdev error\n");
		putbuf(p1);
		return;
	}

	c = 0;
	for(i=0; i<RBUFSIZE; i++)
		if(p1->iobuf[i] != p2->iobuf[i]) {
			print("%4d: %.2x %.2x\n",
				i,
				p1->iobuf[i]&0xff,
				p2->iobuf[i]&0xff);
			c++;
			if(c >= 10)
				break;
		}

	if(c == 0)
		print("no error\n");
	putbuf(p1);
	putbuf(p2);
}

void
wblock(Device dev, long addr)
{
	Iobuf *p1;
	int i;

	p1 = getbuf(dev, addr, Bread);
	if(p1) {
		i = devwrite(WDEV(dev), addr, p1->iobuf);
		print("i = %d\n", i);
		putbuf(p1);
	}
}

void
cwtest(Device dev)
{
	wblock(dev, 20590365);
	wblock(dev, 20606793);
	wblock(dev, 20606794);
	wblock(dev, 20606797);
}

#ifdef	XXX
/* garbage to change sb size
 * probably will need it someday
 */
	fsz = number(0, 0, 10);
	count = 0;
	if(fsz == number(0, -1, 10))
		count = -1;		/* really do it */
	print("fsize = %ld\n", fsz);
	cdev = CDEV(dev);
	cb = getbuf(cdev, CACHE_ADDR, Bread|Bres);
	if(!cb || checktag(cb, Tcache, QPSUPER))
		panic("cwstats: checktag c bucket");
	h = (Cache*)cb->iobuf;
	for(m=0; m<h->msize; m++) {
		p = getbuf(cdev, h->maddr + m/BKPERBLK, Bread|Bmod);
		if(!p || checktag(p, Tbuck, h->maddr + m/BKPERBLK))
			panic("cwtest: checktag c bucket");
		b = (Bucket*)p->iobuf + m%BKPERBLK;
		ce = b->entry + CEPERBK;
		for(c=b->entry; c<ce; c++) {
			if(c->waddr < fsz)
				continue;
			if(count < 0) {
				c->state = Cnone;
				continue;
			}
			if(c->state != Cdirty)
				count++;
		}
		putbuf(p);
	}
	if(count < 0) {
		print("old cache hsize = %ld\n", h->fsize);
		h->fsize = fsz;
		cb->flags |= Bmod;
		p = getbuf(dev, h->sbaddr, Bread|Bmod);
		s = (Superb*)p->iobuf;
		print("old super hsize = %ld\n", s->fsize);
		s->fsize = fsz;
		putbuf(p);
	}
	putbuf(cb);
	print("count = %ld\n", count);
#endif

int
convstate(char *name)
{
	int i;

	for(i=0; i<nelem(cwnames); i++)
		if(cwnames[i])
			if(strcmp(cwnames[i], name) == 0)
				return i;
	return -1;
}

void
searchtag(Device d, int tag, long a)
{
	Iobuf *p;
	Tag *t;
	int i;

	p = getbuf(devnone, Cwxx2, 0);
	t = (Tag*)(p->iobuf+BUFSIZE);
	for(i=0; i<1000; i++) {
		memset(p->iobuf, 0, RBUFSIZE);
		if(devread(d, a+i, p->iobuf))
			continue;
		if(t->tag != tag)
			continue;
		print("tag %d found at %D %ld\n", tag, d, a+i);
		break;
	}
	putbuf(p);
}

void
cmd_cwcmd(int argc, char *argv[])
{
	Filsys *fs;
	Device dev;
	char *arg;
	long s1, s2, a, b;
	char str[28];

	if(argc <= 1) {
		print("	cwcmd mvstate state1 state2 [platter]\n");
		print("	cwcmd prchain [start] [bakflg]\n");
		print("	cwcmd touchsb\n");
		print("	cwcmd savecache [percent]\n");
		print("	cwcmd loadcache [dskno]\n");
		print("	cwcmd wormcmp [dskno]\n");
		print("	cwcmd blockcmp wbno cbno\n");
		print("	cwcmd startdump [01]\n");
		print("	cwcmd acct\n");
		print("	cwcmd clearacct\n");
		return;
	}
	arg = argv[1];
	for(fs=filsys; fs->name; fs++) {
		dev = fs->dev;
		if(dev.type != Devcw)
			continue;

		if(strcmp(arg, "tag") == 0) {
			a = Tsuper;
			if(argc > 2)
				a = number(argv[2], 0, 10);
			b = 0;
			if(argc > 3)
				b = number(argv[3], 0, 10);
			searchtag(WDEV(dev), a, b);
			continue;
		}
		if(strcmp(arg, "mvstate") == 0) {
			if(argc <= 3)
				goto bad;
			s1 = convstate(argv[2]);
			s2 = convstate(argv[3]);
			if(s1 < 0 || s2 < 0)
				goto bad;
			a = -1;
			if(argc > 4)
				a = number(argv[4], 0, 10);
			mvstates(dev, s1, s2, a);
			continue;
		bad:
			print("cwcmd mvstate: bad args\n");
			continue;
		}
		if(strcmp(arg, "prchain") == 0) {
			a = 0;
			if(argc > 2)
				a = number(argv[2], 0, 10);
			s1 = 0;
			if(argc > 3)
				s1 = 1;
			prchain(dev, a, s1);
			continue;
		}
		if(strcmp(arg, "touchsb") == 0) {
			touchsb(dev);
			continue;
		}
		if(strcmp(arg, "savecache") == 0) {
			a = 70;
			if(argc > 2)
				a = number(argv[2], 70, 10);
			savecache(dev, a);
			continue;
		}
		if(strcmp(arg, "loadcache") == 0) {
			s1 = -1;
			if(argc > 2)
				s1 = number(argv[2], 0, 10);
			loadcache(dev, s1);
			continue;
		}
		if(strcmp(arg, "blockcmp") == 0) {
			if(argc < 4) {
				print("cannot arg count\n");
				continue;
			}
			s1 = number(argv[2], 0, 10);
			s2 = number(argv[3], 0, 10);
			blockcmp(dev, s1, s2);
			continue;
		}
		if(strcmp(arg, "startdump") == 0) {
			if(argc > 2)
				cw.nodump = number(argv[2], 0, 10);
			cw.nodump = !cw.nodump;
			if(cw.nodump)
				print("dump stopped\n");
			else
				print("dump allowed\n");
			continue;
		}
		if(strcmp(arg, "allflag") == 0) {
			if(argc > 2)
				cw.allflag = number(argv[2], 0, 10);
			else
				cw.allflag = !cw.allflag;
			print("allflag = %d; falsehits = %d\n",
				cw.allflag, cw.falsehits);
			continue;
		}
		if(strcmp(arg, "writeblock") == 0) {
			if(argc > 2) {
				a = number(argv[2], 0, 10);
				wblock(dev, a);
			}
			continue;
		}
		if(strcmp(arg, "acct") == 0) {
			for(a=0; a<nelem(growacct); a++) {
				b = growacct[a];
				if(b) {
					uidtostr(str, a, 1);
					print("%10d %s\n", (b*ADDFREE*RBUFSIZE+500000)/1000000, str);
				}
			}
			continue;
		}
		if(strcmp(arg, "clearacct") == 0) {
			memset(growacct, 0, sizeof(growacct));
			continue;
		}
		if(strcmp(arg, "test") == 0) {
			cwtest(dev);
			continue;
		}
		print("unknown cwcmd %s\n", arg);
	}
}

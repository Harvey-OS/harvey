/*
 * sheevaplug nand flash driver
 *
 * for now separate from (inferno's) os/port/flashnand.c because the flash
 * seems newer, and has different commands, but that is nand-chip specific,
 * not sheevaplug-specific.  they should be merged in future.
 *
 * the sheevaplug has a hynix 4gbit flash chip: hy27uf084g2m.
 * 2048 byte pages, with 64 spare bytes each; erase block size is 128k.
 *
 * it has a "glueless" interface, at 0xf9000000.  that's the address
 * of the data register.  the command and address registers are those
 * or'ed with 1 and 2 respectively.
 *
 * linux uses this layout for the nand flash (from address 0 onwards):
 *	1mb for u-boot
 *	4mb for kernel
 *	507mb for file system
 *
 * this is not so relevant here except for ecc.  the first two areas
 * (u-boot and kernel) are expected to have 4-bit ecc per 512 bytes
 * (but calculated from last byte to first), bad erase blocks skipped.
 * the file system area has 1-bit ecc per 256 bytes.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"../port/flashif.h"
#include	"../port/nandecc.h"

enum {
	Debug		= 0,

	Nopage		= ~0ul,		/* cache is empty */

	/* vendors */
	Hynix		= 0xad,
	Samsung		= 0xec,

	/* chips */
	Hy27UF084G2M	= 0xdc,

	NandActCEBoot	= 1<<1,
};

typedef struct Nandreg Nandreg;
typedef struct Nandtab Nandtab;
typedef struct Cache Cache;

struct Nandreg {			/* hw registers */
	ulong	rdparms;
	ulong	wrparms;
	uchar	_pad0[0x70 - 0x20];
	ulong	ctl;
};

struct Nandtab {
	int	vid;
	int	did;
	vlong	size;
	char*	name;
};

struct Cache {
	Flash	*flif;
	ulong	pageno;
	ulong	pgsize;			/* r->pagesize */
	char	*page;			/* of pgsize bytes */
};

enum {
	/* commands */
	Readstatus	= 0x70,
	Readid		= 0x90,	/* needs 1 0-address write */
	Resetf		= 0xff,

	/*
	 * needs 5 address writes followed by Readstart,
	 * Readstartcache or Restartcopy.
	 */
	Read		= 0x00,
	Readstart	= 0x30,
	Readstartcache	= 0x31,
	Readstartcopy	= 0x35,
	/* after Readstartcache, to stop reading next pages */
	Readstopcache	= 0x34,

	/* needs 5 address writes, the data, and -start or -cache */
	Program		= 0x80,
	Programstart	= 0x10,
	Programcache	= 0x15,

	Copyback	= 0x85,	/* followed by Programstart */

	/* 3 address writes for block followed by Erasestart */
	Erase		= 0x60,
	Erasestart	= 0xd0,

	Randomread	= 0x85,
	Randomwrite	= 0x05,
	Randomwritestart= 0xe0,

	/* status bits */
	SFail		= 1<<0,
	SCachefail	= 1<<1,
	SIdle		= 1<<5,		/* doesn't seem to come on ever */
	SReady		= 1<<6,
	SNotprotected	= 1<<7,

	Srdymask	= SReady,	/* was SIdle|SReady */
};

Nandtab nandtab[] = {
	{Hynix,		Hy27UF084G2M,	512*MB,	"Hy27UF084G2M"},
	{Samsung,	0xdc,		512*MB,	"Samsung 2Gb"},
};

static Cache cache;

static void
nandcmd(Flash *f, uchar b)
{
	uchar *p = (uchar *)((ulong)f->addr|1);

	*p = b;
	coherence();
}

static void
nandaddr(Flash *f, uchar b)
{
	uchar *p = (uchar *)((ulong)f->addr|2);

	*p = b;
	coherence();
}

static uchar
nandread(Flash *f)
{
	return *(uchar *)f->addr;
}

static void
nandreadn(Flash *f, uchar *buf, long n)
{
	uchar *p = f->addr;

	while(n-- > 0)
		*buf++ = *p;
}

static void
nandwrite(Flash *f, uchar b)
{
	*(uchar *)f->addr = b;
	coherence();
}

static void
nandwriten(Flash *f, uchar *buf, long n)
{
	uchar *p = f->addr;

	while(n-- > 0)
		*p = *buf++;
	coherence();
}

static void
nandclaim(Flash*)
{
	Nandreg *nand = (Nandreg*)soc.nand;

	nand->ctl |= NandActCEBoot;
	coherence();
}

static void
nandunclaim(Flash*)
{
	Nandreg *nand = (Nandreg*)soc.nand;

	nand->ctl &= ~NandActCEBoot;
	coherence();
}


void	mmuidmap(uintptr phys, int mbs);

Nandtab *
findflash(Flash *f, uintptr pa, uchar *id4p)
{
	int i;
	ulong sts;
	uchar maker, device, id3, id4;
	Nandtab *chip;

	mmuidmap(pa, 16);
	f->addr = (void *)pa;

	/* make sure controller is idle */
	nandclaim(f);
	nandcmd(f, Resetf);
	nandunclaim(f);

	nandclaim(f);
	nandcmd(f, Readstatus);
	sts = nandread(f);
	nandunclaim(f);
	for (i = 10; i > 0 && !(sts & SReady); i--) {
		delay(50);
		nandclaim(f);
		nandcmd(f, Readstatus);
		sts = nandread(f);
		nandunclaim(f);
	}
	if(!(sts & SReady))
		return nil;

	nandclaim(f);
	nandcmd(f, Readid);
	nandaddr(f, 0);
	maker = nandread(f);
	device = nandread(f);
	id3 = nandread(f);
	USED(id3);
	id4 = nandread(f);
	nandunclaim(f);
	if (id4p)
		*id4p = id4;

	for(i = 0; i < nelem(nandtab); i++) {
		chip = &nandtab[i];
		if(chip->vid == maker && chip->did == device)
			return chip;
	}
	return nil;
}

int
flashat(Flash *f, uintptr pa)
{
	return findflash(f, pa, nil) != nil;
}

static int
idchip(Flash *f)
{
	uchar id4;
	Flashregion *r;
	Nandtab *chip;
	static int blocksizes[4] = { 64*1024, 128*1024, 256*1024, 0 };
	static int pagesizes[4] = { 1024, 2*1024, 0, 0 };
	static int spares[2] = { 8, 16 };		/* per 512 bytes */

	f->id = 0;
	f->devid = 0;
	f->width = 1;
	chip = findflash(f, (uintptr)f->addr, &id4);
	if (chip == nil)
		return -1;
	f->id = chip->vid;
	f->devid = chip->did;
	f->size = chip->size;
	f->width = 1;
	f->nr = 1;

	r = &f->regions[0];
	r->pagesize = pagesizes[id4 & MASK(2)];
	r->erasesize = blocksizes[(id4 >> 4) & MASK(2)];
	if (r->pagesize == 0 || r->erasesize == 0) {
		iprint("flashkw: bogus flash sizes\n");
		return -1;
	}
	r->n = f->size / r->erasesize;
	r->start = 0;
	r->end = f->size;
	assert(ispow2(r->pagesize));
	r->pageshift = log2(r->pagesize);
	assert(ispow2(r->erasesize));
	r->eraseshift = log2(r->erasesize);
	assert(r->eraseshift >= r->pageshift);
	if (cache.page == nil) {
		cache.pgsize = r->pagesize;
		cache.page = smalloc(r->pagesize);
	}

	r->spares = r->pagesize / 512 * spares[(id4 >> 2) & 1];
	print("#F0: kwnand: %s %,lud bytes pagesize %lud erasesize %,lud"
		" spares per page %lud\n", chip->name, f->size, r->pagesize,
		r->erasesize, r->spares);
	return 0;
}

static int
ctlrwait(Flash *f)
{
	int sts, cnt;

	nandclaim(f);
	for (;;) {
		nandcmd(f, Readstatus);
		for(cnt = 100; cnt > 0 && (nandread(f) & Srdymask) != Srdymask;
		    cnt--)
			microdelay(50);
		nandcmd(f, Readstatus);
		sts = nandread(f);
		if((sts & Srdymask) == Srdymask)
			break;
		print("flashkw: flash ctlr busy, sts %#ux: resetting\n", sts);
		nandcmd(f, Resetf);
	}
	nandunclaim(f);
	return 0;
}

static int
erasezone(Flash *f, Flashregion *r, ulong offset)
{
	int i;
	ulong page, block;
	uchar s;

	if (Debug) {
		print("flashkw: erasezone: offset %#lux, region nblocks %d,"
			" start %#lux, end %#lux\n", offset, r->n, r->start,
			r->end);
		print(" erasesize %lud, pagesize %lud\n",
			r->erasesize, r->pagesize);
	}
	assert(r->erasesize != 0);
	if(offset & (r->erasesize - 1)) {
		print("flashkw: erase offset %lud not block aligned\n", offset);
		return -1;
	}
	page = offset >> r->pageshift;
	block = page >> (r->eraseshift - r->pageshift);
	if (Debug)
		print("flashkw: erase: block %#lux\n", block);

	/* make sure controller is idle */
	if(ctlrwait(f) < 0) {
		print("flashkw: erase: flash busy\n");
		return -1;
	}

	/* start erasing */
	nandclaim(f);
	nandcmd(f, Erase);
	nandaddr(f, page>>0);
	nandaddr(f, page>>8);
	nandaddr(f, page>>16);
	nandcmd(f, Erasestart);

	/* invalidate cache on any erasure (slight overkill) */
	cache.pageno = Nopage;

	/* have to wait until flash is done.  typically ~2ms */
	delay(1);
	nandcmd(f, Readstatus);
	for(i = 0; i < 100; i++) {
		s = nandread(f);
		if(s & SReady) {
			nandunclaim(f);
			if(s & SFail) {
				print("flashkw: erase: failed, block %#lux\n",
					block);
				return -1;
			}
			return 0;
		}
		microdelay(50);
	}
	print("flashkw: erase timeout, block %#lux\n", block);
	nandunclaim(f);
	return -1;
}

static void
flcachepage(Flash *f, ulong page, uchar *buf)
{
	Flashregion *r = &f->regions[0];

	assert(cache.pgsize == r->pagesize);
	cache.flif = f;
	cache.pageno = page;
	/* permit i/o directly to or from the cache */
	if (buf != (uchar *)cache.page)
		memmove(cache.page, buf, cache.pgsize);
}

static int
write1page(Flash *f, ulong offset, void *buf)
{
	int i;
	ulong page, v;
	uchar s;
	uchar *eccp, *p;
	Flashregion *r = &f->regions[0];
	static uchar *oob;

	if (oob == nil)
		oob = smalloc(r->spares);

	page = offset >> r->pageshift;
	if (Debug)
		print("flashkw: write nand offset %#lux page %#lux\n",
			offset, page);

	if(offset & (r->pagesize - 1)) {
		print("flashkw: write offset %lud not page aligned\n", offset);
		return -1;
	}

	p = buf;
	memset(oob, 0xff, r->spares);
	assert(r->spares >= 24);
	eccp = oob + r->spares - 24;
	for(i = 0; i < r->pagesize / 256; i++) {
		v = nandecc(p);
		*eccp++ = v>>8;
		*eccp++ = v>>0;
		*eccp++ = v>>16;
		p += 256;
	}

	if(ctlrwait(f) < 0) {
		print("flashkw: write: nand not ready & idle\n");
		return -1;
	}

	/* write, only whole pages for now, no sub-pages */
	nandclaim(f);
	nandcmd(f, Program);
	nandaddr(f, 0);
	nandaddr(f, 0);
	nandaddr(f, page>>0);
	nandaddr(f, page>>8);
	nandaddr(f, page>>16);
	nandwriten(f, buf, r->pagesize);
	nandwriten(f, oob, r->spares);
	nandcmd(f, Programstart);

	microdelay(100);
	nandcmd(f, Readstatus);
	for(i = 0; i < 100; i++) {
		s = nandread(f);
		if(s & SReady) {
			nandunclaim(f);
			if(s & SFail) {
				print("flashkw: write failed, page %#lux\n",
					page);
				return -1;
			}
			return 0;
		}
		microdelay(10);
	}

	nandunclaim(f);
	flcachepage(f, page, buf);
	print("flashkw: write timeout for page %#lux\n", page);
	return -1;
}

static int
read1page(Flash *f, ulong offset, void *buf)
{
	int i;
	ulong addr, page, w;
	uchar *eccp, *p;
	Flashregion *r = &f->regions[0];
	static uchar *oob;

	if (oob == nil)
		oob = smalloc(r->spares);

	assert(r->pagesize != 0);
	addr = offset & (r->pagesize - 1);
	page = offset >> r->pageshift;
	if(addr != 0) {
		print("flashkw: read1page: read addr %#lux:"
			" must read aligned page\n", addr);
		return -1;
	}

	/* satisfy request from cache if possible */
	if (f == cache.flif && page == cache.pageno &&
	    r->pagesize == cache.pgsize) {
		memmove(buf, cache.page, r->pagesize);
		return 0;
	}

	if (Debug)
		print("flashkw: read offset %#lux addr %#lux page %#lux\n",
			offset, addr, page);

	nandclaim(f);
	nandcmd(f, Read);
	nandaddr(f, addr>>0);
	nandaddr(f, addr>>8);
	nandaddr(f, page>>0);
	nandaddr(f, page>>8);
	nandaddr(f, page>>16);
	nandcmd(f, Readstart);

	microdelay(50);

	nandreadn(f, buf, r->pagesize);
	nandreadn(f, oob, r->spares);

	nandunclaim(f);

	/* verify/correct data. last 8*3 bytes is ecc, per 256 bytes. */
	p = buf;
	assert(r->spares >= 24);
	eccp = oob + r->spares - 24;
	for(i = 0; i < r->pagesize / 256; i++) {
		w = eccp[0] << 8 | eccp[1] << 0 | eccp[2] << 16;
		eccp += 3;
		switch(nandecccorrect(p, nandecc(p), &w, 1)) {
		case NandEccErrorBad:
			print("(page %d)\n", i);
			return -1;
		case NandEccErrorOneBit:
		case NandEccErrorOneBitInEcc:
			print("(page %d)\n", i);
			/* fall through */
		case NandEccErrorGood:
			break;
		}
		p += 256;
	}

	flcachepage(f, page, buf);
	return 0;
}

/*
 * read a page at offset into cache, copy fragment from buf into it
 * at pagoff, and rewrite that page.
 */
static int
rewrite(Flash *f, ulong offset, ulong pagoff, void *buf, ulong size)
{
	if (read1page(f, offset, cache.page) < 0)
		return -1;
	memmove(&cache.page[pagoff], buf, size);
	return write1page(f, offset, cache.page);
}

/* there are no alignment constraints on offset, buf, nor n */
static int
write(Flash *f, ulong offset, void *buf, long n)
{
	uint un, frag, pagoff;
	ulong pgsize;
	uchar *p;
	Flashregion *r = &f->regions[0];

	if(n <= 0)
		panic("flashkw: write: non-positive count %ld", n);
	un = n;
	assert(r->pagesize != 0);
	pgsize = r->pagesize;

	/* if a partial first page exists, update the first page with it. */
	p = buf;
	pagoff = offset % pgsize;
	if (pagoff != 0) {
		frag = pgsize - pagoff;
		if (frag > un)		/* might not extend to end of page */
			frag = un;
		if (rewrite(f, offset - pagoff, pagoff, p, frag) < 0)
			return -1;
		offset += frag;
		p  += frag;
		un -= frag;
	}

	/* copy whole pages */
	while (un >= pgsize) {
		if (write1page(f, offset, p) < 0)
			return -1;
		offset += pgsize;
		p  += pgsize;
		un -= pgsize;
	}

	/* if a partial last page exists, update the last page with it. */
	if (un > 0)
		return rewrite(f, offset, 0, p, un);
	return 0;
}

/* there are no alignment constraints on offset, buf, nor n */
static int
read(Flash *f, ulong offset, void *buf, long n)
{
	uint un, frag, pagoff;
	ulong pgsize;
	uchar *p;
	Flashregion *r = &f->regions[0];

	if(n <= 0)
		panic("flashkw: read: non-positive count %ld", n);
	un = n;
	assert(r->pagesize != 0);
	pgsize = r->pagesize;

	/* if partial 1st page, read it into cache & copy fragment to buf */
	p = buf;
	pagoff = offset % pgsize;
	if (pagoff != 0) {
		frag = pgsize - pagoff;
		if (frag > un)		/* might not extend to end of page */
			frag = un;
		if (read1page(f, offset - pagoff, cache.page) < 0)
			return -1;
		offset += frag;
		memmove(p, &cache.page[pagoff], frag);
		p  += frag;
		un -= frag;
	}

	/* copy whole pages */
	while (un >= pgsize) {
		if (read1page(f, offset, p) < 0)
			return -1;
		offset += pgsize;
		p  += pgsize;
		un -= pgsize;
	}

	/* if partial last page, read into cache & copy initial fragment to buf */
	if (un > 0) {
		if (read1page(f, offset, cache.page) < 0)
			return -1;
		memmove(p, cache.page, un);
	}
	return 0;
}

static int
reset(Flash *f)
{
	if(f->data != nil)
		return 1;
	f->write = write;
 	f->read = read;
	f->eraseall = nil;
	f->erasezone = erasezone;
	f->suspend = nil;
	f->resume = nil;
	f->sort = "nand";
	cache.pageno = Nopage;
	return idchip(f);
}

void
flashkwlink(void)
{
	addflashcard("nand", reset);
}

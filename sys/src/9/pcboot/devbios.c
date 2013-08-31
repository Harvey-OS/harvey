/*
 * read-only driver for BIOS LBA devices.
 * devbios must be initialised first and no disks may be accessed
 * via non-BIOS means (i.e., talking to the disk controller directly).
 * EDD 4.0 defines the INT 0x13 functions.
 *
 * heavily dependent upon correct BIOS implementation.
 * some bioses (e.g., vmware) seem to hang for two minutes then report
 * a disk timeout on reset and extended read operations.
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"pool.h"
#include	"../port/error.h"
#include	"../port/netif.h"
#include	"../port/sd.h"
#include	"dosfs.h"

#define TYPE(q)		((ulong)(q).path & 0xf)
#define UNIT(q)		(((ulong)(q).path>>4) & 0xff)
#define L(q)		(((ulong)(q).path>>12) & 0xf)
#define QID(u, t) 	((u)<<4 | (t))

typedef struct Biosdev Biosdev;
typedef struct Dap Dap;
typedef uvlong Devbytes, Devsects;
typedef uchar Devid;
typedef struct Edrvparam Edrvparam;

enum {
	Debug = 0,
	Pause = 0,			/* delay to read debugging */

	Minsectsz	= 512,		/* for disks */
	Maxsectsz	= 2048,		/* for optical (CDs, etc.) */

	Highshort	= ((1ul<<16) - 1) << 16,  /* upper short of a long */

	Maxdevs		= 8,
	CF		= 1,		/* carry flag: indicates an error */
	Flopid		= 0,		/* first floppy */
	Baseid		= 0x80,		/* first disk */

	Diskint		= 0x13,		/* "INT 13" for bios disk i/o */

	/* cx capability bits in Biosckext results */
	Fixeddisk	= 1<<0,		/* fixed disk access subset */
	Drlock		= 1<<1,
	Edd		= 1<<2,		/* enhanced disk drive support */
	Bit64ext	= 1<<3,

	/* bios calls: int 0x13 disk services w buffer at es:bx */
	Biosinit	= 0,		/* initialise disk & floppy ctlrs */
	Biosdrvsts,			/* status of last int 0x13 call */
	Biosdrvparam	= 8,
	Biosctlrinit,
	Biosreset	=  0xd,		/* reset disk */
	Biosdrvrdy	= 0x10,
	/* extended int 0x13 calls w dap at ds:si */
	Biosckext	= 0x41,
	Biosrdsect,
	Biosedrvparam	= 0x48,

	/* magic numbers for bios calls */
	Imok		= 0x55aa,
	Youreok		= 0xaa55,
};
enum {
	Qzero,				/* assumed to be 0 by devattach */
	Qtopdir		= 1,
	Qtopbase,
	Qtopctl		= Qtopbase,
	Qtopend,

	Qunitdir,
	Qunitbase,
	Qctl		= Qunitbase,
	Qdata,

	Qtopfiles	= Qtopend-Qtopbase,
};

struct Biosdev {
	Devbytes size;
	Devbytes offset;
	Devid	id;			/* drive number; e.g., 0x80 */
	ushort	sectsz;
	Chan	*rootchan;
	Bootfs;
};

struct Dap {				/* a device address packet */
	uchar	size;
	uchar	_unused1;
	uchar	nsects;
	uchar	_unused2;
	union {
		ulong	addr;		/* actual address (nominally seg:off) */
		struct {
			ushort	addroff;	/* :offset */
			ushort	addrseg;	/* segment: */
		};
	};
	uvlong	stsect;			/* starting sector */

	uvlong	addr64;			/* instead of addr, if addr is ~0 */
	ulong	lnsects;		/* nsects to match addr64 */
	ulong	_unused3;
};

struct Edrvparam {
	ushort	size;			/* max. buffer (struct) size */
	ushort	flags;
	ulong	physcyls;
	ulong	physheads;
	ulong	phystracksects;
	uvlong	physsects;
	ushort	sectsz;

	/* pointer is required to be unaligned, bytes 26-29.  ick. */
//	void	*dpte;			/* ~0ull: invalid */
	ushort	dpteoff;		/* device parameter table extension */
	ushort	dpteseg;

	/* remainder from edd 3.0 spec */
	ushort	key;			/* 0xbedd if device path info present */
	uchar	dpilen;			/* must be 44 (says edd 4.0) */
	uchar	_unused1;
	ushort	_unused2;
	char	bustype[4];		/* "PCI" or "ISA" */
	char	ifctype[8]; /* "ATA", "ATAPI", "SCSI", "USB", "1394", "FIBRE" */
	uvlong	ifcpath;
	uvlong	devpath[2];
	uchar	_unused3;
	uchar	dpicksum;
};

int biosinited;
int biosndevs;

void *biosgetfspart(int i, char *name, int chatty);

static Biosdev bdev[Maxdevs];
static Ureg regs;
static RWlock devs;

static int	dreset(Devid drive);
static Devbytes	extgetsize(Biosdev *);
static int	drivecap(Devid drive);

/* convert ah error code to a string (just common cases) */
static char *
strerr(uchar err)
{
	switch (err) {
	case 0:
		return "no error";
	case 1:
		return "bad command";
	case 0x80:
		return "disk timeout";
	default:
		return "unknown";
	}
}

static void
assertlow64k(uintptr p, char *tag)
{
	if (p & Highshort)
		panic("devbios: %s address %#p not in bottom 64k", tag, p);
}

static void
initrealregs(Ureg *ureg)
{
	memset(ureg, 0, sizeof *ureg);
}

/*
 * caller must zero or otherwise initialise *ureg,
 * other than ax, bx, dx, si & ds.
 */
static int
biosdiskcall(Ureg *ureg, uchar op, ulong bx, ulong dx, ulong si)
{
	int s;
	uchar err;

	s = splhi();		/* don't let the bios call be interrupted */
	initrealregs(ureg);
	ureg->ax = op << 8;
	ureg->bx = bx;
	ureg->dx = dx;		/* often drive id */
	assertlow64k(si, "dap");
	if(si && (si & Highshort) != ((si + Maxsectsz - 1) & Highshort))
		print("biosdiskcall: dap address %#lux too near segment boundary\n",
			si);

	ureg->si = si;		/* ds:si forms data address packet addr */
	ureg->ds = 0;		/* bottom 64K */
	ureg->es = 0;		/* es:bx is conventional buffer */
	ureg->di = 0;		/* buffer segment? */
	ureg->flags = 0;

	/*
	 * *ureg is copied into low memory (realmoderegs) and thence into
	 * the machine registers before the BIOS call, and the registers are
	 * copied into realmoderegs and thence into *ureg after.
	 *
	 * realmode loads these registers: di, si, ax, bx, cx, dx, ds, es.
	 */
	ureg->trap = Diskint;
	realmode(ureg);

	if (ureg->flags & CF) {
		if (dx == Baseid) {
			err = ureg->ax >> 8;
			print("\nbiosdiskcall: int %#x op %#ux drive %#lux "
				"failed, ah error code %#ux (%s)\n",
				Diskint, op, dx, err, strerr(err));
		}
		splx(s);
		return -1;
	}
	splx(s);
	return 0;
}

/*
 * Find out what the bios knows about devices.
 * our boot device could be usb; ghod only knows where it will appear.
 */
int
biosinit0(void)
{
	int cap, mask, lastbit, ndrive;
	Devbytes size;
	Devid devid;
	Biosdev *bdp;
	static int beenhere;

	delay(Pause);		/* pause to read the screen (DEBUG) */
	if (biosinited || beenhere)
		return 0;
	beenhere = 1;

	ndrive = *(uchar *)KADDR(0x475);		/* from bda */
	if (Debug)
		print("%d bios drive(s)\n", ndrive);
	mask = lastbit = 0;
	for (devid = Baseid, biosndevs = 0; devid != 0 && biosndevs < Maxdevs &&
	    biosndevs < ndrive; devid++) {
		cap = drivecap(devid);
		/* don't reset; it seems to hang the bios */
		if(cap < 0 || (cap & (Fixeddisk|Edd)) != (Fixeddisk|Edd)
		    /* || devid != Baseid && dreset(devid) < 0 || */)
			continue;		/* no suitable device */

		/* found a live one */
		lastbit = 1 << biosndevs;
		mask |= lastbit;

		bdp = &bdev[biosndevs];
		bdp->id = devid;
		size = extgetsize(bdp);
		if (size == 0)
			continue;		/* no device */
		bdp->size = size;

		print("bios%d: drive %#ux: %,llud bytes, %d-byte sectors\n",
			biosndevs, devid, size, bdp->sectsz);
		biosndevs++;
	}
	USED(lastbit);

	if (Debug && ndrive != biosndevs)
		print("devbios: expected %d drives, found %d\n", ndrive, biosndevs);

	/*
	 * some bioses seem to only be able to read from drive number 0x80 and
	 * can't read from the highest drive number, even if there is only one.
	 */
	if (biosndevs > 0)
		biosinited = 1;
	else
		panic("devbios: no bios drives seen"); /* 9loadusb needs ≥ 1 */
	delay(Pause);		/* pause to read the screen (DEBUG) */
	return mask;
}

static void
biosreset(void)
{
	biosinit0();
}

static void
biosinit(void)
{
}

static Chan*
biosattach(char *spec)
{
	ulong drive;
	char *p;
	Chan *chan;

	drive = 0;
	if(spec && *spec){
		drive = strtoul(spec, &p, 0);
		if((drive == 0 && p == spec) || *p || (drive >= Maxdevs))
			error(Ebadarg);
	}
	if(bdev[drive].rootchan)
		return bdev[drive].rootchan;

	chan = devattach(L'☹', spec);
	if(waserror()){
		chanfree(chan);
		nexterror();
	}
	chan->dev = drive;
	bdev[drive].rootchan = chan;
	/* arbitrary initialisation can go here */
	poperror();
	return chan;
}

static int
unitgen(Chan *c, ulong type, Dir *dp)
{
	int perm, t;
	ulong vers;
	vlong size;
	char *p;
	Qid q;

	perm = 0644;
	size = 0;
//	d = unit2dev(UNIT(c->qid));
//	vers = d->vers;
	vers = 0;
	t = QTFILE;

	switch(type){
	default:
		return -1;
	case Qctl:
		p = "ctl";
		break;
	case Qdata:
		p = "data";
		perm = 0640;
		break;
	}
	mkqid(&q, QID(UNIT(c->qid), type), vers, t);
	devdir(c, q, p, size, eve, perm, dp);
	return 1;
}

static int
topgen(Chan *c, ulong type, Dir *d)
{
	int perm;
	vlong size;
	char *p;
	Qid q;

	size = 0;
	switch(type){
	default:
		return -1;
	case Qdata:
		p = "data";
		perm = 0644;
		break;
	}
	mkqid(&q, type, 0, QTFILE);
	devdir(c, q, p, size, eve, perm, d);
	return 1;
}

static int
biosgen(Chan *c, char *, Dirtab *, int, int s, Dir *dp)
{
	Qid q;

	if(c->qid.path == 0){
		switch(s){
		case DEVDOTDOT:
			q.path = 0;
			q.type = QTDIR;
			devdir(c, q, "#☹", 0, eve, 0555, dp);
			break;
		case 0:
			q.path = Qtopdir;
			q.type = QTDIR;
			devdir(c, q, "bios", 0, eve, 0555, dp);
			break;
		default:
			return -1;
		}
		return 1;
	}

	switch(TYPE(c->qid)){
	default:
		return -1;
	case Qtopdir:
		if(s == DEVDOTDOT){
			mkqid(&q, Qzero, 0, QTDIR);
			devdir(c, q, "bios", 0, eve, 0555, dp);
			return 1;
		}
		if(s < Qtopfiles)
			return topgen(c, Qtopbase + s, dp);
		s -= Qtopfiles;
		if(s >= 1)
			return -1;
		mkqid(&q, QID(s, Qunitdir), 0, QTDIR);
		devdir(c, q, "bios", 0, eve, 0555, dp);
		return 1;
	case Qdata:
		return unitgen(c, TYPE(c->qid), dp);
	}
}

static Walkqid*
bioswalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, nil, 0, biosgen);
}

static int
biosstat(Chan *c, uchar *db, int n)
{
	return devstat(c, db, n, nil, 0, biosgen);
}

static Chan*
biosopen(Chan *c, int omode)
{
	return devopen(c, omode, 0, 0, biosgen);
}

static void
biosclose(Chan *)
{
}

#ifdef UNUSED
int
biosboot(int dev, char *file, Boot *b)
{
	Bootfs *fs;

	if(strncmp(file, "dos!", 4) == 0)
		file += 4;
	if(strchr(file, '!') != nil || strcmp(file, "") == 0) {
		print("syntax is bios0!file\n");
		return -1;
	}

	fs = biosgetfspart(dev, "9fat", 1);
	if(fs == nil)
		return -1;
	return fsboot(fs, file, b);
}
#endif

/* read n bytes at sector offset into a from drive id */
long
sectread(Biosdev *bdp, void *a, long n, Devsects offset)
{
	uchar *xch;
	uintptr xchaddr;
	Dap *dap;

	if(bdp->sectsz <= 0 || n < 0 || n > bdp->sectsz)
		return -1;
	xch = (uchar *)BIOSXCHG;
	assertlow64k(PADDR(xch), "biosxchg");
	if(Debug)
		/* scribble on the buffer to provoke trouble */
		memset(xch, 'r', bdp->sectsz);

	/* read into BIOSXCHG; alloc space for a worst-case (optical) sector */
	dap = (Dap *)(xch + Maxsectsz);
	assertlow64k(PADDR(dap), "Dap");
	memset(dap, 0, sizeof *dap);
	dap->size = sizeof *dap;
	dap->nsects = 1;
	dap->stsect = offset;

	xchaddr = PADDR(xch);
	assertlow64k(xchaddr, "sectread buffer");
	dap->addr = xchaddr;		/* ulong version */
	dap->addroff = xchaddr;		/* pedantic seg:off */
	dap->addrseg = 0;
	dap->addr64 = xchaddr;		/* paranoid redundancy */
	dap->lnsects = 1;

	/*
	 * ensure that entire buffer fits in low memory.
	 */
	if((dap->addr & Highshort) !=
	    ((dap->addr + Minsectsz - 1) & Highshort))
		print("devbios: sectread: address %#lux too near seg boundary\n",
			dap->addr);
	if (Debug)
		print("reading bios drive %#ux sector %lld -> %#lux...",
			bdp->id, offset, dap->addr);
	delay(Pause);			/* pause to read the screen (DEBUG) */

	/*
	 * int 13 read sector expects buffer seg in di?,
	 * dap in si, 0x42 in ah, drive in dl.
	 */
	if (biosdiskcall(&regs, Biosrdsect, 0, bdp->id, PADDR(dap)) < 0) {
		print("devbios: sectread: bios failed to read %ld @ sector %lld of %#ux\n",
			n, offset, bdp->id);
		return -1;
	}
	if (dap->nsects != 1)
		panic("devbios: sector read ok but read %d sectors",
			dap->nsects);
	if (Debug)
		print("OK\n");

	/* copy into caller's buffer */
	memmove(a, xch, n);
	if(0 && Debug)
		print("-%ux %ux %ux %ux--%16.16s-\n",
			xch[0], xch[1], xch[2], xch[3], (char *)xch + 480);
	delay(Pause);		/* pause to read the screen (DEBUG) */
	return n;
}

/* seems to hang bioses, at least vmware's */
static int
dreset(Devid drive)
{
	print("devbios: resetting %#ux...", drive);
	/* ignore carry flag for Biosinit */
	biosdiskcall(&regs, Biosinit, 0, drive, 0);
	print("\n");
	return regs.ax? -1: 0;		/* ax != 0 on error */
}

/* returns capabilities bitmap */
static int
drivecap(Devid drive)
{
	int cap;

	if (biosdiskcall(&regs, Biosckext, Imok, drive, 0) < 0)
		/*
		 * we have an old bios without extensions, in theory.
		 * in practice, there may just be no drive for this number.
		 */
		return -1;
	if(regs.bx != Youreok){
		print("devbios: buggy bios: drive %#ux extension check "
			 "returned %lux in bx\n", drive, regs.bx);
		return -1;
	}
	cap = regs.cx;
	if (Debug) {
		print("bios drive %#ux extensions version %#x.%d cx %#ux\n",
			drive, (uchar)(regs.ax >> 8), (uchar)regs.ax, cap);
		if ((uchar)(regs.ax >> 8) < 0x30) {
			print("drivecap: extensions prior to 0x30\n");
			return -1;
		}
		print("\tsubsets supported:");
		if (cap & Fixeddisk)
			print(" fixed disk access;");
		if (cap & Drlock)
			print(" drive locking;");
		if (cap & Edd)
			print(" enhanced disk support;");
		if (cap & Bit64ext)
			print(" 64-bit extensions;");
		print("\n");
	}
	delay(Pause);			/* pause to read the screen (DEBUG) */
	return cap;
}

/* extended get size; reads bdp->id, fills in bdp->sectsz, returns # sectors */
static Devbytes
extgetsize(Biosdev *bdp)
{
	ulong sectsz;
	Edrvparam *edp;

	edp = (Edrvparam *)BIOSXCHG;
	memset(edp, 0, sizeof *edp);
	edp->size = sizeof *edp;
	edp->dpteseg = edp->dpteoff = ~0;	/* no pointer */
	edp->dpilen = 44;

	if (biosdiskcall(&regs, Biosedrvparam, 0, bdp->id, PADDR(edp)) < 0)
		return 0;		/* old bios without extensions */
	if(Debug) {
		print("bios drive %#ux info flags %#ux", bdp->id, edp->flags);
		if (edp->key == 0xbedd)
			print("; edd 3.0  %.4s %.8s",
				edp->bustype, edp->ifctype);
		else
			print("; NOT edd 3.0 compliant (key %#ux)", edp->key);
		print("\n");
	}
	if (edp->sectsz <= 0) {
		print("devbios: drive %#ux: sector size <= 0\n", bdp->id);
		edp->sectsz = 1;		/* don't divide by 0 */
		return 0;
	}
	sectsz = edp->sectsz;
	if (sectsz > Maxsectsz) {
		print("devbios: sector size %lud > %d\n", sectsz, Maxsectsz);
		return 0;
	}
	bdp->sectsz = sectsz;
	return edp->physsects * sectsz;
}

vlong
biossize(uint dev)
{
	Biosdev *bdp;

	if (dev >= biosndevs)
		return -1;
	bdp = &bdev[dev];
	if (bdp->sectsz <= 0)
		return -1;
	return bdp->size / bdp->sectsz;
}

long
biossectsz(uint dev)
{
	Biosdev *bdp;

	if (dev >= biosndevs)
		return -1;
	bdp = &bdev[dev];
	if (bdp->sectsz <= 0)
		return -1;
	return bdp->sectsz;
}

long
biosread0(Bootfs *fs, void *a, long n)
{
	int want, got, part, dev;
	long totnr, stuck;
	Devbytes offset;
	Biosdev *bdp;

	dev = fs->dev;				/* only use of fs */
	if(dev > biosndevs)
		return -1;
	if (n <= 0)
		return n;
	bdp = &bdev[dev];
	offset = bdp->offset;
	stuck = 0;
	for (totnr = 0; totnr < n && stuck < 4; totnr += got) {
		if (bdp->sectsz == 0) {
			print("devbios: zero sector size\n");
			return -1;
		}
		want = bdp->sectsz;
		if (totnr + want > n)
			want = n - totnr;
		if(0 && Debug && debugload)
			print("bios%d, read: %ld @ off %lld, want: %d, id: %#ux\n",
				dev, n, offset, want, bdp->id);
		part = offset % bdp->sectsz;
		if (part != 0) {	/* back up to start of sector */
			offset -= part;
			totnr  -= part;
			if (totnr < 0) {
				print("biosread0: negative count %ld\n", totnr);
				return -1;
			}
		}
		if ((vlong)offset < 0) {
			print("biosread0: negative offset %lld\n", offset);
			return -1;
		}
		got = sectread(bdp, (char *)a + totnr, want,
			offset / bdp->sectsz);
		if(got <= 0)
			return -1;
		offset += got;
		bdp->offset = offset;
		if (got < bdp->sectsz)
			stuck++;	/* we'll have to re-read this sector */
		else
			stuck = 0;
	}
	return totnr;
}

vlong
biosseek(Bootfs *fs, vlong off)
{
	if (off < 0) {
		print("biosseek(fs, %lld) is illegal\n", off);
		return -1;
	}
	if(fs->dev > biosndevs) {
		print("biosseek: fs->dev %d > biosndevs %d\n", fs->dev, biosndevs);
		return -1;
	}
	bdev[fs->dev].offset = off;	/* do not know size... (yet) */
	return off;
}

static long
biosread(Chan *c, void *db, long n, vlong off)
{
	Biosdev *bp;

	switch(TYPE(c->qid)){
	default:
		error(Eperm);
	case Qzero:
	case Qtopdir:
		return devdirread(c, db, n, 0, 0, biosgen);
	case Qdata:
		bp = &bdev[UNIT(c->qid)];
		if (bp->rootchan == nil)
			panic("biosread: nil root chan for bios%ld",
				UNIT(c->qid));
		biosseek(&bp->Bootfs, off);
		return biosread0(&bp->Bootfs, db, n);
	}
}

/* name is typically "9fat" */
void *
biosgetfspart(int i, char *name, int chatty)
{
	char part[32];
	static Bootfs fs;

	fs.dev = i;
	fs.diskread = biosread0;
	fs.diskseek = biosseek;
	snprint(part, sizeof part, "#S/sdB0/%s", name);
	if(dosinit(&fs, part) < 0){
		if(chatty)
			print("bios%d!%s does not contain a FAT file system\n",
				i, name);
		return nil;
	}
	return &fs;
}

static long
bioswrite(Chan *, void *, long, vlong)
{
	error("bios devices are read-only in bootstrap");
	return 0;
}

Dev biosdevtab = {
	L'☹',
	"bios",

	biosreset,
	biosinit,
	devshutdown,
	biosattach,
	bioswalk,
	biosstat,
	biosopen,
	devcreate,
	biosclose,
	biosread,
	devbread,
	bioswrite,
	devbwrite,
	devremove,
	devwstat,
	devpower,
	devconfig,
};

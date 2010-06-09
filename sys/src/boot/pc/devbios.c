/*
 * boot driver for BIOS LBA devices
 * heavily dependent upon correct BIOS implementation
 */
#include <u.h>
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "fs.h"

typedef uvlong Devbytes, Devsects;

typedef struct Biosdrive Biosdrive;	/* 1 drive -> ndevs */
typedef struct Biosdev Biosdev;

enum {
	Debug = 0,
	Maxdevs = 8,

	CF = 1,				/* carry flag: indicates an error */
	Flopid = 0,			/* first floppy */
	Baseid = 0x80,			/* first disk */

	Dap	= 1<<0,
	Edd	= 1<<2,

	/* bios calls: int 0x13 disk services */
	Biosinit	= 0,		/* initialise disk & floppy ctlrs */
	Biosdrvsts,
	Bioschsrdsects,
	Biosdrvparam	= 8,
	Biosctlrinit,
	Biosreset	=  0xd,		/* reset disk */
	Biosdrvrdy	= 0x10,
	Biosdrvtype	= 0x15,
	Biosckext	= 0x41,
	Biosrdsect,
	Biosedrvparam	= 0x48,

	/* disk types */
	Typenone = 0,
	Typedisk = 3,
};

struct Biosdrive {
	int	ndevs;
};
struct Biosdev {
	Devbytes size;
	Devbytes offset;
	uchar	id;			/* drive number; e.g., 0x80 */
	char	type;
	ushort	sectsz;
};

typedef struct Extread {	/* a device address packet */
	uchar	size;
	uchar	unused1;
	uchar	nsects;
	uchar	unused2;
	ulong	addr;		/* segment:offset; ea = (segment<<4)+offset */
	uvlong	stsect;		/* starting sector */
} Extread;
typedef struct Edrvparam {
	/* from edd 1.1 spec */
	ushort	size;			/* max. buffer size */
	ushort	flags;
	ulong	physcyls;
	ulong	physheads;
	ulong	phystracksects;
	uvlong	physsects;
	ushort	sectsz;
	void	*dpte;			/* ~0ull: invalid */

	/* remainder from edd 3.0 spec */
	ushort	key;			/* 0xbedd if present */
	uchar	dpilen;
	uchar	unused1;
	ushort	unused2;
	char	bustype[4];		/* "PCI" or "ISA" */
	char	ifctype[8]; /* "ATA", "ATAPI", "SCSI", "USB", "1394", "FIBRE" */
	uvlong	ifcpath;
	uvlong	devpath;
	uchar	unused3;
	uchar	dpicksum;
} Edrvparam;

void	realmode(int intr, Ureg *ureg);		/* from trap.c */

int onlybios0;
int biosinited;

static Biosdev bdev[Maxdevs];
static Biosdrive bdrive;
static Ureg regs;

static int	dreset(uchar drive);
static Devbytes	extgetsize(Biosdev *);
static Devsects	getsize(uchar drive, char *type);
static int	islba(uchar drive);

/*
 * caller must zero or otherwise initialise *rp,
 * other than ax, bx, dx, si & ds.
 */
static int
biosdiskcall(Ureg *rp, uchar op, ulong bx, ulong dx, ulong si)
{
	rp->ax = op << 8;
	rp->bx = bx;
	rp->dx = dx;		/* often drive id */
	/*
	 * ensure that addr fits in a short,
	 * then jigger it and DS to avoid segment 0.
	 */
	if((si & 0xffff0000) != 0)
		print("biosdiskcall: address %#lux not a short\n", si);
	rp->ds = si >> 4;
	si &= 0xf;
	if((si & 0xffff0000) != ((si + 512 - 1) & 0xffff0000))
		print("biosdiskcall: address %#lux too near segment boundary\n",
			si);
	rp->si = si;		/* ds:si forms data access packet addr */
	/*
	 * *rp is copied into low memory (realmoderegs) and thence into
	 * the machine registers before the BIOS call, and the registers are
	 * copied into realmoderegs and thence into *rp after.
	 */
	realmode(0x13, rp);
	if (rp->flags & CF) {
		if (Debug && dx == Baseid)
			print("\nbiosdiskcall: int 0x13 op 0x%ux drive 0x%lux "
				"failed, ah error code 0x%ux\n",
				op, dx, (uchar)(rp->ax >> 8));
		return -1;
	}
	return 0;
}

/*
 * Find out what the bios knows about devices.
 * our boot device could be usb; ghod only knows where it will appear.
 */
int
biosinit(void)
{
	int devid, lba, mask, lastbit;
	Devbytes size;
	char type;
	Biosdev *bdp;
	static int beenhere;

	/* 9pxeload can't use bios int 13 calls; they wedge the machine */
	if (pxe || !biosload || onlybios0 || biosinited || beenhere)
		return 0;
	beenhere = 1;

	mask = lastbit = 0;
	for (devid = Baseid; devid < (1 << 8) && bdrive.ndevs < Maxdevs;
	     devid++) {
		lba = islba(devid);
		if (lba < 0)
			break;

		/* don't reset; it seems to hang the bios */
		if(!lba /* || devid != Baseid && dreset(devid) < 0 */ )
			continue;
		type = Typenone;
		if (getsize(devid, &type) == 0) { /* no device, end of range */
//			devid &= ~0xf;
//			devid += 0x10;
//			devid--;
//			break;
			continue;
		}
		lastbit = 1 << bdrive.ndevs;
		mask |= lastbit;
		bdp = &bdev[bdrive.ndevs];
		bdp->id = devid;
		bdp->type = type;
		size = extgetsize(bdp);
		bdp->size = size;
		print("bios%d: drive 0x%ux: %,llud bytes, type %d\n",
			bdrive.ndevs, devid, size, type);
		bdrive.ndevs++;
	}
	USED(lastbit);
//	islba(Baseid);	/* do a successful operation to make bios happy again */

	/*
	 * some bioses seem to only be able to read from drive number 0x80
	 * and certainly can't read from the highest drive number when we
	 * call them, even if there is only one.  attempting to read from the
	 * last drive number may yield a hung machine or a two-minute pause.
	 */
	if (bdrive.ndevs > 0) {
#ifdef NOT_LAST_DRIVE
		if (bdrive.ndevs == 1) {
			print("biosinit: sorry, only one bios drive; "
				"can't read last one\n");
			onlybios0 = 1;
		} else
			biosinited = 1;
		bdrive.ndevs--;	/* omit last drive number; it can't be read */
		mask &= ~lastbit;
#else
		biosinited = 1;
#endif
	}
	return mask;
}

void
biosinitdev(int i, char *name)
{
	if(i >= bdrive.ndevs)
		panic("biosinitdev");
	sprint(name, "bios%d", i);
}

void
biosprintdevs(int i)
{
	if(i >= bdrive.ndevs){
		print("got a print for %d, only got %d\n", i, bdrive.ndevs);
		panic("biosprintdevs");
	}
	print(" bios%d", i);
}

int
biosboot(int dev, char *file, Boot *b)
{
	Fs *fs;

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

static void
dump(void *addr, int wds)
{
	unsigned i;
	ulong *p = addr;

	for (i = 0; i < wds; i++)
		print("%lux ", p[i]);
		if (i % 8 == 7)
			print("\n");
	print("\n");
}

/* read n bytes at sector offset into a from drive id */
long
sectread(Biosdev *bdp, void *a, long n, Devsects offset)
{
	uchar *biosparam, *cp;
	Extread *erp;

	if(n < 0 || n > bdp->sectsz)
		return -1;
	if(Debug)
		/* scribble on the buffer to provoke trouble */
		memset((uchar *)BIOSXCHG, 'r', bdp->sectsz);

//	if(Debug)
//		print("drive ready %#ux...", bdp->id);
//	memset(&regs, 0, sizeof regs);
//	if (biosdiskcall(&regs, Biosdrvrdy, 0, bdp->id, 0) < 0)
//		print("not ready\n");

	/* space for a big, optical-size sector, just in case... */
	biosparam = (uchar *)BIOSXCHG + 2*1024;

	/* read into BIOSXCHG */
	erp = (Extread *)biosparam;
	memset(erp, 0, sizeof *erp);
	erp->size = sizeof *erp;
	erp->nsects = 1;
	erp->addr = PADDR(BIOSXCHG);
	erp->stsect = offset;
	/*
	 * ensure that addr fits in a short,
	 * then jigger it to avoid segment 0.
	 */
	if((erp->addr & 0xffff0000) != 0)
		print("sectread: address %#lux not a short\n", erp->addr);
	erp->addr = ((erp->addr & 0xffff) >> 4) << 16 | (erp->addr & 0xf);
	if((erp->addr & 0xffff0000) != ((erp->addr + 512 - 1) & 0xffff0000))
		print("sectread: address %#lux too near segment boundary\n",
			erp->addr);

	if (0 && Debug)
		print("reading drive %#ux offset %lld into seg:off %lux:%ux...",
			bdp->id, offset, erp->addr>>16, (ushort)erp->addr);
	if (Debug)
		dump(erp, sizeof *erp / 4);
	memset(&regs, 0, sizeof regs);
	regs.es = erp->addr >> 16;
	if (biosdiskcall(&regs, Biosrdsect, (ushort)erp->addr, bdp->id,
	    PADDR(erp)) < 0) {
		print("sectread: bios failed to read %ld @ sector %lld of 0x%ux\n",
			n, offset, bdp->id);
		return -1;
	}

	/* copy into caller's buffer */
	memmove(a, (char *)BIOSXCHG, n);
	if(0 && Debug){
		cp = (uchar *)BIOSXCHG;
		print("-%ux %ux %ux %ux--%16.16s-\n",
			cp[0], cp[1], cp[2], cp[3], (char *)cp + 480);
	}
	return n;
}

/* not tested yet. */
static int
dreset(uchar drive)
{
	print("devbios: resetting %#ux...", drive);
	memset(&regs, 0, sizeof regs);
	if (biosdiskcall(&regs, Biosinit, 0, drive, 0) < 0)
		print("failed");
	print("\n");
	return regs.ax? -1: 0;		/* ax!=0 on error */
}

static int
islba(uchar drive)
{
	memset(&regs, 0, sizeof regs);
	if (biosdiskcall(&regs, Biosckext, 0x55aa, drive, 0) < 0) {
		/*
		 * we have an old bios without extensions, in theory.
		 * in practice, there may just be no drive for this number.
		 */
//		print("islba: drive %ux: Biosckext failed\n", drive);
		return -1;
	}
	if(regs.bx != 0xaa55){
		print("islba: buggy bios: drive %#ux extension check returned "
			"%lux in bx\n", drive, regs.bx);
		return -1;
	}
	if (Debug)
		print("islba: drive 0x%ux extensions version %d.%d cx 0x%lux\n",
			drive, (uchar)(regs.ax >> 8),
			(uchar)regs.ax, regs.cx);  /* cx has Edd, Dap bits */
	return regs.cx & Dap;
}

/*
 * works so so... some floppies are 0x80+x when they shouldn't be,
 * and report lba even if they cannot...
 */
static Devsects
getsize(uchar id, char *typep)
{
	int dtype;

	memset(&regs, 0, sizeof regs);
	if (biosdiskcall(&regs, Biosdrvtype, 0x55aa, id, 0) < 0)
		return 0;

	dtype = (ushort)regs.ax >> 8;
	*typep = dtype;
	if(dtype == Typenone){
		print("no such device 0x%ux of type %d\n", id, dtype);
		return 0;
	}
	if(dtype != Typedisk){
		print("non-disk device 0x%ux of type %d\n", id, dtype);
		return 0;
	}
	return (ushort)regs.cx | regs.dx << 16;
}

/* extended get size */
static Devbytes
extgetsize(Biosdev *bdp)
{
	Edrvparam *edp;

	edp = (Edrvparam *)BIOSXCHG;
	memset(edp, 0, sizeof *edp);
	edp->size = sizeof *edp;
	edp->dpilen = 36;
	memset(&regs, 0, sizeof regs);
	if (biosdiskcall(&regs, Biosedrvparam, 0, bdp->id, PADDR(edp)) < 0)
		return 0;		/* old bios without extensions */
	if(Debug) {
		print("extgetsize: drive 0x%ux info flags 0x%ux",
			bdp->id, edp->flags);
		if (edp->key == 0xbedd)
			print(" %.4s %.8s", edp->bustype, edp->ifctype);
		print("\n");
	}
	if (edp->sectsz <= 0) {
		print("extgetsize: drive 0x%ux: non-positive sector size\n",
			bdp->id);
		edp->sectsz = 1;		/* don't divide by zero */
	}
	bdp->sectsz = edp->sectsz;
	return edp->physsects * edp->sectsz;
}

long
biosread(Fs *fs, void *a, long n)
{
	int want, got, part;
	long totnr, stuck;
	Devbytes offset;
	Biosdev *bdp;

	if(fs->dev > bdrive.ndevs)
		return -1;
	if (n <= 0)
		return n;
	bdp = &bdev[fs->dev];
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
			print("bios%d, read: %ld @ off %lld, want: %d, id: 0x%ux\n",
				fs->dev, n, offset, want, bdp->id);
		part = offset % bdp->sectsz;
		if (part != 0) {	/* back up to start of sector */
			offset -= part;
			totnr  -= part;
			if (totnr < 0) {
				print("biosread: negative count %ld\n", totnr);
				return -1;
			}
		}
		if ((vlong)offset < 0) {
			print("biosread: negative offset %lld\n", offset);
			return -1;
		}
		got = sectread(bdp, (char *)a + totnr, want, offset/bdp->sectsz);
		if(got <= 0){
//			print("biosread: failed to read %ld @ off %lld of 0x%ux, "
//				"want %d got %d\n",
//				n, offset, bdp->id, want, got);
			return -1;
		}
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
biosseek(Fs *fs, vlong off)
{
	if (off < 0) {
		print("biosseek(fs, %lld) is illegal\n", off);
		return -1;
	}
	if(fs->dev > bdrive.ndevs) {
		print("biosseek: fs->dev %d > bdrive.ndevs %d\n",
			fs->dev, bdrive.ndevs);
		return -1;
	}
	bdev[fs->dev].offset = off;	/* do not know size... (yet) */
	return off;
}

void *
biosgetfspart(int i, char *name, int chatty)
{
	static Fs fs;

	if(strcmp(name, "9fat") != 0){
		if(chatty)
			print("unknown partition bios%d!%s (use bios%d!9fat)\n",
				i, name, i);
		return nil;
	}

	fs.dev = i;
	fs.diskread = biosread;
	fs.diskseek = biosseek;

	if(dosinit(&fs) < 0){
		if(chatty)
			print("bios%d!%s does not contain a FAT file system\n",
				i, name);
		return nil;
	}
	return &fs;
}

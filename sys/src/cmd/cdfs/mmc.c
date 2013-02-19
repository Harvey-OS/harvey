/*
 * multi-media commands
 *
 * as of mmc-6, mode page 0x2a (capabilities & mechanical status) is legacy
 * and read-only, last defined in mmc-3.  mode page 5 (write parameters)
 * applies only to cd-r(w) and dvd-r(w); *-rom, dvd+* and bd-* are right out.
 */
#include <u.h>
#include <libc.h>
#include <disk.h>
#include "../scuzz/scsireq.h"
#include "dat.h"
#include "fns.h"

enum
{
	Desperate	= 0,	/* non-zero grubs around in inquiry string */

	Pagesz		= 255,

	Pagerrrecov	= 1,	/* error recovery */
	Pagwrparams	= 5,	/* (cd|dvd)-r(w) device write parameters */
	Pagcache	= 8,
	Pagcapmechsts	= 0x2a,

	Invistrack	= 0xff,	/* the invisible & incomplete track */
	Maxresptracks	= 120,	/* (1024-slop) / 8 */

	/* Pagerrrecov error control bits (buf[2]) */
	Erawre	= 1<<7,		/* recover from write errors */
	Erarre	= 1<<6,		/* recover from read errors */
	Ertb	= 1<<5,		/* transfer bad block to host */
	Errc	= 1<<4,		/* read continuous; no error recovery */
	Erper	= 1<<2,		/* post error: report recovered errors */
	Erdte	= 1<<1,		/* disable transfer on error */
	Erdcr	= 1<<0,		/* disable correction */

	Kilo	= 1000LL,
	GB	= Kilo * Kilo * Kilo,
};

typedef struct Intfeat Intfeat;
typedef struct Mmcaux Mmcaux;

static Dev mmcdev;

struct Mmcaux {
	/* drive characteristics */
	uchar	page05[Pagesz];		/* write parameters */
	int	page05ok;
	int	pagecmdsz;

	/* disc characteristics */
	long	mmcnwa;			/* next writable address (block #) */
	int	nropen;
	int	nwopen;
	vlong	ntotby;
	long	ntotbk;
};

struct Intfeat {
	int	numb;
	char	*name;
};

enum {
	Featrandwrite	= 0x20,
	Featdfctmgmt	= 0x24,
	Featwriteonce	= 0x25,
	Featedfctrpt	= 0x29,
	Featdvdrw	= 0x2a,
};

Intfeat intfeats[] = {
	Featrandwrite,	"random writable",	/* for write-and-verify */
//	0x21,		"incr. streaming writable",
	Featdfctmgmt,	"hw defect mgmt.",
	Featwriteonce,	"write once",		/* for write-and-verify */
	Featedfctrpt,	"enhanced defect reporting",
	Featdvdrw,	"dvd+rw",		/* for dvd write-and-verify */
	0x38,		"pseudo-overwrite",
//	0x40,		"bd read",
//	0x41,		"bd write",
};

/* these will be printed as user ids, so no spaces please */
static char *dvdtype[] = {
	"dvd-rom",
	"dvd-ram",
	"dvd-r",
	"dvd-rw",
	"hd-dvd-rom",
	"hd-dvd-ram",
	"hd-dvd-r",
	"type-7-unknown",
	"type-8-unknown",
	"dvd+rw",
	"dvd+r",
	"type-11-unknown",
	"type-12-unknown",
	"dvd+rw-dl",
	"dvd+r-dl",
	"type-15-unknown",
};

static int format(Drive *drive);
static int getinvistrack(Drive *drive);

static ulong
bige(void *p)
{
	uchar *a;

	a = p;
	return (a[0]<<24)|(a[1]<<16)|(a[2]<<8)|(a[3]<<0);
}

static ushort
biges(void *p)
{
	uchar *a;

	a = p;
	return (a[0]<<8) | a[1];
}

ulong
getnwa(Drive *drive)
{
	Mmcaux *aux;

	aux = drive->aux;
	return aux->mmcnwa;
}

static void
hexdump(void *v, int n)
{
	int i;
	uchar *p;

	p = v;
	for(i=0; i<n; i++){
		print("%.2ux ", p[i]);
		if((i%8) == 7)
			print("\n");
	}
	if(i%8)
		print("\n");
}

static void
initcdb(uchar *cdb, int len, int cmd)
{
	memset(cdb, 0, len);
	cdb[0] = cmd;
}

/*
 * SCSI CDBs (cmd arrays) are 6, 10, 12, 16 or 32 bytes long.
 * The mode sense/select commands implicitly refer to
 * a mode parameter list, which consists of an 8-byte
 * mode parameter header, followed by zero or more block
 * descriptors and zero or more mode pages (MMC-2 §5.5.2).
 * We'll ignore mode sub-pages.
 * Integers are stored big-endian.
 *
 * The format of the mode parameter (10) header is:
 *	ushort	mode_data_length;		// of following bytes
 *	uchar	medium_type;
 *	uchar	device_specific;
 *	uchar	reserved[2];
 *	ushort	block_descriptor_length;	// zero
 *
 * The format of the mode parameter (6) header is:
 *	uchar	mode_data_length;		// of following bytes
 *	uchar	medium_type;
 *	uchar	device_specific;
 *	uchar	block_descriptor_length;	// zero
 *
 * The format of the mode pages is:
 *	uchar	page_code_and_PS;
 *	uchar	page_len;			// of following bytes
 *	uchar	parameter[page_len];
 *
 * see SPC-3 §4.3.4.6 for allocation length and §7.4 for mode parameter lists.
 */

enum {
	Mode10parmhdrlen= 8,
	Mode6parmhdrlen	= 4,
	Modepaghdrlen	= 2,
};

static int
mmcgetpage10(Drive *drive, int page, void *v)
{
	uchar cmd[10], resp[512];
	int n, r;

	initcdb(cmd, sizeof cmd, ScmdMsense10);
	cmd[2] = page;
	cmd[8] = 255;			/* allocation length: buffer size */
	n = scsi(drive, cmd, sizeof(cmd), resp, sizeof(resp), Sread);
	if(n < Mode10parmhdrlen)
		return -1;

	r = (resp[6]<<8) | resp[7];	/* block descriptor length */
	n -= Mode10parmhdrlen + r;

	if(n < 0)
		return -1;
	if(n > Pagesz)
		n = Pagesz;

	memmove(v, &resp[Mode10parmhdrlen + r], n);
	return n;
}

static int
mmcgetpage6(Drive *drive, int page, void *v)
{
	uchar cmd[6], resp[512];
	int n;

	initcdb(cmd, sizeof cmd, ScmdMsense6);
	cmd[2] = page;
	cmd[4] = 255;			/* allocation length */

	n = scsi(drive, cmd, sizeof(cmd), resp, sizeof(resp), Sread);
	if(n < Mode6parmhdrlen)
		return -1;

	n -= Mode6parmhdrlen + resp[3];
	if(n < 0)
		return -1;
	if(n > Pagesz)
		n = Pagesz;

	memmove(v, &resp[Mode6parmhdrlen + resp[3]], n);
	return n;
}

static int
mmcsetpage10(Drive *drive, int page, void *v)
{
	uchar cmd[10], *p, *pagedata;
	int len, n;

	/* allocate parameter list, copy in mode page, fill in header */
	pagedata = v;
	assert(pagedata[0] == page);
	len = Mode10parmhdrlen + Modepaghdrlen + pagedata[1];
	p = emalloc(len);
	memmove(p + Mode10parmhdrlen, pagedata, pagedata[1]);
	/* parameter list header */
	p[0] = 0;
	p[1] = len - 2;

	/* set up CDB */
	initcdb(cmd, sizeof cmd, ScmdMselect10);
	cmd[1] = 0x10;			/* format not vendor-specific */
	cmd[8] = len;

//	print("set: sending cmd\n");
//	hexdump(cmd, 10);
//	print("parameter list header\n");
//	hexdump(p, Mode10parmhdrlen);
//	print("page\n");
//	hexdump(p + Mode10parmhdrlen, len - Mode10parmhdrlen);

	n = scsi(drive, cmd, sizeof(cmd), p, len, Swrite);

//	print("set: got cmd\n");
//	hexdump(cmd, 10);

	free(p);
	if(n < len)
		return -1;
	return 0;
}

static int
mmcsetpage6(Drive *drive, int page, void *v)
{
	uchar cmd[6], *p, *pagedata;
	int len, n;

	if (vflag)
		print("mmcsetpage6 called!\n");
	pagedata = v;
	assert(pagedata[0] == page);
	len = Mode6parmhdrlen + Modepaghdrlen + pagedata[1];
	p = emalloc(len);
	memmove(p + Mode6parmhdrlen, pagedata, pagedata[1]);

	initcdb(cmd, sizeof cmd, ScmdMselect6);
	cmd[1] = 0x10;			/* format not vendor-specific */
	cmd[4] = len;

	n = scsi(drive, cmd, sizeof(cmd), p, len, Swrite);
	free(p);
	if(n < len)
		return -1;
	return 0;
}

static int
mmcgetpage(Drive *drive, int page, void *v)
{
	Mmcaux *aux;

	aux = drive->aux;
	switch(aux->pagecmdsz) {
	case 10:
		return mmcgetpage10(drive, page, v);
	case 6:
		return mmcgetpage6(drive, page, v);
	default:
		assert(0);
	}
	return -1;
}

static int
mmcsetpage(Drive *drive, int page, void *v)
{
	Mmcaux *aux;

	aux = drive->aux;
	switch(aux->pagecmdsz) {
	case 10:
		return mmcsetpage10(drive, page, v);
	case 6:
		return mmcsetpage6(drive, page, v);
	default:
		assert(0);
	}
	return -1;
}

int
mmcstatus(Drive *drive)
{
	uchar cmd[12];

	initcdb(cmd, sizeof cmd, ScmdCDstatus);		/* mechanism status */
	return scsi(drive, cmd, sizeof(cmd), nil, 0, Sread);
}

void
mmcgetspeed(Drive *drive)
{
	int n, maxread, curread, maxwrite, curwrite;
	uchar buf[Pagesz];

	memset(buf, 0, 22);
	n = mmcgetpage(drive, Pagcapmechsts, buf);	/* legacy page */
	if (n < 22) {
		if (vflag)
			fprint(2, "no Pagcapmechsts mode page!\n");
		return;
	}
	maxread =   (buf[8]<<8)|buf[9];
	curread =  (buf[14]<<8)|buf[15];
	maxwrite = (buf[18]<<8)|buf[19];
	curwrite = (buf[20]<<8)|buf[21];

	if(maxread && maxread < 170 || curread && curread < 170)
		return;			/* bogus data */

	drive->readspeed = curread;
	drive->writespeed = curwrite;
	drive->maxreadspeed = maxread;
	drive->maxwritespeed = maxwrite;
}

static int
getdevtype(Drive *drive)
{
	int n;
	uchar cmd[6], resp[Pagesz];

	initcdb(cmd, sizeof cmd, ScmdInq);
	cmd[3] = sizeof resp >> 8;
	cmd[4] = sizeof resp;
	n = scsi(drive, cmd, sizeof(cmd), resp, sizeof resp, Sread);
	if (n < 8)
		return -1;
	return resp[0] & 037;
}

static int
start(Drive *drive, int code)
{
	uchar cmd[6];

	initcdb(cmd, sizeof cmd, ScmdStart);
	cmd[4] = code;
	return scsi(drive, cmd, sizeof(cmd), cmd, 0, Snone);
}

static int
setcaching(Drive *drive)
{
	int n;
	uchar buf[Pagesz];

	/*
	 * we can't actually control caching much.
	 * see SBC-2 §6.3.3 but also MMC-6 §7.6.
	 *
	 * should set read ahead, MMC-6 §6.37; seems to control caching.
	 */
	n = mmcgetpage(drive, Pagcache, buf);
	if (n < 3)
		return -1;
	/* n == 255; buf[1] == 10 (10 bytes after buf[1]) */
	buf[0] &= 077;		/* clear reserved bits, MMC-6 §7.2.3 */
	assert(buf[0] == Pagcache);
	assert(buf[1] >= 10);
	buf[2] = Ccwce;
	if (mmcsetpage(drive, Pagcache, buf) < 0) {
		if (vflag)
			print("mmcprobe: cache control NOT set\n");
		return -1;
	}
	return 0;
}

Drive*
mmcprobe(Scsi *scsi)
{
	Mmcaux *aux;
	Drive *drive;
	uchar buf[Pagesz];
	int cap;

	if (vflag)
		print("mmcprobe: inquiry: %s\n", scsi->inquire);

	drive = emalloc(sizeof(Drive));
	drive->Scsi = *scsi;
	drive->Dev = mmcdev;
	drive->invistrack = -1;
	getinvistrack(drive);

	aux = emalloc(sizeof(Mmcaux));
	drive->aux = aux;

	scsiready(drive);
	drive->type = getdevtype(drive);
	if (drive->type != TypeCD) {
		werrstr("not an mmc device");
		free(aux);
		free(drive);
		return nil;
	}

	/*
	 * drive is an mmc device; learn what we can about it
	 * (as opposed to the disc in it).
	 */

	start(drive, 1);
	/* attempt to read CD capabilities page, but it's now legacy */
	if(mmcgetpage10(drive, Pagcapmechsts, buf) >= 0)
		aux->pagecmdsz = 10;
	else if(mmcgetpage6(drive, Pagcapmechsts, buf) >= 0)
		aux->pagecmdsz = 6;
	else {
		if (vflag)
			fprint(2, "no Pagcapmechsts mode page!\n");
		werrstr("can't read mode page %d!", Pagcapmechsts);
		free(aux);
		free(drive);
		return nil;
	}

	cap = 0;
	if(buf[Capwrite] & (Capcdr|Capcdrw|Capdvdr|Capdvdram) ||
	    buf[Capmisc] & Caprw)
		cap |= Cwrite;
	if(buf[Capmisc] & Capcdda)	/* CD-DA commands supported? */
		cap |= Ccdda;		/* not used anywhere else */

//	print("read %d max %d\n", biges(buf+14), biges(buf+8));
//	print("write %d max %d\n", biges(buf+20), biges(buf+18));

	/* cache optional page 05 (write parameter page) */
	if(/* (cap & Cwrite) && */
	    mmcgetpage(drive, Pagwrparams, aux->page05) >= 0) {
		aux->page05ok = 1;
		cap |= Cwrite;
		if (vflag)
			fprint(2, "mmcprobe: got page 5, assuming drive can write\n");
	} else {
		if (vflag)
			fprint(2, "no Pagwrparams mode page!\n");
		cap &= ~Cwrite;
	}
	drive->cap = cap;

	mmcgetspeed(drive);
	setcaching(drive);
	return drive;
}

static char *tracktype[] = {	/* indices are track modes (Tm*) */
	"audio cdda",
	"2 audio channels",
	"2",
	"3",
	"data, recorded uninterrupted",
	"data, recorded interrupted",
};

/*
 * figure out the first writable block, if we can, into drive->aux->mmcnwa.
 * resp must be from ScmdRtrackinfo.
 */
static long
gettracknwa(Drive *drive, int t, ulong beg, uchar *resp)
{
	long newnwa;
	Mmcaux *aux;

	aux = drive->aux;
	if(resp[7] & 1) {			/* nwa valid? */
		newnwa = bige(&resp[12]);
		if (newnwa >= 0)
			if (aux->mmcnwa < 0)
				aux->mmcnwa = newnwa;
			else if (aux->mmcnwa != newnwa)
				fprint(2, "nwa is %ld but invis track starts blk %ld\n",
					newnwa, aux->mmcnwa);
	}
	/* resp[6] & (1<<7) of zero: invisible track */
	if(t == Invistrack || t == drive->invistrack)
		if (aux->mmcnwa < 0)
			aux->mmcnwa = beg;
		else if (aux->mmcnwa != beg)
			fprint(2, "invis track starts blk %ld but nwa is %ld\n",
				beg, aux->mmcnwa);
	if (vflag && aux->mmcnwa >= 0)
		print(" nwa %lud", aux->mmcnwa);
	return 0;
}

/*
 * map tmode to type & bs.
 */
static void
gettypebs(uchar tmode, int t, int i, int *typep, int *bsp)
{
	int type, bs;

	if(vflag)
		print("track %d type %d (%s)", t, tmode,
			(tmode < nelem(tracktype)? tracktype[tmode]: "**GOK**"));
	type = TypeNone;
	bs = BScdda;
	switch(tmode){
	case Tmcdda:
		type = TypeAudio;
		bs = BScdda;
		break;
	case Tm2audio:	/* 2 audio channels, with pre-emphasis 50/15 μs */
		if(vflag)
			print("audio channels with preemphasis on track %d "
				"(u%.3d)\n", t, i);
		type = TypeNone;
		break;
	case Tmunintr:		/* data track, recorded uninterrupted */
	case Tmintr:		/* data track, recorded interrupted */
		/* treat Tmintr (5) as cdrom; it's probably dvd or bd */
		type = TypeData;
		bs = BScdrom;
		break;
	default:
		if(vflag)
			print("unknown track type %d\n", tmode);
		break;
	}
	*bsp = bs;
	*typep = type;
}

/* t is a track number on disc, i is an index into drive->track[] for result */
static int
mmctrackinfo(Drive *drive, int t, int i)
{
	int n, type, bs;
	ulong beg, size;
	uchar tmode;
	uchar cmd[10], resp[255];
	Track *track;

	initcdb(cmd, sizeof cmd, ScmdRtrackinfo);
	cmd[1] = 1;			/* address below is logical track # */
	cmd[2] = t>>24;
	cmd[3] = t>>16;
	cmd[4] = t>>8;
	cmd[5] = t;
	cmd[7] = sizeof(resp)>>8;
	cmd[8] = sizeof(resp);
	n = scsi(drive, cmd, sizeof(cmd), resp, sizeof(resp), Sread);
	if(n < 28) {
		if(vflag)
			print("trackinfo %d fails n=%d: %r\n", t, n);
		return -1;
	}

	tmode = resp[5] & 0x0D;
//	dmode = resp[6] & 0x0F;

	gettypebs(tmode, t, i, &type, &bs);

	beg  = bige(&resp[8]);
	size = bige(&resp[24]);

	track = &drive->track[i];
	track->mtime = drive->changetime;
	track->beg = beg;
	track->end = beg + size;
	track->type = type;
	track->bs = bs;
	track->size = (vlong)(size-2) * bs;	/* -2: skip lead out */

	if(resp[6] & (1<<6)) {			/* blank? */
		track->type = TypeBlank;
		drive->writeok = Yes;
	}

	if(vflag)
		print(" start %lud end %lud", beg, beg + size - 1);
	gettracknwa(drive, t, beg, resp);
	if (vflag)
		print("\n");
	return 0;
}

/* this may fail for blank media */
static int
mmcreadtoc(Drive *drive, int type, int track, void *data, int nbytes)
{
	uchar cmd[10];

	initcdb(cmd, sizeof cmd, ScmdRTOC);
	cmd[1] = type;				/* msf bit & reserved */
	cmd[2] = Tocfmttoc;
	cmd[6] = track;				/* track/session */
	cmd[7] = nbytes>>8;
	cmd[8] = nbytes;

	/*
	 * printing iounit(drive->Scsi.rawfd) here yields
	 *	iounit(3) = 0;		# for local access
	 *	iounit(3) = 65512;	# for remote access via /mnt/term
	 */
	return scsi(drive, cmd, sizeof(cmd), data, nbytes, Sread);
}

static Msf
rdmsf(uchar *p)
{
	Msf msf;

	msf.m = p[0];
	msf.s = p[1];
	msf.f = p[2];
	return msf;
}

static int
getdiscinfo(Drive *drive, uchar resp[], int resplen)
{
	int n;
	uchar cmd[10];

	initcdb(cmd, sizeof cmd, ScmdRdiscinfo);
	cmd[7] = resplen>>8;
	cmd[8] = resplen;
	n = scsi(drive, cmd, sizeof(cmd), resp, resplen, Sread);
	if(n < 24) {
		if(n >= 0)
			werrstr("rdiscinfo returns %d", n);
		else if (vflag)
			fprint(2, "read disc info failed\n");
		return -1;
	}
	if (vflag)
		fprint(2, "read disc info succeeded\n");
	assert((resp[2] & 0340) == 0);			/* data type 0 */
	drive->erasable = ((resp[2] & 0x10) != 0);	/* -RW? */
	return n;
}

int
isbitset(uint bit, uchar *map)
{
	return map[bit/8] & (1 << (bit%8));
}

/*
 * set read-write error recovery mode page 1 (10 bytes), mmc-6 §7.3.
 *
 * requires defect management feature (0x24) mmc-6 §5.3.13 (mandatory on bd,
 * but has to be enabled by allocating spares with FORMAT UNIT command)
 * or enhanced defect reporting feature (0x29) mmc-6 §5.3.17,
 * and they are mutually exclusive.
 */
static int
seterrrecov(Drive *drive)
{
	int n;
	uchar buf[Pagesz];

	if (!isbitset(Featdfctmgmt, drive->features) &&
	    !isbitset(Featedfctrpt, drive->features)) {
		fprint(2, "defect mgmt. and enhanced defect reporting disabled!\n");
		return -1;
	}
	n = mmcgetpage(drive, Pagerrrecov, buf);
	if (n < 3)
		return -1;
	/* n == 255; buf[1] == 10 (10 bytes after buf[1]) */
	/*
	 * error recovery page as read:
	 * 0: 01 (page: error recovery)
	 * 1: 0a (length: 10)
	 * 2: c0 (error control bits: 0300 == Erawre|Erarre)
	 * 3: 20 (read retry count: 32)
	 * 4: 00
	 * 5: 00
	 * 6: 00
	 * 7: 00
	 * 8: 01 (write retry count: 1)
	 * 9: 00 00 00 (error reporting window size)
	 */
	buf[0] &= ~(1<<6);			/* clear reserved bit */
	assert((buf[0] & 077) == Pagerrrecov);
	assert(buf[1] >= 10);

	buf[2] = Erawre | Erarre;	/* default: Erawre; can't set Ertb */
	if (isbitset(Featedfctrpt, drive->features))
		buf[7] = 1;  /* emcdr: 1==recover, don't tell us; default 0 */

//	buf[3] = 32;	/* rd retry count; default 32; arbitrary */
//	buf[8] = 1;	/* wr retry count; default 1; arbitrary */
//	memset(buf+9, 0, 3); /* err reporting win siz: 0 == no tsr; default 0 */

	if (mmcsetpage(drive, Pagerrrecov, buf) < 0) {
		if (vflag)
			fprint(2, "error recovery NOT set\n");
		return -1;
	}
	if (vflag)
		fprint(2, "error recovery set\n");
	return 0;
}

char *
featname(int feat)
{
	Intfeat *ifp;

	for (ifp = intfeats; ifp < intfeats + nelem(intfeats); ifp++)
		if (feat == ifp->numb)
			return ifp->name;
	return nil;
}

static int
prof2mmctype(Drive *drive, ushort prof)
{
	if(prof == 0 || prof == 0xffff)		/* none or whacko? */
		return -1;
	if(drive->mmctype != Mmcnone)
		return -1;
	switch (prof >> 4) {
	case 0:
		drive->mmctype = Mmccd;
		break;
	case 1:
		if (prof == 0x1a || prof == 0x1b)
			drive->mmctype = Mmcdvdplus;
		else
			drive->mmctype = Mmcdvdminus;
		break;
	case 2:
		drive->mmctype = Mmcdvdplus;	/* dual layer */
		break;
	case 4:
		drive->mmctype = Mmcbd;
		if (vflag)
			fprint(2, "getconf profile for bd = %#x\n", prof);
		/*
		 * further decode prof to set writability flags.
		 * mostly for Pioneer BDR-206M.
		 */
		drive->erasable = drive->recordable = No;
		switch (prof) {
		case 0x40:
			break;
		case 0x41:
		case 0x42:
			drive->recordable = Yes;
			break;
		case 0x43:
			drive->erasable = Yes;
			break;
		}
		break;
	case 5:
		drive->mmctype = Mmcdvdminus;	/* hd dvd, obs. */
		break;
	}
	return 0;
}

static void
notefeats(Drive *drive, uchar *p, ulong datalen)
{
	int left, len;
	uint feat;
	char *ftnm;

	if (vflag)
		fprint(2, "features: ");
	for (left = datalen; left > 0; left -= len, p += len) {
		feat = p[0]<<8 | p[1];
		len = 4 + p[3];
		ftnm = featname(feat);
		if (vflag && ftnm)
			fprint(2, "%#ux (%s) curr %d\n", feat, ftnm, p[2] & 1);
		if (feat >= Maxfeatures) {	/* could be vendor-specific */
			if (vflag)
				fprint(2, "feature %d too big for bit map\n",
					feat);
		} else if (p[2] & 1)
			drive->features[feat/8] |= 1 << (feat%8);
	}
}

static int
getconfcmd(Drive *drive, uchar *resp, int respsz)
{
	int n;
	ulong datalen;
	uchar cmd[10];

	initcdb(cmd, sizeof cmd, Scmdgetconf);
	cmd[3] = 0;			/* start with profile list feature */
	cmd[7] = respsz >> 8;
	cmd[8] = respsz;
	n = scsi(drive, cmd, sizeof cmd, resp, respsz, Sread);
	if (n < 0) {
		if(vflag)
			fprint(2, "get config cmd failed\n");
		return -1;
	}
	if (n < 4)
		return -1;
	datalen = GETBELONG(resp+0);
	if (datalen < 8)
		return -1;
	return datalen;
}

static int
getconf(Drive *drive)
{
	ushort prof;
	long datalen;
	uchar resp[64*1024 - 8];		/* 64k-8 max, 450 typical */

	memset(drive->features, 0, sizeof drive->features);
	datalen = getconfcmd(drive, resp, sizeof resp);
	if(datalen < 0)
		return -1;

	/*
	 * features start with an 8-byte header:
	 * ulong datalen, ushort reserved, ushort current profile.
	 * profile codes (table 92) are: 0 reserved, 1-7 legacy, 8-0xf cd,
	 * 0x10-0x1f 0x2a-0x2b dvd*, 0x40-0x4f bd, 0x50-0x5f hd dvd,
	 * 0xffff whacko.
	 *
	 * this is followed by multiple feature descriptors:
	 * ushort code, uchar bits, uchar addnl_len, addnl_len bytes.
	 */
	prof = resp[6]<<8 | resp[7];
	if (prof2mmctype(drive, prof) < 0)
		return datalen;

	/*
	 * for bd-r, establish non-default recording mode before first write,
	 * with FORMAT UNIT command to allocate spares (BD MMC set
	 * description, v1.1, §4.4.4).
	 */
	if (drive->mmctype == Mmcbd && drive->recordable == Yes &&
	    drive->erasable == No) {
		format(drive);	/* no-op with error if already been done */
		/* read config again, now that bd-r has spares */
		datalen = getconfcmd(drive, resp, sizeof resp);
		if(datalen < 0)
			return -1;
	}

	/* skip header to find feature list */
	notefeats(drive, resp + 8, datalen - 8);

	if (drive->mmctype == Mmcbd)
		seterrrecov(drive);
	return datalen;
}

static int
getdvdstruct(Drive *drive)
{
	int n, cat;
	uchar cmd[12], resp[Pagesz];

	initcdb(cmd, sizeof cmd, ScmdReadDVD); /* actually, read disc structure */
	cmd[1] = 0;			/* media type: dvd */
	cmd[7] = 0;			/* format code: physical format */
	cmd[8] = sizeof resp >> 8;	/* allocation length */
	cmd[9] = sizeof resp;
	n = scsi(drive, cmd, sizeof(cmd), resp, sizeof resp, Sread);
	if (n < 7) {
		if(vflag)
			fprint(2, "read disc structure (dvd) cmd failed\n");
		return -1;
	}

	/* resp[0..1] is resp length */
	cat = (resp[4] & 0xf0) >> 4;	/* disk category, MMC-6 §6.22.3.2.1 */
	if (vflag)
		fprint(2, "dvd type is %s\n", dvdtype[cat]);
	drive->dvdtype = dvdtype[cat];
	/* write parameters mode page may suffice to compute writeok for dvd */
	drive->erasable = drive->recordable = No;
	/*
	 * the layer-type field is a *bit array*,
	 * though an enumeration of types would make more sense,
	 * since the types are exclusive, not orthogonal.
	 */
	if (resp[6] & (1<<2))			/* rewritable? */
		drive->erasable = Yes;
	else if (resp[6] & (1<<1))		/* recordable once? */
		drive->recordable = Yes;
	/* else it's a factory-pressed disk */
	drive->mmctype = (cat >= 8? Mmcdvdplus: Mmcdvdminus);
	return 0;
}

/*
 * ugly hack to divine device type from inquiry string as last resort.
 * mostly for Pioneer BDR-206M.
 */
static int
bdguess(Drive *drive)
{
	if (drive->mmctype == Mmcnone) {
		if (strstr(drive->Scsi.inquire, "BD") == nil)
			return -1;
		if (vflag)
			fprint(2, "drive probably a BD (from inquiry string)\n");
		drive->mmctype = Mmcbd;
	} else if (drive->mmctype == Mmcbd) {
		if (drive->erasable != Unset && drive->recordable != Unset)
			return 0;
	} else
		return -1;

	drive->recordable = drive->writeok = No;
	if (strstr(drive->Scsi.inquire, "RW") != nil) {
		if (vflag)
			fprint(2, "drive probably a burner (from inquiry string)\n");
		drive->recordable = drive->writeok = Yes;
		if (drive->erasable == Unset) {	/* set by getdiscinfo perhaps */
			drive->erasable = No;	/* no way to tell, alas */
			if (vflag)
				fprint(2, "\tassuming -r not -re\n");
		}
	} else {
		if (drive->erasable == Unset)
			drive->erasable = No;
	}
	if (drive->erasable == Yes)
		drive->recordable = No;		/* mutually exclusive */
	return 0;
}

static int
getbdstruct(Drive *drive)
{
	int n;
	uchar cmd[12], resp[4+4096];
	uchar *di, *body;

	initcdb(cmd, sizeof cmd, ScmdReadDVD); /* actually, read disc structure */
	cmd[1] = 1;			/* media type: bd */
	/* cmd[6] is layer #, 0 is first */
	cmd[7] = 0;			/* format code: disc info */
	cmd[8] = sizeof resp >> 8;	/* allocation length */
	cmd[9] = sizeof resp;
	n = scsi(drive, cmd, sizeof(cmd), resp, sizeof resp, Sread);
	if(n < 0) {
		if(vflag)
			fprint(2, "read disc structure (bd) cmd failed\n");
		return -1;
	}

	/*
	 * resp[0..1] is resp length (4100); 2 & 3 are reserved.
	 * there may be multiple disc info structs of 112 bytes each.
	 * disc info (di) starts at 4.  di[0..7] are header, followed by body.
	 * body[0..2] is bd type (disc type identifier):
	 * BDO|BDW|BDR, MMC-6 §6.22.3.3.1.  The above scsi command should
	 * fail on DVD drives, but some seem to ignore media type
	 * and return successfully, so verify that it's a BD drive.
	 */
	di = resp + 4;
	body = di + 8;
	n -= 4 + 8;
	if (n < 3 || di[0] != 'D' || di[1] != 'I' ||
	    body[0] != 'B' || body[1] != 'D') {
		if(vflag)
			fprint(2, "it's not a bd\n");
		return -1;
	}
	if (vflag)
		fprint(2, "read disc structure (bd) succeeded; di format %d\n",
			di[2]);

	drive->erasable = drive->recordable = No;
	switch (body[2]) {
	case 'O':				/* read-Only */
		break;
	case 'R':				/* Recordable */
		drive->recordable = Yes;
		break;
	case 'W':				/* reWritable */
		drive->erasable = Yes;
		break;
	default:
		fprint(2, "%s: unknown bd type BD%c\n", argv0, body[2]);
		return -1;
	}
	/* printed  getbdstruct: di bytes 98 di units/layers 32 */
	// fprint(2, "getbdstruct: di bytes %d di units/layers %d\n", di[6], di[3]);
	drive->mmctype = Mmcbd;
	return 0;
}

/*
 * infer endings from the beginnings of other tracks.
 */
static int
mmcinfertracks(Drive *drive, int first, int last)
{
	int i;
	uchar resp[1024];
	ulong tot;
	Track *t;

	if (vflag)
		fprint(2, "inferring tracks from toc\n");
	for(i = first; i <= last; i++) {
		memset(resp, 0, sizeof(resp));
		if(mmcreadtoc(drive, 0, i, resp, sizeof(resp)) < 0) {
			last = i - 1;
			if (last < 1)
				last = 1;
			break;
		}
		t = &drive->track[i-first];
		t->mtime = drive->changetime;
		t->type = TypeData;
		t->bs = BScdrom;
		t->beg = bige(resp+8);
		if(!(resp[5] & 4)) {
			t->type = TypeAudio;
			t->bs = BScdda;
		}
	}

	if((long)drive->track[0].beg < 0)  /* i've seen negative track 0's */
		drive->track[0].beg = 0;

	tot = 0;
	memset(resp, 0, sizeof(resp));
	/* 0xAA is lead-out */
	if(mmcreadtoc(drive, 0, 0xAA, resp, sizeof(resp)) < 0)
		print("bad mmcreadtoc\n");
	if(resp[6])
		tot = bige(resp+8);

	t = nil;
	for(i=last; i>=first; i--) {
		t = &drive->track[i-first];
		t->end = tot;
		tot = t->beg;
		if(t->end <= t->beg)
			t->beg = t->end = 0;
		/* -2: skip lead out */
		t->size = (t->end - t->beg - 2) * (vlong)t->bs;
	}
	fprint(2, "infertracks: nwa should be %,lld; tot = %,ld\n", t->size, tot);
	return last;
}

static void
initdrive(Drive *drive)
{
	int i;
	Mmcaux *aux;

	drive->ntrack = 0;
	drive->nameok = 0;
	drive->nchange = drive->Scsi.nchange;
	drive->changetime = drive->Scsi.changetime;
	drive->writeok = No;
	drive->erasable = drive->recordable = Unset;
	drive->laysfx = nil;
	getinvistrack(drive);
	aux = drive->aux;
	aux->mmcnwa = -1;
	aux->nropen = aux->nwopen = 0;
	aux->ntotby = aux->ntotbk = 0;

	for(i=0; i<nelem(drive->track); i++){
		memset(&drive->track[i].mbeg, 0, sizeof(Msf));
		memset(&drive->track[i].mend, 0, sizeof(Msf));
	}
}

static long
computenwa(Drive *drive, int first, int last)
{
	int i;
	ulong nwa;
	Track *t;

	/*
	 * work through the tracks in reverse order (last to first).
	 * assume last is the to-be-written invisible track.
	 */
	nwa = 0;
	for(i=last-1; i>=first; i--) {
		t = &drive->track[i-first];
		nwa += t->end - t->beg;
	}
	return nwa;
}

static int
findntracks(Drive *drive, int *firstp, int *lastp)
{
	int i, n, first, last;
	uchar resp[1024];

	if((n = mmcreadtoc(drive, Msfbit, 0, resp, sizeof(resp))) < 4) {
		/*
		 * it could be a blank disc.  in case it's a blank disc in a
		 * cd-rw drive, use readdiscinfo to try to find the track info.
		 */
		if(getdiscinfo(drive, resp, sizeof(resp)) < 7)
			return -1;
		assert((resp[2] & 0340) == 0);	/* data type 0 */
		if(resp[4] != 1)
			print("multi-session disc %d\n", resp[4]);
		drive->writeok = Yes;

		/*
		 * some drives (e.g., LG's BDs) report only 1 track on an
		 * unfinalized disc, so probe every track.  the nwa reported
		 * by such drives may be wrong (e.g., 0), so compute that too.
		 */
		first = resp[3];
		last = resp[6];
		if(vflag)
			print("rdiscinfo tracks %d-%d (last==1 means TBD)\n",
				first, last);
		if(last == 1)
			last = Maxresptracks;	/* probe the tracks */
	} else {
		first = resp[2];
		last = resp[3];
		if(vflag)
			print("toc tracks %d-%d (last==1 means TBD)\n",
				first, last);
		if(last == 1)
			last = Maxresptracks;	/* probe the tracks */

		if(n >= 4+8*(last-first+2)) {
			/* resp[4 + i*8 + 2] is track # */
			/* <=: track[last-first+1] = end */
			for(i=0; i<=last-first+1; i++)
				drive->track[i].mbeg = rdmsf(resp+4+i*8+5);
			for(i=0; i<last-first+1; i++)
				drive->track[i].mend = drive->track[i+1].mbeg;
		}
	}
	*firstp = first;
	*lastp = last;
	return 0;
}

/* needs drive->mmctype to be already set */
static int
alltrackinfo(Drive *drive, int first, int last)
{
	int i;
	long nwa;
	vlong cap;
	char *osfx;
	Mmcaux *aux;
	Track *track;

	for(i = first; i <= last; i++)
		if (mmctrackinfo(drive, i, i - first) < 0) {
			last = i - 1;
			if (last < 1)
				last = 1;
			break;
		}
	track = &drive->track[last - first];
	drive->end = track->end;
	if (drive->mmctype == Mmcbd) {
		/* allow for lower apparent capacity due to reserved spares */
		cap = (vlong)track->end * track->bs;
		osfx = drive->laysfx;
		if (cap >= 101*GB)
			drive->laysfx = "-ql";		/* 128GB nominal */
		else if (cap >= 51*GB)
			drive->laysfx = "-tl";		/* 100GB nominal */
		else if (cap >= 26*GB)
			drive->laysfx = "-dl";		/* 50GB nominal */
		else
			drive->laysfx = "";		/* 25GB nominal */
		if (vflag)
			fprint(2, "capacity %,lud sectors (%,lld bytes); bd%s\n",
				track->end, cap, drive->laysfx);
		if (osfx == nil || strcmp(osfx, drive->laysfx) != 0)
			drive->relearn = 1;
	}
	if (drive->mmctype == Mmcnone)
		return last;				/* no disc */
	nwa = computenwa(drive, first, last);
	aux = drive->aux;
	if (vflag)
		fprint(2, "nwa from drive %,ld; computed nwa %,ld\n",
			aux->mmcnwa, nwa);
	if (aux->mmcnwa == -1 && nwa == 0)
		return last;			/* probably a blank disc */
	if (aux->mmcnwa == -1)
		fprint(2, "%s: disc is full\n", argv0);
	/* reconcile differing nwas */
	if (aux->mmcnwa != nwa) {
		fprint(2, "%s: nwa from drive %,ld != computed nwa %,ld\n",
			argv0, aux->mmcnwa, nwa);
		fprint(2, "\tbe careful!  assuming computed nwa\n");
		/* the invisible track may still start at the old nwa. */
		// aux->mmcnwa = nwa;
	}
	return last;
}

static int
findtracks(Drive *drive, int first, int last)
{
	if(first == 0 && last == 0)
		first = 1;

	if(first <= 0 || first >= Maxtrack) {
		werrstr("first table %d not in range", first);
		return -1;
	}
	if(last <= 0 || last >= Maxtrack) {
		werrstr("last table %d not in range", last);
		return -1;
	}

	if(!(drive->cap & Cwrite))
		last = mmcinfertracks(drive, first, last);
	else
		last = alltrackinfo(drive, first, last);
	drive->firsttrack = first;
	drive->ntrack = last+1-first;
	if (vflag)
		fprint(2, "this time for sure: first %d last %d\n", first, last);
	return 0;
}

static void
finddisctype(Drive *drive, int first, int last)
{
	char *ty;

	/* deduce disc type */
	drive->mmctype = Mmcnone;
	drive->dvdtype = nil;
	getdvdstruct(drive);
	getconf(drive);
	getbdstruct(drive);
	if (Desperate)
		bdguess(drive);
	if (drive->mmctype == Mmcnone)
		drive->mmctype = Mmccd;		/* by default */
	if (drive->recordable == Yes || drive->erasable == Yes)
		drive->writeok = Yes;

	if (vflag) {
		fprint(2, "writeok %d", drive->writeok);
		if (drive->recordable != Unset)
			fprint(2, " recordable %d", drive->recordable);
		if (drive->erasable != Unset)
			fprint(2, " erasable %d", drive->erasable);
		fprint(2, "\n");
		fprint(2, "first %d last ", first);
		if (last == Maxresptracks)
			print("TBD\n");
		else
			print("%d\n", last);

		ty = disctype(drive);
		fprint(2, "it's a %s disc.", ty);
		free(ty);
		if (drive->mmctype == Mmcbd && drive->laysfx == nil)
			fprint(2, "  (number of layers isn't known yet.)");
		fprint(2, "\n\n");
	}
}

/* this gets called a lot from main.c's 9P routines */
static int
mmcgettoc(Drive *drive)
{
	int first, last;
	uchar resp[1024];

	/*
	 * if someone has swapped the cd,
	 * mmcreadtoc will get ``medium changed'' and the scsi routines
	 * will set nchange and changetime in the scsi device.
	 */
	mmcreadtoc(drive, 0, 0, resp, sizeof(resp));
	if(drive->Scsi.changetime == 0) {	/* no media present */
		drive->mmctype = Mmcnone;
		drive->ntrack = 0;
		return 0;
	}
	/*
	 * if the disc doesn't appear to be have been changed, and there
	 * is a disc in this drive, there's nothing to do (the common case).
	 */
	if(drive->nchange == drive->Scsi.nchange && drive->changetime != 0 &&
	    !drive->relearn)
		return 0;
	drive->relearn = 0;

	/*
	 * the disc in the drive may have just been changed,
	 * so rescan it and relearn all about it.
	 */
	if (vflag &&
	    (drive->nchange != drive->Scsi.nchange || drive->changetime == 0))
		fprint(2, "\nnew disc in drive\n");
	initdrive(drive);

	if (findntracks(drive, &first, &last) < 0)
		return -1;
	/*
	 * we would like to call findtracks before finddisctype so that
	 * we can format bd-rs at the right capacity with explicit spares
	 * ratio given, but findtracks needs the disc type to be already set.
	 * format is called from getconf from finddisctype before getbdstruct.
	 * luckily, FORMAT UNIT (thus format()) doesn't seem to care when we
	 * don't provide an explicit spares ratio.
	 */
	/* calls getdvdstruct, getconf and getbdstruct, in that order */
	finddisctype(drive, first, last);	/* formats bds at first use */
	return findtracks(drive, first, last);
}

static void
settrkmode(uchar *p, int mode)
{
	p[Wptrkmode] &= ~0xf;
	p[Wptrkmode] |= mode;
}

/*
 * this uses page 5, which is optional.
 */
static int
mmcsetbs(Drive *drive, int bs)
{
	uchar *p;
	Mmcaux *aux;

	aux = drive->aux;
	if (!aux->page05ok)
		return 0;			/* harmless; assume 2k */

	p = aux->page05;
	/*
	 * establish defaults.
	 */
	p[0] &= 077;			/* clear reserved bits, MMC-6 §7.2.3 */
	p[Wpwrtype] = Bufe | Wttrackonce;
//	if(testonlyflag)
//		p[Wpwrtype] |= 0x10;	/* test-write */
	/* assume dvd values as defaults */
	settrkmode(p, Tmintr);
	p[Wptrkmode] |= Msnext;
	p[Wpdatblktype] = Db2kdata;
	p[Wpsessfmt] = Sfdata;

	switch(drive->mmctype) {
	case Mmcdvdplus:
	case Mmcbd:
		break;
	case Mmcdvdminus:
		/* dvd-r can only do disc-at-once or incremental */
		p[Wpwrtype] = Bufe | Wtsessonce;
		break;
	case Mmccd:
		settrkmode(p, Tmunintr); /* data track, uninterrupted */
		switch(bs){
		case BScdda:
			/* 2 audio channels without pre-emphasis */
			settrkmode(p, Tmcdda);	/* should be Tm2audio? */
			p[Wpdatblktype] = Dbraw;
			break;
		case BScdrom:
			break;
		case BScdxa:
			p[Wpdatblktype] = Db2336;
			p[Wpsessfmt] = Sfcdxa;
			break;
		default:
			fprint(2, "%s: unknown CD type; bs %d\n", argv0, bs);
			assert(0);
		}
		break;
	default:
		fprint(2, "%s: unknown disc sub-type %d\n",
			argv0, drive->mmctype);
		break;
	}
	if(mmcsetpage(drive, Pagwrparams, p) < 0) {
		if (vflag)
			fprint(2, "mmcsetbs: could NOT set write parameters page\n");
		return -1;
	}
	return 0;
}

/*
 * `read cd' only works for CDs; for everybody else,
 * we'll try plain `read (12)'.  only use read cd if it's
 * a cd drive with a cd in it and we're not reading data
 * (e.g., reading audio).
 */
static int
fillread12cdb(Drive *drive, uchar *cmd, long nblock, ulong off, int bs)
{
	if (drive->type == TypeCD && drive->mmctype == Mmccd && bs != BScdrom) {
		initcdb(cmd, 12, ScmdReadcd);
		cmd[6] = nblock>>16;
		cmd[7] = nblock>>8;
		cmd[8] = nblock>>0;
		cmd[9] = 0x10;
		switch(bs){
		case BScdda:
			cmd[1] = 0x04;
			break;
		case BScdrom:
			cmd[1] = 0x08;
			break;
		case BScdxa:
			cmd[1] = 0x0C;
			break;
		default:
			werrstr("unknown bs %d", bs);
			return -1;
		}
	} else {				/* e.g., TypeDA */
		initcdb(cmd, 12, ScmdRead12);
		cmd[6] = nblock>>24;
		cmd[7] = nblock>>16;
		cmd[8] = nblock>>8;
		cmd[9] = nblock;
		// cmd[10] = 0x80;		/* streaming */
	}
	cmd[2] = off>>24;
	cmd[3] = off>>16;
	cmd[4] = off>>8;
	cmd[5] = off>>0;
	return 0;
}

/* off is a block number */
static long
mmcread(Buf *buf, void *v, long nblock, ulong off)
{
	int bs;
	long n, nn;
	uchar cmd[12];
	Drive *drive;
	Otrack *o;

	o = buf->otrack;
	drive = o->drive;
	bs = o->track->bs;
	off += o->track->beg;

	if(nblock >= (1<<10)) {
		werrstr("mmcread too big");
		if(vflag)
			fprint(2, "mmcread too big\n");
		return -1;
	}

	/* truncate nblock modulo size of track */
	if(off > o->track->end - 2) {
		werrstr("read past end of track");
		if(vflag)
			fprint(2, "end of track (%ld->%ld off %ld)",
				o->track->beg, o->track->end - 2, off);
		return -1;
	}
	if(off == o->track->end - 2)
		return 0;

	if(off+nblock > o->track->end - 2)
		nblock = o->track->end - 2 - off;

	if (fillread12cdb(drive, cmd, nblock, off, bs) < 0)
		return -1;

	n = nblock*bs;
	nn = scsi(drive, cmd, sizeof(cmd), v, n, Sread);
	if(nn != n) {
		if(nn != -1)
			werrstr("short read %ld/%ld", nn, n);
		if(vflag)
			print("read off %lud nblock %ld bs %d failed\n",
				off, nblock, bs);
		return -1;
	}
	return nblock;
}

static Otrack*
mmcopenrd(Drive *drive, int trackno)
{
	Otrack *o;
	Mmcaux *aux;

	if(trackno < 0 || trackno >= drive->ntrack) {
		werrstr("track number out of range");
		return nil;
	}

	aux = drive->aux;
	if(aux->nwopen) {
		werrstr("disk in use for writing");
		return nil;
	}

	o = emalloc(sizeof(Otrack));
	o->drive = drive;
	o->track = &drive->track[trackno];
	o->nchange = drive->nchange;
	o->omode = OREAD;
	o->buf = bopen(mmcread, OREAD, o->track->bs, Readblock);
	o->buf->otrack = o;

	aux->nropen++;

	return o;
}

enum {
	Flhlen		= 4,		/* format list header length */
	Fdlen		= 8,		/* format descriptor length */

	/* format unit cdb[1] */
	Formatdata	= 0x10,

	/* format list header [1] */
	Immed		= 1<<1,		/* immediate return, don't wait */

	/* format descriptor [4] */
	Fmtfull		= 0,
		Fullbdrsrm	= 0,	/* sequential */
		Fullbdrsrmspares= 1,	/* + spares for defect mgmt. */
		Fullbdrrrm	= 2,	/* random */
	Fmtbdrspares	= 0x32 << 2,
		Bdrsparessrmplus= 0,	/* srm+pow */
		Bdrsparessrmminus=1,	/* srm-pow */
		Bdrsparesrrm	= 2,	/* random */
};

static int
format(Drive *drive)
{
	int setblksz;
	ulong nblks, blksize;
	uchar *fmtdesc;
	uchar cmd[6], parms[Flhlen+Fdlen];

	if (drive->recordable == Yes && drive->mmctype != Mmcbd) {
		werrstr("don't format write-once cd or dvd media");
		return -1;
	}
	initcdb(cmd, sizeof cmd, ScmdFormat);	/* format unit */
	cmd[1] = Formatdata | 1;		/* mmc format code 1 */

	/* parms is format list header & format descriptor */
	memset(parms, 0, sizeof parms);

	/* format list header */
	parms[1] = Immed;
	parms[3] = Fdlen;

	fmtdesc = parms + Flhlen;
	fmtdesc[4] = Fmtfull;
	/* 0-3 are big-endian # of blocks, 5-7 format-dependent data */

	nblks = 0;
	blksize = BScdrom;
	setblksz = 1;
	switch (drive->mmctype) {
	case Mmccd:
		nblks = 0;
		break;
	case Mmcdvdplus:
		/* format type 0 is optional but equiv to format type 0x26 */
		fmtdesc[4] = 0x26 << 2;
		nblks = ~0ul;
		blksize = 0;
		break;
	case Mmcbd:
		/* enable BD-R defect mgmt. */
		if (drive->recordable == Yes && drive->erasable == No) {
			parms[1] &= ~Immed;
			fmtdesc[4] |= Fullbdrsrmspares;
			setblksz = 0;
		}
		break;
	}

	PUTBELONG(fmtdesc, nblks);
	if (setblksz)
		PUTBE24(fmtdesc + 5, blksize);

//	print("format parameters:\n");
//	hexdump(parms, sizeof parms);

	if(vflag)
		print("%lld ns: format\n", nsec());
	return scsi(drive, cmd, sizeof(cmd), parms, sizeof parms, Swrite);
}

static int
dvdcanverify(Drive *drive)
{
	return (drive->mmctype == Mmcdvdplus ||
		drive->mmctype == Mmcdvdminus) &&
		isbitset(Featrandwrite, drive->features) &&
		isbitset(Featwriteonce, drive->features) &&
		isbitset(Featdvdrw, drive->features);
}

static long
mmcxwrite(Otrack *o, void *v, long nblk)
{
	int r;
	long bs;
	uchar cmd[10];
	Drive *drive;
	Mmcaux *aux;

	assert(o->omode == OWRITE);
	bs = o->track->bs;
	drive = o->drive;
	aux = drive->aux;
	if (aux->mmcnwa == -1 && scsiready(drive) < 0) {
		werrstr("device not ready to write");
		return -1;
	}
	if (aux->mmcnwa == -1 ||
	    drive->end != 0 && aux->mmcnwa + nblk > drive->end) {
		werrstr("writing past last block");
		return -1;
	}
	if (nblk <= 0)
		fprint(2, "mmcxwrite: nblk %ld <= 0\n", nblk);
	aux->ntotby += nblk * bs;
	aux->ntotbk += nblk;

	/*
	 * "write and verify" (ScmdExtwritever) only works on write-once media
	 * and not on CDs (mmc-6 §6.48.1).
	 */
	if ((drive->mmctype == Mmcbd || dvdcanverify(drive)) &&
	    drive->recordable == Yes && drive->erasable == No)
		initcdb(cmd, sizeof cmd, ScmdExtwritever);
	else
		initcdb(cmd, sizeof cmd, ScmdExtwrite);	/* write (10) */
	cmd[2] = aux->mmcnwa>>24;
	cmd[3] = aux->mmcnwa>>16;
	cmd[4] = aux->mmcnwa>>8;
	cmd[5] = aux->mmcnwa;
	cmd[7] = nblk>>8;
	cmd[8] = nblk>>0;
	if(vflag > 1)
		print("%lld ns: write+verify %ld at %#lux\n",
			nsec(), nblk, aux->mmcnwa);
	/*
	 * we are using a private copy of scsi.c that doesn't retry writes
	 * to ensure that the write-verify command is issued exactly once.
	 * the drive may perform internal defect management on write errors,
	 * but explicit attempts to rewrite a given sector on write-once
	 * media are guaranteed to fail.
	 */
	r = scsi(drive, cmd, sizeof(cmd), v, nblk * bs, Swrite);
	if (r < 0)
		fprint(2, "%s: write+verify error at blk offset %,ld = "
			"offset %,lld / bs %ld: %r\n",
			argv0, aux->mmcnwa, (vlong)aux->mmcnwa * bs, bs);
	else
		aux->mmcnwa += nblk;
	return r;
}

static long
mmcwrite(Buf *buf, void *v, long nblk, ulong)
{
	return mmcxwrite(buf->otrack, v, nblk);
}

enum {
	Eccblk	= 128,		/* sectors per ecc block */
	Rsvslop	= 0,
};

static int
reserve(Drive *drive, int track)
{
	ulong sz;
	uchar cmd[10];

	initcdb(cmd, sizeof cmd, ScmdReserve);
	track -= drive->firsttrack;		/* switch to zero-origin */
	if (track >= 0 && track < drive->ntrack)
		/* .end is next sector past sz */
		sz = drive->track[track].end - drive->track[track].beg - Rsvslop;
	else {
		sz = Eccblk;
		fprint(2, "%s: reserve: track #%d out of range 0-%d\n",
			argv0, track, drive->ntrack);
	}
	sz -= sz % Eccblk;		/* round down to ecc-block multiple */
	if ((long)sz < 0) {
		fprint(2, "%s: reserve: bogus size %lud\n", argv0, sz);
		return -1;
	}
	cmd[1] = 0;			/* no ASRV: allocate by size not lba */
	PUTBELONG(cmd + 2 + 3, sz);
	if (vflag)
		fprint(2, "reserving %ld sectors\n", sz);
	return scsi(drive, cmd, sizeof cmd, cmd, 0, Snone);
}

static int
getinvistrack(Drive *drive)
{
	int n;
	uchar cmd[10], resp[Pagesz];

	initcdb(cmd, sizeof(cmd), ScmdRtrackinfo);
	cmd[1] = 1<<2 | 1;	/* open; address below is logical track # */
	PUTBELONG(cmd + 2, 1);		/* find first open track */
	cmd[7] = sizeof(resp)>>8;
	cmd[8] = sizeof(resp);
	n = scsi(drive, cmd, sizeof(cmd), resp, sizeof(resp), Sread);
	if(n < 4) {
		if(vflag)
			print("trackinfo for invis track fails n=%d: %r\n", n);
		return -1;
	}

	if(vflag)
		print("getinvistrack: track #%d session #%d\n",
			resp[2], resp[3]);
	drive->invistrack = resp[2];
	return resp[2];
}

static Otrack*
mmccreate(Drive *drive, int type)
{
	int bs;
	Mmcaux *aux;
	Track *t;
	Otrack *o;

	aux = drive->aux;

	if(aux->nropen || aux->nwopen) {
		werrstr("drive in use");
		return nil;
	}

	switch(type){
	case TypeAudio:
		bs = BScdda;
		break;
	case TypeData:
		bs = BScdrom;
		break;
	default:
		werrstr("bad type %d", type);
		return nil;
	}

	/* comment out the returns for now; it should be no big deal - geoff */
	if(mmctrackinfo(drive, drive->invistrack, Maxtrack)) {
		if (vflag)
			fprint(2, "mmccreate: mmctrackinfo for invis track %d"
				" failed: %r\n", drive->invistrack);
		werrstr("disc not writable");
//		return nil;
	}
	if(mmcsetbs(drive, bs) < 0) {
		werrstr("cannot set bs mode");
//		return nil;
	}
	if(mmctrackinfo(drive, drive->invistrack, Maxtrack)) {
		if (vflag)
			fprint(2, "mmccreate: mmctrackinfo for invis track %d"
				" (2) failed: %r\n", drive->invistrack);
		werrstr("disc not writable 2");
//		return nil;
	}

	/* special hack for dvd-r: reserve the invisible track */
	if (drive->mmctype == Mmcdvdminus && drive->writeok &&
	    drive->recordable == Yes && reserve(drive, drive->invistrack) < 0) {
		if (vflag)
			fprint(2, "mmcreate: reserving track %d for dvd-r "
				"failed: %r\n", drive->invistrack);
		return nil;
	}

	aux->ntotby = 0;
	aux->ntotbk = 0;

	t = &drive->track[drive->ntrack++];
	t->size = 0;
	t->bs = bs;
	t->beg = aux->mmcnwa;
	t->end = 0;
	t->type = type;
	drive->nameok = 0;

	o = emalloc(sizeof(Otrack));
	o->drive = drive;
	o->nchange = drive->nchange;
	o->omode = OWRITE;
	o->track = t;
	o->buf = bopen(mmcwrite, OWRITE, bs, Readblock);
	o->buf->otrack = o;

	aux->nwopen++;

	if(vflag)
		print("mmcinit: nwa = %#luX\n", aux->mmcnwa);
	return o;
}

/*
 * issue some form of close track, close session or finalize disc command.
 * see The Matrix, table 252 in MMC-6 §6.3.2.3.
 */
static int
mmcxclose(Drive *drive, int clf, int trackno)
{
	uchar cmd[10];

	initcdb(cmd, sizeof cmd, ScmdClosetracksess);
	/* cmd[1] & 1 is the immediate bit */
	cmd[2] = clf;				/* close function */
	if(clf == Closetrack)
		cmd[5] = trackno;
	return scsi(drive, cmd, sizeof(cmd), cmd, 0, Snone);
}

/* flush drive cache, close current track */
void
mmcsynccache(Drive *drive)
{
	uchar cmd[10];
	Mmcaux *aux;

	initcdb(cmd, sizeof cmd, ScmdSynccache);
	/*
	 * this will take a long time to burn the remainder of a dvd-r
	 * with a reserved track covering the whole disc.
	 */
	if (vflag) {
		fprint(2, "syncing cache");
		if (drive->mmctype == Mmcdvdminus && drive->writeok &&
		    drive->recordable == Yes)
			fprint(2, "; dvd-r burning rest of track reservation, "
				"will be slow");
		fprint(2, "\n");
	}
	scsi(drive, cmd, sizeof(cmd), cmd, 0, Snone);
	if(vflag) {
		aux = drive->aux;
		print("mmcsynccache: bytes = %lld blocks = %ld, mmcnwa %#luX\n",
			aux->ntotby, aux->ntotbk, aux->mmcnwa);
	}

	/*
	 * rsc: seems not to work on some drives.
	 * so ignore return code & don't issue on dvd+rw.
	 */
	if(drive->mmctype != Mmcdvdplus || drive->erasable == No) {
		if (vflag)
			fprint(2, "closing invisible track %d (not dvd+rw)...\n",
				drive->invistrack);
 		mmcxclose(drive, Closetrack, drive->invistrack);
		if (vflag)
			fprint(2, "... done.\n");
	}
	getinvistrack(drive);		/* track # has probably changed */
}

/*
 * close the open track `o'.
 */
static void
mmcclose(Otrack *o)
{
	Mmcaux *aux;
	static uchar zero[2*BSmax];

	aux = o->drive->aux;
	if(o->omode == OREAD)
		aux->nropen--;
	else if(o->omode == OWRITE) {
		aux->nwopen--;
		mmcxwrite(o, zero, 2);	/* write lead out */
		mmcsynccache(o->drive);
		o->drive->nchange = -1;	/* force reread toc */
	}
	free(o);
}

static int
setonesess(Drive *drive)
{
	uchar *p;
	Mmcaux *aux;

	/* page 5 is legacy and now read-only; see MMC-6 §7.5.4.1 */
	aux = drive->aux;
	if (!aux->page05ok)
		return -1;
	p = aux->page05;
	p[Wptrkmode] &= ~Msbits;
	p[Wptrkmode] |= Msnonext;
	return mmcsetpage(drive, Pagwrparams, p);
}

/*
 * close the current session, then finalize the disc.
 */
static int
mmcfixate(Drive *drive)
{
	int r;

	if((drive->cap & Cwrite) == 0) {
		werrstr("not a writer");
		return -1;
	}
	drive->nchange = -1;		/* force reread toc */

	setonesess(drive);

	/* skip explicit close session on bd-r */
	if (drive->mmctype != Mmcbd || drive->erasable == Yes) {
		if (vflag)
			fprint(2, "closing session and maybe finalizing...\n");
		r = mmcxclose(drive, Closesessfinal, 0);
		if (vflag)
			fprint(2, "... done.\n");
		if (r < 0)
			return r;
	}
	/*
	 * Closesessfinal only closes & doesn't finalize on dvd+r and bd-r.
	 * Closedvdrbdfinal closes & finalizes dvd+r and bd-r.
	 */
	if ((drive->mmctype == Mmcdvdplus || drive->mmctype == Mmcbd) &&
	    drive->erasable == No) {
		if (vflag)
			fprint(2, "finalizing dvd+r or bd-r... "
				"(won't print `done').\n");
		return mmcxclose(drive, Closedvdrbdfinal, 0);
	}
	return 0;
}

static int
mmcblank(Drive *drive, int quick)
{
	uchar cmd[12];

	drive->nchange = -1;		/* force reread toc */

	initcdb(cmd, sizeof cmd, ScmdBlank);	/* blank cd-rw media */
	/* immediate bit is 0x10 */
	/* cmd[1] = 0 means blank the whole disc; = 1 just the header */
	cmd[1] = quick ? 0x01 : 0x00;

	return scsi(drive, cmd, sizeof(cmd), cmd, 0, Snone);
}

static char*
e(int status)
{
	if(status < 0)
		return geterrstr();
	return nil;
}

static char*
mmcctl(Drive *drive, int argc, char **argv)
{
	char *cmd;

	if(argc < 1)
		return nil;
	cmd = argv[0];
	if(strcmp(cmd, "format") == 0)
		return e(format(drive));
	if(strcmp(cmd, "blank") == 0)
		return e(mmcblank(drive, 0));
	if(strcmp(cmd, "quickblank") == 0)
		return e(mmcblank(drive, 1));
	if(strcmp(cmd, "eject") == 0)
		return e(start(drive, 2));
	if(strcmp(cmd, "ingest") == 0)
		return e(start(drive, 3));
	return "bad arg";
}

static char*
mmcsetspeed(Drive *drive, int r, int w)
{
	char *rv;
	uchar cmd[12];

	initcdb(cmd, sizeof cmd, ScmdSetcdspeed);
	cmd[2] = r>>8;
	cmd[3] = r;
	cmd[4] = w>>8;
	cmd[5] = w;
	rv = e(scsi(drive, cmd, sizeof(cmd), nil, 0, Snone));
	mmcgetspeed(drive);
	return rv;
}

static Dev mmcdev = {
	mmcopenrd,
	mmccreate,
	bufread,
	bufwrite,
	mmcclose,
	mmcgettoc,
	mmcfixate,
	mmcctl,
	mmcsetspeed,
};

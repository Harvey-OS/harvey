#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	<bootexec.h>
#include	<mach.h>

/*
 *	All a.out header types.  The dummy entry allows canonical
 *	processing of the union as a sequence of longs
 */

typedef struct {
	union{
		Exec;			/* in a.out.h */
		struct mipsexec;	/* Hobbit uses this header too */
		struct mips4kexec;
		struct sparcexec;
		struct nextexec;
		struct i960exec;
	} e;
	long dummy;		/* padding to ensure extra long */
} ExecHdr;

static	void	i960boot(Fhdr *, ExecHdr *);
static	void	nextboot(Fhdr *, ExecHdr *);
static	void	sparcboot(Fhdr *, ExecHdr *);
static	void	mipsboot(Fhdr *, ExecHdr *);
static	void	mips4kboot(Fhdr *, ExecHdr *);
static	void	common(Fhdr *, ExecHdr *);
static	void	adotout(Fhdr *, ExecHdr *);
static	void	setsym(Fhdr *, long, long, long, long);
static	void	setdata(Fhdr *, long, long, long, long);
static	void	settext(Fhdr *, long, long, long, long);
static	void	hswal(long *, int, long (*) (long));
static	long	_round(long, long);

/*
 *	definition of per-executable file type structures
 */

typedef struct Exectable{
	long	magic;			/* big-endian magic number of file */
	char	*name;			/* executable identifier */
	int	type;			/* Internal code */
	Mach	*mach;			/* Per-machine data */
	ulong	hsize;			/* header size */
	long	(*swal)(long);		/* beswal or leswal */
	void	(*hparse)(Fhdr *, ExecHdr *);
} ExecTable;

extern	Mach	mmips;
extern	Mach	msparc;
extern	Mach	m68020;
extern	Mach	mi386;
extern	Mach	mi960;
extern	Mach	m3210;

ExecTable exectab[] =
{
	{ V_MAGIC,			/* Mips v.out */
		"mips plan 9 executable",
		FMIPS,
		&mmips,
		sizeof(Exec),
		beswal,
		adotout },
	{ 0x160<<16,			/* Mips boot image */
		"mips plan 9 boot image",
		FMIPSB,
		&mmips,
		sizeof(struct mipsexec),
		beswal,
		mipsboot },
	{ (0x160<<16)|3,		/* Mips boot image */
		"mips 4k plan 9 boot image",
		FMIPSB,
		&mmips,
		sizeof(struct mips4kexec),
		beswal,
		mips4kboot },
	{ K_MAGIC,			/* Sparc k.out */
		"sparc plan 9 executable",
		FSPARC,
		&msparc,
		sizeof(Exec),
		beswal,
		adotout },
	{ 0x01030107, 			/* Sparc boot image */
		"sparc plan 9 boot image",
		FSPARCB,
		&msparc,
		sizeof(struct sparcexec),
		beswal,
		sparcboot },
	{ A_MAGIC,			/* 68020 2.out & boot image */
		"68020 plan 9 executable",
		F68020,
		&m68020,
		sizeof(Exec),
		beswal,
		common },
	{ 0xFEEDFACE,			/* Next boot image */
		"next plan 9 boot image",
		FNEXTB,
		&m68020,
		sizeof(struct nextexec),
		beswal,
		nextboot },
	{ I_MAGIC,			/* I386 8.out & boot image */
		"386 plan 9 executable",
		FI386,
		&mi386,
		sizeof(Exec),
		beswal,
		common },
	{ J_MAGIC,			/* I960 6.out (big-endian) */
		"960 plan 9 executable",
		FI960,
		&mi960,
		sizeof(Exec),
		beswal,
		adotout },
	{ 0x61010200, 			/* I960 boot image (little endian) */
		"960 plan 9 boot image",
		FI960B,
		&mi960,
		sizeof(struct i960exec),
		leswal,
		i960boot },
	{ X_MAGIC,			/* 3210 x.out */
		"3210 plan 9 executable",
		F3210,
		&m3210,
		sizeof(Exec),
		beswal,
		adotout },
	{ 0 },
};

Mach	*mach = &mmips;			/* Global current machine table */

int
crackhdr(int fd, Fhdr *fp)
{
	ExecTable *mp;
	ExecHdr d;
	int nb, magic;

	fp->type = FNONE;
	if ((nb = read(fd, (char *)&d.e, sizeof(d.e))) <= 0)
		return 0;
	fp->magic = magic = beswal(d.e.magic);		/* big-endian */
	for (mp = exectab; mp->magic; mp++) {
		if (mp->magic == magic && nb >= mp->hsize) {
			hswal((long *) &d, sizeof(d.e)/sizeof(long), mp->swal);
			fp->type = mp->type;
			fp->name = mp->name;
			fp->hdrsz = mp->hsize;		/* zero on bootables */
			mach = mp->mach;
			mp->hparse(fp, &d);
			seek(fd, mp->hsize, 0);		/* seek to end of header */
			return 1;
		}
	}
	return 0;
}
/*
 * Convert header to canonical form
 */
static void
hswal(long *lp, int n, long (*swap) (long))
{
	while (n--) {
		*lp = (*swap) (*lp);
		lp++;
	}
}
/*
 *	Crack a normal a.out-type header
 */
static void
adotout(Fhdr *fp, ExecHdr *hp)
{
	long pgsize = mach->pgsize;

	settext(fp, hp->e.entry, pgsize+sizeof(Exec),
			hp->e.text, sizeof(Exec));
	setdata(fp, _round(pgsize+fp->txtsz+sizeof(Exec), pgsize),
		hp->e.data, fp->txtsz+sizeof(Exec), hp->e.bss);
	setsym(fp, hp->e.syms, hp->e.spsz, hp->e.pcsz, fp->datoff+fp->datsz);
}

/*
 *	68020 2.out and 68020 bootable images
 *	386I 8.out and 386I bootable images
 *
 */
static void
common(Fhdr *fp, ExecHdr *hp)
{
	long kbase = mach->kbase;

	adotout(fp, hp);
	if (fp->entry & kbase) {		/* Boot image */
		switch(fp->type) {
		case F68020:
			fp->type = F68020B;
			fp->name = "68020 plan 9 boot image";
			fp->hdrsz = 0;		/* header stripped */
			break;
		case FI386:
			fp->type = FI386B;
			fp->txtaddr = sizeof(Exec);
			fp->name = "386 plan 9 boot image";
			fp->hdrsz = 0;		/* header stripped */
			fp->dataddr = fp->txtaddr+fp->txtsz;
			break;
		default:
			break;
		}
		fp->txtaddr |= kbase;
		fp->entry |= kbase;
		fp->dataddr |= kbase;
	}
}

/*
 *	mips bootable image.
 */
static void
mipsboot(Fhdr *fp, ExecHdr *hp)
{
	switch(hp->e.amagic) {
	default:
	case 0407:	/* some kind of mips */
		fp->type = FMIPSB;
		settext(fp, hp->e.mentry, hp->e.text_start, hp->e.tsize,
					sizeof(struct mipsexec)+4);
		setdata(fp, hp->e.data_start, hp->e.dsize,
				fp->txtoff+hp->e.tsize, hp->e.bsize);
		break;
	case 0413:	/* some kind of mips */
		fp->type = FMIPSB;
		settext(fp, hp->e.mentry, hp->e.text_start, hp->e.tsize, 0);
		setdata(fp, hp->e.data_start, hp->e.dsize, hp->e.tsize,
					hp->e.bsize);
		break;
	}
	setsym(fp, hp->e.nsyms, 0, hp->e.pcsize, hp->e.symptr);
	fp->hdrsz = 0;		/* header stripped */
}

/*
 *	mips4k bootable image.
 */
static void
mips4kboot(Fhdr *fp, ExecHdr *hp)
{
	switch(hp->e.h.amagic) {
	default:
	case 0407:	/* some kind of mips */
		fp->type = FMIPSB;
		settext(fp, hp->e.h.mentry, hp->e.h.text_start, hp->e.h.tsize,
					sizeof(struct mips4kexec));
		setdata(fp, hp->e.h.data_start, hp->e.h.dsize,
				fp->txtoff+hp->e.h.tsize, hp->e.h.bsize);
		break;
	case 0413:	/* some kind of mips */
		fp->type = FMIPSB;
		settext(fp, hp->e.h.mentry, hp->e.h.text_start, hp->e.h.tsize, 0);
		setdata(fp, hp->e.h.data_start, hp->e.h.dsize, hp->e.h.tsize,
					hp->e.h.bsize);
		break;
	}
	setsym(fp, hp->e.h.nsyms, 0, hp->e.h.pcsize, hp->e.h.symptr);
	fp->hdrsz = 0;		/* header stripped */
}

/*
 *	sparc bootable image
 */
static void
sparcboot(Fhdr *fp, ExecHdr *hp)
{
	fp->type = FSPARCB;
	settext(fp, hp->e.sentry, hp->e.sentry, hp->e.stext,
					sizeof(struct sparcexec));
	setdata(fp, hp->e.sentry+hp->e.stext, hp->e.sdata,
					fp->txtoff+hp->e.stext, hp->e.sbss);
	setsym(fp, hp->e.ssyms, 0, hp->e.sdrsize, fp->datoff+hp->e.sdata);
	fp->hdrsz = 0;		/* header stripped */
}

/*
 *	next bootable image
 */
static void
nextboot(Fhdr *fp, ExecHdr *hp)
{
	fp->type = FNEXTB;
	settext(fp, hp->e.textc.vmaddr, hp->e.textc.vmaddr,
					hp->e.texts.size, hp->e.texts.offset);
	setdata(fp, hp->e.datac.vmaddr, hp->e.datas.size,
				hp->e.datas.offset, hp->e.bsss.size);
	setsym(fp, hp->e.symc.nsyms, hp->e.symc.spoff, hp->e.symc.pcoff,
					hp->e.symc.symoff);
	fp->hdrsz = 0;		/* header stripped */
}

/*
 *	I960 bootable image
 */
static void
i960boot(Fhdr *fp, ExecHdr *hp)
{
	/* long n = hp->e.i6comments.fptrlineno-hp->e.i6comments.fptrreloc; */

	settext(fp, hp->e.i6entry, hp->e.i6texts.virt, hp->e.i6texts.size,
					hp->e.i6texts.fptr);
	setdata(fp, hp->e.i6datas.virt, hp->e.i6datas.size,
				hp->e.i6datas.fptr, hp->e.i6bsssize);
	setsym(fp, 0, 0, 0, 0);
	/*setsym(fp, n, 0, hp->e.i6comments.size-n, hp->e.i6comments.fptr); */
	fp->hdrsz = 0;		/* header stripped */
}


static void
settext(Fhdr *fp, long e, long a, long s, long off)
{
	fp->txtaddr = a;
	fp->entry = e;
	fp->txtsz = s;
	fp->txtoff = off;
}
static void
setdata(Fhdr *fp, long a, long s, long off, long bss)
{
	fp->dataddr = a;
	fp->datsz = s;
	fp->datoff = off;
	fp->bsssz = bss;
}
static void
setsym(Fhdr *fp, long sy, long sppc, long lnpc, long symoff)
{
	fp->symsz = sy;
	fp->symoff = symoff;
	fp->sppcsz = sppc;
	fp->sppcoff = fp->symoff+fp->symsz;
	fp->lnpcsz = lnpc;
	fp->lnpcoff = fp->sppcoff+fp->sppcsz;
}


static long
_round(long a, long b)
{
	long w;

	w = (a/b)*b;
	if (a!=w)
		w += b;
	return(w);
}

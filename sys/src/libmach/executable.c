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
static	void	hobbitboot(Fhdr *, ExecHdr *);
static	void	common(Fhdr *, ExecHdr *);
static	void	adotout(Fhdr *, ExecHdr *);
static	void	setsym(Fhdr *, long, long, long, long);
static	void	setdata(Fhdr *, long, long, long, long);
static	void	settext(Fhdr *, long, long, long, long);
static	void	hswal(long *, int, long (*) (long));

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
extern	Mach	mhobbit;

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
	{ 0x161<<16,			/* hobbit boot image */
		"hobbit plan 9 boot image",
		FHOBBITB,
		&mhobbit,
		sizeof(struct mipsexec),
		beswal,
		hobbitboot },
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
	{ Z_MAGIC,			/* Hobbit z.out */
		"hobbit plan 9 executable",
		FHOBBIT,
		&mhobbit,
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
			fp->hdrsz = mp->hsize;
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

	settext(fp, hp->e.entry, pgsize, hp->e.text+sizeof(Exec), 0);
	setdata(fp, _round(pgsize+fp->txtsz, pgsize), hp->e.data, fp->txtsz,
							hp->e.bss);
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
			break;
		case FI386:
			fp->type = FI386B;
			fp->name = "386 plan 9 boot image";
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
}
/*
 *	hobbit bootable image.
 */
static void
hobbitboot(Fhdr *fp, ExecHdr *hp)
{
	settext(fp, hp->e.mentry, hp->e.text_start, hp->e.tsize,
				sizeof(struct mipsexec)+4);
	setdata(fp, hp->e.data_start, hp->e.dsize,
			fp->txtoff+hp->e.tsize, hp->e.bsize);
	setsym(fp, hp->e.nsyms, 0, hp->e.pcsize, hp->e.symptr);
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


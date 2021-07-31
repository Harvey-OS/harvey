#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	<bootexec.h>
#include	<mach.h>
#include	"elf.h"

/*
 *	All a.out header types.  The dummy entry allows canonical
 *	processing of the union as a sequence of longs
 */

typedef struct {
	union{
		Exec;			/* in a.out.h */
		Ehdr;			/* in elf.h */
		struct mipsexec;
		struct mips4kexec;
		struct sparcexec;
		struct nextexec;
	} e;
	long dummy;		/* padding to ensure extra long */
} ExecHdr;

static	int	nextboot(int, Fhdr*, ExecHdr*);
static	int	sparcboot(int, Fhdr*, ExecHdr*);
static	int	mipsboot(int, Fhdr*, ExecHdr*);
static	int	mips4kboot(int, Fhdr*, ExecHdr*);
static	int	common(int, Fhdr*, ExecHdr*);
static	int	adotout(int, Fhdr*, ExecHdr*);
static	int	elfdotout(int, Fhdr*, ExecHdr*);
static	int	armdotout(int, Fhdr*, ExecHdr*);
static	int	alphadotout(int, Fhdr*, ExecHdr*);
static	void	setsym(Fhdr*, long, long, long, long);
static	void	setdata(Fhdr*, long, long, long, long);
static	void	settext(Fhdr*, long, long, long, long);
static	void	hswal(long*, int, long(*)(long));
static	long	noswal(long);
static	long	_round(long, long);

/*
 *	definition of per-executable file type structures
 */

typedef struct Exectable{
	long	magic;			/* big-endian magic number of file */
	char	*name;			/* executable identifier */
	char	*dlmname;		/* dynamically loadable module identifier */
	int	type;			/* Internal code */
	Mach	*mach;			/* Per-machine data */
	ulong	hsize;			/* header size */
	long	(*swal)(long);		/* beswal or leswal */
	int	(*hparse)(int, Fhdr*, ExecHdr*);
} ExecTable;

extern	Mach	mmips;
extern	Mach	mmips2le;
extern	Mach	mmips2be;
extern	Mach	msparc;
extern	Mach	msparc64;
extern	Mach	m68020;
extern	Mach	mi386;
extern	Mach	mamd64;
extern	Mach	marm;
extern	Mach	mpower;
extern	Mach	malpha;

ExecTable exectab[] =
{
	{ V_MAGIC,			/* Mips v.out */
		"mips plan 9 executable",
		"mips plan 9 dlm",
		FMIPS,
		&mmips,
		sizeof(Exec),
		beswal,
		adotout },
	{ M_MAGIC,			/* Mips 4.out */
		"mips 4k plan 9 executable BE",
		"mips 4k plan 9 dlm BE",
		FMIPS2BE,
		&mmips2be,
		sizeof(Exec),
		beswal,
		adotout },
	{ N_MAGIC,			/* Mips 0.out */
		"mips 4k plan 9 executable LE",
		"mips 4k plan 9 dlm LE",
		FMIPS2LE,
		&mmips2le,
		sizeof(Exec),
		beswal,
		adotout },
	{ 0x160<<16,			/* Mips boot image */
		"mips plan 9 boot image",
		nil,
		FMIPSB,
		&mmips,
		sizeof(struct mipsexec),
		beswal,
		mipsboot },
	{ (0x160<<16)|3,		/* Mips boot image */
		"mips 4k plan 9 boot image",
		nil,
		FMIPSB,
		&mmips2be,
		sizeof(struct mips4kexec),
		beswal,
		mips4kboot },
	{ K_MAGIC,			/* Sparc k.out */
		"sparc plan 9 executable",
		"sparc plan 9 dlm",
		FSPARC,
		&msparc,
		sizeof(Exec),
		beswal,
		adotout },
	{ 0x01030107, 			/* Sparc boot image */
		"sparc plan 9 boot image",
		nil,
		FSPARCB,
		&msparc,
		sizeof(struct sparcexec),
		beswal,
		sparcboot },
	{ U_MAGIC,			/* Sparc64 u.out */
		"sparc64 plan 9 executable",
		"sparc64 plan 9 dlm",
		FSPARC64,
		&msparc64,
		sizeof(Exec),
		beswal,
		adotout },
	{ A_MAGIC,			/* 68020 2.out & boot image */
		"68020 plan 9 executable",
		"68020 plan 9 dlm",
		F68020,
		&m68020,
		sizeof(Exec),
		beswal,
		common },
	{ 0xFEEDFACE,			/* Next boot image */
		"next plan 9 boot image",
		nil,
		FNEXTB,
		&m68020,
		sizeof(struct nextexec),
		beswal,
		nextboot },
	{ I_MAGIC,			/* I386 8.out & boot image */
		"386 plan 9 executable",
		"386 plan 9 dlm",
		FI386,
		&mi386,
		sizeof(Exec),
		beswal,
		common },
	{ S_MAGIC,			/* amd64 6.out & boot image */
		"amd64 plan 9 executable",
		"amd64 plan 9 dlm",
		FAMD64,
		&mamd64,
		sizeof(Exec),
		beswal,
		common },
	{ Q_MAGIC,			/* PowerPC q.out & boot image */
		"power plan 9 executable",
		"power plan 9 dlm",
		FPOWER,
		&mpower,
		sizeof(Exec),
		beswal,
		common },
	{ ELF_MAG,			/* any elf32 */
		"elf executable",
		nil,
		FNONE,
		&mi386,
		sizeof(Ehdr),
		noswal,
		elfdotout },
	{ E_MAGIC,			/* Arm 5.out */
		"arm plan 9 executable",
		"arm plan 9 dlm",
		FARM,
		&marm,
		sizeof(Exec),
		beswal,
		common },
	{ (143<<16)|0413,		/* (Free|Net)BSD Arm */
		"arm *bsd executable",
		nil,
		FARM,
		&marm,
		sizeof(Exec),
		leswal,
		armdotout },
	{ L_MAGIC,			/* alpha 7.out */
		"alpha plan 9 executable",
		"alpha plan 9 dlm",
		FALPHA,
		&malpha,
		sizeof(Exec),
		beswal,
		common },
	{ 0x0700e0c3,			/* alpha boot image */
		"alpha plan 9 boot image",
		nil,
		FALPHAB,
		&malpha,
		sizeof(Exec),
		beswal,
		alphadotout },
	{ 0 },
};

Mach	*mach = &mi386;			/* Global current machine table */

static ExecTable*
couldbe4k(ExecTable *mp)
{
	Dir *d;
	ExecTable *f;

	if((d=dirstat("/proc/1/regs")) == nil)
		return mp;
	if(d->length < 32*8){		/* R3000 */
		free(d);
		return mp;
	}
	free(d);
	for (f = exectab; f->magic; f++)
		if(f->magic == M_MAGIC) {
			f->name = "mips plan 9 executable on mips2 kernel";
			return f;
		}
	return mp;
}

int
crackhdr(int fd, Fhdr *fp)
{
	ExecTable *mp;
	ExecHdr d;
	int nb, magic, ret;

	fp->type = FNONE;
	nb = read(fd, (char *)&d.e, sizeof(d.e));
	if (nb <= 0)
		return 0;

	ret = 0;
	fp->magic = magic = beswal(d.e.magic);		/* big-endian */
	for (mp = exectab; mp->magic; mp++) {
		if (nb < mp->hsize)
			continue;
		if (mp->magic == (magic & ~DYN_MAGIC)) {
			if(mp->magic == V_MAGIC)
				mp = couldbe4k(mp);

			hswal((long *) &d, sizeof(d.e)/sizeof(long), mp->swal);
			fp->type = mp->type;
			if ((magic & DYN_MAGIC) && mp->dlmname != nil)
				fp->name = mp->dlmname;
			else
				fp->name = mp->name;
			fp->hdrsz = mp->hsize;		/* zero on bootables */
			mach = mp->mach;
			ret  = mp->hparse(fd, fp, &d);
			seek(fd, mp->hsize, 0);		/* seek to end of header */
			break;
		}
	}
	if(mp->magic == 0)
		werrstr("unknown header type");
	return ret;
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
 * noop
 */
static long
noswal(long x)
{
	return x;
}
/*
 *	Crack a normal a.out-type header
 */
static int
adotout(int fd, Fhdr *fp, ExecHdr *hp)
{
	long pgsize;

	USED(fd);
	pgsize = mach->pgsize;
	settext(fp, hp->e.entry, pgsize+sizeof(Exec),
			hp->e.text, sizeof(Exec));
	setdata(fp, _round(pgsize+fp->txtsz+sizeof(Exec), pgsize),
		hp->e.data, fp->txtsz+sizeof(Exec), hp->e.bss);
	setsym(fp, hp->e.syms, hp->e.spsz, hp->e.pcsz, fp->datoff+fp->datsz);
	return 1;
}

/*
 *	68020 2.out and 68020 bootable images
 *	386I 8.out and 386I bootable images
 *	alpha plan9-style bootable images for axp "headerless" boot
 *
 */
static int
common(int fd, Fhdr *fp, ExecHdr *hp)
{
	long kbase;

	adotout(fd, fp, hp);
	if(hp->e.magic & DYN_MAGIC) {
		fp->txtaddr = 0;
		fp->dataddr = fp->txtsz;
		return 1;
	}
	kbase = mach->kbase;
	if ((fp->entry & kbase) == kbase) {		/* Boot image */
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
		case FARM:
			fp->txtaddr = kbase+0x8010;
			fp->name = "ARM plan 9 boot image";
			fp->hdrsz = 0;		/* header stripped */
			fp->dataddr = fp->txtaddr+fp->txtsz;
			return 1;
		case FALPHA:
			fp->type = FALPHAB;
			fp->txtaddr = fp->entry;
			fp->name = "alpha plan 9 boot image?";
			fp->hdrsz = 0;		/* header stripped */
			fp->dataddr = fp->txtaddr+fp->txtsz;
			break;
		case FPOWER:
			fp->type = FPOWERB;
			fp->txtaddr = fp->entry;
			fp->name = "power plan 9 boot image";
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
	return 1;
}

/*
 *	mips bootable image.
 */
static int
mipsboot(int fd, Fhdr *fp, ExecHdr *hp)
{
	USED(fd);
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
	return 1;
}

/*
 *	mips4k bootable image.
 */
static int
mips4kboot(int fd, Fhdr *fp, ExecHdr *hp)
{
	USED(fd);
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
	return 1;
}

/*
 *	sparc bootable image
 */
static int
sparcboot(int fd, Fhdr *fp, ExecHdr *hp)
{
	USED(fd);
	fp->type = FSPARCB;
	settext(fp, hp->e.sentry, hp->e.sentry, hp->e.stext,
					sizeof(struct sparcexec));
	setdata(fp, hp->e.sentry+hp->e.stext, hp->e.sdata,
					fp->txtoff+hp->e.stext, hp->e.sbss);
	setsym(fp, hp->e.ssyms, 0, hp->e.sdrsize, fp->datoff+hp->e.sdata);
	fp->hdrsz = 0;		/* header stripped */
	return 1;
}

/*
 *	next bootable image
 */
static int
nextboot(int fd, Fhdr *fp, ExecHdr *hp)
{
	USED(fd);
	fp->type = FNEXTB;
	settext(fp, hp->e.textc.vmaddr, hp->e.textc.vmaddr,
					hp->e.texts.size, hp->e.texts.offset);
	setdata(fp, hp->e.datac.vmaddr, hp->e.datas.size,
				hp->e.datas.offset, hp->e.bsss.size);
	setsym(fp, hp->e.symc.nsyms, hp->e.symc.spoff, hp->e.symc.pcoff,
					hp->e.symc.symoff);
	fp->hdrsz = 0;		/* header stripped */
	return 1;
}


/*
 * Elf32 binaries.
 */
static int
elfdotout(int fd, Fhdr *fp, ExecHdr *hp)
{

	long (*swal)(long);
	ushort (*swab)(ushort);
	Ehdr *ep;
	Phdr *ph;
	int i, it, id, is, phsz;

	/* bitswap the header according to the DATA format */
	ep = &hp->e;
	if(ep->ident[CLASS] != ELFCLASS32) {
		werrstr("bad ELF class - not 32 bit");
		return 0;
	}
	if(ep->ident[DATA] == ELFDATA2LSB) {
		swab = leswab;
		swal = leswal;
	} else if(ep->ident[DATA] == ELFDATA2MSB) {
		swab = beswab;
		swal = beswal;
	} else {
		werrstr("bad ELF encoding - not big or little endian");
		return 0;
	}

	ep->type = swab(ep->type);
	ep->machine = swab(ep->machine);
	ep->version = swal(ep->version);
	ep->elfentry = swal(ep->elfentry);
	ep->phoff = swal(ep->phoff);
	ep->shoff = swal(ep->shoff);
	ep->flags = swal(ep->flags);
	ep->ehsize = swab(ep->ehsize);
	ep->phentsize = swab(ep->phentsize);
	ep->phnum = swab(ep->phnum);
	ep->shentsize = swab(ep->shentsize);
	ep->shnum = swab(ep->shnum);
	ep->shstrndx = swab(ep->shstrndx);
	if(ep->type != EXEC || ep->version != CURRENT)
		return 0;

	/* we could definitely support a lot more machines here */
	fp->magic = ELF_MAG;
	fp->hdrsz = (ep->ehsize+ep->phnum*ep->phentsize+16)&~15;
	switch(ep->machine) {
	case I386:
		mach = &mi386;
		fp->type = FI386;
		break;
	case MIPS:
		mach = &mmips;
		fp->type = FMIPS;
		break;
	case SPARC64:
		mach = &msparc64;
		fp->type = FSPARC64;
		break;
	case POWER:
		mach = &mpower;
		fp->type = FPOWER;
		break;
	case AMD64:
		mach = &mamd64;
		fp->type = FAMD64;
		break;
	default:
		return 0;
	}

	if(ep->phentsize != sizeof(Phdr)) {
		werrstr("bad ELF header size");
		return 0;
	}
	phsz = sizeof(Phdr)*ep->phnum;
	ph = malloc(phsz);
	if(!ph)
		return 0;
	seek(fd, ep->phoff, 0);
	if(read(fd, ph, phsz) < 0) {
		free(ph);
		return 0;
	}
	hswal((long*)ph, phsz/sizeof(long), swal);

	/* find text, data and symbols and install them */
	it = id = is = -1;
	for(i = 0; i < ep->phnum; i++) {
		if(ph[i].type == LOAD 
		&& (ph[i].flags & (R|X)) == (R|X) && it == -1)
			it = i;
		else if(ph[i].type == LOAD 
		&& (ph[i].flags & (R|W)) == (R|W) && id == -1)
			id = i;
		else if(ph[i].type == NOPTYPE && is == -1)
			is = i;
	}
	if(it == -1 || id == -1) {
		/* 
		 * The SPARC64 boot image is something of an ELF hack.
		 * Text+Data+BSS are represented by ph[0].  Symbols
		 * are represented by ph[1]:
		 *
		 *		filesz, memsz, vaddr, paddr, off
		 * ph[0] : txtsz+datsz, txtsz+datsz+bsssz, txtaddr-KZERO, datasize,  txtoff
		 * ph[1] : symsz, lcsz, 0, 0, symoff
		 */
		if(ep->machine == SPARC64 && ep->phnum == 2) {
			ulong txtaddr, txtsz, dataddr, bsssz;

			txtaddr = ph[0].vaddr | 0x80000000;
			txtsz = ph[0].filesz - ph[0].paddr;
			dataddr = txtaddr + txtsz;
			bsssz = ph[0].memsz - ph[0].filesz;
			settext(fp, ep->elfentry | 0x80000000, txtaddr, txtsz, ph[0].offset);
			setdata(fp, dataddr, ph[0].paddr, ph[0].offset + txtsz, bsssz);
			setsym(fp, ph[1].filesz, 0, ph[1].memsz, ph[1].offset);
			free(ph);
			return 1;
		}

		werrstr("No TEXT or DATA sections");
		free(ph);
		return 0;
	}

	settext(fp, ep->elfentry, ph[it].vaddr, ph[it].memsz, ph[it].offset);
	setdata(fp, ph[id].vaddr, ph[id].filesz, ph[id].offset, ph[id].memsz - ph[id].filesz);
	if(is != -1)
		setsym(fp, ph[is].filesz, 0, ph[is].memsz, ph[is].offset);
	free(ph);
	return 1;
}

/*
 * alpha bootable
 */
static int
alphadotout(int fd, Fhdr *fp, ExecHdr *hp)
{
	long kbase;

	USED(fd);
	settext(fp, hp->e.entry, sizeof(Exec), hp->e.text, sizeof(Exec));
	setdata(fp, fp->txtsz+sizeof(Exec), hp->e.data, fp->txtsz+sizeof(Exec), hp->e.bss);
	setsym(fp, hp->e.syms, hp->e.spsz, hp->e.pcsz, fp->datoff+fp->datsz);

	/*
	 * Boot images have some of bits <31:28> set:
	 *	0x80400000	kernel
	 *	0x20000000	secondary bootstrap
	 */
	kbase = 0xF0000000;
	if (fp->entry & kbase) {
		fp->txtaddr = fp->entry;
		fp->name = "alpha plan 9 boot image";
		fp->hdrsz = 0;		/* header stripped */
		fp->dataddr = fp->entry+fp->txtsz;
	}
	return 1;
}

/*
 * (Free|Net)BSD ARM header.
 */
static int
armdotout(int fd, Fhdr *fp, ExecHdr *hp)
{
	long kbase;

	USED(fd);
	settext(fp, hp->e.entry, sizeof(Exec), hp->e.text, sizeof(Exec));
	setdata(fp, fp->txtsz, hp->e.data, fp->txtsz, hp->e.bss);
	setsym(fp, hp->e.syms, hp->e.spsz, hp->e.pcsz, fp->datoff+fp->datsz);

	kbase = 0xF0000000;
	if ((fp->entry & kbase) == kbase) {		/* Boot image */
		fp->txtaddr = kbase+sizeof(Exec);
		fp->name = "ARM *BSD boot image";
		fp->hdrsz = 0;		/* header stripped */
		fp->dataddr = kbase+fp->txtsz;
	}
	return 1;
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

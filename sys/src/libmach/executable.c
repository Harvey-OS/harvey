/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	<mach.h>
#include	"elf.h"

/*
 *	All a.out header types.  The dummy entry allows canonical
 *	processing of the union as a sequence of longs
 */

typedef struct {
	union{
		struct {
			/* this will die soon. For now, just include it here. */
			int32_t	magic;		/* magic number */
			int32_t	text;	 	/* size of text segment */
			int32_t	data;	 	/* size of initialized data */
			int32_t	bss;	  	/* size of uninitialized data */
			int32_t	syms;	 	/* size of symbol table */
			int32_t	entry;	 	/* entry point */
			int32_t	spsz;		/* size of pc/sp offset table */
			int32_t	pcsz;		/* size of pc/line number table */
		};
#ifdef HARVEY32
		Ehdr;			/* elf.h */
#endif
		E64hdr;
	} e;
	int32_t dummy;			/* padding to ensure extra long */
} ExecHdr;

#ifdef HARVEYNEXT
static	int	nextboot(int, Fhdr*, ExecHdr*);
#elif HARVEYSPARC
static	int	sparcboot(int, Fhdr*, ExecHdr*);
#elif HARVEYMIPS
static	int	mipsboot(int, Fhdr*, ExecHdr*);
static	int	mips4kboot(int, Fhdr*, ExecHdr*);
#endif
static	int	common(int, Fhdr*, ExecHdr*);
static	int	commonllp64(int, Fhdr*, ExecHdr*);
static	int	adotout(int, Fhdr*, ExecHdr*);
static	int	elfdotout(int, Fhdr*, ExecHdr*);
static	int	armdotout(int, Fhdr*, ExecHdr*);
static	void	setsym(Fhdr*, int32_t, int32_t, int32_t, int64_t);
static	void	setdata(Fhdr*, uint64_t, int32_t, int64_t,
				  int32_t);
static	void	settext(Fhdr*, uint64_t, uint64_t, int32_t,
				  int64_t);
static	void	hswal(void*, int, uint32_t(*)(uint32_t));
static	uint64_t	_round(uint64_t, uint32_t);

/*
 *	definition of per-executable file type structures
 */

typedef struct Exectable{
	int32_t	magic;			/* big-endian magic number of file */
	char	*name;			/* executable identifier */
	char	*dlmname;		/* dynamically loadable module identifier */
	uint8_t	type;			/* Internal code */
	uint8_t	_magic;			/* _MAGIC() magic */
	Mach	*mach;			/* Per-machine data */
	int32_t	hsize;			/* header size */
	uint32_t	(*swal)(uint32_t);		/* beswal or leswal */
	int	(*hparse)(int, Fhdr*, ExecHdr*);
} ExecTable;

#ifdef HARVEYMIPS
extern	Mach	mmips;
extern	Mach	mmips2le;
extern	Mach	mmips2be;
#elif HARVEYSPARC
extern	Mach	msparc;
extern	Mach	msparc64;
extern	Mach	m68020;
#elif HARVEY32
extern	Mach	mi386;
#endif
extern	Mach	mamd64;
#ifdef HARVEYARM
extern	Mach	marm;
#elif HARVEYPPC
extern	Mach	mpower;
extern	Mach	mpower64;
#elif HARVEYALPHA
extern	Mach	malpha;
#endif

ExecTable exectab[] =
{
#ifdef HARVEYMIPS
	{ V_MAGIC,			/* Mips v.out */
		"mips plan 9 executable BE",
		"mips plan 9 dlm BE",
		FMIPS,
		1,
		&mmips,
		sizeof(Exec),
		beswal,
		adotout },
	{ P_MAGIC,			/* Mips 0.out (r3k le) */
		"mips plan 9 executable LE",
		"mips plan 9 dlm LE",
		FMIPSLE,
		1,
		&mmips,
		sizeof(Exec),
		beswal,
		adotout },
	{ M_MAGIC,			/* Mips 4.out */
		"mips 4k plan 9 executable BE",
		"mips 4k plan 9 dlm BE",
		FMIPS2BE,
		1,
		&mmips2be,
		sizeof(Exec),
		beswal,
		adotout },
	{ N_MAGIC,			/* Mips 0.out */
		"mips 4k plan 9 executable LE",
		"mips 4k plan 9 dlm LE",
		FMIPS2LE,
		1,
		&mmips2le,
		sizeof(Exec),
		beswal,
		adotout },
	{ 0x160<<16,			/* Mips boot image */
		"mips plan 9 boot image",
		nil,
		FMIPSB,
		0,
		&mmips,
		sizeof(struct mipsexec),
		beswal,
		mipsboot },
	{ (0x160<<16)|3,		/* Mips boot image */
		"mips 4k plan 9 boot image",
		nil,
		FMIPSB,
		0,
		&mmips2be,
		sizeof(struct mips4kexec),
		beswal,
		mips4kboot },
#elif HARVEYSPARC
	{ K_MAGIC,			/* Sparc k.out */
		"sparc plan 9 executable",
		"sparc plan 9 dlm",
		FSPARC,
		1,
		&msparc,
		sizeof(Exec),
		beswal,
		adotout },
	{ 0x01030107, 			/* Sparc boot image */
		"sparc plan 9 boot image",
		nil,
		FSPARCB,
		0,
		&msparc,
		sizeof(struct sparcexec),
		beswal,
		sparcboot },
	{ U_MAGIC,			/* Sparc64 u.out */
		"sparc64 plan 9 executable",
		"sparc64 plan 9 dlm",
		FSPARC64,
		1,
		&msparc64,
		sizeof(Exec),
		beswal,
		adotout },
	{ A_MAGIC,			/* 68020 2.out & boot image */
		"68020 plan 9 executable",
		"68020 plan 9 dlm",
		F68020,
		1,
		&m68020,
		sizeof(Exec),
		beswal,
		common },
#elif HARVEYNEXT
	{ 0xFEEDFACE,			/* Next boot image */
		"next plan 9 boot image",
		nil,
		FNEXTB,
		0,
		&m68020,
		sizeof(struct nextexec),
		beswal,
		nextboot },
#elif HARVEY32
	{ I_MAGIC,			/* I386 8.out & boot image */
		"386 plan 9 executable",
		"386 plan 9 dlm",
		FI386,
		1,
		&mi386,
		sizeof(Exec),
		beswal,
		common },
#endif
	{ S_MAGIC,			/* amd64 6.out & boot image */
		"amd64 plan 9 executable",
		"amd64 plan 9 dlm",
		FAMD64,
		1,
		&mamd64,
		sizeof(Exec)+8,
		nil,
		commonllp64 },
#ifdef HARVEYPPC
	{ Q_MAGIC,			/* PowerPC q.out & boot image */
		"power plan 9 executable",
		"power plan 9 dlm",
		FPOWER,
		1,
		&mpower,
		sizeof(Exec),
		beswal,
		common },
	{ T_MAGIC,			/* power64 9.out & boot image */
		"power64 plan 9 executable",
		"power64 plan 9 dlm",
		FPOWER64,
		1,
		&mpower64,
		sizeof(Exec)+8,
		nil,
		commonllp64 },
#endif
	{ ELF_MAG,			/* any ELF */
		"elf executable",
		nil,
		FNONE,
		0,
/*		&mi386,
		sizeof(Ehdr), */
		&mamd64,
		sizeof(E64hdr),
		nil,
		elfdotout },
#ifdef HARVEYARM
	{ E_MAGIC,			/* Arm 5.out and boot image */
		"arm plan 9 executable",
		"arm plan 9 dlm",
		FARM,
		1,
		&marm,
		sizeof(Exec),
		beswal,
		common },
	{ (143<<16)|0413,		/* (Free|Net)BSD Arm */
		"arm *bsd executable",
		nil,
		FARM,
		0,
		&marm,
		sizeof(Exec),
		leswal,
		armdotout },
#elif HARVEYALPHA
	{ L_MAGIC,			/* alpha 7.out */
		"alpha plan 9 executable",
		"alpha plan 9 dlm",
		FALPHA,
		1,
		&malpha,
		sizeof(Exec),
		beswal,
		common },
	{ 0x0700e0c3,			/* alpha boot image */
		"alpha plan 9 boot image",
		nil,
		FALPHA,
		0,
		&malpha,
		sizeof(Exec),
		beswal,
		common },
#endif
	{ 0 },
};

#ifdef HARVEY32
Mach	*mach = &mi386;			/* Global current machine table */
#endif
Mach	*mach = &mamd64;

static ExecTable*
couldbe4k(ExecTable *mp)
{
//	Dir *d;
	ExecTable *f;

/* undefined for use with kernel
	if((d=dirstat("/proc/1/regs")) == nil)
		return mp;
	if(d->length < 32*8){		/ * R3000 * /
		free(d);
		return mp;
	}
	free(d); */
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
	int nb, ret;
	uint32_t magic;

	fp->type = FNONE;
	nb = read(fd, (char *)&d.e, sizeof(d.e));
	if (nb <= 0)
		return 0;

	ret = 0;
	magic = beswal(d.e.magic);		/* big-endian */
	for (mp = exectab; mp->magic; mp++) {
		if (nb < mp->hsize)
			continue;

		/*
		 * The magic number has morphed into something
		 * with fields (the straw was DYN_MAGIC) so now
		 * a flag is needed in Fhdr to distinguish _MAGIC()
		 * magic numbers from foreign magic numbers.
		 *
		 * This code is creaking a bit and if it has to
		 * be modified/extended much more it's probably
		 * time to step back and redo it all.
		 */
		if(mp->_magic){
			if(mp->magic != (magic & ~DYN_MAGIC))
				continue;

			if(mp->magic == V_MAGIC)
				mp = couldbe4k(mp);

			if ((magic & DYN_MAGIC) && mp->dlmname != nil)
				fp->name = mp->dlmname;
			else
				fp->name = mp->name;
		}
		else{
			if(mp->magic != magic)
				continue;
			fp->name = mp->name;
		}
		fp->type = mp->type;
		fp->hdrsz = mp->hsize;		/* will be zero on bootables */
		fp->_magic = mp->_magic;
		fp->magic = magic;

		mach = mp->mach;
		if(mp->swal != nil)
			hswal(&d, sizeof(d.e)/sizeof(uint32_t), mp->swal);
		ret = mp->hparse(fd, fp, &d);
		seek(fd, mp->hsize, 0);		/* seek to end of header */
		break;
	}
	if(mp->magic == 0)
		werrstr("unknown header type");
	return ret;
}

/*
 * Convert header to canonical form
 */
static void
hswal(void *v, int n, uint32_t (*swap)(uint32_t))
{
	uint32_t *ulp;

	for(ulp = v; n--; ulp++)
		*ulp = (*swap)(*ulp);
}

/*
 *	Crack a normal a.out-type header
 */
static int
adotout(int fd, Fhdr *fp, ExecHdr *hp)
{
	int32_t pgsize;

	USED(fd);
	pgsize = mach->pgsize;
	settext(fp, hp->e.entry, pgsize+sizeof(Exec),
			hp->e.text, sizeof(Exec));
	setdata(fp, _round(pgsize+fp->txtsz+sizeof(Exec), pgsize),
		hp->e.data, fp->txtsz+sizeof(Exec), hp->e.bss);
	setsym(fp, hp->e.syms, hp->e.spsz, hp->e.pcsz, fp->datoff+fp->datsz);
	return 1;
}

static void
commonboot(Fhdr *fp)
{
	if (!(fp->entry & mach->ktmask))
		return;

	switch(fp->type) {				/* boot image */
	case F68020:
		fp->type = F68020B;
		fp->name = "68020 plan 9 boot image";
		break;
	case FI386:
		fp->type = FI386B;
		fp->txtaddr = (uint32_t)fp->entry;
		fp->name = "386 plan 9 boot image";
		fp->dataddr = _round(fp->txtaddr+fp->txtsz, mach->pgsize);
		break;
	case FARM:
		fp->type = FARMB;
		fp->txtaddr = (uint32_t)fp->entry;
		fp->name = "ARM plan 9 boot image";
		fp->dataddr = _round(fp->txtaddr+fp->txtsz, mach->pgsize);
		return;
	case FALPHA:
		fp->type = FALPHAB;
		fp->txtaddr = (uint32_t)fp->entry;
		fp->name = "alpha plan 9 boot image";
		fp->dataddr = fp->txtaddr+fp->txtsz;
		break;
	case FPOWER:
		fp->type = FPOWERB;
		fp->txtaddr = (uint32_t)fp->entry;
		fp->name = "power plan 9 boot image";
		fp->dataddr = fp->txtaddr+fp->txtsz;
		break;
	case FAMD64:
		fp->type = FAMD64B;
		fp->txtaddr = fp->entry;
		fp->name = "amd64 plan 9 boot image";
		fp->dataddr = _round(fp->txtaddr+fp->txtsz, 4096);
		break;
	case FPOWER64:
		fp->type = FPOWER64B;
		fp->txtaddr = fp->entry;
		fp->name = "power64 plan 9 boot image";
		fp->dataddr = fp->txtaddr+fp->txtsz;
		break;
	default:
		return;
	}
	fp->hdrsz = 0;			/* header stripped */
}

/*
 *	_MAGIC() style headers and
 *	alpha plan9-style bootable images for axp "headerless" boot
 *
 */
static int
common(int fd, Fhdr *fp, ExecHdr *hp)
{
	adotout(fd, fp, hp);
	if(hp->e.magic & DYN_MAGIC) {
		fp->txtaddr = 0;
		fp->dataddr = fp->txtsz;
		return 1;
	}
	commonboot(fp);
	return 1;
}

static int
commonllp64(int i, Fhdr *fp, ExecHdr *hp)
{
	int32_t pgsize;
	uint64_t entry;

	hswal(&hp->e, sizeof(Exec)/sizeof(int32_t), beswal);
	if(!(hp->e.magic & HDR_MAGIC))
		return 0;

	/*
	 * There can be more magic here if the
	 * header ever needs more expansion.
	 * For now just catch use of any of the
	 * unused bits.
	 */
	if((hp->e.magic & ~DYN_MAGIC)>>16)
		return 0;
	entry = beswav(hp->e.hdr[0]);

	pgsize = mach->pgsize;
	settext(fp, entry, pgsize+fp->hdrsz, hp->e.text, fp->hdrsz);
	setdata(fp, _round(pgsize+fp->txtsz+fp->hdrsz, pgsize),
		hp->e.data, fp->txtsz+fp->hdrsz, hp->e.bss);
	setsym(fp, hp->e.syms, hp->e.spsz, hp->e.pcsz, fp->datoff+fp->datsz);

	if(hp->e.magic & DYN_MAGIC) {
		fp->txtaddr = 0;
		fp->dataddr = fp->txtsz;
		return 1;
	}
	commonboot(fp);
	return 1;
}

#ifdef HARVEYMIPS
/*
 *	mips bootable image.
 */
static int
mipsboot(int fd, Fhdr *fp, ExecHdr *hp)
{
	USED(fd);
	fp->type = FMIPSB;
	switch(hp->e.amagic) {
	default:
	case 0407:	/* some kind of mips */
		settext(fp, (uint32_t)hp->e.mentry,
			(uint32_t)hp->e.text_start,
			hp->e.tsize, sizeof(struct mipsexec)+4);
		setdata(fp, (uint32_t)hp->e.data_start, hp->e.dsize,
			fp->txtoff+hp->e.tsize, hp->e.bsize);
		break;
	case 0413:	/* some kind of mips */
		settext(fp, (uint32_t)hp->e.mentry,
			(uint32_t)hp->e.text_start,
			hp->e.tsize, 0);
		setdata(fp, (uint32_t)hp->e.data_start, hp->e.dsize,
			hp->e.tsize, hp->e.bsize);
		break;
	}
	setsym(fp, hp->e.nsyms, 0, hp->e.pcsize, hp->e.symptr);
	fp->hdrsz = 0;			/* header stripped */
	return 1;
}

/*
 *	mips4k bootable image.
 */
static int
mips4kboot(int fd, Fhdr *fp, ExecHdr *hp)
{
	USED(fd);
	fp->type = FMIPSB;
	switch(hp->e.h.amagic) {
	default:
	case 0407:	/* some kind of mips */
		settext(fp, (uint32_t)hp->e.h.mentry,
			(uint32_t)hp->e.h.text_start,
			hp->e.h.tsize, sizeof(struct mips4kexec));
		setdata(fp, (uint32_t)hp->e.h.data_start, hp->e.h.dsize,
			fp->txtoff+hp->e.h.tsize, hp->e.h.bsize);
		break;
	case 0413:	/* some kind of mips */
		settext(fp, (uint32_t)hp->e.h.mentry,
			(uint32_t)hp->e.h.text_start,
			hp->e.h.tsize, 0);
		setdata(fp, (uint32_t)hp->e.h.data_start, hp->e.h.dsize,
			hp->e.h.tsize, hp->e.h.bsize);
		break;
	}
	setsym(fp, hp->e.h.nsyms, 0, hp->e.h.pcsize, hp->e.h.symptr);
	fp->hdrsz = 0;			/* header stripped */
	return 1;
}
#endif
#ifdef HARVEYSPARC
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
	fp->hdrsz = 0;			/* header stripped */
	return 1;
}
#endif
#ifdef HARVEYNEXT
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
	fp->hdrsz = 0;			/* header stripped */
	return 1;
}
#endif

/*
 * ELF64 binaries.
 */
static int
elf64dotout(int fd, Fhdr *fp, ExecHdr *hp)
{
	E64hdr *ep;
	P64hdr *ph;
	uint16_t (*swab)(uint16_t);
	uint32_t (*swal)(uint32_t);
	uint64_t (*swav)(uint64_t);
	int i, it, id, is, phsz;
	uint64_t uvl;

	ep = &hp->e;
	if(ep->ident[DATA] == ELFDATA2LSB) {
		swab = leswab;
		swal = leswal;
		swav = leswav;
	} else if(ep->ident[DATA] == ELFDATA2MSB) {
		swab = beswab;
		swal = beswal;
		swav = beswav;
	} else {
		werrstr("bad ELF64 encoding - not big or little endian");
		return 0;
	}

	ep->type = swab(ep->type);
	ep->machine = swab(ep->machine);
	ep->version = swal(ep->version);
	if(ep->type != EXEC || ep->version != CURRENT)
		return 0;
	ep->elfentry = swav(ep->elfentry);
	ep->phoff = swav(ep->phoff);
	ep->shoff = swav(ep->shoff);
	ep->flags = swal(ep->flags);
	ep->ehsize = swab(ep->ehsize);
	ep->phentsize = swab(ep->phentsize);
	ep->phnum = swab(ep->phnum);
	ep->shentsize = swab(ep->shentsize);
	ep->shnum = swab(ep->shnum);
	ep->shstrndx = swab(ep->shstrndx);

	fp->magic = ELF_MAG;
	fp->hdrsz = (ep->ehsize+ep->phnum*ep->phentsize+16)&~15;
	switch(ep->machine) {
	default:
		return 0;
	case AMD64:
		mach = &mamd64;
		fp->type = FAMD64;
		fp->name = "amd64 ELF64 executable";
		break;
	case POWER64:
#ifdef HARVEYPPC
		mach = &mpower64;
#endif
		fp->type = FPOWER64;
		fp->name = "power64 ELF64 executable";
		break;
	}

	if(ep->phentsize != sizeof(P64hdr)) {
		werrstr("bad ELF64 header size");
		return 0;
	}
	phsz = sizeof(P64hdr)*ep->phnum;
	ph = malloc(phsz);
	if(!ph)
		return 0;
	seek(fd, ep->phoff, 0);
	if(read(fd, ph, phsz) < 0) {
		free(ph);
		return 0;
	}
	for(i = 0; i < ep->phnum; i++) {
		ph[i].type = swal(ph[i].type);
		ph[i].flags = swal(ph[i].flags);
		ph[i].offset = swav(ph[i].offset);
		ph[i].vaddr = swav(ph[i].vaddr);
		ph[i].paddr = swav(ph[i].paddr);
		ph[i].filesz = swav(ph[i].filesz);
		ph[i].memsz = swav(ph[i].memsz);
		ph[i].align = swav(ph[i].align);
	}

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
		werrstr("No ELF64 TEXT or DATA sections");
		free(ph);
		return 0;
	}

	settext(fp, ep->elfentry, ph[it].vaddr, ph[it].memsz, ph[it].offset);
	/* 8c: out of fixed registers */
	uvl = ph[id].memsz - ph[id].filesz;
	setdata(fp, ph[id].vaddr, ph[id].filesz, ph[id].offset, uvl);
	if(is != -1)
		setsym(fp, ph[is].filesz, 0, ph[is].memsz, ph[is].offset);
	free(ph);
	return 1;
}

#ifdef HARVEY32
/*
 * ELF32 binaries.
 */
static int
elf32dotout(int fd, Fhdr *fp, ExecHdr *hp)
{
	uint32_t (*swal)(uint32_t);
	uint16_t (*swab)(uint16_t);
	Ehdr *ep;
	Phdr *ph;
	int i, it, id, is, phsz;

	/* bitswap the header according to the DATA format */
	ep = &hp->e;
	if(ep->ident[DATA] == ELFDATA2LSB) {
		swab = leswab;
		swal = leswal;
	} else if(ep->ident[DATA] == ELFDATA2MSB) {
		swab = beswab;
		swal = beswal;
	} else {
		werrstr("bad ELF32 encoding - not big or little endian");
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
		fp->name = "386 ELF32 executable";
		break;
	case MIPS:
		mach = &mmips;
		fp->type = FMIPS;
		fp->name = "mips ELF32 executable";
		break;
	case SPARC64:
		mach = &msparc64;
		fp->type = FSPARC64;
		fp->name = "sparc64 ELF32 executable";
		break;
	case POWER:
		mach = &mpower;
		fp->type = FPOWER;
		fp->name = "power ELF32 executable";
		break;
	case POWER64:
		mach = &mpower64;
		fp->type = FPOWER64;
		fp->name = "power64 ELF32 executable";
		break;
	case AMD64:
		mach = &mamd64;
		fp->type = FAMD64;
		fp->name = "amd64 ELF32 executable";
		break;
	case ARM:
		mach = &marm;
		fp->type = FARM;
		fp->name = "arm ELF32 executable";
		break;
	default:
		return 0;
	}

	if(ep->phentsize != sizeof(Phdr)) {
		werrstr("bad ELF32 header size");
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
	hswal(ph, phsz/sizeof(uint32_t), swal);

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
		 * ph[0] : txtsz+datsz, txtsz+datsz+bsssz, txtaddr-KZERO, datasize, txtoff
		 * ph[1] : symsz, lcsz, 0, 0, symoff
		 */
		if(ep->machine == SPARC64 && ep->phnum == 2) {
			uint32_t txtaddr, txtsz, dataddr, bsssz;

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

		werrstr("No ELF32 TEXT or DATA sections");
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

#endif
/*
 * Elf binaries.
 */
static int
elfdotout(int fd, Fhdr *fp, ExecHdr *hp)
{
//	Ehdr *ep;
	E64hdr *ep;

	/* bitswap the header according to the DATA format */
	ep = &hp->e;
//	if(ep->ident[CLASS] == ELFCLASS32)
//		return elf32dotout(fd, fp, hp);
//	else if(ep->ident[CLASS] == ELFCLASS64)
	if(ep->ident[CLASS] == ELFCLASS64)
		return elf64dotout(fd, fp, hp);

//	werrstr("bad ELF class - not 32- nor 64-bit");
	werrstr("bad ELF class - not 64-bit");
	return 0;
}

/*
 * (Free|Net)BSD ARM header.
 */
static int
armdotout(int fd, Fhdr *fp, ExecHdr *hp)
{
	uint64_t kbase;

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
settext(Fhdr *fp, uint64_t e, uint64_t a, int32_t s, int64_t off)
{
	fp->txtaddr = a;
	fp->entry = e;
	fp->txtsz = s;
	fp->txtoff = off;
}

static void
setdata(Fhdr *fp, uint64_t a, int32_t s, int64_t off, int32_t bss)
{
	fp->dataddr = a;
	fp->datsz = s;
	fp->datoff = off;
	fp->bsssz = bss;
}

static void
setsym(Fhdr *fp, int32_t symsz, int32_t sppcsz, int32_t lnpcsz,
       int64_t symoff)
{
	fp->symsz = symsz;
	fp->symoff = symoff;
	fp->sppcsz = sppcsz;
	fp->sppcoff = fp->symoff+fp->symsz;
	fp->lnpcsz = lnpcsz;
	fp->lnpcoff = fp->sppcoff+fp->sppcsz;
}


static uint64_t
_round(uint64_t a, uint32_t b)
{
	uint64_t w;

	w = (a/b)*b;
	if (a!=w)
		w += b;
	return(w);
}

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
		struct i960exec;
	} e;
	long dummy;		/* padding to ensure extra long */
} ExecHdr;

static	int	i960boot(int, Fhdr*, ExecHdr*);
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
	int	(*hparse)(int, Fhdr*, ExecHdr*);
} ExecTable;

extern	Mach	mmips;
extern	Mach	mmips2le;
extern	Mach	mmips2be;
extern	Mach	msparc;
extern	Mach	m68020;
extern	Mach	mi386;
extern	Mach	mi960;
extern	Mach	m3210;
extern	Mach	m29000;
extern	Mach	marm;
extern	Mach	mpower;
extern	Mach	malpha;

ExecTable exectab[] =
{
	{ V_MAGIC,			/* Mips v.out */
		"mips plan 9 executable",
		FMIPS,
		&mmips,
		sizeof(Exec),
		beswal,
		adotout },
	{ M_MAGIC,			/* Mips 4.out */
		"mips 4k plan 9 executable BE",
		FMIPS2BE,
		&mmips2be,
		sizeof(Exec),
		beswal,
		adotout },
	{ N_MAGIC,			/* Mips 0.out */
		"mips 4k plan 9 executable LE",
		FMIPS2LE,
		&mmips2le,
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
		&mmips2be,
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
	{ I_MAGIC|DYN_MAGIC,	/* I386 dynamic load module */
		"386 plan 9 dynamically loaded module",
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
	{ D_MAGIC,			/* 29000 9.out */
		"29000 plan 9 executable",
		F29000,
		&m29000,
		sizeof(Exec),
		beswal,
		adotout },
	{ Q_MAGIC,			/* PowerPC q.out & boot image */
		"power plan 9 executable",
		FPOWER,
		&mpower,
		sizeof(Exec),
		beswal,
		common },
	{ ELF_MAG,
		"Irix 5.X Elf executable",
		FMIPS,
		&mmips,
		sizeof(Ehdr),
		beswal,
		elfdotout },
	{ E_MAGIC,			/* Arm 5.out */
		"Arm plan 9 executable",
		FARM,
		&marm,
		sizeof(Exec),
		beswal,
		common },
	{ (143<<16)|0413,		/* (Free|Net)BSD Arm */
		"Arm *BSD executable",
		FARM,
		&marm,
		sizeof(Exec),
		leswal,
		armdotout },
	{ L_MAGIC,			/* alpha 7.out */
		"alpha plan 9 executable",
		FALPHA,
		&malpha,
		sizeof(Exec),
		beswal,
		common },
	{ 0x0700e0c3,			/* alpha boot image */
		"alpha plan 9 boot image",
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
		if (mp->magic == magic && nb >= mp->hsize) {
			if(mp->magic == V_MAGIC)
				mp = couldbe4k(mp);

			hswal((long *) &d, sizeof(d.e)/sizeof(long), mp->swal);
			fp->type = mp->type;
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
 *	I960 bootable image
 */
static int
i960boot(int fd, Fhdr *fp, ExecHdr *hp)
{
	/* long n = hp->e.i6comments.fptrlineno-hp->e.i6comments.fptrreloc; */

	USED(fd);
	settext(fp, hp->e.i6entry, hp->e.i6texts.virt, hp->e.i6texts.size,
					hp->e.i6texts.fptr);
	setdata(fp, hp->e.i6datas.virt, hp->e.i6datas.size,
				hp->e.i6datas.fptr, hp->e.i6bsssize);
	setsym(fp, 0, 0, 0, 0);
	/*setsym(fp, n, 0, hp->e.i6comments.size-n, hp->e.i6comments.fptr); */
	fp->hdrsz = 0;		/* header stripped */
	return 1;
}

static Shdr*
elfsectbyname(int fd, Ehdr *hp, Shdr *sp, char *name)
{
	int i, offset, n;
	char s[64];

	offset = sp[hp->shstrndx].offset;
	for(i = 1; i < hp->shnum; i++) {
		seek(fd, offset+sp[i].name, 0);
		n = read(fd, s, sizeof(s)-1);
		if(n < 0)
			continue;
		s[n] = 0;
		if(strcmp(s, name) == 0)
			return &sp[i]; 
	}
	return 0;
}
/*
 *	Decode an Irix 5.x ELF header
 */
static int
elfdotout(int fd, Fhdr *fp, ExecHdr *hp)
{

	Ehdr *ep;
	Shdr *es, *txt, *init, *s;
	long addr, size, offset, bsize;

	ep = &hp->e;
	if(ep->type != 8 || ep->machine != 2 || ep->version != 1)
		return 0;

	fp->magic = ELF_MAG;
	fp->hdrsz = (ep->ehsize+ep->phnum*ep->phentsize+16)&~15;

	if(ep->shnum <= 0) {
		werrstr("no ELF header sections");
		return 0;
	}
	es = malloc(sizeof(Shdr)*ep->shnum);
	if(es == 0)
		return 0;

	seek(fd, ep->shoff, 0);
	if(read(fd, es, sizeof(Shdr)*ep->shnum) < 0){
		free(es);
		return 0;
	}

	txt = elfsectbyname(fd, ep, es, ".text");
	init = elfsectbyname(fd, ep, es, ".init");
	if(txt == 0 || init == 0 || init != txt+1)
		goto bad;
	if(txt->addr+txt->size != init->addr)
		goto bad;
	settext(fp, ep->elfentry, txt->addr, txt->size+init->size, txt->offset);

	addr = 0;
	offset = 0;
	size = 0;
	s = elfsectbyname(fd, ep, es, ".data");
	if(s) {
		addr = s->addr;
		size = s->size;
		offset = s->offset;
	}

	s = elfsectbyname(fd, ep, es, ".rodata");
	if(s) {
		if(addr){
			if(addr+size != s->addr)
				goto bad;
		} else {
			addr = s->addr;
			offset = s->offset;
		}
		size += s->size;
	}

	s = elfsectbyname(fd, ep, es, ".got");
	if(s) {
		if(addr){
			if(addr+size != s->addr)
				goto bad;
		} else {
			addr = s->addr;
			offset = s->offset;
		}
		size += s->size;
	}

	bsize = 0;
	s = elfsectbyname(fd, ep, es, ".bss");
	if(s) {
		if(addr){
			if(addr+size != s->addr)
				goto bad;
		} else {
			addr = s->addr;
			offset = s->offset;
		}
		bsize = s->size;
	}

	if(addr == 0)
		goto bad;

	setdata(fp, addr, size, offset, bsize);
	fp->name = "IRIX Elf a.out executable";
	free(es);
	return 1;
bad:
	free(es);
	werrstr("ELF sections scrambled");
	return 0;
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

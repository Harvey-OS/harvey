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
		E64hdr E64hdr;
	} e;
	int32_t dummy;			/* padding to ensure extra long */
} ExecHdr;

static int	elfdotout(int, Fhdr*, ExecHdr*);
static void	setsym(Fhdr*, int32_t, int32_t, int32_t, int64_t);
static void	setdata(Fhdr*, uint64_t, int32_t, int64_t, int32_t);
static void	settext(Fhdr*, uint64_t, uint64_t, int32_t, int64_t);
static void	setstr(Fhdr *fp, int64_t stroff, uint64_t strsz);
static void	setbssidx(Fhdr *fp, uint16_t bssidx);
static void	setdbglineidx(Fhdr *fp, uint16_t dbglineidx);
static void	hswal(void*, int, uint32_t(*)(uint32_t));

/*
 *	definition of per-executable file type structures
 */

typedef struct Exectable{
	int32_t		magic;			/* big-endian magic number of file */
	char		*name;			/* executable identifier */
	char		*dlmname;		/* dynamically loadable module identifier */
	uint8_t		type;			/* Internal code */
	Mach		*mach;			/* Per-machine data */
	int32_t		hsize;			/* header size */
	uint32_t	(*swal)(uint32_t);	/* beswal or leswal */
	int		(*hparse)(int, Fhdr*, ExecHdr*);
} ExecTable;

extern	Mach	mamd64;

ExecTable exectab[] =
{
	{ ELF_MAG,			/* any ELF */
		"elf executable",
		nil,
		FNONE,
		&mamd64,
		sizeof(E64hdr),
		nil,
		elfdotout },
	{ 0 },
};

Mach	*mach = &mamd64;

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

		if(mp->magic != magic)
			continue;

		fp->name = mp->name;
		fp->type = mp->type;
		fp->hdrsz = mp->hsize;		/* will be zero on bootables */
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
 * ELF64 binaries.
 */
static int
elf64dotout(int fd, Fhdr *fp, ExecHdr *hp)
{
	uint16_t (*swab)(uint16_t);
	uint32_t (*swal)(uint32_t);
	uint64_t (*swav)(uint64_t);

	E64hdr *ep = &hp->e.E64hdr;
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
	fp->bigendian = ep->ident[DATA] == ELFDATA2MSB;

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
	}

	// Program headers
	if(ep->phentsize != sizeof(P64hdr)) {
		werrstr("bad ELF64 program header size");
		return 0;
	}
	int phsz = sizeof(P64hdr)*ep->phnum;
	P64hdr *ph = malloc(phsz);
	if(!ph)
		return 0;
	seek(fd, ep->phoff, 0);
	if(read(fd, ph, phsz) < 0) {
		free(ph);
		return 0;
	}

	for(int i = 0; i < ep->phnum; i++) {
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
	int it = -1, id = -1;
	for(int i = 0; i < ep->phnum; i++) {
		if(ph[i].type == LOAD && (ph[i].flags & (R|X)) == (R|X) && it == -1)
			it = i;
		else if(ph[i].type == LOAD && (ph[i].flags & (R|W)) == (R|W) && id == -1)
			id = i;
	}
	if(it == -1 || id == -1) {
		werrstr("No ELF64 TEXT or DATA sections");
		free(ph);
		return 0;
	}

	settext(fp, ep->elfentry, ph[it].vaddr, ph[it].memsz, ph[it].offset);
	uint64_t uvl = ph[id].memsz - ph[id].filesz;
	setdata(fp, ph[id].vaddr, ph[id].filesz, ph[id].offset, uvl);
	free(ph);

	// Section headers - get the symbol table offset from here
	if (ep->shentsize != sizeof(S64hdr)) {
		werrstr("bad ELF64 section header size");
		return 0;
	}
	int shsz = sizeof(S64hdr)*ep->shnum;
	S64hdr *sh = malloc(shsz);
	if (!sh) {
		return 0;
	}
	seek(fd, ep->shoff, 0);
	if (read(fd, sh, shsz) < 0) {
		free(sh);
		return 0;
	}

	for (int i = 0; i < ep->shnum; i++) {
		sh[i].name = swal(sh[i].name);
		sh[i].type = swal(sh[i].type);
		sh[i].flags = swav(sh[i].flags);
		sh[i].addr = swav(sh[i].addr);
		sh[i].offset = swav(sh[i].offset);
		sh[i].size = swav(sh[i].size);
		sh[i].link = swal(sh[i].link);
		sh[i].info = swal(sh[i].info);
		sh[i].addralign = swav(sh[i].addralign);
		sh[i].entsize = swav(sh[i].entsize);
	}

	int isym = -1, istr = -1, ibss = -1;
	for (int i = 0; i < ep->shnum; i++) {
		if (sh[i].type == SHT_SYMTAB && isym == -1) {
			// Assume the first is the one we want for now
			// There may be more than one if it's dynamic, but we
			// don't support than, so hopefully this is ok for now
			isym = i;
		} else if (sh[i].type == SHT_STRTAB && istr == -1) {
			// Assume first is the one we want for now, but we
			// should probably check that the name is '.strtab' to
			// distinguish from .shstrtab.
			istr = i;
		} else if (sh[i].type == SHT_NOBITS && ibss == -1) {
			ibss = i;
		}
	}

	if (isym != -1) {
		//print("isym: %d\n", isym);
		setsym(fp, sh[isym].size, 0, sh[isym].size, sh[isym].offset);
	}

	if (istr != -1) {
		//print("istr: %d\n", istr);
		setstr(fp, sh[istr].offset, sh[istr].size);
	}

	if (ibss != -1) {
		//print("ibss: %d\n", ibss);
		setbssidx(fp, ibss);
	}

	// Get sections based on names - load the section string table first
	if (ep->shstrndx != SHN_UNDEF) {
		uint64_t shstroff = sh[ep->shstrndx].offset;
		uint64_t shstrsz = sh[ep->shstrndx].size;
		char *shstrtab = (char *)malloc(shstrsz);
		if (shstrtab == 0) {
			werrstr("can't allocate memory to load section string table");
			free(sh);
			return 0;
		}

		Biobuf b;
		Binit(&b, fd, OREAD);
		Bseek(&b, shstroff, 0);
		if (Bread(&b, shstrtab, shstrsz) != shstrsz) {
			werrstr("can't read section string table");
			free(shstrtab);
			free(sh);
			return 0;
		}

		int idbg_line = -1;
		for (int i = 0; i < ep->shnum; i++) {
			const char *secname = &shstrtab[sh[i].name];
			//print("secname[%d] = %s\n", i, secname);
			if (sh[i].type == SHT_PROGBITS
				&& !strcmp(".debug_line", secname)
				&& idbg_line == -1) {
				idbg_line = i;
			}
		}

		if (idbg_line != -1) {
			//print("idbg_line: %d\n", idbg_line);
			setdbglineidx(fp, ibss);
		}

		free(shstrtab);
	}

	free(sh);

	return 1;
}

/*
 * Elf binaries.
 */
static int
elfdotout(int fd, Fhdr *fp, ExecHdr *hp)
{
	/* bitswap the header according to the DATA format */
	E64hdr *ep = &hp->e.E64hdr;
	if(ep->ident[CLASS] == ELFCLASS64)
		return elf64dotout(fd, fp, hp);

	werrstr("bad ELF class - not 64-bit");
	return 0;
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
setsym(Fhdr *fp, int32_t symsz, int32_t sppcsz, int32_t lnpcsz, int64_t symoff)
{
	fp->symsz = symsz;
	fp->symoff = symoff;
	fp->sppcsz = sppcsz;
	fp->sppcoff = fp->symoff+fp->symsz;
	fp->lnpcsz = lnpcsz;
	fp->lnpcoff = fp->sppcoff+fp->sppcsz;
}
 
static void
setstr(Fhdr *fp, int64_t stroff, uint64_t strsz)
{
	fp->stroff = stroff;
	fp->strsz = strsz;
}

static void
setbssidx(Fhdr *fp, uint16_t bssidx)
{
	fp->bssidx = bssidx;
}

static void
setdbglineidx(Fhdr *fp, uint16_t dbglineidx)
{
	fp->dbglineidx = dbglineidx;
}
/*
 * proprietary exec headers, needed to bootstrap various machines
 */
struct mipsexec
{
	short	mmagic;		/* (0x160) mips magic number */
	short	nscns;		/* (unused) number of sections */
	long	timdat;		/* (unused) time & date stamp */
	long	symptr;		/* offset to symbol table */
	long	nsyms;		/* size of symbol table */
	short	opthdr;		/* (0x38) sizeof(optional hdr) */
	short	pcszs;		/* flags */
	short	amagic;		/* see above */
	short	vstamp;		/* version stamp */
	long	tsize;		/* text size in bytes */
	long	dsize;		/* initialized data */
	long	bsize;		/* uninitialized data */
	long	mentry;		/* entry pt.				*/
	long	text_start;	/* base of text used for this file	*/
	long	data_start;	/* base of data used for this file	*/
	long	bss_start;	/* base of bss used for this file	*/
	long	gprmask;	/* general purpose register mask	*/
union{
	long	cprmask[4];	/* co-processor register masks		*/
	long	pcsize;
};
	long	gp_value;	/* the gp value used for this object    */
};

struct sparcexec
{
	short	sjunk;		/* dynamic bit and version number */
	short	smagic;		/* 0407 */
	ulong	stext;
	ulong	sdata;
	ulong	sbss;
	ulong	ssyms;
	ulong	sentry;
	ulong	strsize;
	ulong	sdrsize;
};

struct nextexec
{
	struct	nexthdr{
		ulong	nmagic;
		ulong	ncputype;
		ulong	ncpusubtype;
		ulong	nfiletype;
		ulong	ncmds;
		ulong	nsizeofcmds;
		ulong	nflags;
	};

	struct nextcmd{
		ulong	cmd;
		ulong	cmdsize;
		uchar	segname[16];
		ulong	vmaddr;
		ulong	vmsize;
		ulong	fileoff;
		ulong	filesize;
		ulong	maxprot;
		ulong	initprot;
		ulong	nsects;
		ulong	flags;
	}textc;
	struct nextsect{
		char	sectname[16];
		char	segname[16];
		ulong	addr;
		ulong	size;
		ulong	offset;
		ulong	align;
		ulong	reloff;
		ulong	nreloc;
		ulong	flags;
		ulong	reserved1;
		ulong	reserved2;
	}texts;
	struct nextcmd	datac;
	struct nextsect	datas;
	struct nextsect	bsss;
	struct nextsym{
		ulong	cmd;
		ulong	cmdsize;
		ulong	symoff;
		ulong	nsyms;
		ulong	spoff;
		ulong	pcoff;
	}symc;
};

struct i386exec
{
	struct	i386coff{
		ulong	isectmagic;
		ulong	itime;
		ulong	isyms;
		ulong	insyms;
		ulong	iflags;
	};
	struct	i386hdr{
		ulong	imagic;
		ulong	itextsize;
		ulong	idatasize;
		ulong	ibsssize;
		ulong	ientry;
		ulong	itextstart;
		ulong	idatastart;
	};
	struct i386sect{
		char	name[8];
		ulong	phys;
		ulong	virt;
		ulong	size;
		ulong	fptr;
		ulong	fptrreloc;
		ulong	fptrlineno;
		ulong	nrelocnlineno;
		ulong	flags;
	}itexts;
	struct i386sect idatas;
	struct i386sect ibsss;
	struct i386sect icomments;
};

struct i960exec
{
	struct	i960coff{
		ulong	i6sectmagic;
		ulong	i6time;
		ulong	i6syms;
		ulong	i6nsyms;
		ulong	i6opthdrflags;
	};
	struct	i960hdr{
		ulong	i6magic;
		ulong	i6textsize;
		ulong	i6datasize;
		ulong	i6bsssize;
		ulong	i6entry;
		ulong	i6textstart;
		ulong	i6datastart;
		ulong	i6tagentries;
	};
	struct i960sect{
		char	name[8];
		ulong	phys;
		ulong	virt;
		ulong	size;
		ulong	fptr;
		ulong	fptrreloc;
		ulong	fptrlineno;
		ulong	nrelocnlineno;
		ulong	flags;
		ulong	align;
	}i6texts;
	struct i960sect i6datas;
};

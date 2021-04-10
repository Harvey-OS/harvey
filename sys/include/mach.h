/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 *	Architecture-dependent application data
 */
#include "elf.h"
/*
 *	Supported architectures:
 *		mips,
 *		68020,
 *		i386,
 *		amd64,
 *		sparc,
 *		mips2 (R4000)
 *		arm
 *		powerpc,
 *		powerpc64
 *		arm64
 */
enum
{
	MMIPS,			/* machine types */
	MSPARC,
	M68020,
	MI386,
	MI960,			/* retired */
	M3210,			/* retired */
	MMIPS2,
	NMIPS2,
	M29000,			/* retired */
	MARM,
	MPOWER,
	MALPHA,			/* retired */
	NMIPS,
	MSPARC64,		/* retired */
	MAMD64,
	MPOWER64,
	MARM64,
				/* types of executables */
	FNONE = 0,		/* unidentified */
	FMIPS,			/* v.out */
	FMIPSB,			/* mips bootable */
	FSPARC,			/* k.out */
	FSPARCB,		/* Sparc bootable */
	F68020,			/* 2.out */
	F68020B,		/* 68020 bootable */
	FNEXTB,			/* Next bootable */
	FI386,			/* 8.out */
	FI386B,			/* I386 bootable */
	FI960,			/* retired */
	FI960B,			/* retired */
	F3210,			/* retired */
	FMIPS2BE,		/* 4.out */
	F29000,			/* retired */
	FARM,			/* 5.out */
	FARMB,			/* ARM bootable */
	FPOWER,			/* q.out */
	FPOWERB,		/* power pc bootable */
	FMIPS2LE,		/* 0.out */
	FALPHA,			/* retired */
	FALPHAB,		/* retired DEC Alpha bootable */
	FMIPSLE,		/* 3k little endian */
	FSPARC64,		/* retired */
	FAMD64,			/* 6.out */
	FAMD64B,		/* 6.out bootable */
	FPOWER64,		/* 9.out */
	FPOWER64B,		/* 9.out bootable */
	FARM64,			/* arm64 */
	FARM64B,		/* arm64 bootable */

	ANONE = 0,		/* dissembler types */
	AMIPS,
	AMIPSCO,		/* native mips */
	ASPARC,
	ASUNSPARC,		/* native sun */
	A68020,
	AI386,
	AI8086,			/* oh god */
	AI960,			/* retired */
	A29000,			/* retired */
	AARM,
	APOWER,
	AALPHA,			/* retired */
	ASPARC64,		/* retired */
	AAMD64,
	APOWER64,
	AARM64,
				/* object file types */
	Obj68020 = 0,		/* .2 */
	ObjSparc,		/* .k */
	ObjMips,		/* .v */
	Obj386,			/* .8 */
	Obj960,			/* retired */
	Obj3210,		/* retired */
	ObjMips2,		/* .4 */
	Obj29000,		/* retired */
	ObjArm,			/* .5 */
	ObjPower,		/* .q */
	ObjMips2le,		/* .0 */
	ObjAlpha,		/* retired */
	ObjSparc64,		/* retired */
	ObjAmd64,		/* .6 */
	ObjSpim,		/* .0 */
	ObjPower64,		/* .9 */
	ObjArm64,		/* .4? */
	Maxobjtype,

	CNONE  = 0,		/* symbol table classes */
	CAUTO,
	CPARAM,
	CSTAB,
	CTEXT,
	CDATA,
	CANY,			/* to look for any class */
};

typedef	struct	Map	Map;
typedef struct	Symbol	Symbol;
typedef	struct	Reglist	Reglist;
typedef	struct	Mach	Mach;
typedef	struct	Machdata Machdata;

/*
 * 	Structure to map a segment to a position in a file
 */
struct Map {
	int	nsegs;			/* number of segments */
	struct segment {		/* per-segment map */
		char		*name;	/* the segment name */
		int		fd;	/* file descriptor */
		int		inuse;	/* in use - not in use */
		int		cache;	/* should cache reads? */
		u64	b;	/* base */
		u64	e;	/* end */
		i64	f;		/* offset within file */
	} seg[1];			/* actually n of these */
};

/*
 *	Internal structure describing a symbol table entry
 */
struct Symbol {
	void 	*handle;		/* used internally - owning func */
	struct {
		char	*name;
		i64	value;		/* address or stack offset */
		char	type;		/* as in a.out.h */
		char	class;		/* as above */
		int	index;		/* in findlocal, globalsym, textsym */
	};
};

/*
 *	machine register description
 */
struct Reglist {
	char	*rname;			/* register name */
	i16	roffs;			/* offset in u-block */
	char	rflags;			/* INTEGER/FLOAT, WRITABLE */
	char	rformat;		/* print format: 'x', 'X', 'f', '8', '3', 'Y', 'W' */
};

enum {					/* bits in rflags field */
	RINT	= (0<<0),
	RFLT	= (1<<0),
	RRDONLY	= (1<<1),
};

/*
 *	Machine-dependent data is stored in two structures:
 *		Mach  - miscellaneous general parameters
 *		Machdata - jump vector of service functions used by debuggers
 *
 *	Mach is defined in ?.c and set in executable.c
 *
 *	Machdata is defined in ?db.c
 *		and set in the debugger startup.
 */
struct Mach{
	char		*name;
	int		mtype;		/* machine type code */
	Reglist 	*reglist;	/* register set */
	i32		regsize;	/* sizeof registers in bytes */
	i32		fpregsize;	/* sizeof fp registers in bytes */
	char		*pc;		/* pc name */
	char		*sp;		/* sp name */
	char		*link;		/* link register name */
	char		*sbreg;		/* static base register name */
	u64	sb;		/* static base register value */
	int		pgsize;		/* page size */
	u64	kbase;		/* kernel base address */
	u64	ktmask;		/* ktzero = kbase & ~ktmask */
	u64	utop;		/* user stack top */
	int		pcquant;	/* quantization of pc */
	int		szaddr;		/* sizeof(void*) */
	int		szreg;		/* sizeof(register) */
	int		szfloat;	/* sizeof(float) */
	int		szdouble;	/* sizeof(double) */
};

extern	Mach	*mach;			/* Current machine */

typedef u64	(*Rgetter)(Map*, char*);
typedef	void		(*Tracer)(Map*, u64, u64, Symbol*);

struct	Machdata {		/* Machine-dependent debugger support */
	u8	bpinst[4];			/* break point instr. */
	i16	bpsize;				/* size of break point instr. */

	u16	(*swab)(u16);		/* ushort to local byte order */
	u32	(*swal)(u32);		/* ulong to local byte order */
	u64	(*swav)(u64);		/* uvlong to local byte order */
	int		(*ctrace)(Map*, u64, u64, u64, Tracer); /* C traceback */
	u64	(*findframe)(Map*, u64, u64, u64,
				     u64);		/* frame finder */
	char*		(*excep)(Map*, Rgetter);	/* last exception */
	u32	(*bpfix)(u64);		/* breakpoint fixup */
	int		(*sftos)(char*, int, void*);	/* single precision float */
	int		(*dftos)(char*, int, void*);	/* double precision float */
	int		(*foll)(Map*, u64, Rgetter, u64*);	/* follow set */
	int		(*das)(Map*, u64, char, char*, int);	/* symbolic disassembly */
	int		(*hexinst)(Map*, u64, char*, int); 	/* hex disassembly */
	int		(*instsize)(Map*, u64);	/* instruction size */
};

/*
 *	Common executable header describing all architectures
 */
typedef struct Fhdr
{
	char		*name;		/* identifier of executable */
	u8		type;		/* file type - see codes above */
	u8		hdrsz;		/* header size */
	u8		spare;
	i32		magic;		/* magic number */
	u64	txtaddr;	/* text address */
	i64		txtoff;		/* start of text in file */
	u64	dataddr;	/* start of data segment */
	i64		datoff;		/* offset to data seg in file */
	i64		symoff;		/* offset of symbol table in file */
	u64	entry;		/* entry point */
	i64		sppcoff;	/* offset of sp-pc table in file */
	i64		lnpcoff;	/* offset of line number-pc table in file */
	i32		txtsz;		/* text size */
	i32		datsz;		/* size of data seg */
	i32		bsssz;		/* size of bss */
	i32		symsz;		/* size of symbol table */
	i32		sppcsz;		/* size of sp-pc table */
	i32		lnpcsz;		/* size of line number-pc table */

	// TODO work out which of the above are no longer useful
	// and which should be uint64
	i8		bigendian;	/* big endian or not */
	u64	stroff;		/* strtab offset in file */
	u64	strsz;		/* size of strtab seg */
	u16	bssidx;		/* index of bss section */	
	u16	dbglineidx;	/* index of debug_line section */
} Fhdr;

extern	int	asstype;	/* dissembler type - machdata.c */
extern	Machdata *machdata;	/* jump vector - machdata.c */

Map*		attachproc(int, int, int, Fhdr*);
int		beieee80ftos(char*, int, void*);
int		beieeesftos(char*, int, void*);
int		beieeedftos(char*, int, void*);
u16	beswab(u16);
u32	beswal(u32);
u64	beswav(u64);
u64	ciscframe(Map*, u64, u64, u64, u64);
int		cisctrace(Map*, u64, u64, u64, Tracer);
int		crackhdr(int fd, Fhdr*);
u64	file2pc(char*, i32);
int		fileelem(Sym**, u8 *, char*, int);
i32		fileline(char*, int, u64);
int		filesym(int, char*, int);
int		findlocal(Symbol*, char*, Symbol*);
int		findseg(Map*, char*);
int		findsym(u64, int, Symbol *);
int		fnbound(u64, u64*);
int		fpformat(Map*, Reglist*, char*, int, int);
int		get1(Map*, u64, u8*, int);
int		get2(Map*, u64, u16*);
int		get4(Map*, u64, u32*);
int		get8(Map*, u64, u64*);
int		geta(Map*, u64, u64*);
int		getauto(Symbol*, int, int, Symbol*);
Sym*		getsym(int);
int		globalsym(Symbol *, int);
char*		_hexify(char*, u32, int);
int		ieeesftos(char*, int, u32);
int		ieeedftos(char*, int, u32, u32);
int		isar(Biobuf*);
int		leieee80ftos(char*, int, void*);
int		leieeesftos(char*, int, void*);
int		leieeedftos(char*, int, void*);
u16	leswab(u16);
u32	leswal(u32);
u64	leswav(u64);
u64	line2addr(i32, u64, u64);
Map*		loadmap(Map*, int, Fhdr*);
int		localaddr(Map*, char*, char*, u64*, Rgetter);
int		localsym(Symbol*, int);
int		lookup(char*, char*, Symbol*);
void		machbytype(int);
int		machbyname(char*);
int		nextar(Biobuf*, int, char*);
Map*		newmap(Map*, int);
void		objtraverse(void(*)(Sym*, void*), void*);
int		objtype(Biobuf*, char**);
u64	pc2sp(u64);
i32		pc2line(u64);
int		put1(Map*, u64, u8*, int);
int		put2(Map*, u64, u16);
int		put4(Map*, u64, u32);
int		put8(Map*, u64, u64);
int		puta(Map*, u64, u64);
int		readar(Biobuf*, int, i64, int);
int		readobj(Biobuf*, int);
u64	riscframe(Map*, u64, u64, u64, u64);
int		risctrace(Map*, u64, u64, u64, Tracer);
int		setmap(Map*, int, u64, u64, i64, char*);
Sym*		symbase(i32*);
int		syminit(int, Fhdr*);
int		symoff(char*, int, u64, int);
void		textseg(u64, Fhdr*);
int		textsym(Symbol*, int);
void		unusemap(Map*, int);

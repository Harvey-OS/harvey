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
#include "a.out.h"
#pragma	src	"/sys/src/libmach"
#pragma	lib	"libmach.a"
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
		char	*name;		/* the segment name */
		int	fd;		/* file descriptor */
		int	inuse;		/* in use - not in use */
		int	cache;		/* should cache reads? */
		uint64_t	b;		/* base */
		uint64_t	e;		/* end */
		int64_t	f;		/* offset within file */
	} seg[1];			/* actually n of these */
};

/*
 *	Internal structure describing a symbol table entry
 */
struct Symbol {
	void 	*handle;		/* used internally - owning func */
	struct {
		char	*name;
		int64_t	value;		/* address or stack offset */
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
	int16_t	roffs;			/* offset in u-block */
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
	char	*name;
	int	mtype;			/* machine type code */
	Reglist *reglist;		/* register set */
	int32_t	regsize;		/* sizeof registers in bytes */
	int32_t	fpregsize;		/* sizeof fp registers in bytes */
	char	*pc;			/* pc name */
	char	*sp;			/* sp name */
	char	*link;			/* link register name */
	char	*sbreg;			/* static base register name */
	uint64_t	sb;			/* static base register value */
	int	pgsize;			/* page size */
	uint64_t	kbase;			/* kernel base address */
	uint64_t	ktmask;			/* ktzero = kbase & ~ktmask */
	uint64_t	utop;			/* user stack top */
	int	pcquant;		/* quantization of pc */
	int	szaddr;			/* sizeof(void*) */
	int	szreg;			/* sizeof(register) */
	int	szfloat;		/* sizeof(float) */
	int	szdouble;		/* sizeof(double) */
};

extern	Mach	*mach;			/* Current machine */

typedef uint64_t	(*Rgetter)(Map*, char*);
typedef	void	(*Tracer)(Map*, uint64_t, uint64_t, Symbol*);

struct	Machdata {		/* Machine-dependent debugger support */
	uint8_t	bpinst[4];			/* break point instr. */
	int16_t	bpsize;				/* size of break point instr. */

	uint16_t	(*swab)(uint16_t);		/* ushort to local byte order */
	uint32_t	(*swal)(uint32_t);			/* ulong to local byte order */
	uint64_t	(*swav)(uint64_t);		/* uvlong to local byte order */
	int	(*ctrace)(Map*, uint64_t, uint64_t, uint64_t, Tracer); /* C traceback */
	uint64_t	(*findframe)(Map*, uint64_t, uint64_t, uint64_t,
				     uint64_t);/* frame finder */
	char*	(*excep)(Map*, Rgetter);	/* last exception */
	uint32_t	(*bpfix)(uint64_t);		/* breakpoint fixup */
	int	(*sftos)(char*, int, void*);	/* single precision float */
	int	(*dftos)(char*, int, void*);	/* double precision float */
	int	(*foll)(Map*, uint64_t, Rgetter, uint64_t*);/* follow set */
	int	(*das)(Map*, uint64_t, char, char*, int);	/* symbolic disassembly */
	int	(*hexinst)(Map*, uint64_t, char*, int); 	/* hex disassembly */
	int	(*instsize)(Map*, uint64_t);	/* instruction size */
};

/*
 *	Common a.out header describing all architectures
 */
typedef struct Fhdr
{
	char	*name;		/* identifier of executable */
	uint8_t	type;		/* file type - see codes above */
	uint8_t	hdrsz;		/* header size */
	uint8_t	_magic;		/* _MAGIC() magic */
	uint8_t	spare;
	int32_t	magic;		/* magic number */
	uint64_t	txtaddr;	/* text address */
	int64_t	txtoff;		/* start of text in file */
	uint64_t	dataddr;	/* start of data segment */
	int64_t	datoff;		/* offset to data seg in file */
	int64_t	symoff;		/* offset of symbol table in file */
	uint64_t	entry;		/* entry point */
	int64_t	sppcoff;	/* offset of sp-pc table in file */
	int64_t	lnpcoff;	/* offset of line number-pc table in file */
	int32_t	txtsz;		/* text size */
	int32_t	datsz;		/* size of data seg */
	int32_t	bsssz;		/* size of bss */
	int32_t	symsz;		/* size of symbol table */
	int32_t	sppcsz;		/* size of sp-pc table */
	int32_t	lnpcsz;		/* size of line number-pc table */
} Fhdr;

extern	int	asstype;	/* dissembler type - machdata.c */
extern	Machdata *machdata;	/* jump vector - machdata.c */

Map*		attachproc(int, int, int, Fhdr*);
int		beieee80ftos(char*, int, void*);
int		beieeesftos(char*, int, void*);
int		beieeedftos(char*, int, void*);
uint16_t		beswab(uint16_t);
uint32_t		beswal(uint32_t);
uint64_t		beswav(uint64_t);
uint64_t		ciscframe(Map*, uint64_t, uint64_t, uint64_t,
				  uint64_t);
int		cisctrace(Map*, uint64_t, uint64_t, uint64_t, Tracer);
int		crackhdr(int fd, Fhdr*);
uint64_t		file2pc(char*, int32_t);
int		fileelem(Sym**, uint8_t *, char*, int);
int32_t		fileline(char*, int, uint64_t);
int		filesym(int, char*, int);
int		findlocal(Symbol*, char*, Symbol*);
int		findseg(Map*, char*);
int		findsym(uint64_t, int, Symbol *);
int		fnbound(uint64_t, uint64_t*);
int		fpformat(Map*, Reglist*, char*, int, int);
int		get1(Map*, uint64_t, uint8_t*, int);
int		get2(Map*, uint64_t, uint16_t*);
int		get4(Map*, uint64_t, uint32_t*);
int		get8(Map*, uint64_t, uint64_t*);
int		geta(Map*, uint64_t, uint64_t*);
int		getauto(Symbol*, int, int, Symbol*);
Sym*		getsym(int);
int		globalsym(Symbol *, int);
char*		_hexify(char*, uint32_t, int);
int		ieeesftos(char*, int, uint32_t);
int		ieeedftos(char*, int, uint32_t, uint32_t);
int		isar(Biobuf*);
int		leieee80ftos(char*, int, void*);
int		leieeesftos(char*, int, void*);
int		leieeedftos(char*, int, void*);
uint16_t		leswab(uint16_t);
uint32_t		leswal(uint32_t);
uint64_t		leswav(uint64_t);
uint64_t		line2addr(int32_t, uint64_t, uint64_t);
Map*		loadmap(Map*, int, Fhdr*);
int		localaddr(Map*, char*, char*, uint64_t*, Rgetter);
int		localsym(Symbol*, int);
int		lookup(char*, char*, Symbol*);
void		machbytype(int);
int		machbyname(char*);
int		nextar(Biobuf*, int, char*);
Map*		newmap(Map*, int);
void		objtraverse(void(*)(Sym*, void*), void*);
int		objtype(Biobuf*, char**);
uint64_t		pc2sp(uint64_t);
int32_t		pc2line(uint64_t);
int		put1(Map*, uint64_t, uint8_t*, int);
int		put2(Map*, uint64_t, uint16_t);
int		put4(Map*, uint64_t, uint32_t);
int		put8(Map*, uint64_t, uint64_t);
int		puta(Map*, uint64_t, uint64_t);
int		readar(Biobuf*, int, int64_t, int);
int		readobj(Biobuf*, int);
uint64_t		riscframe(Map*, uint64_t, uint64_t, uint64_t,
				  uint64_t);
int		risctrace(Map*, uint64_t, uint64_t, uint64_t, Tracer);
int		setmap(Map*, int, uint64_t, uint64_t, int64_t, char*);
Sym*		symbase(int32_t*);
int		syminit(int, Fhdr*);
int		symoff(char*, int, uint64_t, int);
void		textseg(uint64_t, Fhdr*);
int		textsym(Symbol*, int);
void		unusemap(Map*, int);

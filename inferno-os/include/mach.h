/*
 *	Architecture-dependent application data
 */
#include "a.out.h"
#pragma src "/usr/inferno/utils/libmach"
#pragma	lib	"libmach.a"
/*
 *	Supported architectures:
 *		mips,
 *		68020,
 *		i386,
 *		amd64,
 *		sparc,
 *		sparc64,
 *		mips2 (R4000)
 *		arm
 *		powerpc,
 *		powerpc64
 *		alpha
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
	MALPHA,
	NMIPS,
	MSPARC64,
	MAMD64,
	MPOWER64,
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
	FALPHA,			/* 7.out */
	FALPHAB,		/* DEC Alpha bootable */
	FMIPSLE,		/* 3k little endian */
	FSPARC64,		/* u.out */
	FAMD64,			/* 6.out */
	FAMD64B,		/* 6.out bootable */
	FPOWER64,		/* 9.out */
	FPOWER64B,		/* 9.out bootable */

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
	AALPHA,
	ASPARC64,
	AAMD64,
	APOWER64,
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
	ObjAlpha,		/* .7 */
	ObjSparc64,		/* .u */
	ObjAmd64,		/* .6 */
	ObjSpim,		/* .0 */
	ObjPower64,	/* .9 */
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

typedef struct	segment	segment;

typedef	int	(*Rsegio)(segment*, ulong, long, char*, int);

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
		uvlong	b;		/* base */
		uvlong	e;		/* end */
		vlong	f;		/* offset within file */
		Rsegio	mget;	/* special get if not nil */
		Rsegio	mput;	/* special put if not nil */
	} seg[1];			/* actually n of these */
};

/*
 *	Internal structure describing a symbol table entry
 */
struct Symbol {
	void 	*handle;		/* used internally - owning func */
	struct {
		char	*name;
		vlong	value;		/* address or stack offset */
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
	short	roffs;			/* offset in u-block */
	char	rflags;			/* INTEGER/FLOAT, WRITABLE */
	char	rformat;		/* print format: 'x', 'X', 'f', '8', '3', 'Y', 'W' */
};

enum {					/* bits in rflags field */
	RINT	= (0<<0),
	RFLT	= (1<<0),
	RRDONLY	= (1<<1)
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
	ulong	regsize;		/* sizeof registers in bytes */
	ulong	fpregsize;		/* sizeof fp registers in bytes */
	char	*pc;			/* pc name */
	char	*sp;			/* sp name */
	char	*link;			/* link register name */
	char	*sbreg;			/* static base register name */
	uvlong	sb;			/* static base register value */
	int	pgsize;			/* page size */
	uvlong	kbase;			/* kernel base address */
	uvlong	ktmask;			/* ktzero = kbase & ~ktmask */
	uvlong	utop;			/* user stack top */
	int	pcquant;		/* quantization of pc */
	int	szaddr;			/* sizeof(void*) */
	int	szreg;			/* sizeof(register) */
	int	szfloat;		/* sizeof(float) */
	int	szdouble;		/* sizeof(double) */
};

extern	Mach	*mach;			/* Current machine */

typedef uvlong	(*Rgetter)(Map*, char*);
typedef	void	(*Tracer)(Map*, uvlong, uvlong, Symbol*);

struct	Machdata {		/* Machine-dependent debugger support */
	uchar	bpinst[4];			/* break point instr. */
	short	bpsize;				/* size of break point instr. */

	ushort	(*swab)(ushort);		/* ushort to local byte order */
	ulong	(*swal)(ulong);			/* ulong to local byte order */
	uvlong	(*swav)(uvlong);		/* uvlong to local byte order */
	int	(*ctrace)(Map*, uvlong, uvlong, uvlong, Tracer); /* C traceback */
	uvlong	(*findframe)(Map*, uvlong, uvlong, uvlong, uvlong);/* frame finder */
	char*	(*excep)(Map*, Rgetter);	/* last exception */
	ulong	(*bpfix)(uvlong);		/* breakpoint fixup */
	int	(*sftos)(char*, int, void*);	/* single precision float */
	int	(*dftos)(char*, int, void*);	/* double precision float */
	int	(*foll)(Map*, uvlong, Rgetter, uvlong*);/* follow set */
	int	(*das)(Map*, uvlong, char, char*, int);	/* symbolic disassembly */
	int	(*hexinst)(Map*, uvlong, char*, int); 	/* hex disassembly */
	int	(*instsize)(Map*, uvlong);	/* instruction size */
};

/*
 *	Common a.out header describing all architectures
 */
typedef struct Fhdr
{
	char	*name;		/* identifier of executable */
	uchar	type;		/* file type - see codes above */
	uchar	hdrsz;		/* header size */
	uchar	_magic;		/* _MAGIC() magic */
	uchar	spare;
	long	magic;		/* magic number */
	uvlong	txtaddr;	/* text address */
	vlong	txtoff;		/* start of text in file */
	uvlong	dataddr;	/* start of data segment */
	vlong	datoff;		/* offset to data seg in file */
	vlong	symoff;		/* offset of symbol table in file */
	uvlong	entry;		/* entry point */
	vlong	sppcoff;	/* offset of sp-pc table in file */
	vlong	lnpcoff;	/* offset of line number-pc table in file */
	long	txtsz;		/* text size */
	long	datsz;		/* size of data seg */
	long	bsssz;		/* size of bss */
	long	symsz;		/* size of symbol table */
	long	sppcsz;		/* size of sp-pc table */
	long	lnpcsz;		/* size of line number-pc table */
} Fhdr;

extern	int	asstype;	/* dissembler type - machdata.c */
extern	Machdata *machdata;	/* jump vector - machdata.c */

Map*		attachproc(int, int, int, Fhdr*);
Map*		attachremt(int, Fhdr*);
int		beieee80ftos(char*, int, void*);
int		beieeesftos(char*, int, void*);
int		beieeedftos(char*, int, void*);
ushort		beswab(ushort);
ulong		beswal(ulong);
uvlong		beswav(uvlong);
uvlong		ciscframe(Map*, uvlong, uvlong, uvlong, uvlong);
int		cisctrace(Map*, uvlong, uvlong, uvlong, Tracer);
int		crackhdr(int fd, Fhdr*);
uvlong		file2pc(char*, long);
int		fileelem(Sym**, uchar *, char*, int);
long		fileline(char*, int, uvlong);
int		filesym(int, char*, int);
int		findlocal(Symbol*, char*, Symbol*);
int		findseg(Map*, char*);
int		findsym(uvlong, int, Symbol *);
int		fnbound(uvlong, uvlong*);
int		fpformat(Map*, Reglist*, char*, int, int);
int		get1(Map*, uvlong, uchar*, int);
int		get2(Map*, uvlong, ushort*);
int		get4(Map*, uvlong, ulong*);
int		get8(Map*, uvlong, uvlong*);
int		geta(Map*, uvlong, uvlong*);
int		getauto(Symbol*, int, int, Symbol*);
Sym*		getsym(int);
int		globalsym(Symbol *, int);
char*		_hexify(char*, ulong, int);
int		ieeesftos(char*, int, ulong);
int		ieeedftos(char*, int, ulong, ulong);
int		isar(Biobuf*);
int		leieee80ftos(char*, int, void*);
int		leieeesftos(char*, int, void*);
int		leieeedftos(char*, int, void*);
ushort		leswab(ushort);
ulong		leswal(ulong);
uvlong		leswav(uvlong);
uvlong		line2addr(long, uvlong, uvlong);
Map*		loadmap(Map*, int, Fhdr*);
int		localaddr(Map*, char*, char*, uvlong*, Rgetter);
int		localsym(Symbol*, int);
int		lookup(char*, char*, Symbol*);
void		machbytype(int);
int		machbyname(char*);
int		nextar(Biobuf*, int, char*);
Map*		newmap(Map*, int);
void		objtraverse(void(*)(Sym*, void*), void*);
int		objtype(Biobuf*, char**);
uvlong		pc2sp(uvlong);
long		pc2line(uvlong);
int		put1(Map*, uvlong, uchar*, int);
int		put2(Map*, uvlong, ushort);
int		put4(Map*, uvlong, ulong);
int		put8(Map*, uvlong, uvlong);
int		puta(Map*, uvlong, uvlong);
int		readar(Biobuf*, int, vlong, int);
int		readobj(Biobuf*, int);
uvlong		riscframe(Map*, uvlong, uvlong, uvlong, uvlong);
int		risctrace(Map*, uvlong, uvlong, uvlong, Tracer);
int		setmap(Map*, int, uvlong, uvlong, vlong, char*);
void		setmapio(Map*, int, Rsegio, Rsegio);
Sym*		symbase(long*);
int		syminit(int, Fhdr*);
int		symoff(char*, int, uvlong, int);
void		textseg(uvlong, Fhdr*);
int		textsym(Symbol*, int);
void	thumbpctab(Biobuf*, Fhdr*);
int	thumbpclookup(uvlong);
void		unusemap(Map*, int);

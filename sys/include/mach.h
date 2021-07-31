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
 *		sparc,
 *		i960 (limited)
 *		3210DSP (limited)
 *		mips2 (R4000)
 *		arm (limited)
 *		power pc (limited)
*/
enum
{
	MMIPS,			/* machine types */
	MSPARC,
	M68020,
	MI386,
	MI960,
	M3210,
	MMIPS2,
	NMIPS2,
	M29000,
	MARM,
	MPOWER,
	MALPHA,
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
	FI960,			/* 6.out */
	FI960B,			/* I960 bootable */
	F3210,			/* x.out */
	FMIPS2BE,			/* 4.out */
	F29000,			/* 9.out */
	FARM,			/* 5.out */
	FARMB,			/* ARM bootable */
	FPOWER,			/* q.out */
	FPOWERB,		/* power pc bootable */
	FMIPS2LE,		/* 4k little endian */
	FALPHA,		/* 7.out */
	FALPHAB,		/* DEC Alpha bootable */

	ANONE = 0,		/* dissembler types */
	AMIPS,
	AMIPSCO,		/* native mips */
	ASPARC,
	ASUNSPARC,		/* native sun */
	A68020,
	AI386,
	AI8086,			/* oh god */
	AI960,
	A29000,
	AARM,
	APOWER,
	AALPHA,
				/* object file types */
	Obj68020 = 0,		/* .2 */
	ObjSparc,		/* .k */
	ObjMips,		/* .v */
	Obj386,			/* .8 */
	Obj960,			/* .6 */
	Obj3210,		/* .x */
	ObjMips2,		/* .4 */
	Obj29000,		/* .9 */
	ObjArm,			/* .5 */
	ObjPower,		/* .q */
	ObjMips2le,		/* .0 */
	ObjAlpha,		/* .7 */
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
		ulong	b;		/* base */
		ulong	e;		/* end */
		ulong	f;		/* offset within file */
	} seg[1];			/* actually n of these */
};

/*
 *	Internal structure describing a symbol table entry
 */
struct Symbol {
	void 	*handle;		/* used internally - owning func */
	struct {
		char	*name;
		long	value;		/* address or stack offset */
		char	type;		/* as in a.out.h */
		char	class;		/* as above */
	};
};

/*
 *	machine register description
 */
struct Reglist {
	char	*rname;			/* register name */
	short	roffs;			/* offset in u-block */
	char	rflags;			/* INTEGER/FLOAT, WRITABLE */
	char	rformat;		/* print format: 'x', 'X', 'f', '8' */
};

enum {				/* bits in rflags field */
	RINT	= (0<<0),
	RFLT	= (1<<0),
	RRDONLY	= (1<<1),
};
/*
 *	Machine-dependent data is stored in two structures:
 *		Mach  - miscellaneous general parameters
 *		Machdata - jump vector of service functions used by debuggers
 *
 *	Mach is defined in 2.c, 4.c, v.c, k.c, 8.c, 6.c and set in executable.c
 *
 *	Machdata is defined in 2db.c, 4db.c, vdb.c, kdb.c, 8db.c, and 6db.c
 *		and set in the debugger startup.
 */


struct Mach{
	char	*name;
	int	mtype;			/* machine type code */
	Reglist *reglist;		/* register set */
	ulong	regsize;		/* sizeof registers in bytes*/
	ulong	fpregsize;		/* sizeof fp registers in bytes*/
	char	*pc;			/* pc name */
	char	*sp;			/* sp name */
	char	*link;			/* link register name */
	char	*sbreg;			/* static base register name */
	ulong	sb;			/* static base register value */
	int	pgsize;			/* page size */
	ulong	kbase;			/* kernel base address */
	ulong	ktmask;			/* ktzero = kbase & ~ktmask */
	int	pcquant;		/* quantization of pc */
	int	szaddr;			/* sizeof(void*) */
	int	szreg;			/* sizeof(register) */
	int	szfloat;		/* sizeof(float) */
	int	szdouble;		/* sizeof(double) */
};

extern	Mach	*mach;			/* Current machine */

typedef vlong	(*Rgetter)(Map*, char*);
typedef	void	(*Tracer)(Map*, ulong, ulong, Symbol*);

struct	Machdata {		/* Machine-dependent debugger support */
	uchar	bpinst[4];			/* break point instr. */
	short	bpsize;				/* size of break point instr. */

	ushort	(*swab)(ushort);		/* short to local byte order */
	long	(*swal)(long);			/* long to local byte order */
	vlong	(*swav)(vlong);			/* vlong to local byte order */
	int	(*ctrace)(Map*, ulong, ulong, ulong, Tracer); /* C traceback */
	ulong	(*findframe)(Map*, ulong, ulong, ulong, ulong);/* frame finder */
	char*	(*excep)(Map*, Rgetter);	/* last exception */
	ulong	(*bpfix)(ulong);		/* breakpoint fixup */
	int	(*sftos)(char*, int, void*);	/* single precision float */
	int	(*dftos)(char*, int, void*);	/* double precision float */
	int	(*foll)(Map*, ulong, Rgetter, ulong*);	/* follow set */
	int	(*das)(Map*, ulong, char, char*, int);	/* symbolic disassembly */
	int	(*hexinst)(Map*, ulong, char*, int); 	/* hex disassembly */
	int	(*instsize)(Map*, ulong);	/* instruction size */
};

/*
 *	Common a.out header describing all architectures
 */
typedef struct Fhdr
{
	char	*name;		/* identifier of executable */
	short	type;		/* file type - see codes above*/
	short	hdrsz;		/* size of this header */
	long	magic;		/* magic number */
	long	txtaddr;	/* text address */
	long	entry;		/* entry point */
	long	txtsz;		/* text size */
	long	txtoff;		/* start of text in file */
	long	dataddr;	/* start of data segment */
	long	datsz;		/* size of data seg */
	long	datoff;		/* offset to data seg in file */
	long	bsssz;		/* size of bss */
	long	symsz;		/* size of symbol table */
	long	symoff;		/* offset of symbol table in file */
	long	sppcsz;		/* size of sp-pc table */
	long	sppcoff;	/* offset of sp-pc table in file */
	long	lnpcsz;		/* size of line number-pc table */
	long	lnpcoff;	/* size of line number-pc table */
} Fhdr;

extern	int	asstype;		/* dissembler type - machdata.c */
extern	Machdata *machdata;		/* jump vector - machdata.c */

Map*		attachproc(int, int, int, Fhdr*);
int		beieee80ftos(char*, int, void*);
int		beieeesftos(char*, int, void*);
int		beieeedftos(char*, int, void*);
ushort		beswab(ushort);
long		beswal(long);
vlong		beswav(vlong);
int		cisctrace(Map*, ulong, ulong, ulong, Tracer);
ulong		ciscframe(Map*, ulong, ulong, ulong, ulong);
int		crackhdr(int fd, Fhdr*);
long		file2pc(char*, ulong);
int		fileelem(Sym**, uchar *, char*, int);
int		fileline(char*, int, ulong);
int		filesym(int, char*, int);
int		findlocal(Symbol*, char*, Symbol*);
int		findseg(Map*, char*);
int		findsym(long, int, Symbol *);
int		fnbound(long, ulong*);
int		fpformat(Map*, Reglist*, char*, int, int);
int		get1(Map*, ulong, uchar*, int);
int		get2(Map*, ulong, ushort*);
int		get4(Map*, ulong, long*);
int		get8(Map*, ulong, vlong*);
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
long		leswal(long);
vlong		leswav(vlong);
long		line2addr(ulong, ulong, ulong);
Map*		loadmap(Map*, int, Fhdr*);
int		localaddr(Map*, char*, char*, long*, Rgetter);
int		localsym(Symbol*, int);
int		lookup(char*, char*, Symbol*);
void		machbytype(int);
int		machbyname(char*);
int		nextar(Biobuf*, int, char*);
Map*		newmap(Map*, int);
void		objtraverse(void(*)(Sym*, void*), void*);
int		objtype(Biobuf*, char**);
long		pc2sp(ulong);
long		pc2line(ulong);
int		put1(Map*, ulong, uchar*, int);
int		put2(Map*, ulong, ushort);
int		put4(Map*, ulong, long);
int		put8(Map*, ulong, vlong);
int		readar(Biobuf*, int, int, int);
int		readobj(Biobuf*, int);
ulong		riscframe(Map*, ulong, ulong, ulong, ulong);
int		risctrace(Map*, ulong, ulong, ulong, Tracer);
int		setmap(Map*, int, ulong, ulong, ulong, char*);
Sym*		symbase(long*);
int		syminit(int, Fhdr*);
int		symoff(char*, int, long, int);
void		textseg(ulong, Fhdr*);
int		textsym(Symbol*, int);
void		unusemap(Map*, int);

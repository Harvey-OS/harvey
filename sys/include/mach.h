/*
 *	Per-machine type data
 */
#include "a.out.h"
#pragma	lib	"libmach.a"
/*
 *	Supported architectures:
 *		mips,
 *		68020,
 *		i386,
 *		sparc,
 *		hobbit,
 *		i960 (limited)
 */
enum
{
	MMIPS,			/* machine types */
	MSPARC,
	M68020,
	MNEXT,
	MI386,
	MI960,
	MHOBBIT,
				/* types of exectables */
	FNONE,			/* unidentified */
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
	FHOBBIT,		/* z.out */
	FHOBBITB,		/* Hobbit bootable */

	ANONE,			/* dissembler types */
	AMIPS,
	AMIPSCO,		/* native mips */
	ASPARC,
	ASUNSPARC,		/* native sun */
	A68020,
	AI386,
	AI8086,			/* oh god */
	AI960,
	AHOBBIT,
				/* object file types */
	Obj68020   = 0,		/* .2 */
	ObjSparc   = 1,		/* .k */
	ObjMips	   = 2,		/* .v */
	ObjHobbit  = 3,		/* .z */
	Obj386	   = 4,		/* .8 */
	Obj960	   = 5,		/* .6 */
	Maxobjtype = 6,

	SEGNONE = -2,		/* file map segment names */
	SEGANY	= -1,
	SEGTEXT	= 0,		/* index into seg array of map */
	SEGDATA	= 1,
	SEGUBLK	= 2,
	SEGREGS = 3,
	MAXSEGS = 4,

	CNONE  = 0,		/* symbol table classes */
	CAUTO  = 1,
	CPARAM = 2,
	CSTAB  = 3,
	CTEXT  = 4,
	CDATA  = 5,
	CANY   = 6,		/* to look for any class */
};
/*
 * 	Structure to map a segment to a position in a file
 */
typedef	struct map Map;

struct map {
	int	fd;			/* file descriptor associated with map */
	struct {			/* per-segment map */
		char	*name;		/* the segment name */
		int	inuse;		/* in use - not in use */
		ulong	b;		/* base */
		ulong	e;		/* end */
		ulong	f;		/* offset within file */
	} seg[MAXSEGS];
};
/*
 *	machine register description
 */
typedef	struct reglist Reglist;

struct reglist {
	char	*rname;			/* register name */
	short	roffs;			/* offset in u-block */
	char	rfloat;			/* 0=>integer; 1=>float */
	char	rformat;		/* print format: 'x', 'X', 'f', '8' */
	ulong	rval;			/* current register value */
};
/*
 *	data describing generic machine characteristics
 *	defined in 2.c, v.c, k.c, 8.c, 6.c, z.c in /sys/src/libmach
 *	set in executable.c
 */
typedef	struct	Mach Mach;
struct Mach{
	char	*name;
	Reglist *reglist;		/* register set */
	int	minreg;			/* minimum register */
	int	maxreg;			/* maximum register */
	ulong	pc;			/* pc offset */
	ulong	sp;			/* sp offset */
	ulong	link;			/* link register offset */
	ulong	retreg;			/* function return register */
	int	firstwr;		/* first writable register */
	int	pgsize;			/* page size */
	ulong	kbase;			/* kernel base address: uarea */
	ulong	ktmask;			/* ktzero = kbase & ~ktmask */
	ulong	kspoff;			/* offset of ksp in /proc/proc */
	ulong	kspdelta;		/* correction to ksp value */
	ulong	kpcoff;			/* offset of kpc in /proc/proc */
	ulong	kpcdelta;		/* correction to kpc value */
	ulong	scalloff;		/* offset to sys call # in ublk */
	int	pcquant;		/* quantization of pc */
	char	*sbreg;			/* static base register name */
	ulong	sb;			/* value */
	int	szaddr;			/* sizeof(void*) */
	int	szreg;			/* sizeof(register) */
	int	szfloat;		/* sizeof(float) */
	int	szdouble;		/* sizeof(double) */
};

extern	Mach	*mach;			/* Current machine */

/*
 *	Per-machine debugger support
 *
 */
typedef	struct	machdata Machdata;

struct	machdata {
	uchar	bpinst[4];		/* break point instr. */
	short	bpsize;			/* size of break point instr. */

	void	(*init)(void);		/* init routine */
	ushort	(*swab)(ushort);	/* convert short to local byte order */
	long	(*swal)(long);		/* convert long to local byte order */
	void	(*ctrace)(int);		/* print C traceback */
	ulong	(*findframe)(ulong);	/* frame finder */
	void	(*fpprint)(int);	/* print fp regs */
	void	(*rsnarf)(Reglist *);	/* grab registers */
	void	(*rrest)(Reglist *);	/* restore registers */
	void	(*excep)(void);		/* print exception */
	ulong	(*bpfix)(ulong);	/* breakpoint fixup */
	void	(*sfpout)(char*);	/* format single precision number */
	void	(*dfpout)(char*);	/* format double precision number */
	int	(*foll)(ulong, ulong*);	/* following instructions */
	void	(*printins)(Map*, char, int);	/* print an instruction */
	void	(*printdas)(Map*, int);	/* dissembler instruction */
};

extern	Machdata 	*machdata;	/* in machine.c */
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
/*
 *	Internal structure describing a symbol table entry
 */
typedef struct symbol Symbol;

struct symbol {
	void 	*handle;		/* used internally - owning func */
	struct {
		char	*name;
		long	value;		/* address or stack offset */
		char	type;		/* as in a.out.h */
		char	class;		/* as above */
	};
};

extern	int	asstype;		/* dissembler type - machine.c */
extern	char	*symerror;		/* symbol table error message */

ushort		beswab(ushort);
long		beswal(long);
int		crackhdr(int fd, Fhdr *fp);
long		file2pc(char *, ulong);
int		fileelem(Sym **, uchar *, char *, int);
int		fileline(char *, int, ulong);
int		filesym(int, char*, int);
int		findlocal(Symbol *, char *, Symbol *);
int		findsym(long, int, Symbol *);
int		getauto(Symbol *, int, int, Symbol *);
Sym*		getsym(int);
int		globalsym(Symbol *, int);
int		isar(Biobuf*);
ushort		leswab(ushort);
long		leswal(long);
long		line2addr(ulong, ulong, ulong);
Map*		loadmap(Map *, int, Fhdr *);
int		localsym(Symbol *, int);
int		lookup(char *, char *, Symbol *);
void		machbytype(int);
int		machbyname(char *);
int		mget(Map *, int, ulong, char*, int);
int		mput(Map *, int, ulong, char*, int);
int		nextar(Biobuf*, int, char*);
Map*		newmap(Map *, int);
Sym*		objbase(long*);
void		objreset(void);
Sym*		objsym(int);
int		objtype(Biobuf*);
long		pc2sp(ulong);
long		pc2line(ulong);
long		_round(long, long);
int		readar(Biobuf*, int, int);
int		readobj(Biobuf*, int);
int		setmap(Map *, int, ulong, ulong, ulong);
Sym*		symbase(long *);
int		syminit(int, Fhdr *);
int		textsym(Symbol*, int);
void		unusemap(Map *, int);

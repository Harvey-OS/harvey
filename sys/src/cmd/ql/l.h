#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"../qc/q.out.h"
#include	"../8l/elf.h"

#ifndef	EXTERN
#define	EXTERN	extern
#endif

#define	LIBNAMELEN	300

typedef	struct	Adr	Adr;
typedef	struct	Sym	Sym;
typedef	struct	Autom	Auto;
typedef	struct	Prog	Prog;
typedef	struct	Optab	Optab;

#define	P		((Prog*)0)
#define	S		((Sym*)0)
#define	TNAME		(curtext&&curtext->from.sym?curtext->from.sym->name:noname)

struct	Adr
{
	union
	{
		long	u0offset;
		char	u0sval[NSNAME];
		Ieee	u0ieee;
	}u0;
	Sym	*sym;
	Auto	*autom;
	char	type;
	uchar	reg;
	char	name;
	char	class;
};

#define	offset	u0.u0offset
#define	sval	u0.u0sval
#define	ieee	u0.u0ieee

struct	Prog
{
	Adr	from;
	Adr	from3;	/* fma and rlwm */
	Adr	to;
	Prog	*forwd;
	Prog	*cond;
	Prog	*link;
	long	pc;
	long	regused;
	short	line;
	short	mark;
	short	optab;		/* could be uchar */
	ushort	as;
	char	reg;
};
struct	Sym
{
	char	*name;
	short	type;
	short	version;
	short	become;
	short	frame;
	uchar	subtype;
	ushort	file;
	long	value;
	long	sig;
	Sym	*link;
};
struct	Autom
{
	Sym	*sym;
	Auto	*link;
	long	aoffset;
	short	type;
};
struct	Optab
{
	ushort	as;
	char	a1;
	char	a2;
	char	a3;
	char	a4;
	char	type;
	char	size;
	char	param;
};
struct
{
	Optab*	start;
	Optab*	stop;
} oprange[ALAST];

enum
{
	FPCHIP		= 1,
	BIG		= 32768-8,
	STRINGSZ	= 200,
	MAXIO		= 8192,
	MAXHIST		= 20,				/* limit of path elements for history symbols */
	DATBLK		= 1024,
	NHASH		= 10007,
	NHUNK		= 100000,
	MINSIZ		= 64,
	NENT		= 100,
	NSCHED		= 20,

/* mark flags */
	LABEL		= 1<<0,
	LEAF		= 1<<1,
	FLOAT		= 1<<2,
	BRANCH		= 1<<3,
	LOAD		= 1<<4,
	FCMP		= 1<<5,
	SYNC		= 1<<6,
	LIST		= 1<<7,
	FOLL		= 1<<8,
	NOSCHED		= 1<<9,

	STEXT		= 1,
	SDATA,
	SBSS,
	SDATA1,
	SXREF,
	SLEAF,
	SFILE,
	SCONST,
	SUNDEF,

	SIMPORT,
	SEXPORT,

	C_NONE		= 0,
	C_REG,
	C_FREG,
	C_CREG,
	C_SPR,		/* special processor register */
	C_SREG,		/* segment register (32 bit implementations only) */
	C_ZCON,
	C_SCON,		/* 16 bit signed */
	C_UCON,		/* low 16 bits 0 */
	C_ADDCON,	/* -0x8000 <= v < 0 */
	C_ANDCON,	/* 0 < v <= 0xFFFF */
	C_LCON,		/* other */
	C_SACON,
	C_SECON,
	C_LACON,
	C_LECON,
	C_SBRA,
	C_LBRA,
	C_SAUTO,
	C_LAUTO,
	C_SEXT,
	C_LEXT,
	C_ZOREG,
	C_SOREG,
	C_LOREG,
	C_FPSCR,
	C_MSR,
	C_XER,
	C_LR,
	C_CTR,
	C_ANY,
	C_GOK,
	C_ADDR,

	C_NCLASS,

	Roffset	= 22,		/* no. bits for offset in relocation address */
	Rindex	= 10		/* no. bits for index in relocation address */
};

EXTERN union
{
	struct
	{
		uchar	obuf[MAXIO];			/* output buffer */
		uchar	ibuf[MAXIO];			/* input buffer */
	} u;
	char	dbuf[1];
} buf;

#define	cbuf	u.obuf
#define	xbuf	u.ibuf

EXTERN	long	HEADR;			/* length of header */
EXTERN	int	HEADTYPE;		/* type of header */
EXTERN	long	INITDAT;		/* data location */
EXTERN	long	INITRND;		/* data round above text location */
EXTERN	long	INITTEXT;		/* text location */
EXTERN	long	INITTEXTP;		/* text location (physical) */
EXTERN	char*	INITENTRY;		/* entry point */
EXTERN	long	autosize;
EXTERN	Biobuf	bso;
EXTERN	long	bsssize;
EXTERN	int	cbc;
EXTERN	uchar*	cbp;
EXTERN	int	cout;
EXTERN	Auto*	curauto;
EXTERN	Auto*	curhist;
EXTERN	Prog*	curp;
EXTERN	Prog*	curtext;
EXTERN	Prog*	datap;
EXTERN	Prog*	prog_movsw;
EXTERN	Prog*	prog_movdw;
EXTERN	Prog*	prog_movws;
EXTERN	Prog*	prog_movwd;
EXTERN	long	datsize;
EXTERN	char	debug[128];
EXTERN	Prog*	firstp;
EXTERN	char	fnuxi8[8];
EXTERN	Sym*	hash[NHASH];
EXTERN	Sym*	histfrog[MAXHIST];
EXTERN	int	histfrogp;
EXTERN	int	histgen;
EXTERN	char*	library[50];
EXTERN	char*	libraryobj[50];
EXTERN	int	libraryp;
EXTERN	int	xrefresolv;
EXTERN	char*	hunk;
EXTERN	char	inuxi1[1];
EXTERN	char	inuxi2[2];
EXTERN	char	inuxi4[4];
EXTERN	Prog*	lastp;
EXTERN	long	lcsize;
EXTERN	char	literal[32];
EXTERN	int	nerrors;
EXTERN	long	nhunk;
EXTERN	char*	noname;
EXTERN	long	instoffset;
EXTERN	char*	outfile;
EXTERN	long	pc;
EXTERN	int	r0iszero;
EXTERN	long	symsize;
EXTERN	long	staticgen;
EXTERN	Prog*	textp;
EXTERN	long	textsize;
EXTERN	long	tothunk;
EXTERN	char	xcmp[C_NCLASS][C_NCLASS];
EXTERN	int	version;
EXTERN	Prog	zprg;
EXTERN	int	dtype;

EXTERN	int	doexp, dlm;
EXTERN	int	imports, nimports;
EXTERN	int	exports, nexports;
EXTERN	char*	EXPTAB;
EXTERN	Prog	undefp;

#define	UP	(&undefp)

extern	Optab	optab[];
extern	char*	anames[];
extern	char*	cnames[];

int	Aconv(Fmt*);
int	Dconv(Fmt*);
int	Nconv(Fmt*);
int	Pconv(Fmt*);
int	Sconv(Fmt*);
int	Rconv(Fmt*);
int	aclass(Adr*);
void	addhist(long, int);
void	histtoauto(void);
void	addlibpath(char*);
void	addnop(Prog*);
void	append(Prog*, Prog*);
void	asmb(void);
void	asmdyn(void);
void	asmlc(void);
int	asmout(Prog*, Optab*, int);
void	asmsym(void);
long	atolwhex(char*);
Prog*	brloop(Prog*);
void	buildop(void);
void	cflush(void);
void	ckoff(Sym*, long);
int	cmp(int, int);
void	cput(long);
int	compound(Prog*);
double	cputime(void);
void	datblk(long, long);
void	diag(char*, ...);
void	dodata(void);
void	doprof1(void);
void	doprof2(void);
void	dynreloc(Sym*, long, int, int, int);
long	entryvalue(void);
void	errorexit(void);
void	exchange(Prog*);
void	export(void);
int	fileexists(char*);
int	find1(long, int);
char*	findlib(char*);
void	follow(void);
void	gethunk(void);
double	ieeedtod(Ieee*);
long	ieeedtof(Ieee*);
void	import(void);
int	isnop(Prog*);
void	ldobj(int, long, char*);
void	loadlib(void);
void	listinit(void);
void	initmuldiv(void);
Sym*	lookup(char*, int);
void	llput(vlong);
void	llputl(vlong);
void	lput(long);
void	lputl(long);
void	mkfwd(void);
void*	mysbrk(ulong);
void	names(void);
void	nocache(Prog*);
void	noops(void);
void	nopout(Prog*);
void	nuxiinit(void);
void	objfile(char*);
int	ocmp(void*, void*);
long	opcode(int);
Optab*	oplook(Prog*);
void	patch(void);
void	prasm(Prog*);
void	prepend(Prog*, Prog*);
Prog*	prg(void);
int	pseudo(Prog*);
void	putsymb(char*, int, long, int);
void	readundefs(char*, int);
long	regoff(Adr*);
int	relinv(int);
long	rnd(long, long);
void	sched(Prog*, Prog*);
void	span(void);
void	strnput(char*, int);
void	undef(void);
void	undefsym(Sym*);
void	wput(long);
void	wputl(long);
void	xdefine(char*, int, long);
void	xfol(Prog*);
void	zerosig(char*);

#pragma	varargck	type	"A"	int
#pragma	varargck	type	"A"	uint
#pragma	varargck	type	"D"	Adr*
#pragma	varargck	type	"N"	Adr*
#pragma	varargck	type	"P"	Prog*
#pragma	varargck	type	"R"	int
#pragma	varargck	type	"S"	char*

#pragma	varargck	argpos	diag 1

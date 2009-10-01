#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"../kc/k.out.h"

#ifndef	EXTERN
#define	EXTERN	extern
#endif

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
	} u0;
	union
	{
		Auto*	u1autom;
		Sym*	u1sym;
	} u1;
	char	type;
	char	reg;
	char	name;
	char	class;
};

#define	offset	u0.u0offset
#define	sval	u0.u0sval
#define	ieee	u0.u0ieee

#define	autom	u1.u1autom
#define	sym	u1.u1sym

struct	Prog
{
	Adr	from;
	Adr	to;
	Prog	*forwd;
	Prog	*cond;
	Prog	*link;
	long	pc;
	long	regused;
	short	line;
	short	mark;
	uchar	optab;
	uchar	as;
	char	reg;
};
struct	Sym
{
	char	*name;
	short	type;
	short	version;
	short	become;
	short	frame;
	long	value;
	Sym	*link;
};
struct	Autom
{
	Sym	*asym;
	Auto	*link;
	long	aoffset;
	short	type;
};
struct	Optab
{
	uchar	as;
	char	a1;
	char	a2;
	char	a3;
	char	type;
	char	size;
	char	param;
};
EXTERN	struct
{
	Optab*	start;
	Optab*	stop;
} oprange[AEND];

enum
{
	AXLD		= AEND+1,
	AXST,
	FPCHIP		= 1,
	BIG		= 4096-8,
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

	C_NONE		= 0,

	C_REG,
	C_FREG,
	C_CREG,
	C_PREG,
	C_FSR,
	C_FQ,

	C_ZCON,		/* 0 */
	C_SCON,		/* 13 bit signed */
	C_UCON,		/* low 10 bits 0 */
	C_LCON,		/* other */

	C_SACON,
	C_SECON,
	C_LACON,
	C_LECON,

	C_SBRA,
	C_LBRA,

	C_ESAUTO,
	C_OSAUTO,
	C_SAUTO,
	C_OLAUTO,
	C_ELAUTO,
	C_LAUTO,

	C_ESEXT,
	C_OSEXT,
	C_SEXT,
	C_ELEXT,
	C_OLEXT,
	C_LEXT,

	C_ZOREG,
	C_SOREG,
	C_LOREG,
	C_ASI,

	C_ANY,

	C_GOK,

	C_NCLASS
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
EXTERN	Prog*	prog_mul;
EXTERN	Prog*	prog_div;
EXTERN	Prog*	prog_divl;
EXTERN	Prog*	prog_mod;
EXTERN	Prog*	prog_modl;
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
EXTERN	long	symsize;
EXTERN	long	staticgen;
EXTERN	Prog*	textp;
EXTERN	long	textsize;
EXTERN	long	tothunk;
EXTERN	char	xcmp[C_NCLASS][C_NCLASS];
EXTERN	int	version;
EXTERN	Prog	zprg;
EXTERN	int	dtype;

extern	Optab	optab[];
extern	char*	anames[];

#pragma	varargck	type	"A"	int
#pragma	varargck	type	"A"	uint
#pragma	varargck	type	"D"	Adr*
#pragma	varargck	type	"N"	Adr*
#pragma	varargck	type	"P"	Prog*
#pragma	varargck	type	"S"	char*

#pragma	varargck	argpos	diag 1

int	Aconv(Fmt*);
int	Dconv(Fmt*);
int	Nconv(Fmt*);
int	Pconv(Fmt*);
int	Sconv(Fmt*);
int	aclass(Adr*);
void	addhist(long, int);
void	histtoauto(void);
void	addnop(Prog*);
void	append(Prog*, Prog*);
void	asmb(void);
void	asmlc(void);
int	asmout(Prog*, Optab*, int);
void	asmsym(void);
long	atolwhex(char*);
Prog*	brloop(Prog*);
void	buildop(void);
void	cflush(void);
int	cmp(int, int);
int	compound(Prog*);
double	cputime(void);
void	datblk(long, long);
void	diag(char*, ...);
void	dodata(void);
void	doprof1(void);
void	doprof2(void);
long	entryvalue(void);
void	errorexit(void);
void	exchange(Prog*);
int	find1(long, int);
void	follow(void);
void	gethunk(void);
double	ieeedtod(Ieee*);
long	ieeedtof(Ieee*);
int	isnop(Prog*);
void	ldobj(int, long, char*);
void	loadlib(void);
void	listinit(void);
void	initmuldiv(void);
Sym*	lookup(char*, int);
void	lput(long);
void	mkfwd(void);
void*	mysbrk(ulong);
void	names(void);
void	nocache(Prog*);
void	noops(void);
void	nuxiinit(void);
void	objfile(char*);
int	ocmp(const void*, const void*);
long	opcode(int);
Optab*	oplook(Prog*);
void	patch(void);
void	prasm(Prog*);
void	prepend(Prog*, Prog*);
Prog*	prg(void);
int	pseudo(Prog*);
void	putsymb(char*, int, long, int);
long	regoff(Adr*);
int	relinv(int);
long	rnd(long, long);
void	sched(Prog*, Prog*);
void	span(void);
void	undef(void);
void	xdefine(char*, int, long);
void	xfol(Prog*);

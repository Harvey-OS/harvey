#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"../qc/q.out.h"

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
		long	offset;
		char	sval[NSNAME];
		Ieee	ieee;
	};
	Sym	*sym;
	Auto	*autom;
	char	type;
	uchar	reg;
	char	name;
	char	class;
};
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
	Sym	*sym;
	Auto	*link;
	long	offset;
	short	type;
};
struct	Optab
{
	uchar	as;
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
} oprange[AEND];

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

	C_NCLASS
};

union
{
	struct
	{
		uchar	cbuf[MAXIO];			/* output buffer */
		uchar	xbuf[MAXIO];			/* input buffer */
	};
	char	dbuf[1];
} buf;

long	HEADR;			/* length of header */
int	HEADTYPE;		/* type of header */
long	INITDAT;		/* data location */
long	INITRND;		/* data round above text location */
long	INITTEXT;		/* text location */
char*	INITENTRY;		/* entry point */
long	autosize;
Biobuf	bso;
long	bsssize;
int	cbc;
uchar*	cbp;
int	cout;
Auto*	curauto;
Auto*	curhist;
Prog*	curp;
Prog*	curtext;
Prog*	datap;
Prog*	prog_movsw;
Prog*	prog_movdw;
Prog*	prog_movws;
Prog*	prog_movwd;
long	datsize;
char	debug[128];
Prog*	firstp;
char	fnuxi8[8];
Sym*	hash[NHASH];
Sym*	histfrog[MAXHIST];
int	histfrogp;
int	histgen;
char*	library[50];
char*	libraryobj[50];
int	libraryp;
int	xrefresolv;
char*	hunk;
char	inuxi1[1];
char	inuxi2[2];
char	inuxi4[4];
Prog*	lastp;
long	lcsize;
char	literal[32];
int	nerrors;
long	nhunk;
char*	noname;
long	offset;
char*	outfile;
long	pc;
long	symsize;
long	staticgen;
Prog*	textp;
long	textsize;
long	tothunk;
char	xcmp[C_NCLASS][C_NCLASS];
int	version;
Prog	zprg;
int	dtype;

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
void	names(void);
void	nocache(Prog*);
void	noops(void);
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
long	regoff(Adr*);
int	relinv(int);
long	rnd(long, long);
void	sched(Prog*, Prog*);
void	span(void);
void	undef(void);
void	xdefine(char*, int, long);
void	xfol(Prog*);

#pragma	varargck	type	"D"	Adr*
#pragma	varargck	type	"N"	Adr*
#pragma	varargck	type	"P"	Prog*
#pragma	varargck	type	"R"	int
#pragma	varargck	type	"A"	int
#pragma	varargck	type	"S"	char*

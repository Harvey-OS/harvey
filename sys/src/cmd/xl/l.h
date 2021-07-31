#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"../xc/x.out.h"

typedef	struct	Adr	Adr;
typedef	struct	Sym	Sym;
typedef	struct	Autom	Auto;
typedef	struct	Prog	Prog;
typedef	struct	Optab	Optab;

#define	P		((Prog*)0)
#define	S		((Sym*)0)
#define	TNAME		(curtext&&curtext->from.sym?curtext->from.sym->name:noname)

enum
{
	BIG		= 0x7fff,
	BIGM		= 0xffffff,
	STRINGSZ	= 200,
	DATBLK		= 1024,
	IOMAX		= 8192,
	MAXHIST		= 20,				/* limit of path elements for history symbols */
	NHASH		= 10007,
	NHUNK		= 100000,
	NSCHED		= 20,

	/*
	 * mark flags
	 */
	LABEL		= 1<<0,
	LEAF		= 1<<1,
	FLOAT		= 1<<2,
	BRANCH		= 1<<3,
	LOAD		= 1<<4,
	FCMP		= 1<<5,
	SYNC		= 1<<6,
	LIST		= 1<<7,
	FOLL		= 1<<8,

	/*
	 * sym types
	 */
	STEXT = 1,
	SSDATA,		/* small data in internal ram */
	SDATA,
	SBSS,
	SXREF,
	SLEAF,
	SFILE,

	/*
	 * address classifications
	 */
	C_NONE = 0,

	C_REG,
	C_REGPC,
	C_FREG,
	C_CREG,

	C_ZCON,		/* 0 */
	C_SCON,		/* 16 bit signed */
	C_HCON,		/* top 16 bits */
	C_MCON,		/* hcon or 24 bit unsigned */
	C_LCON,

	C_SACON,	/* auto or param addresses */
	C_LACON,

	C_SBRA,		/* signed 16 bit; only pc relative jump */
	C_MBRA,		/* 24 bit jump */
	C_LBRA,

	C_SEXT,		/* 16 bit unsigned internal mem ref */
	C_MEXT,
	C_LEXT,

	C_IND,		/* integer memory ref */
	C_INDM,		/* load & modify reg */
	C_INDR,		/* load & modify reg by another reg */
	C_INDF,		/* floating memory ref */
	C_INDFM,
	C_INDFR,
	C_FOP,		/* floating mem ref or reg */
	C_FST,		/* floating mem store or nothing */

	C_SOREG,	/* offset from a register; branch addresses only */
	C_MOREG,	/* 24 bits off a register */
	C_LOREG,

	C_ANY,

	C_GOK,

	C_NCLASS
};

struct	Adr
{
	union
	{
		int	increg;
		long	offset;
		char	sval[NSNAME];
		Dsp	dsp;
	};
	Sym	*sym;
	Auto	*autom;
	char	type;
	char	reg;
	char	name;
	char	class;
};
struct	Prog
{
	Adr	from;
	Adr	from1;		/* extra arguments for fmadd */
	Adr	from2;
	Adr	to;
	Prog	*forwd;		/* skip pointers for finding branch destination */
	Prog	*cond;		/* branch desitination */
	Prog	*link;
	long	pc;
	long	line;
	long	stkoff;		/* floating stack offset */
	short	mark;
	uchar	optab;
	uchar	as;
	char	cc;
	char	reg;

	char	onlyop;
	char	nosched;	/* don't schedule this code */
	char	isf;		/* is a float? */
};
struct	Sym
{
	char	*name;
	char	type;
	char	isram;		/* can be put in ram */
	short	size;		/* for ram */
	short	version;
	short	become;
	short	frame;
	long	value;
	Sym	*link;
	Sym	*ram;		/* list of things to put in fast ram */
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
	char	a1;		/* argument classes */
	char	a2;
	char	a3;
	char	a4;
	char	a5;
	char	type;		/* type of instruction */
	char	size;		/* how big the instruction is */
	char	nops;		/* max nops following */
	char	param;		/* a default param */
};
struct
{
	Optab*	start;
	Optab*	stop;
} oprange[AEND];
union
{
	struct
	{
		uchar	cbuf[IOMAX];			/* output buffer */
		uchar	xbuf[IOMAX];			/* input buffer */
	};
	char	dbuf[1];
} buf;

long	headlen;		/* length of header */
int	header;			/* type of header */
long	datstart;		/* data location */
long	datrnd;			/* data round above text location */
long	textstart;		/* text location */
char*	entry;			/* entry point */
ulong	ramstart;		/* internal memory location */
ulong	maxram;			/* limit of ram to use */
ulong	ramend;			/* limit of ram used */
ulong	ramdstart;		/* start of data relative to ramstart */
ulong	ramover;		/* ram overflow into data segment*/
ulong	ramoffset;		/* ram beginning: 0 or 0x50030000 */

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
Prog*	prog_mul;
Prog*	prog_div;
Prog*	prog_divl;
Prog*	prog_mod;
Prog*	prog_modl;
Prog*	prog_fdiv;
long	datsize;
char	debug[128];
Prog*	firstp;
char	fnuxi4[4];
Sym*	hash[NHASH];
Sym*	histfrog[MAXHIST];
Sym	*ram;				/* list to put in internal ram */
int	histfrogp;
int	histgen;
char*	library[50];
int	libraryp;
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
long	spsize;
long	symsize;
long	staticgen;
Prog*	textp;
long	textsize;
long	tothunk;
char	xcmp[C_NCLASS][C_NCLASS];
int	version;
Prog	zprg;

extern	Optab	optab[];
extern	char*	anames[];
extern	char*	ccnames[];
extern	char*	cnames[];

int	Aconv(void*, Fconv*);
int	Dconv(void*, Fconv*);
int	Jconv(void*, Fconv*);
int	Nconv(void*, Fconv*);
int	Pconv(void*, Fconv*);
int	Rconv(void*, Fconv*);
int	Sconv(void*, Fconv*);
int	Xconv(void*, Fconv*);
int	aclass(Prog*, Adr*);
void	addhist(long, int);
void	addnop(Prog*);
void	adjdata(void);
void	append(Prog*, Prog*);
Prog*	appendp(Prog*);
int	aclass(Prog*, Adr*);
void	asmb(void);
void	asmlc(void);
void	asmlc1(int);
int	asmout(Prog*, Optab*, int);
void	asmsp(void);
void	asmsp1(int);
void	asmsym(void);
void	asmtext(int);
void	assignram(void);
long	atolwhex(char*);
Prog*	brloop(Prog*);
void	buildop(void);
void	cflush(void);
int	cmp(int, int);
void	codeput(long);
int	consize(long, int);
int	compound(Prog*);
void	cput(long);
double	cputime(void);
void	datblk(long, long, int);
void	diag(char*, ...);
void	dodata(void);
void	doprof1(void);
void	doprof2(void);
void	dostkoff(void);
double	dsptod(Dsp);
long	entryvalue(void);
void	errorexit(void);
void	exchange(Prog*);
void	expand(void);
int	find1(long, int);
int	fltarg(Adr*, int);
void	follow(void);
void	genaddr(Prog*, Adr*);
void	gethunk(void);
long	grabccnop(Prog*, int cc);
long	grabnop(Prog*);
void	histtoauto(void);
int	increg(Adr*);
void	initmul(void);
void	initdiv(void);
void	initfdiv(void);
void	installfconst(Adr*);
int	isextern(Adr*);
void	ldobj(int, long, char*);
void	listinit(void);
void	loadlib(int, int);
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
Optab*	opsearch(Prog*);
void	patch(void);
void	prasm(Prog*);
void	prepend(Prog*, Prog*);
Prog*	prg(void);
int	pseudo(Prog*);
void	putsymb(char*, int, long, int);
long	regoff(Prog*, Adr*);
int	relinv(int);
long	rnd(long, long);
void	sched(Prog*, Prog*);
long	setmcon(Prog*, Adr*, int);
void	span(int);
void	sput(long);
void	strnput(char*, int);
long	topcon(long);
void	undef(void);
void	xdefine(char*, int, long);
void	xfol(Prog*);

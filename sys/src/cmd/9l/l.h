#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"../9c/9.out.h"

typedef	struct	Adr	Adr;
typedef	struct	Sym	Sym;
typedef	struct	Autom	Auto;
typedef	struct	Prog	Prog;
typedef	struct	Optab	Optab;
typedef	struct	Oprang	Oprang;
typedef	uchar	Opcross[32][2][32];
typedef	struct	Count	Count;

#define	P		((Prog*)0)
#define	S		((Sym*)0)
#define	TNAME		(curtext&&curtext->from.sym?curtext->from.sym->name:noname)

struct	Adr
{
	union
	{
		long	offset;
		char*	sval;
		Ieee*	ieee;
	};
	union
	{
		Auto*	autom;
		Sym*	sym;
	};
	char	type;
	uchar	reg;
	char	name;
	char	class;
};
struct	Prog
{
	Adr	from;
	Adr	to;
	union
	{
		long	regused;
		Prog*	forwd;
	};
	Prog*	cond;
	Prog*	link;
	long	pc;
	long	line;
	uchar	mark;
	uchar	optab;
	uchar	as;
	uchar	reg;
};
struct	Sym
{
	char	*name;
	short	type;
	short	version;
	short	become;
	short	frame;
	long	value;
	Sym*	link;
};
struct	Autom
{
	Sym*	sym;
	Auto*	link;
	long	offset;
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
struct	Oprang
{
	Optab*	start;
	Optab*	stop;
};
struct	Count
{
	long	count;
	long	outof;
};

enum
{
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

	C_ZCON,		/* 0 */
	C_SCON,		/* [0-0xff] */
	C_MCON,		/* [0xffffff00-0xffffffff] */
	C_PCON,		/* [0-0xffff] */
	C_NCON,		/* [0xffff0000-0xffffffff] */
	C_LCON,		/* none of the above */

	C_ZACON,	/* $0(R) */
	C_SACON,	/* $[0-0xff](R) */
	C_MACON,	/* $[0-0xff](R) */
	C_PACON,	/* $[0-0xffff](R) */
	C_NACON,	/* $[0xffff0000-0xffffffff](R) */
	C_LACON,	/* none of the above */

	C_ZOREG,	/* 0(R) */
	C_SOREG,	/* [0-0xff](R) */
	C_MOREG,	/* [0-0xff](R) */
	C_POREG,	/* [0-0xffff](R) */
	C_NOREG,	/* [0xffff0000-0xffffffff](R) */
	C_LOREG,	/* none of the above */

	C_ZAUTO,	/* $0(R) */
	C_SAUTO,	/* $[0-0xff](R) */
	C_MAUTO,	/* $[0-0xff](R) */
	C_PAUTO,	/* $[0-0xffff](R) */
	C_NAUTO,	/* $[0xffff0000-0xffffffff](R) */
	C_LAUTO,	/* none of the above */

	C_SEXT,		/* none of the above */
	C_LEXT,		/* none of the above */
	C_SBRA,		/* [0-0x3fffc](PC) */
	C_LBRA,		/* none of the above */

	C_GOK,

	NSCHED		= 20,

/* mark flags */
	FOLL		= 1<<0,
	LABEL		= 1<<1,
	LEAF		= 1<<2,
	SYNC		= 1<<3,
	BRANCH		= 1<<4,
	LOAD		= 1<<5,
	FCMP		= 1<<6,
	NOSCHED		= 1<<7,

	STRINGSZ	= 200,
	NHASH		= 10007,
	NHUNK		= 100000,
	MINSIZ		= 8,
	MIDSIZ		= 64,
	NENT		= 100,
	MAXIO		= 8192,
	MAXHIST		= 20,		/* limit of path elements for history symbols */
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
long	datsize;
char	debug[128];
Prog*	etextp;
Prog*	firstp;
char	fnuxi8[8];
char*	noname;
Sym*	hash[NHASH];
Sym*	histfrog[MAXHIST];
int	histfrogp;
int	histgen;
char*	library[50];
char*	libraryobj[50];
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
int	noacache;
long	offset;
Opcross	opcross[8];
Oprang	oprange[ALAST];
char*	outfile;
long	pc;
uchar	repop[ALAST];
long	symsize;
Prog*	textp;
long	textsize;
long	thunk;
int	version;
char	xcmp[32][32];
Prog	zprg;
int	dtype;
Prog*	prog_mull;
Prog*	prog_mulul;
Prog*	prog_mulml;
Prog*	prog_mulmul;
Prog*	prog_divl;
Prog*	prog_divul;

struct
{
	Count	branch;
	Count	fcmp;
	Count	load;
	Count	mfrom;
	Count	page;
	Count	jump;
} nop;

extern	char*	anames[];
extern	Optab	optab[];

#pragma	varargck	type	"A"	int
#pragma	varargck	type	"D"	Adr*
#pragma	varargck	type	"N"	Adr*
#pragma	varargck	type	"P"	Prog*
#pragma	varargck	type	"S"	char*

int	Aconv(Fmt*);
int	Dconv(Fmt*);
int	Nconv(Fmt*);
int	Pconv(Fmt*);
int	Sconv(Fmt*);
int	aclass(Adr*);
void	addhist(long, int);
void	addnop(Prog*);
void	append(Prog*, Prog*);
void	asmb(void);
void	asmlc(void);
int	asmout(Prog*, Optab*, int);
void	asmsym(void);
long	atolwhex(char*);
Prog*	brloop(Prog*);
Biobuf	bso;
void	buildop(void);
void	buildrep(int, int);
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
void	histtoauto(void);
double	ieeedtod(Ieee*);
long	ieeedtof(Ieee*);
int	isnop(Prog*);
void	ldobj(int, long, char*);
void	loadlib(int, int);
void	listinit(void);
Sym*	lookup(char*, int);
void	lput(long);
void	mkfwd(void);
void	names(void);
void	nocache(Prog*);
void	noops(void);
void	nuxiinit(void);
void	objfile(char*);
int	ocmp(void*, void*);
long	opmem(int, int, int, int);
Optab*	oplook(Prog*);
long	oprrr(int, int, int, int);
long	opir(int, int, int);
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
void	strnput(char*, int);
void	undef(void);
void	xdefine(char*, int, long);
void	xfol(Prog*);
void	xfol(Prog*);
void	nopstat(char*, Count*);
void	initmul(void);
void	initdiv(void);

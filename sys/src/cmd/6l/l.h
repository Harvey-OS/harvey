#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"../6c/6.out.h"

#define	P		((Prog*)0)
#define	S		((Sym*)0)
#define	TNAME		(curtext?curtext->from.sym->name:noname)
#define	CPUT(c)\
	{ *cbp++ = c;\
	if(--cbc <= 0)\
		cflush(); }

typedef	struct	Adr	Adr;
typedef	struct	Prog	Prog;
typedef	struct	Sym	Sym;
typedef	struct	Auto	Auto;
typedef	struct	Optab	Optab;

struct	Adr
{
	union
	{
		long	offset;
		char	scon[8];
		Prog	*cond;	/* not used, but should be D_BRANCH */
		Ieee	ieee;
	};
	union
	{
		Auto*	autom;
		Sym*	sym;
	};
	short	type;
	char	index;
	char	scale;
};

struct	Prog
{
	Adr	from;
	Adr	to;
	Prog	*forwd;
	Prog*	link;
	Prog*	cond;	/* work on this */
	long	pc;
	long	line;
	uchar	mark;	/* work on these */
	uchar	back;
	uchar	as;

	char	width;		/* fake for DATA */
	uchar	type;
	uchar	offset;
};
struct	Auto
{
	Sym*	sym;
	Auto*	link;
	long	offset;
	short	type;
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
struct	Optab
{
	uchar	as;
	uchar*	ytab;
	ushort	op[3];
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

	NHASH		= 10007,
	NHUNK		= 100000,
	MINSIZ		= 4,
	STRINGSZ	= 200,
	MAXIO		= 8192,
	MAXHIST		= 20,				/* limit of path elements for history symbols */
	MINLC		= 4,

/* mark flags */
	LABEL		= 1<<0,
	LEAF		= 1<<1,
	ACTIVE1		= 1<<2,
	ACTIVE2		= 1<<3,
	BRANCH		= 1<<4,
	LOAD		= 1<<5,
	COMPARE		= 1<<6,
	MFROM		= 1<<7,
	LIST		= 1<<8,
	FOLL		= 1<<9,

	Yxxx		= 0,
	Ynone,
	Yr,
	Ym,
	Yi5,
	Yi32,
	Ybr,
	Ycol,
	Yri5,
	Ynri5,
	Ymax,

	Zxxx		= 0,
	Zpseudo,
	Zbr,
	Zrxr,
	Zrrx,
	Zirx,
	Zrrr,
	Zirr,
	Zir,
	Zmbr,
	Zrm,
	Zmr,
	Zim,
	Zlong,
	Znone,
	Zmax,

	Anooffset	= 1<<0,
	Abigoffset	= 1<<1,
	Aindex		= 1<<2,
	Abase		= 1<<3,
};
union
{
	struct
	{
		char	cbuf[MAXIO];			/* output buffer */
		uchar	xbuf[MAXIO];			/* input buffer */
	};
	char	dbuf[1];
} buf;

long	HEADR;
long	HEADTYPE;
long	INITDAT;
long	INITRND;
long	INITTEXT;
char*	INITENTRY;		/* entry point */
long	autosize;
Biobuf	bso;
long	bsssize;
long	casepc;
int	cbc;
char*	cbp;
char*	pcstr;
int	cout;
Auto*	curauto;
Auto*	curhist;
Prog*	curp;
Prog*	curtext;
Prog*	datap;
Prog*	edatap;
long	datsize;
char	debug[128];
char	literal[32];
Prog*	etextp;
Prog*	firstp;
char	fnuxi8[8];
char	fnuxi4[4];
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
char	ycover[Ymax*Ymax];
ulong*	andptr;
ulong	and[4];
Prog*	lastp;
long	lcsize;
int	maxop;
int	nerrors;
long	nhunk;
long	nsymbol;
char*	noname;
char*	outfile;
long	pc;
int	printcol;
Sym*	symlist;
Sym*	symSB;
long	symsize;
Prog*	textp;
long	textsize;
long	thunk;
int	version;
Prog	zprg;

extern	Optab	optab[];
extern	char*	anames[];

#pragma	varargck	type	"A"	uint
#pragma	varargck	type	"D"	Adr*
#pragma	varargck	type	"P"	Prog*
#pragma	varargck	type	"R"	int
#pragma	varargck	type	"S"	char*

int	Aconv(Fmt*);
int	Dconv(Fmt*);
int	Pconv(Fmt*);
int	Rconv(Fmt*);
int	Sconv(Fmt*);
void	addhist(long, int);
Prog*	appendp(Prog*);
void	asmb(void);
void	asmins(Prog*);
void	asmlc(void);
void	asmsym(void);
int	atoi(char*);
long	atolwhex(char*);
Prog*	brchain(Prog*);
Prog*	brloop(Prog*);
void	cflush(void);
Prog*	copyp(Prog*);
double	cputime(void);
void	datblk(long, long);
void	diag(char*, ...);
void	dodata(void);
void	doinit(void);
void	doprof1(void);
void	doprof2(void);
void	noops(void);
long	entryvalue(void);
void	errorexit(void);
int	find1(long, int);
int	find2(long, int);
void	follow(void);
void	gethunk(void);
void	histtoauto(void);
double	ieeedtod(Ieee*);
long	ieeedtof(Ieee*);
void	ldobj(int, long, char*);
void	loadlib(int, int);
void	listinit(void);
Sym*	lookup(char*, int);
void	lput(long);
void	lputl(long);
void	main(int, char*[]);
void	mkfwd(void);
void	nuxiinit(void);
void	objfile(char*);
int	opsize(Prog*);
void	patch(void);
Prog*	prg(void);
int	relinv(int);
long	reuse(Prog*, Sym*);
long	rnd(long, long);
void	s8put(char*);
void	span(void);
void	undef(void);
void	xdefine(char*, int, long);
void	xfol(Prog*);
int	zaddr(uchar*, Adr*, Sym*[]);

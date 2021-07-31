#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"../8c/8.out.h"

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
	union
	{
		Prog	*forwd;
	};
	Prog*	link;
	Prog*	cond;	/* work on this */
	long	pc;
	long	line;
	uchar	mark;	/* work on these */
	uchar	back;

	short	as;
	char	width;		/* fake for DATA */
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
	short	as;
	uchar*	ytab;
	uchar	prefix;
	uchar	op[10];
};

enum
{
	STEXT		= 1,
	SDATA,
	SBSS,
	SDATA1,
	SXREF,
	SFILE,
	SCONST,

	NHASH		= 10007,
	NHUNK		= 100000,
	MINSIZ		= 4,
	STRINGSZ	= 200,
	MINLC		= 1,
	MAXIO		= 8192,
	MAXHIST		= 20,				/* limit of path elements for history symbols */

	Yxxx		= 0,
	Ynone,
	Yi0,
	Yi1,
	Yi8,
	Yi32,
	Yiauto,
	Yal,
	Ycl,
	Yax,
	Ycx,
	Yrb,
	Yrl,
	Yrf,
	Yf0,
	Yrx,
	Ymb,
	Yml,
	Ym,
	Ybr,
	Ycol,

	Ycs,	Yss,	Yds,	Yes,	Yfs,	Ygs,
	Ygdtr,	Yidtr,	Yldtr,	Ymsw,	Ytask,
	Ycr0,	Ycr1,	Ycr2,	Ycr3,	Ycr4,	Ycr5,	Ycr6,	Ycr7,
	Ydr0,	Ydr1,	Ydr2,	Ydr3,	Ydr4,	Ydr5,	Ydr6,	Ydr7,
	Ytr0,	Ytr1,	Ytr2,	Ytr3,	Ytr4,	Ytr5,	Ytr6,	Ytr7,
	Ymax,

	Zxxx		= 0,

	Zlit,
	Z_rp,
	Zbr,
	Zcall,
	Zib_,
	Zib_rp,
	Zibo_m,
	Zil_,
	Zil_rp,
	Zilo_m,
	Zjmp,
	Zloop,
	Zm_o,
	Zm_r,
	Zaut_r,
	Zo_m,
	Zpseudo,
	Zr_m,
	Zrp_,
	Z_ib,
	Z_il,
	Zm_ibo,
	Zm_ilo,
	Zclr,
	Zbyte,
	Zmov,
	Zmax,

	Px		= 0,
	Pe		= 0x66,	/* operand escape */
	Pm		= 0x0f,	/* 2byte opcode escape */
	Pq		= 0xff,	/* both escape */
	Pb		= 0xfe,	/* byte operands */
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
int	libraryp;
char*	hunk;
char	inuxi1[1];
char	inuxi2[2];
char	inuxi4[4];
char	ycover[Ymax*Ymax];
uchar*	andptr;
uchar	and[10];
char	reg[D_NONE];
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
long	spsize;
Sym*	symlist;
long	symsize;
Prog*	textp;
long	textsize;
long	thunk;
int	version;
Prog	zprg;
int	dtype;

extern	Optab	optab[];
extern	char*	anames[];

int	Aconv(void*, Fconv*);
int	Dconv(void*, Fconv*);
int	Pconv(void*, Fconv*);
int	Rconv(void*, Fconv*);
int	Sconv(void*, Fconv*);
int	Xconv(void*, Fconv*);
void	addhist(long, int);
Prog*	appendp(Prog*);
void	asmb(void);
void	asmins(Prog*);
void	asmlc(void);
void	asmsp(void);
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
void	dostkoff(void);
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
long	vaddr(Adr*);

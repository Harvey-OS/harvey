#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"../8c/8.out.h"

#ifndef	EXTERN
#define	EXTERN	extern
#endif

#define	P		((Prog*)0)
#define	S		((Sym*)0)
#define	TNAME		(curtext?curtext->from.sym->name:noname)

typedef	struct	Adr	Adr;
typedef	struct	Prog	Prog;
typedef	struct	Sym	Sym;
typedef	struct	Auto	Auto;
typedef	struct	Optab	Optab;
typedef	struct	Reloc	Reloc;
typedef	struct	Relocsym	Relocsym;

struct	Adr
{
	union
	{
		long	u0offset;
		char	u0scon[8];
		Prog	*u0cond;	/* not used, but should be D_BRANCH */
		Ieee	u0ieee;
	} u0;
	union
	{
		Auto*	u1autom;
		Sym*	u1sym;
	} u1;
	short	type;
	char	index;
	char	scale;
};

#define	offset	u0.u0offset
#define	scon	u0.u0scon
#define	cond	u0.u0cond
#define	ieee	u0.u0ieee

#define	autom	u1.u1autom
#define	sym	u1.u1sym

struct	Prog
{
	Adr	from;
	Adr	to;
	Prog	*forwd;
	Prog*	link;
	Prog*	pcond;	/* work on this */
	long	pc;
	long	line;
	uchar	mark;	/* work on these */
	uchar	back;

	short	as;
	char	width;		/* fake for DATA */
};
struct	Auto
{
	Sym*	asym;
	Auto*	link;
	long	aoffset;
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
struct	Reloc
{
	char	type;
	long	pc;
	Reloc	*link;
};
struct	Relocsym
{
	Sym	*s;
	Reloc	*reloc;
	Relocsym	*link;
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
	SUNDEF,

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
	Zib_rr,
	Zil_rr,
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

EXTERN union
{
	struct
	{
		char	obuf[MAXIO];			/* output buffer */
		uchar	ibuf[MAXIO];			/* input buffer */
	} u;
	char	dbuf[1];
} buf;

#define	cbuf	u.obuf
#define	xbuf	u.ibuf

#pragma	varargck	type	"A"	uint
#pragma	varargck	type	"D"	Adr*
#pragma	varargck	type	"P"	Prog*
#pragma	varargck	type	"R"	int
#pragma	varargck	type	"S"	char*

EXTERN	long	HEADR;
EXTERN	long	HEADTYPE;
EXTERN	long	INITDAT;
EXTERN	long	INITRND;
EXTERN	long	INITTEXT;
EXTERN	char*	INITENTRY;		/* entry point */
EXTERN	Biobuf	bso;
EXTERN	long	bsssize;
EXTERN	long	casepc;
EXTERN	int	cbc;
EXTERN	char*	cbp;
EXTERN	char*	pcstr;
EXTERN	int	cout;
EXTERN	Auto*	curauto;
EXTERN	Auto*	curhist;
EXTERN	Prog*	curp;
EXTERN	Prog*	curtext;
EXTERN	Prog*	datap;
EXTERN	Prog*	edatap;
EXTERN	long	datsize;
EXTERN	char	debug[128];
EXTERN	char	literal[32];
EXTERN	Prog*	etextp;
EXTERN	Prog*	firstp;
EXTERN	char	fnuxi8[8];
EXTERN	char	fnuxi4[4];
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
EXTERN	char	ycover[Ymax*Ymax];
EXTERN	uchar*	andptr;
EXTERN	uchar	and[30];
EXTERN	char	reg[D_NONE];
EXTERN	Prog*	lastp;
EXTERN	long	lcsize;
EXTERN	int	maxop;
EXTERN	int	nerrors;
EXTERN	long	nhunk;
EXTERN	long	nsymbol;
EXTERN	char*	noname;
EXTERN	char*	outfile;
EXTERN	long	pc;
EXTERN	long	spsize;
EXTERN	Sym*	symlist;
EXTERN	long	symsize;
EXTERN	Prog*	textp;
EXTERN	long	textsize;
EXTERN	long	thunk;
EXTERN	int	version;
EXTERN	Prog	zprg;
EXTERN	int	dtype;

EXTERN	int	reloc;
EXTERN	Adr*	reloca;
EXTERN	Relocsym*	relocs;
EXTERN	char*	undefs[32];
EXTERN	int	undefsp;
EXTERN	Prog	undefp;

#define	UP	(&undefp)

extern	Optab	optab[];
extern	char*	anames[];

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
void	asmsp(void);
void	asmsym(void);
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
void	loadlib(void);
void	listinit(void);
Sym*	lookup(char*, int);
void	lput(long);
void	cput(int);
void	lputl(long);
void	main(int, char*[]);
void	mkfwd(void);
void*	mysbrk(ulong);
void	nuxiinit(void);
void	objfile(char*);
int	opsize(Prog*);
void	patch(void);
Prog*	prg(void);
void	readundefs(void);
int	relinv(int);
long	reuse(Prog*, Sym*);
long	rnd(long, long);
void	s8put(char*);
void	span(void);
void	undef(void);
long	vaddr(Adr*);
void	wreloc(Sym*, long);
void	xdefine(char*, int, long);
void	xfol(Prog*);
int	zaddr(uchar*, Adr*, Sym*[]);

#pragma	varargck	type	"D"	Adr*
#pragma	varargck	type	"P"	Prog*
#pragma	varargck	type	"R"	int
#pragma	varargck	type	"A"	int

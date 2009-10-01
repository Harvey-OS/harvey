#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"../2c/2.out.h"

#ifndef	EXTERN
#define	EXTERN	extern
#endif

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
	short	type;
	short	index;
	union
	{
		struct
		{
			long	u0displace;
			long	u0offset;
		} s0;
		char	u0scon[8];
		Prog	*u0cond;	/* not used, but should be D_BRANCH */
		Ieee	u0ieee;
	} u0;
	union
	{
		Auto*	u1autom;
		Sym*	u1sym;
	} u1;
	uchar	field;
	uchar	scale;
};

#define	displace u0.s0.u0displace
#define	offset	u0.s0.u0offset
#define	scon	u0.u0scon
#define	cond	u0.u0cond
#define	ieee	u0.u0ieee

#define	autom	u1.u1autom
#define	sym	u1.u1sym

struct	Prog
{
	Adr	from;
	Adr	to;
	union
	{
		long	u0stkoff;
		Prog	*u0forwd;
	} u0;
	Prog*	link;
	Prog*	pcond;	/* work on this */
	long	pc;
	long	line;
	short	as;
	uchar	mark;	/* work on these */
	uchar	back;
};

#define	stkoff	u0.u0stkoff
#define	forwd	u0.u0forwd

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
	short	fas;
	short	srcsp;
	short	dstsp;
	ushort	optype;
	ushort	opcode0;
	ushort	opcode1;
	ushort	opcode2;
	ushort	opcode3;
};

enum
{
	STEXT		= 1,
	SDATA,
	SBSS,
	SDATA1,
	SXREF,
	SAUTO,
	SPARAM,
	SFILE,
	NHASH		= 10007,
	NHUNK		= 100000,
	MINSIZ		= 4,
	STRINGSZ	= 200,
	MAXIO		= 8192,
	MAXHIST		= 20,				/* limit of path elements for history symbols */
	A6OFFSET 	= 32766
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
EXTERN	int	cout;
EXTERN	Auto*	curauto;
EXTERN	Auto*	curhist;
EXTERN	Prog*	curp;
EXTERN	Prog*	curtext;
EXTERN	Prog*	datap;
EXTERN	long	datsize;
EXTERN	char	debug[128];
EXTERN	Prog*	etextp;
EXTERN	Prog*	firstp;
EXTERN	char	fnuxi8[8];
EXTERN	char	gnuxi8[8];
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
EXTERN	long	ncase;
EXTERN	long	ndata;
EXTERN	int	nerrors;
EXTERN	long	nhunk;
EXTERN	long	nsymbol;
EXTERN	char*	noname;
EXTERN	short*	op;
EXTERN	char*	outfile;
EXTERN	long	pc;
EXTERN	char	simple[I_MASK];
EXTERN	char	special[I_MASK];
EXTERN	long	spsize;
EXTERN	Sym*	symlist;
EXTERN	long	symsize;
EXTERN	Prog*	textp;
EXTERN	long	textsize;
EXTERN	long	thunk;
EXTERN	int	version;
EXTERN	Prog	zprg;

extern	Optab	optab[];
extern	char	mmsize[];
extern	char*	anames[];

#pragma	varargck	type	"A"	int
#pragma	varargck	type	"D"	Adr*
#pragma	varargck	type	"P"	Prog*
#pragma	varargck	type	"R"	int
#pragma	varargck	type	"S"	char*

#pragma	varargck	argpos	diag 1

int	Aconv(Fmt*);
int	Dconv(Fmt*);
int	Pconv(Fmt*);
int	Rconv(Fmt*);
int	Sconv(Fmt*);
int	Xconv(Fmt*);
void	addhist(long, int);
int	andsize(Prog*, Adr*);
Prog*	appendp(Prog*);
void	asmb(void);
int	asmea(Prog*, Adr*);
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
void	doprof1(void);
void	doprof2(void);
void	dostkoff(void);
long	entryvalue(void);
void	errorexit(void);
int	find1(long, int);
int	find2(long, int);
void	follow(void);
void	gethunk(void);
int	gnuxi(Ieee*, int, int);
void	histtoauto(void);
double	ieeedtod(Ieee*);
long	ieeedtof(Ieee*);
void	ldobj(int, long, char*);
void	loadlib(void);
void	listinit(void);
Sym*	lookup(char*, int);
void	lput(long);
void	main(int, char*[]);
void	mkfwd(void);
void*	mysbrk(ulong);
void	nuxiinit(void);
void	objfile(char*);
void	patch(void);
Prog*	prg(void);
int	relinv(int);
long	reuse(Prog*, Sym*);
long	rnd(long, long);
void	s16put(char*);
void	span(void);
void	undef(void);
void	xdefine(char*, int, long);
void	xfol(Prog*);
int	zaddr(uchar*, Adr*, Sym*[]);

#include <u.h>
#include <libc.h>
#include <bio.h>
#include "../zc/z.out.h"

typedef	struct	Adr	Adr;
typedef	struct	Prog	Prog;
typedef	struct	Sym	Sym;
typedef struct	Optab	Optab;
typedef struct	Auto	Auto;

#define	P		((Prog*)0)
#define	S		((Sym*)0)
#define	TNAME		(curtext?curtext->from.sym->name:noname)
#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))

struct	Adr
{
	union
	{
		Auto*	autom;
		Sym*	sym;
	};
	short	type;
	short	width;
	union
	{
		long	offset;
		double	dval;
		Ieee	ieee;
		char	sval[NSNAME];
	};
};
struct	Prog
{
	Adr	from;
	Adr	to;
	Prog*	forwd;
	Prog*	cond;
	Prog*	link;
	long	pc;
	long	line;
	short	catch;
	char	as;
	char	mark;
	char	back;
};
struct	Sym
{
	char	name[NNAME];
	short	type;
	short	version;
	short	lstack;
	short	rstack;
	long	value;
	Sym*	link;
	Prog*	calledby;
};
struct	Optab
{
	uchar	as;
	uchar	type;
	uchar	genop;
	char*	decod;
};
struct	Auto
{
	Sym*	sym;
	Auto*	link;
	long	offset;
	short	type;
};
union
{
	struct
	{
		uchar	cbuf[8192];			/* output buffer */
		uchar	xbuf[8192];			/* input buffer */
		Rlent	rlent[8192/sizeof(Rlent)];	/* ranlib buf */
	};
	char	dbuf[1];
} buf;

enum
{
	S_NEG5	= 0,
	S_IMM5,
	S_UNS5,
	S_XOR5,
	S_WAI5,
	S_STK5,
	S_ISTK5,
	S_IMM16,
	S_STK16,
	S_ABS16,
	S_IMM32,
	S_STK32,
	S_ABS32,
	S_ISTK16,
	S_ISTK32,

	STEXT	 = 1,
	SLEAF,
	SDATA,
	SBSS,
	SDATA1,
	SXREF,
	SFILE,

	TPSUD	= 0,
	TNNAD,
	TSMON,
	TDMON,
	TDYAD,
	TFLOT,
	TPCTO,
	TWORD,
	TLONG,
	TTEXT,
	TXXXX,

	JMPLO		= -1024,
	AMOD		= 0x1,		/* rounding of pc */
	SEGBITS		= 0xe0000000,
	DATBLK		= 1024,
	NHASH		= 10007,
	NHUNK		= 50000,
	SBSIZE		= 256,
	MINSIZ		= 4,
	NENT		= 100,
	STRINGSZ	= 200,
};

long	HEADR;
long	HEADTYPE;
long	INITDAT;
long	INITRND;
long	INITTEXT;
char*	INITENTRY;		/* entry point */
Biobuf	bso;
int	XHIG;
int	XLOW;
long	autosize;
long	bsssize;
int	cbc;
uchar*	cbp;
int	swizflag;
int	cout;
Auto*	curauto;
Prog*	curp;
Prog*	curtext;
Prog*	datap;
long	datsize;
char	debug[128];
Prog*	firstp;
char	fnuxi8[8];
Sym*	hash[NHASH];
Sym*	histfrog[NNAME/2-1];
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
char	literal[NNAME];
long	ndata;
int	nerrors;
long	nhunk;
char*	noname;
char*	op;
long	operand;
char*	outfile;
long	pc;
long	symsize;
Prog*	textp;
long	textsize;
long	tothunk;
int	version;

extern	char*	anames[];
extern	char	codc1[];
extern	char	codc2[];
extern	Optab	optab[];
extern	char*	wname[];
extern	char*	wnames[];
extern	short	wsize[];

int	Aconv(void*, Fconv*);
int	Dconv(void*, Fconv*);
int	Pconv(void*, Fconv*);
int	Sconv(void*, Fconv*);
int	Wconv(void*, Fconv*);
void	addhist(long, int);
void	addto(Sym *s, int d);
int	and10asm(Prog *p, Optab *o);
int	and10size(Prog *p);
int	andasm(Prog *p, Optab *o, int flt);
int	anddec(int sf, int st, char *c);
int	andpcasm(Prog *p, Optab *o);
int	andsize(Prog *p, Adr *a);
void	asmb(void);
void	asmlc(void);
void	asmsym(void);
long	atolwhex(char *s);
Prog*	brloop(Prog *p);
void	bugs(void);
void	called(void);
void	cflush(void);
double	cputime(void);
void	datblk(long s, long n);
void	diag(char *a, ...);
void	dodata(void);
void	doprof1(void);
void	doprof2(void);
long	entryvalue(void);
void	errorexit(void);
int	exactieeef(Ieee*);
int	fandsize(Prog *p, Adr *a);
int	find1(long, int);
int	fltadr(Prog *p, int s, int w);
void	follow(void);
int	genadr(Prog *p, int s, int w);
void	gethunk(void);
double	ieeedtod(Ieee*);
long	ieeedtof(Ieee*);
void	ldobj(int f, long c, char *pn);
void	loadlib(int, int);
void	listinit(void);
Sym*	lookup(char *symb, int v);
void	lput(long l);
void	main(int argc, char *argv[]);
int	maxref(Prog *q);
void	mkfwd(void);
void	names(void);
void	nuxiinit(void);
void	objfile(char *file);
void	patch(void);
Prog*	prg(void);
void	putsymb(char *s, int t, long v, int ver);
int	relinv(int a);
long	rnd(long v, long r);
void	span(void);
void	undef(void);
void	xdefine(char *p, int t, long v);
void	xfol(Prog *p);

#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"../2c/2.out.h"

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
			long	displace;
			long	offset;
		};
		char	scon[8];
		Prog	*cond;	/* not used, but should be D_BRANCH */
		Ieee	ieee;
	};
	union
	{
		Auto*	autom;
		Sym*	sym;
	};
	uchar	field;
	uchar	scale;
};

struct	Prog
{
	Adr	from;
	Adr	to;
	union
	{
		long	stkoff;
		Prog	*forwd;
	};
	Prog*	link;
	Prog*	cond;	/* work on this */
	long	pc;
	long	line;
	short	as;
	uchar	mark;	/* work on these */
	uchar	back;
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
	A6OFFSET 	= 32766,
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
char	gnuxi8[8];
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
Prog*	lastp;
long	lcsize;
int	maxop;
long	ncase;
long	ndata;
int	nerrors;
long	nhunk;
long	nsymbol;
char*	noname;
short*	op;
char*	outfile;
long	pc;
char	simple[I_MASK];
char	special[I_MASK];
long	spsize;
Sym*	symlist;
long	symsize;
Prog*	textp;
long	textsize;
long	thunk;
int	version;
Prog	zprg;

extern	Optab	optab[];
extern	char	msize[];
extern	char*	anames[];

int	Aconv(void*, Fconv*);
int	Dconv(void*, Fconv*);
int	Pconv(void*, Fconv*);
int	Rconv(void*, Fconv*);
int	Sconv(void*, Fconv*);
int	Xconv(void*, Fconv*);
void	addhist(long, int);
int	andsize(Prog*, Adr*);
Prog*	appendp(Prog*);
void	asmb(void);
int	asmea(Prog*, Adr*);
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
void	loadlib(int, int);
void	listinit(void);
Sym*	lookup(char*, int);
void	lput(long);
void	main(int, char*[]);
void	mkfwd(void);
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

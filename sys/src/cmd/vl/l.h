#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"../vc/v.out.h"
#include	"../8l/elf.h"

#ifndef	EXTERN
#define	EXTERN	extern
#endif

#define	LIBNAMELEN	300

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
		long	u0offset;
		char*	u0sval;
		Ieee*	u0ieee;
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
	union
	{
		long	u0regused;
		Prog*	u0forwd;
	} u0;
	Prog*	cond;
	Prog*	link;
	long	pc;
	long	line;
	uchar	mark;
	uchar	optab;
	char	as;
	char	reg;
};
#define	regused	u0.u0regused
#define	forwd	u0.u0forwd

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
	Sym*	asym;
	Auto*	link;
	long	aoffset;
	short	type;
};
struct	Optab
{
	char	as;
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
	SSTRING,

	C_NONE		= 0,
	C_REG,
	C_FREG,
	C_FCREG,
	C_MREG,
	C_HI,
	C_LO,
	C_ZCON,
	C_SCON,
	C_ADD0CON,
	C_AND0CON,
	C_ADDCON,
	C_ANDCON,
	C_UCON,
	C_LCON,
	C_SACON,
	C_SECON,
	C_LACON,
	C_LECON,
	C_SBRA,
	C_LBRA,
	C_SAUTO,
	C_SEXT,
	C_LAUTO,
	C_LEXT,
	C_ZOREG,
	C_SOREG,
	C_LOREG,
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

	BIG		= 32766,
	STRINGSZ	= 200,
	NHASH		= 10007,
	NHUNK		= 100000,
	MINSIZ		= 64,
	NENT		= 100,
	MAXIO		= 8192,
	MAXHIST		= 20,				/* limit of path elements for history symbols */
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
EXTERN	long	INITTEXTP;		/* text location (physical) */
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
EXTERN	long	datsize;
EXTERN	char	debug[128];
EXTERN	Prog*	etextp;
EXTERN	Prog*	firstp;
EXTERN	char	fnuxi4[4];	/* for 3l [sic] */
EXTERN	char	fnuxi8[8];
EXTERN	char*	noname;
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
EXTERN	long	instoffset;
EXTERN	Opcross	opcross[10];
EXTERN	Oprang	oprange[ALAST];
EXTERN	char*	outfile;
EXTERN	long	pc;
EXTERN	uchar	repop[ALAST];
EXTERN	long	symsize;
EXTERN	Prog*	textp;
EXTERN	long	textsize;
EXTERN	long	thunk;
EXTERN	int	version;
EXTERN	char	xcmp[32][32];
EXTERN	Prog	zprg;
EXTERN	int	dtype;
EXTERN	int	little;

EXTERN	struct
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

#pragma	varargck	argpos	diag 1

int	Aconv(Fmt*);
int	Dconv(Fmt*);
int	Nconv(Fmt*);
int	Pconv(Fmt*);
int	Sconv(Fmt*);
int	aclass(Adr*);
void	addhist(long, int);
void	addlibpath(char*);
void	addnop(Prog*);
void	append(Prog*, Prog*);
void	asmb(void);
void	asmlc(void);
int	asmout(Prog*, Optab*, int);
void	asmsym(void);
long	atolwhex(char*);
Prog*	brloop(Prog*);
void	buildop(void);
void	buildrep(int, int);
void	cflush(void);
int	cmp(int, int);
void	cput(long);
int	compound(Prog*);
double	cputime(void);
void	datblk(long, long, int);
void	diag(char*, ...);
void	dodata(void);
void	doprof1(void);
void	doprof2(void);
long	entryvalue(void);
void	errorexit(void);
void	exchange(Prog*);
int	fileexists(char*);
int	find1(long, int);
char*	findlib(char*);
void	follow(void);
void	gethunk(void);
void	histtoauto(void);
double	ieeedtod(Ieee*);
long	ieeedtof(Ieee*);
int	isnop(Prog*);
void	ldobj(int, long, char*);
void	loadlib(void);
void	listinit(void);
Sym*	lookup(char*, int);
void	llput(vlong);
void	llputl(vlong);
void	lput(long);
void	lputl(long);
void	bput(long);
void	mkfwd(void);
void*	mysbrk(ulong);
void	names(void);
void	nocache(Prog*);
void	noops(void);
void	nuxiinit(void);
void	objfile(char*);
int	ocmp(const void*, const void*);
long	opirr(int);
Optab*	oplook(Prog*);
long	oprrr(int);
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
void	wput(long);
void	wputl(long);
void	xdefine(char*, int, long);
void	xfol(Prog*);
void	xfol(Prog*);
void	nopstat(char*, Count*);

#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"../7c/7.out.h"

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
		vlong	offset;
		char*	sval;
		Ieee*	ieee;
	};
	union
	{
		Auto*	autom;
		Sym*	sym;
	};
	char	type;
	char	reg;
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
	C_FCREG,
	C_PREG,
	C_PCC,
	C_ZCON,
	C_BCON,
	C_NCON,
	C_SCON,
	C_UCON,
	C_LCON,
	C_QCON,
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
	LABEL2		= 1<<5,
	COMPARE		= 1<<6,
	NOSCHED		= 1<<7,

	BIG		= 32760,
	STRINGSZ	= 200,
	NHASH		= 10007,
	NHUNK		= 100000,
	MINSIZ		= 64,
	NENT		= 100,
	MAXIO		= 8192,
	MAXHIST		= 20,				/* limit of path elements for history symbols */
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
vlong	offset;
Opcross	opcross[7];
Oprang	oprange[ALAST];
char*	outfile;
long	pc;
Prog	*prog_divq;
Prog	*prog_divqu;
Prog	*prog_modq;
Prog	*prog_modqu;
Prog	*prog_divl;
Prog	*prog_divlu;
Prog	*prog_modl;
Prog	*prog_modlu;
uchar	repop[ALAST];
long	symsize;
Prog*	textp;
long	textsize;
long	thunk;
int	version;
char	xcmp[32][32];
Prog	zprg;
int	dtype;

extern	char*	anames[];
extern	Optab	optab[];

#pragma	varargck	type	"A"	int
#pragma	varargck	type	"A"	uint
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
void	addnop(Prog*);
void	append(Prog*, Prog*);
void	asmb(void);
void	asmlc(void);
int	asmout(Prog*, Optab*);
void	asmsym(void);
long	atolwhex(char*);
Prog*	brloop(Prog*);
Biobuf	bso;
void	buildop(void);
void	buildrep(int, int);
void	cflush(void);
int	cmp(int, int);
int	compound(Prog*);
int	conflict(long, long);
double	cputime(void);
void	datblk(long, long);
int	depend(long, long);
void	diag(char*, ...);
Prog *divsubr(int);
void	dodata(void);
void	doprof1(void);
void	doprof2(void);
long	entryvalue(void);
void	errorexit(void);
void	exchange(Prog*);
int	find1(long, int);
void	follow(void);
Prog *genXXX(Prog *, int, Adr*, int, Adr*);
Prog *genRRR(Prog *, int, int, int, int);
Prog *genIRR(Prog *, int, vlong, int, int);
Prog *genstore(Prog *, int, int, vlong, int);
Prog *genload(Prog *, int, vlong, int, int);
Prog *genjmp(Prog *, int, vlong, int);
void	gethunk(void);
void	histtoauto(void);
double	ieeedtod(Ieee*);
long	ieeedtof(Ieee*);
void	initdiv(void);
void	ldobj(int, long, char*);
void	loadlib(void);
void	listinit(void);
Sym*	lookup(char*, int);
void	lput(long);
void	lputbe(long);
void	mkfwd(void);
void*	mysbrk(ulong);
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
vlong	regoff(Adr*);
long	regused(Prog*);
int	relinv(int);
long	rnd(long, long);
void	sched(Prog*, Prog*);
void	span(void);
void	vlput(vlong);
void	undef(void);
void	xdefine(char*, int, long);
void	xfol(Prog*);

#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"../vc/v.out.h"

typedef	struct	Adr	Adr;
typedef	struct	Sym	Sym;
typedef	struct	Autom	Auto;
typedef	struct	Prog	Prog;
typedef	struct	Optab	Optab;
typedef	struct	Oprang	Oprang;
typedef	uchar	Opcross[32][2][32];

#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))
#define	P		((Prog*)0)
#define	S		((Sym*)0)
#define	TNAME		(curtext?curtext->from.sym->name:noname)

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
	char	as;
	char	reg;
};
struct	Sym
{
	char	name[NNAME];
	short	type;
	short	version;
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
	STEXT		= 1,
	SDATA,
	SBSS,
	SDATA1,
	SXREF,
	SLEAF,
	SFILE,

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

	FOLL		= 1<<0,

	LABEL		= 1<<0,
	LEAF		= 1<<1,
	ACTIVE1		= 1<<2,
	ACTIVE2		= 1<<3,
	BRANCH		= 1<<4,
	LOAD		= 1<<5,
	COMPARE		= 1<<6,
	MFROM		= 1<<7,

	BIG		= 32766,
	STRINGSZ	= 200,
	NHASH		= 10007,
	NHUNK		= 100000,
	MINSIZ		= 64,
	NENT		= 100,
};

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
int	nerrors;
long	nhunk;
long	offset;
Opcross	opcross[7];
Oprang	oprange[AEND];
char*	outfile;
long	pc;
uchar	repop[AEND];
long	symsize;
Prog*	textp;
long	textsize;
long	thunk;
int	version;
char	xcmp[32][32];
Prog	zprg;

extern	char*	anames[];
extern	Optab	optab[];

int	Aconv(void*, Fconv*);
int	Dconv(void*, Fconv*);
int	Nconv(void*, Fconv*);
int	Pconv(void*, Fconv*);
int	Sconv(void*, Fconv*);
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
int	conflict(long, long);
double	cputime(void);
void	datblk(long, long);
int	depend(long, long);
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
long	regused(Prog*);
int	relinv(int);
long	rnd(long, long);
void	sched(Prog*, Prog*);
void	span(void);
void	undef(void);
void	xdefine(char*, int, long);
void	xfol(Prog*);
void	xfol(Prog*);

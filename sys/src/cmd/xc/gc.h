#include	"../cc/cc.h"
#include	"../xc/x.out.h"

#define	TINT		TLONG
#define	TFIELD		TLONG
#define	SZ_CHAR		1
#define	SZ_SHORT	2
#define	SZ_LONG		4
#define	SZ_VLONG	4
#define	SZ_IND		4
#define	SZ_FLOAT	4
#define	SZ_DOUBLE	4
#define	SU_ALLIGN	SZ_LONG
#define	SU_PAD		SZ_LONG
#define	FNX		100
#define	INLINE		(5*SZ_LONG)

typedef	struct	Adr	Adr;
typedef	struct	Prog	Prog;
typedef	struct	Case	Case;
typedef	struct	C1	C1;
typedef	struct	Multab	Multab;
typedef	struct	Hintab	Hintab;
typedef	struct	Bits	Bits;
typedef	struct	Var	Var;
typedef	struct	Reg	Reg;
typedef	struct	Rgn	Rgn;


struct	Adr
{
	union
	{
		long	offset;
		double	dval;
		char	sval[NSNAME];
	};
	Sym*	sym;
	char	type;
	char	reg;
	char	name;
	char	etype;
};
#define	A	((Adr*)0)

#define	ADDABLE	9	/* borderline of addressable trees */
struct	Prog
{
	Adr	from;
	Adr	from1;		/* second argument for fadd, fmul, ... */
	Adr	from2;		/* third argument for fmadd, fmsub, ... */
	Adr	to;
	Prog*	link;
	ulong	lineno;
	uchar	as;
	uchar	isf;		/* floating op? */
	char	reg;
	char	cc;
};
#define	P	((Prog*)0)

struct	Case
{
	Case*	link;
	long	val;
	long	label;
	char	def;
};
#define	C	((Case*)0)

struct	C1
{
	long	val;
	long	label;
};

struct	Multab
{
	long	val;
	char	code[20];
};

struct	Hintab
{
	ushort	val;
	char	hint[10];
};

#define	BITS	5
#define	NVAR	(BITS*sizeof(ulong)*8)
struct	Bits
{
	ulong	b[BITS];
};

struct	Var
{
	long	offset;
	Sym*	sym;
	char	name;
	char	etype;
};

struct	Reg
{
	long	pc;

	Bits	set;
	Bits	use1;
	Bits	use2;

	Bits	refbehind;
	Bits	refahead;
	Bits	calbehind;
	Bits	calahead;
	Bits	regdiff;
	Bits	act;

	long	regu;
	long	loop;		/* could be shorter */

	union	{
		Reg*	log5;
		int	active;
	};
	Reg*	p1;
	Reg*	p2;
	Reg*	p2link;
	Reg*	s1;
	Reg*	s2;
	Reg*	link;
	Prog*	prog;
};
#define	R	((Reg*)0)

#define	NRGN	600
struct	Rgn
{
	Reg*	enter;
	short	cost;			/* savings if kept in a reg */
	short	costa;			/* savings if a kept as a pointer */
	short	varno;
	short	regno;
	short	isa;			/* true if regno is a pointer */
};

long	breakpc;
Case*	cases;
Node	constnode;
Node	fconstnode;
long	continpc;
long	curarg;
long	cursafe;
Prog*	firstp;
Prog*	lastp;
long	maxargsafe;
int	mnstring;
int	retok;
Node*	nodrat;
Node*	nodret;
Node*	nodsafe;
long	nrathole;
long	nstring;
Prog*	p;
long	pc;
Node	regnode;
char	string[NSNAME];
Sym*	symrathole;
Node	znode;
Prog	zprog;
char	reg[NREG+NREG];
long	exregoffset;
long	exfregoffset;
int	multabsize;

#define	BLOAD(r)	band(bnot(r->refbehind), r->refahead)
#define	BSTORE(r)	band(bnot(r->calbehind), r->calahead)
#define	LOAD(r,z)	(~r->refbehind.b[z] & r->refahead.b[z])
#define	STORE(r,z)	(~r->calbehind.b[z] & r->calahead.b[z])

#define	bset(a,n)	((a).b[(n)/32]&(1L<<(n)%32))
#define fpreg(r)	((r) <= 14)

#define	CLOAD	5		/* cost to load */
#define	CREF	5		/* cost to reference */
#define	CLOADA	2		/* cost to calc addr */
#define	CREFA	3		/* cost to reference an addr */
#define	CINF	1000
#define	LOOP	3

Rgn	region[NRGN];
Rgn*	rgp;
int	nregion;
int	nvar;

Bits	externs;
Bits	params;
Bits	consts;
Bits	addrs;
Bits	zbits;

long	regbits;

int	change;
int	changea;

Reg*	firstr;
Reg*	lastr;
Reg	zreg;
Reg*	freer;
Var	var[NVAR];
int	vinreg;
int	regwithv;

extern	char*	anames[];
extern	char*	ccnames[];
extern	Multab	multab[];
extern	Hintab	hintab[];
extern	int	hintabsize;

/*
 * sgen.c
 */
void	codgen(Node*, Node*);
void	gen(Node*);
void	noretval(int);
void	xcom(Node*);
void	bcomplex(Node*);

/*
 * cgen.c
 */
void	cgen(Node*, Node*);
void	reglcgen(Node*, Node*, Node*);
void	lcgen(Node*, Node*);
void	bcgen(Node*, int);
void	boolgen(Node*, int, Node*);
void	sugen(Node*, Node*, long);
void	layout(Node*, Node*, int, int, Node*);

/*
 * txt.c
 */
void	ginit(void);
void	gclean(void);
void	nextpc(void);
void	gargs(Node*, Node*, Node*);
void	garg1(Node*, Node*, Node*, int, Node**, int);
Node*	nodconst(long);
Node*	nodfconst(double);
void	nodreg(Node*, Node*, int);
void	regret(Node*, Node*);
void	regalloc(Node*, Node*, Node*);
void	regfree(Node*);
void	regrelease(Node*);
void	regrealloc(Node*);
void	regialloc(Node*, Node*, Node*);
void	regsalloc(Node*, Node*);
void	regaalloc1(Node*, Node*);
void	regaalloc(Node*, Node*);
void	regind(Node*, Node*);
void	regaddr(Node*, Node*);
void	adjsp(long);
void	gprep(Node*, Node*);
void	raddr(Node*, Prog*);
void	naddr(Node*, Adr*);
int	gaddr(Node*, Node*);
void	gmove(Node*, Node*);
void	gins(int, Node*, Node*);
void	ginsf(int, Node*, Node*, Node*, Node*);
void	gopcode(int, Node*, Node*, Node*);
int	samaddr(Node*, Node*);
void	gbranch(int);
void	patch(Prog*, long);
int	sconst(Node*);
int	sval(long);
void	gpseudo(int, Sym*, Node*);

/*
 * swt.c
 */
int	swcmp(void*, void*);
void	doswit(Node*);
void	swit1(C1*, int, long, Node*, Node*);
void	cas(void);
void	bitload(Node*, Node*, Node*, Node*, Node*);
void	bitstore(Node*, Node*, Node*, Node*, Node*);
long	outstring(char*, long);
int	vlog(Node*);
int	mulcon(Node*, Node*);
int	asmulcon(Node*, Node*);
Multab*	mulcon0(long);
void	mulcon1(Multab*, long, Node*);
void	nullwarn(Node*, Node*);
void	sextern(Sym*, Node*, long, long);
void	gextern(Sym*, Node*, long, long);
void	outcode(void);
Dsp	dtodsp(double);

/*
 * list
 */
void	listinit(void);
int	Pconv(void*, Fconv*);
int	Aconv(void*, Fconv*);
int	Dconv(void*, Fconv*);
int	Sconv(void*, Fconv*);
int	Nconv(void*, Fconv*);
int	Bconv(void*, Fconv*);
int	Jconv(void*, Fconv*);

/*
 * reg.c
 */
Reg*	rega(void);
int	rcmp(void*, void*);
void	regopt(Prog*);
Bits	mkvar(Adr*, int);
void	prop(Reg*, Bits, Bits);
int	loopit(Reg*);
void	synch(Reg*, Bits);
ulong	allreg(ulong, Rgn*);
void	paint1(Reg*, int);
ulong	paint2(Reg*, int);
void	paint3(Reg*, int, long, int, int);
void	addreg(Adr*, int);
void	addref(Adr*, int);
void	addmove(Reg*, int, int, int, int);
void	regbefore(Reg*);
void	regafter(Reg*);
void	newreg(Reg*);

long	RtoB(int);
long	FtoB(int);
long	AtoB(int);
int	BtoR(long);
int	BtoF(long);
int	BtoA(long);

/*
 * peep.c
 */
enum
{
	MAXLOOP	= 128/4,	/* conservative max length of do loop */
	MINBLOCK = 6,		/* min length block to lock */
	MAXBLOCK = 16,		/* max length block to lock; for the mp3210 board, 8μsec */
};

void	peep(void);
void	excise(Reg*);
Reg*	uniqp(Reg*);
Reg*	uniqs(Reg*);
int	blocklen(Reg*);
int	regtyp(Adr*);
int	contyp(Adr*);
int	regzer(Adr*);
int	memref(Adr*);
int	indreg(Adr*);
int	isadj(Prog*);
int	unaliased(Adr*);
int	a2type(Prog*);
Adr	*mkfreg(Adr*, int);
Adr	*mkreg(Adr*, int);
Adr	*mkconst(Adr*, long);

int	copyas(Adr*, Adr*);
int	copyau(Adr*, Adr*);
int	copyau1(Prog*, Adr*);
int	copysub(Adr*, Adr*, Adr*, int);
int	copysubf(Adr*, Adr*, Adr*, int);
int	copysub1(Prog*, Adr*, Adr*, int);
int	copysubind(Adr*, Adr*, Adr*, int);

Reg	*insertlock(Reg*);
Reg	*findused(Reg*, Adr*);
Reg	*findusedb(Reg*, Adr*);
int	notused(Reg*, Adr*);
int	notusedbra(Reg*, Adr*);
int	notused1(Reg*, Adr*);
int	setbetween(Reg*, Reg*, Adr*);
void	addmovea(Reg*, Adr*);
Reg	*addjmp(Reg*, int, int, Reg*);
Reg	*adddbra(Reg*, Adr*, Reg*);
Reg	*addinst(Reg*, int, Adr*, int, Adr*);
Reg	*insertinst(Reg*, int, Adr*, int, Adr*);
void	adddo(int, Reg*, Reg*);
void	addlock(Reg*, Reg*);

/*
 * copies.c
 */
int	subprop(Reg*);
int	copyprop(Reg*);
int	copypropind(Reg*);
int	copy1(Adr*, Adr*, Reg*, int, int);
int	copyu(Prog*, Adr*, Adr*, int);
int	copyuind(Prog*, Adr*, Adr*);

/*
 * opt.c
 */
int	forward(Reg *r);
void	mergemuladd(Reg*);
void	mergestore(Reg*);
int	propcopies(void);
void	killrmove(Reg*);
void	killadj(Reg*);
void	mergeinc(Reg*, Adr*, int);
void	addprop(Reg*);
int	subreg(Reg*, Reg*, Adr*, Adr *);
int	findtst(Reg*, Prog*, int, int);
int	setcc(Prog*, Prog*, int);
int	compat(Adr*, Adr*);
int	consetscc(Adr*);

/*
 * loop.c
 */
void	optloop(Reg*);
int	onlybra(Reg*);
Reg	*findentry(Reg*);
Reg	*findcmp(Reg*);
Reg	*indvarinc(Reg*, Reg*, Adr*);
int	loopinvar(Reg*, Adr*);
int	isfor(Reg*, Reg*, Adr*);

/*
 * bits.c
 */
Bits	bor(Bits, Bits);
Bits	band(Bits, Bits);
Bits	bnot(Bits);
int	bany(Bits*);
int	bnum(Bits);
Bits	blsh(unsigned);
int	beq(Bits, Bits);

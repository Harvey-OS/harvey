#include	"../cc/cc.h"
#include	"../2c/2.out.h"

#define	TINT		TLONG
#define	TFIELD		TLONG
#define	SZ_CHAR		1
#define	SZ_SHORT	2
#define	SZ_LONG		4
#define	SZ_VLONG	8
#define	SZ_IND		4
#define	SZ_FLOAT	4
#define	SZ_DOUBLE	8
#define	SU_ALLIGN	SZ_SHORT
#define	SU_PAD		SZ_LONG

#define	ALLOP	90
#define	NRGN	300
#define	FNX	100
#define	INDEXED	9

#define	PRE	1
#define	POST	2
#define	TEST	4

typedef	struct	Adr	Adr;
typedef	struct	Prog	Prog;
typedef	struct	Txt	Txt;
typedef	struct	Cases	Case;
typedef	struct	Reg	Reg;
typedef	struct	Rgn	Rgn;
typedef	struct	Bits	Bits;
typedef	struct	Var	Var;
typedef	struct	Multab	Multab;
typedef	struct	C1	C1;

struct
{
	Node*	regtree;
	Node*	basetree;
	short	scale;
} idx;

struct	Adr
{
	union
	{
		struct
		{
			long	displace;
			long	offset;
		};
		char	sval[NSNAME];
		double	dval;
	};
	Sym*	sym;
	short	type;
	short	index;
	short	scale;
	short	field;
	short	etype;
};
#define	A	((Adr*)0)

struct	Prog
{
	Adr	from;
	Adr	to;
	Prog*	link;
	long	lineno;
	short	as;
};
#define	P	((Prog*)0)

struct	Txt
{
	short	movas;
	short	postext;
	char	preclr;
};

struct	Cases
{
	long	val;
	long	label;
	uchar	def;
	Case*	link;
};
#define	C	((Case*)0)

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
	char	type;
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

	ulong	regu;
	long	loop;		/* could be shorter */

	union
	{
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

struct	Rgn
{
	Reg*	enter;
	short	costr;
	short	costa;
	short	varno;
	short	regno;
};

struct	Multab
{
	short	val;
	char	code[6];
};

struct	C1
{
	long	val;
	long	label;
};

enum
{
	OTST = OEND+1,
	OINDEX,
	OFAS,

	OXEND
};

#define	BLOAD(r)	band(bnot(r->refbehind), r->refahead)
#define	BSTORE(r)	band(bnot(r->calbehind), r->calahead)
#define	LOAD(r)		(~r->refbehind.b[z] & r->refahead.b[z])
#define	STORE(r)	(~r->calbehind.b[z] & r->calahead.b[z])

#define	bset(a,n)	((a).b[(n)/32]&(1L<<(n)%32))

#define	CLOAD	8
#define	CREF	5
#define	CTEST	2
#define	CXREF	3
#define	CINF	1000
#define	LOOP	3

Bits	externs;
Bits	params;
Bits	addrs;
Bits	zbits;
ulong	regbits;

#define	B_INDIR	(1<<0)
#define	B_ADDR	(1<<1)
int	mvbits;
int	changer;
int	changea;

Txt	txt[NTYPE][NTYPE];
short	opxt[ALLOP][NTYPE];
Txt*	txtp;
int	multabsize;

Reg*	firstr;
Reg*	lastr;
Reg	zreg;
Reg*	freer;

long	argoff;
long	breakpc;
Case*	cases;
long	continpc;
Prog*	firstp;
Reg*	firstr;
int	inargs;
Prog*	lastp;
int	retok;
long	mnstring;
Node*	nodrat;
Node*	nodret;
long	nrathole;
long	nstatic;
int	nregion;
long	nstring;
int	nvar;
Prog*	p;
long	pc;
Rgn	region[NRGN];
Rgn*	rgp;
char	string[NSNAME];
Sym*	symrathole;
Sym*	symstatic;
Var	var[NVAR];
Prog	zprog;

uchar	regused[NREG];
uchar	aregused[NREG];
uchar	fregused[NREG];
uchar	regbase[I_MASK];
long	exregoffset;
long	exaregoffset;
long	exfregoffset;
extern	char*	anames[];
extern	Multab	multab[];

void	cgen(Node*, int, Node*);
void	lcgen(Node*, int, Node*);
void	bcgen(Node*, int);
void	boolgen(Node*, int, int, Node*, Node*);
void	sugen(Node*, int, Node*, long);


void	listinit(void);
int	Bconv(void*, Fconv*);
int	Pconv(void*, Fconv*);
int	Aconv(void*, Fconv*);
int	Xconv(void*, Fconv*);
int	Dconv(void*, Fconv*);
int	Rconv(void*, Fconv*);
int	Sconv(void*, Fconv*);

void	peep(void);
void	excise(Reg*);
Reg*	uniqp(Reg*);
Reg*	uniqs(Reg*);
int	findtst(Reg*, Prog*, int);
int	setcc(Prog*, Prog*);
int	compat(Adr*, Adr*);
int	aregind(Adr*);
int	asize(int);
int	usedin(int, Adr*);
Reg*	findccr(Reg*);
int	setccr(Prog*);
Reg*	findop(Reg*, int, int, int);
int	regtyp(int);
int	anyvar(Adr*);
int	subprop(Reg*);
int	copyprop(Reg*);
int	copy1(Adr*, Adr*, Reg*, int);
int	copyu(Prog*, Adr*, Adr*);
int	copyas(Adr*, Adr*);
int	tasas(Adr*, Adr*);
int	copyau(Adr*, Adr*);
int	copysub(Adr*, Adr*, Adr*, Prog*, int);

ulong	RtoB(int);
ulong	AtoB(int);
ulong	FtoB(int);
int	BtoR(ulong);
int	BtoA(ulong);
int	BtoF(ulong);

Reg*	rega(void);
int	rcmp(void*, void*);
void	regopt(Prog*);
void	addmove(Reg*, int, int, int);
Bits	mkvar(Adr*, int);
void	prop(Reg*, Bits, Bits);
int	loopit(Reg*);
void	synch(Reg*, Bits);
ulong	allreg(ulong, Rgn*);
void	paint1(Reg*, int);
ulong	paint2(Reg*, int);
void	paint3(Reg*, int, ulong, int);
void	addreg(Adr*, int);

void	codgen(Node*, Node*);
void	gen(Node*);
void	noretval(int);
Node*	nodconst(long);

int	swcmp(void*, void*);
void	doswit(int, Node*);
void	swit1(C1*, int, long, int, Node*);
void	cas(void);
int	bitload(Node*, int, int, int, Node*);
void	bitstore(Node*, int, int, int, int, Node*);
long	outstring(char*, long);
int	doinc(Node*, int);
void	setsp(void);
void	adjsp(long);
int	simplv(Node*);
int	eval(Node*, int);
int	vlog(Node*);
void	outcode(void);
void	ieeedtod(Ieee*, double);
int	nodalloc(Type*, int, Node*);
int	mulcon(Node*, Node*, int, Node*);
int	shlcon(Node*, Node*, int, Node*);
int	mulcon1(Node*, long, int, Node*);
void	nullwarn(Node*, Node*);

void	tindex(Type*, Type*);
void	ginit(void);
void	gclean(void);
void	oinit(int, int, int, int, int, int);
Prog*	prg(void);
void	nextpc(void);
void	gargs(Node*);
void	naddr(Node*, Adr*, int);
int	regalloc(Type*, int);
int	regaddr(int);
int	regpair(int);
int	regret(Type*);
void	regfree(int);
void	gmove(Type*, Type*, int, Node*, int, Node*);
void	gopcode(int, Type*, int, Node*, int, Node*);
void	asopt(void);
int	relindex(int);
void	gbranch(int);
void	fpbranch(void);
void	patch(Prog*, long);
void	gpseudo(int, Sym*, int, long);

void	indx(Node*);
void	bcomplex(Node*);

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

/*
 * com64
 */
int	com64(Node*);
void	com64init(void);
void	bool64(Node*);

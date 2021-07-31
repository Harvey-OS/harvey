#include	"../cc/cc.h"
#include	"../2c/2.out.h"
/*
 * 2c/68020
 * Motorola 68020
 */

#define	SZ_CHAR		1
#define	SZ_SHORT	2
#define	SZ_INT		4
#define	SZ_LONG		4
#define	SZ_IND		4
#define	SZ_FLOAT	4
#define	SZ_VLONG	8
#define	SZ_DOUBLE	8

#define	ALLOP	OEND
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
typedef	struct	Var	Var;
typedef	struct	Multab	Multab;
typedef	struct	C1	C1;
typedef	struct	Index	Index;

struct Index
{
	int	o0;
	int	o1;
};

EXTERN	struct
{
	Node*	regtree;
	Node*	basetree;
	short	scale;
} idx;

struct	Adr
{
	long	displace;
	long	offset;

	char	sval[NSNAME];
	double	dval;

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
	long	rpo;		/* reverse post ordering */

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

	Reg*	log5;
	long	active;

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

EXTERN	Bits	externs;
EXTERN	Bits	params;
EXTERN	Bits	addrs;
EXTERN	ulong	regbits;

#define	B_INDIR	(1<<0)
#define	B_ADDR	(1<<1)
EXTERN	int	mvbits;
EXTERN	int	changer;
EXTERN	int	changea;

EXTERN	Txt	txt[NTYPE][NTYPE];
EXTERN	short	opxt[ALLOP][NTYPE];
EXTERN	Txt*	txtp;
EXTERN	int	multabsize;

EXTERN	Reg*	firstr;
EXTERN	Reg*	lastr;
EXTERN	Reg	zreg;
EXTERN	Reg*	freer;

EXTERN	long	argoff;
EXTERN	long	breakpc;
EXTERN	Case*	cases;
EXTERN	long	continpc;
EXTERN	Prog*	firstp;
EXTERN	Reg*	firstr;
EXTERN	int	inargs;
EXTERN	Prog*	lastp;
EXTERN	int	retok;
EXTERN	long	mnstring;
EXTERN	Node*	nodrat;
EXTERN	Node*	nodret;
EXTERN	long	nrathole;
EXTERN	long	nstatic;
EXTERN	int	nregion;
EXTERN	long	nstring;
EXTERN	int	nvar;
EXTERN	Prog*	p;
EXTERN	long	pc;
EXTERN	Rgn	region[NRGN];
EXTERN	Rgn*	rgp;
EXTERN	char	string[NSNAME];
EXTERN	Sym*	symrathole;
EXTERN	Sym*	symstatic;
EXTERN	Var	var[NVAR];
EXTERN	long*	idom;
EXTERN	Reg**	rpo2r;
EXTERN	long	maxnr;
EXTERN	Prog	zprog;

EXTERN	uchar	regused[NREG];
EXTERN	uchar	aregused[NREG];
EXTERN	uchar	fregused[NREG];
EXTERN	uchar	regbase[I_MASK];
EXTERN	long	exregoffset;
EXTERN	long	exaregoffset;
EXTERN	long	exfregoffset;

extern	char*	anames[];
extern	Multab	multab[];

void	cgen(Node*, int, Node*);
void	lcgen(Node*, int, Node*);
void	bcgen(Node*, int);
void	boolgen(Node*, int, int, Node*, Node*);
void	sugen(Node*, int, Node*, long);


void	listinit(void);
int	Bconv(Fmt*);
int	Pconv(Fmt*);
int	Aconv(Fmt*);
int	Xconv(Fmt*);
int	Dconv(Fmt*);
int	Rconv(Fmt*);
int	Sconv(Fmt*);

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
int	rcmp(const void*, const void*);
void	regopt(Prog*);
void	addmove(Reg*, int, int, int);
Bits	mkvar(Adr*, int);
void	prop(Reg*, Bits, Bits);
void	loopit(Reg*, long);
void	synch(Reg*, Bits);
ulong	allreg(ulong, Rgn*);
void	paint1(Reg*, int);
ulong	paint2(Reg*, int);
void	paint3(Reg*, int, ulong, int);
void	addreg(Adr*, int);

void	codgen(Node*, Node*);
void	gen(Node*);
void	usedset(Node*, int);
void	noretval(int);
Node*	nodconst(long);

int	swcmp(const void*, const void*);
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
void	gpseudotree(int, Sym*, Node*);

void	indx(Node*);
void	bcomplex(Node*);

/*
 * com64
 */
int	com64(Node*);
void	com64init(void);
void	bool64(Node*);

#pragma	varargck	type	"A"	int
#pragma	varargck	type	"B"	Bits
#pragma	varargck	type	"D"	Adr*
#pragma	varargck	type	"N"	Adr*
#pragma	varargck	type	"P"	Prog*
#pragma	varargck	type	"S"	char*
#pragma	varargck	type	"R"	int
#pragma	varargck	type	"X"	Index

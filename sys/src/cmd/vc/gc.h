#include	"../cc/cc.h"
#include	"../vc/v.out.h"

#define	TINT		TLONG
#define	TFIELD		TLONG
#define	SZ_CHAR		1
#define	SZ_SHORT	2
#define	SZ_LONG		4
#define	SZ_VLONG	8
#define	SZ_IND		4
#define	SZ_FLOAT	4
#define	SZ_DOUBLE	8
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
		Ieee	ieee;
	};
	Sym*	sym;
	char	type;
	char	reg;
	char	name;
	char	etype;
};
#define	A	((Adr*)0)

#define	INDEXED	9
struct	Prog
{
	Adr	from;
	Adr	to;
	Prog*	link;
	long	lineno;
	char	as;
	char	reg;
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

#define	BITS	6
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
	short	cost;
	short	varno;
	short	regno;
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
Multab	multab[20];
int	retok;
int	hintabsize;
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

#define	BLOAD(r)	band(bnot(r->refbehind), r->refahead)
#define	BSTORE(r)	band(bnot(r->calbehind), r->calahead)
#define	LOAD(r)		(~r->refbehind.b[z] & r->refahead.b[z])
#define	STORE(r)	(~r->calbehind.b[z] & r->calahead.b[z])

#define	bset(a,n)	((a).b[(n)/32]&(1L<<(n)%32))

#define	CLOAD	4
#define	CREF	5
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
long	exregbits;

int	change;

Reg*	firstr;
Reg*	lastr;
Reg	zreg;
Reg*	freer;
Var	var[NVAR];

extern	char*	anames[];
extern	Hintab	hintab[];

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
void	garg1(Node*, Node*, Node*, int, Node**);
Node*	nodconst(long);
Node*	nod32const(vlong);
Node*	nodfconst(double);
void	nodreg(Node*, Node*, int);
void	regret(Node*, Node*);
void	regalloc(Node*, Node*, Node*);
void	regfree(Node*);
void	regialloc(Node*, Node*, Node*);
void	regsalloc(Node*, Node*);
void	regaalloc1(Node*, Node*);
void	regaalloc(Node*, Node*);
void	regind(Node*, Node*);
void	gprep(Node*, Node*);
void	raddr(Node*, Prog*);
void	naddr(Node*, Adr*);
void	gmove(Node*, Node*);
void	gins(int a, Node*, Node*);
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
Multab*	mulcon0(long);
void	nullwarn(Node*, Node*);
void	sextern(Sym*, Node*, long, long);
void	gextern(Sym*, Node*, long, long);
void	outcode(void);
void	ieeedtod(Ieee*, double);

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

/*
 * reg.c
 */
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
void	paint3(Reg*, int, long, int);
void	addreg(Adr*, int);

/*
 * peep.c
 */
void	peep(void);
void	excise(Reg*);
Reg*	uniqp(Reg*);
Reg*	uniqs(Reg*);
int	regtyp(Adr*);
int	regzer(Adr*);
int	anyvar(Adr*);
int	subprop(Reg*);
int	copyprop(Reg*);
int	copy1(Adr*, Adr*, Reg*, int);
int	copyu(Prog*, Adr*, Adr*);

int	copyas(Adr*, Adr*);
int	copyau(Adr*, Adr*);
int	copyau1(Prog*, Adr*);
int	copysub(Adr*, Adr*, Adr*, int);
int	copysub1(Prog*, Adr*, Adr*, int);

long	RtoB(int);
long	FtoB(int);
int	BtoR(long);
int	BtoF(long);

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

typedef	struct	Reg	Reg;
typedef	struct	Rgn	Rgn;
typedef	struct	Bits	Bits;
typedef	struct	Var	Var;

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
Reg*	firstr;
Reg*	lastr;
Reg	zreg;
Reg*	freer;
Rgn	region[NRGN];
Rgn*	rgp;
Var	var[NVAR];

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

/*
 * bits.c
 */
Bits	bor(Bits, Bits);
Bits	band(Bits, Bits);
Bits	bnot(Bits);
int	bany(Bits*);
int	bnum(Bits);
Bits	blsh(int);
int	beq(Bits, Bits);

/*
 * Machine dependant mips
 */

#include "/sys/src/cmd/vc/v.out.h"
#define AOUTL	".v"
#define IALEF	"-I/sys/include/alef"

enum
{
	/* Basic machine type size in bytes */
	Machptr 		= 4,
	Machint 		= 4,
	Machsint		= 2,
	Machchar 		= 1,
	Machchan 		= 4,
	Machfloat 		= 8,

	Autobase		= 4,	/* Automatics start at Autobase(FP) */
	Parambase		= 0,	/* Parameters start at Parambase(SP) */
	Argbase			= 4,	/* Space for return pc/frame info */

	Shortfoff		= 2,
	Charfoff		= 3,

	/* Type alignments for structs and data */
	Align_Machptr 		= 4,
	Align_Machint 		= 4,
	Align_Machsint 		= 2,
	Align_Machchar 		= 1,
	Align_Machchan 		= 4,
	Align_Machfloat 	= 4,
	Align_data 		= Align_Machint,

	Sucall			= 100,	/* Complexity of a function */
	Sucompute		= 9,	/* >sun Address needs to be computed */
};

#define isaddr(nr)	(nr->islval > Sucompute)

typedef struct Adres Adres;
typedef struct Inst Inst;
typedef struct Glab Glab;
typedef	struct Bits Bits;
typedef	struct Var Var;
typedef	struct Reg Reg;
typedef	struct Rgn Rgn;
typedef struct Scache Scache;

enum
{
	A_NONE	= 0,
	A_CONST,
	A_FCONST,
	A_REG,
	A_FREG,
	A_INDREG,
	A_BRANCH,
	A_STRING,
	A_LO,
	A_HI,
};

struct Scache
{
	Sym	*s;
	char	class;
};

struct Glab
{
	Node	*n;
	int	par;
	int	crit;
	union {
		Inst	*i;
		ulong	pc;
	};
	Glab	*next;
};

struct Adres
{
	char	type;
	char	class;
	char	etype;
	char	reg;
	Sym	*sym;
	union {
		ulong	ival;
		float	fval;
		char	str[NSNAME];
	};
};
#define A	((Adres*)0)

struct Inst
{
	char	op;
	char	fmt;
	Adres	src1;
	Adres	dst;
	int	reg;
	Node	*n;
	ulong	pc;
	int 	lineno;
	Inst	*next;
};
#define P	((Inst*)0)

enum
{
	Retireg		= 1,		/* Integer return */
	Regspass	= 1,		/* Complex ptr */
	Ireg		= 1,		/* Base integer registers */
	Maxireg		= 20,
	Retfreg		= 21,
	Freg		= 21,
	Maxfreg		= 21+31,
	Hireg		= 21+31+1,
	Loreg		= 21+31+2,
	Retfregno	= 0,
	Retiregno	= 1,
	Nreg		= 55,
	RegSP		= 29,
	Pregs		= 25,		/* Private registers */
};

#define	BITS	5
#define	NVAR	(BITS*sizeof(ulong)*8)
struct	Bits
{
	ulong	b[BITS];
};

struct	Var
{
	long	ival;
	Sym*	sym;
	char	class;
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
	Reg*	p2next;
	Reg*	s1;
	Reg*	s2;
	Reg*	next;
	Inst*	prog;
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

#define	BLOAD(r)	band(bnot(r->refbehind), r->refahead)
#define	BSTORE(r)	band(bnot(r->calbehind), r->calahead)
#define	LOAD(r)		(~r->refbehind.b[z] & r->refahead.b[z])
#define	STORE(r)	(~r->calbehind.b[z] & r->calahead.b[z])

#define	bset(a,n)	((a).b[(n)/32]&(1L<<(n)%32))

#define	CLOAD	5
#define	CREF	5
#define	CINF	1000
#define	LOOP	3

Node	ratv;
Node	*atv;
int	frsize;
ulong 	args;

Rgn	region[NRGN];
Rgn*	rgp;
int	nregion;
int	nvar;

Bits	externs;
Bits	param;
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

Inst	zprog;
ulong	pc;

Scache	scache[NSYM];

/* Code generator and machine specific optimisation */
Inst		*ai(void);
void		mkaddr(Node*, Adres*, int);
void		reg(Node*, Type*, Node*);
Node		*regtmp(void);
Node		*regn(int);
void		regfree(Node*);
void		preamble(Node*);
void		sucalc(Node*);
void		scopeis(int);
void		regret(Node*, Type*);
void		genarg(Node*);
void		codmop(Node*, Node*, Node*, Node*);
Inst		*instruction(int, Node*, Node*, Node*);
void		label(Inst*, ulong);
void		gencond(Node*, Node*, int);
int		immed(Node*);
void		invertcond(Node*);
void		codcond(int, Node*, Node*, Node*);
void		genexp(Node*, Node*);
void		ilink(Inst*);
int		vconst(Node*);
void		assign(Node*, Node*);
void		oasgn(Node*, Node*);
Node		*argnode(Type*);
void		orecv(Node*, Node*);
void		switchcode(Node*);
void		gencmps(Node**, int, long, Node*);
void		regcheck(void);
void		setlabel(Node*, ulong);
void		setgoto(Node*, Inst*);
void		resolvegoto(void);
void		gencomplex(Node*, Node*);
void		parcode(Node*);
Node		*stknode(Type*);
void		mkdata(Node*, int, int, Inst*);
Reg*		rega(void);
int		rcmp(void*, void*);
void		regopt(Inst*);
void		addmove(Reg*, int, int, int);
Bits		mkvar(Adres*, int);
void		prop(Reg*, Bits, Bits);
int		loopit(Reg*);
void		synch(Reg*, Bits);
ulong		allreg(ulong, Rgn*);
void		paint1(Reg*, int);
ulong		paint2(Reg*, int);
void		paint3(Reg*, int, long, int);
void		addreg(Adres*, int);
void		peep(void);
void		excise(Reg*);
Reg*		uniqp(Reg*);
Reg*		uniqs(Reg*);
int		regtyp(Adres*);
int		regzer(Adres*);
int		anyvar(Adres*);
int		subprop(Reg*);
int		copyprop(Reg*);
int		copy1(Adres*, Adres*, Reg*, int);
int		copyu(Inst*, Adres*, Adres*);
int		copyas(Adres*, Adres*);
int		copyau(Adres*, Adres*);
int		copyau1(Inst*, Adres*);
int		copysub(Adres*, Adres*, Adres*, int);
int		copysub1(Inst*, Adres*, Adres*, int);
long		RtoB(int);
long		FtoB(int);
int		BtoR(long);
int		BtoF(long);
Bits		bor(Bits, Bits);
Bits		band(Bits, Bits);
Bits		bnot(Bits);
int		bany(Bits*);
int		bnum(Bits);
Bits		blsh(int);
int		beq(Bits, Bits);
int		bitno(long);
int		Bconv(void*, Fconv*);
void		ieeedtod(Ieee*, double);
int		vcache(Biobuf*, Adres*);
char		*vaddr(char*, Adres*, int);
void		vname(Biobuf*, char, char*, int);
void		cominit(Node*, Type*, Node*, int);
Node*		internnode(Type*);
void		lblock(Node*);

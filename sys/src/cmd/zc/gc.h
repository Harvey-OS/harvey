#include "../cc/cc.h"
#include "../zc/z.out.h"

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

typedef	struct	Prog	Prog;
typedef	struct	Adr	Adr;
typedef	struct	Cases	Case;
typedef	struct	C1	C1;

struct	Adr
{
	union
	{
		long	offset;
		double	dval;
		char	sval[NSNAME];
	};
	Sym*	sym;
	short	type;
	short	width;
};
#define	A	((Adr*)0)

#define	PRE	1
#define	APOST	2
#define	BPOST	4

#define	ADDABLE	10
#define	INDEXED	9

struct	Prog
{
	int	as;
	long	lineno;
	Adr	from;
	Adr	to;
	Prog*	link;
};
#define	P	((Prog*)0)

typedef
struct	Cases
{
	long	val;
	long	label;
	int	def;
	Case*	link;
};
#define	C	((Case*)0)

typedef
struct	C1
{
	long	val;
	long	label;
};

extern	char*	anames[];
extern	char*	wnames[];

char	string[NSNAME];
int	curreg;
int	maxreg;
int	cursafe;
int	maxsafe;
long	pc;
Prog*	p;
Node*	nodret;
Node	znode;
Case*	cases;
long	breakpc;
long	continpc;
long	nstring;
int	mnstring;
int	retok;
Prog*	firstp;
Prog*	lastp;
Prog	zprog;
Node	regnode;
Node	constnode;
Node	fconstnode;
Node*	nodsafe;
long	nrathole;
Node*	nodrat;
Sym*	symrathole;

/*
 * cgen.c
 */
void	cgen(Node*, Node*);
void	lcgen(Node*, Node*);
void	bcgen(Node*, int);
void	boolgen(Node*, int, Node*);
void	sugen(Node*, Node*, long);

/*
 * sgen.c
 */
void	codgen(Node*, Node*);
void	gen(Node*);
int	retalias(Node*);
void	xcom(Node*);
void	bcomplex(Node*);

/*
 * txt.c
 */
void	ginit(void);
void	gclean(void);
void	nextpc(void);
void	gargs(Node*, Node*);
void	garg1(Node*, Node*, Node*, int);
Node*	nodconst(long);
Node*	nodfconst(double);
void	regalloc(Node*, Node*);
void	regialloc(Node*, Node*);
void	regsalloc(Node*, Node*);
void	regind(Node*, Node*);
void	regret(Node*, Node*);
void	gprep(Node*, Node*);
void	naddr(Node*, Adr*);
void	gopcode(int , Node*, Node*);
void	gopcode3(int, Node*, Node*);
int	samaddr(Node*, Node*);
int	acc(Node*);
void	gbranch(int);
void	patch(Prog*, long);
void	gpseudo(int, Sym*, Node*);
int	twidth(Type*);
long	exreg(Type*);

/*
 * swt.c
 */
int	swcmp(void*, void*);
void	doswit(Node*);
void	swit1(C1*, int, long, Node*);
void	cas(void);
void	bitload(Node*, Node*, Node*, Node*);
void	bitstore(Node*, Node*, Node*, Node*, Node*);
long	outstring(char*, long);
void	doinc(Node*, int);
int	vlog(Node*);
void	nullwarn(Node*, Node*);
void	sextern(Sym*, Node*, long, long);
void	gextern(Sym*, Node*, long, long);
void	outcode(void);
void	ieeedtod(Ieee*, double);

/*
 * list.c
 */
void	listinit(void);
int	Pconv(void*, Fconv*);
int	Aconv(void*, Fconv*);
int	Dconv(void*, Fconv*);
int	Wconv(void*, Fconv*);
int	Sconv(void*, Fconv*);

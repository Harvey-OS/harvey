#define K	4		/* number of inputs/block */

typedef struct Term Term;
typedef struct Sym Sym;
typedef struct Tree Tree;
typedef Tree* NODEPTR;

struct Term {
	ulong value;
	ulong mask;
};

struct Sym {
	char *name;
	char *pin;
	Tree *exp;
	Tree *clk;
	Tree *ena;
	Tree *cke;
	Tree *rd;
	Tree *pre;
	char inst;		/* xnf macro, number of pins */
	char hard;		/* x4000 hard macro */
	char lval;
	char rval;
	char inv;
	char tog;
	char latch;
	char internal;
	char clock;
};

struct Tree {
	int op;
	ulong mask;		/* depends on these inputs */
	int vis;		/* always invisible */
	Tree *left;
	Tree *right;
	int val;
	Sym *id;
	int use;
	Tree *next;
};

extern char *opname[];
extern Term eqn[];
extern Sym symtab[];
extern Sym *dep[];
extern Tree *Root;
extern int ndep,neqn;
extern int cflag;
extern int dflag;
extern int fflag;
extern int kflag;
extern int mflag;
extern int nflag;
extern int uflag;
extern int vflag;
extern int maxload;
extern int aflag;
extern int xflag;
extern int levcrit;
extern Tree *ONE,*ZERO;

void _matchinit(void);
void _match(void);
void terminit(void);
Tree *opnode(int, Tree *, Tree *);
Tree *constnode(int);
Tree *idnode(Sym *, int);
Tree *nameit(Sym *, Tree *);
Tree *uniq(Tree *);
Tree *tree(Term *, int);
Tree *xortree(Sym **, int, int);
int nbits(ulong);
void bitreverse(Term *, int, Sym **, int);
#define notnode(l)	opnode(not,l,0)
#define bufnode(l)	opnode(buffer,l,0)
#define andnode(l,r)	opnode(and,l,r)
#define ornode(l,r)	opnode(or,l,r)
#define xornode(l,r)	opnode(xor,l,r)
Tree *muxnode(Tree *, Tree *, Tree *);
Tree *factnode(ulong, ulong, Sym **);
Sym *lookup(char *);
Sym *looktup(char *, char *);
void definst(Sym *, int, Sym *);
void prtree(Tree *);
int pinconv(void *, Fconv *);
int treeconv(void *, Fconv *);
void apply(void (*f)(Sym *));
void merge(Sym *);
void dovis(Sym *);
void outsym(Sym *);
void prsym(Sym *);
int eq(Tree *, Tree *);
void yyerror(char *, ...);
void markvis(Tree *);
void outbin(ulong);

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
	Tree *exp;
	Tree *clk;
	Tree *ena;
	Tree *cke;
	Tree *rd;
	Tree *pre;
	char lval;
	char rval;
	char inv;
	char tog;
	char latch;
	char internal;
	char putout;
	char clock;
};

struct Tree {
	int op;
	Tree *left;
	Tree *right;
	Tree *cke;		/* extra baggage for dff's */
	Tree *clr;
	Tree *pre;
	Tree *buf;		/* for fanout */
	int val;
	Sym *id;
	char *fname;		/* function (gate) name */
	char *pname;		/* pin name, might be one of >1 (hack) */
	int net;
	int fanout;
	int use;
	int level;
	Tree *next;
};

extern Term eqn[];
extern Sym symtab[];
extern Sym *dep[];
extern char *opname[];
extern Tree *Root;
extern int ndep,neqn;
extern int bflag;
extern int cflag;
extern int dflag;
extern int kflag;
extern int mflag;
extern int nflag;
extern int uflag;
extern int vflag;
extern int maxload;
extern int aflag;
extern int xflag;
extern int levcrit;
extern Tree *ONE,*ZERO,*CLOK,*CLK0,*CLK1;
extern FILE *devnull;
extern FILE *critfile;

void _matchinit(void);
void _match(void);
void terminit(void);
Tree *opnode(int, Tree *, Tree *);
Tree *constnode(int);
Tree *idnode(Sym *, int);
Tree *nameit(Sym *, Tree *);
Tree *uniq(Tree *);
Tree *tree(Term *, int);
Tree *xortree(Sym **, int);
Tree *downprop(Tree *, int);
void bitreverse(Term *, int, Sym **, int);
#define notnode(l)	opnode(not,l,0)
#define bufnode(l)	opnode(buffer,l,0)
#define andnode(l,r)	opnode(and,l,r)
#define ornode(l,r)	opnode(or,l,r)
#define xornode(l,r)	opnode(xor,l,r)
Tree *muxnode(Tree *, Tree *, Tree *);
Tree *padnode(int, Sym *, Tree *, Tree *);
#define innode(s,l)	padnode(inbuf,s,l,0)
#define clknode(s,l)	padnode(clkbuf,s,l,0)
#define outnode(s,l)	padnode(outbuf,s,l,0)
#define binode(s,l,r)	padnode(bibuf,s,l,r)
#define trinode(s,l,r)	padnode(tribuf,s,l,r)
Tree *factnode(ulong, ulong, Sym **);
Tree *dffnode(Sym *, Tree *, Tree *, Tree *, Tree *, Tree *);
Tree *latnode(Sym *, Tree *, Tree *, Tree *, Tree *);
Tree *mtGetNodes(Tree *, int);
Sym *lookup(char *);
void prtree(Tree *);
void ctree(Tree *);
void cout(void);
void symout(void);
int eq(Tree *, Tree *);
void push(Tree *);
void namepin(char *, int);
void func(Tree *, char *, int, ...);
void evalprint(FILE *);
void yyerror(char *, ...);

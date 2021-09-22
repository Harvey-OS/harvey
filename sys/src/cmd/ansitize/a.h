#include <u.h>
#include <libc.h>

typedef struct AST	AST;
typedef struct Alist	Alist;
typedef struct DFA	DFA;
typedef struct Gval	Gval;
typedef struct Hash	Hash;
typedef struct Hashiter	Hashiter;
typedef struct Hashkv	Hashkv;
typedef struct Line	Line;
typedef struct List	List;
typedef struct Pval	Pval;
typedef struct Yedge 	Yedge;
typedef struct Ygram	Ygram;
typedef struct Yparse	Yparse;
typedef struct Ypath 	Ypath;
typedef struct Yrpos	Yrpos;
typedef struct Yrule	Yrule;
typedef struct Yset	Yset;
typedef struct Yshift 	Yshift;
typedef struct Ystack 	Ystack;
typedef struct Ystate	Ystate;
typedef struct Ystats	Ystats;
typedef struct Ysym	Ysym;

#pragma incomplete DFA
#pragma incomplete Hash

typedef char*	Name;
typedef int	Yact(Ygram*, Yrule*, void*, void*);
typedef Ysym*	Ylex(Ygram*, void*, void*);

enum
{
	RegexpNormal = 0,
	RegexpLiteral,
	RegexpShortest = 0x10,
};

struct Alist
{
	List	*first;
	List	*last;
};

struct Hashiter
{
	int	i;
	int	j;
	Hash	*h;
};

struct Hashkv
{
	void	*key;
	void	*value;
};

struct Line
{
	char	*file;
	int	line;
};

struct List
{
	void	*hd;
	List	*tl;
};

/* associativity (added to 2*prec) */
enum
{
	Yleft		= 1,
	Ynonassoc	= 0,
	Ytoken		= Ynonassoc,
	Ynonterm	= 100,
	Yright		= -1
};

struct Ygram
{
	int	debug;
	int	valsize;
	Ysym	*wildcard;
	Ysym	*eof;
	
	/* internal use only */
	int	dirty;		/* needs compilation */
	uint	seqnum;		/* incremented on compilation */
	Ysym	**sym;		/* all symbols */
	int	nsym;
	Yrule	**rule;		/* all rules */
	int	nrule;
	Ystate	**state;	/* all states */
	int	nstate;
	Ystate	*statehash[128];
};

/* terminal or non-terminal in grammar */
enum
{
	YsymTerminal	= 1<<0,	/* is terminal */
	YsymEmpty	= 1<<1,	/* reduces to empty string */
};
struct Ysym
{
	int	n;		/* symbol number */
	int	prec;		/* base precedence */
	int	assoc;		/* associativity */
	int	flags;		/* YsymReduces, etc. above */
	char	*type;		/* type */

	char	*name;		/* name of symbol */
	Yrule	**rule;		/* rules with symbol on left */
	int	nrule;
	Yrule	*parseme;

	Yset	*first;		/* first terms of derived strings */
	Yset	*starts;	/* closure of initial symbols */
	Yset	*follow;	/* follow set */
};

/* grammar production */
struct Yrule
{
	int	n;
	int	prec;		/* reduce precedence */
	Ysym	*left;		/* left hand side */
	Ysym	**right;	/* right hand side */
	int 	nright;
	int	toplevel;
	Yact	*fn;		/* reduce fn */
};

/* single position in a rule */
struct Yrpos
{
	Yrule	*r;
	int	i;
};

/* bit vector */
enum { YW = 8*sizeof(uint) };
struct Yset
{
	int n;
	uint a[1];		/* really a[n] */
};

/*
 * Simple LR
 */
struct Yshift
{
	Ysym	*sym;
	Ystate	*state;
};

/* a single SLR state */
struct Ystate
{
	int	n;		/* state number */
	Ygram	*g;		/* grammar */
	Yrpos	*pos;		/* list of positions */
	int	npos;
	int	compiled;	/* are shift and reduce filled out? */
	Yshift	*shift;		/* computed shifts */
	int	nshift;
	Yrule	**reduce;	/* computed reductions */
	int	nreduce;
	Ystate	*next;		/* in hash table */
};

/*
 * Generalized LR.
 */
struct Yedge
{
	Yedge	*sib;
	Ystack	*down;
	Ysym	*sym;
	char	val[];
};

struct Yparse
{
	Ygram	*g;
	uint	seqnum;
	Ystack	**top;
	uint	ntop;
	uint	mtop;
	Ysym	*tok;
	void	*tokval;
	void	*tmpval;
	Ypath	**queue;
	uint	nqueue;
	uint	mqueue;
	void	*aux;
	void	(*merge)(void*, void*);
};

struct Ypath 
{
	Yrule	*rule;
	Ystack	*sp;
	Yedge	*edge[];
};

struct Ystack
{
	Ystate	*state;
	Yedge	*edge;
	Ystate	*doshift;
};

/*
 * Parsing values
 */

/* grammar value */
struct Gval
{
	Ysym	*yaccsym;	/* must be first - known to yyparse */
	
	int	op;		/* Left, Right, Token */
	int	flag;
	Ysym	*prec;
	int	assoc;
	Name	x;		/* name, literal, regexp */
	Ysym	*sym;
	Alist	al;
};

/* parsed value */
struct Pval
{
	Ysym	*yaccsym;	/* must be first - known to yyparse */

	char	*s;
	char	*out;
	AST	**ast;
};

/* abstract syntax */
struct AST
{
	Ysym	*sym;
	char	*pre;
	char	*post;
	char	*s;
	AST	**up;
	AST	***right;
	int	nright;
};

int	addtodfa(DFA*, char*, int, int, void*);
int	astfmt(Fmt*);
void*	hashget(Hash*, void*);
void	hashiterend(Hashiter*);
int	hashiternextkey(Hashiter*, void**);
int	hashiternextkv(Hashiter*, Hashkv*);
int	hashiterstart(Hashiter*, Hash*);
List*	hashkeys(Hash*);
void*	hashput(Hash*, void*, void*);
void	listaconcat(Alist*, Alist*);
void	listadd(Alist*, void*);
void	listconcat(Alist*, List*);
List*	listcopy(List*);
int	listeq(List*, List*);
void**	listflatten(List*);
int	listlen(List*);
void	listreset(Alist*);
List*	listsort(List*, int(*)(void*,void*));
void	listtoalist(List*, Alist*);
void	loadgrammar(char*);
int	match(AST*, char*);
int	matchrule(AST*, char*, char*, ...);
AST**	mkast(Ysym*, int);
DFA*	mkdfa(void);
Hash*	mkhash(void);
List*	mklist(void*, List*);
Name	nameof(char*);
AST*	parsefile(char*);
void	parsegrammar(char*);
char*	readfile(char*);
void*	rundfa(DFA*, char*, char**);
int	yyaddset(Yset*, Yset*);
Ystate*	yycanonstate(Ygram*, Ystate*);
int	yycanshift(Ygram*, Ystate*, Ysym*, Ystate**);
void	yycfgcomp(Ygram*);
void	yycheckstate(Ystate*);
int	yycompilestate(Ygram*, Ystate*);
void	yycompile(Ygram*);
Ystate*	yycomputeshift(Ygram*, Ystate*, Ysym*);
void	yycomp(Ygram*);
int	yycountset(Yset*);
void	yydumptables(Ygram*);
void	yyerror(Ygram*, Yparse*, char*);
int	yyfmtinstall(void);	/* install %R (Yrule*) and %Y (Ystate*) */
void	yyfreegram(Ygram*);
void	yyfreeparse(Yparse*);
void	yyfreeset(Yset*);
void	yyfreestate(Ystate*);
Ystate*	yyinitstate(Ygram*, char*);
Ysym*	yylooksym(Ygram*, char*);
Ygram*	yynewgram(void);
Yparse*	yynewparse(void);
Yset*	yynewset(uint);
Ysym*	yynewsym(Ygram*, char*, char*, int, int);
void*	yyparse(Ygram*, char*, Yparse*, Ylex*, void*);
int	yyrulefmt(Fmt*);
int	yyrulestr(Ygram*, Yact*, char*, char*, char*, ...);
int	yyrulesym(Ygram*, Yact*, Ysym*, Ysym*, Ysym*, ...);
int	yyrule(Ygram*, Yact*, Ysym*, Ysym*, Ysym**, int);
void	yyslrcomp(Ygram*);
int	yystatecmp(Ystate*, Ystate*);
int	yystatefmt(Fmt*);

/* poor substitutes for real iterators as in rscc */
#define forlist(x, l) for(List *_l=(l); _l ? ((x)=_l->hd, 1) : 0; _l=_l->tl)
#define foreach(name, x, s) \
	for(int _i=0; _i<(s)->n ## name ? ((x)=(s)->name[_i], 1) : 0; _i++)
#define foreachptr(name, x, s) \
	for(int _i=0; _i<(s)->n ## name ? ((x)=&(s)->name[_i], 1) : 0; _i++)
#define forset(i, s) \
	for((i)=yynextset((s), 0); (i)!=-1; (i)=yynextset((s), (i)+1))

static inline int
yyinset(Yset *s, int x)
{
	return s->a[x/YW] & (1<<(x%YW));
}

static inline int
yyaddbit(Yset *s, int x)
{
	int b;
	
	b = 1<<(x%YW);
	if(s->a[x/YW] & b)
		return 1;
	s->a[x/YW] |= b;
	return 0;
}

static inline int
yynextset(Yset *s, int i)
{
	for(; i<s->n*YW; i++)
		if(s->a[i/YW]&(1<<(i%YW)))
			return i;
	return -1;
}

extern Line L, ZL;

#pragma varargck type "A" AST*
#pragma varargck type "B" AST*
#pragma varargck type "Y" Ystate*
#pragma varargck type "lY" Ystate*
#pragma varargck type "R" Yrule*

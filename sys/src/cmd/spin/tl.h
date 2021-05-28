/***** tl_spin: tl.h *****/

/*
 * This file is part of the public release of Spin. It is subject to the
 * terms in the LICENSE file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com
 *
 * Based on the translation algorithm by Gerth, Peled, Vardi, and Wolper,
 * presented at the PSTV Conference, held in 1995, Warsaw, Poland 1995.
 */

#ifndef TLH
#define TLH

#include <stdio.h>
#include <string.h>

typedef struct Symbol {
	char		*name;
	struct Symbol	*next;	/* linked list, symbol table */
} Symbol;

typedef struct Node {
	short		ntyp;	/* node type */
	struct Symbol	*sym;
	struct Node	*lft;	/* tree */
	struct Node	*rgt;	/* tree */
	struct Node	*nxt;	/* if linked list */
} Node;

typedef struct Graph {
	Symbol		*name;
	Symbol		*incoming;
	Symbol		*outgoing;
	Symbol		*oldstring;
	Symbol		*nxtstring;
	Node		*New;
	Node		*Old;
	Node		*Other;
	Node		*Next;
	unsigned char	isred[64], isgrn[64];
	unsigned char	redcnt, grncnt;
	unsigned char	reachable;
	struct Graph	*nxt;
} Graph;

typedef struct Mapping {
	char	*from;
	Graph	*to;
	struct Mapping	*nxt;
} Mapping;

enum {
	ALWAYS=257,
	AND,		/* 258 */
	EQUIV,		/* 259 */
	EVENTUALLY,	/* 260 */
	FALSE,		/* 261 */
	IMPLIES,	/* 262 */
	NOT,		/* 263 */
	OR,		/* 264 */
	PREDICATE,	/* 265 */
	TRUE,		/* 266 */
	U_OPER,		/* 267 */
	V_OPER		/* 268 */
#ifdef NXT
	, NEXT		/* 269 */
#endif
	, CEXPR		/* 270 */
};

Node	*Canonical(Node *);
Node	*canonical(Node *);
Node	*cached(Node *);
Node	*dupnode(Node *);
Node	*getnode(Node *);
Node	*in_cache(Node *);
Node	*push_negation(Node *);
Node	*right_linked(Node *);
Node	*tl_nn(int, Node *, Node *);

Symbol	*tl_lookup(char *);
Symbol	*getsym(Symbol *);
Symbol	*DoDump(Node *);

extern char	*emalloc(size_t);	/* in main.c */

extern unsigned int	hash(const char *);	/* in sym.c */

int	anywhere(int, Node *, Node *);
int	dump_cond(Node *, Node *, int);
int	isalnum_(int);	/* in spinlex.c */
int	isequal(Node *, Node *);
int	tl_Getchar(void);

void	*tl_emalloc(int);
void	*tl_erealloc(void*, int, int);
void	a_stats(void);
void	addtrans(Graph *, char *, Node *, char *);
void	cache_stats(void);
void	dump(Node *);
void	exit(int);
void	Fatal(char *, char *);
void	fatal(char *, char *);
void	fsm_print(void);
void	ini_buchi(void);
void	ini_cache(void);
void	ini_rewrt(void);
void	ini_trans(void);
void	releasenode(int, Node *);
void	tfree(void *);
void	tl_explain(int);
void	tl_UnGetchar(void);
void	tl_parse(void);
void	tl_yyerror(char *);
void	trans(Node *);

#define ZN	(Node *)0
#define ZS	(Symbol *)0
#define Nhash	255    	/* must match size in spin.h */
#define True	tl_nn(TRUE,  ZN, ZN)
#define False	tl_nn(FALSE, ZN, ZN)
#define Not(a)	push_negation(tl_nn(NOT, a, ZN))
#define rewrite(n)	canonical(right_linked(n))

typedef Node	*Nodeptr;
#define YYSTYPE	 Nodeptr

#define Debug(x)	{ if (tl_verbose) printf(x); }
#define Debug2(x,y)	{ if (tl_verbose) printf(x,y); }
#define Dump(x)		{ if (tl_verbose) dump(x); }
#define Explain(x)	{ if (tl_verbose) tl_explain(x); }

#define Assert(x, y)	{ if (!(x)) { tl_explain(y); \
			  Fatal(": assertion failed\n",(char *)0); } }

#endif

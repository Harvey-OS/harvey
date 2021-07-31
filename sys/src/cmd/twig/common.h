#include <u.h>
#include <libc.h>
#include <bio.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

#define assert(e) \
	if(!(e)){fprint(2, "assertion failed\n");abort();}else

enum{
	MAXIDSIZE	= 100,
	HASHSIZE	= 1181,
};

/* forward and external type defs */
extern Biobuf *bout;
extern Biobuf *symfile;
extern Biobuf *bin;

extern int debug_flag;
#	define	DB_LEX	1	/* print out lexical analyser debugging info */
#	define	DB_MACH	2	/* print out machine information */
#	define	DB_SYM	4	/* print out symbol debugging info */
#	define	DB_TREE	8	/* print out trees */
#	define	DB_MEM	16	/* check memory usage */
extern int tflag;		/* generate tables only */
extern int sflag;		/* don't output symbols */
extern int ntrees;
extern int unique;

/* tree definitions */
typedef struct node		Node;
typedef struct symbol_entry	SymbolEntry;

/* Nodes provide the backbone for the trees built by the parser */
struct node {
	SymbolEntry *sym;
	int nlleaves;	/* count of the labelled leaves */
	Node *children;
	Node *siblings;
};

/* tree.c */
Node	*TreeBuild(SymbolEntry *, Node *);
int	cntnodes(Node *);
void	TreeFree(Node *);
void	TreePrint(Node *, int);
void	TreeInit(void);

/* path.c */
void	path_setup(Node *root);

/* lex.c */
extern int yyline;
extern char *inFileName;
extern char token_buffer[MAXIDSIZE+1];
void	LexInit(void);
int	yylex(void);
void	lexCleanup(void);

/* io.c */
void	oreset(void);
int	ointcnt(void);
void	oputint(int);
void	oputoct(int);

/* machine.c */
void	MachInit(void);

/* twig.y */
void	*Malloc(unsigned long);
void	cleanup(int);
void	error(int rc, char *, ...);
void	sem_error(char *, ...);
void	sem_error2(char *, ...);
void	yyerror(char *fmt, ...);
void	yyerror2(char *fmt, ...);

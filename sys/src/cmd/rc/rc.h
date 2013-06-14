/*
 * Assume plan 9 by default; if Unix is defined, assume unix.
 * Please don't litter the code with ifdefs.  The five below should be enough.
 */

#ifndef Unix
/* plan 9 */
#include <u.h>
#include <libc.h>

#define	NSIG	32
#define	SIGINT	2
#define	SIGQUIT	3

#define fcntl(fd, op, arg)	/* unix compatibility */
#define F_SETFD
#define FD_CLOEXEC
#else
#include "unix.h"
#endif

#ifndef ERRMAX
#define ERRMAX 128
#endif

#define	YYMAXDEPTH	500
#ifndef YYPREFIX
#ifndef PAREN
#include "x.tab.h"
#endif
#endif

typedef struct tree tree;
typedef struct word word;
typedef struct io io;
typedef union code code;
typedef struct var var;
typedef struct list list;
typedef struct redir redir;
typedef struct thread thread;
typedef struct builtin builtin;

#ifndef Unix
#pragma incomplete word
#pragma incomplete io
#endif

struct tree{
	int	type;
	int	rtype, fd0, fd1;	/* details of REDIR PIPE DUP tokens */
	char	*str;
	int	quoted;
	int	iskw;
	tree	*child[3];
	tree	*next;
};
tree *newtree(void);
tree *token(char*, int), *klook(char*), *tree1(int, tree*);
tree *tree2(int, tree*, tree*), *tree3(int, tree*, tree*, tree*);
tree *mung1(tree*, tree*), *mung2(tree*, tree*, tree*);
tree *mung3(tree*, tree*, tree*, tree*), *epimung(tree*, tree*);
tree *simplemung(tree*), *heredoc(tree*);
void freetree(tree*);
tree *cmdtree;

/*
 * The first word of any code vector is a reference count.
 * Always create a new reference to a code vector by calling codecopy(.).
 * Always call codefree(.) when deleting a reference.
 */
union code{
	void	(*f)(void);
	int	i;
	char	*s;
};

char *promptstr;
int doprompt;

#define	NTOK	8192		/* maximum bytes in a word (token) */

char tok[NTOK + UTFmax];

#define	APPEND	1
#define	WRITE	2
#define	READ	3
#define	HERE	4
#define	DUPFD	5
#define	CLOSE	6
#define RDWR	7

struct var{
	char	*name;		/* ascii name */
	word	*val;		/* value */
	int	changed;
	code	*fn;		/* pointer to function's code vector */
	int	fnchanged;
	int	pc;		/* pc of start of function */
	var	*next;		/* next on hash or local list */
};
var *vlook(char*), *gvlook(char*), *newvar(char*, var*);

#define	NVAR	521

var *gvar[NVAR];		/* hash for globals */

#define	new(type)	((type *)emalloc(sizeof(type)))

void *emalloc(long);
void *Malloc(ulong);
void efree(void *);

struct here{
	tree	*tag;
	char	*name;
	struct here *next;
};
int mypid;

/*
 * Glob character escape in strings:
 *	In a string, GLOB must be followed by *?[ or GLOB.
 *	GLOB* matches any string
 *	GLOB? matches any single character
 *	GLOB[...] matches anything in the brackets
 *	GLOBGLOB matches GLOB
 */
#define	GLOB	'\001'

char **argp;
char **args;
int nerror;		/* number of errors encountered during compilation */
int doprompt;		/* is it time for a prompt? */
/*
 * Which fds are the reading/writing end of a pipe?
 * Unfortunately, this can vary from system to system.
 * 9th edition Unix doesn't care, the following defines
 * work on plan 9.
 */
#define	PRD	0
#define	PWR	1

char *Rcmain, *Fdprefix;
/*
 * How many dot commands have we executed?
 * Used to ensure that -v flag doesn't print rcmain.
 */
int ndot;
char *getstatus(void);
int lastc;
int lastword;

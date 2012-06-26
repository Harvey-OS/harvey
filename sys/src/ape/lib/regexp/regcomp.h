/*
 *  substitution list
 */
typedef struct Resublist	Resublist;
struct	Resublist
{
	Resub	m[32];
};

/* max subexpressions per program */
Resublist ReSuBlIsT;
#define NSUBEXP (sizeof(ReSuBlIsT.m)/sizeof(Resub))

/* max character classes per program */
Reprog	RePrOg;
#define	NCLASS	(sizeof(RePrOg.class)/sizeof(Reclass))

/* max rune ranges per character class */
#define NCCRUNE	(sizeof(Reclass)/sizeof(wchar_t))

/*
 * Actions and Tokens (Reinst types)
 *
 *	02xx are operators, value == precedence
 *	03xx are tokens, i.e. operands for operators
 */
#define RUNE		0177
#define	OPERATOR	0200	/* Bitmask of all operators */
#define	START		0200	/* Start, used for marker on stack */
#define	RBRA		0201	/* Right bracket, ) */
#define	LBRA		0202	/* Left bracket, ( */
#define	OR		0203	/* Alternation, | */
#define	CAT		0204	/* Concatentation, implicit operator */
#define	STAR		0205	/* Closure, * */
#define	PLUS		0206	/* a+ == aa* */
#define	QUEST		0207	/* a? == a|nothing, i.e. 0 or 1 a's */
#define	ANY		0300	/* Any character except newline, . */
#define	ANYNL		0301	/* Any character including newline, . */
#define	NOP		0302	/* No operation, internal use only */
#define	BOL		0303	/* Beginning of line, ^ */
#define	EOL		0304	/* End of line, $ */
#define	CCLASS		0305	/* Character class, [] */
#define	NCCLASS		0306	/* Negated character class, [] */
#define	END		0377	/* Terminate: match found */

/*
 *  regexec execution lists
 */
#define LISTINCREMENT 8
typedef struct Relist	Relist;
struct Relist
{
	Reinst		*inst;		/* Reinstruction of the thread */
	Resublist	se;		/* matched subexpressions in this thread */
};
extern Relist*	_relist[2];
extern Relist*	_reliste[2];
extern int	_relistsize;

extern Relist*	_renewthread(Relist*, Reinst*, Resublist*);
extern void	_renewmatch(Resub*, int, Resublist*);

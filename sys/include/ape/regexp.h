#ifndef __REGEXP_H
#define __REGEXP_H
#ifndef _REGEXP_EXTENSION
    This header file is an extension to ANSI/POSIX
#endif
#pragma lib "/$M/lib/ape/libregexp.a"

#ifdef	UTF
#define	Runeself	0xA0
#else
#define	Runeself	0
#endif

typedef struct Resub		Resub;
typedef struct Reclass		Reclass;
typedef struct Reinst		Reinst;
typedef struct Reprog		Reprog;

/*
 *	Sub expression matches
 */
struct Resub{
	union
	{
		char *sp;
		wchar_t *rsp;
	} s;
	union
	{
		char *ep;
		wchar_t *rep;
	} e;
};

/*
 *	character class, each pair of rune's defines a range
 */
struct Reclass{
	wchar_t	*end;
	wchar_t	spans[64];
};

/*
 *	Machine instructions
 */
struct Reinst{
	int	type;			/* < 0200 ==> literal, otherwise action */
	union	{
		Reclass	*cp;		/* class pointer */
		wchar_t	r;		/* character */
		int	subid;		/* sub-expression id for RBRA and LBRA */
		Reinst	*right;		/* right child of OR */
	} r;
	union {	/* regexp relies on these two being in the same union */
		Reinst *left;		/* left child of OR */
		Reinst *next;		/* next instruction for CAT & LBRA */
	} l;
};

/*
 *	Reprogram definition
 */
struct Reprog{
	Reinst	*startinst;	/* start pc */
	Reclass	class[16];	/* .data */
	Reinst	firstinst[5];	/* .text */
};

extern Reprog	*regcomp(char*);
extern Reprog	*regcomplit(char*);
extern Reprog	*regcompnl(char*);
extern void	regerror(char*);
extern int	regexec(Reprog*, char*, Resub*, int);
extern void	regsub(char*, char*, int, Resub*, int);
extern int	rregexec(Reprog*, wchar_t*, Resub*, int);
extern void	rregsub(wchar_t*, wchar_t*, int, Resub*, int);
#endif

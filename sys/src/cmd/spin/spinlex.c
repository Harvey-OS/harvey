/***** spin: spinlex.c *****/

/* Copyright (c) 1991-2000 by Lucent Technologies - Bell Laboratories     */
/* All Rights Reserved.  This software is for educational purposes only.  */
/* Permission is given to distribute this code provided that this intro-  */
/* ductory message is not removed and no monies are exchanged.            */
/* No guarantee is expressed or implied by the distribution of this code. */
/* Software written by Gerard J. Holzmann as part of the book:            */
/* `Design and Validation of Computer Protocols,' ISBN 0-13-539925-4,     */
/* Prentice Hall, Englewood Cliffs, NJ, 07632.                            */
/* Send bug-reports and/or questions to: gerard@research.bell-labs.com    */

#include <stdlib.h>
#include "spin.h"
#ifdef PC
#include "y_tab.h"
#else
#include "y.tab.h"
#endif

#define MAXINL	16	/* max recursion depth inline fcts */
#define MAXPAR	32	/* max params to an inline call */
#define MAXLEN	512	/* max len of an actual parameter text */

typedef struct IType {
	Symbol *nm;		/* name of the type */
	Lextok *cn;		/* contents */
	Lextok *params;		/* formal pars if any */
	char   **anms;		/* literal text for actual pars */
	int    dln, cln;	/* def and call linenr */
	Symbol *dfn, *cfn;	/* def and call filename */
	struct IType *nxt;	/* linked list */
} IType;

extern Symbol	*Fname;
extern YYSTYPE	yylval;
extern short	has_last, terse;
extern int	verbose, IArgs;

int	lineno  = 1, IArgno = 0;
int	Inlining = -1;
char	*Inliner[MAXINL], IArg_cont[MAXPAR][MAXLEN];
IType	*Inline_stub[MAXINL];
char	yytext[2048];
FILE	*yyin, *yyout;

static unsigned char	in_comment=0;
static char	*ReDiRect;
static int	check_name(char *);

#if 1
#define Token(y)	{ if (in_comment) goto again; \
			yylval = nn(ZN,0,ZN,ZN); return y; }

#define ValToken(x, y)	{ if (in_comment) goto again; \
			yylval = nn(ZN,0,ZN,ZN); yylval->val = x; return y; }

#define SymToken(x, y)	{ if (in_comment) goto again; \
			yylval = nn(ZN,0,ZN,ZN); yylval->sym = x; return y; }
#else
#define Token(y)	{ yylval = nn(ZN,0,ZN,ZN); \
			if (!in_comment) return y; else goto again; }

#define ValToken(x, y)	{ yylval = nn(ZN,0,ZN,ZN); yylval->val = x; \
			if (!in_comment) return y; else goto again; }

#define SymToken(x, y)	{ yylval = nn(ZN,0,ZN,ZN); yylval->sym = x; \
			if (!in_comment) return y; else goto again; }
#endif

#define Getchar()	((Inlining<0)?getc(yyin):getinline())
#define Ungetch(c)	{if (Inlining<0) ungetc(c,yyin); else uninline(); }

static int	getinline(void);
static void	uninline(void);

static int
notquote(int c)
{	return (c != '\"' && c != '\n');
}

int
isalnum_(int c)
{	return (isalnum(c) || c == '_');
}

static int
isalpha_(int c)
{	return isalpha(c);	/* could be macro */
}
       
static int
isdigit_(int c)
{	return isdigit(c);	/* could be macro */
}

static void
getword(int first, int (*tst)(int))
{	int i=0; char c;

	yytext[i++]= (char) first;
	while (tst(c = Getchar()))
		yytext[i++] = c;
	yytext[i] = '\0';
	Ungetch(c);
}

static int
follow(int tok, int ifyes, int ifno)
{	int c;

	if ((c = Getchar()) == tok)
		return ifyes;
	Ungetch(c);

	return ifno;
}

static IType *seqnames;

static void
def_inline(Symbol *s, int ln, char *ptr, Lextok *nms)
{	IType *tmp;
	char *nw = (char *) emalloc(strlen(ptr)+1);
	strcpy(nw, ptr);

	for (tmp = seqnames; tmp; tmp = tmp->nxt)
		if (!strcmp(s->name, tmp->nm->name))
		{	non_fatal("procedure name %s redefined",
				tmp->nm->name);
			tmp->cn = (Lextok *) nw;
			tmp->params = nms;
			tmp->dln = ln;
			tmp->dfn = Fname;
			return;
		}
	tmp = (IType *) emalloc(sizeof(IType));
	tmp->nm = s;
	tmp->cn = (Lextok *) nw;
	tmp->params = nms;
	tmp->dln = ln;
	tmp->dfn = Fname;
	tmp->nxt = seqnames;
	seqnames = tmp;
}

static int
iseqname(char *t)
{	IType *tmp;

	for (tmp = seqnames; tmp; tmp = tmp->nxt)
	{	if (!strcmp(t, tmp->nm->name))
			return 1;
	}
	return 0;
}

static int
getinline(void)
{	int c;

	if (ReDiRect)
	{	c = *ReDiRect++;
		if (c == '\0')
		{	ReDiRect = (char *) 0;
			c = *Inliner[Inlining]++;
		}
	} else
		c = *Inliner[Inlining]++;

	if (c == '\0')
	{	lineno = Inline_stub[Inlining]->cln;
		Fname  = Inline_stub[Inlining]->cfn;
		Inlining--;
#if 0
		if (verbose&32)
		printf("spin: line %d, done inlining %s\n",
			lineno, Inline_stub[Inlining+1]->nm->name);
#endif
		return Getchar();
	}
	return c;
}

static void
uninline(void)
{
	if (ReDiRect)
		ReDiRect--;
	else
		Inliner[Inlining]--;
}

void
pickup_inline(Symbol *t, Lextok *apars)
{	IType *tmp; Lextok *p, *q; int j;

	for (tmp = seqnames; tmp; tmp = tmp->nxt)
		if (!strcmp(t->name, tmp->nm->name))
			break;

	if (!tmp)
		fatal("cannot happen, ns %s", t->name);
	if (++Inlining >= MAXINL)
		fatal("inline fcts too deeply nested", 0);

	tmp->cln = lineno;	/* remember calling point */
	tmp->cfn = Fname;	/* and filename */

	for (p = apars, q = tmp->params, j = 0; p && q; p = p->rgt, q = q->rgt)
		j++; /* count them */
	if (p || q)
		fatal("wrong nr of params on call of '%s'", t->name);

	tmp->anms  = (char **) emalloc(j * sizeof(char *));
	for (p = apars, j = 0; p; p = p->rgt, j++)
	{	tmp->anms[j] = (char *) emalloc(strlen(IArg_cont[j])+1);
		strcpy(tmp->anms[j], IArg_cont[j]);
	}

	lineno = tmp->dln;	/* linenr of def */
	Fname = tmp->dfn;	/* filename of same */
	Inliner[Inlining] = (char *)tmp->cn;
	Inline_stub[Inlining] = tmp;
#if 0
	if (verbose&32)
	printf("spin: line %d, file %s, inlining '%s' (from line %d, file %s)\n",
		tmp->cln, tmp->cfn->name, t->name, tmp->dln, tmp->dfn->name);
#endif
	for (j = 0; j < Inlining; j++)
		if (Inline_stub[j] == Inline_stub[Inlining])
		fatal("cyclic inline attempt on: %s", t->name);
}

static void
do_directive(int first, int fromwhere)
{	int c = first;	/* handles lines starting with pound */

	getword(c, isalpha_);

	if ((c = Getchar()) != ' ')
		fatal("malformed preprocessor directive - # .", 0);

	if (!isdigit_(c = Getchar()))
		fatal("malformed preprocessor directive - # .lineno", 0);

	getword(c, isdigit_);
	lineno = atoi(yytext);	/* pickup the line number */

	if ((c = Getchar()) == '\n')
		return;	/* no filename */

	if (c != ' ')
		fatal("malformed preprocessor directive - .fname", 0);

	if ((c = Getchar()) != '\"')
		fatal("malformed preprocessor directive - .fname", 0);

	getword(c, notquote);
	if (Getchar() != '\"')
		fatal("malformed preprocessor directive - fname.", 0);

	strcat(yytext, "\"");
	Fname = lookup(yytext);
	while (Getchar() != '\n')
		;
}

#define SOMETHINGBIG	65536

void
prep_inline(Symbol *s, Lextok *nms)
{	int c, nest = 1, dln, firstchar, cnr;
	char *p, buf[SOMETHINGBIG];
	Lextok *t;

	for (t = nms; t; t = t->rgt)
		if (t->lft)
		{	if (t->lft->ntyp != NAME)
			fatal("bad param to inline %s", s->name);
			t->lft->sym->hidden |= 32;
		}
		
	s->type = PREDEF;
	p = &buf[0];
	for (;;)
	{	c = Getchar();
		switch (c) {
		case '{':
			break;
		case '\n':
			lineno++;
			/* fall through */
		case ' ': case '\t': case '\f': case '\r':
			continue;
		default : fatal("bad inline: %s", s->name);
		}
		break;
	}
	dln = lineno;
	sprintf(buf, "\n#line %d %s\n{", lineno, Fname->name);
	p += strlen(buf);
	firstchar = 1;

	cnr = 1; /* not zero */
more:
	*p++ = c = Getchar();
	if (p - buf >= SOMETHINGBIG)
		fatal("inline text too long", 0);
	switch (c) {
	case '\n':
		lineno++;
		cnr = 0;
		break;
	case '{':
		cnr++;
		nest++;
		break;
	case '}':
		cnr++;
		if (--nest <= 0)
		{	*p = '\0';
			def_inline(s, dln, &buf[0], nms);
			if (firstchar)
			fatal("empty inline definition '%s'", s->name);
			return;
		}
		break;
	case '#':
		if (cnr == 0)
		{	p--;
			do_directive(c, 4); /* reads to newline */
			break;
		} /* else fall through */
	default:
		firstchar = 0;
	case '\t':
	case ' ':
	case '\f':
		cnr++;
		break;
	}
	goto more;
}

static int
lex(void)
{	int c;

again:
	c = Getchar();
	yytext[0] = (char) c;
	yytext[1] = '\0';
	switch (c) {
	case '\n':		/* newline */
		lineno++;
	case '\r':		/* carriage return */
		goto again;

	case  ' ': case '\t': case '\f':	/* white space */
		goto again;

	case '#':		/* preprocessor directive */
		if (in_comment) goto again;
		do_directive(c, 5);
		goto again;

	case '\"':
		getword(c, notquote);
		if (Getchar() != '\"')
			fatal("string not terminated", yytext);
		strcat(yytext, "\"");
		SymToken(lookup(yytext), STRING)

	case '\'':	/* new 3.0.9 */
		c = Getchar();
		if (c == '\\')
		{	c = Getchar();
			if (c == 'n') c = '\n';
			else if (c == 'r') c = '\r';
			else if (c == 't') c = '\t';
			else if (c == 'f') c = '\f';
		}
		if (Getchar() != '\'')
			fatal("character quote missing", yytext);
		ValToken(c, CONST)

	default:
		break;
	}

	if (isdigit_(c))
	{	getword(c, isdigit_);
		ValToken(atoi(yytext), CONST)
	}

	if (isalpha_(c) || c == '_')
	{	getword(c, isalnum_);
		if (!in_comment)
		{	c = check_name(yytext);
			if (c) return c;
			/* else fall through */
		}
		goto again;
	}

	switch (c) {
	case '/': c = follow('*', 0, '/');
		  if (!c) { in_comment = 1; goto again; }
		  break;
	case '*': c = follow('/', 0, '*');
		  if (!c) { in_comment = 0; goto again; }
		  break;
	case ':': c = follow(':', SEP, ':'); break;
	case '-': c = follow('>', SEMI, follow('-', DECR, '-')); break;
	case '+': c = follow('+', INCR, '+'); break;
	case '<': c = follow('<', LSHIFT, follow('=', LE, LT)); break;
	case '>': c = follow('>', RSHIFT, follow('=', GE, GT)); break;
	case '=': c = follow('=', EQ, ASGN); break;
	case '!': c = follow('=', NE, follow('!', O_SND, SND)); break;
	case '?': c = follow('?', R_RCV, RCV); break;
	case '&': c = follow('&', AND, '&'); break;
	case '|': c = follow('|', OR, '|'); break;
	case ';': c = SEMI; break;
	default : break;
	}
	Token(c)
}

static struct {
	char *s;	int tok;	int val;	char *sym;
} Names[] = {
	{"active",	ACTIVE,		0,		0},
	{"assert",	ASSERT,		0,		0},
	{"atomic",	ATOMIC,		0,		0},
	{"bit",		TYPE,		BIT,		0},
	{"bool",	TYPE,		BIT,		0},
	{"break",	BREAK,		0,		0},
	{"byte",	TYPE,		BYTE,		0},
	{"D_proctype",	D_PROCTYPE,	0,		0},
	{"do",		DO,		0,		0},
	{"chan",	TYPE,		CHAN,		0},
	{"else", 	ELSE,		0,		0},
	{"empty",	EMPTY,		0,		0},
	{"enabled",	ENABLED,	0,		0},
	{"eval",	EVAL,		0,		0},
	{"false",	CONST,		0,		0},
	{"fi",		FI,		0,		0},
	{"full",	FULL,		0,		0},
	{"goto",	GOTO,		0,		0},
	{"hidden",	HIDDEN,		0,		":hide:"},
	{"if",		IF,		0,		0},
	{"init",	INIT,		0,		":init:"},
	{"int",		TYPE,		INT,		0},
	{"local",	ISLOCAL,	0,		":local:"},
	{"len",		LEN,		0,		0},
	{"mtype",	TYPE,		MTYPE,		0},
	{"nempty",	NEMPTY,		0,		0},
	{"never",	CLAIM,		0,		":never:"},
	{"nfull",	NFULL,		0,		0},
	{"notrace",	TRACE,		0,		":notrace:"},
	{"np_",		NONPROGRESS,	0,		0},
	{"od",		OD,		0,		0},
	{"of",		OF,		0,		0},
	{"pc_value",	PC_VAL,		0,		0},
	{"printf",	PRINT,		0,		0},
	{"priority",	PRIORITY,	0,		0},
	{"proctype",	PROCTYPE,	0,		0},
	{"provided",	PROVIDED,	0,		0},
	{"run",		RUN,		0,		0},
	{"d_step",	D_STEP,		0,		0},
	{"inline",	INLINE,		0,		0},
	{"short",	TYPE,		SHORT,		0},
	{"skip",	CONST,		1,		0},
	{"timeout",	TIMEOUT,	0,		0},
	{"trace",	TRACE,		0,		":trace:"},
	{"true",	CONST,		1,		0},
	{"show",	SHOW,		0,		":show:"},
	{"typedef",	TYPEDEF,	0,		0},
	{"unless",	UNLESS,		0,		0},
	{"unsigned",	TYPE,		UNSIGNED,	0},
	{"xr",		XU,		XR,		0},
	{"xs",		XU,		XS,		0},
	{0, 		0,		0,		0},
};

static int
check_name(char *s)
{	register int i;

	yylval = nn(ZN, 0, ZN, ZN);
	for (i = 0; Names[i].s; i++)
		if (strcmp(s, Names[i].s) == 0)
		{	yylval->val = Names[i].val;
			if (Names[i].sym)
				yylval->sym = lookup(Names[i].sym);
			return Names[i].tok;
		}

	if (yylval->val = ismtype(s))
	{	yylval->ismtyp = 1;
		return CONST;
	}

	if (strcmp(s, "_last") == 0)
		has_last++;

	if (Inlining >= 0 && !ReDiRect)
	{	Lextok *tt, *t = Inline_stub[Inlining]->params;
		for (i = 0; t; t = t->rgt, i++)
		 if (!strcmp(s, t->lft->sym->name)
		 &&   strcmp(s, Inline_stub[Inlining]->anms[i]) != 0)
		 {
#if 0
			if (verbose&32)
			printf("\tline %d, replace %s in call of '%s' with %s\n",
				lineno, s,
				Inline_stub[Inlining]->nm->name,
				Inline_stub[Inlining]->anms[i]);
#endif
			tt = Inline_stub[Inlining]->params;
			for ( ; tt; tt = tt->rgt)
			if (!strcmp(Inline_stub[Inlining]->anms[i],
				tt->lft->sym->name))
			{	/* would be cyclic if not caught */
				yylval->ntyp = tt->lft->ntyp;
				yylval->sym = lookup(tt->lft->sym->name);
				return NAME;
			}
			ReDiRect = Inline_stub[Inlining]->anms[i];
			return 0;
	}	 }

	yylval->sym = lookup(s);	/* symbol table */
	if (isutype(s))
		return UNAME;
	if (isproctype(s))
		return PNAME;
	if (iseqname(s))
		return INAME;

	return NAME;
}

int
yylex(void)
{	static int last = 0;
	static int hold = 0;
	int c;
	/*
	 * repair two common syntax mistakes with
	 * semi-colons before or after a '}'
	 */
	if (hold)
	{	c = hold;
		hold = 0;
	} else
	{	c = lex();
		if (last == ELSE
		&&  c != SEMI
		&&  c != FI)
		{	hold = c;
			last = 0;
			return SEMI;
		}
		if (last == '}'
		&&  c != PROCTYPE
		&&  c != INIT
		&&  c != CLAIM
		&&  c != SEP
		&&  c != FI
		&&  c != OD
		&&  c != '}'
		&&  c != UNLESS
		&&  c != SEMI
		&&  c != EOF)
		{	hold = c;
			last = 0;
			return SEMI;	/* insert ';' */
		}
		if (c == SEMI)
		{	extern Symbol *context, *owner;
			/* if context, we're not in a typedef
			 * because they're global.
			 * if owner, we're at the end of a ref
			 * to a struct field -- prevent that the
			 * lookahead is interpreted as a field of
			 * the same struct...
			 */
			if (context) owner = ZS;
			hold = lex();	/* look ahead */
			if (hold == '}'
			||  hold == SEMI)
			{	c = hold; /* omit ';' */
				hold = 0;
			}
		}
	}
	last = c;

	if (IArgs)
	{	static int IArg_nst = 0;
		if (strcmp(yytext, ",") == 0)
		{	IArg_cont[++IArgno][0] = '\0';
		} else if (strcmp(yytext, "(") == 0)
		{	if (IArg_nst++ == 0)
			{	IArgno = 0;
				IArg_cont[0][0] = '\0';
			}
		} else if (strcmp(yytext, ")") == 0)
		{	IArg_nst--;
		} else
			strcat(IArg_cont[IArgno], yytext);
	}

	return c;
}

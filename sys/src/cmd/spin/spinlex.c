/***** spin: spinlex.c *****/

/* Copyright (c) 1991,1995 by AT&T Corporation.  All Rights Reserved.     */
/* This software is for educational purposes only.                        */
/* Permission is given to distribute this code provided that this intro-  */
/* ductory message is not removed and no monies are exchanged.            */
/* No guarantee is expressed or implied by the distribution of this code. */
/* Software written by Gerard J. Holzmann as part of the book:            */
/* `Design and Validation of Computer Protocols,' ISBN 0-13-539925-4,     */
/* Prentice Hall, Englewood Cliffs, NJ, 07632.                            */
/* Send bug-reports and/or questions to: gerard@research.att.com          */

#include "spin.h"
#include "y.tab.h"

int		lineno=1;
unsigned char	in_comment=0;
char		yytext[512];
FILE		*yyin  = {stdin};
FILE		*yyout = {stdout};

extern Symbol	*Fname;
extern YYSTYPE	yylval;
extern int	has_last;

extern int	isdigit(int);
extern int	isalpha(int);
extern int	isalnum(int);

#define Token(y)	{ yylval = nn(ZN,0,ZN,ZN); \
			if (!in_comment) return y; else goto again; }

#define ValToken(x, y)	{ yylval = nn(ZN,0,ZN,ZN); yylval->val = x; \
			if (!in_comment) return y; else goto again; }

#define SymToken(x, y)	{ yylval = nn(ZN,0,ZN,ZN); yylval->sym = x; \
			if (!in_comment) return y; else goto again; }

#define Getchar()	getc(yyin)
#define Ungetch(c)	ungetc(c, yyin)

int
notquote(int c)
{	return (c != '\"' && c != '\n');
}

int
isalnum_(int c)
{	return (isalnum(c) || c == '_');
}

void
getword(int first, int (*tst)(int))
{	int i=0; char c;

	yytext[i++]= first;
	while (tst(c = Getchar()))
		yytext[i++] = c;
	yytext[i] = '\0';
	Ungetch(c);
}

int
follow(int tok, int ifyes, int ifno)
{	int c;

	if ((c = Getchar()) == tok)
		return ifyes;
	Ungetch(c);

	return ifno;
}

int
lex(void)
{	int c;

again:
	c = Getchar();
	yytext[0] = c;
	yytext[1] = '\0';
	switch (c) {
	case '\n':		/* newline */
		lineno++;
		goto again;

	case  ' ': case '\t':	/* white space */
		goto again;

	case '#':		/* preprocessor directive */
		getword(c, isalpha);
		if (Getchar() != ' ')
			fatal("malformed preprocessor directive - # .", 0);
		if (!isdigit(c = Getchar()))
			fatal("malformed preprocessor directive - # .lineno", 0);
		getword(c, isdigit);
		lineno = atoi(yytext);		/* removed -1 */
		c = Getchar();
		if (c == '\n') goto again;	/* no filename */

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
		goto again;

	case '\"':
		getword(c, notquote);
		if (Getchar() != '\"')
			fatal("string not terminated", yytext);
		strcat(yytext, "\"");
		SymToken(lookup(yytext), STRING);

	default:
		break;
	}

	if (isdigit(c))
	{	getword(c, isdigit);
		ValToken(atoi(yytext), CONST);
	}

	if (isalpha(c) || c == '_')
	{	getword(c, isalnum_);
		if (!in_comment)
			return check_name(yytext);
		else goto again;
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
	Token(c);
}

static struct {
	char *s;	int tok;	int val;	char *sym;
} Names[] = {
	"active",	ACTIVE,		0,		0,
	"assert",	ASSERT,		0,		0,
	"atomic",	ATOMIC,		0,		0,
	"bit",		TYPE,		BIT,		0,
	"bool",		TYPE,		BIT,		0,
	"break",	BREAK,		0,		0,
	"byte",		TYPE,		BYTE,		0,
	"do",		DO,		0,		0,
	"chan",		TYPE,		CHAN,		0,
	"else", 	ELSE,		0,		0,
	"empty",	EMPTY,		0,		0,
	"enabled",	ENABLED,	0,		0,
	"fi",		FI,		0,		0,
	"full",		FULL,		0,		0,
	"goto",		GOTO,		0,		0,
	"hidden",	HIDDEN,		0,		0,
	"if",		IF,		0,		0,
	"init",		INIT,		0,		":init:",
	"int",		TYPE,		INT,		0,
	"len",		LEN,		0,		0,
	"mtype",	MTYPE,		0,		0,
	"nempty",	NEMPTY,		0,		0,
	"never",	CLAIM,		0,		":never:",
	"nfull",	NFULL,		0,		0,
	"od",		OD,		0,		0,
	"of",		OF,		0,		0,
	"pc_value",	PC_VAL,		0,		0,
	"printf",	PRINT,		0,		0,
	"proctype",	PROCTYPE,	0,		0,
	"run",		RUN,		0,		0,
	"d_step",	D_STEP,		0,		0,
	"short",	TYPE,		SHORT,		0,
	"skip",		CONST,		1,		0,
	"timeout",	TIMEOUT,	0,		0,
	"typedef",	TYPEDEF,	0,		0,
	"unless",	UNLESS,		0,		0,
	"xr",		XU,		XR,		0,
	"xs",		XU,		XS,		0,
	0, 		0,		0,		0,
};

int
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

	yylval->sym = lookup(s);	/* symbol table */

	if (isutype(s))
		return UNAME;
	if (isproctype(s))
		return PNAME;

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
	return c;
}

/***** tl_spin: tl_lex.c *****/

/* Copyright (c) 1995-2003 by Lucent Technologies, Bell Laboratories.     */
/* All Rights Reserved.  This software is for educational purposes only.  */
/* No guarantee whatsoever is expressed or implied by the distribution of */
/* this code.  Permission is given to distribute this code provided that  */
/* this introductory message is not removed and no monies are exchanged.  */
/* Software written by Gerard J. Holzmann.  For tool documentation see:   */
/*             http://spinroot.com/                                       */
/* Send all bug-reports and/or questions to: bugs@spinroot.com            */

/* Based on the translation algorithm by Gerth, Peled, Vardi, and Wolper, */
/* presented at the PSTV Conference, held in 1995, Warsaw, Poland 1995.   */

#include <stdlib.h>
#include <ctype.h>
#include "tl.h"

static Symbol	*symtab[Nhash+1];
static int	tl_lex(void);

extern YYSTYPE	tl_yylval;
extern char	yytext[];

#define Token(y)        tl_yylval = tl_nn(y,ZN,ZN); return y

static void
tl_getword(int first, int (*tst)(int))
{	int i=0; char c;

	yytext[i++] = (char ) first;
	while (tst(c = tl_Getchar()))
		yytext[i++] = c;
	yytext[i] = '\0';
	tl_UnGetchar();
}

static int
tl_follow(int tok, int ifyes, int ifno)
{	int c;
	char buf[32];
	extern int tl_yychar;

	if ((c = tl_Getchar()) == tok)
		return ifyes;
	tl_UnGetchar();
	tl_yychar = c;
	sprintf(buf, "expected '%c'", tok);
	tl_yyerror(buf);	/* no return from here */
	return ifno;
}

int
tl_yylex(void)
{	int c = tl_lex();
#if 0
	printf("c = %d\n", c);
#endif
	return c;
}

static int
tl_lex(void)
{	int c;

	do {
		c = tl_Getchar();
		yytext[0] = (char ) c;
		yytext[1] = '\0';

		if (c <= 0)
		{	Token(';');
		}

	} while (c == ' ');	/* '\t' is removed in tl_main.c */

	if (islower(c))
	{	tl_getword(c, isalnum_);
		if (strcmp("true", yytext) == 0)
		{	Token(TRUE);
		}
		if (strcmp("false", yytext) == 0)
		{	Token(FALSE);
		}
		tl_yylval = tl_nn(PREDICATE,ZN,ZN);
		tl_yylval->sym = tl_lookup(yytext);
		return PREDICATE;
	}
	if (c == '<')
	{	c = tl_Getchar();
		if (c == '>')
		{	Token(EVENTUALLY);
		}
		if (c != '-')
		{	tl_UnGetchar();
			tl_yyerror("expected '<>' or '<->'");
		}
		c = tl_Getchar();
		if (c == '>')
		{	Token(EQUIV);
		}
		tl_UnGetchar();
		tl_yyerror("expected '<->'");
	}

	switch (c) {
	case '/' : c = tl_follow('\\', AND, '/'); break;
	case '\\': c = tl_follow('/', OR, '\\'); break;
	case '&' : c = tl_follow('&', AND, '&'); break;
	case '|' : c = tl_follow('|', OR, '|'); break;
	case '[' : c = tl_follow(']', ALWAYS, '['); break;
	case '-' : c = tl_follow('>', IMPLIES, '-'); break;
	case '!' : c = NOT; break;
	case 'U' : c = U_OPER; break;
	case 'V' : c = V_OPER; break;
#ifdef NXT
	case 'X' : c = NEXT; break;
#endif
	default  : break;
	}
	Token(c);
}

Symbol *
tl_lookup(char *s)
{	Symbol *sp;
	int h = hash(s);

	for (sp = symtab[h]; sp; sp = sp->next)
		if (strcmp(sp->name, s) == 0)
			return sp;

	sp = (Symbol *) tl_emalloc(sizeof(Symbol));
	sp->name = (char *) tl_emalloc((int) strlen(s) + 1);
	strcpy(sp->name, s);
	sp->next = symtab[h];
	symtab[h] = sp;

	return sp;
}

Symbol *
getsym(Symbol *s)
{	Symbol *n = (Symbol *) tl_emalloc(sizeof(Symbol));

	n->name = s->name;
	return n;
}

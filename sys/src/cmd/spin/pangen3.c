/***** spin: pangen3.c *****/

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
#include <ctype.h>
#include "y.tab.h"

extern FILE	*th;

typedef struct SRC {
	short ln, st;
	struct SRC *nxt;
} SRC;

SRC	*frst = (SRC *) 0;
SRC	*skip = (SRC *) 0;
int	col;

extern	int claimnr;

void
putskip(int m)	/* states that need not be reached */
{	SRC *tmp;

	for (tmp = skip; tmp; tmp = tmp->nxt)
		if (tmp->st == m)
			return;
	tmp = (SRC *) emalloc(sizeof(SRC));
	tmp->st = (short) m;
	tmp->nxt = skip;
	skip = tmp;
}

void
unskip(int m)	/* a state that needs to be reached after all */
{	SRC *tmp, *lst=(SRC *)0;

	for (tmp = skip; tmp; lst = tmp, tmp = tmp->nxt)
		if (tmp->st == m)
		{	if (tmp == skip)
				skip = skip->nxt;
			else
				lst->nxt = tmp->nxt;
			break;
		}
}

void
putsrc(int n, int m)	/* match states to source lines */
{	SRC *tmp;

	for (tmp = frst; tmp; tmp = tmp->nxt)
		if (tmp->st == m)
		{	if (tmp->ln != n)
			printf("putsrc mismatch %d - %d\n", n, tmp->ln);
			return;
		}
	tmp = (SRC *) emalloc(sizeof(SRC));
	tmp->ln = (short) n;
	tmp->st = (short) m;
	tmp->nxt = frst;
	frst = tmp;
}

void
dumpskip(int n, int m)
{	SRC *tmp, *lst;
	int j;

	fprintf(th, "uchar reached%d [] = {\n\t", m);
	for (j = 0, col = 0; j <= n; j++)
	{	lst = (SRC *) 0;
		for (tmp = skip; tmp; lst = tmp, tmp = tmp->nxt)
			if (tmp->st == j)
			{	putnr(1);
				if (lst)
					lst->nxt = tmp->nxt;
				else
					skip = tmp->nxt;
				break;
			}
		if (!tmp)
			putnr(0);
	}
	fprintf(th, "};\n");
	if (m == claimnr)
		fprintf(th, "#define reached_claim	reached%d\n", m);

	skip = (SRC *) 0;
}

void
dumpsrc(int n, int m)
{	SRC *tmp, *lst;
	int j;

	fprintf(th, "short src_ln%d [] = {\n\t", m);
	for (j = 0, col = 0; j <= n; j++)
	{	lst = (SRC *) 0;
		for (tmp = frst; tmp; lst = tmp, tmp = tmp->nxt)
			if (tmp->st == j)
			{	putnr(tmp->ln);
				if (lst)
					lst->nxt = tmp->nxt;
				else
					frst = tmp->nxt;
				break;
			}
		if (!tmp)
			putnr(0);
	}
	fprintf(th, "};\n");

	if (m == claimnr)
		fprintf(th, "#define src_claim	src_ln%d\n", m);

	frst = (SRC *) 0;
	dumpskip(n, m);
}

void
putnr(int n)
{
	if (col++ == 8)
	{	fprintf(th, "\n\t");
		col = 1;
	}
	fprintf(th, "%3d, ", n);
}

#define Cat0(x)   	comwork(fd,now->lft,m); fprintf(fd, x); \
			comwork(fd,now->rgt,m)
#define Cat1(x)		fprintf(fd,"("); Cat0(x); fprintf(fd,")")
#define Cat2(x,y)  	fprintf(fd,x); comwork(fd,y,m)
#define Cat3(x,y,z)	fprintf(fd,x); comwork(fd,y,m); fprintf(fd,z)

int
symbolic(FILE *fd, Lextok *tv)
{	Lextok *n; extern Lextok *Mtype;
	int cnt = 1;

	if (tv->ismtyp)
	for (n = Mtype; n; n = n->rgt, cnt++)
		if (cnt == tv->val)
		{	fprintf(fd, "%s", n->lft->sym->name);
			return 1;
		}
	return 0;
}

void
comwork(FILE *fd, Lextok *now, int m)
{	Lextok *v;
	int i, j; extern int Mpars;

	if (!now) { fprintf(fd, "0"); return; }
	switch (now->ntyp) {
	case CONST:	fprintf(fd, "%d", now->val); break;
	case '!':	Cat3("!(", now->lft, ")"); break;
	case UMIN:	Cat3("-(", now->lft, ")"); break;
	case '~':	Cat3("~(", now->lft, ")"); break;

	case '/':	Cat1("/");  break;
	case '*':	Cat1("*");  break;
	case '-':	Cat1("-");  break;
	case '+':	Cat1("+");  break;
	case '%':	Cat1("%%"); break;
	case '&':	Cat1("&");  break;
	case '|':	Cat1("|");  break;
	case LE:	Cat1("<="); break;
	case GE:	Cat1(">="); break;
	case GT:	Cat1(">"); break;
	case LT:	Cat1("<"); break;
	case NE:	Cat1("!="); break;
	case EQ:	Cat1("=="); break;
	case OR:	Cat1("||"); break;
	case AND:	Cat1("&&"); break;
	case LSHIFT:	Cat1("<<"); break;
	case RSHIFT:	Cat1(">>"); break;

	case RUN:	fprintf(fd, "run %s(", now->sym->name);
			for (v = now->lft; v; v = v->rgt)
				if (v == now->lft)
				{	Cat2("", v->lft);
				} else
				{	Cat2(",", v->lft);
				}
			fprintf(fd, ")");
			break;

	case LEN:	putname(fd, "len(", now->lft, m, ")");
			break;
	case FULL:	putname(fd, "full(", now->lft, m, ")");
			break;
	case EMPTY:	putname(fd, "empty(", now->lft, m, ")");
			break;
	case NFULL:	putname(fd, "nfull(", now->lft, m, ")");
			break;
	case NEMPTY:	putname(fd, "nempty(", now->lft, m, ")");
			break;

	case 's':	putname(fd, "", now->lft, m, now->val?"!!":"!");
			for (v = now->rgt, i=0; v; v = v->rgt, i++)
			{	if (v != now->rgt) fprintf(fd,",");
				if (!symbolic(fd, v->lft))
					comwork(fd,v->lft,m);
			}
			break;
	case 'r':	putname(fd, "", now->lft, m, now->val?"??":"?");
			for (v = now->rgt, i=0; v; v = v->rgt, i++)
			{	if (v != now->rgt) fprintf(fd,",");
				if (!symbolic(fd, v->lft))
					comwork(fd,v->lft,m);
			}
			break;
	case 'R':	putname(fd, "", now->lft, m,  now->val?"??[":"?[");
			for (v = now->rgt, i=0; v; v = v->rgt, i++)
			{	if (v != now->rgt) fprintf(fd,",");
				if (!symbolic(fd, v->lft))
					comwork(fd,v->lft,m);
			}
			fprintf(fd, "]");
			break;

	case ENABLED:	Cat3("enabled(", now->lft, ")");
			break;

	case PC_VAL:	Cat3("pc_value(", now->lft, ")");
			break;

	case 'c':	Cat3("(", now->lft, ")");
			break;

	case '?':	Cat3("( (", now->lft, ") -> ");
			Cat3("(", now->rgt->lft, ") : ");
			Cat3("(", now->rgt->rgt, ") )");
			break;	

	case ASGN:	comwork(fd,now->lft,m);
			fprintf(fd," = ");
			comwork(fd,now->rgt,m);
			break;

	case PRINT:	{	char c, buf[512];
				strncpy(buf, now->sym->name, 510);
				for (i = j = 0; i < 510; i++, j++)
				{	c = now->sym->name[i];
					buf[j] = c;
					if (c == '\\') buf[++j] = c;
					if (c == '\"') buf[j] = '\'';
					if (c == '\0') break;
				}
				fprintf(fd, "printf(%s", buf);
			}
			for (v = now->lft; v; v = v->rgt)
			{	Cat2(",", v->lft);
			}
			fprintf(fd, ")");
			break;
	case NAME:	putname(fd, "", now, m, "");
			break;
	case   'p':	putremote(fd, now, m);
			break;
	case   'q':	fprintf(fd, "%s", now->sym->name);
			break;
	case ASSERT:	Cat3("assert(", now->lft, ")");
			break;
	case   '.':	fprintf(fd, ".(goto)"); break;
	case  GOTO:	fprintf(fd, "goto"); break;
	case BREAK:	fprintf(fd, "break"); break;
	case  ELSE:	fprintf(fd, "else"); break;
	case   '@':	fprintf(fd, "-end-"); break;

	case D_STEP:	fprintf(fd, "D_STEP"); break;
	case ATOMIC:	fprintf(fd, "ATOMIC"); break;
	case NON_ATOMIC: fprintf(fd, "sub-sequence"); break;
	case IF:	fprintf(fd, "IF"); break;
	case DO:	fprintf(fd, "DO"); break;
	case UNLESS:	fprintf(fd, "unless"); break;
	case TIMEOUT:	fprintf(fd, "timeout"); break;
	default:	if (isprint(now->ntyp))
				fprintf(fd, "'%c'", now->ntyp);
			else
				fprintf(fd, "%d", now->ntyp);
			break;
	}
}

void
comment(FILE *fd, Lextok *now, int m)
{	extern int terse, nocast;

	terse=nocast=1;
	comwork(fd, now, m);
	terse=nocast=0;
}

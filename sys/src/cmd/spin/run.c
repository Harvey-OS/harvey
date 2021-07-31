/***** spin: run.c *****/

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

extern Symbol	*Fname;
extern Element	*LastStep;
extern int	Rvous, lineno, Tval, Interactive, SubChoice;
extern int	TstOnly, verbose, s_trail, xspin;

static long Seed=1;

void
Srand(unsigned int s)
{	Seed = s;
}

long
Rand(void)
{	/* CACM 31(10), Oct 1988 */
	Seed = 16807*(Seed%127773) - 2836*(Seed/127773);
	if (Seed <= 0) Seed += 2147483647;
	return Seed;
}

Element *
eval_sub(Element *e)
{	Element *f, *g;
	SeqList *z;
	int i, j, k;

	if (!e->n)
		return ZE;
#ifdef DEBUG
	printf("eval_sub(%d) ", e->Seqno);
	comment(stdout, e->n, 0);
	printf("\n");
#endif
	if (e->n->ntyp == GOTO)
	{	if (Rvous) return ZE;
		f = get_lab(e->n, 1);
		cross_dsteps(e->n, f->n);
		return f;
	}
	if (e->n->ntyp == UNLESS)
	{	/* escapes were distributed into sequence */
		return eval_sub(e->sub->this->frst);
	} else if (e->sub)	/* true for IF, DO, and UNLESS */
	{	Element *has_else = ZE;

		if (Interactive && !SubChoice)
		{	printf("Select stmnt (");
			whoruns(0); printf(")\n");
			printf("\tchoice 0: other process\n");
		}
		for (z = e->sub, j=0; z; z = z->nxt)
		{	j++;
			if (Interactive
			&& !SubChoice
			&& z->this->frst
			&& (xspin || Enabled0(z->this->frst)))
			{	printf("\tchoice %d: ", j);
				if (!Enabled0(z->this->frst))
					printf("unexecutable, ");
				comment(stdout, z->this->frst->n, 0);
				printf("\n");
		}	}
		if (Interactive)
		{	if (!SubChoice)
			{	if (xspin)
					printf("Make Selection %d\n\n", j);
				else
					printf("Select [0-%d]: ", j);
				fflush(stdout);
				scanf("%d", &k);
			} else
			{	k = SubChoice;
				SubChoice = 0;
			}
			if (k < 1 || k > j)
			{	printf("\tchoice outside range\n");
				return ZE;
			}
			k--;
		} else
			k = Rand()%j;	/* nondeterminism */
		for (i = 0, z = e->sub; i < j+k; i++)
		{	if (z->this->frst
			&&  z->this->frst->n->ntyp == ELSE)
			{	has_else = z->this->frst->nxt;
				if (!Interactive)
				{	z = (z->nxt)?z->nxt:e->sub;
					continue;
			}	}
			if (i >= k)
			{	if (f = eval_sub(z->this->frst))
					return f;
				else if (Interactive)
				{	printf("\tunexecutable\n");
					return ZE;
			}	}
			z = (z->nxt)?z->nxt:e->sub;
		}
		LastStep = z->this->frst;
		return has_else;
	} else
	{	if (e->n->ntyp == ATOMIC
		||  e->n->ntyp == D_STEP)
		{	f = e->n->sl->this->frst;
			g = e->n->sl->this->last;
			g->nxt = e->nxt;
			if (!(g = eval_sub(f)))	/* atomic guard */
				return ZE;
			/* new implementation of atomic sequences */
			return g;
		} else if (e->n->ntyp == NON_ATOMIC)
		{	f = e->n->sl->this->frst;
			g = e->n->sl->this->last;
			g->nxt = e->nxt;		/* close it */
			return eval_sub(f);
		} else if (Rvous)
		{	if (eval_sync(e))
				return e->nxt;
		} else if (e->n->ntyp == '.')
		{	return e->nxt;
		} else
		{	SeqList *x;
			if (e->esc && verbose&32)
			{	printf("Statement [");
				comment(stdout, e->n, 0);
				printf("] - can be escaped by\n");
				for (x = e->esc; x; x = x->nxt)
				{	printf("\t[");
					comment(stdout, x->this->frst->n, 0);
					printf("]\n");
			}	}
			for (x = e->esc; x; x = x->nxt)
			{	if (g = eval_sub(x->this->frst))
				{	if (verbose&4)
						printf("\tEscape taken\n");
					return g;
			}	}
			switch (e->n->ntyp) {
			case TIMEOUT: case RUN:
			case PRINT: case ASGN: case ASSERT:
			case 's': case 'r': case 'c':
				/* toplevel statements only */
				LastStep = e;
			default:
				break;
			}
			return (eval(e->n))?e->nxt:ZE;
		}
	}
	return ZE;
}

int
eval_sync(Element *e)
{	/* allow only synchronous receives
	/* and related node types    */
	Lextok *now = (e)?e->n:ZN;

	if (!now
	||  now->ntyp != 'r'
	||  !q_is_sync(now))
		return 0;

	LastStep = e;
	return eval(now);
}

int
assign(Lextok *now)
{	int t;

	if (TstOnly) return 1;

	switch (now->rgt->ntyp) {
	case FULL:	case NFULL:
	case EMPTY:	case NEMPTY:
	case RUN:	case LEN:
		t = BYTE;
		break;
	default:
		t = Sym_typ(now->rgt);
		break;
	}
	typ_ck(Sym_typ(now->lft), t, "assignment"); 
	return setval(now->lft, eval(now->rgt));
}

int
eval(Lextok *now)
{
	if (now) {
	lineno = now->ln;
	Fname  = now->fn;
#ifdef DEBUG
	printf("eval ");
	comment(stdout, now, 0);
	printf("\n");
#endif
	switch (now->ntyp) {
	case CONST: return now->val;
	case   '!': return !eval(now->lft);
	case  UMIN: return -eval(now->lft);
	case   '~': return ~eval(now->lft);

	case   '/': return (eval(now->lft) / eval(now->rgt));
	case   '*': return (eval(now->lft) * eval(now->rgt));
	case   '-': return (eval(now->lft) - eval(now->rgt));
	case   '+': return (eval(now->lft) + eval(now->rgt));
	case   '%': return (eval(now->lft) % eval(now->rgt));
	case    LT: return (eval(now->lft) <  eval(now->rgt));
	case    GT: return (eval(now->lft) >  eval(now->rgt));
	case   '&': return (eval(now->lft) &  eval(now->rgt));
	case   '|': return (eval(now->lft) |  eval(now->rgt));
	case    LE: return (eval(now->lft) <= eval(now->rgt));
	case    GE: return (eval(now->lft) >= eval(now->rgt));
	case    NE: return (eval(now->lft) != eval(now->rgt));
	case    EQ: return (eval(now->lft) == eval(now->rgt));
	case    OR: return (eval(now->lft) || eval(now->rgt));
	case   AND: return (eval(now->lft) && eval(now->rgt));
	case LSHIFT: return (eval(now->lft) << eval(now->rgt));
	case RSHIFT: return (eval(now->lft) >> eval(now->rgt));
	case   '?': return (eval(now->lft) ? eval(now->rgt->lft)
					   : eval(now->rgt->rgt));

	case     'p': return remotevar(now);	/* only _p allowed */
	case     'q': return remotelab(now);
	case     'R': return qrecv(now, 0);	/* test only    */
	case     LEN: return qlen(now);
	case    FULL: return (qfull(now));
	case   EMPTY: return (qlen(now)==0);
	case   NFULL: return (!qfull(now));
	case  NEMPTY: return (qlen(now)>0);
	case ENABLED: return 1;	/* can only be hit with -t option*/
	case  PC_VAL: return pc_value(now->lft);
	case    NAME: return getval(now);

	case TIMEOUT: return Tval;
	case     RUN: return TstOnly?1:enable(now->sym, now->lft);

	case   's': return qsend(now);		/* send         */
	case   'r': return qrecv(now, 1);	/* full-receive */
	case   'c': return eval(now->lft);	/* condition    */
	case PRINT: return TstOnly?1:interprint(now);
	case  ASGN: return assign(now);
	case ASSERT: if (TstOnly || eval(now->lft)) return 1;
		     non_fatal("assertion violated", (char *) 0);
		     if (s_trail) return 1; /* else */
		     wrapup(1); /* doesn't return */

	case  IF: case DO: case BREAK: case UNLESS:	/* compound structure */
	case   '.': return 1;	/* return label for compound */
	case   '@': return 0;	/* stop state */
	case  ELSE: return 1;	/* only hit here in guided trails */
	default   : printf("spin: bad node type %d (run)\n", now->ntyp);
		    fatal("aborting", 0);
	}}
	return 0;
}

int
interprint(Lextok *n)
{	Lextok *tmp = n->lft;
	char c, *s = n->sym->name;
	int i, j;
	
	for (i = 0; i < strlen(s); i++)
		switch (s[i]) {
		default:   putchar(s[i]); break;
		case '\"': break; /* ignore */
		case '\\':
			 switch(s[++i]) {
			 case 't': putchar('\t'); break;
			 case 'n': putchar('\n'); break;
			 default:  putchar(s[i]); break;
			 }
			 break;
		case  '%':
			 if ((c = s[++i]) == '%')
			 {	putchar('%'); /* literal */
				break;
			 }
			 if (!tmp)
			 {	non_fatal("too few print args %s", s);
				break;
			 }
			 j = eval(tmp->lft);
			 tmp = tmp->rgt;
			 switch(c) {
			 case 'c': printf("%c", j); break;
			 case 'd': printf("%d", j); break;
			 case 'o': printf("%o", j); break;
			 case 'u': printf("%u", (unsigned) j); break;
			 case 'x': printf("%x", j); break;
			 default:  non_fatal("unrecognized print cmd: '%s'", &s[i-1]);
				   break;
			 }
			 break;
		}
	fflush(stdout);
	return 1;
}

/* new */

int
Enabled1(Lextok *n)
{	int i; int v = verbose;

	if (n)
	switch (n->ntyp) {
	default:	/* side-effect free */
		verbose = 0;
		i = eval(n);
		verbose = v;
		return i;

	case PRINT:  case  ASGN:
	case ASSERT: case   RUN:
		return 1;

	case 's':	/* guesses for rv */
		if (q_is_sync(n)) return !Rvous;
		return (!qfull(n));
	case 'r':	/* guesses for rv */
		if (q_is_sync(n)) return Rvous;
		n->ntyp = 'R'; verbose = 0;
		i = eval(n);
		n->ntyp = 'r'; verbose = v;
		return i;
	}
	return 0;
}

int
Enabled0(Element *e)
{	SeqList *z;

	if (!e || !e->n)
		return 0;

	switch (e->n->ntyp) {
	case '.':
		return 1;
	case GOTO:
		if (Rvous) return 0;
		return 1;
	case UNLESS:
		return Enabled0(e->sub->this->frst);
	case ATOMIC:
	case D_STEP:
	case NON_ATOMIC:
		return Enabled0(e->n->sl->this->frst);
	}
	if (e->sub)	/* true for IF, DO, and UNLESS */
	{	for (z = e->sub; z; z = z->nxt)
			if (Enabled0(z->this->frst))
				return 1;
		return 0;
	}
	for (z = e->esc; z; z = z->nxt)
	{	if (Enabled0(z->this->frst))
			return 1;
	}
	return Enabled1(e->n);
}

/***** spin: run.c *****/

/* Copyright (c) 1989-2003 by Lucent Technologies, Bell Laboratories.     */
/* All Rights Reserved.  This software is for educational purposes only.  */
/* No guarantee whatsoever is expressed or implied by the distribution of */
/* this code.  Permission is given to distribute this code provided that  */
/* this introductory message is not removed and no monies are exchanged.  */
/* Software written by Gerard J. Holzmann.  For tool documentation see:   */
/*             http://spinroot.com/                                       */
/* Send all bug-reports and/or questions to: bugs@spinroot.com            */

#include <stdlib.h>
#include "spin.h"
#include "y.tab.h"

extern RunList	*X, *run;
extern Symbol	*Fname;
extern Element	*LastStep;
extern int	Rvous, lineno, Tval, interactive, MadeChoice;
extern int	TstOnly, verbose, s_trail, xspin, jumpsteps, depth;
extern int	nproc, nstop, no_print, like_java;

static long	Seed = 1;
static int	E_Check = 0, Escape_Check = 0;

static int	eval_sync(Element *);
static int	pc_enabled(Lextok *n);
extern void	sr_buf(int, int);

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
rev_escape(SeqList *e)
{	Element *r;

	if (!e)
		return (Element *) 0;

	if ((r = rev_escape(e->nxt)) != ZE) /* reversed order */
		return r;

	return eval_sub(e->this->frst);		
}

Element *
eval_sub(Element *e)
{	Element *f, *g;
	SeqList *z;
	int i, j, k;

	if (!e->n)
		return ZE;
#ifdef DEBUG
	printf("\n\teval_sub(%d %s: line %d) ",
		e->Seqno, e->esc?"+esc":"", e->n?e->n->ln:0);
	comment(stdout, e->n, 0);
	printf("\n");
#endif
	if (e->n->ntyp == GOTO)
	{	if (Rvous) return ZE;
		LastStep = e; f = get_lab(e->n, 1);
		cross_dsteps(e->n, f->n);
		return f;
	}
	if (e->n->ntyp == UNLESS)
	{	/* escapes were distributed into sequence */
		return eval_sub(e->sub->this->frst);
	} else if (e->sub)	/* true for IF, DO, and UNLESS */
	{	Element *has_else = ZE;
		Element *bas_else = ZE;
		int nr_else = 0, nr_choices = 0;

		if (interactive
		&& !MadeChoice && !E_Check
		&& !Escape_Check
		&& !(e->status&(D_ATOM))
		&& depth >= jumpsteps)
		{	printf("Select stmnt (");
			whoruns(0); printf(")\n");
			if (nproc-nstop > 1)
			printf("\tchoice 0: other process\n");
		}
		for (z = e->sub, j=0; z; z = z->nxt)
		{	j++;
			if (interactive
			&& !MadeChoice && !E_Check
			&& !Escape_Check
			&& !(e->status&(D_ATOM))
			&& depth >= jumpsteps
			&& z->this->frst
			&& (xspin || (verbose&32) || Enabled0(z->this->frst)))
			{	if (z->this->frst->n->ntyp == ELSE)
				{	has_else = (Rvous)?ZE:z->this->frst->nxt;
					nr_else = j;
					continue;
				}
				printf("\tchoice %d: ", j);
#if 0
				if (z->this->frst->n)
					printf("line %d, ", z->this->frst->n->ln);
#endif
				if (!Enabled0(z->this->frst))
					printf("unexecutable, ");
				else
					nr_choices++;
				comment(stdout, z->this->frst->n, 0);
				printf("\n");
		}	}

		if (nr_choices == 0 && has_else)
			printf("\tchoice %d: (else)\n", nr_else);

		if (interactive && depth >= jumpsteps
		&& !Escape_Check
		&& !(e->status&(D_ATOM))
		&& !E_Check)
		{	if (!MadeChoice)
			{	char buf[256];
				if (xspin)
					printf("Make Selection %d\n\n", j);
				else
					printf("Select [0-%d]: ", j);
				fflush(stdout);
				scanf("%s", buf);
				if (isdigit(buf[0]))
					k = atoi(buf);
				else
				{	if (buf[0] == 'q')
						alldone(0);
					k = -1;
				}
			} else
			{	k = MadeChoice;
				MadeChoice = 0;
			}
			if (k < 1 || k > j)
			{	if (k != 0) printf("\tchoice outside range\n");
				return ZE;
			}
			k--;
		} else
		{	if (e->n && e->n->indstep >= 0)
				k = 0;	/* select 1st executable guard */
			else
				k = Rand()%j;	/* nondeterminism */
		}
		has_else = ZE;
		bas_else = ZE;
		for (i = 0, z = e->sub; i < j+k; i++)
		{	if (z->this->frst
			&&  z->this->frst->n->ntyp == ELSE)
			{	bas_else = z->this->frst;
				has_else = (Rvous)?ZE:bas_else->nxt;
				if (!interactive || depth < jumpsteps
				|| Escape_Check
				|| (e->status&(D_ATOM)))
				{	z = (z->nxt)?z->nxt:e->sub;
					continue;
				}
			}
			if (z->this->frst
			&&  ((z->this->frst->n->ntyp == ATOMIC
			  ||  z->this->frst->n->ntyp == D_STEP)
			  &&  z->this->frst->n->sl->this->frst->n->ntyp == ELSE))
			{	bas_else = z->this->frst->n->sl->this->frst;
				has_else = (Rvous)?ZE:bas_else->nxt;
				if (!interactive || depth < jumpsteps
				|| Escape_Check
				|| (e->status&(D_ATOM)))
				{	z = (z->nxt)?z->nxt:e->sub;
					continue;
				}
			}
			if (i >= k)
			{	if ((f = eval_sub(z->this->frst)) != ZE)
					return f;
				else if (interactive && depth >= jumpsteps
				&& !(e->status&(D_ATOM)))
				{	if (!E_Check && !Escape_Check)
						printf("\tunexecutable\n");
					return ZE;
			}	}
			z = (z->nxt)?z->nxt:e->sub;
		}
		LastStep = bas_else;
		return has_else;
	} else
	{	if (e->n->ntyp == ATOMIC
		||  e->n->ntyp == D_STEP)
		{	f = e->n->sl->this->frst;
			g = e->n->sl->this->last;
			g->nxt = e->nxt;
			if (!(g = eval_sub(f)))	/* atomic guard */
				return ZE;
			return g;
		} else if (e->n->ntyp == NON_ATOMIC)
		{	f = e->n->sl->this->frst;
			g = e->n->sl->this->last;
			g->nxt = e->nxt;		/* close it */
			return eval_sub(f);
		} else if (e->n->ntyp == '.')
		{	if (!Rvous) return e->nxt;
			return eval_sub(e->nxt);
		} else
		{	SeqList *x;
			if (!(e->status & (D_ATOM))
			&&  e->esc && verbose&32)
			{	printf("Stmnt [");
				comment(stdout, e->n, 0);
				printf("] has escape(s): ");
				for (x = e->esc; x; x = x->nxt)
				{	printf("[");
					g = x->this->frst;
					if (g->n->ntyp == ATOMIC
					||  g->n->ntyp == NON_ATOMIC)
						g = g->n->sl->this->frst;
					comment(stdout, g->n, 0);
					printf("] ");
				}
				printf("\n");
			}
#if 0
			if (!(e->status & D_ATOM))	/* escapes don't reach inside d_steps */
			/* 4.2.4: only the guard of a d_step can have an escape */
#endif
			{	Escape_Check++;
				if (like_java)
				{	if ((g = rev_escape(e->esc)) != ZE)
					{	if (verbose&4)
							printf("\tEscape taken\n");
						Escape_Check--;
						return g;
					}
				} else
				{	for (x = e->esc; x; x = x->nxt)
					{	if ((g = eval_sub(x->this->frst)) != ZE)
						{	if (verbose&4)
								printf("\tEscape taken\n");
							Escape_Check--;
							return g;
				}	}	}
				Escape_Check--;
			}
		
			switch (e->n->ntyp) {
			case TIMEOUT: case RUN:
			case PRINT: case PRINTM:
			case C_CODE: case C_EXPR:
			case ASGN: case ASSERT:
			case 's': case 'r': case 'c':
				/* toplevel statements only */
				LastStep = e;
			default:
				break;
			}
			if (Rvous)
			{
				return (eval_sync(e))?e->nxt:ZE;
			}
			return (eval(e->n))?e->nxt:ZE;
		}
	}
	return ZE; /* not reached */
}

static int
eval_sync(Element *e)
{	/* allow only synchronous receives
	   and related node types    */
	Lextok *now = (e)?e->n:ZN;

	if (!now
	||  now->ntyp != 'r'
	||  now->val >= 2	/* no rv with a poll */
	||  !q_is_sync(now))
	{
		return 0;
	}

	LastStep = e;
	return eval(now);
}

static int
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

static int
nonprogress(void)	/* np_ */
{	RunList	*r;

	for (r = run; r; r = r->nxt)
	{	if (has_lab(r->pc, 4))	/* 4=progress */
			return 0;
	}
	return 1;
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
	case   '^': return (eval(now->lft) ^  eval(now->rgt));
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

	case     'p': return remotevar(now);	/* _p for remote reference */
	case     'q': return remotelab(now);
	case     'R': return qrecv(now, 0);	/* test only    */
	case     LEN: return qlen(now);
	case    FULL: return (qfull(now));
	case   EMPTY: return (qlen(now)==0);
	case   NFULL: return (!qfull(now));
	case  NEMPTY: return (qlen(now)>0);
	case ENABLED: if (s_trail) return 1;
		      return pc_enabled(now->lft);
	case    EVAL: return eval(now->lft);
	case  PC_VAL: return pc_value(now->lft);
	case NONPROGRESS: return nonprogress();
	case    NAME: return getval(now);

	case TIMEOUT: return Tval;
	case     RUN: return TstOnly?1:enable(now);

	case   's': return qsend(now);		/* send         */
	case   'r': return qrecv(now, 1);	/* receive or poll */
	case   'c': return eval(now->lft);	/* condition    */
	case PRINT: return TstOnly?1:interprint(stdout, now);
	case PRINTM: return TstOnly?1:printm(stdout, now);
	case  ASGN: return assign(now);

	case C_CODE: printf("%s:\t", now->sym->name);
		     plunk_inline(stdout, now->sym->name, 0);
		     return 1; /* uninterpreted */

	case C_EXPR: printf("%s:\t", now->sym->name);
		     plunk_expr(stdout, now->sym->name);
		     printf("\n");
		     return 1; /* uninterpreted */

	case ASSERT: if (TstOnly || eval(now->lft)) return 1;
		     non_fatal("assertion violated", (char *) 0);
			printf("spin: text of failed assertion: assert(");
			comment(stdout, now->lft, 0);
			printf(")\n");
		     if (s_trail && !xspin) return 1;
		     wrapup(1); /* doesn't return */

	case  IF: case DO: case BREAK: case UNLESS:	/* compound */
	case   '.': return 1;	/* return label for compound */
	case   '@': return 0;	/* stop state */
	case  ELSE: return 1;	/* only hit here in guided trails */
	default   : printf("spin: bad node type %d (run)\n", now->ntyp);
		    if (s_trail) printf("spin: trail file doesn't match spec?\n");
		    fatal("aborting", 0);
	}}
	return 0;
}

int
printm(FILE *fd, Lextok *n)
{	extern char Buf[];
	int j;

	Buf[0] = '\0';
	if (!no_print)
	if (!s_trail || depth >= jumpsteps) {
		if (n->lft->ismtyp)
			j = n->lft->val;
		else
			j = eval(n->lft);
		Buf[0] = '\0';
		sr_buf(j, 1);
		dotag(fd, Buf);
	}
	return 1;
}

int
interprint(FILE *fd, Lextok *n)
{	Lextok *tmp = n->lft;
	char c, *s = n->sym->name;
	int i, j; char lbuf[512];
	extern char Buf[];
	char tBuf[4096];

	Buf[0] = '\0';
	if (!no_print)
	if (!s_trail || depth >= jumpsteps) {
	for (i = 0; i < (int) strlen(s); i++)
		switch (s[i]) {
		case '\"': break; /* ignore */
		case '\\':
			 switch(s[++i]) {
			 case 't': strcat(Buf, "\t"); break;
			 case 'n': strcat(Buf, "\n"); break;
			 default:  goto onechar;
			 }
			 break;
		case  '%':
			 if ((c = s[++i]) == '%')
			 {	strcat(Buf, "%"); /* literal */
				break;
			 }
			 if (!tmp)
			 {	non_fatal("too few print args %s", s);
				break;
			 }
			 j = eval(tmp->lft);
			 tmp = tmp->rgt;
			 switch(c) {
			 case 'c': sprintf(lbuf, "%c", j); break;
			 case 'd': sprintf(lbuf, "%d", j); break;

			 case 'e': strcpy(tBuf, Buf);	/* event name */
				   Buf[0] = '\0';
				   sr_buf(j, 1);
				   strcpy(lbuf, Buf);
				   strcpy(Buf, tBuf);
				   break;

			 case 'o': sprintf(lbuf, "%o", j); break;
			 case 'u': sprintf(lbuf, "%u", (unsigned) j); break;
			 case 'x': sprintf(lbuf, "%x", j); break;
			 default:  non_fatal("bad print cmd: '%s'", &s[i-1]);
				   lbuf[0] = '\0'; break;
			 }
			 goto append;
		default:
onechar:		 lbuf[0] = s[i]; lbuf[1] = '\0';
append:			 strcat(Buf, lbuf);
			 break;
		}
		dotag(fd, Buf);
	}
	if (strlen(Buf) > 4096) fatal("printf string too long", 0);
	return 1;
}

static int
Enabled1(Lextok *n)
{	int i; int v = verbose;

	if (n)
	switch (n->ntyp) {
	case 'c':
		if (has_typ(n->lft, RUN))
			return 1;	/* conservative */
		/* else fall through */
	default:	/* side-effect free */
		verbose = 0;
		E_Check++;
		i = eval(n);
		E_Check--;
		verbose = v;
		return i;

	case C_CODE: case C_EXPR:
	case PRINT: case PRINTM:
	case   ASGN: case ASSERT:
		return 1;

	case 's':
		if (q_is_sync(n))
		{	if (Rvous) return 0;
			TstOnly = 1; verbose = 0;
			E_Check++;
			i = eval(n);
			E_Check--;
			TstOnly = 0; verbose = v;
			return i;
		}
		return (!qfull(n));
	case 'r':
		if (q_is_sync(n))
			return 0;	/* it's never a user-choice */
		n->ntyp = 'R'; verbose = 0;
		E_Check++;
		i = eval(n);
		E_Check--;
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
	case '@':
		return X->pid == (nproc-nstop-1);
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
#if 0
	printf("enabled1 ");
	comment(stdout, e->n, 0);
	printf(" ==> %s\n", Enabled1(e->n)?"yes":"nope");
#endif
	return Enabled1(e->n);
}

int
pc_enabled(Lextok *n)
{	int i = nproc - nstop;
	int pid = eval(n);
	int result = 0;
	RunList *Y, *oX;

	if (pid == X->pid)
		fatal("used: enabled(pid=thisproc) [%s]", X->n->name);

	for (Y = run; Y; Y = Y->nxt)
		if (--i == pid)
		{	oX = X; X = Y;
			result = Enabled0(Y->pc);
			X = oX;
			break;
		}
	return result;
}

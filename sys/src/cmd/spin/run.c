/***** spin: run.c *****/

/*
 * This file is part of the public release of Spin. It is subject to the
 * terms in the LICENSE file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com
 */

#include <stdlib.h>
#include "spin.h"
#include "y.tab.h"

extern RunList	*X_lst, *run_lst;
extern Symbol	*Fname;
extern Element	*LastStep;
extern int	Rvous, lineno, Tval, interactive, MadeChoice, Priority_Sum;
extern int	TstOnly, verbose, s_trail, xspin, jumpsteps, depth;
extern int	analyze, nproc, nstop, no_print, like_java, old_priority_rules;
extern short	Have_claim;

static long	Seed = 1;
static int	E_Check = 0, Escape_Check = 0;

static int	eval_sync(Element *);
static int	pc_enabled(Lextok *n);
static int	get_priority(Lextok *n);
static void	set_priority(Lextok *n, Lextok *m);
extern void	sr_buf(int, int, const char *);

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
{	Element *r = (Element *) 0;

	if (e)
	{	if ((r = rev_escape(e->nxt)) == ZE) /* reversed order */
		{	r = eval_sub(e->this->frst);
	}	}

	return r;		
}

Element *
eval_sub(Element *e)
{	Element *f, *g;
	SeqList *z;
	int i, j, k, only_pos;

	if (!e || !e->n)
		return ZE;
#ifdef DEBUG
	printf("\n\teval_sub(%d %s: line %d) ",
		e->Seqno, e->esc?"+esc":"", e->n?e->n->ln:0);
	comment(stdout, e->n, 0);
	printf("\n");
#endif
	if (e->n->ntyp == GOTO)
	{	if (Rvous) return ZE;
		LastStep = e;
		f = get_lab(e->n, 1);
		f = huntele(f, e->status, -1); /* 5.2.3: was missing */
		cross_dsteps(e->n, f->n);
#ifdef DEBUG
		printf("GOTO leads to %d\n", f->seqno);
#endif
		return f;
	}
	if (e->n->ntyp == UNLESS)
	{	/* escapes were distributed into sequence */
		return eval_sub(e->sub->this->frst);
	} else if (e->sub)	/* true for IF, DO, and UNLESS */
	{	Element *has_else = ZE;
		Element *bas_else = ZE;
		int nr_else = 0, nr_choices = 0;
		only_pos = -1;

		if (interactive
		&& !MadeChoice && !E_Check
		&& !Escape_Check
		&& !(e->status&(D_ATOM))
		&& depth >= jumpsteps)
		{	printf("Select stmnt (");
			whoruns(0); printf(")\n");
			if (nproc-nstop > 1)
			{	printf("\tchoice 0: other process\n");
				nr_choices++;
				only_pos = 0;
		}	}
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
				{	nr_choices++;
					only_pos = j;
				}
				comment(stdout, z->this->frst->n, 0);
				printf("\n");
		}	}

		if (nr_choices == 0 && has_else)
		{	printf("\tchoice %d: (else)\n", nr_else);
			only_pos = nr_else;
		}

		if (nr_choices <= 1 && only_pos != -1 && !MadeChoice)
		{	MadeChoice = only_pos;
		}

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
				if (scanf("%64s", buf) <= 0)
				{	printf("no input\n");
					return ZE;
				}
				if (isdigit((int)buf[0]))
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
			&&  e->esc && (verbose&32))
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
#if 1
			if (!s_trail)	/* trail determines selections, new 5.2.5 */
#endif
			{	Escape_Check++;
				if (like_java)
				{	if ((g = rev_escape(e->esc)) != ZE)
					{	if (verbose&4)
						{	printf("\tEscape taken (-J) ");
							if (g->n && g->n->fn)
								printf("%s:%d", g->n->fn->name, g->n->ln);
							printf("\n");
						}
						Escape_Check--;
						return g;
					}
				} else
				{	for (x = e->esc; x; x = x->nxt)
					{	if ((g = eval_sub(x->this->frst)) != ZE)
						{	if (verbose&4)
							{	printf("\tEscape taken ");
								if (g->n && g->n->fn)
									printf("%s:%d", g->n->fn->name, g->n->ln);
								printf("\n");
							}
							Escape_Check--;
							return g;
				}	}	}
				Escape_Check--;
			}
			switch (e->n->ntyp) {
			case ASGN:
				if (check_track(e->n) == STRUCT) { break; }
				/* else fall thru */
			case TIMEOUT: case RUN:
			case PRINT: case PRINTM:
			case C_CODE: case C_EXPR:
			case ASSERT:
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

	for (r = run_lst; r; r = r->nxt)
	{	if (has_lab(r->pc, 4))	/* 4=progress */
			return 0;
	}
	return 1;
}

int
eval(Lextok *now)
{	int temp;

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

	case   '/': temp = eval(now->rgt);
		    if (temp == 0)
		    {	fatal("division by zero", (char *) 0);
		    }
		    return (eval(now->lft) / temp);
	case   '*': return (eval(now->lft) * eval(now->rgt));
	case   '-': return (eval(now->lft) - eval(now->rgt));
	case   '+': return (eval(now->lft) + eval(now->rgt));
	case   '%': temp = eval(now->rgt);
		    if (temp == 0)
		    {	fatal("taking modulo of zero", (char *) 0);
		    }
		    return (eval(now->lft) % temp);
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

	case GET_P: return get_priority(now->lft);
	case SET_P: set_priority(now->lft->lft, now->lft->rgt); return 1;

	case    EVAL:	if (now->lft->ntyp == ',')
			{	Lextok *fix = now->lft;
				do {					/* new */
					if (eval(fix->lft) == 0)	/* usertype6 */
					{	return 0;
					}
					fix = fix->rgt;
				} while (fix && fix->ntyp == ',');
				return 1;
			}
			return eval(now->lft);

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
	case  ASGN:
		if (check_track(now) == STRUCT) { return 1; }
		return assign(now);

	case C_CODE: if (!analyze)
		     {	printf("%s:\t", now->sym->name);
		     	plunk_inline(stdout, now->sym->name, 0, 1);
		     }
		     return 1; /* uninterpreted */

	case C_EXPR: if (!analyze)
		     {	printf("%s:\t", now->sym->name);
		     	plunk_expr(stdout, now->sym->name);
		     	printf("\n");
		     }
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

	case ',':	/* reached through option -A with array initializer */
	case 0:
		    return 0;	/* not great, but safe */

	default   : printf("spin: bad node type %d (run)\n", now->ntyp);
		    if (s_trail) printf("spin: trail file doesn't match spec?\n");
		    fatal("aborting", 0);
	}}
	return 0;
}

int
printm(FILE *fd, Lextok *n)
{	extern char GBuf[];
	char *s = 0;
	int j;

	GBuf[0] = '\0';
	if (!no_print)
	if (!s_trail || depth >= jumpsteps)
	{
		if (n->lft->sym
		&&  n->lft->sym->mtype_name)
		{	s = n->lft->sym->mtype_name->name;
		}

		if (n->lft->ismtyp)
		{	j = n->lft->val;
		} else	/* constant */
		{	j = eval(n->lft);
		}
		sr_buf(j, 1, s);
		dotag(fd, GBuf);
	}
	return 1;
}

int
interprint(FILE *fd, Lextok *n)
{	Lextok *tmp = n->lft;
	char c, *s = n->sym->name, *t = 0;
	int i, j; char lbuf[512]; /* matches value in sr_buf() */
	extern char GBuf[];	/* global, size 4096 */
	char tBuf[4096];	/* match size of global GBuf[] */

	GBuf[0] = '\0';
	if (!no_print)
	if (!s_trail || depth >= jumpsteps) {
	for (i = 0; i < (int) strlen(s); i++)
		switch (s[i]) {
		case '\"': break; /* ignore */
		case '\\':
			 switch(s[++i]) {
			 case 't': strcat(GBuf, "\t"); break;
			 case 'n': strcat(GBuf, "\n"); break;
			 default:  goto onechar;
			 }
			 break;
		case  '%':
			 if ((c = s[++i]) == '%')
			 {	strcat(GBuf, "%"); /* literal */
				break;
			 }
			 if (!tmp)
			 {	non_fatal("too few print args %s", s);
				break;
			 }
			 j = eval(tmp->lft);

			 if (c == 'e'
			 &&  tmp->lft
			 &&  tmp->lft->sym
			 &&  tmp->lft->sym->mtype_name)
			 {	t = tmp->lft->sym->mtype_name->name;
			 }

			 tmp = tmp->rgt;
			 switch(c) {
			 case 'c': sprintf(lbuf, "%c", j); break;
			 case 'd': sprintf(lbuf, "%d", j); break;

			 case 'e': strcpy(tBuf, GBuf);	/* event name */
				   GBuf[0] = '\0';

				   sr_buf(j, 1, t);

				   strcpy(lbuf, GBuf);
				   strcpy(GBuf, tBuf);
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
append:			 strcat(GBuf, lbuf);
			 break;
		}
		dotag(fd, GBuf);
	}
	if (strlen(GBuf) >= 4096) fatal("printf string too long", 0);
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

	case SET_P:
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
		return X_lst->pid == (nproc-nstop-1);
	case '.':
	case SET_P:
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

	if (pid == X_lst->pid)
		fatal("used: enabled(pid=thisproc) [%s]", X_lst->n->name);

	for (Y = run_lst; Y; Y = Y->nxt)
		if (--i == pid)
		{	oX = X_lst; X_lst = Y;
			result = Enabled0(X_lst->pc);
			X_lst = oX;
			break;
		}
	return result;
}

int
pc_highest(Lextok *n)
{	int i = nproc - nstop;
	int pid = eval(n);
	int target = 0, result = 1;
	RunList *Y, *oX;

	if (X_lst->prov && !eval(X_lst->prov))
	{	return 0; /* can't be highest unless fully enabled */
	}

	for (Y = run_lst; Y; Y = Y->nxt)
	{	if (--i == pid)
		{	target = Y->priority;
			break;
	}	}
if (0) printf("highest for pid %d @ priority = %d\n", pid, target);

	oX = X_lst;
	i = nproc - nstop;
	for (Y = run_lst; Y; Y = Y->nxt)
	{	i--;
if (0) printf("	pid %d @ priority %d\t", Y->pid, Y->priority);
		if (Y->priority > target)
		{	X_lst = Y;
if (0) printf("enabled: %s\n", Enabled0(X_lst->pc)?"yes":"nope");
if (0) printf("provided: %s\n", eval(X_lst->prov)?"yes":"nope");
			if (Enabled0(X_lst->pc) && (!X_lst->prov || eval(X_lst->prov)))
			{	result = 0;
				break;
		}	}
else
if (0) printf("\n");
	}
	X_lst = oX;

	return result;
}

int
get_priority(Lextok *n)
{	int i = nproc - nstop;
	int pid = eval(n);
	RunList *Y;

	if (old_priority_rules)
	{	return 1;
	}

	for (Y = run_lst; Y; Y = Y->nxt)
	{	if (--i == pid)
		{	return Y->priority;
	}	}
	return 0;
}

void
set_priority(Lextok *n, Lextok *p)
{	int i = nproc - nstop - Have_claim;
	int pid = eval(n);
	RunList *Y;

	if (old_priority_rules)
	{	return;
	}
	for (Y = run_lst; Y; Y = Y->nxt)
	{	if (--i == pid)
		{	Priority_Sum -= Y->priority;
			Y->priority = eval(p);
			Priority_Sum += Y->priority;
			if (1)
			{	printf("%3d: setting priority of proc %d (%s) to %d\n",
					depth, pid, Y->n->name, Y->priority);
	}	}	}
	if (verbose&32)
	{	printf("\tPid\tName\tPriority\n");
		for (Y = run_lst; Y; Y = Y->nxt)
		{	printf("\t%d\t%s\t%d\n",
				Y->pid,
				Y->n->name,
				Y->priority);
	}	}
}

/***** spin: sched.c *****/

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

extern int	verbose, s_trail, analyze, no_wrapup;
extern char	*claimproc, *eventmap, Buf[];
extern Ordered	*all_names;
extern Symbol	*Fname, *context;
extern int	lineno, nr_errs, dumptab, xspin, jumpsteps, columns;
extern int	u_sync, Elcnt, interactive, TstOnly, cutoff;
extern short	has_enabled;
extern int	limited_vis, old_scope_rules, product, nclaims;

RunList		*X   = (RunList  *) 0;
RunList		*run = (RunList  *) 0;
RunList		*LastX  = (RunList  *) 0; /* previous executing proc */
ProcList	*rdy = (ProcList *) 0;
Element		*LastStep = ZE;
int		nproc=0, nstop=0, Tval=0;
int		Rvous=0, depth=0, nrRdy=0, MadeChoice;
short		Have_claim=0, Skip_claim=0;

static int	Priority_Sum = 0;
static void	setlocals(RunList *);
static void	setparams(RunList *, ProcList *, Lextok *);
static void	talk(RunList *);

void
runnable(ProcList *p, int weight, int noparams)
{	RunList *r = (RunList *) emalloc(sizeof(RunList));

	r->n  = p->n;
	r->tn = p->tn;
	r->b  = p->b;
	r->pid = nproc++ - nstop + Skip_claim;

	if (!noparams && ((verbose&4) || (verbose&32)))
		printf("Starting %s with pid %d\n",
			p->n?p->n->name:"--", r->pid);

	if (!p->s)
		fatal("parsing error, no sequence %s",
			p->n?p->n->name:"--");

	r->pc = huntele(p->s->frst, p->s->frst->status, -1);
	r->ps = p->s;

	if (p->s->last)
		p->s->last->status |= ENDSTATE; /* normal end state */

	r->nxt = run;
	r->prov = p->prov;
	r->priority = weight;

	if (noparams) setlocals(r);
	Priority_Sum += weight;

	run = r;
}

ProcList *
ready(Symbol *n, Lextok *p, Sequence *s, int det, Lextok *prov, enum btypes b)
	/* n=name, p=formals, s=body det=deterministic prov=provided */
{	ProcList *r = (ProcList *) emalloc(sizeof(ProcList));
	Lextok *fp, *fpt; int j; extern int Npars;

	r->n = n;
	r->p = p;
	r->s = s;
	r->b = b;
	r->prov = prov;
	r->tn = nrRdy++;
	if (det != 0 && det != 1)
	{	fprintf(stderr, "spin: bad value for det (cannot happen)\n");
	}
	r->det = (short) det;
	r->nxt = rdy;
	rdy = r;

	for (fp  = p, j = 0;  fp;  fp = fp->rgt)
	for (fpt = fp->lft;  fpt; fpt = fpt->rgt)
		j++;	/* count # of parameters */
	Npars = max(Npars, j);

	return rdy;
}

int
find_maxel(Symbol *s)
{	ProcList *p;

	for (p = rdy; p; p = p->nxt)
		if (p->n == s)
			return p->s->maxel++;
	return Elcnt++;
}

static void
formdump(void)
{	ProcList *p;
	Lextok *f, *t;
	int cnt;

	for (p = rdy; p; p = p->nxt)
	{	if (!p->p) continue;
		cnt = -1;
		for (f = p->p; f; f = f->rgt)	/* types */
		for (t = f->lft; t; t = t->rgt)	/* formals */
		{	if (t->ntyp != ',')
				t->sym->Nid = cnt--;	/* overload Nid */
			else
				t->lft->sym->Nid = cnt--;
		}
	}
}

void
announce(char *w)
{
	if (columns)
	{	extern char Buf[];
		extern int firstrow;
		firstrow = 1;
		if (columns == 2)
		{	sprintf(Buf, "%d:%s",
			run->pid - Have_claim, run->n->name);
			pstext(run->pid - Have_claim, Buf);
		} else
			printf("proc %d = %s\n",
			run->pid - Have_claim, run->n->name);
		return;
	}

	if (dumptab
	||  analyze
	||  product
	||  s_trail
	|| !(verbose&4))
		return;

	if (w)
		printf("  0:	proc  - (%s) ", w);
	else
		whoruns(1);
	printf("creates proc %2d (%s)",
		run->pid - Have_claim,
		run->n->name);
	if (run->priority > 1)
		printf(" priority %d", run->priority);
	printf("\n");
}

#ifndef MAXP
#define MAXP	255	/* matches max nr of processes in verifier */
#endif

int
enable(Lextok *m)
{	ProcList *p;
	Symbol *s = m->sym;	/* proctype name */
	Lextok *n = m->lft;	/* actual parameters */

	if (m->val < 1) m->val = 1;	/* minimum priority */
	for (p = rdy; p; p = p->nxt)
		if (strcmp(s->name, p->n->name) == 0)
		{	if (nproc-nstop >= MAXP)
			{	printf("spin: too many processes (%d max)\n", MAXP);
				break;
			}
			runnable(p, m->val, 0);
			announce((char *) 0);
			setparams(run, p, n);
			setlocals(run); /* after setparams */
			return run->pid - Have_claim + Skip_claim; /* effective simu pid */
		}
	return 0; /* process not found */
}

void
check_param_count(int i, Lextok *m)
{	ProcList *p;
	Symbol *s = m->sym;	/* proctype name */
	Lextok *f, *t;		/* formal pars */
	int cnt = 0;

	for (p = rdy; p; p = p->nxt)
	{	if (strcmp(s->name, p->n->name) == 0)
		{	if (m->lft)	/* actual param list */
			{	lineno = m->lft->ln;
				Fname  = m->lft->fn;
			}
			for (f = p->p;   f; f = f->rgt) /* one type at a time */
			for (t = f->lft; t; t = t->rgt)	/* count formal params */
			{	cnt++;
			}
			if (i != cnt)
			{	printf("spin: saw %d parameters, expected %d\n", i, cnt);
				non_fatal("wrong number of parameters", "");
			}
			break;
	}	}
}

void
start_claim(int n)
{	ProcList *p;
	RunList  *r, *q = (RunList *) 0;

	for (p = rdy; p; p = p->nxt)
		if (p->tn == n && p->b == N_CLAIM)
		{	runnable(p, 1, 1);
			goto found;
		}
	printf("spin: couldn't find claim %d (ignored)\n", n);
	if (verbose&32)
	for (p = rdy; p; p = p->nxt)
		printf("\t%d = %s\n", p->tn, p->n->name);

	Skip_claim = 1;
	goto done;
found:
	/* move claim to far end of runlist, and reassign it pid 0 */
	if (columns == 2)
	{	extern char Buf[];
		depth = 0;
		sprintf(Buf, "%d:%s", 0, p->n->name);
		pstext(0, Buf);
		for (r = run; r; r = r->nxt)
		{	if (r->b != N_CLAIM)
			{	sprintf(Buf, "%d:%s", r->pid+1, r->n->name);
				pstext(r->pid+1, Buf);
	}	}	}

	if (run->pid == 0) return; /* it is the first process started */

	q = run; run = run->nxt;
	q->pid = 0; q->nxt = (RunList *) 0;	/* remove */
done:
	Have_claim = 1;
	for (r = run; r; r = r->nxt)
	{	r->pid = r->pid+Have_claim;	/* adjust */
		if (!r->nxt)
		{	r->nxt = q;
			break;
	}	}
}

int
f_pid(char *n)
{	RunList *r;
	int rval = -1;

	for (r = run; r; r = r->nxt)
		if (strcmp(n, r->n->name) == 0)
		{	if (rval >= 0)
			{	printf("spin: remote ref to proctype %s, ", n);
				printf("has more than one match: %d and %d\n",
					rval, r->pid);
			} else
				rval = r->pid;
		}
	return rval;
}

void
wrapup(int fini)
{
	limited_vis = 0;
	if (columns)
	{	extern void putpostlude(void);
		if (columns == 2) putpostlude();
		if (!no_wrapup)
		printf("-------------\nfinal state:\n-------------\n");
	}
	if (no_wrapup)
		goto short_cut;
	if (nproc != nstop)
	{	int ov = verbose;
		printf("#processes: %d\n", nproc-nstop - Have_claim + Skip_claim);
		verbose &= ~4;
		dumpglobals();
		verbose = ov;
		verbose &= ~1;	/* no more globals */
		verbose |= 32;	/* add process states */
		for (X = run; X; X = X->nxt)
			talk(X);
		verbose = ov;	/* restore */
	}
	printf("%d process%s created\n",
		nproc - Have_claim + Skip_claim,
		(xspin || nproc!=1)?"es":"");
short_cut:
	if (xspin) alldone(0);	/* avoid an abort from xspin */
	if (fini)  alldone(1);
}

static char is_blocked[256];

static int
p_blocked(int p)
{	int i, j;

	is_blocked[p%256] = 1;
	for (i = j = 0; i < nproc - nstop; i++)
		j += is_blocked[i];
	if (j >= nproc - nstop)
	{	memset(is_blocked, 0, 256);
		return 1;
	}
	return 0;
}

static Element *
silent_moves(Element *e)
{	Element *f;

	if (e->n)
	switch (e->n->ntyp) {
	case GOTO:
		if (Rvous) break;
		f = get_lab(e->n, 1);
		cross_dsteps(e->n, f->n);
		return f; /* guard against goto cycles */
	case UNLESS:
		return silent_moves(e->sub->this->frst);
	case NON_ATOMIC:
	case ATOMIC:
	case D_STEP:
		e->n->sl->this->last->nxt = e->nxt;
		return silent_moves(e->n->sl->this->frst);
	case '.':
		return silent_moves(e->nxt);
	}
	return e;
}

static RunList *
pickproc(RunList *Y)
{	SeqList *z; Element *has_else;
	short Choices[256];
	int j, k, nr_else = 0;

	if (nproc <= nstop+1)
	{	X = run;
		return NULL;
	}
	if (!interactive || depth < jumpsteps)
	{	/* was: j = (int) Rand()%(nproc-nstop); */
		if (Priority_Sum < nproc-nstop)
			fatal("cannot happen - weights", (char *)0);
		j = (int) Rand()%Priority_Sum;

		while (j - X->priority >= 0)
		{	j -= X->priority;
			Y = X;
			X = X->nxt;
			if (!X) { Y = NULL; X = run; }
		}
	} else
	{	int only_choice = -1;
		int no_choice = 0, proc_no_ch, proc_k;

		Tval = 0;	/* new 4.2.6 */
try_again:	printf("Select a statement\n");
try_more:	for (X = run, k = 1; X; X = X->nxt)
		{	if (X->pid > 255) break;

			Choices[X->pid] = (short) k;

			if (!X->pc
			||  (X->prov && !eval(X->prov)))
			{	if (X == run)
					Choices[X->pid] = 0;
				continue;
			}
			X->pc = silent_moves(X->pc);
			if (!X->pc->sub && X->pc->n)
			{	int unex;
				unex = !Enabled0(X->pc);
				if (unex)
					no_choice++;
				else
					only_choice = k;
				if (!xspin && unex && !(verbose&32))
				{	k++;
					continue;
				}
				printf("\tchoice %d: ", k++);
				p_talk(X->pc, 0);
				if (unex)
					printf(" unexecutable,");
				printf(" [");
				comment(stdout, X->pc->n, 0);
				if (X->pc->esc) printf(" + Escape");
				printf("]\n");
			} else {
			has_else = ZE;
			proc_no_ch = no_choice;
			proc_k = k;
			for (z = X->pc->sub, j=0; z; z = z->nxt)
			{	Element *y = silent_moves(z->this->frst);
				int unex;
				if (!y) continue;

				if (y->n->ntyp == ELSE)
				{	has_else = (Rvous)?ZE:y;
					nr_else = k++;
					continue;
				}

				unex = !Enabled0(y);
				if (unex)
					no_choice++;
				else
					only_choice = k;
				if (!xspin && unex && !(verbose&32))
				{	k++;
					continue;
				}
				printf("\tchoice %d: ", k++);
				p_talk(X->pc, 0);
				if (unex)
					printf(" unexecutable,");
				printf(" [");
				comment(stdout, y->n, 0);
				printf("]\n");
			}
			if (has_else)
			{	if (no_choice-proc_no_ch >= (k-proc_k)-1)
				{	only_choice = nr_else;
					printf("\tchoice %d: ", nr_else);
					p_talk(X->pc, 0);
					printf(" [else]\n");
				} else
				{	no_choice++;
					printf("\tchoice %d: ", nr_else);
					p_talk(X->pc, 0);
					printf(" unexecutable, [else]\n");
			}	}
		}	}
		X = run;
		if (k - no_choice < 2 && Tval == 0)
		{	Tval = 1;
			no_choice = 0; only_choice = -1;
			goto try_more;
		}
		if (xspin)
			printf("Make Selection %d\n\n", k-1);
		else
		{	if (k - no_choice < 2)
			{	printf("no executable choices\n");
				alldone(0);
			}
			printf("Select [1-%d]: ", k-1);
		}
		if (!xspin && k - no_choice == 2)
		{	printf("%d\n", only_choice);
			j = only_choice;
		} else
		{	char buf[256];
			fflush(stdout);
			if (scanf("%64s", buf) == 0)
			{	printf("\tno input\n");
				goto try_again;
			}
			j = -1;
			if (isdigit((int) buf[0]))
				j = atoi(buf);
			else
			{	if (buf[0] == 'q')
					alldone(0);
			}
			if (j < 1 || j >= k)
			{	printf("\tchoice is outside range\n");
				goto try_again;
		}	}
		MadeChoice = 0;
		Y = NULL;
		for (X = run; X; Y = X, X = X->nxt)
		{	if (!X->nxt
			||   X->nxt->pid > 255
			||   j < Choices[X->nxt->pid])
			{
				MadeChoice = 1+j-Choices[X->pid];
				break;
		}	}
	}
	return Y;
}

void
multi_claims(void)
{	ProcList *p, *q = NULL;

	if (nclaims > 1)
	{	printf("  the model contains %d never claims:", nclaims);
		for (p = rdy; p; p = p->nxt)
		{	if (p->b == N_CLAIM)
			{	printf("%s%s", q?", ":" ", p->n->name);
				q = p;
		}	}
		printf("\n");
		printf("  only one claim is used in a verification run\n");
		printf("  choose which one with ./pan -N name (defaults to -N %s)\n",
			q?q->n->name:"--");
	}
}

void
sched(void)
{	Element *e;
	RunList *Y = NULL;	/* previous process in run queue */
	RunList *oX;
	int go, notbeyond = 0;
#ifdef PC
	int bufmax = 100;
#endif
	if (dumptab)
	{	formdump();
		symdump();
		dumplabels();
		return;
	}

	if (has_enabled && u_sync > 0)
	{	printf("spin: error, cannot use 'enabled()' in ");
		printf("models with synchronous channels.\n");
		nr_errs++;
	}
	if (product)
	{	sync_product();
		alldone(0);
	}
	if (analyze)
	{	gensrc();
		multi_claims();
		return;
	}
	if (s_trail)
	{	match_trail();
		return;
	}
	if (claimproc)
	printf("warning: never claim not used in random simulation\n");
	if (eventmap)
	printf("warning: trace assertion not used in random simulation\n");

	X = run;
	Y = pickproc(Y);

	while (X)
	{	context = X->n;
		if (X->pc && X->pc->n)
		{	lineno = X->pc->n->ln;
			Fname  = X->pc->n->fn;
		}
		if (cutoff > 0 && depth >= cutoff)
		{	printf("-------------\n");
			printf("depth-limit (-u%d steps) reached\n", cutoff);
			break;
		}
#ifdef PC
		if (xspin && !interactive && --bufmax <= 0)
		{	int c; /* avoid buffer overflow on pc's */
			printf("spin: type return to proceed\n");
			fflush(stdout);
			c = getc(stdin);
			if (c == 'q') wrapup(0);
			bufmax = 100;
		}
#endif
		depth++; LastStep = ZE;
		oX = X;	/* a rendezvous could change it */
		go = 1;
		if (X->prov && X->pc
		&& !(X->pc->status & D_ATOM)
		&& !eval(X->prov))
		{	if (!xspin && ((verbose&32) || (verbose&4)))
			{	p_talk(X->pc, 1);
				printf("\t<<Not Enabled>>\n");
			}
			go = 0;
		}
		if (go && (e = eval_sub(X->pc)))
		{	if (depth >= jumpsteps
			&& ((verbose&32) || (verbose&4)))
			{	if (X == oX)
				if (!(e->status & D_ATOM) || (verbose&32)) /* no talking in d_steps */
				{	p_talk(X->pc, 1);
					printf("	[");
					if (!LastStep) LastStep = X->pc;
					comment(stdout, LastStep->n, 0);
					printf("]\n");
				}
				if (verbose&1) dumpglobals();
				if (verbose&2) dumplocal(X);

				if (!(e->status & D_ATOM))
				if (xspin)
					printf("\n");
			}
			if (oX != X
			||  (X->pc->status & (ATOM|D_ATOM)))		/* new 5.0 */
			{	e = silent_moves(e);
				notbeyond = 0;
			}
			oX->pc = e; LastX = X;

			if (!interactive) Tval = 0;
			memset(is_blocked, 0, 256);

			if (X->pc && (X->pc->status & (ATOM|L_ATOM))
			&&  (notbeyond == 0 || oX != X))
			{	if ((X->pc->status & L_ATOM))
					notbeyond = 1;
				continue; /* no process switch */
			}
		} else
		{	depth--;
			if (oX->pc && (oX->pc->status & D_ATOM))
			{	non_fatal("stmnt in d_step blocks", (char *)0);
			}
			if (X->pc
			&&  X->pc->n
			&&  X->pc->n->ntyp == '@'
			&&  X->pid == (nproc-nstop-1))
			{	if (X != run && Y != NULL)
					Y->nxt = X->nxt;
				else
					run = X->nxt;
				nstop++;
				Priority_Sum -= X->priority;
				if (verbose&4)
				{	whoruns(1);
					dotag(stdout, "terminates\n");
				}
				LastX = X;
				if (!interactive) Tval = 0;
				if (nproc == nstop) break;
				memset(is_blocked, 0, 256);
				/* proc X is no longer in runlist */
				X = (X->nxt) ? X->nxt : run;
			} else
			{	if (p_blocked(X->pid))
				{	if (Tval) break;
					Tval = 1;
					if (depth >= jumpsteps)
					{	oX = X;
						X = (RunList *) 0; /* to suppress indent */
						dotag(stdout, "timeout\n");
						X = oX;
		}	}	}	}

		if (!run || !X) break;	/* new 5.0 */

		Y = pickproc(X);
		notbeyond = 0;
	}
	context = ZS;
	wrapup(0);
}

int
complete_rendez(void)
{	RunList *orun = X, *tmp;
	Element  *s_was = LastStep;
	Element *e;
	int j, ointer = interactive;

	if (s_trail)
		return 1;
	if (orun->pc->status & D_ATOM)
		fatal("rv-attempt in d_step sequence", (char *)0);
	Rvous = 1;
	interactive = 0;

	j = (int) Rand()%Priority_Sum;	/* randomize start point */
	X = run;
	while (j - X->priority >= 0)
	{	j -= X->priority;
		X = X->nxt;
		if (!X) X = run;
	}
	for (j = nproc - nstop; j > 0; j--)
	{	if (X != orun
		&& (!X->prov || eval(X->prov))
		&& (e = eval_sub(X->pc)))
		{	if (TstOnly)
			{	X = orun;
				Rvous = 0;
				goto out;
			}
			if ((verbose&32) || (verbose&4))
			{	tmp = orun; orun = X; X = tmp;
				if (!s_was) s_was = X->pc;
				p_talk(s_was, 1);
				printf("	[");
				comment(stdout, s_was->n, 0);
				printf("]\n");
				tmp = orun; orun = X; X = tmp;
				if (!LastStep) LastStep = X->pc;
				p_talk(LastStep, 1);
				printf("	[");
				comment(stdout, LastStep->n, 0);
				printf("]\n");
			}
			Rvous = 0; /* before silent_moves */
			X->pc = silent_moves(e);
out:				interactive = ointer;
			return 1;
		}

		X = X->nxt;
		if (!X) X = run;
	}
	Rvous = 0;
	X = orun;
	interactive = ointer;
	return 0;
}

/***** Runtime - Local Variables *****/

static void
addsymbol(RunList *r, Symbol  *s)
{	Symbol *t;
	int i;

	for (t = r->symtab; t; t = t->next)
		if (strcmp(t->name, s->name) == 0
		&& (old_scope_rules
		 || strcmp((const char *)t->bscp, (const char *)s->bscp) == 0))
			return;		/* it's already there */

	t = (Symbol *) emalloc(sizeof(Symbol));
	t->name = s->name;
	t->type = s->type;
	t->hidden = s->hidden;
	t->nbits  = s->nbits;
	t->nel  = s->nel;
	t->ini  = s->ini;
	t->setat = depth;
	t->context = r->n;

	t->bscp  = (unsigned char *) emalloc(strlen((const char *)s->bscp)+1);
	strcpy((char *)t->bscp, (const char *)s->bscp);

	if (s->type != STRUCT)
	{	if (s->val)	/* if already initialized, copy info */
		{	t->val = (int *) emalloc(s->nel*sizeof(int));
			for (i = 0; i < s->nel; i++)
				t->val[i] = s->val[i];
		} else
		{	(void) checkvar(t, 0);	/* initialize it */
		}
	} else
	{	if (s->Sval)
			fatal("saw preinitialized struct %s", s->name);
		t->Slst = s->Slst;
		t->Snm  = s->Snm;
		t->owner = s->owner;
	/*	t->context = r->n; */
	}
	t->next = r->symtab;	/* add it */
	r->symtab = t;
}

static void
setlocals(RunList *r)
{	Ordered	*walk;
	Symbol	*sp;
	RunList	*oX = X;

	X = r;
	for (walk = all_names; walk; walk = walk->next)
	{	sp = walk->entry;
		if (sp
		&&  sp->context
		&&  strcmp(sp->context->name, r->n->name) == 0
		&&  sp->Nid >= 0
		&& (sp->type == UNSIGNED
		||  sp->type == BIT
		||  sp->type == MTYPE
		||  sp->type == BYTE
		||  sp->type == CHAN
		||  sp->type == SHORT
		||  sp->type == INT
		||  sp->type == STRUCT))
		{	if (!findloc(sp))
			non_fatal("setlocals: cannot happen '%s'",
				sp->name);
		}
	}
	X = oX;
}

static void
oneparam(RunList *r, Lextok *t, Lextok *a, ProcList *p)
{	int k; int at, ft;
	RunList *oX = X;

	if (!a)
		fatal("missing actual parameters: '%s'", p->n->name);
	if (t->sym->nel > 1 || t->sym->isarray)
		fatal("array in parameter list, %s", t->sym->name);
	k = eval(a->lft);

	at = Sym_typ(a->lft);
	X = r;	/* switch context */
	ft = Sym_typ(t);

	if (at != ft && (at == CHAN || ft == CHAN))
	{	char buf[256], tag1[64], tag2[64];
		(void) sputtype(tag1, ft);
		(void) sputtype(tag2, at);
		sprintf(buf, "type-clash in params of %s(..), (%s<-> %s)",
			p->n->name, tag1, tag2);
		non_fatal("%s", buf);
	}
	t->ntyp = NAME;
	addsymbol(r, t->sym);
	(void) setval(t, k);
	
	X = oX;
}

static void
setparams(RunList *r, ProcList *p, Lextok *q)
{	Lextok *f, *a;	/* formal and actual pars */
	Lextok *t;	/* list of pars of 1 type */

	if (q)
	{	lineno = q->ln;
		Fname  = q->fn;
	}
	for (f = p->p, a = q; f; f = f->rgt) /* one type at a time */
	for (t = f->lft; t; t = t->rgt, a = (a)?a->rgt:a)
	{	if (t->ntyp != ',')
			oneparam(r, t, a, p);	/* plain var */
		else
			oneparam(r, t->lft, a, p); /* expanded struct */
	}
}

Symbol *
findloc(Symbol *s)
{	Symbol *r;

	if (!X)
	{	/* fatal("error, cannot eval '%s' (no proc)", s->name); */
		return ZS;
	}
	for (r = X->symtab; r; r = r->next)
		if (strcmp(r->name, s->name) == 0
		&& (old_scope_rules || strcmp((const char *)r->bscp, (const char *)s->bscp) == 0))
			break;
	if (!r)
	{	addsymbol(X, s);
		r = X->symtab;
	}
	return r;
}

int
in_bound(Symbol *r, int n)
{
	if (!r)	return 0;

	if (n >= r->nel || n < 0)
	{	printf("spin: indexing %s[%d] - size is %d\n",
			r->name, n, r->nel);
		non_fatal("indexing array \'%s\'", r->name);
		return 0;
	}
	return 1;
}

int
getlocal(Lextok *sn)
{	Symbol *r, *s = sn->sym;
	int n = eval(sn->lft);

	r = findloc(s);
	if (r && r->type == STRUCT)
		return Rval_struct(sn, r, 1); /* 1 = check init */
	if (in_bound(r, n))
		return cast_val(r->type, r->val[n], r->nbits);
	return 0;
}

int
setlocal(Lextok *p, int m)
{	Symbol *r = findloc(p->sym);
	int n = eval(p->lft);

	if (in_bound(r, n))
	{	if (r->type == STRUCT)
			(void) Lval_struct(p, r, 1, m); /* 1 = check init */
		else
		{
#if 0
			if (r->nbits > 0)
				m = (m & ((1<<r->nbits)-1));
			r->val[n] = m;
#else
			r->val[n] = cast_val(r->type, m, r->nbits);
#endif
			r->setat = depth;
	}	}

	return 1;
}

void
whoruns(int lnr)
{	if (!X) return;

	if (lnr) printf("%3d:	", depth);
	printf("proc ");
	if (Have_claim && X->pid == 0)
		printf(" -");
	else
		printf("%2d", X->pid - Have_claim);
	printf(" (%s) ", X->n->name);
}

static void
talk(RunList *r)
{
	if ((verbose&32) || (verbose&4))
	{	p_talk(r->pc, 1);
		printf("\n");
		if (verbose&1) dumpglobals();
		if (verbose&2) dumplocal(r);
	}
}

void
p_talk(Element *e, int lnr)
{	static int lastnever = -1;
	static char nbuf[128];
	int newnever = -1;

	if (e && e->n)
		newnever = e->n->ln;

	if (Have_claim && X && X->pid == 0
	&&  lastnever != newnever && e)
	{	if (xspin)
		{	printf("MSC: ~G line %d\n", newnever);
#if 0
			printf("%3d:	proc  - (NEVER) line   %d \"never\" ",
				depth, newnever);
			printf("(state 0)\t[printf('MSC: never\\\\n')]\n");
		} else
		{	printf("%3d:	proc  - (NEVER) line   %d \"never\"\n",
				depth, newnever);
#endif
		}
		lastnever = newnever;
	}

	whoruns(lnr);
	if (e)
	{	if (e->n)
		{	char *ptr = e->n->fn->name;
			char *qtr = nbuf;
			while (*ptr != '\0')
			{	if (*ptr != '"')
				{	*qtr++ = *ptr;
				}
				ptr++;
			}
			*qtr = '\0';
		} else
		{	strcpy(nbuf, "-");
		}
		printf("%s:%d (state %d)",
			nbuf,
			e->n?e->n->ln:-1,
			e->seqno);
		if (!xspin
		&&  ((e->status&ENDSTATE) || has_lab(e, 2)))	/* 2=end */
		{	printf(" <valid end state>");
		}
	}
}

int
remotelab(Lextok *n)
{	int i;

	lineno = n->ln;
	Fname  = n->fn;
	if (n->sym->type != 0 && n->sym->type != LABEL)
	{	printf("spin: error, type: %d\n", n->sym->type);
		fatal("not a labelname: '%s'", n->sym->name);
	}
	if (n->indstep >= 0)
	{	fatal("remote ref to label '%s' inside d_step",
			n->sym->name);
	}
	if ((i = find_lab(n->sym, n->lft->sym, 1)) == 0)
		fatal("unknown labelname: %s", n->sym->name);
	return i;
}

int
remotevar(Lextok *n)
{	int prno, i, added=0;
	RunList *Y, *oX;
	Lextok *onl;
	Symbol *os;

	lineno = n->ln;
	Fname  = n->fn;

	if (!n->lft->lft)
		prno = f_pid(n->lft->sym->name);
	else
	{	prno = eval(n->lft->lft); /* pid - can cause recursive call */
#if 0
		if (n->lft->lft->ntyp == CONST)	/* user-guessed pid */
#endif
		{	prno += Have_claim;
			added = Have_claim;
	}	}

	if (prno < 0)
		return 0;	/* non-existing process */
#if 0
	i = nproc - nstop;
	for (Y = run; Y; Y = Y->nxt)
	{	--i;
		printf("	%s: i=%d, prno=%d, ->pid=%d\n", Y->n->name, i, prno, Y->pid);
	}
#endif
	i = nproc - nstop + Skip_claim;	/* 6.0: added Skip_claim */
	for (Y = run; Y; Y = Y->nxt)
	if (--i == prno)
	{	if (strcmp(Y->n->name, n->lft->sym->name) != 0)
		{	printf("spin: remote reference error on '%s[%d]'\n",
				n->lft->sym->name, prno-added);
			non_fatal("refers to wrong proctype '%s'", Y->n->name);
		}
		if (strcmp(n->sym->name, "_p") == 0)
		{	if (Y->pc)
				return Y->pc->seqno;
			/* harmless, can only happen with -t */
			return 0;
		}
#if 1
		/* new 4.0 allow remote variables */
		oX = X;
		X = Y;

		onl = n->lft;
		n->lft = n->rgt;

		os = n->sym;
		n->sym = findloc(n->sym);

		i = getval(n);

		n->sym = os;
		n->lft = onl;
		X = oX;
		return i;
#else
		break;
#endif
	}
	printf("remote ref: %s[%d] ", n->lft->sym->name, prno-added);
	non_fatal("%s not found", n->sym->name);
	printf("have only:\n");
	i = nproc - nstop - 1;
	for (Y = run; Y; Y = Y->nxt, i--)
		if (!strcmp(Y->n->name, n->lft->sym->name))
		printf("\t%d\t%s\n", i, Y->n->name);

	return 0;
}

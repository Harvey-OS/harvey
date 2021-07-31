/***** spin: sched.c *****/

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

extern int	verbose, s_trail, analyze;
extern char	*claimproc;
extern Symbol	*Fname, *context;
extern int	lineno, nr_errs, dumptab, xspin;
extern int	has_enabled, u_sync, Elcnt, Interactive;

RunList		*X   = (RunList  *) 0;
RunList		*run = (RunList  *) 0;
RunList		*LastX  = (RunList  *) 0; /* previous executing proc */
ProcList	*rdy = (ProcList *) 0;
Element		*LastStep = ZE;
int		Have_claim=0, nproc=0, nstop=0, Tval=0;
int		Rvous=0, depth=0, nrRdy=0, SubChoice;

void
runnable(ProcList *p)
{	RunList *r = (RunList *) emalloc(sizeof(RunList));

	r->n  = p->n;
	r->tn = p->tn;
	r->pid = ++nproc-nstop-1;	/* was: nproc++; */
	r->pc = huntele(p->s->frst, p->s->frst->status); /* was: s->frst; */
	r->ps = p->s;		/* was: r->maxseq = s->last->seqno */
	r->nxt = run;
	run = r;
}

ProcList *
ready(Symbol *n, Lextok *p, Sequence *s) /* n=name, p=formals, s=body */
{	ProcList *r = (ProcList *) emalloc(sizeof(ProcList));
	Lextok *fp, *fpt; int j; extern int Npars;

	r->n = n;
	r->p = p;
	r->s = s;
	r->tn = nrRdy++;
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

void
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

int
enable(Symbol *s, Lextok *n)		/* s=name, n=actuals */
{	ProcList *p;

	for (p = rdy; p; p = p->nxt)
		if (strcmp(s->name, p->n->name) == 0)
		{	runnable(p);
			setparams(run, p, n);
			return (nproc-nstop-1);	/* pid */
		}
	return 0; /* process not found */
}

void
start_claim(int n)
{	ProcList *p;

	for (p = rdy; p; p = p->nxt)
		if (p->tn == n
		&&  strcmp(p->n->name, ":never:") == 0)
		{	runnable(p);
			Have_claim = run->pid;	/* was 1 */
			return;
		}
	fatal("couldn't find claim", (char *) 0);
}

void
wrapup(int fini)
{
	if (nproc != nstop)
	{	printf("#processes: %d\n", nproc-nstop);
		dumpglobals();
		verbose &= ~1;	/* no more globals */
		verbose |= 32;	/* add process states */
		for (X = run; X; X = X->nxt)
			talk(X);
	}
	printf("%d processes created\n", nproc);

	if (xspin) exit(0);	/* avoid an abort from xspin */
	if (fini)  exit(1);
}

static char is_blocked[256];

int
p_blocked(int p)
{	register int i, j;

	is_blocked[p%256] = 1;
	for (i = j = 0; i < nproc - nstop; i++)
		j += is_blocked[i];
	if (j >= nproc - nstop)
	{	memset(is_blocked, 0, 256);
		return 1;
	}
	return 0;
}

Element *
silent_moves(Element *e)
{	Element *f;

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
		return e->nxt;
	}
	return e;
}

void
sched(void)
{	Element *e;
	RunList *Y=0;	/* previous process in run queue */
	RunList *oX;
	SeqList *z;
	int j, k;
	short Choices[256];

	if (dumptab)
	{	formdump();
		symdump();
		dumplabels();
		return;
	}

	if (has_enabled && u_sync > 0)
	{	printf("spin: error: >> cannot use enabled() in ");
		printf("models with synchronous channels <<\n");
		nr_errs++;
	}
	if (analyze)
	{	gensrc();
		return;
	} else if (s_trail)
	{	match_trail();
		return;
	}
	if (claimproc)
		printf("warning: claims are ignored in simulations\n");

	if (Interactive) Tval = 1;

	X = run;
	while (X)
	{	context = X->n;
		if (X->pc && X->pc->n)
		{	lineno = X->pc->n->ln;
			Fname  = X->pc->n->fn;
		}
		depth++; LastStep = ZE;
		oX = X;	/* a rendezvous could change it */
		if (e = eval_sub(X->pc))
		{	if (verbose&32 || verbose&4)
			{	if (X == oX)
				{	p_talk(X->pc, 1);
					printf("	[");
					if (!LastStep) LastStep = X->pc;
					comment(stdout, LastStep->n, 0);
					printf("]\n");
				}
				if (verbose&1) dumpglobals();
				if (verbose&2) dumplocal(X);
				if (xspin) printf("\n"); /* xspin looks for these */
			}
			oX->pc = e; LastX = X;
			if (!Interactive) Tval = 0;
			memset(is_blocked, 0, 256);

			/* new implementation of atomic sequences */
			if (X->pc->status & (ATOM|L_ATOM))
				continue; /* no switch */
		} else
		{	depth--;
			if (X->pc->n->ntyp == '@'
			&&  X->pid == (nproc-nstop-1))
			{	if (X != run)
					Y->nxt = X->nxt;
				else
					run = X->nxt;
				nstop++;
				if (verbose&4)
				{	whoruns(1);
					printf("terminates\n");
				}
				LastX = X;
				if (!Interactive) Tval = 0;
				if (nproc == nstop) break;
				memset(is_blocked, 0, 256);
			} else
			{	if (!Interactive && p_blocked(X->pid))
				{	if (Tval) break;
					Tval = 1;
					printf("timeout\n");
		}	}	}
		Y = X;
		if (Interactive)
		{
try_again:		printf("Select a statement\n");
			for (X = run, k = 1; X; X = X->nxt)
			{	if (X->pid > 255) break;
				Choices[X->pid] = 0;
				if (!X->pc)
					continue;
				Choices[X->pid] = k;
				X->pc = silent_moves(X->pc);
				if (!X->pc->sub && X->pc->n)
				{	if (!xspin && !Enabled0(X->pc))
					{	k++;
						continue;
					}
					printf("\tchoice %d: ", k++);
					p_talk(X->pc, 0);
					if (!Enabled0(X->pc))
						printf(" unexecutable,");
					printf(" [");
					comment(stdout, X->pc->n, 0);
					printf("]\n");
				} else
				for (z = X->pc->sub, j=0; z; z = z->nxt)
				{	Element *y = z->this->frst;
					if (!y) continue;

					if (!xspin && !Enabled0(y))
					{	k++;
						continue;
					}
					printf("\tchoice %d: ", k++);
					p_talk(X->pc, 0);
					if (!Enabled0(y))
						printf(" unexecutable,");
					printf(" [");
					comment(stdout, y->n, 0);
					printf("]\n");
			}	}
			X = run;
			if (xspin)
				printf("Make Selection %d\n\n", k-1);
			else
				printf("Select [1-%d]: ", k-1);
			fflush(stdout);
			scanf("%d", &j);
			if (j < 1 || j >= k)
			{	printf("\tchoice is outside range\n");
				goto try_again;
			}
			SubChoice = 0;
			for (X = run; X; X = X->nxt)
			{	if (!X->nxt
				||   X->nxt->pid > 255
				||   j < Choices[X->nxt->pid])
				{
					SubChoice = 1+j-Choices[X->pid];
					break;
				}
			}
		} else
		{	j = (int) Rand()%(nproc-nstop);
			while (j-- >= 0)
			{	X = X->nxt;
				if (!X) X = run;
		}	}
	}
	context = ZS;
	wrapup(0);
}

int
complete_rendez(void)
{	RunList *orun = X, *tmp;
	Element  *s_was = LastStep;
	Element *e;

	if (s_trail)
		return 1;
	Rvous = 1;
	for (X = run; X; X = X->nxt)
		if (X != orun && (e = eval_sub(X->pc)))
		{	if (verbose&4)
			{	tmp = orun; orun = X; X = tmp;
				if (!s_was) s_was = X->pc;
				p_talk(s_was, 1);
				printf("	[");
				comment(stdout, s_was->n, 0);
				printf("]\n");
				tmp = orun; orun = X; X = tmp;
			/*	printf("\t");	*/
				if (!LastStep) LastStep = X->pc;
				p_talk(LastStep, 1);
				printf("	[");
				comment(stdout, LastStep->n, 0);
				printf("]\n");
			}
			X->pc = e;
			Rvous = 0;
			return 1;
		}
	Rvous = 0;
	X = orun;
	return 0;
}

/***** Runtime - Local Variables *****/

void
addsymbol(RunList *r, Symbol  *s)
{	Symbol *t;
	int i;

	for (t = r->symtab; t; t = t->next)
		if (strcmp(t->name, s->name) == 0)
			return;		/* it's already there */

	t = (Symbol *) emalloc(sizeof(Symbol));
	t->name = s->name;
	t->type = s->type;
	t->nel  = s->nel;
	t->ini  = s->ini;
	t->setat = depth;
	if (s->type != STRUCT)
	{	if (s->val)	/* if already initialized, copy info */
		{	t->val = (int *) emalloc(s->nel*sizeof(int));
			for (i = 0; i < s->nel; i++)
				t->val[i] = s->val[i];
		} else
			(void) checkvar(t, 0);	/* initialize it */
	} else
	{	if (s->Sval)
			fatal("saw preinitialized struct %s", s->name);
		t->Slst = s->Slst;
		t->Snm  = s->Snm;
		t->owner = s->owner;
		t->context = r->n;
	}
	t->next = r->symtab;	/* add it */
	r->symtab = t;
}

void
oneparam(RunList *r, Lextok *t, Lextok *a, ProcList *p)
{	int k; int at, ft;
	RunList *oX = X;

	if (!a)
		fatal("missing actual parameters: '%s'", p->n->name);
	if (t->sym->nel != 1)
		fatal("array in parameter list, %s", t->sym->name);
	k = eval(a->lft);

	at = Sym_typ(a->lft);
	X = r;	/* switch context */
	ft = Sym_typ(t);

	if (at != ft && (at == CHAN || ft == CHAN))
	{	char buf[128], tag1[32], tag2[32];
		(void) sputtype(tag1, ft);
		(void) sputtype(tag2, at);
		sprintf(buf, "type-clash in param-list of %s(..), (%s<-> %s)",
			p->n->name, tag1, tag2);
		non_fatal("%s", buf);
	}
	t->ntyp = NAME;
	addsymbol(r, t->sym);
	(void) setval(t, k);
	
	X = oX;
}

void
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
			oneparam(r, t->lft, a, p);	/* expanded structure */
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
		if (strcmp(r->name, s->name) == 0)
			break;
	if (!r)
	{	addsymbol(X, s);
		r = X->symtab;
	}
	return r;
}

int
getlocal(Lextok *sn)
{	Symbol *r, *s = sn->sym;
	int n = eval(sn->lft);

	r = findloc(s);
	if (r && r->type == STRUCT)
		return Rval_struct(sn, r, 1); /* 1 = check init */
	if (r)
	{	if (n >= r->nel || n < 0)
		{	printf("spin: indexing %s[%d] - size is %d\n",
				s->name, n, r->nel);
			non_fatal("array indexing error %s", s->name);
		} else
		{	return cast_val(r->type, r->val[n]);
	}	}

	return 0;
}

int
setlocal(Lextok *p, int m)
{	Symbol *r = findloc(p->sym);
	int n = eval(p->lft);

	if (!r) return 1;

	if (n >= r->nel || n < 0)
	{	printf("spin: indexing %s[%d] - size is %d\n",
			r->name, n, r->nel);
		non_fatal("array indexing error %s", r->name);
	} else
	{	if (r->type == STRUCT)
			(void) Lval_struct(p, r, 1, m); /* 1 = check init */
		else
		{	r->val[n] = m;
			r->setat = depth;
	}	}

	return 1;
}

void
whoruns(int lnr)
{	if (!X) return;

	if (lnr) printf("%3d:	", depth);
	printf("proc ");
	if (Have_claim)
	{	if (X->pid == Have_claim)
			printf(" -");
		else if (X->pid > Have_claim)
			printf("%2d", X->pid-1);
		else
			printf("%2d", X->pid);
	} else
		printf("%2d", X->pid);
	printf(" (%s) ", X->n->name);
}

void
talk(RunList *r)
{
	if (verbose&32 || verbose&4)
	{	p_talk(r->pc, 1);
		printf("\n");
		if (verbose&1) dumpglobals();
		if (verbose&2) dumplocal(r);
	}
}

void
p_talk(Element *e, int lnr)
{
	whoruns(lnr);
	if (e)
	printf("line %3d %s (state %d)",
			e->n?e->n->ln:-1,
			e->n?e->n->fn->name:"-",
			e->seqno);
}

int
remotelab(Lextok *n)
{	int i;

	lineno = n->ln;
	Fname  = n->fn;
	if (n->sym->type)
		fatal("not a labelname: '%s'", n->sym->name);
	if ((i = find_lab(n->sym, n->lft->sym)) == 0)
		fatal("unknown labelname: %s", n->sym->name);
	return i;
}

int
remotevar(Lextok *n)
{	int pno, i, trick=0;
	RunList *Y;

	lineno = n->ln;
	Fname  = n->fn;
	if (!n->lft->lft)
	{	non_fatal("missing pid in %s", n->sym->name);
		return 0;
	}

	pno = eval(n->lft->lft);	/* pid - can cause recursive call */
TryAgain:
	i = nproc - nstop;
	for (Y = run; Y; Y = Y->nxt)
	if (--i == pno)
	{	if (strcmp(Y->n->name, n->lft->sym->name) != 0)
		{	if (!trick && Have_claim)
			{	trick = 1; pno++;
				/* assumes user guessed the pid */
				goto TryAgain;
			}
			printf("remote ref %s[%d] refers to %s\n",
				n->lft->sym->name, pno, Y->n->name);
			non_fatal("wrong proctype %s", Y->n->name);
		}
		if (strcmp(n->sym->name, "_p") == 0)
			return Y->pc->seqno;
		break;
	}
	printf("remote ref: %s[%d] ", n->lft->sym->name, pno);
	non_fatal("%s not found", n->sym->name);

	return 0;
}

/***** spin: pangen7.c *****/

/*
 * This file is part of the public release of Spin. It is subject to the
 * terms in the LICENSE file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com
 */

#include <stdlib.h>
#include <assert.h>
#include "spin.h"
#include "y.tab.h"
#include <assert.h>
#ifndef PC
#include <unistd.h>
#endif

extern ProcList	*ready;
extern Element *Al_El;
extern int nclaims, verbose, Strict;
extern short has_accept;

typedef struct Succ_List Succ_List;
typedef struct SQueue SQueue;
typedef struct OneState OneState;
typedef struct State_Stack State_Stack;
typedef struct Guard Guard;

struct Succ_List {
	SQueue	*s;
	Succ_List *nxt;
};

struct OneState {
	int	*combo;	/* the combination of claim states */
	Succ_List	*succ;	/* list of ptrs to immediate successor states */
};

struct SQueue {
	OneState	state;
	SQueue	*nxt;
};

struct State_Stack {
	int *n;
	State_Stack *nxt;
};

struct Guard {
	Lextok *t;
	Guard *nxt;
};

static SQueue	*sq, *sd, *render;	/* states move from sq to sd to render to holding */
static SQueue	*holding, *lasthold;
static State_Stack *dsts;

static int nst;		/* max nr of states in claims */
static int *Ist;	/* initial states */
static int *Nacc;	/* number of accept states in claim */
static int *Nst;	/* next states */
static int **reached;	/* n claims x states */
static int unfolding;	/* to make sure all accept states are reached */
static int is_accept;	/* remember if the current state is accepting in any claim */
static int not_printing; /* set during explore_product */

static Element ****matrix;	/* n x two-dimensional arrays state x state */
static Element **Selfs;	/* self-loop states at end of claims */

static void get_seq(int, Sequence *);
static void set_el(int n, Element *e);
static void gen_product(void);
static void print_state_nm(char *, int *, char *);
static SQueue *find_state(int *);
static SQueue *retrieve_state(int *);

static int
same_state(int *a, int *b)
{	int i;

	for (i = 0; i < nclaims; i++)
	{	if (a[i] != b[i])
		{	return 0;
	}	}
	return 1;
}

static int
in_stack(SQueue *s, SQueue *in)
{	SQueue *q;

	for (q = in; q; q = q->nxt)
	{	if (same_state(q->state.combo, s->state.combo))
		{	return 1;
	}	}
	return 0;
}

static void
to_render(SQueue *s)
{	SQueue *a, *q, *last; /* find in sd/sq and move to render, if not already there */
	int n;

	for (n = 0; n < nclaims; n++)
	{	reached[n][ s->state.combo[n] ] |= 2;
	}

	for (q = render; q; q = q->nxt)
	{	if (same_state(q->state.combo, s->state.combo))
		{	return;
	}	}
	for (q = holding; q; q = q->nxt)
	{	if (same_state(q->state.combo, s->state.combo))
		{	return;
	}	}

	a = sd;
more:
	for (q = a, last = 0; q; last = q, q = q->nxt)
	{	if (same_state(q->state.combo, s->state.combo))
		{	if (!last)
			{	if (a == sd)
				{	sd = q->nxt;
				} else if (a == sq)
				{	sq = q->nxt;
				} else
				{	holding = q->nxt;
				}
			} else
			{	last->nxt = q->nxt;
			}
			q->nxt = render;
			render = q;
			return;
	}	}
	if (verbose)
	{	print_state_nm("looking for: ", s->state.combo, "\n");
	}
	(void) find_state(s->state.combo);	/* creates it in sq */
	if (a != sq)
	{	a = sq;
		goto more;
	}
	fatal("cannot happen, to_render", 0);
}

static void
wrap_text(char *pre, Lextok *t, char *post)
{
	printf(pre, (char *) 0);
	comment(stdout, t, 0);
	printf(post, (char *) 0);
}

static State_Stack *
push_dsts(int *n)
{	State_Stack *s;
	int i;

	for (s = dsts; s; s = s->nxt)
	{	if (same_state(s->n, n))
		{	if (verbose&64)
			{	printf("\n");
				for (s = dsts; s; s = s->nxt)
				{	print_state_nm("\t", s->n, "\n");
				}
				print_state_nm("\t", n, "\n");
			}
			return s;
	}	}

	s = (State_Stack *) emalloc(sizeof(State_Stack));
	s->n = (int *) emalloc(nclaims * sizeof(int));
	for (i = 0; i < nclaims; i++)
		s->n[i] = n[i];
	s->nxt = dsts;
	dsts = s;
	return 0;
}

static void
pop_dsts(void)
{
	assert(dsts != NULL);
	dsts = dsts->nxt;
}

static void
complete_transition(Succ_List *sl, Guard *g)
{	Guard *w;
	int cnt = 0;

	printf("	:: ");
	for (w = g; w; w = w->nxt)
	{	if (w->t->ntyp == CONST
		&&  w->t->val == 1)
		{	continue;
		} else if (w->t->ntyp == 'c'
		&&  w->t->lft->ntyp == CONST
		&&  w->t->lft->val == 1)
		{	continue; /* 'true' */
		}

		if (cnt > 0)
		{	printf(" && ");
		}
		wrap_text("", w->t, "");
		cnt++;
	}
	if (cnt == 0)
	{	printf("true");
	}
	print_state_nm(" -> goto ", sl->s->state.combo, "");

	if (is_accept > 0)
	{	printf("_U%d\n", (unfolding+1)%nclaims);
	} else
	{	printf("_U%d\n", unfolding);
	}
}

static void
state_body(OneState *s, Guard *guard)
{	Succ_List *sl;
	State_Stack *y;
	Guard *g;
	int i, once;

	for (sl = s->succ; sl; sl = sl->nxt)
	{	once = 0;

		for (i = 0; i < nclaims; i++)
		{	Element *e;
			e = matrix[i][s->combo[i]][sl->s->state.combo[i]];

			/* if one of the claims has a DO or IF move
			   then pull its target state forward, once
			 */

			if (!e
			|| e->n->ntyp == NON_ATOMIC
			||  e->n->ntyp == DO
			||  e->n->ntyp == IF)
			{	s = &(sl->s->state);
				y = push_dsts(s->combo);
				if (!y)
				{	if (once++ == 0)
					{	assert(s->succ != NULL);
						state_body(s, guard);
					}
					pop_dsts();
				} else if (!y->nxt)	/* self-loop transition */
				{	if (!not_printing) printf(" /* self-loop */\n");
				} else
				{	/* non_fatal("loop in state body", 0); ** maybe ok */
				}
				continue;
			} else
			{	g = (Guard *) emalloc(sizeof(Guard));
				g->t = e->n;
				g->nxt = guard;
				guard = g;
		}	}

		if (guard && !once)
		{	if (!not_printing) complete_transition(sl, guard);
			to_render(sl->s);
	}	}
}

static struct X_tbl {
	char *s;	int n;
} spl[] = {
	{"end",		3 },
	{"accept",	6 },
	{0,		0 },
};

static int slcnt;
extern Label *labtab;

static ProcList *
locate_claim(int n)
{	ProcList *p;
	int i;

	for (p = ready, i = 0; p; p = p->nxt, i++) /* find claim name */
	{	if (i == n)
		{	break;
	}	}
	assert(p && p->b == N_CLAIM);

	return p;
}

static void
elim_lab(Element *e)
{	Label *l, *lst;

	for (l = labtab, lst = NULL; l; lst = l, l = l->nxt)
	{	if (l->e == e)
		{	if (lst)
			{	lst->nxt = l->nxt;
			} else
			{	labtab = l->nxt;
			}
			break;
	}	}
}

static int
claim_has_accept(ProcList *p)
{	Label *l;

	for (l = labtab; l; l = l->nxt)
	{	if (strcmp(l->c->name, p->n->name) == 0
		&&  strncmp(l->s->name, "accept", 6) == 0)
		{	return 1;
	}	}
	return 0;
}

static void
prune_accept(void)
{	int n;

	for (n = 0; n < nclaims; n++)
	{	if ((reached[n][Selfs[n]->seqno] & 2) == 0)
		{	if (verbose)
			{	printf("claim %d: selfloop not reachable\n", n);
			}
			elim_lab(Selfs[n]);
			Nacc[n] = claim_has_accept(locate_claim(n));
	}	}
}

static void
mk_accepting(int n, Element *e)
{	ProcList *p;
	Label *l;
	int i;

	assert(!Selfs[n]);
	Selfs[n] = e;

	l = (Label *) emalloc(sizeof(Label));
	l->s = (Symbol *) emalloc(sizeof(Symbol));
	l->s->name = "accept00";
	l->c = (Symbol *) emalloc(sizeof(Symbol));
	l->uiid = 0;	/* this is not in an inline */

	for (p = ready, i = 0; p; p = p->nxt, i++) /* find claim name */
	{	if (i == n)
		{	l->c->name = p->n->name;
			break;
	}	}
	assert(p && p->b == N_CLAIM);
	Nacc[n] = 1;
	has_accept = 1;

	l->e = e;
	l->nxt = labtab;
	labtab = l;
}

static void
check_special(int *nrs)
{	ProcList *p;
	Label *l;
	int i, j, nmatches;
	int any_accepts = 0;

	for (i = 0; i < nclaims; i++)
	{	any_accepts += Nacc[i];
	}

	is_accept = 0;
	for (j = 0; spl[j].n; j++) /* 2 special label prefixes */
	{	nmatches = 0;
		for (p = ready, i = 0; p; p = p->nxt, i++) /* check each claim */
		{	if (p->b != N_CLAIM)
			{	continue;
			}
			/* claim i in state nrs[i], type p->tn, name p->n->name
			 * either the state has an accept label, or the claim has none,
			 * so that all its states should be considered accepting
			 * --- but only if other claims do have accept states!
			 */
			if (Strict == 0 && j == 1 && Nacc[i] == 0 && any_accepts > 0)
			{	if ((verbose&32) && i == unfolding)
				{	printf("	/* claim %d pseudo-accept */\n", i);
				}
				goto is_accepting;
			}
			for (l = labtab; l; l = l->nxt)	/* check its labels */
			{	if (strcmp(l->c->name, p->n->name) == 0  /* right claim */
				&&  l->e->seqno == nrs[i]		 /* right state */
				&&  strncmp(l->s->name, spl[j].s, spl[j].n) == 0)
				{	if (j == 1)	/* accept state */
					{	char buf[32];
is_accepting:					if (strchr(p->n->name, ':'))
						{	sprintf(buf, "N%d", i);
						} else
						{	assert(strlen(p->n->name) < sizeof(buf));
							strcpy(buf, p->n->name);
						}
						if (unfolding == 0 && i == 0)
						{	if (!not_printing)
							printf("%s_%s_%d:\n",	/* true accept */
								spl[j].s, buf, slcnt++);
						} else if (verbose&32)
						{	if (!not_printing)
							printf("%s_%s%d:\n",
								buf, spl[j].s, slcnt++);
						}
						if (i == unfolding)
						{	is_accept++; /* move to next unfolding */
						}
					} else
					{	nmatches++;
					}
					break;
		}	}	}
		if (j == 0 && nmatches == nclaims)	/* end-state */
		{	if (!not_printing)
			{	printf("%s%d:\n", spl[j].s, slcnt++);
	}	}	}
}

static int
render_state(SQueue *q)
{
	if (!q || !q->state.succ)
	{	if (verbose&64)
		{	printf("	no exit\n");
		}
		return 0;
	}

	check_special(q->state.combo); /* accept or end-state labels */

	dsts = (State_Stack *) 0;
	push_dsts(q->state.combo);	/* to detect loops */

	if (!not_printing)
	{	print_state_nm("", q->state.combo, "");	/* the name */
		printf("_U%d:\n\tdo\n", unfolding);
	}

	state_body(&(q->state), (Guard *) 0);

	if (!not_printing)
	{	printf("\tod;\n");
	}
	pop_dsts();
	return 1;
}

static void
explore_product(void)
{	SQueue *q;

	/* all states are in the sd queue */

	q = retrieve_state(Ist);	/* retrieve from the sd q */
	q->nxt = render;		/* put in render q */
	render = q;
	do {
		q = render;
		render = render->nxt;
		q->nxt = 0;		/* remove from render q */

		if (verbose&64)
		{	print_state_nm("explore: ", q->state.combo, "\n");
		}

		not_printing = 1;
		render_state(q);	/* may add new states */
		not_printing = 0;

		if (lasthold)
		{	lasthold->nxt = q;
			lasthold = q;
		} else
		{	holding = lasthold = q;
		}
	} while (render);
	assert(!dsts);
	
}

static void
print_product(void)
{	SQueue *q;
	int cnt;

	if (unfolding == 0)
	{	printf("never Product {\n");	/* name expected by iSpin */
		q = find_state(Ist);	/* should find it in the holding q */
		assert(q != NULL);
		q->nxt = holding;	/* put it at the front */
		holding = q;
	}
	render = holding;
	holding = lasthold = 0;

	printf("/* ============= U%d ============= */\n", unfolding);
	cnt = 0;
	do {
		q = render;
		render = render->nxt;
		q->nxt = 0;
		if (verbose&64)
		{	print_state_nm("print: ", q->state.combo, "\n");
		}
		cnt += render_state(q);

		if (lasthold)
		{	lasthold->nxt = q;
			lasthold = q;
		} else
		{	holding = lasthold = q;
		}
	} while (render);
	assert(!dsts);

	if (cnt == 0)
	{	printf("	0;\n");
	}

	if (unfolding == nclaims-1)
	{	printf("}\n");
	}
}

static void
prune_dead(void)
{	Succ_List *sl, *last;
	SQueue *q;
	int cnt;

	do {	cnt = 0;
		for (q = sd; q; q = q->nxt)
		{	/* if successor is deadend, remove it
			 * unless it's a move to the end-state of the claim
			 */
			last = (Succ_List *) 0;
			for (sl = q->state.succ; sl; last = sl, sl = sl->nxt)
			{	if (!sl->s->state.succ)	/* no successor */
				{	if (!last)
					{	q->state.succ = sl->nxt;
					} else
					{	last->nxt = sl->nxt;
					}
					cnt++;
		}	}	}
	} while (cnt > 0);
}

static void
print_raw(void)
{	int i, j, n;

	printf("#if 0\n");
	for (n = 0; n < nclaims; n++)
	{	printf("C%d:\n", n);
		for (i = 0; i < nst; i++)
		{	if (reached[n][i])
			for (j = 0; j < nst; j++)
			{	if (matrix[n][i][j])
				{	if (reached[n][i] & 2) printf("+");
					if (i == Ist[n]) printf("*");
					printf("\t%d", i);
					wrap_text(" -[", matrix[n][i][j]->n, "]->\t");
					printf("%d\n", j);
	}	}	}	}
	printf("#endif\n\n");
	fflush(stdout);
}

void
sync_product(void)
{	ProcList *p;
	Element *e;
	int n, i;

	if (nclaims <= 1) return;

	(void) unlink("pan.pre");

	Ist  = (int *) emalloc(sizeof(int) * nclaims);
	Nacc = (int *) emalloc(sizeof(int) * nclaims);
	Nst  = (int *) emalloc(sizeof(int) * nclaims);
	reached = (int **) emalloc(sizeof(int *) * nclaims);
	Selfs   = (Element **) emalloc(sizeof(Element *) * nclaims);
	matrix  = (Element ****) emalloc(sizeof(Element ***) * nclaims); /* claims */

	for (p = ready, i = 0; p; p = p->nxt, i++)
	{	if (p->b == N_CLAIM)
		{	nst = max(p->s->maxel, nst);
			Nacc[i] = claim_has_accept(p);
	}	}

	for (n = 0; n < nclaims; n++)
	{	reached[n] = (int *) emalloc(sizeof(int) * nst);
		matrix[n] = (Element ***) emalloc(sizeof(Element **) * nst);	/* rows */
		for (i = 0; i < nst; i++)					/* cols */
		{	matrix[n][i] = (Element **) emalloc(sizeof(Element *) * nst);
	}	}

	for (e = Al_El; e; e = e->Nxt)
	{	e->status &= ~DONE;
	}

	for (p = ready, n=0; p; p = p->nxt, n++)
	{	if (p->b == N_CLAIM)
		{	/* fill in matrix[n] */
			e = p->s->frst;
			Ist[n] = huntele(e, e->status, -1)->seqno;

			reached[n][Ist[n]] = 1|2;
			get_seq(n, p->s);
	}	}

	if (verbose)	/* show only the input automata */
	{	print_raw();
	}

	gen_product();	/* create product automaton */
}

static int
nxt_trans(int n, int cs, int frst)
{	int j;

	for (j = frst; j < nst; j++)
	{	if (reached[n][cs]
		&&  matrix[n][cs][j])
		{	return j;
	}	}
	return -1;
}

static void
print_state_nm(char *p, int *s, char *a)
{	int i;
	printf("%sP", p);
	for (i = 0; i < nclaims; i++)
	{	printf("_%d", s[i]);
	}
	printf("%s", a);
}

static void
create_transition(OneState *s, SQueue *it)
{	int n, from, upto;
	int *F = s->combo;
	int *T = it->state.combo;
	Succ_List *sl;
	Lextok *t;

	if (verbose&64)
	{	print_state_nm("", F, " ");
		print_state_nm("-> ", T, "\t");
	}

	/* check if any of the claims is blocked */
	/* which makes the state a dead-end */
	for (n = 0; n < nclaims; n++)
	{	from = F[n];
		upto = T[n];
		t = matrix[n][from][upto]->n;
		if (verbose&64)
		{	wrap_text("", t, " ");
		}
		if (t->ntyp == 'c'
		&&  t->lft->ntyp == CONST)
		{	if (t->lft->val == 0)	/* i.e., false */
			{	goto done;
	}	}	}

	sl = (Succ_List *) emalloc(sizeof(Succ_List));
	sl->s = it;
	sl->nxt = s->succ;
	s->succ = sl;
done:
	if (verbose&64)
	{	printf("\n");
	}
}

static SQueue *
find_state(int *cs)
{	SQueue *nq, *a = sq;
	int i;

again:	/* check in nq, sq, and then in the render q */
	for (nq = a; nq; nq = nq->nxt)
	{	if (same_state(nq->state.combo, cs))
		{	return nq;	/* found */
	}	}
	if (a == sq && sd)
	{	a = sd;
		goto again; /* check the other stack too */
	} else if (a == sd && render)
	{	a = render;
		goto again;
	}

	nq = (SQueue *) emalloc(sizeof(SQueue));
	nq->state.combo = (int *) emalloc(nclaims * sizeof(int));
	for (i = 0; i < nclaims; i++)
	{	nq->state.combo[i] = cs[i];
	}
	nq->nxt = sq;	/* add to sq stack */
	sq = nq;

	return nq;
}

static SQueue *
retrieve_state(int *s)
{	SQueue *nq, *last = NULL;

	for (nq = sd; nq; last = nq, nq = nq->nxt)
	{	if (same_state(nq->state.combo, s))
		{	if (last)
			{	last->nxt = nq->nxt;
			} else
			{	sd = nq->nxt;	/* 6.4.0: was sd = nq */
			}
			return nq;	/* found */
	}	}

	fatal("cannot happen: retrieve_state", 0);
	return (SQueue *) 0;
}

static void
all_successors(int n, OneState *cur)
{	int i, j = 0;

	if (n >= nclaims)
	{	create_transition(cur, find_state(Nst));
	} else
	{	i = cur->combo[n];
		for (;;)
		{	j = nxt_trans(n, i, j);
			if (j < 0) break;
			Nst[n] = j;
			all_successors(n+1, cur);
			j++;
	}	}
}

static void
gen_product(void)
{	OneState *cur_st;
	SQueue *q;

	find_state(Ist);	/* create initial state */

	while (sq)
	{	if (in_stack(sq, sd))
		{	sq = sq->nxt;
			continue;
		}
		cur_st = &(sq->state);

		q = sq;
		sq = sq->nxt;	/* delete from sq stack */
		q->nxt = sd;	/* and move to done stack */
		sd = q;

		all_successors(0, cur_st);
	}
	/* all states are in the sd queue now */
	prune_dead();
	explore_product();	/* check if added accept-self-loops are reachable */
	prune_accept();

	if (verbose)
	{	print_raw();
	}

	/* PM: merge states with identical successor lists */

	/* all outgoing transitions from accept-states
	   from claim n in copy n connect to states in copy (n+1)%nclaims
	   only accept states from claim 0 in copy 0 are true accept states
	   in the product

	   PM: what about claims that have no accept states (e.g., restrictions)
	*/

	for (unfolding = 0; unfolding < nclaims; unfolding++)
	{	print_product();
	}
}

static void
t_record(int n, Element *e, Element *g)
{	int from = e->seqno, upto = g?g->seqno:0;

	assert(from >= 0 && from < nst);
	assert(upto >= 0 && upto < nst);

	matrix[n][from][upto] = e;
	reached[n][upto] |= 1;
}

static void
get_sub(int n, Element *e)
{
	if (e->n->ntyp == D_STEP
	||  e->n->ntyp == ATOMIC)
	{	fatal("atomic or d_step in never claim product", 0);
	} 
	/* NON_ATOMIC */
	e->n->sl->this->last->nxt = e->nxt;
	get_seq(n, e->n->sl->this);

	t_record(n, e, e->n->sl->this->frst);

}

static void
set_el(int n, Element *e)
{	Element *g;

	if (e->n->ntyp == '@')	/* change to self-loop */
	{	e->n->ntyp = CONST;
		e->n->val = 1;	/* true */
		e->nxt = e;
		g = e;
		mk_accepting(n, e);
	} else

	if (e->n->ntyp == GOTO)
	{	g = get_lab(e->n, 1);
		g = huntele(g, e->status, -1);
	} else if (e->nxt)
	{	g = huntele(e->nxt, e->status, -1);
	} else
	{	g = NULL;
	}

	t_record(n, e, g);
}

static void
get_seq(int n, Sequence *s)
{	SeqList *h;
	Element *e;

	e = huntele(s->frst, s->frst->status, -1);
	for ( ; e; e = e->nxt)
	{	if (e->status & DONE)
		{	goto checklast;
		}
		e->status |= DONE;

		if (e->n->ntyp == UNLESS)
		{	fatal("unless stmnt in never claim product", 0);
		}

		if (e->sub)	/* IF or DO */
		{	Lextok *x = NULL;
			Lextok *y = NULL;
			Lextok *haselse = NULL;

			for (h = e->sub; h; h = h->nxt)
			{	Lextok *t = h->this->frst->n;
				if (t->ntyp == ELSE)
				{	if (verbose&64) printf("else at line %d\n", t->ln);
					haselse = t;
					continue;
				}
				if (t->ntyp != 'c')
				{	fatal("product, 'else' combined with non-condition", 0);
				}

				if (t->lft->ntyp == CONST	/* true */
				&&  t->lft->val == 1
				&&  y == NULL)
				{	y = nn(ZN, CONST, ZN, ZN);
					y->val = 0;
				} else
				{	if (!x)
						x = t;
					else
						x = nn(ZN, OR, x, t);
					if (verbose&64)
					{	wrap_text(" [", x, "]\n");
			}	}	}
			if (haselse)
			{	if (!y)
				{	y = nn(ZN, '!', x, ZN);
				}
				if (verbose&64)
				{	wrap_text(" [else: ", y, "]\n");
				}
				haselse->ntyp = 'c';	/* replace else */
				haselse->lft = y;
			}

			for (h = e->sub; h; h = h->nxt)
			{	t_record(n, e, h->this->frst);
				get_seq(n, h->this);
			}
		} else
		{	if (e->n->ntyp == ATOMIC
			||  e->n->ntyp == D_STEP
			||  e->n->ntyp == NON_ATOMIC)
			{	get_sub(n, e);
			} else 
			{	set_el(n, e);
			}
		}
checklast:	if (e == s->last)
			break;
	}
}

/***** spin: pangen5.c *****/

/* Copyright (c) 1999-2003 by Lucent Technologies, Bell Laboratories.     */
/* All Rights Reserved.  This software is for educational purposes only.  */
/* No guarantee whatsoever is expressed or implied by the distribution of */
/* this code.  Permission is given to distribute this code provided that  */
/* this introductory message is not removed and no monies are exchanged.  */
/* Software written by Gerard J. Holzmann.  For tool documentation see:   */
/*             http://spinroot.com/                                       */
/* Send all bug-reports and/or questions to: bugs@spinroot.com            */

#include "spin.h"
#include "y.tab.h"

typedef struct BuildStack {
	FSM_trans *t;
	struct BuildStack *nxt;
} BuildStack;

extern ProcList	*rdy;
extern int verbose, eventmapnr, claimnr, rvopt, export_ast, u_sync;
extern Element *Al_El;

static FSM_state *fsm_free;
static FSM_trans *trans_free;
static BuildStack *bs, *bf;
static int max_st_id;
static int cur_st_id;
int o_max;
FSM_state *fsm;
FSM_state **fsm_tbl;
FSM_use   *use_free;

static void ana_seq(Sequence *);
static void ana_stmnt(FSM_trans *, Lextok *, int);

extern void AST_slice(void);
extern void AST_store(ProcList *, int);
extern int  has_global(Lextok *);
extern void exit(int);

static void
fsm_table(void)
{	FSM_state *f;
	max_st_id += 2;
	/* fprintf(stderr, "omax %d, max=%d\n", o_max, max_st_id); */
	if (o_max < max_st_id)
	{	o_max = max_st_id;
		fsm_tbl = (FSM_state **) emalloc(max_st_id * sizeof(FSM_state *));
	} else
		memset((char *)fsm_tbl, 0, max_st_id * sizeof(FSM_state *));
	cur_st_id = max_st_id;
	max_st_id = 0;

	for (f = fsm; f; f = f->nxt)
		fsm_tbl[f->from] = f;
}

static int
FSM_DFS(int from, FSM_use *u)
{	FSM_state *f;
	FSM_trans *t;
	FSM_use *v;
	int n;

	if (from == 0)
		return 1;

	f = fsm_tbl[from];

	if (!f)
	{	printf("cannot find state %d\n", from);
		fatal("fsm_dfs: cannot happen\n", (char *) 0);
	}

	if (f->seen)
		return 1;
	f->seen = 1;

	for (t = f->t; t; t = t->nxt)
	{
		for (n = 0; n < 2; n++)
		for (v = t->Val[n]; v; v = v->nxt)
			if (u->var == v->var)
				return n;	/* a read or write */

		if (!FSM_DFS(t->to, u))
			return 0;
	}
	return 1;
}

static void
new_dfs(void)
{	int i;

	for (i = 0; i < cur_st_id; i++)
		if (fsm_tbl[i])
			fsm_tbl[i]->seen = 0;
}

static int
good_dead(Element *e, FSM_use *u)
{
	switch (u->special) {
	case 2:	/* ok if it's a receive */
		if (e->n->ntyp == ASGN
		&&  e->n->rgt->ntyp == CONST
		&&  e->n->rgt->val == 0)
			return 0;
		break;
	case 1:	/* must be able to use oval */
		if (e->n->ntyp != 'c'
		&&  e->n->ntyp != 'r')
			return 0;	/* can't really happen */
		break;
	}
	return 1;
}

#if 0
static int howdeep = 0;
#endif

static int
eligible(FSM_trans *v)
{	Element	*el = ZE;
	Lextok	*lt = ZN;

	if (v) el = v->step;
	if (el) lt = v->step->n;

	if (!lt				/* dead end */
	||  v->nxt			/* has alternatives */
	||  el->esc			/* has an escape */
	||  (el->status&CHECK2)		/* remotely referenced */
	||  lt->ntyp == ATOMIC
	||  lt->ntyp == NON_ATOMIC	/* used for inlines -- should be able to handle this */
	||  lt->ntyp == IF
	||  lt->ntyp == C_CODE
	||  lt->ntyp == C_EXPR
	||  has_lab(el, 0)		/* any label at all */

	||  lt->ntyp == DO
	||  lt->ntyp == UNLESS
	||  lt->ntyp == D_STEP
	||  lt->ntyp == ELSE
	||  lt->ntyp == '@'
	||  lt->ntyp == 'c'
	||  lt->ntyp == 'r'
	||  lt->ntyp == 's')
		return 0;

	if (!(el->status&(2|4)))	/* not atomic */
	{	int unsafe = (el->status&I_GLOB)?1:has_global(el->n);
		if (unsafe)
			return 0;
	}

	return 1;
}

static int
canfill_in(FSM_trans *v)
{	Element	*el = v->step;
	Lextok	*lt = v->step->n;

	if (!lt				/* dead end */
	||  v->nxt			/* has alternatives */
	||  el->esc			/* has an escape */
	||  (el->status&CHECK2))	/* remotely referenced */
		return 0;

	if (!(el->status&(2|4))		/* not atomic */
	&&  ((el->status&I_GLOB)
	||   has_global(el->n)))	/* and not safe */
		return 0;

	return 1;
}

static int
pushbuild(FSM_trans *v)
{	BuildStack *b;

	for (b = bs; b; b = b->nxt)
		if (b->t == v)
			return 0;
	if (bf)
	{	b = bf;
		bf = bf->nxt;
	} else
		b = (BuildStack *) emalloc(sizeof(BuildStack));
	b->t = v;
	b->nxt = bs;
	bs = b;
	return 1;
}

static void
popbuild(void)
{	BuildStack *f;
	if (!bs)
		fatal("cannot happen, popbuild", (char *) 0);
	f = bs;
	bs = bs->nxt;
	f->nxt = bf;
	bf = f;				/* freelist */
}

static int
build_step(FSM_trans *v)
{	FSM_state *f;
	Element	*el;
#if 0
	Lextok	*lt = ZN;
#endif
	int	st;
	int	r;

	if (!v) return -1;

	el = v->step;
	st = v->to;

	if (!el) return -1;

	if (v->step->merge)
		return v->step->merge;	/* already done */

	if (!eligible(v))		/* non-blocking */
		return -1;

	if (!pushbuild(v))		/* cycle detected */
		return -1;		/* break cycle */

	f = fsm_tbl[st];
#if 0
	lt = v->step->n;
	if (verbose&32)
	{	if (++howdeep == 1)
			printf("spin: %s:%d, merge:\n", lt->fn->name, lt->ln);
		printf("\t[%d] <seqno %d>\t", howdeep, el->seqno);
		comment(stdout, lt, 0);
		printf(";\n");
	}
#endif
	r = build_step(f->t);
	v->step->merge = (r == -1) ? st : r;
#if 0
	if (verbose&32)
	{	printf("	merge value: %d (st=%d,r=%d, line %d)\n",
			v->step->merge, st, r, el->n->ln);
		howdeep--;
	}
#endif
	popbuild();

	return v->step->merge;
}

static void
FSM_MERGER(/* char *pname */ void)	/* find candidates for safely merging steps */
{	FSM_state *f, *g;
	FSM_trans *t;
	Lextok	*lt;

	for (f = fsm; f; f = f->nxt)		/* all states */
	for (t = f->t; t; t = t->nxt)		/* all edges */
	{	if (!t->step) continue;		/* happens with 'unless' */

		t->step->merge_in = f->in;	/* ?? */

		if (t->step->merge)
			continue;
		lt = t->step->n;

		if (lt->ntyp == 'c'
		||  lt->ntyp == 'r'
		||  lt->ntyp == 's')	/* blocking stmnts */
			continue;	/* handled in 2nd scan */

		if (!eligible(t))
			continue;

		g = fsm_tbl[t->to];
		if (!g || !eligible(g->t))
		{
#define SINGLES
#ifdef SINGLES
			t->step->merge_single = t->to;
#if 0
			if ((verbose&32))
			{	printf("spin: %s:%d, merge_single:\n\t<seqno %d>\t",
					t->step->n->fn->name,
					t->step->n->ln,
					t->step->seqno);
				comment(stdout, t->step->n, 0);
				printf(";\n");
			}
#endif
#endif
			/* t is an isolated eligible step:
			 *
			 * a merge_start can connect to a proper
			 * merge chain or to a merge_single
			 * a merge chain can be preceded by
			 * a merge_start, but not by a merge_single
			 */

			continue;
		}

		(void) build_step(t);
	}

	/* 2nd scan -- find possible merge_starts */

	for (f = fsm; f; f = f->nxt)		/* all states */
	for (t = f->t; t; t = t->nxt)		/* all edges */
	{	if (!t->step || t->step->merge)
			continue;

		lt = t->step->n;
#if 0
	4.1.3:
	an rv send operation inside an atomic, *loses* atomicity
	when executed
	and should therefore never be merged with a subsequent
	statement within the atomic sequence
	the same is not true for non-rv send operations
#endif

		if (lt->ntyp == 'c'	/* potentially blocking stmnts */
		||  lt->ntyp == 'r'
		||  (lt->ntyp == 's' && u_sync == 0))	/* added !u_sync in 4.1.3 */
		{	if (!canfill_in(t))		/* atomic, non-global, etc. */
				continue;

			g = fsm_tbl[t->to];
			if (!g || !g->t || !g->t->step)
				continue;
			if (g->t->step->merge)
				t->step->merge_start = g->t->step->merge;
#ifdef SINGLES
			else if (g->t->step->merge_single)
				t->step->merge_start = g->t->step->merge_single;
#endif
#if 0
			if ((verbose&32)
			&& t->step->merge_start)
			{	printf("spin: %s:%d, merge_START:\n\t<seqno %d>\t",
						lt->fn->name, lt->ln,
						t->step->seqno);
				comment(stdout, lt, 0);
				printf(";\n");
			}
#endif
		}
	}
}

static void
FSM_ANA(void)
{	FSM_state *f;
	FSM_trans *t;
	FSM_use *u, *v, *w;
	int n;

	for (f = fsm; f; f = f->nxt)		/* all states */
	for (t = f->t; t; t = t->nxt)		/* all edges */
	for (n = 0; n < 2; n++)			/* reads and writes */
	for (u = t->Val[n]; u; u = u->nxt)
	{	if (!u->var->context	/* global */
		||   u->var->type == CHAN
		||   u->var->type == STRUCT)
			continue;
		new_dfs();
		if (FSM_DFS(t->to, u))	/* cannot hit read before hitting write */
			u->special = n+1;	/* means, reset to 0 after use */
	}

	if (!export_ast)
	for (f = fsm; f; f = f->nxt)
	for (t = f->t; t; t = t->nxt)
	for (n = 0; n < 2; n++)
	for (u = t->Val[n], w = (FSM_use *) 0; u; )
	{	if (u->special)
		{	v = u->nxt;
			if (!w)			/* remove from list */
				t->Val[n] = v;
			else
				w->nxt = v;
#if q
			if (verbose&32)
			{	printf("%s : %3d:  %d -> %d \t",
					t->step->n->fn->name,
					t->step->n->ln,
					f->from,
					t->to);
				comment(stdout, t->step->n, 0);
				printf("\t%c%d: %s\n", n==0?'R':'L',
					u->special, u->var->name);
			}
#endif
			if (good_dead(t->step, u))
			{	u->nxt = t->step->dead;	/* insert into dead */
				t->step->dead = u;
			}
			u = v;
		} else
		{	w = u;
			u = u->nxt;
	}	}
}

void
rel_use(FSM_use *u)
{
	if (!u) return;
	rel_use(u->nxt);
	u->var = (Symbol *) 0;
	u->special = 0;
	u->nxt = use_free;
	use_free = u;
}

static void
rel_trans(FSM_trans *t)
{
	if (!t) return;
	rel_trans(t->nxt);
	rel_use(t->Val[0]);
	rel_use(t->Val[1]);
	t->Val[0] = t->Val[1] = (FSM_use *) 0;
	t->nxt = trans_free;
	trans_free = t;
}

static void
rel_state(FSM_state *f)
{
	if (!f) return;
	rel_state(f->nxt);
	rel_trans(f->t);
	f->t = (FSM_trans *) 0;
	f->nxt = fsm_free;
	fsm_free = f;
}

static void
FSM_DEL(void)
{
	rel_state(fsm);
	fsm = (FSM_state *) 0;
}

static FSM_state *
mkstate(int s)
{	FSM_state *f;

	/* fsm_tbl isn't allocated yet */
	for (f = fsm; f; f = f->nxt)
		if (f->from == s)
			break;
	if (!f)
	{	if (fsm_free)
		{	f = fsm_free;
			memset(f, 0, sizeof(FSM_state));
			fsm_free = fsm_free->nxt;
		} else
			f = (FSM_state *) emalloc(sizeof(FSM_state));
		f->from = s;
		f->t = (FSM_trans *) 0;
		f->nxt = fsm;
		fsm = f;
		if (s > max_st_id)
			max_st_id = s;
	}
	return f;
}

static FSM_trans *
get_trans(int to)
{	FSM_trans *t;

	if (trans_free)
	{	t = trans_free;
		memset(t, 0, sizeof(FSM_trans));
		trans_free = trans_free->nxt;
	} else
		t = (FSM_trans *) emalloc(sizeof(FSM_trans));

	t->to = to;
	return t;
}

static void
FSM_EDGE(int from, int to, Element *e)
{	FSM_state *f;
	FSM_trans *t;

	f = mkstate(from);	/* find it or else make it */
	t = get_trans(to);

	t->step = e;
	t->nxt = f->t;
	f->t = t;

	f = mkstate(to);
	f->in++;

	if (export_ast)
	{	t = get_trans(from);
		t->step = e;
		t->nxt = f->p;	/* from is a predecessor of to */
		f->p = t;
	}

	if (t->step)
		ana_stmnt(t, t->step->n, 0);
}

#define LVAL	1
#define RVAL	0

static void
ana_var(FSM_trans *t, Lextok *now, int usage)
{	FSM_use *u, *v;

	if (!t || !now || !now->sym)
		return;

	if (now->sym->name[0] == '_'
	&&  (strcmp(now->sym->name, "_") == 0
	||   strcmp(now->sym->name, "_pid") == 0
	||   strcmp(now->sym->name, "_last") == 0))
		return;

	v = t->Val[usage];
	for (u = v; u; u = u->nxt)
		if (u->var == now->sym)
			return;	/* it's already there */

	if (!now->lft)
	{	/* not for array vars -- it's hard to tell statically
		   if the index would, at runtime, evaluate to the
		   same values at lval and rval references
		*/
		if (use_free)
		{	u = use_free;
			use_free = use_free->nxt;
		} else
			u = (FSM_use *) emalloc(sizeof(FSM_use));
	
		u->var = now->sym;
		u->nxt = t->Val[usage];
		t->Val[usage] = u;
	} else
		 ana_stmnt(t, now->lft, RVAL);	/* index */

	if (now->sym->type == STRUCT
	&&  now->rgt
	&&  now->rgt->lft)
		ana_var(t, now->rgt->lft, usage);
}

static void
ana_stmnt(FSM_trans *t, Lextok *now, int usage)
{	Lextok *v;

	if (!t || !now) return;

	switch (now->ntyp) {
	case '.':
	case BREAK:
	case GOTO:
	case CONST:
	case TIMEOUT:
	case NONPROGRESS:
	case  ELSE:
	case '@':
	case 'q':
	case IF:
	case DO:
	case ATOMIC:
	case NON_ATOMIC:
	case D_STEP:
	case C_CODE:
	case C_EXPR:
		break;

	case '!':	
	case UMIN:
	case '~':
	case ENABLED:
	case PC_VAL:
	case LEN:
	case FULL:
	case EMPTY:
	case NFULL:
	case NEMPTY:
	case ASSERT:
	case 'c':
		ana_stmnt(t, now->lft, RVAL);
		break;

	case '/':
	case '*':
	case '-':
	case '+':
	case '%':
	case '&':
	case '^':
	case '|':
	case LT:
	case GT:
	case LE:
	case GE:
	case NE:
	case EQ:
	case OR:
	case AND:
	case LSHIFT:
	case RSHIFT:
		ana_stmnt(t, now->lft, RVAL);
		ana_stmnt(t, now->rgt, RVAL);
		break;

	case ASGN:
		ana_stmnt(t, now->lft, LVAL);
		ana_stmnt(t, now->rgt, RVAL);
		break;

	case PRINT:
	case RUN:
		for (v = now->lft; v; v = v->rgt)
			ana_stmnt(t, v->lft, RVAL);
		break;

	case PRINTM:
		if (now->lft && !now->lft->ismtyp)
			ana_stmnt(t, now->lft, RVAL);
		break;

	case 's':
		ana_stmnt(t, now->lft, RVAL);
		for (v = now->rgt; v; v = v->rgt)
			ana_stmnt(t, v->lft, RVAL);
		break;

	case 'R':
	case 'r':
		ana_stmnt(t, now->lft, RVAL);
		for (v = now->rgt; v; v = v->rgt)
		{	if (v->lft->ntyp == EVAL)
				ana_stmnt(t, v->lft->lft, RVAL);
			else
			if (v->lft->ntyp != CONST
			&&  now->ntyp != 'R')		/* was v->lft->ntyp */
				ana_stmnt(t, v->lft, LVAL);
		}
		break;

	case '?':
		ana_stmnt(t, now->lft, RVAL);
		if (now->rgt)
		{	ana_stmnt(t, now->rgt->lft, RVAL);
			ana_stmnt(t, now->rgt->rgt, RVAL);
		}
		break;

	case NAME:
		ana_var(t, now, usage);
		break;

	case   'p':	/* remote ref */
		ana_stmnt(t, now->lft->lft, RVAL);	/* process id */
		ana_var(t, now, RVAL);
		ana_var(t, now->rgt, RVAL);
		break;

	default:
		printf("spin: %s:%d, bad node type %d (ana_stmnt)\n",
			now->fn->name, now->ln, now->ntyp);
		fatal("aborting", (char *) 0);
	}
}

void
ana_src(int dataflow, int merger)	/* called from main.c and guided.c */
{	ProcList *p;
	Element *e;
#if 0
	int counter = 1;
#endif
	for (p = rdy; p; p = p->nxt)
	{
		ana_seq(p->s);
		fsm_table();

		e = p->s->frst;
#if 0
		if (dataflow || merger)
		{	printf("spin: %d, optimizing '%s'",
				counter++, p->n->name);
			fflush(stdout);
		}
#endif
		if (dataflow)
		{	FSM_ANA();
		}
		if (merger)
		{	FSM_MERGER(/* p->n->name */);
			huntele(e, e->status, -1)->merge_in = 1; /* start-state */
#if 0
			printf("\n");
#endif
		}
		if (export_ast)
			AST_store(p, huntele(e, e->status, -1)->seqno);

		FSM_DEL();
	}
	for (e = Al_El; e; e = e->Nxt)
	{
		if (!(e->status&DONE) && (verbose&32))
		{	printf("unreachable code: ");
			printf("%s:%3d  ", e->n->fn->name, e->n->ln);
			comment(stdout, e->n, 0);
			printf("\n");
		}
		e->status &= ~DONE;
	}
	if (export_ast)
	{	AST_slice();
		alldone(0);	/* changed in 5.3.0: was exit(0) */
	}
}

void
spit_recvs(FILE *f1, FILE *f2)	/* called from pangen2.c */
{	Element *e;
	Sequence *s;
	extern int Unique;

	fprintf(f1, "unsigned char Is_Recv[%d];\n", Unique);

	fprintf(f2, "void\nset_recvs(void)\n{\n");
	for (e = Al_El; e; e = e->Nxt)
	{	if (!e->n) continue;

		switch (e->n->ntyp) {
		case 'r':
markit:			fprintf(f2, "\tIs_Recv[%d] = 1;\n", e->Seqno);
			break;
		case D_STEP:
			s = e->n->sl->this;
			switch (s->frst->n->ntyp) {
			case DO:
				fatal("unexpected: do at start of d_step", (char *) 0);
			case IF: /* conservative: fall through */
			case 'r': goto markit;
			}
			break;
		}
	}
	fprintf(f2, "}\n");

	if (rvopt)
	{
	fprintf(f2, "int\nno_recvs(int me)\n{\n");
	fprintf(f2, "	int h; uchar ot; short tt;\n");
	fprintf(f2, "	Trans *t;\n");
	fprintf(f2, "	for (h = BASE; h < (int) now._nr_pr; h++)\n");
	fprintf(f2, "	{	if (h == me) continue;\n");
	fprintf(f2, "		tt = (short) ((P0 *)pptr(h))->_p;\n");
	fprintf(f2, "		ot = (uchar) ((P0 *)pptr(h))->_t;\n");
	fprintf(f2, "		for (t = trans[ot][tt]; t; t = t->nxt)\n");
	fprintf(f2, "			if (Is_Recv[t->t_id]) return 0;\n");
	fprintf(f2, "	}\n");
	fprintf(f2, "	return 1;\n");
	fprintf(f2, "}\n");
	}
}

static void
ana_seq(Sequence *s)
{	SeqList *h;
	Sequence *t;
	Element *e, *g;
	int From, To;

	for (e = s->frst; e; e = e->nxt)
	{	if (e->status & DONE)
			goto checklast;

		e->status |= DONE;

		From = e->seqno;

		if (e->n->ntyp == UNLESS)
			ana_seq(e->sub->this);
		else if (e->sub)
		{	for (h = e->sub; h; h = h->nxt)
			{	g = huntstart(h->this->frst);
				To = g->seqno;

				if (g->n->ntyp != 'c'
				||  g->n->lft->ntyp != CONST
				||  g->n->lft->val != 0
				||  g->esc)
					FSM_EDGE(From, To, e);
				/* else it's a dead link */
			}
			for (h = e->sub; h; h = h->nxt)
				ana_seq(h->this);
		} else if (e->n->ntyp == ATOMIC
			||  e->n->ntyp == D_STEP
			||  e->n->ntyp == NON_ATOMIC)
		{
			t = e->n->sl->this;
			g = huntstart(t->frst);
			t->last->nxt = e->nxt;
			To = g->seqno;
			FSM_EDGE(From, To, e);

			ana_seq(t);
		} else 
		{	if (e->n->ntyp == GOTO)
			{	g = get_lab(e->n, 1);
				g = huntele(g, e->status, -1);
				To = g->seqno;
			} else if (e->nxt)
			{	g = huntele(e->nxt, e->status, -1);
				To = g->seqno;
			} else
				To = 0;

			FSM_EDGE(From, To, e);

			if (e->esc
			&&  e->n->ntyp != GOTO
			&&  e->n->ntyp != '.')
			for (h = e->esc; h; h = h->nxt)
			{	g = huntstart(h->this->frst);
				To = g->seqno;
				FSM_EDGE(From, To, ZE);
				ana_seq(h->this);
			}
		}

checklast:	if (e == s->last)
			break;
	}
}

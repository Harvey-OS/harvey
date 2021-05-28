/***** tl_spin: tl_buchi.c *****/

/*
 * This file is part of the public release of Spin. It is subject to the
 * terms in the LICENSE file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com
 *
 * Based on the translation algorithm by Gerth, Peled, Vardi, and Wolper,
 * presented at the PSTV Conference, held in 1995, Warsaw, Poland 1995.
 */

#include "tl.h"

extern int tl_verbose, tl_clutter, Total, Max_Red;
extern char *claim_name;
extern void  put_uform(void);

FILE	*tl_out;	/* if standalone: = stdout; */

typedef struct Transition {
	Symbol *name;
	Node *cond;
	int redundant, merged, marked;
	struct Transition *nxt;
} Transition;

typedef struct State {
	Symbol	*name;
	Transition *trans;
	Graph	*colors;
	unsigned char redundant;
	unsigned char accepting;
	unsigned char reachable;
	struct State *nxt;
} State;

static State *never = (State *) 0;
static int hitsall;

void
ini_buchi(void)
{
	never = (State *) 0;
	hitsall = 0;
}

static int
sametrans(Transition *s, Transition *t)
{
	if (strcmp(s->name->name, t->name->name) != 0)
		return 0;
	return isequal(s->cond, t->cond);
}

static Node *
Prune(Node *p)
{
	if (p)
	switch (p->ntyp) {
	case PREDICATE:
	case NOT:
	case FALSE:
	case TRUE:
#ifdef NXT
	case NEXT:
#endif
	case CEXPR:
		return p;
	case OR:
		p->lft = Prune(p->lft);
		if (!p->lft)
		{	releasenode(1, p->rgt);
			return ZN;
		}
		p->rgt = Prune(p->rgt);
		if (!p->rgt)
		{	releasenode(1, p->lft);
			return ZN;
		}
		return p;
	case AND:
		p->lft = Prune(p->lft);
		if (!p->lft)
			return Prune(p->rgt);
		p->rgt = Prune(p->rgt);
		if (!p->rgt)
			return p->lft;
		return p;
	}
	releasenode(1, p);
	return ZN;
}

static State *
findstate(char *nm)
{	State *b;
	for (b = never; b; b = b->nxt)
		if (!strcmp(b->name->name, nm))
			return b;
	if (strcmp(nm, "accept_all"))
	{	if (strncmp(nm, "accept", 6))
		{	int i; char altnm[64];
			for (i = 0; i < 64; i++)
				if (nm[i] == '_')
					break;
			if (i >= 64)
				Fatal("name too long %s", nm);
			sprintf(altnm, "accept%s", &nm[i]);
			return findstate(altnm);
		}
	/*	Fatal("buchi: no state %s", nm); */
	}
	return (State *) 0;
}

static void
Dfs(State *b)
{	Transition *t;

	if (!b || b->reachable) return;
	b->reachable = 1;

	if (b->redundant)
		printf("/* redundant state %s */\n",
			b->name->name);
	for (t = b->trans; t; t = t->nxt)
	{	if (!t->redundant)
		{	Dfs(findstate(t->name->name));
			if (!hitsall
			&&  strcmp(t->name->name, "accept_all") == 0)
				hitsall = 1;
		}
	}
}

void
retarget(char *from, char *to)
{	State *b;
	Transition *t;
	Symbol *To = tl_lookup(to);

	if (tl_verbose) printf("replace %s with %s\n", from, to);

	for (b = never; b; b = b->nxt)
	{	if (!strcmp(b->name->name, from))
			b->redundant = 1;
		else
		for (t = b->trans; t; t = t->nxt)
		{	if (!strcmp(t->name->name, from))
				t->name = To;
	}	}
}

#ifdef NXT
static Node *
nonxt(Node *n)
{
	switch (n->ntyp) {
	case U_OPER:
	case V_OPER:
	case NEXT:
		return ZN;
	case OR:
		n->lft = nonxt(n->lft);
		n->rgt = nonxt(n->rgt);
		if (!n->lft || !n->rgt)
			return True;
		return n;
	case AND:
		n->lft = nonxt(n->lft);
		n->rgt = nonxt(n->rgt);
		if (!n->lft)
		{	if (!n->rgt)
				n = ZN;
			else
				n = n->rgt;
		} else if (!n->rgt)
			n = n->lft;
		return n;
	}
	return n;
}
#endif

static Node *
combination(Node *s, Node *t)
{	Node *nc;
#ifdef NXT
	Node *a = nonxt(s);
	Node *b = nonxt(t);

	if (tl_verbose)
	{	printf("\tnonxtA: "); dump(a);
		printf("\n\tnonxtB: "); dump(b);
		printf("\n");
	}
	/* if there's only a X(f), its equivalent to true */
	if (!a || !b)
		nc = True;
	else
		nc = tl_nn(OR, a, b);
#else
	nc = tl_nn(OR, s, t);
#endif
	if (tl_verbose)
	{	printf("\tcombo: "); dump(nc);
		printf("\n");
	}
	return nc;
}

Node *
unclutter(Node *n, char *snm)
{	Node *t, *s, *v, *u;
	Symbol *w;

	/* check only simple cases like !q && q */
	for (t = n; t; t = t->rgt)
	{	if (t->rgt)
		{	if (t->ntyp != AND || !t->lft)
				return n;
			if (t->lft->ntyp != PREDICATE
#ifdef NXT
			&&  t->lft->ntyp != NEXT
#endif
			&&  t->lft->ntyp != NOT)
				return n;
		} else
		{	if (t->ntyp != PREDICATE
#ifdef NXT
			&&  t->ntyp != NEXT
#endif
			&&  t->ntyp != NOT)
				return n;
		}
	}

	for (t = n; t; t = t->rgt)
	{	if (t->rgt)
			v = t->lft;
		else
			v = t;
		if (v->ntyp == NOT
		&&  v->lft->ntyp == PREDICATE)
		{	w = v->lft->sym;
			for (s = n; s; s = s->rgt)
			{	if (s == t) continue;
				if (s->rgt)
					u = s->lft;
				else
					u = s;
				if (u->ntyp == PREDICATE
				&&  strcmp(u->sym->name, w->name) == 0)
				{	if (tl_verbose)
					{	printf("BINGO %s:\t", snm);
						dump(n);
						printf("\n");
					}
					return False;
				}
			}
	}	}
	return n;
}

static void
clutter(void)
{	State *p;
	Transition *s;

	for (p = never; p; p = p->nxt)
	for (s = p->trans; s; s = s->nxt)
	{	s->cond = unclutter(s->cond, p->name->name);
		if (s->cond
		&&  s->cond->ntyp == FALSE)
		{	if (s != p->trans 
			||  s->nxt)
				s->redundant = 1;
		}
	}
}

static void
showtrans(State *a)
{	Transition *s;

	for (s = a->trans; s; s = s->nxt)
	{	printf("%s ", s->name?s->name->name:"-");
		dump(s->cond);
		printf(" %d %d %d\n", s->redundant, s->merged, s->marked);
	}
}

static int
mergetrans(void)
{	State *b;
	Transition *s, *t;
	Node *nc; int cnt = 0;

	for (b = never; b; b = b->nxt)
	{	if (!b->reachable) continue;

		for (s = b->trans; s; s = s->nxt)
		{	if (s->redundant) continue;

			for (t = s->nxt; t; t = t->nxt)
			if (!t->redundant
			&&  !strcmp(s->name->name, t->name->name))
			{	if (tl_verbose)
				{	printf("===\nstate %s, trans to %s redundant\n",
					b->name->name, s->name->name);
					showtrans(b);
					printf(" conditions ");
					dump(s->cond); printf(" <-> ");
					dump(t->cond); printf("\n");
				}

				if (!s->cond) /* same as T */
				{	releasenode(1, t->cond); /* T or t */
					nc = True;
				} else if (!t->cond)
				{	releasenode(1, s->cond);
					nc = True;
				} else
				{	nc = combination(s->cond, t->cond);
				}
				t->cond = rewrite(nc);
				t->merged = 1;
				s->redundant = 1;
				cnt++;
				break;
	}	}	}
	return cnt;
}

static int
all_trans_match(State *a, State *b)
{	Transition *s, *t;
	int found, result = 0;

	if (a->accepting != b->accepting)
		goto done;

	for (s = a->trans; s; s = s->nxt)
	{	if (s->redundant) continue;
		found = 0;
		for (t = b->trans; t; t = t->nxt)
		{	if (t->redundant) continue;
			if (sametrans(s, t))
			{	found = 1;
				t->marked = 1;
				break;
		}	}
		if (!found)
			goto done;
	}
	for (s = b->trans; s; s = s->nxt)
	{	if (s->redundant || s->marked) continue;
		found = 0;
		for (t = a->trans; t; t = t->nxt)
		{	if (t->redundant) continue;
			if (sametrans(s, t))
			{	found = 1;
				break;
		}	}
		if (!found)
			goto done;
	}
	result = 1;
done:
	for (s = b->trans; s; s = s->nxt)
		s->marked = 0;
	return result;
}

#ifndef NO_OPT
#define BUCKY
#endif

#ifdef BUCKY
static int
all_bucky(State *a, State *b)
{	Transition *s, *t;
	int found, result = 0;

	for (s = a->trans; s; s = s->nxt)
	{	if (s->redundant) continue;
		found = 0;
		for (t = b->trans; t; t = t->nxt)
		{	if (t->redundant) continue;

			if (isequal(s->cond, t->cond))
			{	if (strcmp(s->name->name, b->name->name) == 0
				&&  strcmp(t->name->name, a->name->name) == 0)
				{	found = 1;	/* they point to each other */
					t->marked = 1;
					break;
				}
				if (strcmp(s->name->name, t->name->name) == 0
				&&  strcmp(s->name->name, "accept_all") == 0)
				{	found = 1;
					t->marked = 1;
					break;
				/* same exit from which there is no return */
				}
			}
		}
		if (!found)
			goto done;
	}
	for (s = b->trans; s; s = s->nxt)
	{	if (s->redundant || s->marked) continue;
		found = 0;
		for (t = a->trans; t; t = t->nxt)
		{	if (t->redundant) continue;

			if (isequal(s->cond, t->cond))
			{	if (strcmp(s->name->name, a->name->name) == 0
				&&  strcmp(t->name->name, b->name->name) == 0)
				{	found = 1;
					t->marked = 1;
					break;
				}
				if (strcmp(s->name->name, t->name->name) == 0
				&&  strcmp(s->name->name, "accept_all") == 0)
				{	found = 1;
					t->marked = 1;
					break;
				}
			}
		}
		if (!found)
			goto done;
	}
	result = 1;
done:
	for (s = b->trans; s; s = s->nxt)
		s->marked = 0;
	return result;
}

static int
buckyballs(void)
{	State *a, *b, *c, *d;
	int m, cnt=0;

	do {
		m = 0; cnt++;
		for (a = never; a; a = a->nxt)
		{	if (!a->reachable) continue;

			if (a->redundant) continue;

			for (b = a->nxt; b; b = b->nxt)
			{	if (!b->reachable) continue;

				if (b->redundant) continue;

				if (all_bucky(a, b))
				{	m++;
					if (tl_verbose)
					{	printf("%s bucky match %s\n",
						a->name->name, b->name->name);
					}

					if (a->accepting && !b->accepting)
					{	if (strcmp(b->name->name, "T0_init") == 0)
						{	c = a; d = b;
							b->accepting = 1;
						} else
						{	c = b; d = a;
						}
					} else
					{	c = a; d = b;
					}

					retarget(c->name->name, d->name->name);
					if (!strncmp(c->name->name, "accept", 6)
					&&  Max_Red == 0)
					{	char buf[64];
						sprintf(buf, "T0%s", &(c->name->name[6]));
						retarget(buf, d->name->name);
					}
					break;
				}
		}	}
	} while (m && cnt < 10);
	return cnt-1;
}
#endif

static int
mergestates(int v)
{	State *a, *b;
	int m, cnt=0;

	if (tl_verbose)
		return 0;

	do {
		m = 0; cnt++;
		for (a = never; a; a = a->nxt)
		{	if (v && !a->reachable) continue;

			if (a->redundant) continue; /* 3.3.10 */

			for (b = a->nxt; b; b = b->nxt)
			{	if (v && !b->reachable) continue;

				if (b->redundant) continue; /* 3.3.10 */

				if (all_trans_match(a, b))
				{	m++;
					if (tl_verbose)
					{	printf("%d: state %s equals state %s\n",
						cnt, a->name->name, b->name->name);
						showtrans(a);
						printf("==\n");
						showtrans(b);
					}
					retarget(a->name->name, b->name->name);
					if (!strncmp(a->name->name, "accept", 6)
					&&  Max_Red == 0)
					{	char buf[64];
						sprintf(buf, "T0%s", &(a->name->name[6]));
						retarget(buf, b->name->name);
					}
					break;
				}
#if 0
				else if (tl_verbose)
				{	printf("\n%d: state %s differs from state %s [%d,%d]\n",
						cnt, a->name->name, b->name->name,
						a->accepting, b->accepting);
					showtrans(a);
					printf("==\n");
					showtrans(b);
					printf("\n");
				}
#endif
		}	}
	} while (m && cnt < 10);
	return cnt-1;
}

static int tcnt;

static void
rev_trans(Transition *t) /* print transitions  in reverse order... */
{
	if (!t) return;
	rev_trans(t->nxt);

	if (t->redundant && !tl_verbose) return;

	if (strcmp(t->name->name, "accept_all") == 0)	/* 6.2.4 */
	{	/* not d_step because there may be remote refs */
		fprintf(tl_out, "\t:: atomic { (");
		if (dump_cond(t->cond, t->cond, 1))
			fprintf(tl_out, "1");
		fprintf(tl_out, ") -> assert(!(");
		if (dump_cond(t->cond, t->cond, 1))
			fprintf(tl_out, "1");
		fprintf(tl_out, ")) }\n");
	} else
	{	fprintf(tl_out, "\t:: (");
		if (dump_cond(t->cond, t->cond, 1))
			fprintf(tl_out, "1");
		fprintf(tl_out, ") -> goto %s\n", t->name->name);
	}
	tcnt++;
}

static void
printstate(State *b)
{
	if (!b || (!tl_verbose && !b->reachable)) return;

	b->reachable = 0;	/* print only once */
	fprintf(tl_out, "%s:\n", b->name->name);

	if (tl_verbose)
	{	fprintf(tl_out, "	/* ");
		dump(b->colors->Other);
		fprintf(tl_out, " */\n");
	}

	if (strncmp(b->name->name, "accept", 6) == 0
	&&  Max_Red == 0)
		fprintf(tl_out, "T0%s:\n", &(b->name->name[6]));

	fprintf(tl_out, "\tdo\n");
	tcnt = 0;
	rev_trans(b->trans);
	if (!tcnt) fprintf(tl_out, "\t:: false\n");
	fprintf(tl_out, "\tod;\n");
	Total++;
}

void
addtrans(Graph *col, char *from, Node *op, char *to)
{	State *b;
	Transition *t;

	t = (Transition *) tl_emalloc(sizeof(Transition));
	t->name = tl_lookup(to);
	t->cond = Prune(dupnode(op));

	if (tl_verbose)
	{	printf("\n%s <<\t", from); dump(op);
		printf("\n\t"); dump(t->cond);
		printf(">> %s\n", t->name->name);
	}
	if (t->cond) t->cond = rewrite(t->cond);

	for (b = never; b; b = b->nxt)
		if (!strcmp(b->name->name, from))
		{	t->nxt = b->trans;
			b->trans = t;
			return;
		}
	b = (State *) tl_emalloc(sizeof(State));
	b->name   = tl_lookup(from);
	b->colors = col;
	b->trans  = t;
	if (!strncmp(from, "accept", 6))
		b->accepting = 1;
	b->nxt = never;
	never  = b;
}

static void
clr_reach(void)
{	State *p;
	for (p = never; p; p = p->nxt)
		p->reachable = 0;
	hitsall = 0;
}

void
fsm_print(void)
{	State *b; int cnt1, cnt2=0;

	if (tl_clutter) clutter();

	b = findstate("T0_init");
	if (b && (Max_Red == 0))
		b->accepting = 1;

	mergestates(0); 
	b = findstate("T0_init");

	fprintf(tl_out, "never %s {    /* ", claim_name?claim_name:"");
		put_uform();
	fprintf(tl_out, " */\n");

	do {
		clr_reach();
		Dfs(b);
		cnt1 = mergetrans();
		cnt2 = mergestates(1);
		if (tl_verbose)
			printf("/* >>%d,%d<< */\n", cnt1, cnt2);
	} while (cnt2 > 0);

#ifdef BUCKY
	buckyballs();
	clr_reach();
	Dfs(b);
#endif
	if (b && b->accepting)
		fprintf(tl_out, "accept_init:\n");

	if (!b && !never)
	{	fprintf(tl_out, "	0 /* false */;\n");
	} else
	{	printstate(b);	/* init state must be first */
		for (b = never; b; b = b->nxt)
			printstate(b);
	}
	if (hitsall)
	fprintf(tl_out, "accept_all:\n	skip\n");
	fprintf(tl_out, "}\n");
}

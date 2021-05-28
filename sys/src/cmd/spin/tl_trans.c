/***** tl_spin: tl_trans.c *****/

/*
 * This file is part of the public release of Spin. It is subject to the
 * terms in the LICENSE file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com
 *
 * Based on the translation algorithm by Gerth, Peled, Vardi, and Wolper,
 * presented at the PSTV Conference, held in 1995, Warsaw, Poland 1995.
 */

#include "tl.h"

extern FILE	*tl_out;
extern int	tl_errs, tl_verbose, tl_terse, newstates, state_cnt;

int	Stack_mx=0, Max_Red=0, Total=0;

static Mapping	*Mapped = (Mapping *) 0;
static Graph	*Nodes_Set = (Graph *) 0;
static Graph	*Nodes_Stack = (Graph *) 0;

static char	*dumpbuf = NULL;
static size_t dumpbuf_size = 0;
static size_t dumpbuf_capacity = 0;

static int	Red_cnt  = 0;
static int	Lab_cnt  = 0;
static int	Base     = 0;
static int	Stack_sz = 0;

static Graph	*findgraph(char *);
static Graph	*pop_stack(void);
static Node	*Duplicate(Node *);
static Node	*flatten(Node *);
static Symbol	*catSlist(Symbol *, Symbol *);
static Symbol	*dupSlist(Symbol *);
static char	*newname(void);
static int	choueka(Graph *, int);
static int	not_new(Graph *);
static int	set_prefix(char *, int, Graph *);
static void	Addout(char *, char *);
static void	fsm_trans(Graph *, int, char *);
static void	mkbuchi(void);
static void	expand_g(Graph *);
static void	fixinit(Node *);
static void	liveness(Node *);
static void	mk_grn(Node *);
static void	mk_red(Node *);
static void	ng(Symbol *, Symbol *, Node *, Node *, Node *);
static void	push_stack(Graph *);
static void	sdump(Node *);

void
append_to_dumpbuf(char* s)
{
    size_t len = strlen(s);
    size_t size_needed = dumpbuf_size + len + 1;
    if (size_needed > dumpbuf_capacity) {
        dumpbuf = tl_erealloc(dumpbuf, size_needed, dumpbuf_capacity);
        dumpbuf_capacity = size_needed;
    }

    strncpy(&(dumpbuf[dumpbuf_size]), s, len + 1);
    dumpbuf_size += len;
}

void
ini_trans(void)
{
	Stack_mx = 0;
	Max_Red = 0;
	Total = 0;

	Mapped = (Mapping *) 0;
	Nodes_Set = (Graph *) 0;
	Nodes_Stack = (Graph *) 0;

	dumpbuf_capacity = 4096;
	dumpbuf = tl_emalloc(dumpbuf_capacity);
	dumpbuf_size = 0;
	memset(dumpbuf, 0, dumpbuf_capacity);
	Red_cnt  = 0;
	Lab_cnt  = 0;
	Base     = 0;
	Stack_sz = 0;
}

static void
dump_graph(Graph *g)
{	Node *n1;

	printf("\n\tnew:\t");
	for (n1 = g->New; n1; n1 = n1->nxt)
	{ dump(n1); printf(", "); }
	printf("\n\told:\t");
	for (n1 = g->Old; n1; n1 = n1->nxt)
	{ dump(n1); printf(", "); }
	printf("\n\tnxt:\t");
	for (n1 = g->Next; n1; n1 = n1->nxt)
	{ dump(n1); printf(", "); }
	printf("\n\tother:\t");
	for (n1 = g->Other; n1; n1 = n1->nxt)
	{ dump(n1); printf(", "); }
	printf("\n");
}

static void
push_stack(Graph *g)
{
	if (!g) return;

	g->nxt = Nodes_Stack;
	Nodes_Stack = g;
	if (tl_verbose)
	{	Symbol *z;
		printf("\nPush %s, from ", g->name->name);
		for (z = g->incoming; z; z = z->next)
			printf("%s, ", z->name);
		dump_graph(g);
	}
	Stack_sz++;
	if (Stack_sz > Stack_mx) Stack_mx = Stack_sz;
}

static Graph *
pop_stack(void)
{	Graph *g = Nodes_Stack;

	if (g) Nodes_Stack = g->nxt;

	Stack_sz--;

	return g;
}

static char *
newname(void)
{	static char buf[32];
	sprintf(buf, "S%d", state_cnt++);
	return buf;
}

static int
has_clause(int tok, Graph *p, Node *n)
{	Node *q, *qq;

	switch (n->ntyp) {
	case AND:
		return	has_clause(tok, p, n->lft) &&
			has_clause(tok, p, n->rgt);
	case OR:
		return	has_clause(tok, p, n->lft) ||
			has_clause(tok, p, n->rgt);
	}

	for (q = p->Other; q; q = q->nxt)
	{	qq = right_linked(q);
		if (anywhere(tok, n, qq))
			return 1;
	}
	return 0;
}

static void
mk_grn(Node *n)
{	Graph *p;

	if (!n) return;

	n = right_linked(n);
more:
	for (p = Nodes_Set; p; p = p->nxt)
		if (p->outgoing
		&&  has_clause(AND, p, n))
		{	p->isgrn[p->grncnt++] =
				(unsigned char) Red_cnt;
			Lab_cnt++;
		}

	if (n->ntyp == U_OPER)	/* 3.4.0 */
	{	n = n->rgt;
		goto more;
	}
}

static void
mk_red(Node *n)
{	Graph *p;

	if (!n) return;

	n = right_linked(n);
	for (p = Nodes_Set; p; p = p->nxt)
	{	if (p->outgoing
		&&  has_clause(0, p, n))
		{	if (p->redcnt >= 63)
				Fatal("too many Untils", (char *)0);
			p->isred[p->redcnt++] =
				(unsigned char) Red_cnt;
			Lab_cnt++; Max_Red = Red_cnt;
	}	}
}

static void
liveness(Node *n)
{
	if (n)
	switch (n->ntyp) {
#ifdef NXT
	case NEXT:
		liveness(n->lft);
		break;
#endif
	case U_OPER:
		Red_cnt++;
		mk_red(n);
		mk_grn(n->rgt);
		/* fall through */
	case V_OPER:
	case OR: case AND:
		liveness(n->lft);
		liveness(n->rgt);
		break;
	}
}

static Graph *
findgraph(char *nm)
{	Graph	*p;
	Mapping *m;

	for (p = Nodes_Set; p; p = p->nxt)
		if (!strcmp(p->name->name, nm))
			return p;
	for (m = Mapped; m; m = m->nxt)
		if (strcmp(m->from, nm) == 0)
			return m->to;

	printf("warning: node %s not found\n", nm);
	return (Graph *) 0;
}

static void
Addout(char *to, char *from)
{	Graph	*p = findgraph(from);
	Symbol	*s;

	if (!p) return;
	s = getsym(tl_lookup(to));
	s->next = p->outgoing;
	p->outgoing = s;
}

#ifdef NXT
int
only_nxt(Node *n)
{
	switch (n->ntyp) {
	case NEXT:
		return 1;
	case OR:
	case AND:
		return only_nxt(n->rgt) && only_nxt(n->lft);
	default:
		return 0;
	}
}
#endif

int
dump_cond(Node *pp, Node *r, int first)
{	Node *q;
	int frst = first;

	if (!pp) return frst;

	q = dupnode(pp);
	q = rewrite(q);

	if (q->ntyp == CEXPR)
	{	if (!frst) fprintf(tl_out, " && ");
		fprintf(tl_out, "c_expr { ");
		dump_cond(q->lft, r, 1);
		fprintf(tl_out, " } ");
		frst = 0;
		return frst;
	}

	if (q->ntyp == PREDICATE
	||  q->ntyp == NOT
#ifndef NXT
	||  q->ntyp == OR
#endif
	||  q->ntyp == FALSE)
	{	if (!frst) fprintf(tl_out, " && ");
		dump(q);
		frst = 0;
#ifdef NXT
	} else if (q->ntyp == OR)
	{	if (!frst) fprintf(tl_out, " && ");
		fprintf(tl_out, "((");
		frst = dump_cond(q->lft, r, 1);

		if (!frst)
			fprintf(tl_out, ") || (");
		else
		{	if (only_nxt(q->lft))
			{	fprintf(tl_out, "1))");
				return 0;
			}
		}

		frst = dump_cond(q->rgt, r, 1);

		if (frst)
		{	if (only_nxt(q->rgt))
				fprintf(tl_out, "1");
			else
				fprintf(tl_out, "0");
			frst = 0;
		}

		fprintf(tl_out, "))");
#endif
	} else  if (q->ntyp == V_OPER
		&& !anywhere(AND, q->rgt, r))
	{	frst = dump_cond(q->rgt, r, frst);
	} else  if (q->ntyp == AND)
	{
		frst = dump_cond(q->lft, r, frst);
		frst = dump_cond(q->rgt, r, frst);
	}

	return frst;
}

static int
choueka(Graph *p, int count)
{	int j, k, incr_cnt = 0;

	for (j = count; j <= Max_Red; j++) /* for each acceptance class */
	{	int delta = 0;

		/* is state p labeled Grn-j OR not Red-j ? */

		for (k = 0; k < (int) p->grncnt; k++)
			if (p->isgrn[k] == j)
			{	delta = 1;
				break;
			}
		if (delta)
		{	incr_cnt++;
			continue;
		}
		for (k = 0; k < (int) p->redcnt; k++)
			if (p->isred[k] == j)
			{	delta = 1;
				break;
			}

		if (delta) break;

		incr_cnt++;
	}
	return incr_cnt;
}

static int
set_prefix(char *pref, int count, Graph *r2)
{	int incr_cnt = 0;	/* acceptance class 'count' */

	if (Lab_cnt == 0
	||  Max_Red == 0)
		sprintf(pref, "accept");	/* new */
	else if (count >= Max_Red)
		sprintf(pref, "T0");		/* cycle */
	else
	{	incr_cnt = choueka(r2, count+1);
		if (incr_cnt + count >= Max_Red)
			sprintf(pref, "accept"); /* last hop */
		else
			sprintf(pref, "T%d", count+incr_cnt);
	}
	return incr_cnt;
}

static void
fsm_trans(Graph *p, int count, char *curnm)
{	Graph	*r;
	Symbol	*s;
	char	prefix[128], nwnm[256];

	if (!p->outgoing)
		addtrans(p, curnm, False, "accept_all");

	for (s = p->outgoing; s; s = s->next)
	{	r = findgraph(s->name);
		if (!r) continue;
		if (r->outgoing)
		{	(void) set_prefix(prefix, count, r);
			sprintf(nwnm, "%s_%s", prefix, s->name);
		} else
			strcpy(nwnm, "accept_all");

		if (tl_verbose)
		{	printf("maxred=%d, count=%d, curnm=%s, nwnm=%s ",
				Max_Red, count, curnm, nwnm);
			printf("(greencnt=%d,%d, redcnt=%d,%d)\n",
				r->grncnt, r->isgrn[0],
				r->redcnt, r->isred[0]);
		}
		addtrans(p, curnm, r->Old, nwnm);
	}
}

static void
mkbuchi(void)
{	Graph	*p;
	int	k;
	char	curnm[64];

	for (k = 0; k <= Max_Red; k++)
	for (p = Nodes_Set; p; p = p->nxt)
	{	if (!p->outgoing)
			continue;
		if (k != 0
		&& !strcmp(p->name->name, "init")
		&&  Max_Red != 0)
			continue;

		if (k == Max_Red
		&&  strcmp(p->name->name, "init") != 0)
			strcpy(curnm, "accept_");
		else
			sprintf(curnm, "T%d_", k);

		strcat(curnm, p->name->name);

		fsm_trans(p, k, curnm);
	}
	fsm_print();
}

static Symbol *
dupSlist(Symbol *s)
{	Symbol *p1, *p2, *p3, *d = ZS;

	for (p1 = s; p1; p1 = p1->next)
	{	for (p3 = d; p3; p3 = p3->next)
		{	if (!strcmp(p3->name, p1->name))
				break;
		}
		if (p3) continue;	/* a duplicate */

		p2 = getsym(p1);
		p2->next = d;
		d = p2;
	}
	return d;
}

static Symbol *
catSlist(Symbol *a, Symbol *b)
{	Symbol *p1, *p2, *p3, *tmp;

	/* remove duplicates from b */
	for (p1 = a; p1; p1 = p1->next)
	{	p3 = ZS;
		p2 = b;
		while (p2)
		{ if (strcmp(p1->name, p2->name))
			{	p3 = p2;
				p2 = p2->next;
				continue;
			}
			tmp = p2->next;
			tfree((void *) p2);
			p2 = tmp;
			if (p3)
				p3->next = tmp;
			else
				b = tmp;
	}	}
	if (!a) return b;
	if (!b) return a;
	if (!b->next)
	{	b->next = a;
		return b;
	}
	/* find end of list */
	for (p1 = a; p1->next; p1 = p1->next)
		;
	p1->next = b;
	return a;
}

static void
fixinit(Node *orig)
{	Graph	*p1, *g;
	Symbol	*q1, *q2 = ZS;

	ng(tl_lookup("init"), ZS, ZN, ZN, ZN);
	p1 = pop_stack();
	if (p1)
	{	p1->nxt = Nodes_Set;
		p1->Other = p1->Old = orig;
		Nodes_Set = p1;
	}

	for (g = Nodes_Set; g; g = g->nxt)
	{	for (q1 = g->incoming; q1; q1 = q2)
		{	q2 = q1->next;
			Addout(g->name->name, q1->name);
			tfree((void *) q1);
		}
		g->incoming = ZS;
	}
}

static Node *
flatten(Node *p)
{	Node *q, *r, *z = ZN;

	for (q = p; q; q = q->nxt)
	{	r = dupnode(q);
		if (z)
			z = tl_nn(AND, r, z);
		else
			z = r;
	}
	if (!z) return z;
	z = rewrite(z);
	return z;
}

static Node *
Duplicate(Node *n)
{	Node *n1, *n2, *lst = ZN, *d = ZN;

	for (n1 = n; n1; n1 = n1->nxt)
	{	n2 = dupnode(n1);
		if (lst)
		{	lst->nxt = n2;
			lst = n2;
		} else
			d = lst = n2;
	}
	return d;
}

static void
ng(Symbol *s, Symbol *in, Node *isnew, Node *isold, Node *next)
{	Graph *g = (Graph *) tl_emalloc(sizeof(Graph));

	if (s)     g->name = s;
	else       g->name = tl_lookup(newname());

	if (in)    g->incoming = dupSlist(in);
	if (isnew) g->New  = flatten(isnew);
	if (isold) g->Old  = Duplicate(isold);
	if (next)  g->Next = flatten(next);

	push_stack(g);
}

static void
sdump(Node *n)
{
	switch (n->ntyp) {
	case PREDICATE:	append_to_dumpbuf(n->sym->name);
			break;
	case U_OPER:	append_to_dumpbuf("U");
			goto common2;
	case V_OPER:	append_to_dumpbuf("V");
			goto common2;
	case OR:	append_to_dumpbuf("|");
			goto common2;
	case AND:	append_to_dumpbuf("&");
common2:		sdump(n->rgt);
common1:		sdump(n->lft);
			break;
#ifdef NXT
	case NEXT:	append_to_dumpbuf("X");
			goto common1;
#endif
	case CEXPR:	append_to_dumpbuf("c_expr {");
			sdump(n->lft);
			append_to_dumpbuf("}");
			break;
	case NOT:	append_to_dumpbuf("!");
			goto common1;
	case TRUE:	append_to_dumpbuf("T");
			break;
	case FALSE:	append_to_dumpbuf("F");
			break;
	default:	append_to_dumpbuf("?");
			break;
	}
}

Symbol *
DoDump(Node *n)
{
	if (!n) return ZS;

	if (n->ntyp == PREDICATE)
		return n->sym;

    if (dumpbuf) {
        dumpbuf[0] = '\0';
        dumpbuf_size = 0;
        sdump(n);
        return tl_lookup(dumpbuf);
    }

    sdump(n);
    return tl_lookup("");
}

static int
not_new(Graph *g)
{	Graph	*q1; Node *tmp, *n1, *n2;
	Mapping	*map;

	tmp = flatten(g->Old);	/* duplicate, collapse, normalize */
	g->Other = g->Old;	/* non normalized full version */
	g->Old = tmp;

	g->oldstring = DoDump(g->Old);

	tmp = flatten(g->Next);
	g->nxtstring = DoDump(tmp);

	if (tl_verbose) dump_graph(g);

	Debug2("\tformula-old: [%s]\n", g->oldstring?g->oldstring->name:"true");
	Debug2("\tformula-nxt: [%s]\n", g->nxtstring?g->nxtstring->name:"true");
	for (q1 = Nodes_Set; q1; q1 = q1->nxt)
	{	Debug2("	compare old to: %s", q1->name->name);
		Debug2(" [%s]", q1->oldstring?q1->oldstring->name:"true");

		Debug2("	compare nxt to: %s", q1->name->name);
		Debug2(" [%s]", q1->nxtstring?q1->nxtstring->name:"true");

		if (q1->oldstring != g->oldstring
		||  q1->nxtstring != g->nxtstring)
		{	Debug(" => different\n");
			continue;
		}
		Debug(" => match\n");

		if (g->incoming)
			q1->incoming = catSlist(g->incoming, q1->incoming);

		/* check if there's anything in g->Other that needs
		   adding to q1->Other
		*/
		for (n2 = g->Other; n2; n2 = n2->nxt)
		{	for (n1 = q1->Other; n1; n1 = n1->nxt)
				if (isequal(n1, n2))
					break;
			if (!n1)
			{	Node *n3 = dupnode(n2);
				/* don't mess up n2->nxt */
				n3->nxt = q1->Other;
				q1->Other = n3;
		}	}

		map = (Mapping *) tl_emalloc(sizeof(Mapping));
	  	map->from = g->name->name;
	  	map->to   = q1;
	  	map->nxt = Mapped;
	  	Mapped = map;

		for (n1 = g->Other; n1; n1 = n2)
		{	n2 = n1->nxt;
			releasenode(1, n1);
		}
		for (n1 = g->Old; n1; n1 = n2)
		{	n2 = n1->nxt;
			releasenode(1, n1);
		}
		for (n1 = g->Next; n1; n1 = n2)
		{	n2 = n1->nxt;
			releasenode(1, n1);
		}
		return 1;
	}

	if (newstates) tl_verbose=1;
	Debug2("	New Node %s [", g->name->name);
	for (n1 = g->Old; n1; n1 = n1->nxt)
	{ Dump(n1); Debug(", "); }
	Debug2("] nr %d\n", Base);
	if (newstates) tl_verbose=0;

	Base++;
	g->nxt = Nodes_Set;
	Nodes_Set = g;

	return 0;
}

static void
expand_g(Graph *g)
{	Node *now, *n1, *n2, *nx;
	int can_release;

	if (!g->New)
	{	Debug2("\nDone with %s", g->name->name);
		if (tl_verbose) dump_graph(g);
		if (not_new(g))
		{	if (tl_verbose) printf("\tIs Not New\n");
			return;
		}
		if (g->Next)
		{	Debug("	Has Next [");
			for (n1 = g->Next; n1; n1 = n1->nxt)
			{ Dump(n1); Debug(", "); }
			Debug("]\n");

			ng(ZS, getsym(g->name), g->Next, ZN, ZN);
		}
		return;
	}

	if (tl_verbose)
	{	Symbol *z;
		printf("\nExpand %s, from ", g->name->name);
		for (z = g->incoming; z; z = z->next)
			printf("%s, ", z->name);
		printf("\n\thandle:\t"); Explain(g->New->ntyp);
		dump_graph(g);
	}

	if (g->New->ntyp == AND)
	{	if (g->New->nxt)
		{	n2 = g->New->rgt;
			while (n2->nxt)
				n2 = n2->nxt;
			n2->nxt = g->New->nxt;
		}
		n1 = n2 = g->New->lft;
		while (n2->nxt)
			n2 = n2->nxt;
		n2->nxt = g->New->rgt;

		releasenode(0, g->New);

		g->New = n1;
		push_stack(g);
		return;
	}

	can_release = 0;	/* unless it need not go into Old */
	now = g->New;
	g->New = g->New->nxt;
	now->nxt = ZN;

	if (now->ntyp != TRUE)
	{	if (g->Old)
		{	for (n1 = g->Old; n1->nxt; n1 = n1->nxt)
				if (isequal(now, n1))
				{	can_release = 1;
					goto out;
				}
			n1->nxt = now;
		} else
			g->Old = now;
	}
out:
	switch (now->ntyp) {
	case FALSE:
		push_stack(g);
		break;
	case TRUE:
		releasenode(1, now);
		push_stack(g);
		break;
	case PREDICATE:
	case NOT:
	case CEXPR:
		if (can_release) releasenode(1, now);
		push_stack(g);
		break;
	case V_OPER:
		Assert(now->rgt != ZN, now->ntyp);
		Assert(now->lft != ZN, now->ntyp);
		Assert(now->rgt->nxt == ZN, now->ntyp);
		Assert(now->lft->nxt == ZN, now->ntyp);
		n1 = now->rgt;
		n1->nxt = g->New;

		if (can_release)
			nx = now;
		else
			nx = getnode(now); /* now also appears in Old */
		nx->nxt = g->Next;

		n2 = now->lft;
		n2->nxt = getnode(now->rgt);
		n2->nxt->nxt = g->New;
		g->New = flatten(n2);
		push_stack(g);
		ng(ZS, g->incoming, n1, g->Old, nx);
		break;

	case U_OPER:
		Assert(now->rgt->nxt == ZN, now->ntyp);
		Assert(now->lft->nxt == ZN, now->ntyp);
		n1 = now->lft;

		if (can_release)
			nx = now;
		else
			nx = getnode(now); /* now also appears in Old */
		nx->nxt = g->Next;

		n2 = now->rgt;
		n2->nxt = g->New;

		goto common;

#ifdef NXT
	case NEXT:
		Assert(now->lft != ZN, now->ntyp);
		nx = dupnode(now->lft);
		nx->nxt = g->Next;
		g->Next = nx;
		if (can_release) releasenode(0, now);
		push_stack(g);
		break;
#endif

	case OR:
		Assert(now->rgt->nxt == ZN, now->ntyp);
		Assert(now->lft->nxt == ZN, now->ntyp);
		n1 = now->lft;
		nx = g->Next;

		n2 = now->rgt;
		n2->nxt = g->New;
common:
		n1->nxt = g->New;

		ng(ZS, g->incoming, n1, g->Old, nx);
		g->New = flatten(n2);

		if (can_release) releasenode(1, now);

		push_stack(g);
		break;
	}
}

Node *
twocases(Node *p)
{	Node *q;
	/* 1: ([]p1 && []p2) == [](p1 && p2) */
	/* 2: (<>p1 || <>p2) == <>(p1 || p2) */

	if (!p) return p;

	switch(p->ntyp) {
	case AND:
	case OR:
	case U_OPER:
	case V_OPER:
		p->lft = twocases(p->lft);
		p->rgt = twocases(p->rgt);
		break;
#ifdef NXT
	case NEXT:
#endif
	case NOT:
		p->lft = twocases(p->lft);
		break;

	default:
		break;
	}
	if (p->ntyp == AND	/* 1 */
	&&  p->lft->ntyp == V_OPER
	&&  p->lft->lft->ntyp == FALSE
	&&  p->rgt->ntyp == V_OPER
	&&  p->rgt->lft->ntyp == FALSE)
	{	q = tl_nn(V_OPER, False,
			tl_nn(AND, p->lft->rgt, p->rgt->rgt));
	} else
	if (p->ntyp == OR	/* 2 */
	&&  p->lft->ntyp == U_OPER
	&&  p->lft->lft->ntyp == TRUE
	&&  p->rgt->ntyp == U_OPER
	&&  p->rgt->lft->ntyp == TRUE)
	{	q = tl_nn(U_OPER, True,
			tl_nn(OR, p->lft->rgt, p->rgt->rgt));
	} else
		q = p;
	return q;
}

void
trans(Node *p)
{	Node	*op;
	Graph	*g;

	if (!p || tl_errs) return;

	p = twocases(p);

	if (tl_verbose || tl_terse)
	{	fprintf(tl_out, "\t/* Normlzd: ");
		dump(p);
		fprintf(tl_out, " */\n");
	}
	if (tl_terse)
		return;

	op = dupnode(p);

	ng(ZS, getsym(tl_lookup("init")), p, ZN, ZN);
	while ((g = Nodes_Stack) != (Graph *) 0)
	{	Nodes_Stack = g->nxt;
		expand_g(g);
	}
	if (newstates)
		return;

	fixinit(p);
	liveness(flatten(op));	/* was: liveness(op); */

	mkbuchi();
	if (tl_verbose)
	{	printf("/*\n");
		printf(" * %d states in Streett automaton\n", Base);
		printf(" * %d Streett acceptance conditions\n", Max_Red);
		printf(" * %d Buchi states\n", Total);
		printf(" */\n");
	}
}

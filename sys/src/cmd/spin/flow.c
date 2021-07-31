/***** spin: flow.c *****/

/* Copyright (c) 1989-2003 by Lucent Technologies, Bell Laboratories.     */
/* All Rights Reserved.  This software is for educational purposes only.  */
/* No guarantee whatsoever is expressed or implied by the distribution of */
/* this code.  Permission is given to distribute this code provided that  */
/* this introductory message is not removed and no monies are exchanged.  */
/* Software written by Gerard J. Holzmann.  For tool documentation see:   */
/*             http://spinroot.com/                                       */
/* Send all bug-reports and/or questions to: bugs@spinroot.com            */

#include "spin.h"
#include "y.tab.h"

extern Symbol	*Fname;
extern int	nr_errs, lineno, verbose;
extern short	has_unless, has_badelse;

Element *Al_El = ZE;
Label	*labtab = (Label *) 0;
int	Unique=0, Elcnt=0, DstepStart = -1;

static Lbreak	*breakstack = (Lbreak *) 0;
static Lextok	*innermost;
static SeqList	*cur_s = (SeqList *) 0;
static int	break_id=0;

static Element	*if_seq(Lextok *);
static Element	*new_el(Lextok *);
static Element	*unless_seq(Lextok *);
static void	add_el(Element *, Sequence *);
static void	attach_escape(Sequence *, Sequence *);
static void	mov_lab(Symbol *, Element *, Element *);
static void	walk_atomic(Element *, Element *, int);

void
open_seq(int top)
{	SeqList *t;
	Sequence *s = (Sequence *) emalloc(sizeof(Sequence));

	t = seqlist(s, cur_s);
	cur_s = t;
	if (top) Elcnt = 1;
}

void
rem_Seq(void)
{
	DstepStart = Unique;
}

void
unrem_Seq(void)
{
	DstepStart = -1;
}

static int
Rjumpslocal(Element *q, Element *stop)
{	Element *lb, *f;
	SeqList *h;

	/* allow no jumps out of a d_step sequence */
	for (f = q; f && f != stop; f = f->nxt)
	{	if (f && f->n && f->n->ntyp == GOTO)
		{	lb = get_lab(f->n, 0);
			if (!lb || lb->Seqno < DstepStart)
			{	lineno = f->n->ln;
				Fname = f->n->fn;
				return 0;
		}	}
		for (h = f->sub; h; h = h->nxt)
		{	if (!Rjumpslocal(h->this->frst, h->this->last))
				return 0;
	
	}	}
	return 1;
}

void
cross_dsteps(Lextok *a, Lextok *b)
{
	if (a && b
	&&  a->indstep != b->indstep)
	{	lineno = a->ln;
		Fname  = a->fn;
		fatal("jump into d_step sequence", (char *) 0);
	}
}

int
is_skip(Lextok *n)
{
	return (n->ntyp == PRINT
	||	n->ntyp == PRINTM
	||	(n->ntyp == 'c'
		&& n->lft
		&& n->lft->ntyp == CONST
		&& n->lft->val  == 1));
}

void
check_sequence(Sequence *s)
{	Element *e, *le = ZE;
	Lextok *n;
	int cnt = 0;

	for (e = s->frst; e; le = e, e = e->nxt)
	{	n = e->n;
		if (is_skip(n) && !has_lab(e, 0))
		{	cnt++;
			if (cnt > 1
			&&  n->ntyp != PRINT
			&&  n->ntyp != PRINTM)
			{	if (verbose&32)
					printf("spin: line %d %s, redundant skip\n",
						n->ln, n->fn->name);
				if (e != s->frst
				&&  e != s->last
				&&  e != s->extent)
				{	e->status |= DONE;	/* not unreachable */
					le->nxt = e->nxt;	/* remove it */
					e = le;
				}
			}
		} else
			cnt = 0;
	}
}

void
prune_opts(Lextok *n)
{	SeqList *l;
	extern Symbol *context;
	extern char *claimproc;

	if (!n
	|| (context && claimproc && strcmp(context->name, claimproc) == 0))
		return;

	for (l = n->sl; l; l = l->nxt)	/* find sequences of unlabeled skips */
		check_sequence(l->this);
}

Sequence *
close_seq(int nottop)
{	Sequence *s = cur_s->this;
	Symbol *z;

	if (nottop > 0 && (z = has_lab(s->frst, 0)))
	{	printf("error: (%s:%d) label %s placed incorrectly\n",
			(s->frst->n)?s->frst->n->fn->name:"-",
			(s->frst->n)?s->frst->n->ln:0,
			z->name);
		switch (nottop) {
		case 1:
			printf("=====> stmnt unless Label: stmnt\n");
			printf("sorry, cannot jump to the guard of an\n");
			printf("escape (it is not a unique state)\n");
			break;
		case 2:
			printf("=====> instead of  ");
			printf("\"Label: stmnt unless stmnt\"\n");
			printf("=====> always use  ");
			printf("\"Label: { stmnt unless stmnt }\"\n");
			break;
		case 3:
			printf("=====> instead of  ");
			printf("\"atomic { Label: statement ... }\"\n");
			printf("=====> always use  ");
			printf("\"Label: atomic { statement ... }\"\n");
			break;
		case 4:
			printf("=====> instead of  ");
			printf("\"d_step { Label: statement ... }\"\n");
			printf("=====> always use  ");
			printf("\"Label: d_step { statement ... }\"\n");
			break;
		case 5:
			printf("=====> instead of  ");
			printf("\"{ Label: statement ... }\"\n");
			printf("=====> always use  ");
			printf("\"Label: { statement ... }\"\n");
			break;
		case 6:
			printf("=====>instead of\n");
			printf("	do (or if)\n");
			printf("	:: ...\n");
			printf("	:: Label: statement\n");
			printf("	od (of fi)\n");
			printf("=====>always use\n");
			printf("Label:	do (or if)\n");
			printf("	:: ...\n");
			printf("	:: statement\n");
			printf("	od (or fi)\n");
			break;
		case 7:
			printf("cannot happen - labels\n");
			break;
		}
		alldone(1);
	}

	if (nottop == 4
	&& !Rjumpslocal(s->frst, s->last))
		fatal("non_local jump in d_step sequence", (char *) 0);

	cur_s = cur_s->nxt;
	s->maxel = Elcnt;
	s->extent = s->last;
	if (!s->last)
		fatal("sequence must have at least one statement", (char *) 0);
	return s;
}

Lextok *
do_unless(Lextok *No, Lextok *Es)
{	SeqList *Sl;
	Lextok *Re = nn(ZN, UNLESS, ZN, ZN);
	Re->ln = No->ln;
	Re->fn = No->fn;

	has_unless++;
	if (Es->ntyp == NON_ATOMIC)
		Sl = Es->sl;
	else
	{	open_seq(0); add_seq(Es);
		Sl = seqlist(close_seq(1), 0);
	}

	if (No->ntyp == NON_ATOMIC)
	{	No->sl->nxt = Sl;
		Sl = No->sl;
	} else	if (No->ntyp == ':'
		&& (No->lft->ntyp == NON_ATOMIC
		||  No->lft->ntyp == ATOMIC
		||  No->lft->ntyp == D_STEP))
	{
		int tok = No->lft->ntyp;

		No->lft->sl->nxt = Sl;
		Re->sl = No->lft->sl;

		open_seq(0); add_seq(Re);
		Re = nn(ZN, tok, ZN, ZN);
		Re->sl = seqlist(close_seq(7), 0);
		Re->ln = No->ln;
		Re->fn = No->fn;

		Re = nn(No, ':', Re, ZN);	/* lift label */
		Re->ln = No->ln;
		Re->fn = No->fn;
		return Re;
	} else
	{	open_seq(0); add_seq(No);
		Sl = seqlist(close_seq(2), Sl);
	}

	Re->sl = Sl;
	return Re;
}

SeqList *
seqlist(Sequence *s, SeqList *r)
{	SeqList *t = (SeqList *) emalloc(sizeof(SeqList));

	t->this = s;
	t->nxt = r;
	return t;
}

static Element *
new_el(Lextok *n)
{	Element *m;

	if (n)
	{	if (n->ntyp == IF || n->ntyp == DO)
			return if_seq(n);
		if (n->ntyp == UNLESS)
			return unless_seq(n);
	}
	m = (Element *) emalloc(sizeof(Element));
	m->n = n;
	m->seqno = Elcnt++;
	m->Seqno = Unique++;
	m->Nxt = Al_El; Al_El = m;
	return m;
}

static int
has_chanref(Lextok *n)
{
	if (!n) return 0;

	switch (n->ntyp) {
	case 's':	case 'r':
#if 0
	case 'R':	case LEN:
#endif
	case FULL:	case NFULL:
	case EMPTY:	case NEMPTY:
		return 1;
	default:
		break;
	}
	if (has_chanref(n->lft))
		return 1;

	return has_chanref(n->rgt);
}

void
loose_ends(void)	/* properly tie-up ends of sub-sequences */
{	Element *e, *f;

	for (e = Al_El; e; e = e->Nxt)
	{	if (!e->n
		||  !e->nxt)
			continue;
		switch (e->n->ntyp) {
		case ATOMIC:
		case NON_ATOMIC:
		case D_STEP:
			f = e->nxt;
			while (f && f->n->ntyp == '.')
				f = f->nxt;
			if (0) printf("link %d, {%d .. %d} -> %d (ntyp=%d) was %d\n",
				e->seqno,
				e->n->sl->this->frst->seqno,
				e->n->sl->this->last->seqno,
				f?f->seqno:-1, f?f->n->ntyp:-1,
				e->n->sl->this->last->nxt?e->n->sl->this->last->nxt->seqno:-1);
			if (!e->n->sl->this->last->nxt)
				e->n->sl->this->last->nxt = f;
			else
			{	if (e->n->sl->this->last->nxt->n->ntyp != GOTO)
				{	if (!f || e->n->sl->this->last->nxt->seqno != f->seqno)
					non_fatal("unexpected: loose ends", (char *)0);
				} else
					e->n->sl->this->last = e->n->sl->this->last->nxt;
				/*
				 * fix_dest can push a goto into the nxt position
				 * in that case the goto wins and f is not needed
				 * but the last fields needs adjusting
				 */
			}
			break;
	}	}
}

static Element *
if_seq(Lextok *n)
{	int	tok = n->ntyp;
	SeqList	*s  = n->sl;
	Element	*e  = new_el(ZN);
	Element	*t  = new_el(nn(ZN,'.',ZN,ZN)); /* target */
	SeqList	*z, *prev_z = (SeqList *) 0;
	SeqList *move_else  = (SeqList *) 0;	/* to end of optionlist */
	int	ref_chans = 0;

	for (z = s; z; z = z->nxt)
	{	if (!z->this->frst)
			continue;
		if (z->this->frst->n->ntyp == ELSE)
		{	if (move_else)
				fatal("duplicate `else'", (char *) 0);
			if (z->nxt)	/* is not already at the end */
			{	move_else = z;
				if (prev_z)
					prev_z->nxt = z->nxt;
				else
					s = n->sl = z->nxt;
				continue;
			}
		} else
			ref_chans |= has_chanref(z->this->frst->n);
		prev_z = z;
	}
	if (move_else)
	{	move_else->nxt = (SeqList *) 0;
		/* if there is no prev, then else was at the end */
		if (!prev_z) fatal("cannot happen - if_seq", (char *) 0);
		prev_z->nxt = move_else;
		prev_z = move_else;
	}
	if (prev_z
	&&  ref_chans
	&&  prev_z->this->frst->n->ntyp == ELSE)
	{	prev_z->this->frst->n->val = 1;
		has_badelse++;
		non_fatal("dubious use of 'else' combined with i/o,",
			(char *)0);
		nr_errs--;
	}

	e->n = nn(n, tok, ZN, ZN);
	e->n->sl = s;			/* preserve as info only */
	e->sub = s;
	for (z = s; z; prev_z = z, z = z->nxt)
		add_el(t, z->this);	/* append target */
	if (tok == DO)
	{	add_el(t, cur_s->this); /* target upfront */
		t = new_el(nn(n, BREAK, ZN, ZN)); /* break target */
		set_lab(break_dest(), t);	/* new exit  */
		breakstack = breakstack->nxt;	/* pop stack */
	}
	add_el(e, cur_s->this);
	add_el(t, cur_s->this);
	return e;			/* destination node for label */
}

static void
escape_el(Element *f, Sequence *e)
{	SeqList *z;

	for (z = f->esc; z; z = z->nxt)
		if (z->this == e)
			return;	/* already there */

	/* cover the lower-level escapes of this state */
	for (z = f->esc; z; z = z->nxt)
		attach_escape(z->this, e);

	/* now attach escape to the state itself */

	f->esc = seqlist(e, f->esc);	/* in lifo order... */
#ifdef DEBUG
	printf("attach %d (", e->frst->Seqno);
	comment(stdout, e->frst->n, 0);
	printf(")	to %d (", f->Seqno);
	comment(stdout, f->n, 0);
	printf(")\n");
#endif
	switch (f->n->ntyp) {
	case UNLESS:
		attach_escape(f->sub->this, e);
		break;
	case IF:
	case DO:
		for (z = f->sub; z; z = z->nxt)
			attach_escape(z->this, e);
		break;
	case D_STEP:
		/* attach only to the guard stmnt */
		escape_el(f->n->sl->this->frst, e);
		break;
	case ATOMIC:
	case NON_ATOMIC:
		/* attach to all stmnts */
		attach_escape(f->n->sl->this, e);
		break;
	}
}

static void
attach_escape(Sequence *n, Sequence *e)
{	Element *f;

	for (f = n->frst; f; f = f->nxt)
	{	escape_el(f, e);
		if (f == n->extent)
			break;
	}
}

static Element *
unless_seq(Lextok *n)
{	SeqList	*s  = n->sl;
	Element	*e  = new_el(ZN);
	Element	*t  = new_el(nn(ZN,'.',ZN,ZN)); /* target */
	SeqList	*z;

	e->n = nn(n, UNLESS, ZN, ZN);
	e->n->sl = s;			/* info only */
	e->sub = s;

	/* need 2 sequences: normal execution and escape */
	if (!s || !s->nxt || s->nxt->nxt)
		fatal("unexpected unless structure", (char *)0);

	/* append the target state to both */
	for (z = s; z; z = z->nxt)
		add_el(t, z->this);

	/* attach escapes to all states in normal sequence */
	attach_escape(s->this, s->nxt->this);

	add_el(e, cur_s->this);
	add_el(t, cur_s->this);
#ifdef DEBUG
	printf("unless element (%d,%d):\n", e->Seqno, t->Seqno);
	for (z = s; z; z = z->nxt)
	{	Element *x; printf("\t%d,%d,%d :: ",
		z->this->frst->Seqno,
		z->this->extent->Seqno,
		z->this->last->Seqno);
		for (x = z->this->frst; x; x = x->nxt)
			printf("(%d)", x->Seqno);
		printf("\n");
	}
#endif
	return e;
}

Element *
mk_skip(void)
{	Lextok  *t = nn(ZN, CONST, ZN, ZN);
	t->val = 1;
	return new_el(nn(ZN, 'c', t, ZN));
}

static void
add_el(Element *e, Sequence *s)
{
	if (e->n->ntyp == GOTO)
	{	Symbol *z = has_lab(e, (1|2|4));
		if (z)
		{	Element *y; /* insert a skip */
			y = mk_skip();
			mov_lab(z, e, y); /* inherit label */
			add_el(y, s);
	}	}
#ifdef DEBUG
	printf("add_el %d after %d -- ",
	e->Seqno, (s->last)?s->last->Seqno:-1);
	comment(stdout, e->n, 0);
	printf("\n");
#endif
	if (!s->frst)
		s->frst = e;
	else
		s->last->nxt = e;
	s->last = e;
}

static Element *
colons(Lextok *n)
{
	if (!n)
		return ZE;
	if (n->ntyp == ':')
	{	Element *e = colons(n->lft);
		set_lab(n->sym, e);
		return e;
	}
	innermost = n;
	return new_el(n);
}

void
add_seq(Lextok *n)
{	Element *e;

	if (!n) return;
	innermost = n;
	e = colons(n);
	if (innermost->ntyp != IF
	&&  innermost->ntyp != DO
	&&  innermost->ntyp != UNLESS)
		add_el(e, cur_s->this);
}

void
set_lab(Symbol *s, Element *e)
{	Label *l; extern Symbol *context;

	if (!s) return;
	for (l = labtab; l; l = l->nxt)
		if (l->s == s && l->c == context)
		{	non_fatal("label %s redeclared", s->name);
			break;
		}
	l = (Label *) emalloc(sizeof(Label));
	l->s = s;
	l->c = context;
	l->e = e;
	l->nxt = labtab;
	labtab = l;
}

Element *
get_lab(Lextok *n, int md)
{	Label *l;
	Symbol *s = n->sym;

	for (l = labtab; l; l = l->nxt)
		if (s == l->s)
			return (l->e);

	lineno = n->ln;
	Fname = n->fn;
	if (md) fatal("undefined label %s", s->name);
	return ZE;
}

Symbol *
has_lab(Element *e, int special)
{	Label *l;

	for (l = labtab; l; l = l->nxt)
	{	if (e != l->e)
			continue;
		if (special == 0
		||  ((special&1) && !strncmp(l->s->name, "accept", 6))
		||  ((special&2) && !strncmp(l->s->name, "end", 3))
		||  ((special&4) && !strncmp(l->s->name, "progress", 8)))
			return (l->s);
	}
	return ZS;
}

static void
mov_lab(Symbol *z, Element *e, Element *y)
{	Label *l;

	for (l = labtab; l; l = l->nxt)
		if (e == l->e)
		{	l->e = y;
			return;
		}
	if (e->n)
	{	lineno = e->n->ln;
		Fname  = e->n->fn;
	}
	fatal("cannot happen - mov_lab %s", z->name);
}

void
fix_dest(Symbol *c, Symbol *a)		/* c:label name, a:proctype name */
{	Label *l; extern Symbol *context;

#if 0
	printf("ref to label '%s' in proctype '%s', search:\n",
		c->name, a->name);
	for (l = labtab; l; l = l->nxt)
		printf("	%s in	%s\n", l->s->name, l->c->name);
#endif

	for (l = labtab; l; l = l->nxt)
	{	if (strcmp(c->name, l->s->name) == 0
		&&  strcmp(a->name, l->c->name) == 0)	/* ? */
			break;
	}
	if (!l)
	{	printf("spin: label '%s' (proctype %s)\n", c->name, a->name);
		non_fatal("unknown label '%s'", c->name);
		if (context == a)
		printf("spin: cannot remote ref a label inside the same proctype\n");
		return;
	}
	if (!l->e || !l->e->n)
		fatal("fix_dest error (%s)", c->name);
	if (l->e->n->ntyp == GOTO)
	{	Element	*y = (Element *) emalloc(sizeof(Element));
		int	keep_ln = l->e->n->ln;
		Symbol	*keep_fn = l->e->n->fn;

		/* insert skip - or target is optimized away */
		y->n = l->e->n;		  /* copy of the goto   */
		y->seqno = find_maxel(a); /* unique seqno within proc */
		y->nxt = l->e->nxt;
		y->Seqno = Unique++; y->Nxt = Al_El; Al_El = y;

		/* turn the original element+seqno into a skip */
		l->e->n = nn(ZN, 'c', nn(ZN, CONST, ZN, ZN), ZN);
		l->e->n->ln = l->e->n->lft->ln = keep_ln;
		l->e->n->fn = l->e->n->lft->fn = keep_fn;
		l->e->n->lft->val = 1;
		l->e->nxt = y;		/* append the goto  */
	}
	l->e->status |= CHECK2;	/* treat as if global */
	if (l->e->status & (ATOM | L_ATOM | D_ATOM))
	{	non_fatal("cannot reference label inside atomic or d_step (%s)",
			c->name);
	}
}

int
find_lab(Symbol *s, Symbol *c, int markit)
{	Label *l;

	for (l = labtab; l; l = l->nxt)
	{	if (strcmp(s->name, l->s->name) == 0
		&&  strcmp(c->name, l->c->name) == 0)
		{	l->visible |= markit;
			return (l->e->seqno);
	}	}
	return 0;
}

void
pushbreak(void)
{	Lbreak *r = (Lbreak *) emalloc(sizeof(Lbreak));
	Symbol *l;
	char buf[64];

	sprintf(buf, ":b%d", break_id++);
	l = lookup(buf);
	r->l = l;
	r->nxt = breakstack;
	breakstack = r;
}

Symbol *
break_dest(void)
{
	if (!breakstack)
		fatal("misplaced break statement", (char *)0);
	return breakstack->l;
}

void
make_atomic(Sequence *s, int added)
{	Element *f;

	walk_atomic(s->frst, s->last, added);

	f = s->last;
	switch (f->n->ntyp) {	/* is last step basic stmnt or sequence ? */
	case NON_ATOMIC:
	case ATOMIC:
		/* redo and search for the last step of that sequence */
		make_atomic(f->n->sl->this, added);
		break;

	case UNLESS:
		/* escapes are folded into main sequence */
		make_atomic(f->sub->this, added);
		break;

	default:
		f->status &= ~ATOM;
		f->status |= L_ATOM;
		break;
	}
}

static void
walk_atomic(Element *a, Element *b, int added)
{	Element *f; Symbol *ofn; int oln;
	SeqList *h;

	ofn = Fname;
	oln = lineno;
	for (f = a; ; f = f->nxt)
	{	f->status |= (ATOM|added);
		switch (f->n->ntyp) {
		case ATOMIC:
			if (verbose&32)
			  printf("spin: warning, line %3d %s, atomic inside %s (ignored)\n",
			  f->n->ln, f->n->fn->name, (added)?"d_step":"atomic");
			goto mknonat;
		case D_STEP:
			if (!(verbose&32))
			{	if (added) goto mknonat;
				break;
			}
			printf("spin: warning, line %3d %s, d_step inside ",
			 f->n->ln, f->n->fn->name);
			if (added)
			{	printf("d_step (ignored)\n");
				goto mknonat;
			}
			printf("atomic\n");
			break;
		case NON_ATOMIC:
mknonat:		f->n->ntyp = NON_ATOMIC; /* can jump here */
			h = f->n->sl;
			walk_atomic(h->this->frst, h->this->last, added);
			break;
		case UNLESS:
			if (added)
			{ printf("spin: error, line %3d %s, unless in d_step (ignored)\n",
			 	 f->n->ln, f->n->fn->name);
			}
		}
		for (h = f->sub; h; h = h->nxt)
			walk_atomic(h->this->frst, h->this->last, added);
		if (f == b)
			break;
	}
	Fname = ofn;
	lineno = oln;
}

void
dumplabels(void)
{	Label *l;

	for (l = labtab; l; l = l->nxt)
		if (l->c != 0 && l->s->name[0] != ':')
		printf("label	%s	%d	<%s>\n",
		l->s->name, l->e->seqno, l->c->name);
}

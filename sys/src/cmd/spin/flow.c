/***** spin: flow.c *****/

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

Element *Al_El = ZE;
Label	*labtab = (Label *) 0;
Lbreak	*breakstack = (Lbreak *) 0;
Lextok	*innermost;
SeqList	*cur_s = (SeqList *) 0;
int	Elcnt=0, Unique=0, break_id=0;
int	DstepStart = -1;

extern Symbol	*Fname;
extern int	lineno, has_unless;

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

int
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

Sequence *
close_seq(int nottop)
{	Sequence *s = cur_s->this;
	Symbol *z;

	if (nottop > 0 && (z = has_lab(s->frst)))
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
		exit(1);
	}

	if (nottop == 4
	&& !Rjumpslocal(s->frst, s->last))
		fatal("non_local jump in d_step sequence", (char *) 0);

	cur_s = cur_s->nxt;
	s->maxel = Elcnt;
	s->extent = s->last;
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

Element *
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

Element *
if_seq(Lextok *n)
{	int	tok = n->ntyp;
	SeqList	*s  = n->sl;
	Element	*e  = new_el(ZN);
	Element	*t  = new_el(nn(ZN,'.',ZN,ZN)); /* target */
	SeqList	*z, *prev_z = (SeqList *) 0;
	SeqList *move_else  = (SeqList *) 0;	/* to end of optionlist */
	int	has_probes = 0;

	for (z = s; z; z = z->nxt)
	{	if (z->this->frst->n->ntyp == ELSE)
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
		{	has_probes |= has_typ(z->this->frst->n, FULL);
			has_probes |= has_typ(z->this->frst->n, NFULL);
			has_probes |= has_typ(z->this->frst->n, EMPTY);
			has_probes |= has_typ(z->this->frst->n, NEMPTY);
		}
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
	&&  has_probes
	&&  prev_z->this->frst->n->ntyp == ELSE)
		prev_z->this->frst->n->val = 1;

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
	return e;	/* destination node for label */
}

void
attach_escape(Sequence *n, Sequence *e)
{	Element *f; SeqList *z;

	for (f = n->frst; f; f = f->nxt)
	{	f->esc = seqlist(e, f->esc);	/* but, this is lifo order... */
#ifdef DEBUG
		printf("attach %d (", e->frst->Seqno);
		comment(stdout, e->frst->n, 0);
		printf(")	to %d (", f->Seqno);
		comment(stdout, f->n, 0);
		printf(")\n");
#endif
		if (f->n->ntyp == UNLESS)
		{	attach_escape(f->sub->this, e);
		} else
		if (f->n->ntyp == IF
		||  f->n->ntyp == DO)
		{	for (z = f->sub; z; z = z->nxt)
				attach_escape(z->this, e);
		} else
		if (f->n->ntyp == ATOMIC
		||  f->n->ntyp == D_STEP
		||  f->n->ntyp == NON_ATOMIC)
		{	attach_escape(f->n->sl->this, e);
		}
		if (f == n->extent) break;
	}
}

Element *
unless_seq(Lextok *n)
{	SeqList	*s  = n->sl;
	Element	*e  = new_el(ZN);
	Element	*t  = new_el(nn(ZN,'.',ZN,ZN)); /* target */
	SeqList	*z;

	e->n = nn(n, UNLESS, ZN, ZN);
	e->n->sl = s;			/* info only */
	e->sub = s;

	/*
	 * check that there are precisely two sequences:
	 * the normal execution and the escape.
	 */
	if (!s || !s->nxt || s->nxt->nxt)
		fatal("unexpected unless structure", (char *)0);
	/*
	 * append the target state to both
	 */
	for (z = s; z; z = z->nxt)
		add_el(t, z->this);
	/*
	 * distributed the escape sequence over all states in
	 * the normal execution sequence
	 */
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

void
add_el(Element *e, Sequence *s)
{
	if (e->n->ntyp == GOTO)
	{	Symbol *z;
		if ((z = has_lab(e))
		&& (strncmp(z->name, "progress", 8) == 0
		||  strncmp(z->name, "accept", 6) == 0
		||  strncmp(z->name, "end", 3) == 0))
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

Element *
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
has_lab(Element *e)
{	Label *l;

	for (l = labtab; l; l = l->nxt)
		if (e == l->e)
			return (l->s);
	return ZS;
}

void
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
fix_dest(Symbol *c, Symbol *a)	/* label, proctype */
{	Label *l;


	for (l = labtab; l; l = l->nxt)
	{	if (strcmp(c->name, l->s->name) == 0
		&&  strcmp(a->name, l->c->name) == 0)
			break;
	}

	if (!l)
	{	non_fatal("unknown label '%s'", c->name);
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
}

int
find_lab(Symbol *s, Symbol *c)
{	Label *l;

	for (l = labtab; l; l = l->nxt)
	{	if (strcmp(s->name, l->s->name) == 0
		&&  strcmp(c->name, l->c->name) == 0)
			return (l->e->seqno);
	}
	return 0;
}

void
pushbreak(void)
{	Lbreak *r = (Lbreak *) emalloc(sizeof(Lbreak));
	Symbol *l;
	char buf[32];

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
{
	walk_atomic(s->frst, s->last, added);
	s->last->status &= ~ATOM;
	s->last->status |= L_ATOM;
}

void
walk_atomic(Element *a, Element *b, int added)
{	Element *f;
	SeqList *h;
	char *str = (added)?"d_step":"atomic";

	for (f = a; ; f = f->nxt)
	{	f->status |= (ATOM|added);
		switch (f->n->ntyp) {
		case ATOMIC:
			lineno = f->n->ln;
			Fname = f->n->fn;
			non_fatal("atomic inside %s", str);
			break;
		case D_STEP:
			lineno = f->n->ln;
			Fname = f->n->fn;
			non_fatal("d_step inside %s", str);
			break;
		case NON_ATOMIC:
			h = f->n->sl;
			walk_atomic(h->this->frst, h->this->last, added);
			break;
		}

		for (h = f->sub; h; h = h->nxt)
			walk_atomic(h->this->frst, h->this->last, added);
		if (f == b)
			break;
	}
}

void
dumplabels(void)
{	Label *l;

	for (l = labtab; l; l = l->nxt)
		if (l->c != 0 && l->s->name[0] != ':')
		printf("label	%s	%d	<%s>\n",
		l->s->name, l->e->seqno, l->c->name);
}

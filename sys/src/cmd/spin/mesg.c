/***** spin: mesg.c *****/

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

#define MAXQ	2500		/* default max # queues  */

extern RunList	*X;
extern Symbol	*Fname;
extern int	verbose, TstOnly, s_trail;
extern int	lineno, depth, xspin, m_loss;

Queue	*qtab = (Queue *) 0;	/* linked list of queues */
Queue	*ltab[MAXQ];		/* linear list of queues */
int	nqs=0;

int
cnt_mpars(Lextok *n)
{	Lextok *m;
	int i=0;

	for (m = n; m; m = m->rgt)
		i += Cnt_flds(m);
	return i;
}

int
qmake(Symbol *s)
{	Lextok *m;
	Queue *q;
	int i; extern int analyze;

	if (!s->ini)
		return 0;

	if (nqs >= MAXQ)
	{	lineno = s->ini->ln;
		Fname  = s->ini->fn;
		fatal("too many queues (%s)", s->name);
	}

	if (s->ini->ntyp != CHAN)
		return eval(s->ini);

	q = (Queue *) emalloc(sizeof(Queue));
	q->qid    = ++nqs;
	q->nslots = s->ini->val;
	q->nflds  = cnt_mpars(s->ini->rgt);
	q->setat  = depth;

	i = max(1, q->nslots);	/* 0-slot qs get 1 slot minimum */

	q->contents  = (int *) emalloc(q->nflds*i*sizeof(int));
	q->fld_width = (int *) emalloc(q->nflds*sizeof(int));

	for (m = s->ini->rgt, i = 0; m; m = m->rgt)
	{	if (m->sym && m->ntyp == STRUCT)
			i = Width_set(q->fld_width, i, getuname(m->sym));
		else
			q->fld_width[i++] = m->ntyp;
	}
	q->nxt = qtab;
	qtab = q;
	ltab[q->qid-1] = q;

	return q->qid;
}

int
qfull(Lextok *n)
{	int whichq = eval(n->lft)-1;

	if (whichq < MAXQ && whichq >= 0 && ltab[whichq])
		return (ltab[whichq]->qlen >= ltab[whichq]->nslots);
	return 0;
}

int
qlen(Lextok *n)
{	int whichq = eval(n->lft)-1;

	if (whichq < MAXQ && whichq >= 0 && ltab[whichq])
		return ltab[whichq]->qlen;
	return 0;
}

int
q_is_sync(Lextok *n)
{	int whichq = eval(n->lft)-1;

	if (whichq < MAXQ && whichq >= 0 && ltab[whichq])
		return (ltab[whichq]->nslots == 0);
	return 0;
}

int
qsend(Lextok *n)
{	int whichq = eval(n->lft)-1;

	if (whichq == -1)
	{	printf("Error: sending to an uninitialized chan\n");
		whichq = 0;
		return 0;
	}
	if (whichq < MAXQ && whichq >= 0 && ltab[whichq])
	{	ltab[whichq]->setat = depth;
		if (ltab[whichq]->nslots > 0)
			return a_snd(ltab[whichq], n);
		else
			return s_snd(ltab[whichq], n);
	}
	return 0;
}

int
qrecv(Lextok *n, int full)
{	int whichq = eval(n->lft)-1;

	if (whichq == -1)
	{	printf("Error: receiving from an uninitialized chan\n");
		whichq = 0;
		return 0;
	}
	if (whichq < MAXQ && whichq >= 0 && ltab[whichq])
	{	ltab[whichq]->setat = depth;
		return a_rcv(ltab[whichq], n, full);
	}
	return 0;
}

int
sa_snd(Queue *q, Lextok *n)	/* sorted asynchronous */
{	Lextok *m;
	int i, j, k;
	int New, Old;

	for (i = 0; i < q->qlen; i++)
	for (j = 0, m = n->rgt; m && j < q->nflds; m = m->rgt, j++)
	{	New = cast_val(q->fld_width[j], eval(m->lft));
		Old = q->contents[i*q->nflds+j];
		if (New == Old) continue;
		if (New >  Old) break;			/* inner loop */
		if (New <  Old) goto found;
	}
found:
	for (j = q->qlen-1; j >= i; j--)
	for (k = 0; k < q->nflds; k++)
	{	q->contents[(j+1)*q->nflds+k] =
			q->contents[j*q->nflds+k];	/* shift up */
	}
	return i*q->nflds;				/* new q offset */
}

void
typ_ck(int ft, int at, char *s)
{
	if (verbose&32 && ft != at
	&& (ft == CHAN || at == CHAN))
	{	char buf[128], tag1[32], tag2[32];
		(void) sputtype(tag1, ft);
		(void) sputtype(tag2, at);
		sprintf(buf, "type-clash in %s, (%s<-> %s)", s, tag1, tag2);
		non_fatal("%s", buf);
	}
}

int
a_snd(Queue *q, Lextok *n)
{	Lextok *m;
	int i = q->qlen*q->nflds;	/* q offset */
	int j = 0;			/* q field# */

	if (q->nslots > 0 && q->qlen >= q->nslots)
		return m_loss;	/* q is full */

	if (TstOnly) return 1;

	if (n->val) i = sa_snd(q, n);	/* sorted insert */

	for (m = n->rgt; m && j < q->nflds; m = m->rgt, j++)
	{	int New = eval(m->lft);
		q->contents[i+j] = cast_val(q->fld_width[j], New);
		if (verbose&16)
			sr_talk(n, New, "Send ", "->", j, q);
		typ_ck(q->fld_width[j], Sym_typ(m->lft), "send");
	}
	if (verbose&16)
	{	for (i = j; i < q->nflds; i++)
			sr_talk(n, 0, "Send ", "->", i, q);
		if (j < q->nflds)
			printf("\twarning: missing params in send\n");
		if (m)
			printf("\twarning: too many params in send\n");
	}
	q->qlen++;
	return 1;
}

int
a_rcv(Queue *q, Lextok *n, int full)
{	Lextok *m;
	int i=0, j, k;
	extern int Rvous;

	if (q->qlen == 0) return 0;	/* q is empty */
try_slot:
	/* test executability */
	for (m = n->rgt, j=0; m && j < q->nflds; m = m->rgt, j++)
		if (m->lft->ntyp == CONST
		&&  q->contents[i*q->nflds+j] != m->lft->val)
		{	if (n->val == 0		/* fifo recv */
			|| ++i >= q->qlen)	/* last slot */
				return 0;	/* no match  */
			goto try_slot;
		}

	if (TstOnly) return 1;

	if (verbose&8)
	{	if (j < q->nflds)
			printf("\twarning: missing params in next recv\n");
		else if (m)
			printf("\twarning: too many params in next recv\n");
	}

	/* set the fields */
	for (m = n->rgt, j = 0; j < q->nflds; m = (m)?m->rgt:m, j++)
	{	if (verbose&8 && !Rvous)
		sr_talk(n, q->contents[i*q->nflds+j],
				(full)?"Recv ":"[Recv] ", "<-", j, q);

		if (m && m->lft->ntyp != CONST)
		{	(void) setval(m->lft, q->contents[i*q->nflds+j]);
			typ_ck(q->fld_width[j], Sym_typ(m->lft), "recv");
		}
		for (k = i; full && k < q->qlen-1; k++)
			q->contents[k*q->nflds+j] =
			  q->contents[(k+1)*q->nflds+j];
	}
	if (full) q->qlen--;
	return 1;
}

int
s_snd(Queue *q, Lextok *n)
{	Lextok *m;
	RunList *rX, *sX = X;	/* rX=recvr, sX=sendr */
	int i, j = 0;	/* q field# */

	for (m = n->rgt; m && j < q->nflds; m = m->rgt, j++)
	{	q->contents[j] = cast_val(q->fld_width[j], eval(m->lft));
		typ_ck(q->fld_width[j], Sym_typ(m->lft), "rv-send");
	}
	q->qlen = 1;
	if (!complete_rendez())
	{	q->qlen = 0;
		return 0;
	}
	if (TstOnly) return 1;
	if (verbose&16)
	{	m = n->rgt;
		rX = X; X = sX;
		for (j = 0; m && j < q->nflds; m = m->rgt, j++)
			sr_talk(n, eval(m->lft), "Sent ", "->", j, q);
		for (i = j; i < q->nflds; i++)
			sr_talk(n, 0, "Sent ", "->", i, q);
		if (j < q->nflds)
			  printf("\twarning: missing params in rv-send\n");
		else if (m)
			  printf("\twarning: too many params in rv-send\n");
		X = rX;
		m = n->rgt;
		if (!s_trail)
		for (j = 0; m && j < q->nflds; m = m->rgt, j++)
			sr_talk(n, eval(m->lft), "Recv ", "<-", j, q);
	}
	return 1;
}

void
sr_talk(Lextok *n, int v, char *tr, char *a, int j, Queue *q)
{	char s[64];
	if (xspin)
	{	if (verbose&4)
		sprintf(s, "(state -)\t[values: %d",
			eval(n->lft));
		else
		sprintf(s, "(state -)\t[%d", eval(n->lft));
		if (strncmp(tr, "Sen", 3) == 0)
			strcat(s, "!");
		else
			strcat(s, "?");
	} else
	{	strcpy(s, tr);
	}
	if (j == 0)
	{	whoruns(1);
		printf("line %3d %s %s",
			n->ln, n->fn->name, s);
	} else
		printf(",");
	sr_mesg(v, q->fld_width[j] == MTYPE);

	if (j == q->nflds - 1)
	{	if (xspin)
		{	printf("]\n");
			if (!(verbose&4)) printf("\n");
			return;
		}
		printf("\t%s queue %d (", a, eval(n->lft));
		if (n->sym->type == CHAN)
			printf("%s", n->sym->name);
		else if (n->sym->type == NAME)
			printf("%s", lookup(n->sym->name)->name);
		else if (n->sym->type == STRUCT)
		{	Symbol *r = n->sym;
			if (r->context)
				r = findloc(r);
			ini_struct(r);
			printf("%s", r->name);
			struct_name(n->lft, r, 1);
		} else
			printf("-");
		if (n->lft->lft)
			printf("[%d]", eval(n->lft->lft));
		printf(")\n");
	}
	fflush(stdout);
}

void
sr_mesg(int v, int j)
{	extern Lextok *Mtype;
	int cnt = 1;
	Lextok *n;

	for (n = Mtype; n && j; n = n->rgt, cnt++)
		if (cnt == v)
		{	printf("%s", n->lft->sym->name);
			return;
		}
	printf("%d", v);
}

void
doq(Symbol *s, int n, RunList *r)
{	Queue *q;
	int j, k;

	if (!s->val)	/* uninitialized queue */
		return;
	for (q = qtab; q; q = q->nxt)
	if (q->qid == s->val[n])
	{	if (xspin > 0
		&& (verbose&4)
		&& q->setat < depth)
			continue;
		if (q->nslots == 0)
			continue; /* rv q always empty */
		printf("\t\tqueue %d (", q->qid);
		if (r)
		printf("%s(%d):", r->n->name, r->pid);
		if (s->nel != 1)
		  printf("%s[%d]): ", s->name, n);
		else
		  printf("%s): ", s->name);
		for (k = 0; k < q->qlen; k++)
		{	printf("[");
			for (j = 0; j < q->nflds; j++)
			{	if (j > 0) printf(",");
				sr_mesg(q->contents[k*q->nflds+j],
					q->fld_width[j] == MTYPE);
			}
			printf("]");
		}
		printf("\n");
		break;
	}
}

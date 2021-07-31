/***** spin: dstep.c *****/

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

char	*NextLab[64];
int	Level=0, GenCode=0, IsGuard=0, TestOnly=0;
int	Tj=0, Jt=0, LastGoto=0;
int	Tojump[512], Jumpto[512], Special[512];

void
Sourced(int n, int special)
{	int i;
	for (i = 0; i < Tj; i++)
		if (Tojump[i] == n)
			return;
	Special[Tj] = special;
	Tojump[Tj++] = n;
}

void
Dested(int n)
{	int i;

	LastGoto = 1;
	for (i = 0; i < Tj; i++)
		if (Tojump[i] == n)
			return;
	for (i = 0; i < Jt; i++)
		if (Jumpto[i] == n)
			return;
	Jumpto[Jt++] = n;
}

void
Mopup(FILE *fd)
{	int i, j;
	for (i = 0; i < Jt; i++)
	{	for (j = 0; j < Tj; j++)
		{	if (Tojump[j] == Jumpto[i])
				break;
		}
		if (j == Tj)
		{	char buf[12];
			sprintf(buf, "S_%.3d_0", Jumpto[i]);
			non_fatal("goto %s breaks from d_step seq", buf);
	}	}
	for (j = 0; j < Tj; j++)
	{	for (i = 0; i < Jt; i++)
		{	if (Tojump[j] == Jumpto[i])
				break;
		}
#ifdef DEBUG
		if (i == Jt && !Special[i])
		{	fprintf(fd, "\t\t/* >>no goto's for S_%.3d_0<< */\n",
			Tojump[j]);
		}
#endif
	}
	for (j = i = 0; j < Tj; j++)
	{	if (Special[j])
		{	Tojump[i] = Tojump[j];
			Special[i] = 2;
			i++;
	}	}
	Tj = i;	/* keep only the global exit-labels */
	Jt = 0;
}

int
FirstTime(int n)
{	int i;
	for (i = 0; i < Tj; i++)
		if (Tojump[i] == n)
			return (Special[i] <= 1);
	return 1;
}

void
illegal(Element *e, char *str)
{
	printf("illegal operator in 'step:' '");
	comment(stdout, e->n, 0);
	printf("'\n");
	fatal("saw %s", str);
}

void
filterbad(Element *e)
{
	switch (e->n->ntyp) {
	case ASSERT:
	case PRINT:
	case 'c':
		/* run cannot be completely undone
		 * with sv_save-sv_restor
		 */
		if (any_oper(e->n->lft, RUN))
			illegal(e, "run operator");
		break;
	case '@':
		illegal(e, "process termination");
		break;
	case D_STEP:
		illegal(e, "nested d_step sequence");
		break;
	case ATOMIC:
		illegal(e, "nested atomic sequence");
		break;
	default:
		break;
	}
}

int
CollectGuards(FILE *fd, Element *e, int inh)
{	SeqList *h; Element *ee;

	for (h = e->sub; h; h = h->nxt)
	{	ee = huntstart(h->this->frst);
		filterbad(ee);
		switch (ee->n->ntyp) {
		case  IF:
			inh += CollectGuards(fd, ee, inh);
			break;
		case '.':
			if (ee->nxt->n->ntyp == DO)
				inh += CollectGuards(fd, ee->nxt, inh);
			break;
		case ELSE:
			if (inh++ > 0) fprintf(fd, " || ");
			fprintf(fd, "(1 /* else */)");
			break;
		case 'c':
			if (inh++ > 0) fprintf(fd, " || ");
			fprintf(fd, "("); TestOnly=1;
			putstmnt(fd, ee->n->lft, e->seqno);
			fprintf(fd, ")"); TestOnly=0;
			break;
		}
	}
	return inh;
}

int
putcode(FILE *fd, Sequence *s, Element *nxt, int justguards)
{	int isg=0;

	NextLab[0] = "continue";

	filterbad(s->frst);

	switch (s->frst->n->ntyp) {
	case UNLESS:
		non_fatal("'unless' inside d_step - ignored", (char *) 0);
		return putcode(fd, s->frst->n->sl->this, nxt, 0);
	case NON_ATOMIC:
		(void) putcode(fd, s->frst->n->sl->this, ZE, 1);
		break;
	case IF:
		fprintf(fd, "if (!(");
		if (!CollectGuards(fd, s->frst, 0))
			fprintf(fd, "1");
		fprintf(fd, "))\n\t\t\tcontinue;");
		isg = 1;
		break;
	case '.':
		if (s->frst->nxt->n->ntyp == DO)
		{	fprintf(fd, "if (!(");
			if (!CollectGuards(fd, s->frst->nxt, 0))
				fprintf(fd, "1");
			fprintf(fd, "))\n\t\t\tcontinue;");
			isg = 1;
		}
		break;
	case 'R':
	case 'r':
	case 's':
		fprintf(fd, "if (!("); TestOnly=1;
		putstmnt(fd, s->frst->n, s->frst->seqno);
		fprintf(fd, "))\n\t\t\tcontinue;"); TestOnly=0;
		break;
	case 'c':
		fprintf(fd, "if (!("); TestOnly=1;
		putstmnt(fd, s->frst->n->lft, s->frst->seqno);
		fprintf(fd, "))\n\t\t\tcontinue;"); TestOnly=0;
		break;
	}
	if (justguards) return 0;

	fprintf(fd, "\n#if defined(FULLSTACK) && defined(NOCOMP)\n");
	fprintf(fd, "\t\tif (t->atom&2)\n");
	fprintf(fd, "#endif\n");
	fprintf(fd, "\t\tsv_save((char *)&now);\n");
	putCode(fd, s->frst, s->extent, nxt, isg);

	if (nxt)
	{	if (FirstTime(nxt->Seqno)
		&& (!(nxt->status & DONE2) || !(nxt->status & D_ATOM)))
		{	fprintf(fd, "S_%.3d_0: /* 1 */\n", nxt->Seqno);
			nxt->status |= DONE2;
			LastGoto = 0;
		}
		Sourced(nxt->Seqno, 1);
		Mopup(fd);
	}
	unskip(s->frst->seqno);
	return LastGoto;
}

void
putCode(FILE *fd, Element *f, Element *last, Element *next, int isguard)
{	Element *e, *N;
	SeqList *h; int i;
	char NextOpt[32];

	for (e = f; e; e = e->nxt)
	{	if (e->status & DONE2)
			continue;
		e->status |= DONE2;

		if (!(e->status & D_ATOM))
		{	if (!LastGoto)
			{	fprintf(fd, "\t\tgoto S_%.3d_0;\n", e->Seqno);
				Dested(e->Seqno);
			}
			break;
		}
		fprintf(fd, "S_%.3d_0: /* 2 */\n", e->Seqno);
		Sourced(e->Seqno, 0);

		if (!e->sub)
		{	filterbad(e);
			switch (e->n->ntyp) {
			case NON_ATOMIC:
				h = e->n->sl;
				putCode(fd, h->this->frst, h->this->extent, e->nxt, 0);
				break;
			case BREAK:
				if (LastGoto) break;
				i = target(huntele(e->nxt, e->status))->Seqno;
				fprintf(fd, "\t\tgoto S_%.3d_0;	/* 'break' */\n", i);
				Dested(i);
				break;
			case GOTO:
				if (LastGoto) break;
				i = huntele(get_lab(e->n,1), e->status)->Seqno;
				fprintf(fd, "\t\tgoto S_%.3d_0;	/* 'goto' */\n", i);
				Dested(i);
				break;
			case '.':
				if (LastGoto) break;
				if (e->nxt->status & DONE2)
				{	i = e->nxt?e->nxt->Seqno:0;
					fprintf(fd, "\t\tgoto S_%.3d_0;	/* '.' */\n", i);
					Dested(i);
				}
				break;
			default:
				putskip(e->seqno);
				GenCode = 1; IsGuard = isguard;
				fprintf(fd, "\t\t");
				putstmnt(fd, e->n, e->seqno);
				fprintf(fd, ";\n");
				GenCode = IsGuard = isguard = LastGoto = 0;
				if (e->n->ntyp == ELSE)
					LastGoto = 1;
				break;
			}
			i = e->nxt?e->nxt->Seqno:0;
			if (e->nxt && e->nxt->status & DONE2 && !LastGoto)
			{	fprintf(fd, "\t\tgoto S_%.3d_0; /* ';' */\n", i);
				Dested(i);
				break;
			}
		} else
		{	for (h = e->sub, i=1; h; h = h->nxt, i++)
			{	sprintf(NextOpt, "goto S_%.3d_%d", e->Seqno, i);
				NextLab[++Level] = NextOpt;
				N = (e->n->ntyp == DO) ? e : e->nxt;
				putCode(fd, h->this->frst, h->this->extent, N, 1);
				Level--;
				fprintf(fd, "%s: /* 3 */\n", &NextOpt[5]);
			}
			if (!LastGoto)
			{	fprintf(fd, "\t\tUerror(\"blocking sel in d_step\");\n");
				LastGoto = 0;
			}
		}
		if (e == last)
		{	if (!LastGoto && next)
			{	fprintf(fd, "\t\tgoto S_%.3d_0;\n", next->Seqno);
				Dested(next->Seqno);
			}
			break;
	}	}
}

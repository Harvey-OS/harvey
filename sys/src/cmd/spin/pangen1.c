/***** spin: pangen1.c *****/

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
#include "pangen1.h"
#include "pangen3.h"

extern FILE	*tc, *th;
extern Lextok	*Mtype;
extern ProcList	*rdy;
extern Queue	*qtab;
extern Symbol	*symtab[Nhash+1], *Fname;
extern char	*claimproc;
extern int	lineno, has_sorted, has_random;
extern int	nrRdy, nqs, mst, Mpars, claimnr;

enum	{ INIV, PUTV };
int	Npars=0, u_sync=0, u_async=0;
int	acceptors=0, progressors=0, nBits=0;
Symbol	*LstSet=ZS;

void
reverse_names(ProcList *p)
{
	if (!p) return;
	reverse_names(p->nxt);
	fprintf(tc, "   \"%s\",\n", p->n->name);
}

void
genheader(void)
{	ProcList *p; int i;

	fprintf(th, "#define SYNC	%d\n", u_sync);
	fprintf(th, "#define ASYNC	%d\n\n", u_async);

	putunames(th);

	fprintf(tc, "short Air[] = { ");
	for (p = rdy, i=0; p; p = p->nxt, i++)
		fprintf(tc, "%s Air%d", (p!=rdy)?",":"", i);
	fprintf(tc, " };\n");

	fprintf(tc, "char *procname[] = {\n");
		reverse_names(rdy);
	fprintf(tc, "};\n\n");
	for (p = rdy; p; p = p->nxt)
		put_ptype(p->n->name, p->tn, mst, nrRdy);

	ntimes(th, 0, 1, Header);
	LstSet = ZS;
	doglobal(PUTV);
	fprintf(th, "	uchar sv[VECTORSZ];\n");
	fprintf(th, "} State;\n\n");
	dohidden();
}

void
genaddproc(void)
{	ProcList *p;
	int i;

	fprintf(tc, "int\naddproc(int n");
	for (i = 0; i < Npars; i++)
		fprintf(tc, ", int par%d", i);

	ntimes(tc, 0, 1, Addp0);
	ntimes(tc, 1, nrRdy, R5);
	ntimes(tc, 0, 1, Addp1);

	for (p = rdy; p; p = p->nxt)
		put_pinit(p->s->frst, p->n, p->p, p->tn);

	ntimes(tc, i, i+1, R6);
}

void
genother(void)
{	ProcList *p;

	ntimes(tc,     0,     1, Code0);
	ntimes(tc,     0, nrRdy,   R0);
	ntimes(tc, nrRdy, nrRdy+1, R1);

	for (p = rdy; p; p = p->nxt)
		end_labs(p->n, p->tn);

	ntimes(tc, 0, nrRdy, R0a);
	ntimes(tc, 0,     1, R0b);

	ntimes(th, acceptors,   acceptors+1,   Code1);
	ntimes(th, progressors, progressors+1, Code3);
	ntimes(th, nrRdy+1, nrRdy+2, R2);

	doglobal(INIV);
	ntimes(tc, 1, nqs+1, R3);
	ntimes(tc, 0,     1, Code2);
	ntimes(tc, 0,     nrRdy, R4);
	fprintf(tc, "}\n\n");
}

static struct {
	char *s,	*t;		int n,	m,	p;
} ln[] = {
	"end",  	"stopstate",	3,	0,	0,
	"progress",	"progstate",	8,	0,	1,
	"accept",	"accpstate",	6,	1,	0,
	0,		0,		0,	0,	0,
};

void
end_labs(Symbol *s, int i)
{	extern Label *labtab;
	Label *l;
	int j;

	for (l = labtab; l; l = l->nxt)
	for (j = 0; ln[j].n; j++)
		if (strncmp(l->s->name, ln[j].s, ln[j].n) == 0
		&&  strcmp(l->s->context->name, s->name) == 0)
		{	fprintf(tc, "\t%s[%d][%d] = 1;\n",
				ln[j].t, i, l->e->seqno);
			acceptors += ln[j].m;
			progressors += ln[j].p;
		}
}

void
ntimes(FILE *fd, int n, int m, char *c[])
{
	int i, j;
	for (j = 0; c[j]; j++)
	for (i = n; i < m; i++)
	{	fprintf(fd, c[j], i, i, i, i, i, i);
		fprintf(fd, "\n");
	}
}

int Types[] = { BIT, BYTE, CHAN, SHORT, INT, STRUCT };

int
dolocal(int dowhat, int p, char *s)
{	int i, j, k=0; extern int nr_errs;
	Symbol *sp;
	char buf[64];

	for (j = 0; j < 6; j++)
	for (i = 0; i <= Nhash; i++)
	for (sp = symtab[i]; sp; sp = sp->next)
		if (sp->context
		&& !sp->owner
		&&  sp->type == Types[j]
		&&  strcmp(s, sp->context->name) == 0)
		{	sprintf(buf, "((P%d *)pptr(h))->", p);
			do_var(dowhat, buf, sp); k++;
			if (strcmp(s, ":never:") == 0)
			{	printf("error: %s defines local %s\n", s, sp->name);
				nr_errs++;
		}	}
	return k;
}

void
doglobal(int dowhat)
{	Symbol *sp;
	int i, j;

	for (j = 0; j < 6; j++)
	for (i = 0; i <= Nhash; i++)
	for (sp = symtab[i]; sp; sp = sp->next)
		if (!sp->context
		&&  !sp->owner
		&&  !sp->hidden
		&&  sp->type == Types[j])
			do_var(dowhat, "now.", sp);
}

void
dohidden(void)
{	Symbol *sp;
	int i, j;

	for (j = 0; j < 6; j++)
	for (i = 0; i <= Nhash; i++)
	for (sp = symtab[i]; sp; sp = sp->next)
		if (sp->hidden
		&&  sp->type == Types[j])
		{	if (sp->context || sp->owner)
			fatal("cannot hide non-globals (%s)", sp->name);
			if (sp->type == CHAN)
			fatal("cannot hide channels (%s)", sp->name);
			fprintf(th, "/* hidden variable: */");
			typ2c(sp);
		}
}

void
do_var(int dowhat, char *s, Symbol *sp)
{	int i;

	switch(dowhat) {
	case PUTV:
		typ2c(sp);
		break;
	case INIV:
		if (sp->type == STRUCT)
		{	/* struct may contain a chan */
			char gather[128];
			char pregat[128]; Symbol *z = (Symbol *)0;
			int nc, ov=0, last;
			sprintf(pregat, "%s%s", s, sp->name);
			do {
				strcpy(gather, pregat);
				nc = fill_struct(sp, 0, ov, gather, &z);
				if (z && last != z->Nid)
				{	last = z->Nid;
					if (nc < 0)
					do_var(INIV, gather, z);
				}
				ov = -nc;
			} while (nc < 0);
			break;
		}
		if (!sp->ini)
			break;
		if (sp->nel == 1)
		{	fprintf(tc, "\t\t%s%s = ", s, sp->name);
			do_init(sp);
		} else
		for (i = 0; i < sp->nel; i++)
		{	fprintf(tc, "\t\t%s%s[%d] = ", s, sp->name, i);
			do_init(sp);
		}
		break;
	}
}

void
do_init(Symbol *sp)
{	int i; extern Queue *ltab[];

	if (sp->type == CHAN && ((i = qmake(sp)) > 0))
	{	if (sp->ini->ntyp == CHAN)
			fprintf(tc, "addqueue(%d, %d);\n",
			i, ltab[i-1]->nslots == 0);
		else
			fprintf(tc, "%d;\n", i);
	} else
	{	putstmnt(tc, sp->ini, 0);
		fprintf(tc, ";\n");
	}
}

int
blog(int n)	/* for small log2 without rounding problems */
{	int m=1, r=2;

	while (r < n) { m++; r *= 2; }
	return 1+m;
}

void
put_ptype(char *s, int i, int m0, int m1)
{	int k;

	fprintf(th, "typedef struct P%d { /* %s */\n", i, s);
	fprintf(th, "	unsigned _pid : 8;  /* 0..255 */\n");
	fprintf(th, "	unsigned _t   : %d; /* proctype */\n", blog(m1));
	fprintf(th, "	unsigned _p   : %d; /* state    */\n", blog(m0));
	LstSet = ZS;
	nBits = 8 + blog(m1) + blog(m0);
	k = dolocal(PUTV, i, s);	/* includes pars */
	fprintf(th, "} P%d;\n", i);
	if (!LstSet && k > 0)
		fprintf(th, "#define Air%d	0\n", i);
	else
	{	fprintf(th, "#define Air%d	(sizeof(P%d) - ", i, i);
		if (k == 0)
		{	fprintf(th, "%d", (nBits+7)/8);
			goto done;
		}
		if (LstSet->type != BIT || LstSet->nel != 1)
		{	fprintf(th, "Offsetof(P%d, %s%s) - %d*sizeof(",
				i,
				LstSet->name,
				(LstSet->nel > 1)?"[0]":"",
				LstSet->nel);
		}
		switch(LstSet->type) {
		case BIT:
			if (LstSet->type == 1)
			{	fprintf(th, "%d", (nBits+7)/8);
				break;
			}	/* else fall through */
		case BYTE: case CHAN:
			fprintf(th, "uchar)"); break;
		case SHORT:
			fprintf(th, "short)"); break;
		case INT:
			fprintf(th, "int)"); break;
		default:
			fatal("cannot happen Air %s",
				LstSet->name);
		}
done:		fprintf(th, ")\n");
	}
}

void
put_pinit(Element *e, Symbol *s, Lextok *p, int i)
{	Lextok *fp, *fpt, *t;
	int ini, j;

	ini = huntele(e, e->status)->seqno;
	fprintf(th, "#define start%d	%d\n", i, ini);
	if (i == claimnr)
	fprintf(th, "#define start_claim	%d\n", ini);

	fprintf(tc, "\tcase %d:	/* %s */\n", i, s->name);
	fprintf(tc, "\t\t((P%d *)pptr(h))->_t = %d;\n", i, i);
	fprintf(tc, "\t\t((P%d *)pptr(h))->_p = %d;", i, ini);
	fprintf(tc, " reached%d[%d]=1;\n", i, ini);

	(void) dolocal(INIV, i, s->name);

	for (fp  = p, j=0; fp; fp = fp->rgt)
	for (fpt = fp->lft; fpt; fpt = fpt->rgt, j++)
	{	t = (fpt->ntyp == ',') ? fpt->lft : fpt;
		if (t->sym->nel != 1)
		{	lineno = t->ln;
			Fname  = t->fn;
			fatal("array in parameter list, %s",
				t->sym->name);
		}
		fprintf(tc, "\t\t((P%d *)pptr(h))->", i);
		if (t->sym->type == STRUCT)
			full_name(tc, t, t->sym, 1);
		else
			fprintf(tc, "%s", t->sym->name);
		fprintf(tc, " = par%d;\n", j);
	}
	dumpclaims(tc, i, s->name);
	fprintf(tc, "\t	break;\n");
}

Element *
huntstart(Element *f)
{	Element *e = f;

	if (e->n)
	{	if (e->n->ntyp == '.' && e->nxt)
			e = e->nxt;
		else if (e->n->ntyp == ATOMIC
		     ||  e->n->ntyp == D_STEP
		     ||  e->n->ntyp == NON_ATOMIC)
			e->n->sl->this->last->nxt = e->nxt;
		else if (e->n->ntyp == UNLESS)
			return e->sub->this->frst;
	}
	return e;
}

Element *
huntele(Element *f, int o)
{	Element *g, *e = f;
	int cnt; /* a precaution against loops */

	if (e)
	for (cnt=0; cnt < 10 && e->n; cnt++)
	{	switch (e->n->ntyp) {
		case GOTO:
			g = get_lab(e->n,1);
			cross_dsteps(e->n, g->n);
			break;
		case '.':
		case BREAK:
			if (!e->nxt)
				return e;
			g = e->nxt;
			break;
		case UNLESS:
			g = huntele(e->sub->this->frst, o);
			break;
		case D_STEP:
		case ATOMIC:
		case NON_ATOMIC:
			e->n->sl->this->last->nxt = e->nxt;
		default:	/* fall through */
			return e;
		}
		if ((o & ATOM) && !(g->status & ATOM))
			return e;
		e = g;
	}
	return e;
}

void
typ2c(Symbol *sp)
{
	switch (sp->type) {
	case BIT:
		if (sp->nel == 1)
		{	fprintf(th, "\tunsigned %s : 1", sp->name);
			LstSet = sp; 
			nBits++;
			break;
		} /* else fall through */
		nBits += 8*sp->nel;
	case BYTE:
	case CHAN:	/* good for up to 255 channels */
		fprintf(th, "\tuchar %s", sp->name);
		LstSet = sp;
		break;
	case SHORT:
		fprintf(th, "\tshort %s", sp->name);
		LstSet = sp;
		break;
	case INT:
		fprintf(th, "\tint %s", sp->name);
		LstSet = sp;
		break;
	case STRUCT:
		fprintf(th, "\tstruct %s %s",
			sp->Snm->name,
			sp->name);
		LstSet = ZS;
		break;
	case PREDEF:
		return;
	default:
		fatal("variable %s undeclared", sp->name);
	}

	if (sp->nel != 1)
		fprintf(th, "[%d]", sp->nel);
	fprintf(th, ";\n");
}

void
ncases(FILE *fd, int p, int n, int m, char *c[])
{	int i, j;

	for (j = 0; c[j]; j++)
	for (i = n; i < m; i++)
	{	fprintf(fd, c[j], i, p, i);
		fprintf(fd, "\n");
	}
}

void
genaddqueue(void)
{	char *buf0;
	int j;
	Queue *q;

	buf0 = (char *) emalloc(32);
	ntimes(tc, 0, 1, Addq0);
	fprintf(th, "short q_flds[%d];\n", nqs+1);
	for (q = qtab; q; q = q->nxt)
	{	j = q->qid;
		fprintf(tc, "\tcase %d: j = sizeof(Q%d);", j, j);
		fprintf(tc, " q_flds[%d] = %d;", j, q->nflds);
		fprintf(tc, " break;\n");

		ntimes(th, j, j+1, R9);
		for (j = 0; j < q->nflds; j++)
		{	switch (q->fld_width[j]) {
			case BIT:
				if (q->nflds != 1)
				{	fprintf(th, "\t\tunsigned");
					fprintf(th, " fld%d : 1;\n", j);
					break;
				} /* else fall through: gives smaller struct */
			case MTYPE:
			case CHAN:
			case BYTE:
				fprintf(th, "\t\tuchar fld%d;\n", j);
				break;
			case SHORT:
				fprintf(th, "\t\tshort fld%d;\n", j);
				break;
			case INT:
				fprintf(th, "\t\tint fld%d;\n", j);
				break;
			default:
				fatal("bad channel spec", "");
			}
		}
		fprintf(th, "	} contents[%d];\n", max(1, q->nslots));
		fprintf(th, "} Q%d;\n", q->qid);
	}
	ntimes(th, 0, 1, R10);
	ntimes(tc, 0, 1, Addq1);

	if (has_random)
	{	fprintf(tc, "int\nQ_has(int into");
		for (j = 0; j < Mpars; j++)
			fprintf(tc, ", int want%d, int fld%d", j, j);
		fprintf(tc, ")\n");
		fprintf(tc, "{	int i;\n\n");
		fprintf(tc, "	if (!into--)\n");
		fprintf(tc, "	uerror(\"ref to unknown chan (recv-poll)\");\n\n");
		fprintf(tc, "	if (into >= now._nr_qs || into < 0)\n");
		fprintf(tc, "		Uerror(\"qrecv bad queue#\");\n\n");
		fprintf(tc, "	for (i = 0; i < ((Q0 *)qptr(into))->Qlen; i++)\n");
		fprintf(tc, "	{\n");
		for (j = 0; j < Mpars; j++)
		{	fprintf(tc, "		if (want%d && ", j);
			fprintf(tc, "qrecv(into+1, i, %d, 0) != fld%d)\n", j, j);
			fprintf(tc, "			continue;\n");
		}
		fprintf(tc, "		return i+1;\n");
		fprintf(tc, "	}\n");
		fprintf(tc, "	return 0;\n");
		fprintf(tc, "}\n");
	}

	fprintf(tc, "void\nqsend(int into, int sorted");
	for (j = 0; j < Mpars; j++)
		fprintf(tc, ", int fld%d", j);
	fprintf(tc, ")\n");
	ntimes(tc, 0, 1, Addq11);

	for (q = qtab; q; q = q->nxt)
	{	sprintf(buf0, "((Q%d *)z)->", q->qid);
		fprintf(tc, "\tcase %d:%s\n", q->qid, (q->nslots)?"":" /* =rv= */");
		if (q->nslots == 0)	/* reset handshake point */
			fprintf(tc, "\t\t(trpt+2)->o_m = 0;\n");

	if (has_sorted)
	{	fprintf(tc, "\t\tif (!sorted) goto append%d;\n", q->qid);
		fprintf(tc, "\t\tfor (j = 0; j < %sQlen; j++)\n", buf0);
		fprintf(tc, "\t\t{\t/* find insertion point */\n");
		sprintf(buf0, "((Q%d *)z)->contents[j].fld", q->qid);
		for (j = 0; j < q->nflds; j++)
		{	fprintf(tc, "\t\t\tif (fld%d > %s%d) continue;\n",
					j, buf0, j);
			fprintf(tc, "\t\t\tif (fld%d < %s%d) goto found%d;\n\n",
					j, buf0, j, q->qid);
		}
		fprintf(tc, "\t\t}\n");
		fprintf(tc, "\tfound%d:\n", q->qid);
		sprintf(buf0, "((Q%d *)z)->", q->qid);
		fprintf(tc, "\t\tfor (k = %sQlen - 1; k >= j; k--)\n", buf0);
		fprintf(tc, "\t\t{\t/* shift up */\n");
		for (j = 0; j < q->nflds; j++)
		{	fprintf(tc, "\t\t\t%scontents[k+1].fld%d = ", buf0, j);
			fprintf(tc, "%scontents[k].fld%d;\n", buf0, j);
		}
		fprintf(tc, "\t\t}\n");

		fprintf(tc, "\tappend%d:\t/* insert in slot j */\n", q->qid);
	}

		fprintf(tc, "\t\t(trpt+1)->oval = j;\n");
		fprintf(tc, "\t\t%sQlen = %sQlen + 1;\n", buf0, buf0);
		sprintf(buf0, "((Q%d *)z)->contents[j].fld", q->qid);
		for (j = 0; j < q->nflds; j++)
			fprintf(tc, "\t\t%s%d = fld%d;\n", buf0, j, j);
		fprintf(tc, "\t\tbreak;\n");
	}
	ntimes(tc, 0, 1, Addq2);

	for (q = qtab; q; q = q->nxt)
	fprintf(tc, "\tcase %d: return %d;\n", q->qid, (!q->nslots));

	ntimes(tc, 0, 1, Addq3);

	for (q = qtab; q; q = q->nxt)
	fprintf(tc, "\tcase %d: return (q_sz(from) == %d);\n",
			q->qid, max(1, q->nslots));

	ntimes(tc, 0, 1, Addq4);
	for (q = qtab; q; q = q->nxt)
	{	sprintf(buf0, "((Q%d *)z)->", q->qid);
		fprintf(tc, "	case %d:%s\n\t\t",
			q->qid, (q->nslots)?"":" /* =rv= */");
		if (q->nflds == 1)
		{	fprintf(tc, "if (fld == 0) r = %s", buf0);
			fprintf(tc, "contents[slot].fld0;\n");
		} else
		{	fprintf(tc, "switch (fld) {\n");
			ncases(tc, q->qid, 0, q->nflds, R12);
			fprintf(tc, "\t\t}\n");
		}
		fprintf(tc, "\t\tif (done)\n");
		fprintf(tc, "\t\t{	j = %sQlen;\n",  buf0);
		fprintf(tc, "\t\t	%sQlen = --j;\n", buf0);
		fprintf(tc, "\t\t	for (k=slot; k<j; k++)\n");
		fprintf(tc, "\t\t	{\n");

		sprintf(buf0, "\t\t\t((Q%d *)z)->contents", q->qid);
		for (j = 0; j < q->nflds; j++)
		{	fprintf(tc, "\t%s[k].fld%d = \n", buf0, j);
			fprintf(tc, "\t\t%s[k+1].fld%d;\n", buf0, j);
		}
		fprintf(tc, "\t\t	}\n");
		for (j = 0; j < q->nflds; j++)
			fprintf(tc, "%s[j].fld%d = 0;\n", buf0, j);
		fprintf(tc, "\t\t\tif (fld+1 != %d)\n\t\t\t", q->nflds);
		fprintf(tc, "\tuerror(\"missing pars in receive\");\n");
		/* incompletely received msgs cannot be unrecv'ed */
		fprintf(tc, "\t\t}\n");
		fprintf(tc, "\t\tbreak;\n");
	}
	ntimes(tc, 0, 1, Addq5);
	ntimes(th, 0, 1, Proto);	/* tag on function prototypes */
	fprintf(th, "void qsend(int, int");
	for (j = 0; j < Mpars; j++)
		fprintf(th, ", int");
	fprintf(th, ");\n");

	fprintf(th, "#define Addproc(x)	addproc(x");
	for (j = 0; j < Npars; j++)
		fprintf(th, ", 0");
	fprintf(th, ")\n");
}

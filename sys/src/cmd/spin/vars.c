/***** spin: vars.c *****/

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

extern RunList	*X, *LastX;
extern Symbol	*Fname;
extern int	lineno, depth, verbose, xspin, Have_claim, no_arrays;

int
getval(Lextok *sn)
{	Symbol *s = sn->sym;

	if (strcmp(s->name, "_last") == 0)
		return (LastX)?LastX->pid:0;
	if (strcmp(s->name, "_p") == 0)
		return (X && X->pc)?X->pc->seqno:0;
	if (strcmp(s->name, "_pid") == 0)
	{	if (!X) return 0;
		if (Have_claim
		&&  Have_claim >= X->pid)
			return X->pid - 1;
		return X->pid;
		
	}
	if (s->context && s->type)
		return getlocal(sn);

	if (!s->type)	/* not declared locally */
	{	s = lookup(s->name); /* try global */
		sn->sym = s;	/* fix it */
	}

	return getglobal(sn);
}

int
setval(Lextok *v, int n)
{
	if (v->sym->context && v->sym->type)
		return setlocal(v, n);
	if (!v->sym->type)
		v->sym = lookup(v->sym->name);
	return setglobal(v, n);
}

int
checkvar(Symbol *s, int n)
{	int i;

	if (n >= s->nel || n < 0)
	{	non_fatal("array indexing error, '%s'", s->name);
		return 0;
	}

	if (s->type == 0)
	{	non_fatal("undecl var '%s' (assuming int)", s->name);
		s->type = INT;
	}
	/* it cannot be a STRUCT */
	if (s->val == (int *) 0)	/* uninitialized */
	{	s->val = (int *) emalloc(s->nel*sizeof(int));
		for (i = 0; i < s->nel; i++)
		{	if (s->type != CHAN)
				s->val[i] = eval(s->ini);
			else
				s->val[i] = qmake(s);
	}	}
	return 1;
}

int
getglobal(Lextok *sn)
{	Symbol *s = sn->sym;
	int n = eval(sn->lft);
	int i;

	if (s->type == 0 && X && (i = find_lab(s, X->n)))
	{	printf("findlab through getglobal on %s\n", s->name);
		return i;	/* can this happen? */
	}
	if (s->type == STRUCT)
		return Rval_struct(sn, s, 1); /* 1 = check init */
	if (checkvar(s, n))
		return cast_val(s->type, s->val[n]);
	return 0;
}

int
cast_val(int t, int v)
{	int i=0; short s=0; unsigned char u=0;

	if (t == INT || t == CHAN) i = v;
	else if (t == SHORT) s = (short) v;
	else if (t == BYTE || t == MTYPE)  u = (unsigned char)v;
	else if (t == BIT)   u = (unsigned char)(v&1);

	if (v != i+s+u)
	{	char buf[32]; sprintf(buf, "%d->%d (%d)", v, i+s+u, t);
		non_fatal("value (%s) truncated in assignment", buf);
	}
	return (int)(i+s+u);
}

int
setglobal(Lextok *v, int m)
{
	if (v->sym->type == STRUCT)
		(void) Lval_struct(v, v->sym, 1, m);
	else
	{	int n = eval(v->lft);
		if (checkvar(v->sym, n))
		{	v->sym->val[n] = m;
			v->sym->setat = depth;
	}	}
	return 1;
}

void
dumpclaims(FILE *fd, int pid, char *s)
{	extern Lextok *Xu_List; extern int terse, Pid;
	Lextok *m; int cnt = 0; int oPid = Pid;

	for (m = Xu_List; m; m = m->rgt)
		if (strcmp(m->sym->name, s) == 0)
		{	cnt=1;
			break;
		}
	if (cnt == 0) return;

	Pid = pid;
	fprintf(fd, "#ifndef XUSAFE\n");
	for (m = Xu_List; m; m = m->rgt)
	{	if (strcmp(m->sym->name, s) != 0)
			continue;
		no_arrays = 1;
		putname(fd, "\t\tsetq_claim(", m->lft, 0, "");
		no_arrays = 0;
		fprintf(fd, ", %d, ", m->val);
		terse = 1;
		putname(fd, "\"", m->lft, 0, "\", h, ");
		terse = 0;
		fprintf(fd, "\"%s\");\n", s);
	}
	fprintf(fd, "#endif\n");
	Pid = oPid;
}

void
dumpglobals(void)
{	extern Symbol *symtab[Nhash+1];
	Lextok *dummy;
	Symbol *sp;
	int i, j;

	for (i = 0; i <= Nhash; i++)
	for (sp = symtab[i]; sp; sp = sp->next)
	{	if (!sp->type || sp->context || sp->owner
		||  sp->type == PROCTYPE || sp->type == PREDEF)
			continue;
		if (sp->type == STRUCT)
			dump_struct(sp, sp->name, 0);
		else
		for (j = 0; j < sp->nel; j++)
		{	if (sp->type == CHAN)
			{	doq(sp, j, 0);
				continue;
			} else
			{	if (xspin > 0
				&&  (verbose&4)
				&&  sp->setat < depth)
					continue;

				dummy = nn(ZN, NAME,
					nn(ZN,CONST,ZN,ZN), ZN);
				dummy->sym = sp;
				dummy->lft->val = j;
				printf("\t\t%s", sp->name);
				if (sp->nel > 1) printf("[%d]", j);
				printf(" = %d\n", getglobal(dummy));
	}	}	}
}

void
dumplocal(RunList *r)
{	Lextok *dummy;
	Symbol *z, *s = r->symtab;
	int i;

	for (z = s; z; z = z->next)
	{	if (z->type == STRUCT)
			dump_struct(z, z->name, r);
		else
		for (i = 0; i < z->nel; i++)
		{	if (z->type == CHAN)
				doq(z, i, r);
			else
			{
				if (xspin > 0
				&&  (verbose&4)
				&&  z->setat < depth)
					continue;
				dummy = nn(ZN, NAME,
					nn(ZN,CONST,ZN,ZN), ZN);
				dummy->sym = z;
				dummy->lft->val = i;
				printf("\t\t%s(%d):%s",
					r->n->name, r->pid, z->name);
				if (z->nel > 1) printf("[%d]", i);
				printf(" = %d\n", getval(dummy));
	}	}	}
}

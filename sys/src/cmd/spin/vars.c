/***** spin: vars.c *****/

/* Copyright (c) 1991-2000 by Lucent Technologies - Bell Laboratories     */
/* All Rights Reserved.  This software is for educational purposes only.  */
/* Permission is given to distribute this code provided that this intro-  */
/* ductory message is not removed and no monies are exchanged.            */
/* No guarantee is expressed or implied by the distribution of this code. */
/* Software written by Gerard J. Holzmann as part of the book:            */
/* `Design and Validation of Computer Protocols,' ISBN 0-13-539925-4,     */
/* Prentice Hall, Englewood Cliffs, NJ, 07632.                            */
/* Send bug-reports and/or questions to: gerard@research.bell-labs.com    */

#include "spin.h"
#ifdef PC
#include "y_tab.h"
#else
#include "y.tab.h"
#endif

extern Ordered	*all_names;
extern RunList	*X, *LastX;
extern Symbol	*Fname;
extern char	Buf[];
extern int	lineno, depth, verbose, xspin, limited_vis;
extern int	analyze, jumpsteps, nproc, nstop, columns;
extern short	no_arrays, Have_claim, Skip_claim;
extern void	sr_mesg(FILE *, int, int);
extern void	sr_buf(int, int);

static int	getglobal(Lextok *);
static int	setglobal(Lextok *, int);
static int	maxcolnr = 1;

int
getval(Lextok *sn)
{	Symbol *s = sn->sym;

	if (strcmp(s->name, "_") == 0)
	{	non_fatal("attempt to read value of '_'", 0);
		return 0;
	}
	if (strcmp(s->name, "_last") == 0)
		return (LastX)?LastX->pid:0;
	if (strcmp(s->name, "_p") == 0)
		return (X && X->pc)?X->pc->seqno:0;
	if (strcmp(s->name, "_pid") == 0)
	{	if (!X) return 0;
		return X->pid - Have_claim;
	}
	if (strcmp(s->name, "_nr_pr") == 0)
		return nproc-nstop;	/* new 3.3.10 */

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

void
rm_selfrefs(Symbol *s, Lextok *i)
{
	if (!i) return;

	if (i->ntyp == NAME
	&&  strcmp(i->sym->name, s->name) == 0
	&& (	(!i->sym->context && !s->context)
	   ||	( i->sym->context &&  s->context
		&& strcmp(i->sym->context->name, s->context->name) == 0)))
	{	lineno  = i->ln;
		Fname   = i->fn;
		non_fatal("self-reference initializing '%s'", s->name);
		i->ntyp = CONST;
		i->val  = 0;
	} else
	{	rm_selfrefs(s, i->lft);
		rm_selfrefs(s, i->rgt);
	}
}

int
checkvar(Symbol *s, int n)
{	int	i, oln = lineno;	/* calls on eval() change it */
	Symbol	*ofnm = Fname;

	if (n >= s->nel || n < 0)
	{	printf("spin: indexing %s[%d] - size is %d\n",
			s->name, n, s->nel);
		non_fatal("indexing array \'%s\'", s->name);
		return 0;
	}
	if (s->type == 0)
	{	non_fatal("undecl var %s (assuming int)", s->name);
		s->type = INT;
	}
	/* not a STRUCT */
	if (s->val == (int *) 0)	/* uninitialized */
	{	s->val = (int *) emalloc(s->nel*sizeof(int));
		for (i = 0; i < s->nel; i++)
		{	if (s->type != CHAN)
			{	rm_selfrefs(s, s->ini);
				s->val[i] = eval(s->ini);
			} else if (!analyze)
				s->val[i] = qmake(s);
	}	}
	lineno = oln;
	Fname  = ofnm;
	return 1;
}

static int
getglobal(Lextok *sn)
{	Symbol *s = sn->sym;
	int i, n = eval(sn->lft);

	if (s->type == 0 && X && (i = find_lab(s, X->n, 0)))
	{	printf("findlab through getglobal on %s\n", s->name);
		return i;	/* can this happen? */
	}
	if (s->type == STRUCT)
		return Rval_struct(sn, s, 1); /* 1 = check init */
	if (checkvar(s, n))
		return cast_val(s->type, s->val[n], s->nbits);
	return 0;
}

int
cast_val(int t, int v, int w)
{	int i=0; short s=0; unsigned char u=0;

	if (t == INT || t == CHAN) i = v;
	else if (t == SHORT) s = (short) v;
	else if (t == BYTE || t == MTYPE)  u = (unsigned char)v;
	else if (t == BIT)   u = (unsigned char)(v&1);
	else if (t == UNSIGNED)
	{	if (w == 0)
			fatal("cannot happen, cast_val", (char *)0);
		u = (unsigned char)(v& ((1<<w)-1));
	}

	if (v != i+s+u)
	{	char buf[32]; sprintf(buf, "%d->%d (%d)", v, i+s+u, t);
		non_fatal("value (%s) truncated in assignment", buf);
	}
	return (int)(i+s+u);
}

static int
setglobal(Lextok *v, int m)
{
	if (v->sym->type == STRUCT)
		(void) Lval_struct(v, v->sym, 1, m);
	else
	{	int n = eval(v->lft);
		if (checkvar(v->sym, n))
		{	if (v->sym->nbits > 0)
				m = (m & ((1<<v->sym->nbits)-1));	
			v->sym->val[n] = m;
			v->sym->setat = depth;
	}	}
	return 1;
}

void
dumpclaims(FILE *fd, int pid, char *s)
{	extern Lextok *Xu_List; extern int Pid;
	extern short terse;
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
{	Ordered *walk;
	Lextok *dummy;
	Symbol *sp;
	int j;

	for (walk = all_names; walk; walk = walk->next)
	{	sp = walk->entry;
		if (!sp->type || sp->context || sp->owner
		||  sp->type == PROCTYPE || sp->type == PREDEF
		||  (sp->type == MTYPE && ismtype(sp->name)))
			continue;

		if (sp->type == STRUCT)
		{	dump_struct(sp, sp->name, 0);
			continue;
		}
		for (j = 0; j < sp->nel; j++)
		{	int prefetch;
			if (sp->type == CHAN)
			{	doq(sp, j, 0);
				continue;
			}
			if ((verbose&4) && !(verbose&64)
			&&  (sp->setat < depth
			&&   jumpsteps != depth))
				continue;
			dummy = nn(ZN, NAME, nn(ZN,CONST,ZN,ZN), ZN);
			dummy->sym = sp;
			dummy->lft->val = j;
			/* in case of cast_val warnings, do this first: */
			prefetch = getglobal(dummy);
			printf("\t\t%s", sp->name);
			if (sp->nel > 1) printf("[%d]", j);
			printf(" = ");
			sr_mesg(stdout, prefetch,
				sp->type == MTYPE);
			printf("\n");
			if (limited_vis && (sp->hidden&2))
			{	int colpos;
				Buf[0] = '\0';
				if (!xspin)
				{	if (columns == 2)
					sprintf(Buf, "~G%s = ", sp->name);
					else
					sprintf(Buf, "%s = ", sp->name);
				}
				sr_buf(prefetch, sp->type == MTYPE);
				if (sp->colnr == 0)
				{	sp->colnr = maxcolnr;
					maxcolnr = 1+(maxcolnr%10);
				}
				colpos = nproc+sp->colnr-1;
				if (columns == 2)
				{	pstext(colpos, Buf);
					continue;
				}
				if (!xspin)
				{	printf("\t\t%s\n", Buf);
					continue;
				}
				printf("MSC: ~G %s %s\n", sp->name, Buf);
				printf("%3d:\tproc %3d (TRACK) line   1 \"var\" ",
					depth, colpos);
				printf("(state 0)\t[printf('MSC: globvar\\\\n')]\n");
				printf("\t\t%s", sp->name);
				if (sp->nel > 1) printf("[%d]", j);
				printf(" = %s\n", Buf);
	}	}	}
}

void
dumplocal(RunList *r)
{	Lextok *dummy;
	Symbol *z, *s = r->symtab;
	int i;

	for (z = s; z; z = z->next)
	{	if (z->type == STRUCT)
		{	dump_struct(z, z->name, r);
			continue;
		}
		for (i = 0; i < z->nel; i++)
		{	if (z->type == CHAN)
			{	doq(z, i, r);
				continue;
			}
			if ((verbose&4) && !(verbose&64)
			&&  (z->setat < depth
			&&   jumpsteps != depth))
				continue;

			dummy = nn(ZN, NAME, nn(ZN,CONST,ZN,ZN), ZN);
			dummy->sym = z;
			dummy->lft->val = i;

			printf("\t\t%s(%d):%s",
				r->n->name, r->pid, z->name);
			if (z->nel > 1) printf("[%d]", i);
			printf(" = ");
			sr_mesg(stdout, getval(dummy),
				z->type == MTYPE);
			printf("\n");
			if (limited_vis && (z->hidden&2))
			{	int colpos;
				Buf[0] = '\0';
				if (!xspin)
				{	if (columns == 2)
					sprintf(Buf, "~G%s(%d):%s = ",
					r->n->name, r->pid, z->name);
					else
					sprintf(Buf, "%s(%d):%s = ",
					r->n->name, r->pid, z->name);
				}
				sr_buf(getval(dummy), z->type==MTYPE);
				if (z->colnr == 0)
				{	z->colnr = maxcolnr;
					maxcolnr = 1+(maxcolnr%10);
				}
				colpos = nproc+z->colnr-1;
				if (columns == 2)
				{	pstext(colpos, Buf);
					continue;
				}
				if (!xspin)
				{	printf("\t\t%s\n", Buf);
					continue;
				}
				printf("MSC: ~G %s(%d):%s %s\n",
					r->n->name, r->pid, z->name, Buf);

				printf("%3d:\tproc %3d (TRACK) line   1 \"var\" ",
					depth, colpos);
				printf("(state 0)\t[printf('MSC: locvar\\\\n')]\n");
				printf("\t\t%s(%d):%s",
					r->n->name, r->pid, z->name);
				if (z->nel > 1) printf("[%d]", i);
				printf(" = %s\n", Buf);
	}	}	}
}

/***** spin: vars.c *****/

/*
 * This file is part of the public release of Spin. It is subject to the
 * terms in the LICENSE file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com
 */

#include "spin.h"
#include "y.tab.h"

extern char	GBuf[];
extern int	analyze, jumpsteps, nproc, nstop, columns, old_priority_rules;
extern int	lineno, depth, verbose, xspin, limited_vis, Pid_nr;
extern Lextok	*Xu_List;
extern Ordered	*all_names;
extern RunList	*X_lst, *LastX;
extern short	no_arrays, Have_claim, terse;
extern Symbol	*Fname;

extern void	sr_buf(int, int, const char *);
extern void	sr_mesg(FILE *, int, int, const char *);

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
		return (X_lst && X_lst->pc)?X_lst->pc->seqno:0;
	if (strcmp(s->name, "_pid") == 0)
	{	if (!X_lst) return 0;
		return X_lst->pid - Have_claim;
	}
	if (strcmp(s->name, "_priority") == 0)
	{	if (!X_lst) return 0;

		if (old_priority_rules)
		{	non_fatal("cannot refer to _priority with -o6", (char *) 0);
			return 1;
		}
		return X_lst->priority;
	}

	if (strcmp(s->name, "_nr_pr") == 0)
	{	return nproc-nstop;	/* new 3.3.10 */
	}

	if (s->context && s->type)
	{	return getlocal(sn);
	}

	if (!s->type)	/* not declared locally */
	{	s = lookup(s->name); /* try global */
		sn->sym = s;	/* fix it */
	}

	return getglobal(sn);
}

int
setval(Lextok *v, int n)
{
	if (strcmp(v->sym->name, "_last") == 0
	||  strcmp(v->sym->name, "_p") == 0
	||  strcmp(v->sym->name, "_pid") == 0
	||  strcmp(v->sym->name, "_nr_qs") == 0
	||  strcmp(v->sym->name, "_nr_pr") == 0)
	{	non_fatal("illegal assignment to %s", v->sym->name);
	}
	if (strcmp(v->sym->name, "_priority") == 0)
	{	if (old_priority_rules)
		{	non_fatal("cannot refer to _priority with -o6", (char *) 0);
			return 1;
		}
		if (!X_lst)
		{	non_fatal("no context for _priority", (char *) 0);
			return 1;
		}
		X_lst->priority = n;
	}

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
	Lextok  *z, *y;

	if (!in_bound(s, n))
		return 0;

	if (s->type == 0)
	{	non_fatal("undecl var %s (assuming int)", s->name);
		s->type = INT;
	}
	/* not a STRUCT */
	if (s->val == (int *) 0)	/* uninitialized */
	{	s->val = (int *) emalloc(s->nel*sizeof(int));
		z = s->ini;
		for (i = 0; i < s->nel; i++)
		{	if (z && z->ntyp == ',')
			{	y = z->lft;
				z = z->rgt;
			} else
			{	y = z;
			}
			if (s->type != CHAN)
			{	rm_selfrefs(s, y);
				s->val[i] = eval(y);
			} else if (!analyze)
			{	s->val[i] = qmake(s);
	}	}	}
	lineno = oln;
	Fname  = ofnm;

	return 1;
}

static int
getglobal(Lextok *sn)
{	Symbol *s = sn->sym;
	int i, n = eval(sn->lft);

	if (s->type == 0 && X_lst && (i = find_lab(s, X_lst->n, 0)))	/* getglobal */
	{	printf("findlab through getglobal on %s\n", s->name);
		return i;	/* can this happen? */
	}
	if (s->type == STRUCT)
	{	return Rval_struct(sn, s, 1); /* 1 = check init */
	}
	if (checkvar(s, n))
	{	return cast_val(s->type, s->val[n], s->nbits);
	}
	return 0;
}

int
cast_val(int t, int v, int w)
{	int i=0; short s=0; unsigned int u=0;

	if (t == PREDEF || t == INT || t == CHAN) i = v;	/* predef means _ */
	else if (t == SHORT) s = (short) v;
	else if (t == BYTE || t == MTYPE)  u = (unsigned char)v;
	else if (t == BIT)   u = (unsigned char)(v&1);
	else if (t == UNSIGNED)
	{	if (w == 0)
			fatal("cannot happen, cast_val", (char *)0);
	/*	u = (unsigned)(v& ((1<<w)-1));		problem when w=32	*/
		u = (unsigned)(v& (~0u>>(8*sizeof(unsigned)-w)));	/* doug */
	}

	if (v != i+s+ (int) u)
	{	char buf[64]; sprintf(buf, "%d->%d (%d)", v, i+s+(int)u, t);
		non_fatal("value (%s) truncated in assignment", buf);
	}
	return (int)(i+s+(int)u);
}

static int
setglobal(Lextok *v, int m)
{
	if (v->sym->type == STRUCT)
	{	(void) Lval_struct(v, v->sym, 1, m);
	} else
	{	int n = eval(v->lft);
		if (checkvar(v->sym, n))
		{	int oval = v->sym->val[n];
			int nval = cast_val(v->sym->type, m, v->sym->nbits);
			v->sym->val[n] = nval;
			if (oval != nval)
			{	v->sym->setat = depth;
	}	}	}
	return 1;
}

void
dumpclaims(FILE *fd, int pid, char *s)
{	Lextok *m; int cnt = 0; int oPid = Pid_nr;

	for (m = Xu_List; m; m = m->rgt)
		if (strcmp(m->sym->name, s) == 0)
		{	cnt=1;
			break;
		}
	if (cnt == 0) return;

	Pid_nr = pid;
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
	Pid_nr = oPid;
}

void
dumpglobals(void)
{	Ordered *walk;
	static Lextok *dummy = ZN;
	Symbol *sp;
	int j;

	if (!dummy)
		dummy = nn(ZN, NAME, nn(ZN,CONST,ZN,ZN), ZN);

	for (walk = all_names; walk; walk = walk->next)
	{	sp = walk->entry;
		if (!sp->type || sp->context || sp->owner
		||  sp->type == PROCTYPE  || sp->type == PREDEF
		||  sp->type == CODE_FRAG || sp->type == CODE_DECL
		||  (sp->type == MTYPE && ismtype(sp->name)))
			continue;

		if (sp->type == STRUCT)
		{	if ((verbose&4) && !(verbose&64)
			&&  (sp->setat < depth
			&&   jumpsteps != depth))
			{	continue;
			}
			dump_struct(sp, sp->name, 0);
			continue;
		}
		for (j = 0; j < sp->nel; j++)
		{	int prefetch;
			char *s = 0;
			if (sp->type == CHAN)
			{	doq(sp, j, 0);
				continue;
			}
			if ((verbose&4) && !(verbose&64)
			&&  (sp->setat < depth
			&&   jumpsteps != depth))
			{	continue;
			}

			dummy->sym = sp;
			dummy->lft->val = j;
			/* in case of cast_val warnings, do this first: */
			prefetch = getglobal(dummy);
			printf("\t\t%s", sp->name);
			if (sp->nel > 1 || sp->isarray) printf("[%d]", j);
			printf(" = ");
			if (sp->type == MTYPE
			&&  sp->mtype_name)
			{	s = sp->mtype_name->name;
			}
			sr_mesg(stdout, prefetch, sp->type == MTYPE, s);
			printf("\n");
			if (limited_vis && (sp->hidden&2))
			{	int colpos;
				GBuf[0] = '\0';
				if (!xspin)
				{	if (columns == 2)
						sprintf(GBuf, "~G%s = ", sp->name);
					else
						sprintf(GBuf, "%s = ", sp->name);
				}
				sr_buf(prefetch, sp->type == MTYPE, s);
				if (sp->colnr == 0)
				{	sp->colnr = (unsigned char) maxcolnr;
					maxcolnr = 1+(maxcolnr%10);
				}
				colpos = nproc+sp->colnr-1;
				if (columns == 2)
				{	pstext(colpos, GBuf);
					continue;
				}
				if (!xspin)
				{	printf("\t\t%s\n", GBuf);
					continue;
				}
				printf("MSC: ~G %s %s\n", sp->name, GBuf);
				printf("%3d:\tproc %3d (TRACK) line   1 \"var\" ",
					depth, colpos);
				printf("(state 0)\t[printf('MSC: globvar\\\\n')]\n");
				printf("\t\t%s", sp->name);
				if (sp->nel > 1 || sp->isarray) printf("[%d]", j);
				printf(" = %s\n", GBuf);
	}	}	}
}

void
dumplocal(RunList *r, int final)
{	static Lextok *dummy = ZN;
	Symbol *z, *s;
	int i;

	if (!r) return;

	s = r->symtab;

	if (!dummy)
	{	dummy = nn(ZN, NAME, nn(ZN,CONST,ZN,ZN), ZN);
	}

	for (z = s; z; z = z->next)
	{	if (z->type == STRUCT)
		{	dump_struct(z, z->name, r);
			continue;
		}
		for (i = 0; i < z->nel; i++)
		{	char *t = 0;
			if (z->type == CHAN)
			{	doq(z, i, r);
				continue;
			}

			if ((verbose&4) && !(verbose&64)
			&&  !final
			&&  (z->setat < depth
			&&   jumpsteps != depth))
			{	continue;
			}

			dummy->sym = z;
			dummy->lft->val = i;

			printf("\t\t%s(%d):%s",
				r->n->name, r->pid - Have_claim, z->name);
			if (z->nel > 1 || z->isarray) printf("[%d]", i);
			printf(" = ");

			if (z->type == MTYPE
			&&  z->mtype_name)
			{	t = z->mtype_name->name;
			}
			sr_mesg(stdout, getval(dummy), z->type == MTYPE, t);
			printf("\n");
			if (limited_vis && (z->hidden&2))
			{	int colpos;
				GBuf[0] = '\0';
				if (!xspin)
				{	if (columns == 2)
					sprintf(GBuf, "~G%s(%d):%s = ",
					r->n->name, r->pid, z->name);
					else
					sprintf(GBuf, "%s(%d):%s = ",
					r->n->name, r->pid, z->name);
				}
				sr_buf(getval(dummy), z->type==MTYPE, t);
				if (z->colnr == 0)
				{	z->colnr = (unsigned char) maxcolnr;
					maxcolnr = 1+(maxcolnr%10);
				}
				colpos = nproc+z->colnr-1;
				if (columns == 2)
				{	pstext(colpos, GBuf);
					continue;
				}
				if (!xspin)
				{	printf("\t\t%s\n", GBuf);
					continue;
				}
				printf("MSC: ~G %s(%d):%s %s\n",
					r->n->name, r->pid, z->name, GBuf);

				printf("%3d:\tproc %3d (TRACK) line   1 \"var\" ",
					depth, colpos);
				printf("(state 0)\t[printf('MSC: locvar\\\\n')]\n");
				printf("\t\t%s(%d):%s",
					r->n->name, r->pid, z->name);
				if (z->nel > 1 || z->isarray) printf("[%d]", i);
				printf(" = %s\n", GBuf);
	}	}	}
}

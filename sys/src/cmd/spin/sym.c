/***** spin: sym.c *****/

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

extern Symbol	*Fname, *owner;
extern int	lineno, has_xu, depth;

Symbol	*symtab[Nhash+1];
Symbol	*context = ZS;
int	Nid = 0;

int
samename(Symbol *a, Symbol *b)
{
	if (!a && !b) return 1;
	if (!a || !b) return 0;
	return !strcmp(a->name, b->name);
}

int
hash(char *s)
{	int h=0;

	while (*s)
	{	h += *s++;
		h <<= 1;
		if (h&(Nhash+1))
			h |= 1;
	}
	return h&Nhash;
}

Symbol *
lookup(char *s)
{	Symbol *sp;
	int h=hash(s);

	for (sp = symtab[h]; sp; sp = sp->next)
		if (strcmp(sp->name, s) == 0
		&&  samename(sp->context, context)
		&&  samename(sp->owner, owner))
			return sp;			/* found  */

	if (context)					/* local scope */
	for (sp = symtab[h]; sp; sp = sp->next)
		if (strcmp(sp->name, s) == 0
		&& !sp->context
		&&  samename(sp->owner, owner))
			return sp;			/* global reference */

	sp = (Symbol *) emalloc(sizeof(Symbol));	/* add    */
	sp->name = (char *) emalloc(strlen(s) + 1);
	strcpy(sp->name, s);
	sp->nel = 1;
	sp->setat = depth;
	sp->context = context;
	sp->owner = owner;				/* if fld in struct */
	sp->next = symtab[h];
	symtab[h] = sp;

	return sp;
}

void
setptype(Lextok *n, int t, Lextok *vis)	/* predefined types */
{	int oln = lineno;
	while (n)
	{	if (n->sym->type)
		{ lineno = n->ln; Fname = n->fn;
		  non_fatal("redeclaration of '%s'", n->sym->name);
		  lineno = oln;
		}
		n->sym->type = t;
		if (vis)
			n->sym->hidden = 1;
		if (t == CHAN)
			n->sym->Nid = ++Nid;
		else
			n->sym->Nid = 0;
		if (n->sym->nel <= 0)
		{ lineno = n->ln; Fname = n->fn;
		  non_fatal("bad array size for '%s'", n->sym->name);
		  lineno = oln;
		}
		n = n->rgt;
	}
}

void
setonexu(Symbol *sp, int t)
{
	sp->xu |= t;
	if (t == XR || t == XS)
	{	if (sp->xup[t-1]
		&&  strcmp(sp->xup[t-1]->name, context->name))
		{	printf("error: x[rs] claims from %s and %s\n",
				sp->xup[t-1]->name, context->name);
			non_fatal("conflicting claims on chan '%s'",
				sp->name);
		}
		sp->xup[t-1] = context;
	}
}

void
setallxu(Lextok *n, int t)
{	Lextok *fp, *tl;

	for (fp = n; fp; fp = fp->rgt)
	for (tl = fp->lft; tl; tl = tl->rgt)
	{	if (tl->sym->type == STRUCT)
			setallxu(tl->sym->Slst, t);
		else if (tl->sym->type == CHAN)
			setonexu(tl->sym, t);
	}
}

Lextok *Xu_List = (Lextok *) 0;

void
setxus(Lextok *p, int t)
{	Lextok *m, *n;

	has_xu = 1; 
	if (!context)
	{	lineno = p->ln;
		Fname = p->fn;
		non_fatal("non-local x[rs] assertion", (char *)0);
	}
	for (m = p; m; m = m->rgt)
	{	Lextok *Xu_new = (Lextok *) emalloc(sizeof(Lextok));
		Xu_new->val = t;
		Xu_new->lft = m->lft;
		Xu_new->sym = context;
		Xu_new->rgt = Xu_List;
		Xu_List = Xu_new;

		n = m->lft;
		if (n->sym->type == STRUCT)
			setallxu(n->sym->Slst, t);
		else if (n->sym->type == CHAN)
			setonexu(n->sym, t);
		else
		{	int oln = lineno;
			lineno = n->ln; Fname = n->fn;
			non_fatal("xr or xs of non-chan '%s'",
				n->sym->name);
			lineno = oln;
		}
	}
}

Lextok *Mtype = (Lextok *) 0;

void
setmtype(Lextok *m)
{	Lextok *n = m;
	int oln = lineno;
	int cnt = 1;

	if (n)
	{	lineno = n->ln; Fname = n->fn;
	}

	if (Mtype)
		non_fatal("mtype redeclared", (char *)0);

	Mtype = n;

	while (n)	/* syntax check */
	{	if (!n->lft || !n->lft->sym
		||  (n->lft->ntyp != NAME)
		||   n->lft->lft)	/* indexed variable */
			fatal("bad mtype definition", (char *)0);

		/* label the name */
		n->lft->sym->type = MTYPE;
		n->lft->sym->ini = nn(ZN,CONST,ZN,ZN);
		n->lft->sym->ini->val = cnt;
		n = n->rgt;
		cnt++;
	}
	lineno = oln;
}

int
ismtype(char *str)
{	Lextok *n;
	int cnt = 1;

	for (n = Mtype; n; n = n->rgt)
	{	if (strcmp(str, n->lft->sym->name) == 0)
			return cnt;
		cnt++;
	}
	return 0;
}

int
sputtype(char *foo, int m)
{
	switch (m) {
	case BIT:	strcpy(foo, "bit   "); break;
	case BYTE:	strcpy(foo, "byte  "); break;
	case CHAN:	strcpy(foo, "chan  "); break;
	case SHORT:	strcpy(foo, "short "); break;
	case INT:	strcpy(foo, "int   "); break;
	case MTYPE:	strcpy(foo, "mtype "); break;
	case STRUCT:	strcpy(foo, "struct"); break;
	case PROCTYPE:	strcpy(foo, "proctype"); break;
	case LABEL:	strcpy(foo, "label "); return 0;
	default:	strcpy(foo, "value "); return 0;
	}
	return 1;
}


int
puttype(int m)
{	char buf[32];

	if (sputtype(buf, m))
	{	printf("%s", buf);
		return 1;
	}
	return 0;
}

void
symvar(Symbol *sp)
{	Lextok *m;

	if (!puttype(sp->type))
		return;

	printf("\t");
	if (sp->owner) printf("%s.", sp->owner->name);
	printf("%s", sp->name);
	if (sp->nel > 1) printf("[%d]", sp->nel);

	if (sp->type == CHAN)
		printf("\t%d", (sp->ini)?sp->ini->val:0);
	else
		printf("\t%d", eval(sp->ini));

	if (sp->owner)
		printf("\t<struct-field>");
	else
	if (!sp->context)
		printf("\t<global>");
	else
		printf("\t<%s>", sp->context->name);

	if (sp->Nid < 0)	/* formal parameter */
		printf("\t<parameter %d>", -(sp->Nid));
	else
		printf("\t<variable>");
	if (sp->type == CHAN && sp->ini)
	{	int i;
		for (m = sp->ini->rgt, i = 0; m; m = m->rgt)
			i++;
		printf("\t%d\t", i);
		for (m = sp->ini->rgt; m; m = m->rgt)
		{	(void) puttype(m->ntyp);
			if (m->rgt) printf("\t");
		}
	}
	printf("\n");
}

void
symdump(void)
{	Symbol *sp;
	int i;

	for (i = 0; i <= Nhash; i++)
	for (sp = symtab[i]; sp; sp = sp->next)
		symvar(sp);
}

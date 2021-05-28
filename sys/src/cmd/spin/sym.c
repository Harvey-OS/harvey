/***** spin: sym.c *****/

/*
 * This file is part of the public release of Spin. It is subject to the
 * terms in the LICENSE file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com
 */

#include "spin.h"
#include "y.tab.h"

extern Symbol	*Fname, *owner;
extern int	lineno, depth, verbose, NamesNotAdded, deadvar;
extern int	has_hidden, m_loss, old_scope_rules;
extern short	has_xu;
extern char	CurScope[MAXSCOPESZ];

Symbol	*context = ZS;
Ordered	*all_names = (Ordered *)0;
int	Nid_nr = 0;

Mtypes_t	*Mtypes;
Lextok		*runstmnts = ZN;

static Ordered	*last_name = (Ordered *)0;
static Symbol	*symtab[Nhash+1];

static int
samename(Symbol *a, Symbol *b)
{
	if (!a && !b) return 1;
	if (!a || !b) return 0;
	return !strcmp(a->name, b->name);
}

unsigned int
hash(const char *s)
{	unsigned int h = 0;

	while (*s)
	{	h += (unsigned int) *s++;
		h <<= 1;
		if (h&(Nhash+1))
			h |= 1;
	}
	return h&Nhash;
}

void
disambiguate(void)
{	Ordered *walk;
	Symbol *sp;
	char *n, *m;

	if (old_scope_rules)
		return;

	/* prepend the scope_prefix to the names */

	for (walk = all_names; walk; walk = walk->next)
	{	sp = walk->entry;
		if (sp->type != 0
		&&  sp->type != LABEL
		&&  strlen((const char *)sp->bscp) > 1)
		{	if (sp->context)
			{	m = (char *) emalloc(strlen((const char *)sp->bscp) + 1);
				sprintf(m, "_%d_", sp->context->sc);
				if (strcmp((const char *) m, (const char *) sp->bscp) == 0)
				{	continue;
				/* 6.2.0: only prepend scope for inner-blocks,
				   not for top-level locals within a proctype
				   this means that you can no longer use the same name
				   for a global and a (top-level) local variable
				 */
			}	}

			n = (char *) emalloc(strlen((const char *)sp->name)
				+ strlen((const char *)sp->bscp) + 1);
			sprintf(n, "%s%s", sp->bscp, sp->name);
			sp->name = n;	/* discard the old memory */
	}	}
}

Symbol *
lookup(char *s)
{	Symbol *sp; Ordered *no;
	unsigned int h = hash(s);

	if (old_scope_rules)
	{	/* same scope - global refering to global or local to local */
		for (sp = symtab[h]; sp; sp = sp->next)
		{	if (strcmp(sp->name, s) == 0
			&&  samename(sp->context, context)
			&&  samename(sp->owner, owner))
			{	return sp;		/* found */
		}	}
	} else
	{	/* added 6.0.0: more traditional, scope rule */
		for (sp = symtab[h]; sp; sp = sp->next)
		{	if (strcmp(sp->name, s) == 0
			&&  samename(sp->context, context)
			&&  (strcmp((const char *)sp->bscp, CurScope) == 0
			||   strncmp((const char *)sp->bscp, CurScope, strlen((const char *)sp->bscp)) == 0)
			&&  samename(sp->owner, owner))
			{
				if (!samename(sp->owner, owner))
				{	printf("spin: different container %s\n", sp->name);
					printf("	old: %s\n", sp->owner?sp->owner->name:"--");
					printf("	new: %s\n", owner?owner->name:"--");
				/*	alldone(1);	*/
				}
				return sp;		/* found */
	}	}	}

	if (context)				/* in proctype, refers to global */
	for (sp = symtab[h]; sp; sp = sp->next)
	{	if (strcmp(sp->name, s) == 0
		&& !sp->context
		&&  samename(sp->owner, owner))
		{	return sp;		/* global */
	}	}

	sp = (Symbol *) emalloc(sizeof(Symbol));
	sp->name = (char *) emalloc(strlen(s) + 1);
	strcpy(sp->name, s);
	sp->nel = 1;
	sp->setat = depth;
	sp->context = context;
	sp->owner = owner;			/* if fld in struct */
	sp->bscp = (unsigned char *) emalloc(strlen((const char *)CurScope)+1);
	strcpy((char *)sp->bscp, CurScope);

	if (NamesNotAdded == 0)
	{	sp->next = symtab[h];
		symtab[h] = sp;
		no = (Ordered *) emalloc(sizeof(Ordered));
		no->entry = sp;
		if (!last_name)
			last_name = all_names = no;
		else
		{	last_name->next = no;
			last_name = no;
	}	}

	return sp;
}

void
trackvar(Lextok *n, Lextok *m)
{	Symbol *sp = n->sym;

	if (!sp) return;	/* a structure list */
	switch (m->ntyp) {
	case NAME:
		if (m->sym->type != BIT)
		{	sp->hidden |= 4;
			if (m->sym->type != BYTE)
				sp->hidden |= 8;
		}
		break;
	case CONST:
		if (m->val != 0 && m->val != 1)
			sp->hidden |= 4;
		if (m->val < 0 || m->val > 256)
			sp->hidden |= 8; /* ditto byte-equiv */
		break;
	default:	/* unknown */
		sp->hidden |= (4|8); /* not known bit-equiv */
	}
}

void
trackrun(Lextok *n)
{
	runstmnts = nn(ZN, 0, n, runstmnts);
}

void
checkrun(Symbol *parnm, int posno)
{	Lextok *n, *now, *v; int i, m;
	int res = 0; char buf[16], buf2[16];

	for (n = runstmnts; n; n = n->rgt)
	{	now = n->lft;
		if (now->sym != parnm->context)
			continue;
		for (v = now->lft, i = 0; v; v = v->rgt, i++)
			if (i == posno)
			{	m = v->lft->ntyp;
				if (m == CONST)
				{	m = v->lft->val;
					if (m != 0 && m != 1)
						res |= 4;
					if (m < 0 || m > 256)
						res |= 8;
				} else if (m == NAME)
				{	m = v->lft->sym->type;
					if (m != BIT)
					{	res |= 4;
						if (m != BYTE)
							res |= 8;
					}
				} else
					res |= (4|8); /* unknown */
				break;
	}		}
	if (!(res&4) || !(res&8))
	{	if (!(verbose&32)) return;
		strcpy(buf2, (!(res&4))?"bit":"byte");
		sputtype(buf, parnm->type);
		i = (int) strlen(buf);
		while (i > 0 && buf[--i] == ' ') buf[i] = '\0';
		if (i == 0 || strcmp(buf, buf2) == 0) return;
		prehint(parnm);
		printf("proctype %s, '%s %s' could be declared",
			parnm->context?parnm->context->name:"", buf, parnm->name);
		printf(" '%s %s'\n", buf2, parnm->name);
	}
}

void
trackchanuse(Lextok *m, Lextok *w, int t)
{	Lextok *n = m; int cnt = 1;
	while (n)
	{	if (n->lft
		&&  n->lft->sym
		&&  n->lft->sym->type == CHAN)
			setaccess(n->lft->sym, w?w->sym:ZS, cnt, t);
		n = n->rgt; cnt++;
	}
}

void
setptype(Lextok *mtype_name, Lextok *n, int t, Lextok *vis)	/* predefined types */
{	int oln = lineno, cnt = 1; extern int Expand_Ok;

	while (n)
	{	if (n->sym->type && !(n->sym->hidden&32))
		{	lineno = n->ln; Fname = n->fn;
			fatal("redeclaration of '%s'", n->sym->name);
			lineno = oln;
		}
		n->sym->type = (short) t;

		if (mtype_name && t != MTYPE)
		{	lineno = n->ln; Fname = n->fn;
			fatal("missing semi-colon after '%s'?",
				mtype_name->sym->name);
			lineno = oln;
		}

		if (mtype_name && n->sym->mtype_name
		&& strcmp(mtype_name->sym->name, n->sym->mtype_name->name) != 0)
		{	fprintf(stderr, "spin: %s:%d, Error: '%s' is type '%s' but assigned type '%s'\n",
				n->fn->name, n->ln,
				n->sym->name,
				mtype_name->sym->name,
				n->sym->mtype_name->name);
			non_fatal("type error", (char *) 0);
		}

		n->sym->mtype_name = mtype_name?mtype_name->sym:0; /* if mtype, else 0 */

		if (Expand_Ok)
		{	n->sym->hidden |= (4|8|16); /* formal par */
			if (t == CHAN)
			setaccess(n->sym, ZS, cnt, 'F');
		}

		if (t == UNSIGNED)
		{	if (n->sym->nbits < 0 || n->sym->nbits >= 32)
			fatal("(%s) has invalid width-field", n->sym->name);
			if (n->sym->nbits == 0)
			{	n->sym->nbits = 16;
				non_fatal("unsigned without width-field", 0);
			}
		} else if (n->sym->nbits > 0)
		{	non_fatal("(%s) only an unsigned can have width-field",
				n->sym->name);
		}

		if (vis)
		{	if (strncmp(vis->sym->name, ":hide:", (size_t) 6) == 0)
			{	n->sym->hidden |= 1;
				has_hidden++;
				if (t == BIT)
				fatal("bit variable (%s) cannot be hidden",
					n->sym->name);
			} else if (strncmp(vis->sym->name, ":show:", (size_t) 6) == 0)
			{	n->sym->hidden |= 2;
			} else if (strncmp(vis->sym->name, ":local:", (size_t) 7) == 0)
			{	n->sym->hidden |= 64;
			}
		}

		if (t == CHAN)
		{	n->sym->Nid = ++Nid_nr;
		} else
		{	n->sym->Nid = 0;
			if (n->sym->ini
			&&  n->sym->ini->ntyp == CHAN)
			{	Fname = n->fn;
				lineno = n->ln;
				fatal("chan initializer for non-channel %s",
				n->sym->name);
		}	}

		if (n->sym->nel <= 0)
		{	lineno = n->ln; Fname = n->fn;
			non_fatal("bad array size for '%s'", n->sym->name);
			lineno = oln;
		}

		n = n->rgt; cnt++;
	}
}

static void
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

static void
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

	if (m_loss && t == XS)
	{	printf("spin: %s:%d, warning, xs tag not compatible with -m (message loss)\n",
			(p->fn != NULL) ? p->fn->name : "stdin", p->ln);
	}

	if (!context)
	{	lineno = p->ln;
		Fname = p->fn;
		fatal("non-local x[rs] assertion", (char *)0);
	}
	for (m = p; m; m = m->rgt)
	{	Lextok *Xu_new = (Lextok *) emalloc(sizeof(Lextok));
		Xu_new->uiid = p->uiid;
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

Lextok **
find_mtype_list(const char *s)
{	Mtypes_t *lst;

	for (lst = Mtypes; lst; lst = lst->nxt)
	{	if (strcmp(lst->nm, s) == 0)
		{	return &(lst->mt);
	}	}

	/* not found, create it */
	lst = (Mtypes_t *) emalloc(sizeof(Mtypes_t));
	lst->nm = (char *) emalloc(strlen(s)+1);
	strcpy(lst->nm, s);
	lst->nxt = Mtypes;
	Mtypes = lst;
	return &(lst->mt);
}

void
setmtype(Lextok *mtype_name, Lextok *m)
{	Lextok **mtl;	/* mtype list */
	Lextok *n, *Mtype;
	int cnt, oln = lineno;
	char *s = "_unnamed_";

	if (m) { lineno = m->ln; Fname = m->fn; }

	if (mtype_name && mtype_name->sym)
	{	s = mtype_name->sym->name;
	}

	mtl = find_mtype_list(s);
	Mtype = *mtl;

	if (!Mtype)
	{	*mtl = Mtype = m;
	} else
	{	for (n = Mtype; n->rgt; n = n->rgt)
		{	;
		}
		n->rgt = m;	/* concatenate */
	}

	for (n = Mtype, cnt = 1; n; n = n->rgt, cnt++)	/* syntax check */
	{	if (!n->lft || !n->lft->sym
		||   n->lft->ntyp != NAME
		||   n->lft->lft)	/* indexed variable */
			fatal("bad mtype definition", (char *)0);

		/* label the name */
		if (n->lft->sym->type != MTYPE)
		{	n->lft->sym->hidden |= 128;	/* is used */
			n->lft->sym->type = MTYPE;
			n->lft->sym->ini = nn(ZN,CONST,ZN,ZN);
			n->lft->sym->ini->val = cnt;
		} else if (n->lft->sym->ini->val != cnt)
		{	non_fatal("name %s appears twice in mtype declaration",
				n->lft->sym->name);
	}	}

	lineno = oln;
	if (cnt > 256)
	{	fatal("too many mtype elements (>255)", (char *) 0);
	}
}

char *
which_mtype(const char *str) /* which mtype is str, 0 if not an mtype at all  */
{	Mtypes_t *lst;
	Lextok *n;

	for (lst = Mtypes; lst; lst = lst->nxt)
	for (n = lst->mt; n; n = n->rgt)
	{	if (strcmp(str, n->lft->sym->name) == 0)
		{	return lst->nm;
	}	}

	return (char *) 0;
}

int
ismtype(char *str)	/* name to number */
{	Mtypes_t *lst;
	Lextok *n;
	int cnt;

	for (lst = Mtypes; lst; lst = lst->nxt)
	{	cnt = 1;
		for (n = lst->mt; n; n = n->rgt)
		{	if (strcmp(str, n->lft->sym->name) == 0)
			{	return cnt;
			}
			cnt++;
	}	}

	return 0;
}

int
sputtype(char *foo, int m)
{
	switch (m) {
	case UNSIGNED:	strcpy(foo, "unsigned "); break;
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


static int
puttype(int m)
{	char buf[128];

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
	if (sp->nel > 1 || sp->isarray == 1) printf("[%d]", sp->nel);

	if (sp->type == CHAN)
		printf("\t%d", (sp->ini)?sp->ini->val:0);
	else if (sp->type == STRUCT && sp->Snm != NULL) /* Frank Weil, 2.9.8 */
		printf("\t%s", sp->Snm->name);
	else
		printf("\t%d", eval(sp->ini));

	if (sp->owner)
		printf("\t<:struct-field:>");
	else
	if (!sp->context)
		printf("\t<:global:>");
	else
		printf("\t<%s>", sp->context->name);

	if (sp->Nid < 0)	/* formal parameter */
		printf("\t<parameter %d>", -(sp->Nid));
	else if (sp->type == MTYPE)
		printf("\t<constant>");
	else if (sp->isarray)
		printf("\t<array>");
	else
		printf("\t<variable>");

	if (sp->type == CHAN && sp->ini)
	{	int i;
		for (m = sp->ini->rgt, i = 0; m; m = m->rgt)
			i++;
		printf("\t%d\t", i);
		for (m = sp->ini->rgt; m; m = m->rgt)
		{	if (m->ntyp == STRUCT)
				printf("struct %s", m->sym->name);
			else
				(void) puttype(m->ntyp);
			if (m->rgt) printf("\t");
		}
	}

	if (!old_scope_rules)
	{	printf("\t{scope %s}", sp->bscp);
	}

	printf("\n");
}

void
symdump(void)
{	Ordered *walk;

	for (walk = all_names; walk; walk = walk->next)
		symvar(walk->entry);
}

void
chname(Symbol *sp)
{	printf("chan ");
	if (sp->context) printf("%s-", sp->context->name);
	if (sp->owner) printf("%s.", sp->owner->name);
	printf("%s", sp->name);
	if (sp->nel > 1 || sp->isarray == 1) printf("[%d]", sp->nel);
	printf("\t");
}

static struct X_lkp {
	int typ; char *nm;
} xx[] = {
	{ 'A', "exported as run parameter" },
	{ 'F', "imported as proctype parameter" },
	{ 'L', "used as l-value in asgnmnt" },
	{ 'V', "used as r-value in asgnmnt" },
	{ 'P', "polled in receive stmnt" },
	{ 'R', "used as parameter in receive stmnt" },
	{ 'S', "used as parameter in send stmnt" },
	{ 'r', "received from" },
	{ 's', "sent to" },
};

static void
chan_check(Symbol *sp)
{	Access *a; int i, b=0, d;

	if (verbose&1) goto report;	/* -C -g */

	for (a = sp->access; a; a = a->lnk)
		if (a->typ == 'r')
			b |= 1;
		else if (a->typ == 's')
			b |= 2;
	if (b == 3 || (sp->hidden&16))	/* balanced or formal par */
		return;
report:
	chname(sp);
	for (i = d = 0; i < (int) (sizeof(xx)/sizeof(struct X_lkp)); i++)
	{	b = 0;
		for (a = sp->access; a; a = a->lnk)
		{	if (a->typ == xx[i].typ)
			{	b++;
		}	}
		if (b == 0)
		{	continue;
		}
		d++;
		printf("\n\t%s by: ", xx[i].nm);
		for (a = sp->access; a; a = a->lnk)
		  if (a->typ == xx[i].typ)
		  {	printf("%s", a->who->name);
			if (a->what) printf(" to %s", a->what->name);
			if (a->cnt)  printf(" par %d", a->cnt);
			if (--b > 0) printf(", ");
		  }
	}
	printf("%s\n", (!d)?"\n\tnever used under this name":"");
}

void
chanaccess(void)
{	Ordered *walk;
	char buf[128];
	extern int Caccess, separate;
	extern short has_code;

	for (walk = all_names; walk; walk = walk->next)
	{	if (!walk->entry->owner)
		switch (walk->entry->type) {
		case CHAN:
			if (Caccess) chan_check(walk->entry);
			break;
		case MTYPE:
		case BIT:
		case BYTE:
		case SHORT:
		case INT:
		case UNSIGNED:
			if ((walk->entry->hidden&128))	/* was: 32 */
				continue;

			if (!separate
			&&  !walk->entry->context
			&&  !has_code
			&&   deadvar)
				walk->entry->hidden |= 1; /* auto-hide */

			if (!(verbose&32) || has_code) continue;

			printf("spin: %s:0, warning, ", Fname->name);
			sputtype(buf, walk->entry->type);
			if (walk->entry->context)
				printf("proctype %s",
					walk->entry->context->name);
			else
				printf("global");
			printf(", '%s%s' variable is never used (other than in print stmnts)\n",
				buf, walk->entry->name);
	}	}
}

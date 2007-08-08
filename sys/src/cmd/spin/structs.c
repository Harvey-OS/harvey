/***** spin: structs.c *****/

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

typedef struct UType {
	Symbol *nm;	/* name of the type */
	Lextok *cn;	/* contents */
	struct UType *nxt;	/* linked list */
} UType;

extern	Symbol	*Fname;
extern	int	lineno, depth, Expand_Ok;

Symbol	*owner;

static UType *Unames = 0;
static UType *Pnames = 0;

static Lextok	*cpnn(Lextok *, int, int, int);
extern void	sr_mesg(FILE *, int, int);

void
setuname(Lextok *n)
{	UType *tmp;

	for (tmp = Unames; tmp; tmp = tmp->nxt)
		if (!strcmp(owner->name, tmp->nm->name))
		{	non_fatal("typename %s was defined before",
				tmp->nm->name);
			return;
		}
	if (!owner) fatal("illegal reference inside typedef",
		(char *) 0);
	tmp = (UType *) emalloc(sizeof(UType));
	tmp->nm = owner;
	tmp->cn = n;
	tmp->nxt = Unames;
	Unames = tmp;
}

static void
putUname(FILE *fd, UType *tmp)
{	Lextok *fp, *tl;

	if (!tmp) return;
	putUname(fd, tmp->nxt); /* postorder */
	fprintf(fd, "struct %s { /* user defined type */\n",
		tmp->nm->name);
	for (fp = tmp->cn; fp; fp = fp->rgt)
	for (tl = fp->lft; tl; tl = tl->rgt)
		typ2c(tl->sym);
	fprintf(fd, "};\n");
}

void
putunames(FILE *fd)
{
	putUname(fd, Unames);
}

int
isutype(char *t)
{	UType *tmp;

	for (tmp = Unames; tmp; tmp = tmp->nxt)
	{	if (!strcmp(t, tmp->nm->name))
			return 1;
	}
	return 0;
}

Lextok *
getuname(Symbol *t)
{	UType *tmp;

	for (tmp = Unames; tmp; tmp = tmp->nxt)
	{	if (!strcmp(t->name, tmp->nm->name))
			return tmp->cn;
	}
	fatal("%s is not a typename", t->name);
	return (Lextok *)0;
}

void
setutype(Lextok *p, Symbol *t, Lextok *vis)	/* user-defined types */
{	int oln = lineno;
	Symbol *ofn = Fname;
	Lextok *m, *n;

	m = getuname(t);
	for (n = p; n; n = n->rgt)
	{	lineno = n->ln;
		Fname = n->fn;
		if (n->sym->type)
		non_fatal("redeclaration of '%s'", n->sym->name);

		if (n->sym->nbits > 0)
		non_fatal("(%s) only an unsigned can have width-field",
			n->sym->name);

		if (Expand_Ok)
			n->sym->hidden |= (4|8|16); /* formal par */

		if (vis)
		{	if (strncmp(vis->sym->name, ":hide:", 6) == 0)
				n->sym->hidden |= 1;
			else if (strncmp(vis->sym->name, ":show:", 6) == 0)
				n->sym->hidden |= 2;
			else if (strncmp(vis->sym->name, ":local:", 7) == 0)
				n->sym->hidden |= 64;
		}
		n->sym->type = STRUCT;	/* classification   */
		n->sym->Slst = m;	/* structure itself */
		n->sym->Snm  = t;	/* name of typedef  */
		n->sym->Nid  = 0;	/* this is no chan  */
		n->sym->hidden |= 4;
		if (n->sym->nel <= 0)
		non_fatal("bad array size for '%s'", n->sym->name);
	}
	lineno = oln;
	Fname = ofn;
}

static Symbol *
do_same(Lextok *n, Symbol *v, int xinit)
{	Lextok *tmp, *fp, *tl;
	int ix = eval(n->lft);
	int oln = lineno;
	Symbol *ofn = Fname;

	lineno = n->ln;
	Fname = n->fn;
	
	/* n->sym->type == STRUCT
	 * index:		n->lft
	 * subfields:		n->rgt
	 * structure template:	n->sym->Slst
	 * runtime values:	n->sym->Sval
	 */
	if (xinit) ini_struct(v);	/* once, at top level */

	if (ix >= v->nel || ix < 0)
	{	printf("spin: indexing %s[%d] - size is %d\n",
				v->name, ix, v->nel);
		fatal("indexing error \'%s\'", v->name);
	}
	if (!n->rgt || !n->rgt->lft)
	{	non_fatal("no subfields %s", v->name);	/* i.e., wants all */
		lineno = oln; Fname = ofn;
		return ZS;
	}

	if (n->rgt->ntyp != '.')
	{	printf("bad subfield type %d\n", n->rgt->ntyp);
		alldone(1);
	}

	tmp = n->rgt->lft;
	if (tmp->ntyp != NAME && tmp->ntyp != TYPE)
	{	printf("bad subfield entry %d\n", tmp->ntyp);
		alldone(1);
	}
	for (fp = v->Sval[ix]; fp; fp = fp->rgt)
	for (tl = fp->lft; tl; tl = tl->rgt)
		if (!strcmp(tl->sym->name, tmp->sym->name))
		{	lineno = oln; Fname = ofn;
			return tl->sym;
		}
	fatal("cannot locate subfield %s", tmp->sym->name);
	return ZS;
}

int
Rval_struct(Lextok *n, Symbol *v, int xinit)	/* n varref, v valref */
{	Symbol *tl;
	Lextok *tmp;
	int ix;

	if (!n || !(tl = do_same(n, v, xinit)))
		return 0;

	tmp = n->rgt->lft;
	if (tmp->sym->type == STRUCT)
	{	return Rval_struct(tmp, tl, 0);
	} else if (tmp->rgt)
		fatal("non-zero 'rgt' on non-structure", 0);

	ix = eval(tmp->lft);
	if (ix >= tl->nel || ix < 0)
		fatal("indexing error \'%s\'", tl->name);

	return cast_val(tl->type, tl->val[ix], tl->nbits);
}

int
Lval_struct(Lextok *n, Symbol *v, int xinit, int a)  /* a = assigned value */
{	Symbol *tl;
	Lextok *tmp;
	int ix;

	if (!(tl = do_same(n, v, xinit)))
		return 1;

	tmp = n->rgt->lft;
	if (tmp->sym->type == STRUCT)
		return Lval_struct(tmp, tl, 0, a);
	else if (tmp->rgt)
		fatal("non-zero 'rgt' on non-structure", 0);

	ix = eval(tmp->lft);
	if (ix >= tl->nel || ix < 0)
		fatal("indexing error \'%s\'", tl->name);

	if (tl->nbits > 0)
		a = (a & ((1<<tl->nbits)-1));	
	tl->val[ix] = a;
	tl->setat = depth;

	return 1;
}

int
Cnt_flds(Lextok *m)
{	Lextok *fp, *tl, *n;
	int cnt = 0;

	if (m->ntyp == ',')
	{	n = m;
		goto is_lst;
	}
	if (!m->sym || m->ntyp != STRUCT)
		return 1;

	n = getuname(m->sym);
is_lst:
	for (fp = n; fp; fp = fp->rgt)
	for (tl = fp->lft; tl; tl = tl->rgt)
	{	if (tl->sym->type == STRUCT)
		{	if (tl->sym->nel != 1)
				fatal("array of structures in param list, %s",
					tl->sym->name);
			cnt += Cnt_flds(tl->sym->Slst);
		}  else
			cnt += tl->sym->nel;
	}
	return cnt;
}

int
Sym_typ(Lextok *t)
{	Symbol *s = t->sym;

	if (!s) return 0;

	if (s->type != STRUCT)
		return s->type;

	if (!t->rgt
	||  !t->rgt->ntyp == '.'
	||  !t->rgt->lft)
		return STRUCT;	/* not a field reference */

	return Sym_typ(t->rgt->lft);
}

int
Width_set(int *wdth, int i, Lextok *n)
{	Lextok *fp, *tl;
	int j = i, k;

	for (fp = n; fp; fp = fp->rgt)
	for (tl = fp->lft; tl; tl = tl->rgt)
	{	if (tl->sym->type == STRUCT)
			j = Width_set(wdth, j, tl->sym->Slst);
		else
		{	for (k = 0; k < tl->sym->nel; k++, j++)
				wdth[j] = tl->sym->type;
	}	}
	return j;
}

void
ini_struct(Symbol *s)
{	int i; Lextok *fp, *tl;

	if (s->type != STRUCT)	/* last step */
	{	(void) checkvar(s, 0);
		return;
	}
	if (s->Sval == (Lextok **) 0)
	{	s->Sval = (Lextok **) emalloc(s->nel * sizeof(Lextok *));
		for (i = 0; i < s->nel; i++)
		{	s->Sval[i] = cpnn(s->Slst, 1, 1, 1);

			for (fp = s->Sval[i]; fp; fp = fp->rgt)
			for (tl = fp->lft; tl; tl = tl->rgt)
				ini_struct(tl->sym);
	}	}
}

static Lextok *
cpnn(Lextok *s, int L, int R, int S)
{	Lextok *d; extern int Nid;

	if (!s) return ZN;

	d = (Lextok *) emalloc(sizeof(Lextok));
	d->ntyp = s->ntyp;
	d->val  = s->val;
	d->ln   = s->ln;
	d->fn   = s->fn;
	d->sym  = s->sym;
	if (L) d->lft = cpnn(s->lft, 1, 1, S);
	if (R) d->rgt = cpnn(s->rgt, 1, 1, S);

	if (S && s->sym)
	{	d->sym = (Symbol *) emalloc(sizeof(Symbol));
		memcpy(d->sym, s->sym, sizeof(Symbol));
		if (d->sym->type == CHAN)
			d->sym->Nid = ++Nid;
	}
	if (s->sq || s->sl)
		fatal("cannot happen cpnn", (char *) 0);

	return d;
}

int
full_name(FILE *fd, Lextok *n, Symbol *v, int xinit)
{	Symbol *tl;
	Lextok *tmp;
	int hiddenarrays = 0;

	fprintf(fd, "%s", v->name);

	if (!n || !(tl = do_same(n, v, xinit)))
		return 0;
	tmp = n->rgt->lft;

	if (tmp->sym->type == STRUCT)
	{	fprintf(fd, ".");
		hiddenarrays = full_name(fd, tmp, tl, 0);
		goto out;
	}
	fprintf(fd, ".%s", tl->name);
out:	if (tmp->sym->nel > 1)
	{	fprintf(fd, "[%d]", eval(tmp->lft));
		hiddenarrays = 1;
	}
	return hiddenarrays;
}

void
validref(Lextok *p, Lextok *c)
{	Lextok *fp, *tl;
	char lbuf[512];

	for (fp = p->sym->Slst; fp; fp = fp->rgt)
	for (tl = fp->lft; tl; tl = tl->rgt)
		if (strcmp(tl->sym->name, c->sym->name) == 0)
			return;

	sprintf(lbuf, "no field '%s' defined in structure '%s'\n",
		c->sym->name, p->sym->name);
	non_fatal(lbuf, (char *) 0);
}

void
struct_name(Lextok *n, Symbol *v, int xinit, char *buf)
{	Symbol *tl;
	Lextok *tmp;
	char lbuf[512];

	if (!n || !(tl = do_same(n, v, xinit)))
		return;
	tmp = n->rgt->lft;
	if (tmp->sym->type == STRUCT)
	{	strcat(buf, ".");
		struct_name(tmp, tl, 0, buf);
		return;
	}
	sprintf(lbuf, ".%s", tl->name);
	strcat(buf, lbuf);
	if (tmp->sym->nel > 1)
	{	sprintf(lbuf, "[%d]", eval(tmp->lft));
		strcat(buf, lbuf);
	}
}

void
walk2_struct(char *s, Symbol *z)
{	Lextok *fp, *tl;
	char eprefix[128];
	int ix;
	extern void Done_case(char *, Symbol *);

	ini_struct(z);
	if (z->nel == 1)
		sprintf(eprefix, "%s%s.", s, z->name);
	for (ix = 0; ix < z->nel; ix++)
	{	if (z->nel > 1)
			sprintf(eprefix, "%s%s[%d].", s, z->name, ix);
		for (fp = z->Sval[ix]; fp; fp = fp->rgt)
		for (tl = fp->lft; tl; tl = tl->rgt)
		{	if (tl->sym->type == STRUCT)
				walk2_struct(eprefix, tl->sym);
			else if (tl->sym->type == CHAN)
				Done_case(eprefix, tl->sym);
	}	}
}

void
walk_struct(FILE *ofd, int dowhat, char *s, Symbol *z, char *a, char *b, char *c)
{	Lextok *fp, *tl;
	char eprefix[128];
	int ix;

	ini_struct(z);
	if (z->nel == 1)
		sprintf(eprefix, "%s%s.", s, z->name);
	for (ix = 0; ix < z->nel; ix++)
	{	if (z->nel > 1)
			sprintf(eprefix, "%s%s[%d].", s, z->name, ix);
		for (fp = z->Sval[ix]; fp; fp = fp->rgt)
		for (tl = fp->lft; tl; tl = tl->rgt)
		{	if (tl->sym->type == STRUCT)
			 walk_struct(ofd, dowhat, eprefix, tl->sym, a,b,c);
			else
			 do_var(ofd, dowhat, eprefix, tl->sym, a,b,c);
	}	}
}

void
c_struct(FILE *fd, char *ipref, Symbol *z)
{	Lextok *fp, *tl;
	char pref[256], eprefix[256];
	int ix;

	ini_struct(z);

	for (ix = 0; ix < z->nel; ix++)
	for (fp = z->Sval[ix]; fp; fp = fp->rgt)
	for (tl = fp->lft; tl; tl = tl->rgt)
	{	strcpy(eprefix, ipref);
		if (z->nel > 1)
		{	/* insert index before last '.' */
			eprefix[strlen(eprefix)-1] = '\0';
			sprintf(pref, "[ %d ].", ix);
			strcat(eprefix, pref);
		}
		if (tl->sym->type == STRUCT)
		{	strcat(eprefix, tl->sym->name);
			strcat(eprefix, ".");
			c_struct(fd, eprefix, tl->sym);
		} else
			c_var(fd, eprefix, tl->sym);
	}
}

void
dump_struct(Symbol *z, char *prefix, RunList *r)
{	Lextok *fp, *tl;
	char eprefix[256];
	int ix, jx;

	ini_struct(z);

	for (ix = 0; ix < z->nel; ix++)
	{	if (z->nel > 1)
			sprintf(eprefix, "%s[%d]", prefix, ix);
		else
			strcpy(eprefix, prefix);
		
		for (fp = z->Sval[ix]; fp; fp = fp->rgt)
		for (tl = fp->lft; tl; tl = tl->rgt)
		{	if (tl->sym->type == STRUCT)
			{	char pref[256];
				strcpy(pref, eprefix);
				strcat(pref, ".");
				strcat(pref, tl->sym->name);
				dump_struct(tl->sym, pref, r);
			} else
			for (jx = 0; jx < tl->sym->nel; jx++)
			{	if (tl->sym->type == CHAN)
					doq(tl->sym, jx, r);
				else
				{	printf("\t\t");
					if (r)
					printf("%s(%d):", r->n->name, r->pid);
					printf("%s.%s", eprefix, tl->sym->name);
					if (tl->sym->nel > 1)
						printf("[%d]", jx);
					printf(" = ");
					sr_mesg(stdout, tl->sym->val[jx],
						tl->sym->type == MTYPE);
					printf("\n");
		}	}	}
	}
}

static int
retrieve(Lextok **targ, int i, int want, Lextok *n, int Ntyp)
{	Lextok *fp, *tl;
	int j = i, k;

	for (fp = n; fp; fp = fp->rgt)
	for (tl = fp->lft; tl; tl = tl->rgt)
	{	if (tl->sym->type == STRUCT)
		{	j = retrieve(targ, j, want, tl->sym->Slst, Ntyp);
			if (j < 0)
			{	Lextok *x = cpnn(tl, 1, 0, 0);
				x->rgt = nn(ZN, '.', (*targ), ZN);
				(*targ) = x;
				return -1;
			}
		} else
		{	for (k = 0; k < tl->sym->nel; k++, j++)
			{	if (j == want)
				{	*targ = cpnn(tl, 1, 0, 0);
					(*targ)->lft = nn(ZN, CONST, ZN, ZN);
					(*targ)->lft->val = k;
					if (Ntyp)
					(*targ)->ntyp = (short) Ntyp;
					return -1;
				}
	}	}	}
	return j;
}

static int
is_explicit(Lextok *n)
{
	if (!n) return 0;
	if (!n->sym) fatal("unexpected - no symbol", 0);
	if (n->sym->type != STRUCT) return 1;
	if (!n->rgt) return 0;
	if (n->rgt->ntyp != '.')
	{	lineno = n->ln;
		Fname  = n->fn;
		printf("ntyp %d\n", n->rgt->ntyp);
		fatal("unexpected %s, no '.'", n->sym->name);
	}
	return is_explicit(n->rgt->lft);
}

Lextok *
expand(Lextok *n, int Ok)
	/* turn rgt-lnked list of struct nms, into ',' list of flds */
{	Lextok *x = ZN, *y;

	if (!Ok) return n;

	while (n)
	{	y = mk_explicit(n, 1, 0);
		if (x)
			(void) tail_add(x, y);
		else
			x = y;

		n = n->rgt;
	}
	return x;
}

Lextok *
mk_explicit(Lextok *n, int Ok, int Ntyp)
	/* produce a single ',' list of fields */
{	Lextok *bld = ZN, *x;
	int i, cnt; extern int IArgs;

	if (n->sym->type != STRUCT
	||  is_explicit(n))
		return n;

	if (n->rgt
	&&  n->rgt->ntyp == '.'
	&&  n->rgt->lft
	&&  n->rgt->lft->sym
	&&  n->rgt->lft->sym->type == STRUCT)
	{	Lextok *y;
		bld = mk_explicit(n->rgt->lft, Ok, Ntyp);
		for (x = bld; x; x = x->rgt)
		{	y = cpnn(n, 1, 0, 0);
			y->rgt = nn(ZN, '.', x->lft, ZN);
			x->lft = y;
		}

		return bld;
	}

	if (!Ok || !n->sym->Slst)
	{	if (IArgs) return n;
		printf("spin: saw '");
		comment(stdout, n, 0);
		printf("'\n");
		fatal("incomplete structure ref '%s'", n->sym->name);
	}

	cnt = Cnt_flds(n->sym->Slst);
	for (i = cnt-1; i >= 0; i--)
	{	bld = nn(ZN, ',', ZN, bld);
		if (retrieve(&(bld->lft), 0, i, n->sym->Slst, Ntyp) >= 0)
		{	printf("cannot retrieve field %d\n", i);
			fatal("bad structure %s", n->sym->name);
		}
		x = cpnn(n, 1, 0, 0);
		x->rgt = nn(ZN, '.', bld->lft, ZN);
		bld->lft = x;
	}
	return bld;
}

Lextok *
tail_add(Lextok *a, Lextok *b)
{	Lextok *t;

	for (t = a; t->rgt; t = t->rgt)
		if (t->ntyp != ',')
		fatal("unexpected type - tail_add", 0);
	t->rgt = b;
	return a;
}

void
setpname(Lextok *n)
{	UType *tmp;

	for (tmp = Pnames; tmp; tmp = tmp->nxt)
		if (!strcmp(n->sym->name, tmp->nm->name))
		{	non_fatal("proctype %s redefined",
				n->sym->name);
			return;
		}
	tmp = (UType *) emalloc(sizeof(UType));
	tmp->nm = n->sym;
	tmp->nxt = Pnames;
	Pnames = tmp;
}

int
isproctype(char *t)
{	UType *tmp;

	for (tmp = Pnames; tmp; tmp = tmp->nxt)
	{	if (!strcmp(t, tmp->nm->name))
			return 1;
	}
	return 0;
}

#include <u.h>
#include <libc.h>
#include <bio.h>
#include "parl.h"
#include "y.tab.h"
#define Extern
#include "globl.h"

typedef struct tyconst Tyconst;
struct tyconst
{
	int	size;
	int	align;
	char	*rcvfun;
	char	*sndfun;
}tyconst[] = {
	[TXXX]		0,	   0,		   0,		0,
	[TINT]		Machint,   Align_Machint,  "rcvint",	"sndint",
	[TUINT]		Machint,   Align_Machint,  "rcvint",	"sndint",
	[TSINT]		Machsint,  Align_Machsint, "rcvsint",	"sndsint",
	[TSUINT]	Machsint,  Align_Machsint, "rcvsint",	"sndsint",
	[TCHAR]		Machchar,  Align_Machchar, "rcvchar",	"sndchar",
	[TFLOAT]	Machfloat, Align_Machfloat,"rcvflt",	"sndflt",
	[TIND]		Machptr,   Align_Machptr,  "rcvint",	"sndint",
	[TCHANNEL]	Machchan,  Align_Machchan, "rcvint",	"sndint",
	[TARRAY]	0,	   0,		   0,		0,
	[TAGGREGATE]	0,	   Align_Machint,  "rcvmem",	"sndmem",		
	[TUNION]	0,	   Align_Machint,  "rcvmem",	"sndmem",
	[TFUNC]		0,	   0,		   0,		0,
	[TVOID]		0,	   0,		   0,		0,
	[TADT]		0,	   Align_Machint,  "rcvmem",	"sndmem",
};

char *typestr[] =
{
	[TXXX]		"UNRESOLVED",
	[TINT]		"INT",
	[TUINT]		"UINT",
	[TSINT]		"SINT",
	[TSUINT]	"USINT",
	[TCHAR]		"CHAR",
	[TFLOAT]	"FLOAT",
	[TIND]		"*",
	[TCHANNEL]	"CHAN<-",
	[TARRAY]	"[]",
	[TAGGREGATE]	"AGGR",
	[TUNION]	"UNION",
	[TFUNC]		"FUNC()",
	[TVOID]		"VOID",
	[TADT]		"ADT",
};

int
typeconv(void *t, Fconv *f)
{
	char *tag, buf[128];
	Type *type;
	int tn, i;

	type = *((Type**)t);
	i = 0;
	if(type == 0)
		strcpy(buf, "TZ");
	else {
		while(type) {
			tn = type->type;
			if(opt('z'))
				i += sprint(buf+i, "%s(%d)", typestr[tn], type->size);
			else
				i += sprint(buf+i, "%s", typestr[tn]);

			if(tn == TAGGREGATE || tn == TUNION || tn == TADT) {
				tag = "UNNAMED";
				if(type->tag)
					tag = type->tag->name;
				else if(type->sym)
					tag = type->sym->name;
				sprint(buf+i, " %s", tag);
				break;
			}
			type = type->next;
		}
	}

	strconv(buf, f);
	return(sizeof(Type*));
}

/*
 * Print out a complex type with sizes and types
 */
void
prnagun(Type *t, int depth)
{
	Type *m, *n;
	char indent[10];

	if(depth > 5) {
		print("\nRecursive ...\n");
		return;
	}
	memset(indent, ' ', sizeof(indent));
	indent[depth*2] = '\0';

	if(t->tag)
		print("%s'%s' %s", indent, t->tag->name, typestr[t->type]);
	else
		print("%s%s", indent, typestr[t->type]);
		
	print(" (%d %d)\n", t->size, t->offset);
	for(m = t->next; m; m = m->member) {
		print("%s", indent);
		for(n = m; n; n = n->next) {
			if(n->type == TADT || n->type == TUNION || n->type == TAGGREGATE)
				break;

			if(n->tag)
		 		print(" '%s' %s ",
					n->tag->name, typestr[n->type]);
			else
				print(" %s ", typestr[n->type]);
			print("(%d %d) ", n->size, n->offset);
		}
		if(n)
			prnagun(n, depth+1);
		else
			print("\n");
	}
}

/* 
 * Install the basic types
 */
void
typeinit(void)
{
	int i;
	Sym *s;
	Type *t;
	Node *n;
	Tinfo *ti;
	Tyconst *ty;
	char name[32];

	ty = tyconst;
	for(i = 0; i < Ntype; i++) {
		t = abt(i);
		t->size = ty->size;
		t->align = ty->align;
		builtype[i] = t;

		if(ty->rcvfun) {
			sprint(name, "ALEF_%s", ty->rcvfun);
			n = an(ONAME, ZeroN, ZeroN);
			s = lookup(name);
			if(s == 0)
				s = enter(name, Tid);
			n->sym = s;
			ti = malloc(sizeof(Tinfo));
			ti->class = External;
			ti->offset = 0;
			n->ti = ti;
			rfun[i] = n;
		}
		if(ty->sndfun) {
			sprint(name, "ALEF_%s", ty->sndfun);
			n = an(ONAME, ZeroN, ZeroN);
			s = lookup(name);
			if(s == 0)
				s = enter(name, Tid);
			n->sym = s;
			ti = malloc(sizeof(Tinfo));
			ti->class = External;
			ti->offset = 0;
			n->ti = ti;
			sfun[i] = n;
		}
		ty++;
	}
	for(i = 0; i < Ntype; i++) {
		if(rfun[i])
			rfun[i]->t = at(i, 0);
		if(sfun[i])
			sfun[i]->t = at(TFUNC, at(TVOID, 0));
	}
}

/*
 * take the address of func/array automatically
 */
int
tyaddr(Node *n)
{
	Node *new;

	switch(n->t->type) {
	case TFUNC:
		if(chklval(n))
			return 1;

		new = an(0, ZeroN, ZeroN);
		*new = *n;
		n->type = OADDR;
		n->left = new;
		n->t = at(TIND, new->t);
		break;

	case TARRAY:
		if(chklval(n))
			return 1;

		new = an(0, ZeroN, ZeroN);
		*new = *n;
		n->type = OADDR;
		n->left = new;
		n->t = at(TIND, new->t->next);
		break;
	}

	return 0;
}

void
typesu(Node *n)
{
	Type *t;

	t = n->t;
	if(t->type != TXXX)
		return;

	if(t->sym->ltype == ZeroT)
		diag(n, "partially formed complex");
	else
		*t = *t->sym->ltype;
}

void
typesut(Type *t)
{
	if(t->sym)
	if(t->sym->ltype) {
		*t = *(t->sym->ltype);
		return;
	}

	diag(ZeroN, "partially formed complex %s", t->sym->name);
}

/*
 * check for a valid type combination in the supplied table
 */
int
typeval(ulong *bvec, Type *t1, Type *t2)
{
	int ok;
	ulong m;

	/* Same type always ok */
	if(typecmp(t1, t2, 5))
		return 0;

	/* Use the provided table */
	m = 1<<t1->type;
	if(m & bvec[t2->type])
		return 0;

	/* Fix up void pointers */
	ok = 0;
	while(t1->next && t2->next) {
		if(t1->type != TIND || t2->type != TIND)
			break;
		t2 = t2->next;
		t1 = t1->next;
		ok = 1;
	}
	if(ok && (t1->type == TVOID || t2->type == TVOID))
		return 0;

	return 1;
}

int
typecmp(Type *left, Type *right, int d)
{
	int t;

	d--;
	for(;;) {
		if(left == right)
			return 1;

		if(left == ZeroT || right == ZeroT)
			return 0;

		if(left->type == TXXX)
			typesut(left);

		if(right->type == TXXX)
			typesut(right);

		if(left == right)
			return 1;

		if(d < 0)
			return 1;

		t = left->type;
		if(t != right->type)
			return 0;

		left = left->next;
		right = right->next;

		if((1<<t) & MCOMPLEX)
			for(;;) {
				if(left == right)
					return 1;
				if(typecmp(left, right, d) == 0)
					return 0;
				left = left->member;
				right = right->member;
			}

	}
	return 0;
}

void
derivetype(Node *n)
{
	if(n->sym->instance) {
		n->ti = n->sym->instance;
		n->ti->ref = 1;
		n->t = n->sym->instance->t;
	}
}

Type*
mkcast(Type *t, Node *n)
{
	while(n) {
		t = at(TIND, t);
		n = n->left;
	}
	return t;
}

Type *
typecopy(Type *t, int m)
{
	Type *nt;

	nt = at(t->type, 0);
	nt->offset = t->offset;
	nt->class = t->class;
	nt->size = t->size;
	nt->tag = t->tag;
	nt->sym = t->sym;

	if(t->next)
		nt->next = typecopy(t->next, 1);

	if(t->member)
	if(m)
		nt->member = typecopy(t->member, 1);

	return nt;
}

/* 
 * Serach for a tag/type in complex types
 */
Type*
walktype(Type *wt, Node *s, int *o)
{
	char *name;
	int subo;
	Type *t, *f, *n, *lt;

	SET(parent);

	f = ZeroT;
	name = s->sym->name;
	lt = s->sym->ltype;

	if(opt('a')) {
		print("walktype: %T for '%s'\n", wt, name);
		prnagun(wt, 0);
	}

	/* Look for direct names/types */
	for(t = wt->next; t; t = t->member) {
		if((t->tag && strcmp(t->tag->name, name) == 0) ||
		   (lt && typecmp(t, lt, 5))) {
			if(f) {
				diag(s, "ambiguous complex member: %s", name);
				break;
			}
			if(f == ZeroT) {
				/* Check access */
				if(wt->type == TADT && t->class != External)
				if(typecmp(wt, adtbfun, 5) == 0)
					diag(s, "%s is hidden member", name);

				f = t;
				parent = wt;
				*o += f->offset;
			}
		}
	}

	if(f)
		return f;

	/* Look for unnamed complex types */
	subo = 0;
	for(t = wt->next; t; t = t->member) {
		if(t->tag == 0) {
			n = walktype(t, s, &subo);
			if(n == 0)
				continue;
			if(f == ZeroT) {
				f = n;
				parent = t;
				*o += t->offset + subo;
			}
			else {
				diag(s, "ambiguous unnamed complex member: %s", name);
				break;
			}
		}
	}
	return f;
}

int
chklval(Node *n)
{
	if(n->islval)
		return 0;

	diag(n, "not an l-value");
	return 1;
}

/*
 * typecheck structure dereferencing
 */
int
tydot(Node *n)
{
	Type *t;
	char *sname;
	Node *l, *new1, *new2;
	int offset, tn;

	sname = n->sym->name;
	l = n->left;
	tn = l->t->type;
	switch(tn) {
	default:
		diag(n, "incompatible type %T.%s", l->t, sname);
		return 1;

	case TAGGREGATE:
	case TUNION:
	case TADT:
		break;
	}

	offset = 0;
	t = walktype(l->t, n, &offset);
	if(t == ZeroT) {
		diag(n, "%s not a member of '%T'", sname, l->t);
		return 1;
	}
	if(t->type == TFUNC)
		t = parent;

	if(opt('o')) {
		print("%s offset %d\n", sname, offset);
		prnagun(l->t, 0);
	}

	n->islval = l->islval;
	if(n->islval == 0) {
		new1 = con(offset);
		n->right = new1;
		n->t = t;

		return 0;
	}
	n->left->t = t;
	if(offset == 0) {
		*n = *n->left;
		return 0;
	}
	n->t = t;

	new1 = con(offset);

	new2 = an(OADDR, n->left, ZeroN);
	new2->t = at(TIND, t);
	new2->t->size = builtype[TIND]->size;

	new1 = an(OADD, new1, new2);
	new1->t = new2->t;

	n->type = OIND;
	n->left = new1;
	n->right = ZeroN;

	return 0;
}

/* 
 * rewrite <-=
 */
int
tysend(Node *n)
{
	Type *chant;
	Node *l, *r, *c, *size;

	l = n->left;
	r = n->right;

	*l = *(l->left);
	if(typechk(l, 0))
		return 1;

	if(l->t->type != TCHANNEL) {
		diag(n, "must send on a chan: '%T'", l->t);
		return 1;
	}

	if(typechk(r, 0))
		return 1;

	chant = l->t->next;

	/* Check legality */
	if(typeval(typeasgn, chant, r->t)) {
		diag(n, "incompatible types: '%T' <- '%T'", chant, r->t);
		return 1;
	}

	switch(r->t->type) {
	case TADT:
	case TAGGREGATE:
	case TUNION:
		size = con(r->t->size);
		c = an(0, ZeroN, ZeroN);
		*c = *r;
		r->left = c;
		r->right = ZeroN;
		r->t = at(TIND, c->t);
		r->type = OADDR;
		r = an(OLIST, r, size);
		c = an(OCALL, sfun[chant->type], an(OLIST, l, r));
		c->t = l->t->next->next;
		*n = *c;
		break;

	default:
		/* Insert the cast */
		if(typecmp(chant, r->t, 5) == 0) {
			r = an(OCONV, r, ZeroN);
			r->t = chant;
		}
		c = an(OLIST, l, r);
		n->type = OCALL;
		n->left = sfun[chant->type];
		n->right = c;
		break;
	}
	n->t = builtype[TVOID];
	return 0;
}

/*
 * rewrite a proc call from  func(arg1, arg2) into ALEF_proc(&func, framesize, arg1 ...)
 */
void
typroc(Node *n, Node *runtime)
{
	Node *rf;
	Node *fsize;
	Node *narg;

	rf = n->left;		/* real function */
	n->left = runtime;
	n->left->t = runtime->t;

	if(rf->type != OIND)
		fatal("typroc");

	frsize = 0;
	framesize(n->right);
	fsize = con(frsize);

	narg = an(OLIST, rf->left, fsize);
	if(n->right == ZeroN)
		n->right = narg;
	else
		addarg(n, narg);
}

int
tyalloc(Node *n)
{
	Node *alf;
	int t, sz;
	Node *cons, *n1;

	if(n == ZeroN)
		return 0;

	switch(n->type) {
	case OLIST:
		if(tyalloc(n->left) || tyalloc(n->right))
			return 1;
		return 0;

	default:
		if(typechk(n, 0))
			break;
		if(chklval(n))
			return 1;

		t = n->t->type;
		sz = n->t->next->size;
		alf = allocnode;
		if(t == TCHANNEL) {
			alf = challocnode;
			cons = an(OLIST, con(n->t->nbuf), con(sz));
		}
		else
		if(t == TIND)
			cons = con(sz);
		else
			break;

		if(sz == 0) {
			diag(n, "alloc needs to know sizeof %T", n->t);
			return 1;
		}

		n1 = an(0, ZeroN, ZeroN);
		*n1 = *n;
		n->type = OASGN;
		n->t = builtype[TIND];
		n->left = n1;
		n->right = an(OCALL, alf, cons);
		n->right->t = n->t;		
		return 0;
	}
	diag(n, "incompatible type for alloc");
	return 1;
}

int
tyunalloc(Node *n)
{
	int t;
	Node *alf, *n1;

	if(n == ZeroN)
		return 0;

	switch(n->type) {
	case OLIST:
		if(tyunalloc(n->left) || tyunalloc(n->right))
			return 1;
		return 0;

	default:
		if(typechk(n, 0))
			break;

		t = n->t->type;
		alf = unallocnode;
		if(t == TCHANNEL)
			alf = chunallocnode;
		else
		if(t != TIND)
			break;


		n1 = an(0, ZeroN, ZeroN);
		*n1 = *n;
		n->left = alf;
		n->right = n1;

		n->type = OCALL;
		n->t = builtype[TVOID];		
		return 0;
	}
	diag(n, "incompatible type for unalloc: %T %s", n->t, n->sym->name);
	return 1;
}

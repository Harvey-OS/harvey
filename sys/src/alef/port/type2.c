#include <u.h>
#include <libc.h>
#include <bio.h>
#include "parl.h"
#include "y.tab.h"
#define Extern extern
#include "globl.h"

typedef struct tyconst Tyconst;
struct tyconst
{
	int	type;
	int	size;
	int	align;
	char	*rcvfun;
	char	*sndfun;
}tyconst[] = {
	TXXX,		0,	   0,		   0,		0,
	TINT,		Machint,   Align_Machint,  "rcvint",	"sndint",
	TUINT,		Machint,   Align_Machint,  "rcvint",	"sndint",
	TSINT,		Machsint,  Align_Machsint, "rcvsint",	"sndsint",
	TSUINT,		Machsint,  Align_Machsint, "rcvsint",	"sndsint",
	TCHAR,		Machchar,  Align_Machchar, "rcvchar",	"sndchar",
	TFLOAT,		Machfloat, Align_Machfloat,"rcvflt",	"sndflt",
	TIND,		Machptr,   Align_Machptr,  "rcvint",	"sndint",
	TCHANNEL,	Machchan,  Align_Machchan, "rcvint",	"sndint",
	TARRAY,		0,	   0,		   0,		0,
	TAGGREGATE,	0,	   Align_Machint,  "rcvmem",	"sndmem",	
	TUNION,		0,	   Align_Machint,  "rcvmem",	"sndmem",
	TFUNC,		0,	   0,		   0,		0,
	TVOID,		0,	   0,		   0,		0,
	TADT,		0,	   Align_Machint,  "rcvmem",	"sndmem",
	TPOLY,		2*Machint, Align_Machint,  "rcvmem",	"sndmem",
};

char *typestr[] =
{
	[TXXX]		"UNRESOLVED",
	[TINT]		"INT",
	[TUINT]		"UINT",
	[TSINT]		"SINT",
	[TSUINT]	"USINT",
	[TCHAR]		"BYTE",
	[TFLOAT]	"FLOAT",
	[TIND]		"*",
	[TCHANNEL]	"CHAN<-",
	[TARRAY]	"[]",
	[TAGGREGATE]	"AGGR",
	[TUNION]	"UNION",
	[TFUNC]		"FUNC()",
	[TVOID]		"VOID",
	[TADT]		"ADT",
	[TPOLY]		"POLY",
};

int
issend(Node *n)
{
	int i;

	if(n->type != ONAME)
		return 0;

	for(i = 0; i < Ntype; i++)
		if(sfun[i] == n)
			return 1;

	return 0;
}

int
telem(int i, Type *t, char *buf)
{
	int tn;

 	if(t->type == TXXX && t->sym && t->sym->ltype)
		*t = *(t->sym->ltype);

	tn = t->type;

	if(opt('z'))
		i += sprint(buf+i, "%s(%d)", typestr[tn], t->size);

	if(tn == TARRAY && t->next && t->next->size != 0)
		i += sprint(buf+i, "[%d]", t->size/t->next->size);
	else
	if(tn == TPOLY)
		i += sprint(buf+i, "%s<%s>", typestr[tn], t->sym->name);
	else
		i += sprint(buf+i, "%s", typestr[tn]);

	if(tn != TAGGREGATE && tn != TUNION && tn != TADT)
		return i;

	if(t->tag)
		i += sprint(buf+i, " %s", t->tag->name);
	else
	if(t->sym)
		i += sprint(buf+i, " %s", t->sym->name);
	else
		i += sprint(buf+i, " %t", t);
		
	return i;
}

int
tvarp(int i, Type *t, char *buf, char *paren)
{
	buf[i++] = paren[0];
	while(t) {
		i = telem(i, t, buf);
		t = t->variant;
		if(t) {
			buf[i++] = ',';
			buf[i++] = ' ';
		}
	}
	buf[i++] = paren[1];
	buf[i] = '\0';
	return i;
}

int
typeconv(void *type, Fconv *f)
{
	Type *t;
	int i, n;
	char buf[1024];

	t = *((Type**)type);

	if(t == 0) {
		strconv("TZ", f);
		return(sizeof(Type*));
	}

	i = 0;
	while(t) {
		if(t->variant && f->chr != 'V')
			i = tvarp(i, t, buf, "()");
		else
			i = telem(i, t, buf);
		n = t->type;
		if(n == TAGGREGATE || n == TADT || n == TUNION) {
			if(t->param)
				tvarp(i, t->param, buf, "[]");
			break;
		}
		t = t->next;
	}
	strconv(buf, f);
	return(sizeof(Type*));
}

int
tconv(void *tx, Fconv *f)
{
	int i;
	Sym *s;
	Type *t;
	char buf[1024];

	t = *((Type**)tx);

	s = ltytosym(t);
	if(s != 0)
		strconv(s->name, f);
	else {	
		t = t->next;
		i = 1;
		buf[0] = '(';
		while(t) {
			if(t->member)
				i += sprint(buf+i, "%T, ", t);
			else
				i += sprint(buf+i, "%T", t);
			t = t->member;
		}
		buf[i++] = ')';
		buf[i] = '\0';

		strconv(buf, f);
	}
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
			if(MCOMPLEX & (1<<n->type))
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
	Node *n;
	Type *t;
	Tyconst *ty;
	char name[32];

	ty = tyconst;
	for(i = 0; i < Ntype; i++) {
		t = abt(ty->type);
		t->size = ty->size;
		t->align = ty->align;
		builtype[ty->type] = t;

		if(ty->rcvfun) {
			sprint(name, "ALEF_%s", ty->rcvfun);
			n = an(ONAME, nil, nil);
			s = lookup(name);
			if(s == 0)
				s = enter(name, Tid);
			n->sym = s;
			n->t = abt(ty->type);
			n->ti = ati(n->t, Global);
			rfun[ty->type] = n;
		}
		if(ty->sndfun) {
			sprint(name, "ALEF_%s", ty->sndfun);
			n = an(ONAME, nil, nil);
			s = lookup(name);
			if(s == 0)
				s = enter(name, Tid);
			n->sym = s;
			n->t = abt(TFUNC);
			n->t->next = abt(TVOID);
			n->ti = ati(n->t, Global);
			sfun[ty->type] = n;
		}
		ty++;
	}

	t = at(TINT, 0);
	t->offset = Machptr;
	t->tag = polysig;
	polyshape = at(TVOID, 0);
	polyshape = at(TIND, polyshape);
	polyshape->member = t;
	polyshape->tag = polyptr;
	polyshape = at(TAGGREGATE, polyshape);
	polyshape->size = builtype[TPOLY]->size;

	alefsig = rtnode("ALEF_SIG", nil, at(TARRAY, builtype[TCHAR]));
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

		new = an(0, nil, nil);
		*new = *n;
		n->type = OADDR;
		n->islval = 0;
		n->left = new;
		n->t = at(TIND, new->t);
		break;

	case TARRAY:
		if(chklval(n))
			return 1;

		new = an(0, nil, nil);
		*new = *n;
		n->type = OADDR;
		n->islval = 0;
		n->left = new;
		n->t = at(TIND, new->t->next);
		break;
	}

	return 0;
}

void
typesut(Type *t)
{
	if(t->sym && t->sym->ltype) {
		*t = *(t->sym->ltype);
		return;
	}

	diag(nil, "partially formed complex %s", t->sym->name);
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
vcmp(Type *l, Type *r)
{
	for(;;) {
		if(l == r)
			return 1;

		if(typecmp(l, r, 5) == 0)
			return 0;

		l = l->variant;
		r = r->variant;
	}
	return 1;
}

int
typecmp(Type *l, Type *r, int d)
{
	int t;

	d--;
	for(;;) {
		if(l == r)
			return 1;

		if(l == nil || r == nil)
			return 0;

		if(l->type == TXXX)
			typesut(l);
		if(r->type == TXXX)
			typesut(r);

		if(l == r)
			return 1;

		if(d < 0)
			return 1;

		t = l->type;
		if(t != r->type)
			return 0;

		if(t == TPOLY && l->sym != r->sym)
			return 0;

		l = l->next;
		r = r->next;

		switch(t) {
		case TADT:
		case TUNION:
		case TAGGREGATE:
			for(;;) {
				if(l == r)
					return 1;

				if(typecmp(l, r, d) == 0)
					return 0;

				l = l->member;
				r = r->member;
			}
			break;
		case TCHANNEL:
			if(vcmp(l->variant, r->variant) == 0)
				return 0;
			break;
		}
	}
	return 0;
}

void
derivetype(Node *n)
{
	if(n->sym->instance == nil)
		return;

	n->ti = n->sym->instance;
	n->ti->ref = 1;
	n->t = n->sym->instance->t;
	if(n->t->subst != nil)
		n->t = tmcpy(n->t);
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

Type*
polysub(Type *pl, Type *t)
{
	Type *nt, *f;

	if(t == nil)
		return nil;

	nt = abt(TXXX);
	*nt = *t;
	if(t->type == TPOLY) {
		for(f = pl; f; f = f->variant) {
			if(f->sym == t->sym) {
				*nt = *f;
				nt->size = t->size;
				nt->variant = 0;
				nt->subst = t;
				return nt;
			}
		}
		if(!f)
			fatal("polysub");
	}
	else
		*nt = *t;

	nt->next = polysub(pl, t->next);
	return nt;	
}

/* 
 * Serach for a tag/type offset in a complex type
 */
Type*
walktype(Type *wt, Node *s, int *o)
{
	int subo;
	char *name;
	Type *t, *f, *n, *lt;
	static char e1[] = "ambiguous complex member: %s";
	static char e2[] = "ambiguous unnamed complex member: %s";

	SET(parent);

	f = nil;
	name = s->sym->name;
	lt = s->sym->ltype;

	if(opt('a'))
		print("walktype: %T for '%s'\n", wt, name);

	/*
	 * Look for direct reference by type in type/unamed tag which is
	 * the highest precedence
	 */
	for(t = wt->next; t; t = t->member) {
		if(t->tag == 0 && lt && typecmp(t, lt, 5)) {
			if(f) {
				diag(s, e1, name);
				break;
			}
			/*
			 * Check access
			 */
			if(wt->type == TADT && t->class != External)
			if(typecmp(wt, adtbfun, 5) == 0)
				diag(s, "%s is a hidden member", name);

			f = t;
			parent = wt;
			*o += f->offset;
		}
	}

	if(f) {
		if(wt->param)
			f = polysub(wt->param, f);
		return f;
	}
	/*
	 * Look for direct tag/type
	 */
	for(t = wt->next; t; t = t->member) {
		if(t->tag && strcmp(t->tag->name, name) == 0) {
			if(f) {
				diag(s, e1, name);
				break;
			}
			if(wt->type == TADT && t->class != External)
			if(typecmp(wt, adtbfun, 5) == 0)
				diag(s, "%s is a hidden member", name);

			f = t;
			parent = wt;
			*o += f->offset;
		}
	}

	if(f) {
		if(wt->param)
			f = polysub(wt->param, f);
		return f;
	}

	/*
	 * Look for unnamed complex types by walking recursively through the
	 * type.
	 */
	subo = 0;
	for(t = wt->next; t; t = t->member) {
		if(t->tag == 0) {
			n = walktype(t, s, &subo);
			if(n) {
				if(opt('a'))
					print("found %T\n", n);
				if(f) {
					diag(s, e2, name);
					break;
				}
				f = n;
				*o += t->offset + subo;
			}
		}
	}
	if(wt->param)
		f = polysub(wt->param, f);
	return f;
}

/*
 * typecheck structure dereferencing
 */
int
tydot(Node *n)
{
	Sym *s;
	Type *t;
	char *sname;
	char buf[Strsize];
	int offset, tn;
	Node *l, *new1, *new2;

	line = n->srcline;

	l = n->left;

	if(typechk(l, 0))
		return 1;

	sname = n->sym->name;

	tn = l->t->type;
	switch(tn) {
	default:
		diag(n, "incompatible type: %T.%s", l->t, sname);
		return 1;

	case TAGGREGATE:
	case TUNION:
	case TADT:
		break;
	}

	offset = 0;
	t = walktype(l->t, n, &offset);
	if(t == nil) {
		diag(n, "%s not a member of %T", sname, l->t);
		return 1;
	}

	if(opt('o')) {
		print("%s offset %d\n", sname, offset);
		prnagun(l->t, 0);
	}

	if(t->type == TFUNC) {
		s = ltytosym(parent);
		if(s == ZeroS)
			fatal("tydot");
		sprint(buf, "%s_%s", s->name, t->tag->name);
		*n = *rtnode(buf, nil, t);
		return 0;
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

	new2 = an(OADDR, n->left, nil);
	new2->t = at(TIND, t);
	new2->t->size = builtype[TIND]->size;

	new1 = con(offset);
	new1 = an(OADD, new1, new2);
	new1->t = new2->t;

	n->type = OIND;
	n->left = new1;
	n->right = nil;

	return 0;
}

/* 
 * rewrite <-=
 */
int
tysend(Node *n)
{
	Type *chant;
	Node *l, *r, *c, *size, *signature;

	line = n->srcline;

	l = n->left;
	r = n->right;

	*l = *(l->left);
	if(typechk(l, 0))
		return 1;

	if(l->t->type != TCHANNEL) {
		diag(n, "must send on a channel %T", l->t);
		return 1;
	}

	if(typechk(r, 0))
		return 1;

	/*
	 * Search for match in variant types
	 */
	for(chant = l->t->next; chant; chant = chant->variant)
		if(typeval(typeasgn, chant, r->t) == 0)
			break;

	if(chant == nil) {
		diag(n, "incompatible types: %T <-= %T", l->t->next, r->t);
		return 1;
	}

	signature = con(typesig(chant));

	if(MCOMPLEX & (1<<r->t->type)) {
		size = con(r->t->size);
		c = an(0, nil, nil);
		*c = *r;
		r->left = c;
		r->right = nil;
		r->t = at(TIND, c->t);
		r->type = OADDR;
		size = an(OLIST, size, signature);
		r = an(OLIST, r, size);
		c = an(OCALL, sfun[chant->type], an(OLIST, l, r));
		c->t = l->t->next->next;
		n->type = OSEND;
		n->left = c;
	}
	else {
		/* Insert the cast */
		if(typecmp(chant, r->t, 5) == 0) {
			r = an(OCONV, r, nil);
			r->t = chant;
		}
		r = an(OLIST, r, signature);
		c = an(OLIST, l, r);
		n->type = OSEND;
		n->left = an(OCALL, sfun[chant->type], c);
		n->left->t = builtype[TVOID];
	}
	return 0;
}

/*
 * rewrite a proc call from  func(arg1, arg2) into
 * ALEF_proc(&func, framesize, arg1 ...)
 */
void
typroc(Node *n, Node *runtime)
{
	Node *rf, *fsize, *narg;

	rf = n->left;		/* real function */
	n->left = runtime;
	n->left->t = runtime->t;

	if(rf->type != OIND)
		fatal("typroc");

	frsize = 0;
	framesize(n->right);
	fsize = con(frsize);

	narg = an(OLIST, rf->left, fsize);
	if(n->right == nil)
		n->right = narg;
	else
		addarg(n, narg);
}

int
tyalloc(Node *n)
{
	int sz;
	Type *vt;
	Node *alf, *cons;

	if(n == nil)
		return 0;

	line = n->srcline;

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

		switch(n->t->type) {
		default:
			diag(n, "bad type for alloc %T", n->t);
			return 1;
		case TCHANNEL:
			alf = challocnode;
			/* Size of largest member of variant set */
			sz = 0;
			for(vt = n->t->next; vt; vt = vt->variant)
				if(vt->size > sz)
					sz = vt->size;
			cons = an(OLIST, con(n->t->nbuf), con(sz));
			break;
		case TIND:
			alf = allocnode;
			sz = n->t->next->size;
			cons = con(sz);
			break;
		}

		if(sz == 0) {
			diag(n, "alloc cannot size %T", n->t);
			return 1;
		}

		n->left = dupn(n);
		n->type = OASGN;
		n->t = at(TIND, builtype[TVOID]);
		n->right = an(OCALL, dupn(alf), cons);
		n->right->t = n->t;
		return 0;
	}
	return 1;
}

int
tyunalloc(Node *n)
{
	char *s;
	Node *alf, *n1;

	if(n == nil)
		return 0;

	line = n->srcline;

	switch(n->type) {
	case OLIST:
		if(tyunalloc(n->left) || tyunalloc(n->right))
			return 1;
		break;
	default:
		if(typechk(n, 0))
			break;

		alf = unallocnode;
		n1 = an(OTCHKED, dupn(n), nil);
		switch(n->t->type) {
		case TCHANNEL:
			alf = chunallocnode;
			break;
		case TPOLY:
			n1->left->t = polyshape;
			n1 = an(ODOT, n1, nil);
			n1->sym = polyptr;
			break;
		case TIND:
			break;
		default:
			s = "";
			if(n->sym && n->sym->name)
				s = n->sym->name;
				diag(n, "bad type for unalloc: %T %s", n->t, s);
			return 1;
		}
		n->left = dupn(alf);
		n->right = n1;
		n->type = OCALL;
		n->t = builtype[TVOID];		
		if(typechk(n, 0))
			fatal("unalloc");
	}
	return 0;
}

int
tyvasgn(Node *n)
{
	Type *t;
	Node *l, *r;

	l = n->left;
	r = n->right;

	if(typechk(l, 0))
		return 1;
	if(chklval(l))
		return 1;
	if(typechk(r, NOUNBOX))
		return 1;

	if(r->t->subst != nil) {
		t = r->t;
		t->subst = nil;
		r->t = polyshape;
		r = an(OTCHKED, r, nil);
		r = an(ODOT, r, nil);
		r->sym = polyptr;
		r = an(OCONV, r, nil);
		r->t = at(TIND, t);
		r = an(OIND, r, nil);
		r->t = t;

		if(typeval(typeasgn, l->t, r->t)) {
			diag(n, "incompatible types: %T := %T", l->t, r->t);
			return 1;
		}

		if(typecmp(l->t, r->t, 5) == 0) {
			r = an(OCONV, r, nil);
			r->t = l->t;
		}

		n->right = r;
		n->type = OASGN;
		n->t = l->t;
		if(typechk(r, 0))
			fatal("tyvasgn");
		return 0;
	}

	if(l->t->type == TPOLY && r->t->type == TPOLY && l->t->sym == r->t->sym) {
		diadicpoly(n, "AS");
		return 0;
	}

	diag(n, "incompatible types: %T := %T", l->t, r->t);
	return 1;
}

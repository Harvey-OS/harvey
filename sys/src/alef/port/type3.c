#include <u.h>
#include <libc.h>
#include <bio.h>
#include "parl.h"
#include "y.tab.h"
#define Extern extern
#include "globl.h"

static Type **agtail;
static Type *sptr;
static Type *tu;

void
applyarray(Node *n, Type *t)
{
	int sz;
	char *s;
	Node *f, *l;

	s = n->sym->name;

	for(f = n->left; f; f = f->right) {
		sz = 0;
		if(f->left) {
			l = f->left;
			if(l->t == nil || ((1<<l->t->type)&MINTEGRAL) == 0) {
				diag(n, "%s array size not integral type", s);
				break;
			}
			if(l->type != OCONST) {
				diag(n, "%s[] must have constant size", s);
				break;
			}
			sz = l->ival;
		}
		if(sz < 0 ||(sz == 0 && !n->init && t->class != External)) {
			diag(n, "%s[%d] must be positive constant", s, sz);
			break;
		}
		t = at(TARRAY, t);
		t->size = t->next->size*sz;
	}
	n->t = t;
}

Node*
argproto(Type *tbasic, Node *indsp, Node *arrayspec)
{
	Node *n;
	Sym s;

	n = an(OPROTO, nil, nil);
	if(arrayspec) {
		s.name = "PROTO";
		n->sym = &s;
		n->left = arrayspec;
		applyarray(n, mkcast(tbasic, indsp));
		n->t->type = TIND;
		n->t->size = builtype[TIND]->size;
	}
	else
		n->t = mkcast(tbasic, indsp);

	return n;
}
/* 
 * recurse through the declaration list attaching types to symbols and tags
 */
void
applytype(Type *tbasic, Node *n)
{
	char class;
	Node *f, *proto, *p;

	if(n == nil)
		return;

	proto = nil;

	switch(n->type) {
	default:
		applytype(tbasic, n->left);
		applytype(tbasic, n->right);
		break;

	case OPROTO:
		diag(n, "illegal argument list");
		break;

	case OFUNC:
		n->type = ONAME;
		proto = n->left;
		n->left = nil;

		/* Fall through */
	case ONAME:
		class = tbasic->class;

		for(f = n->right; f; f = f->left)		/* OIND */
			tbasic = at(TIND, tbasic);

		applyarray(n, tbasic);

		/* Rearrange for pointer to function declaration */
		if(proto) {
			n->t = at(TFUNC, n->t);
			p = an(ONAME, n->proto, nil);
			p->sym = n->sym;
			p->t = n->t;
			n->proto = p;
			n->t->proto = p;
			for(f = proto->right; f; f = f->left)
				n->t = at(TIND, n->t);

			if(proto->left) {
				n->left = proto->left;
				applyarray(n, n->t);
			}
		}
		n->t->class = class;
		if(class == Private)
		if(((1<<n->t->type) & (MINTEGRAL|MIND)) == 0)
			diag(n, "bad storage class for type %T", n->t);
		break;
	}
}

Type*
tmcpy(Type *t)
{
	Type *new;

	new = abt(t->type);
	new->next = t->next;
	new->size = t->size;
	new->align = t->align;
	new->member = 0;
	new->class = t->class;
	new->sym = t->sym;
	new->tag = t->tag;
	new->nbuf = t->nbuf;
	new->variant = t->variant;
	new->param = t->param;
	new->subst = t->subst;

	return new;
}

void
agunshape(Node *n)
{
	Type *t;

	if(n == nil)
		return;

	switch(n->type) {
	default:
		agunshape(n->left);
		agunshape(n->right);
		return;

	case OTYPE:
		t = tmcpy(n->t);
		t->tag = 0;			/* Unnamed so kill tag */
		break;

	case OAGDECL:
	case OUNDECL:
	case OADTDECL:
		t = tmcpy(n->t);
		if(t->tag == 0)
			t->tag = n->sym;
		break;

	case ONAME:
		t = tmcpy(n->t);
		t->tag = n->sym;
		break;
	}	
	*agtail = t;
	agtail = &t->member;
}

Type*
unshape(Type *t)
{
	Type *m;
	ulong offset;
	
	t = at(TAGGREGATE, t);

	offset = 0;
	for(m = t->next; m; m = m->member) {
		m->class = External;
		offset = align(offset, m);
		m->offset = offset;
		offset += m->size;
	}
	t->size = align(offset, builtype[TINT]);

	if(opt('C')) {
		print("unshape:\n");
		prnagun(t, 0);
	}
	return t;
}

int
ispoly(Type *t)
{
	while(t) {
		if(t->type == TIND)
			break;
		if(t->type == TFUNC && t->next->type != TPOLY)
			break;
		if(t->type == TPOLY)
			return 1;
		t = t->next;
	}
	return 0;		
}

/*
 * Build a type structure for a complex
 */
void
buildtype(Node *n)
{
	Node *f;
	ulong offset;
	Sym *sym, *tag;
	Type *t, *m, *s;
	char *sn, *tn, *e, *e1;

	if(n->type ==  OSETDECL)
		return;

	f = n->left;
	sym = ZeroS;
	if(f->left)
		sym = f->left->sym;

	tag = ZeroS;
	if(f->right)
		tag = f->right->sym;

	switch(n->type) {
	default:
		fatal("buildtype %N", n);

	case OAGDECL:
		t = at(TAGGREGATE, 0);
		break;

	case OADTDECL:
		t = at(TADT, 0);
		break;

	case OUNDECL:
		t = at(TUNION, 0);
		break;
	}		

	/* Recurse through */
	agtail = &t->next;
	agunshape(n->right);
	n->t = t;


	/* Tag the parse tree with the name */
	if(sym) {
		if(sym->ltype)
		if(sym->ltype->type != TXXX)
			diag(n, "%s redeclared type specifier %T", sym->name, sym->ltype);

		sym->ltype = t;
	}

	/* pick up the tag if this is a sub of a complex type */
	if(tag)
		t->tag = tag;
	else
		t->tag = sym;

	/* propagate sizes and align structure members */
	offset = 0;
	for(m = t->next; m; m = m->member) {
		if(m->size == 0 && m->type != TFUNC) {
			e = "UNNAMED";
			e1 = e;
			if(t->tag)
				e = t->tag->name;
			if(m->tag)
				e1 = m->tag->name;
			diag(n, "%s member %s is of unknown type", e, e1);
			s = at(TINT, 0);
			s->member = m->member;
			*m = *s;
		}
		if(m->class == Global)
			m->class = External;

		/* Type derives something polymorphic */
		m->poly = ispoly(m);
		switch(t->type) {
		case TAGGREGATE:
			m->class = External;
			/* Fall through */

		case TADT:
			if(m->class == Adtdeflt) {
				m->class = Internal;
				if(m->type == TFUNC)
					m->class = External;
			}
			if(m->poly)
				t->dcltree = n;
			if(m->type == TFUNC)
				break;

			offset = align(offset, m);
			m->offset = offset;
			offset += m->size;
			break;

		case TUNION:
			m->class = External;
			if(m->poly)
				diag(n, "polymorphic union type illegal");
			if(m->size > offset)
				offset = m->size;
			m->offset = 0;
			break;
		}
	}
	/* Align the tail to integral machine type */
	t->size = align(offset, builtype[TINT]);
	if(t->size == 0 && t->type == TADT)
		t->size = builtype[TINT]->size;

	if(sym && (acid == 1 || (acid > 1 && inbase)))
		typerec(n, sym->ltype);

	if(opt('C')) {
		sn = "unnamed";
		tn = sn;
		if(sym)
			sn = sym->name;
		if(tag)
			tn = tag->name;
		print("COMPLEX %s.%s\n", sn, tn);
		prnagun(t, 0);
		print("*\n");
	}
}

static
void
covertype(Node *n)
{
	Type *t;

	if(n == nil)
		return;

	switch(n->type) {
	case OLIST:
		covertype(n->left);
		covertype(n->right);
		break;
	default:
		t = at(n->t->type, n->t->next);
		t->size = n->t->size;
		*agtail = t;
		agtail = &t->member;
	}
}

void
tupleof(Node *n)
{
	Type *t, *m;
	ulong offset;

	t = n->t;
	agtail = &t->next;

	covertype(n->left);

	offset = 0;
	for(m = t->next; m; m = m->member) {
		m->class = External;
		offset = align(offset, m);
		m->offset = offset;
		offset += m->size;
	}
	t->size = align(offset, builtype[TINT]);

	if(opt('C')) {
		print("shapeof:\n");
		prnagun(t, 0);
	}
}

static
int
mkasgn(Node *n, ulong off)
{
	Node *r, *l;
	Type *svsptr;

	line = n->srcline;

	if(n == nil)
		return 0;

	if(sptr == 0) {
		diag(n, "tuples are incompatible %t", tu);
		return 1;
	}

	switch(n->type) {
	case OLIST:
		if(mkasgn(n->left, off) || mkasgn(n->right, off))
			return 1;
		break;
	default:
		/*
		 * nil in the l-val means skip the assignment
		 */
		if(n->type == OCONST && n->ival == 0) {
			n->type = OLIST;
			n->left = nil;
			n->right = nil;
			n->t = nil;
			sptr = sptr->member;
			break;
		}

		if(typechk(n, 0))
			return 1;

		if(n->type == OILIST) {
			n->type = OLIST;
			svsptr = sptr;
			if(n->t->type != TAGGREGATE) {
				diag(n, "tuple must be aggregate %T", n->t);
				return 1;
			}
			sptr = n->t->next;
			if(mkasgn(n->left, off+svsptr->offset))
				return 1;
			if(sptr != nil) {
				diag(n, "tuple is incompatible %t", tu);
				return 1;
			}
			sptr = svsptr->member;
			return 0;		
		}

		if(chklval(n))
			return 1;

		l = an(0, nil, nil);
		*l = *n;

		n->type = OASGN;
		n->left = l;

		/* OREG is a placeholder until we compute $rhs in genexp */
		r = an(OREGISTER, nil, nil);
		r->t = sptr;
		r = an(OADD, con(off+sptr->offset), r);
		r->t = at(TIND, sptr);
		r = an(OIND, r, nil);
		r->t = sptr;

		n->right = r;

		if(typeval(typeasgn, l->t, sptr)) {
			diag(n, "incompatible types: %T = %T", l->t, sptr);
			return 1;
		}

		/* Insert the cast */
		if(typecmp(l->t, sptr, 5) == 0) {
			n->right = an(OCONV, r, nil);
			n->right->t = l->t;
		}
		n->t = l->t;
		sptr = sptr->member;
		/* Skip function declarations in an adt */
		while(sptr && sptr->type == TFUNC)
			sptr = sptr->member;
		break;
	}
	return 0;
}

int
tupleasgn(Node *n)
{
	Node *r;
	r = n->right;

	if(typechk(r, 0))
		return 1;

	if(r->t->type != TAGGREGATE) {
		diag(n, "tuple must be aggregate %T", r->t);
		return 1;
	}

	n->t = r->t;
	tu = n->t;
	sptr = tu->next;

	if(mkasgn(n->left->left, 0))
		return 1;

	if(sptr != nil) {
		diag(n, "tuple is incompatible %t", tu);
		return 1;
	}

	return 0;
}

ulong
tsigacc(Type *t, ulong sig, int depth)
{
	if(t == nil || depth > 10)
		return sig;

	while(t) {
		sig = tsigacc(t->member, sig, depth+1);
		sig = tsigacc(t->variant, sig, depth+1);
		sig = tsigacc(t->param, sig, depth+1);
		sig = sig*13 + t->type;
		t = t->next;
	}

	return sig;
}

ulong
typesig(Type *t)
{
	Sym *s;
	char *c;
	ulong sig;

	sig = 0;
	s = ltytosym(t);
	if(s != 0) {
		c = s->name;
		while(*c)
			sig = sig*7 + *c++;
	}
	sig = tsigacc(t, sig, 0);

	if(opt('S'))
		print("HASH #%.8luX %V\n", sig, t);

	return sig;
}

void
mkdynt(Node *sn, Type *f)
{
	ulong s;
	Dynt *h, **l;

	s = typesig(f);
	l = &dynhash[s%NDYHASH];
	for(h = *l; h; h = h->hash)
		if(h->sig == s)
			return;

	h = malloc(sizeof(Dynt));
	h->sig = s;
	h->t = f;
	h->n = sn;
	h->hash = *l;
	*l = h;

	dynt(nil, sn);
	init(alefsig, con(s));
}

void
newalt(void)
{
	Altrec *a;

	a = malloc(sizeof(Altrec));
	a->r = 0;
	a->prev = altrec;
	altrec = a;
}

/*
 * Check a variant alt for exhaustive matches
 *	this routine should check no type is supplied by more than one
 *	variant
 */
void
tyalt(Node *n)
{
	Type *t;
	Altrec *a;
	Tyrec *r, *k;

	a = altrec;
	altrec = a->prev;

	if(a->r == 0)
		return;

	/* Force type compare of whole variant */
	for(r = a->r; r; r = r->next)
		r->vch = at(TCHANNEL, r->vch);

	/* Make a unique list of the variants */
	for(r = a->r; r; r = r->next) {
		for(k = a->r; k; k = k->next) {
			if(r == k)
				continue;
			if(typecmp(r->vch, k->vch, 5))
				r->vch = 0;
		}
	}

	/* Check each variant for an alt clause */
	for(r = a->r; r; r = r->next) {
		if(r->vch == 0)
			continue;
		for(t = r->vch->next; t; t = t->variant) {
			for(k = a->r; k; k = k->next) {
				if(typecmp(t, k->rcv, 5))
					break;
			}
			if(k == 0)
				diag(n, "non exhaustive match: no %V", t);
		}
	}
}

void
polytyp(Node *n)
{
	Sym *s;

	s = n->sym;
	if(s->instance)
		diag(n, "redeclaration of %s as type variable", s->name);

	if(s->ltype && s->ltype->type != TPOLY)
		diag(n, "redeclaration of %s from %T", s->name, s->ltype);

	s->ltype = at(TPOLY, 0);
	s->ltype->sym = s;
	s->lexval = Ttypename;
}

void
diadicpoly(Node *n, char *op)
{
	char buf[32];
	Node *l, *r, *rhs, *t;

	line = n->srcline;

	r = n->right;
	r->t = polyshape;
	if(r->type != ONAME) {
		rhs = stknode(polyshape);
		t = an(OASGN, rhs, r);
		t->t = polyshape;
	}
	else {
		t = nil;
		rhs = dupn(r);
	}

	l = an(OADDR, n->left, nil);
	l->t = at(TIND, l->left->t);

	n->type = OLIST;
	n->t = builtype[TVOID];		/* for typesu */
	n->left = t;

	rhs = an(OADDR, rhs, nil);
	rhs->t = at(TIND, rhs->left->t);
	r = an(OLIST, l, rhs);

	t = an(ONAME, nil, nil);
	t->sym = malloc(sizeof(Sym));
	t->t = at(TFUNC, builtype[TVOID]);
	t->t = at(TIND, t->t);
	t->ti = ati(n->t, Global);
	sprint(buf, "ALEF_%s", op);
	t->sym->name = strdup(buf);
	t = an(OADDR, t, nil);
	t->t = at(TIND, t->left->t);

	rhs = an(OADD, rhs, con(4));
	rhs->t = at(TIND, rhs->left->t);
	rhs = an(OIND, rhs, nil);
	rhs->t = builtype[TINT];
	t = an(OADD, t, rhs);
	t->t = t->left->t;
	t = an(OIND, t, nil);
	t->t = t->left->t->next;
	t = an(OIND, t, nil);
	t->t = t->left->t->next;

	r = an(OCALL, t, r);
	r->t = builtype[TVOID];

	n->right = r;
}

Type*
polybuild(Type *base, Type *param)
{
	Node *n;
	Sym *tn;
	Type *t;

	if(opt('b'))
		print("buildpoly: %T[%T]\n", base, param);

	if(base->dcltree == nil) {
		diag(nil, "%T not a polymorphic adt", base);
		return base;
	}
	tn = base->dcltree->left->left->sym;

	n = base->dcltree->poly;
	for(t = param; t; t = t->variant) {
		if(n == nil)
			goto paramc;

		t->sym = n->sym;
		n = n->left;
	}
	if(n)
		goto paramc;

	t = tmcpy(base);
	t->param = param;
	return t;
paramc:
	diag(nil, "wrong number of type parameters: %s[%T]", tn->name, param);
	return base;
}

int
polybox(Node *n)
{
	Type *t;
	Node *l, *c, *tmp, *a;

	line = n->srcline;

	t = n->t;
	if(t->type != TPOLY) {
		diag(n, "(alloc %T) must box with a type variable", n->t);
		return 1;
	}

	if(typechk(n->left, 0))
		return 1;

	l = an(OTCHKED, n->left, nil);
	l->t = l->left->t;

	tmp = stknode(at(TIND, l->t));

	a = rtnode("ALEF_SI", l->t, builtype[TINT]);
	mkdynt(a, l->t);
	a = an(OADDR, a, nil);
	a = an(OCONV, a, nil);
	a->t = builtype[TINT];

	n->left = an(OLIST, nil, a);

	a = an(OCALL, dupn(allocnode), con(l->t->size));
	a->t = at(TIND, builtype[TVOID]);
	a = an(OASGN, tmp, a);

	c = an(OIND, dupn(tmp), nil);
	c = an(OASGN, c, l);
	c = an(OLIST, a, c);

	c = an(OBLOCK, c, dupn(tmp));
	c->t = c->right->t;

	n->left->left = c;
	n->type = OILIST;
	n->t = polyshape;

	if(opt('b')) {
		print("polybox:\n");
		ptree(n, 0);
	}

	if(typechk(n->left, 0))
		fatal("polybox");

	n->t = t;
	return 0;
}

void
polyxerox(Node *n)
{
	Type *ot;
	Node *tmp, *l, *ptr, *c, *d;

	line = n->srcline;

	tmp = stknode(polyshape);
	ptr = stknode(at(TIND, builtype[TVOID]));

	l = n->left;
	ot = l->t;
	l->t = polyshape;

	c = an(OTCHKED, l, nil);
	c = an(OASGN, tmp, c);
	n->left = c;

	c = rtnode("ALEF_SZOF", nil, at(TARRAY, builtype[TCHAR]));
	c = an(OADD, c, an(ODOT, dupn(tmp), nil));
	c->right->sym = polysig;
	c = an(OCONV, c, nil);
	c->t = at(TIND, builtype[TINT]);
	c = an(OIND, c, nil);
	c = an(OCALL, dupn(allocnode), c);
	c = an(OASGN, ptr, c);
	n->left = an(OLIST, n->left, c);

	c = an(ODOT, dupn(tmp), nil);
	c->sym = polyptr;
	c = an(OLIST, ptr, c);
	d = rtnode("ALEF_SZOF", nil, at(TARRAY, builtype[TCHAR]));
	d = an(OADD, d, an(ODOT, dupn(tmp), nil));
	d->right->sym = polysig;
	d = an(OCONV, d, nil);
	d->t = at(TIND, builtype[TINT]);
	d = an(OIND, d, nil);
	c = an(OLIST, c, d);
	c = an(OCALL, movenode, c);
	n->left = an(OLIST, n->left, c);

	c = an(ODOT, dupn(tmp), nil);
	c->sym = polyptr;
	c = an(OASGN, c, ptr);
	n->left = an(OLIST, n->left, c);

	n->type = OBLOCK;
	n->right = tmp;	
	n->t = ot;

	if(typechk(n->left, 0))
		fatal("polyxerox");
}

void
polyswitch(Node *n)
{
	Node *l;

	line = n->srcline;

	l = n->left;
	l->t = polyshape;
	l = an(OTCHKED, l, nil);
	l = an(ODOT, l, nil);
	l->sym = polysig;
	l = an(OADD, alefsig, l);
	l = an(OCONV, l, nil);
	l->t = at(TIND, builtype[TINT]);
	l = an(OIND, l, nil);

	n->left = l;

	if(typechk(n->left, 0))
		fatal("polyswitch");
}

void
settype(Type *t)
{
	Tinfo *v;

	if(swstack->type != ONAME)
		return;

	if(opt('b'))
		print("case %T: on %s\n", t, swstack->sym->name);

	v = swstack->sym->instance;
	if(v == nil)
		return;

	t->subst = builtype[TPOLY];
	v->t = t;
}

void
unsettype(void)
{
	Tinfo *v;

	if(swstack->type != ONAME)
		return;

	v = swstack->sym->instance;
	if(v == nil)
		return;
	v->t = v->t->subst;
}

void
polysize(Node *n)
{
	Node *l, *r;

	line = n->srcline;

	n->left->t = polyshape;
	l = an(OTCHKED, n->left, nil);
	l = an(ODOT, l, nil);
	l->sym = polysig;
	r = rtnode("ALEF_SZOF", nil, at(TARRAY, builtype[TCHAR]));
	l = an(OADD, r, l);
	l = an(OCONV, l, nil);
	l->t = at(TIND, builtype[TINT]);
	n->type = OIND;
	n->left = l;

	if(typechk(n, 0))
		fatal("polysize");
}

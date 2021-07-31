#include <u.h>
#include <libc.h>
#include <bio.h>
#include "parl.h"
#include "y.tab.h"
#define Extern extern
#include "globl.h"

int
sinit(Node *v, Type *t, Node *i, int off)
{
	Inst *p;
	Node *n;
	int j, z;
	String *s, **l;

	if(i->type != OADDR)
		return 0;

	n = i->left;
	if(n->type != ONAME)
		return 0;
	if(n->t->type != TARRAY)
		return 0;

	z = n->t->next->type;
	if(z != TCHAR && z != TSUINT)
		return 0;

	
	if(t->next->type != z)
		diag(n, "incompatible constant string initialisation %T = %T", t, i->t);

	l = &strdat;
	for(s = *l; s; s = s->next) {
		if(n->sym == s->n.sym) {
			*l = s->next;
			break;	
		}
		l = &s->next;
	}
	if(s == 0) {
		diag(n, "array initialisation must be constant");
		return 1;
	}

	if(t->size == 0)
		t->size = s->len;
	else
	if(t->size < s->len)
		diag(n, "array too small for string constant, need %d bytes", s->len);

	for(j = 0; j < s->len; j++) {
		p = ai();
		p->op = ADATA;
		mkdata(v, off+j, 1, p);
		i->type = OCONST;
		i->t = builtype[TINT];
		i->ival = s->string[j];
		mkaddr(i, &p->dst, 0);
		ilink(p);
	}

	return 1;
}

int
sametag(Type *t, char *name)
{
	if(t->tag == ZeroS)
		return 0;

	if(strcmp(t->tag->name, name) == 0)
		return 1;
	return 0;
}

void
doinit(Node *v, Type *t, Node *i, int off)
{
	char *n;
	Inst *p;
	Type *comt;
	Node **ilist, *in;
	int ind, l, cnt, dim, sz;

	n = v->sym->name;

	if(opt('i')) {
		print("DOINIT %N T %T\n", v, t);
		ptree(i, 0);
	}

	switch(t->type) {
	case TINT:
	case TUINT:
	case TSINT:
	case TSUINT:
	case TCHAR:
	case TFLOAT:
		if(i->t == nil) {
			if(i->sym)
				diag(i, "undefined symbol %s initialising %s", i->sym->name, n);
			else
				diag(i, "undefined symbol initialising %s type %T", n, t);
			return;
		}

		if(i->type == OINDEX) {
			diag(v, "array index specifier not in array");
			return;
		}

		if(i->type != OCONST) {
			diag(v, "%s initialisation must be constant", n);
			return;
		}

		if(typeval(typeasgn, t, i->t)) {
			diag(v, "illegal initialiser type: %s '%T'", n, i->t);
			return;
		}
		goto emit;
	
	case TIND:
		if(i->t == nil) {
			diag(i, "undefined symbol in initialisation %s", n);
			return;
		}

		switch(i->t->type) {
		default:
			diag(v, "illegal initialiser type: %s '%T'", n, i->t);
			return;

		case TIND:
			if(typeval(typeasgn, t, i->t)) {
				diag(v, "illegal initialiser type: %s '%T'", n, i->t);
				return;
			}
			comt = i->t->next;
			if(comt == nil || comt->type != TFUNC)
				break;
			if(protocmp(comt->proto, t->next->proto) == 0)
				break;

			diag(v, "prototype mismatch: %P = %P",comt->proto, t->next->proto);
			break;
		case TFUNC:
		case TARRAY:
			i = an(OADDR, i, nil);
			break;
		}
	emit:
		p = ai();
		p->op = ADATA;
		mkdata(v, off, t->size, p);
		mkaddr(i, &p->dst, 0);
		ilink(p);
		break;

	case TARRAY:
		if(sinit(v, t, i, off))
			break;

		if(i->type != OILIST) {
			diag(v, "%s[] initialiser requires '{ }' element list", n);
			return;
		}

		veccnt = 0;
		listcount(i->left, 0);
		ilist = malloc(sizeof(Node*)*veccnt);
		veccnt = 0;
		listcount(i->left, ilist);
		cnt = veccnt;

		sz = t->next->size;
		ind = 0;
		dim = 0;
		for(l = 0; l < cnt; l++) {
			if(ind > dim)
				dim = ind;
			in = ilist[l];

			if(in->type != OINDEX) {
				doinit(v, t->next, in, off+ind);
				ind += sz;
				continue;
			}

			if(in->left->type != OCONST) {
				diag(v, "array index must have constant expression");
				return;
			}

			ind = in->left->ival*sz;	
			doinit(v, t->next, in->right, off+ind);
			ind += sz;
		}	
		if(ind > dim)
			dim = ind;
		if(t->size == 0)
			t->size = dim;
		if(dim > t->size)
			diag(v, "%s too many initialisers", n);
		if(t->size == 0)
			diag(v, "%s[] cannot have zero elements", n);
		break;

	case TUNION:
		diag(v, "cannot initialise union");
		return;

	case TADT:
	case TAGGREGATE:
		if(i->type == OINDEX) {
			diag(v, "array index specifier not in array");
			return;
		}

		if(i->type != OILIST) {
			diag(v, "initialiser requires '{ }' member list");
			return;
		}

		veccnt = 0;
		listcount(i->left, 0);
		ilist = malloc(sizeof(Node*)*veccnt);
		veccnt = 0;
		listcount(i->left, ilist);

		cnt = veccnt;
		comt = t->next;
		for(l = 0; l < cnt; l++) {
			if(comt == nil) {
				diag(v, "%s too many members for '%T'", n, t);
				return;
			}
			in = ilist[l];
			if(in->type == OINDEX) {
				comt = t->next;
				while(comt) {
					if(sametag(comt, in->sym->name))
						break;
					comt = comt->member;
				}
				if(comt == nil) {
					diag(in, "%s not member of %T", in->sym->name, t);
					return;
				}
				in = in->left;
			}
			doinit(v, comt, in, off+comt->offset);
			for(;;) {
				comt = comt->member;
				if(comt == nil)
					break;
				/* Skip adt prototypes */
				if(comt->type != TFUNC)
					break;
			}
		}
		break;

	default:
		diag(v, "cannot initialise %s to type '%T'", n, v->t);
	}
}

int
tasgninit(Type *t, Node *i, int off)
{
	Type *comt;
	Node **ilist, *in, *n;
	int ind, l, cnt, dim, sz, nilok;

	if(opt('i')) {
		print("TASGNINIT %N T %T\n", i, t);
		ptree(i, 0);
	}

	nilok = 0;
	switch(t->type) {
	case TCHANNEL:
		nilok = 1;
	case TIND:
	case TINT:
	case TUINT:
	case TSINT:
	case TSUINT:
	case TCHAR:
	case TFLOAT:
	assign:
		if(i->type == OINDEX) {
			diag(i, "array index specifier not in array");
			return 1;
		}
		if(typechk(i, 0))
			return 1;

		if(nilok && isnil(i))
			break;

		if(typeval(typeasgn, t, i->t)) {
			diag(i, "illegal initialiser type: '%T' = '%T'", t, i->t);
			return 1;
		}

		/* Insert the cast */
		if(typecmp(t, i->t, 5) == 0) {
			n = an(0, nil, nil);
			*n = *i;
			i->type = OCONV;
			i->t = t;
			i->left = n;
			i->right = nil;
		}
		break;
	

	case TARRAY:
		if(i->type != OILIST) {
			diag(i, "[] initialiser requires '{ }' element list");
			return 1;
		}

		veccnt = 0;
		listcount(i->left, 0);
		ilist = malloc(sizeof(Node*)*veccnt);
		veccnt = 0;
		listcount(i->left, ilist);
		cnt = veccnt;

		if(t->size == 0) {
			diag(i, "attempt to initialise unsized array");
			return 1;
		}

		sz = t->next->size;
		ind = 0;
		dim = 0;
		for(l = 0; l < cnt; l++) {
			if(ind > dim)
				dim = ind;
			in = ilist[l];

			if(in->type != OINDEX) {
				if(tasgninit(t->next, in, off+ind))
					return 1;
				ind += sz;
				continue;
			}

			if(in->left->type != OCONST) {
				diag(i, "array index must have constant expression");
				return 1;
			}

			ind = in->left->ival*sz;
			if(tasgninit(t->next, in->right, off+ind))
				return 1;
			ind += sz;
		}	
		if(ind > dim)
			dim = ind;
		if(dim > t->size) {
			diag(i, "too many initialisers");
			return 1;
		}
		if(t->size == 0) {
			diag(i, "array cannot have zero elements");
			return 1;
		}
		break;

	case TADT:
	case TUNION:
	case TAGGREGATE:
		if(i->type == OINDEX) {
			diag(i, "array index specifier not in array");
			return 1;
		}

		if(i->type != OILIST)
			goto assign;

		veccnt = 0;
		listcount(i->left, 0);
		ilist = malloc(sizeof(Node*)*veccnt);
		veccnt = 0;
		listcount(i->left, ilist);

		cnt = veccnt;
		comt = t->next;
		for(l = 0; l < cnt; l++) {
			if(comt == nil) {
				diag(i, "too many initialisers for '%T'", t);
				return 1;
			}

			if(tasgninit(comt, ilist[l], off+comt->offset))
				return 1;

			for(;;) {
				comt = comt->member;
				if(comt == nil)
					break;
				/* Skip adt prototypes */
				if(comt->type != TFUNC)
					break;
			}
		}
		if(comt != nil)
			diag(i, "incomplete tuple/aggregate assignment");
		break;

	default:
		diag(i, "cannot initialise type '%T'", i->t);
		return 1;
	}
	return 0;
}

void
polycode(Dynt *d)
{
	Type *t;
	char buf[32];
	Node *n, *a, *b, *c, *x, *f, *arg;

	a = an(ONAME, nil, nil);
	a->t = at(TIND, polyshape);
	a->sym = enter("ALEF_a", Tid);
	b = an(ONAME, nil, nil);
	b->t = at(TIND, polyshape);
	b->sym = enter("ALEF_b", Tid);

	arg = an(OLIST, dupn(a), dupn(b));

	t = abt(TVOID);
	t->class = Global;

	n = an(ONAME, nil, nil);
	sprint(buf, "ALEF_AS_%luX", d->sig);
	n->sym = enter(buf, Tid);

	fundecl(t, n, arg);
	dupok();

	derivetype(a);
	derivetype(b);

	switch(d->t->type) {
	default:
		fatal("polycode: %T", d->t);
	case TIND:
	case TINT:
	case TUINT:
	case TSINT:
	case TSUINT:
	case TCHAR:
	case TFLOAT:
	case TCHANNEL:
	case TPOLY:
		c = an(OIND, dupn(a), nil);
		c = an(ODOT, c, nil);
		c->sym = polysig;
		x = an(OIND, dupn(b), nil);
		x = an(ODOT, x, nil);
		x->sym = polysig;
		f = an(OASGN, c, x);
		c = an(OIND, dupn(a), nil);
		c = an(ODOT, c, nil);
		c->sym = polyptr;
		c = an(OCONV, c, nil);
		c->t = at(TIND, d->t);
		c = an(OIND, c, nil);
		x = an(OIND, dupn(b), nil);
		x = an(ODOT, x, nil);
		x->sym = polyptr;
		x = an(OCONV, x, nil);
		x->t = at(TIND, d->t);
		x = an(OIND, x, nil);
		c = an(OASGN, c, x);
		f = an(OLIST, f, c);
		break;
	case TADT:
	case TUNION:
	case TAGGREGATE:
		c = an(OIND, dupn(a), nil);
		c = an(ODOT, c, nil);
		c->sym = polysig;
		x = an(OIND, dupn(b), nil);
		x = an(ODOT, x, nil);
		x->sym = polysig;
		f = an(OASGN, c, x);
		c = an(OIND, dupn(a), nil);
		c = an(ODOT, c, nil);
		c->sym = polyptr;
		c = an(OCONV, c, nil);
		c->t = at(TIND, d->t);
		c = an(OIND, c, nil);
		x = an(OIND, dupn(b), nil);
		x = an(ODOT, x, nil);
		x->sym = polyptr;
		x = an(OCONV, x, nil);
		x->t = at(TIND, d->t);
		x = an(OIND, x, nil);
		c = an(OASGN, c, x);
		f = an(OLIST, f, c);
		break;
	}

	if(opt('b')) {
		print("polycode %T %N:\n", d->t, n);
		ptree(f, 0);
	}

	fungen(f, n);

	dynt(rtnode("ALEF_AS", nil, at(TIND, builtype[TFUNC])), d->n);
	init(rtnode("ALEF_SZOF", nil, at(TARRAY, builtype[TINT])), con(d->t->size));
}

void
polyasgn(void)
{
	int i;
	Dynt *h;

	for(i = 0; i < NDYHASH; i++) {
		for(h = dynhash[i]; h; h = h->hash) {
			if(opt('S'))
				print("POLY #%.8luX %T\n",  h->sig, h->t);
			polycode(h);
		}
	}
}

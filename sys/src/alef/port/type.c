#include <u.h>
#include <libc.h>
#include <bio.h>
#include "parl.h"
#include "y.tab.h"
#define Extern extern
#include "globl.h"
#include "tcom.h"

/*
 * detect casts which generate no code
 */
int
convisnop(Type *t1, Type *t2)
{
	int i, b;

	i = 0;
	if(t2)
		i = t2->type;
	b = 1<<i;
	i = 0;
	if(t1)
		i = t1->type;
	if(b)
	if(b & nopconv[i])
		return 1;

	return 0;
}

int
isnil(Node *n)
{
	Type *t;

	if(n->type != OCONST)
		return 0;

	t = n->t;
	if(t->type != TIND || t->next->type != TVOID)
		return 0;

	if(n->ival != 0)
		return 0;

	return 1;
}

/*
 * type arithmetic operators and do pointer arith scaling
 */
int
tmathop(Node *n, int logical, char tab[Ntype][Ntype])
{
	int v;
	char *op;
	Type *bt;
	Node *newn, *l, *r, *siz;

	line = n->srcline;

	l = n->left;
	r = n->right;
	if(opt('m'))
		print("tmathop: %N l %N r %N\n", n,  l, r);

	if(l->t->type == TIND && n->type == OSUB && r->t->type == TIND) {
		n->t = builtype[TINT];
		bt = l->t->next;
		if(bt->type == TXXX)
			typesut(bt);
		v = bt->size;
		/* Make ODIV (OSUB ptr ptr) size */
		if(v <= 0) {
			diag(n, "pointer SUB '%T' illegal", l->t);
			return 1;
		}
		if(v == 1)
			return 0;

		newn = con(v);
		newn = an(ODIV, nil, newn);
		newn->t = newn->right->t;
		newn->left = dupn(n);
		*n = *newn;
		return 0;
	}
	v = tab[l->t->type][r->t->type];

	switch(v) {
	case TFLOAT:
	case TIND:
		if(logical == 0)
			break;

		/* Fall through */
	case 0:
		goto bad;
	}

	/* One side needs fixing */
	if(v == TIND && tab != logtype) {
		switch(n->type) {
		default:
			goto bad;
		case OADD:
		case OSUB:
		case OADDEQ:
		case OSUBEQ:
			break;
		}
		op = treeop[n->type]+1;
		/* Insert OADD (OMUL size int) ptr */
		if(l->t->type == TIND) {
			n->t = l->t;
			bt = l->t->next;
			if(bt->type == TXXX)
				typesut(bt);

			v = bt->size;
			if(v <= 0)
				goto perr;
			if(v == 1)
				return 0;
			siz = con(v);
			newn = an(OMUL, r, siz);
			newn->t = builtype[TINT];
			n->right = newn;
			return 0;
		}

		n->t = r->t;
		bt = r->t->next;
		if(bt->type == TXXX)
			typesut(bt);
		v = bt->size;
		if(v <= 0) {
			perr:
			diag(n, "pointer %s '%T' illegal", op, l->t);
			return 1;
		}
		if(v == 1)
			return 0;
		siz = con(v);
		newn = an(OMUL, l, siz);
		newn->t = builtype[TINT];
		n->left = newn;
		return 0;
	}

	/* Propogate results and insert converts to fix types */
	n->t = builtype[v];	
	if(l->t->type != v) {
		if(typecmp(n->t, l->t, 5) == 0) {
			newn = dupn(l);
			l->type = OCONV;
			l->right = nil;
			l->t = n->t;
			l->left = newn;
		}
	}
	if(r->t->type != v) {
		n->t = l->t;
		if(typecmp(n->t, r->t, 5) == 0) {
			newn = dupn(r);
			r->type = OCONV;
			r->right = nil;
			r->t = n->t;
			r->left = newn;
		}
	}
	if(opt('m')) {
		print("tmathop: %T %T result %s\n", l->t, r->t, typestr[v]);
		ptree(n, 0);
	}
	return 0;
bad:
	diag(n, "bad pointer math %T %s %T", l->t, treeop[n->type]+1, r->t);
	return 1;
}

/*
 * Build a call through an ADT
 */
int
tyadt(Node *n)
{
	Sym *s;
	Type *t;
	int offset;
	Node *l, *adt, *al;
	char buf[Strsize];

	l = n->left;
	adt = l->left;
	if(typechk(adt, 0))
		return 1;

	switch(adt->t->type) {
	default:
		diag(n, "incompatible type: %T.%s()", adt->t, l->sym->name);
		return 1;
	case TCHANNEL:
		t = at(TFUNC, at(TIND, builtype[TCHAR]));
		t->proto = an(ONAME, an(OVARARG, nil, nil), nil);
		if(chkchan(n, l->sym->name))
			return 1;
		sprint(buf, "ALEF_%s", l->sym->name);
		n->left = rtnode(buf, nil, t);
		n->right = an(OLIST, an(OTCHKED, l->left, nil), n->right);
		t = adt->t->next;
		n->right = an(OLIST, n->right, con(typesig(t)));
		while(t) {
			packmk(typesig(t), t, Needed);
			t = t->variant;
		}
		return typechk(n, 0);
	case TADT:
	case TUNION:
	case TAGGREGATE:
		break;
	}

	n->poly = adt;		/* tag the node with its deriving type */

	offset = 0;
	t = walktype(adt->t, l, &offset);
	if(t == nil) {
		diag(n, "%s not a member of %T", l->sym->name, adt->t);
		return 1;
	}
	if(t->type != TFUNC) {
		diag(n, "adt call must be to function: %T", t);
		return 1;
	}

	adt = l;
	adt->type = OTCHKED;

	s = ltytosym(parent);
	if(s == ZeroS)
		fatal("tyadt: %T", parent);
	sprint(buf, "%s_%s", s->name, t->tag->name);
	n->left = rtnode(buf, nil, t);

	l = t->proto;
	while(l->left)
		l = l->left;

	if(l->type != OADTARG)
		return typechk(n, 0);
	/*
	 * has *ADT or .ADT as first arg
	 */
	al = adt->left;
	al->islval = 1;
	adt = an(OADDR, adt, nil);
	adt = an(OCONV, adt, nil);
	adt->t = at(TIND, builtype[TCHAR]);
	adt = an(OADD, adt, con(offset));
	adt = an(OCONV, adt, nil);
	adt->t = at(TIND, parent);
	if(l->t->type != TIND) {
		if(al->left && al->left->type == OCONST) {
			diag(n, "cannot pass *nil as implicit parameter");
			return 1;
		}
		adt = an(OIND, adt, nil);
	}
	if(n->right == nil)
		n->right = adt;
	else
		addarg(n, adt);

	return typechk(n, 0);
}

int
tvariant(Node *n)
{
	Tyrec *tr;
	Type *lt, *rt;

	if(n->t->variant == nil)
		return 0;

	if(altrec == 0) {
		diag(n, "variant receive not in alt");
		return 1;
	}
	if(n->vlval == nil) {
		diag(n, "variant receive requires a simple assignment");
		return 1;
	}

	lt = n->vlval->t;
	rt = n->left->t;

	tr = malloc(sizeof(Tyrec));
	tr->vch = n->t;

	/* Search in variant channel */
	while(n->t) {
		if(typecmp(n->t, lt, 5))
			break;

		n->t = n->t->variant;
	}

	if(n->t == nil) {
		diag(n, "no match in variant %T = <-%T", lt, rt->next);
		return 1;
	}
	tr->rcv = n->t;

	tr->next = altrec->r;
	altrec->r = tr;

	return 0;
}

void
tptlist(Node *n, int t)
{
	Node *x;

	if(n == 0)
		return;
	switch(n->type) {
	case OLIST:
		tptlist(n->left, t);
		tptlist(n->right, t);
		break;
	default:
		x = dupn(n);
		n->type = t;
		n->left = x;
		n->right = nil;
	}
}

void
typepoly(Node *n, int eval)
{
	Type *ot;
	int dealloc;
	Node *x, *tmp, *ptr;

	line = n->srcline;

	if(opt('b')) {
		print("typepoly: bind %T -> %T\n", n->t, n->t->subst);
		ptree(n, 0);
	}

	if(eval & NOUNBOX)
		return;

	ot = n->t;
	dealloc = 1;
	if(n->t->subst->sym == nil)
		dealloc = 0;
	n->t->subst = nil;
	n->t = polyshape;

	x = an(OTCHKED, dupn(n), nil);
	if(dealloc) {
		tmp = stknode(ot);
		ptr = stknode(at(TIND, ot));

		x = an(ODOT, x, nil);
		x->sym = polyptr;
		x = an(OASGN, dupn(ptr), x);
		n->left = x;

		x = an(OIND, dupn(ptr), nil);
		x = an(OASGN, dupn(tmp), x);
		n->left = an(OLIST, n->left, x);

		x = an(OCALL, dupn(unallocnode), ptr);
		n->left = an(OLIST, n->left, x);
		n->right = tmp;
		n->type = OBLOCK;
		n->t = ot;
	}
	else {
		x = an(ODOT, x, nil);
		x->sym = polyptr;
		x = an(OCONV, x, nil);
		x->t = at(TIND, ot);
		x = an(OIND, x, nil);
		*n = *x;
	}
	if(typechk(n, 0))
		fatal("typepoly");

	if(opt('b'))
		ptree(n, 0);
}

/*
 * typecheck a portion of the tree
 */
int
typechk(Node *n, int evalfor)
{
	Type *t;
	int v, size;
	Node *l, *r;

	if(n == nil)
		return 0;

	l = n->left;
	r = n->right;

	switch(n->type) {
	default:
		return typechk(l, 0) | typechk(r, 0);

	case OSWITCH:
		if(typechk(l, 0) || typechk(r, 0))
			return 1;

		switch(l->t->type) {
		default:
			diag(n, "switch %T must be integral or poly", l->t);
			return 1;
		case TINT:
		case TUINT:
			break;
		case TCHAR:
		case TSINT:
		case TSUINT:
			l = an(OCONV, l, nil);
			l->t = builtype[TINT];
			n->left = l;
			break;
		case TPOLY:
			polyswitch(n);
			break;
		}
		return 0;
		
	case OITER:
		return 0;

	case OXEROX:
		if(typechk(l, 0))
			return 1;
		if(l->t->type != TPOLY) {
			diag(l, "cannot xerox type %T", l->t);
			return 1;
		}
		polyxerox(n);
		break;

	case OIF:
		if(typechk(l, 0) || typechk(r, 0))
			return 1;

		v = l->t->type;
		if(((1<<v) & MBOOL) == 0) {
			diag(n, "if has non boolean type %T", l->t);
			return 1;
		}
		return 0;

	case OWHILE:
	case ODWHILE:
		if(typechk(l, 0) || typechk(r, 0))
			return 1;

		v = l->t->type;
		if(((1<<v) & MBOOL) == 0) {
			diag(n, "while has non boolean type %T", l->t);
			return 1;
		}
		return 0;

	case OFOR:
		if(typechk(l, 0) || typechk(r, 0))
			return 1;

		l = l->right;
		if(l->left) {
			t = l->left->t;
			v = t->type;
			if(((1<<v) & MBOOL) == 0) {
				diag(n, "for has non boolean type %T", t);
				return 1;
			}
		}
		return 0;

	case OTCHKED:
		*n = *l;
		return 0;

	case OALLOC:
		if(n->t)
			return polybox(n);
		*n = *l;
		return tyalloc(n);

	case OUALLOC:
		*n = *l;
		return tyunalloc(n);

	case OCALL:
		if(l->type == ODOT)
			return tyadt(n);

		if(typechk(l, 0) || typechk(r, 0))
			return 1;

		if(l->t->type == TIND && l->t->next->type == TFUNC) {
			l = an(OIND, l, nil);
			l->t = l->left->t->next;
			n->left = l;
		}

		if(tycall(n))
			return 1;
		
		/* Pick up the return type from TFUNC->next */
		n->t = l->t->next;
		break;

	case OPROCESS:
		if(l->type == OLIST) {
			tptlist(l, OPROCESS);
			*n = *l;
			return typechk(n, 0);
		}
		if(typechk(l, 0))
			return 1;

		if(l->type != OCALL) {
			diag(n, "proc must call a function");
			return 1;
		}

		typroc(l, procnode);
		*n = *l;
		return 0;

	case OTASK:
		if(l->type == OLIST) {
			tptlist(l, OTASK);
			*n = *l;
			return typechk(n, 0);
		}
		if(typechk(l, 0))
			return 1;

		if(l->type != OCALL) {
			diag(n, "task must call a function");
			return 1;
		}

		typroc(l, tasknode);
		*n = *l;
		return 0;

	case OBECOME:
		if(l->type != OCALL) {
			n->type = ORET;
			return typechk(n, 0);
		}

		if(typechk(l, 0))
			return 1;

		t = curfunc->t->next;	/* return type */
		if(typecmp(t, l->t, 5) == 0) {
			diag(n, "incompatible types: %T become %T", t, l->t);
			return 1;
		}
		return 0;

	case OCHECK:
		return tyassert(n);

	case OILIST:
		if(n->t == nil)
			return typechk(l, 0);

		if(((1<<n->t->type)&MCOMPLEX) == 0) {
			diag(n, "initialiser type must be complex %T", n->t);
			return 1;
		}

		n->islval = 1;

		/* If we have no members build a tuple */
		if(n->t->next == 0) {
			if(typechk(l, 0))
				return 1;

			tupleof(n);
			return 0;
		}

		return tasgninit(n->t, n, 0);

	case ONAME:
		if(n->t == 0) {
			diag(n, "undefined variable %s", n->sym->name);
			return 1;
		}
		if(n->ti->class == Private) {
			switch(n->t->type) {
			default:
				diag(n, "private must be integral: %T", n->t);
				break;
			case TINT:
			case TUINT:
			case TSINT:
			case TSUINT:
			case TCHAR:
			case TIND:
				break;
			}
			n->type = OREGISTER;
			n->reg = n->ti->offset;
		}
		if(n->ti->t == 0)
			fatal("no t in tinfo %s", n->sym->name);
		n->islval = 1;
		break;

	case OAGDECL:
	case OUNDECL:
	case OSETDECL:
	case OADTDECL:
		fatal("typechk");

	case OPINC:
	case OPDEC:
	case OEINC:
	case OEDEC:
		if(typechk(l, 0))
			return 1;

		if(chklval(l))
			return 1;

		v = (1<<l->t->type) & MINCDEC;
		if(v == 0) {
			diag(n, "bad type for ++ or -- '%T'", l->t);
			return 1;
		}
		n->t = l->t;
		if(n->t->type == TIND && n->t->next->size < 1)
			diag(n, "illegal ++ or -- pointer math");
		break;

	case ONOT:
		if(typechk(l, 0))
			return 1;
		if(((1<<l->t->type)&MBOOL) == 0) {
			diag(n, "! has non boolean type %T", l->t);
			return 1;
		}
		n->t = builtype[TINT];
		break;

	case OEQ:
	case ONEQ:
		if(typechk(l, 0) || typechk(r, 0))
			return 1;

		n->t = builtype[TINT];
		if(l->t->type == TCHANNEL && r->t->type == TCHANNEL)
			break;

		if(isnil(l)) {
			if(r->t->type == TCHANNEL)
				break;
			if(r->t->type == TPOLY) {
				r->t = polyshape;
				r = an(OTCHKED, r, nil);
				r = an(ODOT, r, nil);
				r->sym = polyptr;
				n->right = r;
				if(typechk(r, 0))
					return 1;
				n->t = l->t;
				break;
			}
		}
		if(isnil(r)) {
			if(l->t->type == TCHANNEL)
				break;
			if(l->t->type == TPOLY) {
				l->t = polyshape;
				l = an(OTCHKED, l, nil);
				l = an(ODOT, l, nil);
				l->sym = polyptr;
				n->left = l;
				if(typechk(l, 0))
					return 1;
				n->t = r->t;
				break;
			}
		}
		goto condchk;

	case OCAND:
	case OLEQ:
	case OLT:
	case OGEQ:
	case OGT:
	case OCOR:
		if(typechk(l, 0) || typechk(r, 0))
			return 1;

	condchk:
		if(tmathop(n, 0, logtype))
			return 1;

		if(((1<<l->t->type)&MUNSIGNED)||((1<<r->t->type)&MUNSIGNED)) {
			switch(n->type) {
			case OLT:
				n->type = OLO;
				break;
			case OGT:
				n->type = OHI;
				break;
			case OGEQ:
				n->type = OHIS;
				break;
			case OLEQ:
				n->type = OLOS;
				break;
			}
		}
		n->t = builtype[TINT];
		break;

	case OSTORAGE:
		size = 0;
		if(n->t)
			size = n->t->size;
		else {
			if(typechk(l, NADDR))
				return 1;

			if(l->t) {
				if(l->t->type == TPOLY) {
					polysize(n);
					break;
				}
				size = l->t->size;
			}
		}
		if(size == 0)
			diag(n, "sizeof unsized/unresovled type");

		n->type = OCONST;
		n->ival = size;
		n->t = builtype[TINT];
		break;

	case OADD:
		if(n->t)			/* Adt already typed */
			return 0;

	case ODIV:
	case OSUB:
	case OMUL:
		if(typechk(l, 0) || typechk(r, 0))
			return 1;

		return tmathop(n, 0, moptype);

	case OMOD:
	case OXOR:
	case OLOR:
	case OLAND:
	case OLSH:
	case ORSH:
		if(typechk(l, 0) || typechk(r, 0))
			return 1;

		return tmathop(n, 1, moptype);

	case OADDEQ:
	case OSUBEQ:
		if(typechk(l, 0) || typechk(r, 0))
			return 1;

		if(chklval(l))
			return 1;

		if(typeval(typeaddasgn, l->t, r->t)) {
			diag(n, "incompatible types: %T %s %T",
						l->t, treeop[n->type]+1, r->t);
			return 1;
		}

		return tmathop(n, 0, moptype);
		break;

	case OMULEQ:
	case ODIVEQ:
		if(typechk(l, 0) || typechk(r, 0))
			return 1;

		if(chklval(l))
			return 1;

		if(typeval(typeasgn, l->t, r->t)) {
			diag(n, "incompatible types: %T %s %T",
						l->t, treeop[n->type]+1, r->t);
			return 1;
		}

		return tmathop(n, 0, moptype);
		break;

	case ORSHEQ:
	case OLSHEQ:
		if(typechk(l, 0) || typechk(r, 0))
			return 1;

		if(chklval(l))
			return 1;

		v = 1<<l->t->type;
		v &= MINTEGRAL;
		if(v == 0) {
			diag(n, "bitwise shift of non-integral type");
			return 1;
		}
		v = 1<<r->t->type;
		v &= MINTEGRAL;
		if(v == 0) {
			diag(n, "shift count of non-integral type");
			return 1;
		}
		n->t = l->t;
		break;
	case OOREQ:
	case OANDEQ:
	case OXOREQ:
	case OMODEQ:
		if(typechk(l, 0) || typechk(r, 0))
			return 1;

		if(chklval(l))
			return 1;

		if(typeval(typeasand, l->t, r->t)) {
			diag(n, "incompatible types: %T %s %T",
						l->t, treeop[n->type]+1, r->t);
			return 1;
		}

		return tmathop(n, 1, moptype);
		break;
	
	case ORECV:
		if(typechk(l, 0))
			return 1;

		t = l->t;
		if(t->type != TCHANNEL) {
			diag(n, "must receive on a channel: %T", t);
			return 1;
		}

		n->t = t->next;
		if(n->t->type == TXXX)
			typesut(n->t);

		if(tvariant(n))
			return 1;

		switch(n->t->type) {
		case TADT:
		case TUNION:
		case TAGGREGATE:
			l = an(OLIST, l, con(n->t->size));
			break;
		}
		n->right = l;
		n->left = rfun[n->t->type];
		break;

	case OCSND:
	case OCRCV:
		if(typechk(l, 0))
			return 1;

		if(l->t->type != TCHANNEL) {
			diag(n, "must ? a channel: %T", l->t);
			return 1;
		}
		n->t = builtype[TINT];
		if(n->type == OCSND)
			n->left = csndnode;
		else
			n->left = crcvnode;
		n->type = OCALL;
		n->right = l;
		break;

	case OVASGN:
		return tyvasgn(n);

	case OASGN:
		switch(l->type) {
		case OSEND:
			if(tysend(n))
				return 1;
			*n = *(n->left);
			return 0;
		case OILIST:
			return tupleasgn(n);	/* lhs is complex shape */
		}

		if(typechk(l, NOUNBOX) || chklval(l))
			return 1;

		r->vlval = l;
		if(typechk(r, 0))
			return 1;

		if(l->t->subst != nil) { /* deal with binding (no promotion??) */
			if(typeval(typeasgn, l->t, r->t)) {
				diag(n, "incompatible types: %T = %T", l->t, r->t);
				return 1;
			}
			r = an(OTCHKED, r, nil);
			if(typecmp(l->t, n->right->t, 5) == 0) {
				r = an(OCONV, r, nil);
				r->t = l->t;
			}
			r = an(OALLOC, r, nil);
			r->t = l->t->subst;
			n->right = r;
			l->t = polyshape;
			n->t = polyshape;
			return typechk(r, 0);
		}

		/*
		 * Special for nil
		 */
		if(isnil(r)) {
			if(l->t->type == TCHANNEL) {
				n->t = l->t;
				break;
			}
			if(l->t->type == TPOLY) {
				l->t = polyshape;
				l = an(OTCHKED, l, nil);
				l = an(ODOT, l, nil);
				l->sym = polyptr;
				n->left = l;
				if(typechk(l, 0))
					return 1;
				n->t = r->t;
				break;		
			}
		}

		/* Check legality */
		if(typeval(typeasgn, l->t, r->t) && promote(r, l)) {
			diag(n, "incompatible types: %T = %T", l->t, r->t);
			return 1;
		}

		switch(l->t->type) {
		case TPOLY:
			l->t = polyshape;
			r->t = polyshape;
			break;
		case TADT:
		case TAGGREGATE:
			if(vcmp(l->t->param, r->t->param))
				break;
			diag(n, "incompatible binding: %T = %T", l->t, r->t);
			return 1;
		default:
			/* Insert the cast */
			if(typecmp(l->t, r->t, 5) == 0) {
				n->right = an(OCONV, r, nil);
				n->right->t = l->t;
			}
		}
		n->t = l->t;
		break;

	case OSEND:
		diag(n, "send requires assignment");
		return 1;

	case OIND:
		if(typechk(l, 0))
			return 1;

		if(l->t->type != TIND) {
			diag(n, "illegal indirection through type %T", l->t);
			return 1;
		}
		n->t = l->t->next;
		n->islval = 1;
		break;

	case OADDR:
		if(typechk(l, NADDR))
			return 1;

		if(l->t->type == TARRAY || l->t->type == TFUNC) {
			warn(n, "& of array/function ignored");
			tyaddr(l);
			*n = *l;
			return 0;
		}

		if(chklval(l))
			return 1;

		if(l->type == OREGISTER)
			diag(n, "cannot take address of private");

		n->t = at(TIND, l->t);
		break;

	case ODOT:
		if(tydot(n))
			return 1;
		break;

	case OCAST:
		if(typechk(l, 0))
			return 1;

		if(n->t->size != l->t->size) {
			diag(n, "cast must be same size: [%T]%T", n->t, l->t);
			return 1;
		}
		break;

	case OCONV:
		if(l->type == OILIST) {
			t = n->t;
			*n = *l;
			n->t = t;
			return typechk(n, 0);
		}
		if(typechk(l, 0))
			return 1;

		v = convtab[n->t->type] & (1<<l->t->type);
		if(v == 0) {
			diag(n, "illegal convert (%T)%T", n->t, l->t);
			return 1;
		}
		if(convisnop(n->t, l->t)) {
			l->t = n->t;
			*n = *l;
		}
		break;

	case ORET:
		t = curfunc->t->next;	/* return type */

		if(l == nil) {
			if(t->type != TVOID) {
				diag(n, "return should be %P", curfunc);
				return 1; 
			}
			n->t = builtype[TVOID];
			break;
		}

		if(t->type == TVOID) {
			diag(n, "return should be %P", curfunc);
			return 1;
		}

		if(l->type == OILIST)
			l->t = t;

		if(typechk(l, 0))
			return 1;

		n->t = t;

		if(typeval(typeasgn, n->t, l->t) && promote(l, n)) {
			diag(n, "incompatible types: %P returns %T", curfunc, l->t);
			return 1;
		}

		if(n->t->type == TPOLY) {
			n->t = polyshape;
			n->left->t = polyshape;
			break;
		}

		/* Insert the cast */
		if(typecmp(n->t, l->t, 5) == 0) {
			n->left = an(OCONV, l, nil);
			n->left->t = n->t;
		}
		break;

	case OSELECT:
		newalt();
		if(typechk(l, 0)) {
			altrec = altrec->prev;
			return 1;
		}
		tyalt(n);
		return 0;
	}

	if(n->t->type == TXXX)
		typesut(n->t);
	if(n->t->subst != nil)
		typepoly(n, evalfor);

	if(evalfor & NADDR)
		return 0;
	return tyaddr(n);
}

int
tyassert(Node *n)
{
	Node *s1, *s2;
	static Node *c;

	if(chks == 0) {
		n->type = OLIST;
		n->left = nil;
		n->right = nil;
		return 0;
	}

	n->left = an(ONOT, n->left, nil);
	n->type = OIF;
	s1 = strnode(mkstring("%L%s()", n->srcline, curfunc->sym->name), 0);
	s2 = n->right;
	if(s2 == nil) {
		if(c == nil)
			c = strnode(mkstring("check"), 0);
		s2 = dupn(c);
	}
	n->right = an(OCALL, checknode, an(OLIST, s1, s2));

	return typechk(n, 0);
}

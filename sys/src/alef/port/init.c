#include <u.h>
#include <libc.h>
#include <bio.h>
#include "parl.h"
#include "y.tab.h"
#define Extern
#include "globl.h"

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
		print("INIT %N T %T\n", v, t);
		ptree(i, 0);
	}

	switch(t->type) {
	case TINT:
	case TUINT:
	case TSINT:
	case TSUINT:
	case TCHAR:
	case TFLOAT:
		if(i->t == ZeroT) {
			diag(i, "undefined symbol in initialisation %s", n);
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
		if(i->t == ZeroT) {
			diag(i, "undefined symbol in initialisation %s", n);
			return;
		}

		switch(i->t->type) {
		default:
			diag(v, "illegal initialiser type: %s '%T'", n, i->t);
			return;

		case TIND:
			break;

		case TARRAY:
		case TFUNC:
			i = an(OADDR, i, ZeroN);
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

	case TADT:
	case TUNION:
	case TAGGREGATE:
		if(i->type == OINDEX) {
			diag(v, "array index specifier not in array");
			return;
		}

		if(i->type != OILIST) {
			diag(v, "complex initialiser requires '{ }' member list");
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
			if(comt == ZeroT) {
				diag(v, "%s too many initialisers for '%T'", n, t);
				return;
			}
			doinit(v, comt, ilist[l], off+comt->offset);
			for(;;) {
				comt = comt->member;
				if(comt == ZeroT)
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
	int ind, l, cnt, dim, sz;

	if(opt('i')) {
		print("INIT %N T %T\n", i, t);
		ptree(i, 0);
	}

	switch(t->type) {
	case TIND:
	case TINT:
	case TUINT:
	case TSINT:
	case TSUINT:
	case TCHAR:
	case TFLOAT:
		if(i->type == OINDEX) {
			diag(i, "array index specifier not in array");
			return 1;
		}
		if(typechk(i, 0))
			return 1;

		if(typeval(typeasgn, t, i->t)) {
			diag(i, "illegal initialiser type: '%T' = '%T'", t, i->t);
			return 1;
		}

		/* Insert the cast */
		if(typecmp(t, i->t, 5) == 0) {
			n = an(0, ZeroN, ZeroN);
			*n = *i;
			i->type = OCONV;
			i->t = t;
			i->left = n;
			i->right = ZeroN;
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

		if(i->type != OILIST) {
			diag(i, "complex initialiser requires '{ }' member list");
			return 1;
		}

		veccnt = 0;
		listcount(i->left, 0);
		ilist = malloc(sizeof(Node*)*veccnt);
		veccnt = 0;
		listcount(i->left, ilist);

		cnt = veccnt;
		comt = t->next;
		for(l = 0; l < cnt; l++) {
			if(comt == ZeroT) {
				diag(i, "too many initialisers for '%T'", t);
				return 1;
			}

			if(tasgninit(comt, ilist[l], off+comt->offset))
				return 1;

			for(;;) {
				comt = comt->member;
				if(comt == ZeroT)
					break;
				/* Skip adt prototypes */
				if(comt->type != TFUNC)
					break;
			}
		}
		break;

	default:
		diag(i, "cannot initialise type '%T'", i->t);
		return 1;
	}
	return 0;
}

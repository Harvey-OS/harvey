#include <u.h>
#include <libc.h>
#include <bio.h>
#include "parl.h"
#include "y.tab.h"
#define Extern
#include "globl.h"

/* fold constant operators */
void
foldop(Node *n)
{
	int li, ri, fop;
	float lf, rf;
	int isflt;
	Node *l, *r;

	fop = 0;
	isflt = 0;
	n->fval = 0.0;
	n->ival = 0;

	r = n->right;
	l = n->left;

	if(l->t->type == TFLOAT) {
		isflt = 1;
		lf = l->fval;
		li = (int)lf;
	}
	else {
		li = l->ival;
		lf = (float)li;
	}
	if(r->t->type == TFLOAT) {
		isflt |= 1;
		rf = r->fval;
		ri = (int)rf;
	}
	else {
		ri = r->ival;
		rf = (float)ri;
	}


	switch(n->type) {
	default:
		fatal("fold: %N", n);
	case OADD:
		if(isflt)
			n->fval = lf + rf;
		else
			n->ival = li + ri;
		fop = 1;
		break;

	case ODIV:
		if(isflt)
			n->fval = lf / rf;
		else
			n->ival = li / ri;
		fop = 1;
		break;

	case OMOD:
		n->ival = li % ri;
		break;

	case OSUB:
		if(isflt)
			n->fval = lf - rf;
		else
			n->ival = li - ri;
		fop = 1;
		break;

	case OMUL:
		if(isflt)
			n->fval = lf * rf;
		else
			n->ival = li * ri;

		fop = 1;
		break;

	case OXOR:
		n->ival = li ^ ri;
		break;

	case OLOR:
		n->ival = li | ri;
		break;

	case OLAND:
		n->ival = li & ri;
		break;

	case OCAND:
		n->ival = li && ri;
		break;

	case OCOR:
		n->ival = li || ri;
		break;

	case ONOT:
		n->ival = !li;
		break;

	case OARSH:				/* BUG */
	case ORSH:
		n->ival = li >> ri;
		break;

	case OALSH:				/* BUG */
	case OLSH:
		n->ival = li << ri;
		break;

	case OEQ:
		n->ival = li == ri;
		break;

	case OGT:
		n->ival = li > ri;
		break;

	case ONEQ:
		n->ival = li != ri;
		break;

	case OGEQ:
		n->ival = li >= ri;
		break;

	case OLEQ:
		n->ival = li <= ri;
		break;

	case OLT:
		n->ival = li < ri;
		break;

	}

	if(opt('c'))
		print("fold: %d(%N) [%N] %d(%N) = %d\n", li, l, n, ri, r, n->ival);

	n->type = OCONST;
	n->left = ZeroN;
	n->right = ZeroN;
	if(isflt) {
		n->t = builtype[TFLOAT];
		if(fop == 0)
			diag(n, "invalid float operator %s", treeop[n->type]+1);
	}
	else
		n->t = builtype[TINT];
}

void
commute(Node *n)
{
	Node *l, *r;

	if(n->t == ZeroT)
		return;

	l = n->left;
	r = n->right;

	if(n->type != l->type)
		return;
	if(r->type != OCONST || l->right->type != OCONST)
		return;

	switch(n->t->type) {
	default:
		return;
	case TINT:
	case TUINT:
	case TSINT:
	case TSUINT:
	case TCHAR:
	case TIND:
		break;
	}

	switch(n->type) {
	default:
		fatal("commute");

	case OADD:
		r->ival = r->ival + l->right->ival;
		n->left = l->left;
		break;

	case OMUL:
		r->ival = r->ival * l->right->ival;
		n->left = l->left;
		break;

	case OEQ:
		r->ival = r->ival == l->right->ival;
		n->left = l->left;
		break;

	case OXOR:
		r->ival = r->ival ^ l->right->ival;
		n->left = l->left;
		break;

	case OLAND:
		r->ival = r->ival & l->right->ival;
		n->left = l->left;
		break;

	case OLOR:
		r->ival = r->ival | l->right->ival;
		n->left = l->left;
		break;
	}
}

int
ispow2(int a)
{
	int m, i;

	for(i = 0; i < Machint*4; i++) {
		m = 1<<i;
		if((a & m) == a)
			return i;
	}
	return -1;
}

/*
 * fold away constant casts
 */
void
convop(Node *n)
{
	int v;
	Node *l;

	l = n->left;
	if(l->t->type == TFLOAT) {
		if(l->fval > 0x7fffffff || l->fval < -0x7fffffff) {
			warn(n, "overflow in (INT)FLOAT conversion");
			return;
		}
		v = l->fval;
	}
	else
		v = l->ival;
	
	switch(n->t->type) {
	default:
		n->ival = v;
		break;

	case TCHAR:
		n->ival = v & 0xff;
		break;

	case TSINT:
		n->ival = v & 0xffff;
		if(n->ival & 0x8000)
			n->ival |= ~0xffff;
		break;

	case TSUINT:
		n->ival = v & 0xffff;
		break;

	case TFLOAT:
		n->fval = v;
		break;
	}
	n->left = ZeroN;
	n->type = OCONST;
}

void
rewrite(Node *n)
{
	int p2;
	Type *t;
	Node *l, *r;

	if(n == ZeroN)
		return;

	if(opt('d'))
		print("rewrite: %N\n", n);

	l = n->left;
	r = n->right;

	switch(n->type) {
	default:
		rewrite(l);
		rewrite(r);
		return;

	case OADD:			/* Commutative ops rewrite constants */
	case OEQ:			/* to the right */
	case OXOR:
	case OLAND:
	case OLOR:
		rewrite(l);
		rewrite(r);
		if(l->type == OCONST) {
			n->right = n->left;
			n->left = r;	
		}
		commute(n);
		l = n->left;
		r = n->right;
		break;

	case OMUL:
		rewrite(l);
		rewrite(r);

		/* Rewrite constant integer multiplies to shifts */
		if(r->type == OCONST)
		if(r->t->type != TFLOAT) {
			p2 = ispow2(r->ival);
			if(p2 > 0) {
				n->type = OALSH;
				r->ival = p2;
				rewrite(n);
				return;
			}
		}

		if(l->type == OCONST) {
			n->right = n->left;
			n->left = r;	
		}
		commute(n);
		l = n->left;
		r = n->right;
		break;

	case ODIV:
		rewrite(l);
		rewrite(r);

		/* Rewrite constant integer divides to shifts */
		if(r->type == OCONST)
		if(r->t->type != TFLOAT) {
			p2 = ispow2(r->ival);
			if(p2 <= 0)
				break;
			n->type = OARSH;
			r->ival = p2;
		}
		break;

	case OASGN:
	case OMOD:
	case OSUB:
	case OCAND:
	case OCOR:
	case OGT:
	case ONEQ:
	case ONOT:
	case OGEQ:
	case OLEQ:
	case OLT:
	case ORSH:
	case OLSH:
	case OARSH:
	case OALSH:
		rewrite(l);
		rewrite(r);
		break;

	case OADDR:
		rewrite(l);
		if(l->type == OIND)
			*n = *l->left;
		if(l->type == OCONST)
			*n = *l;
		return;

	case OIND:
		rewrite(l);
		t = n->t;			/* Maybe rewrite from DOT */
		if(l->type == OADDR)
			*n = *l->left;
		n->t = t;
		return;

	case OCAST:
		rewrite(l);
		return;

	case OCONV:
		rewrite(l);
		if(l->type == OCONST)
			convop(n);
		return;

	case ODOT:
		rewrite(l);
		return;

	case OCONST:
	case ONAME:
	case OFUNC:
	case OAGDECL:
	case OUNDECL:
	case OADTDECL:
		return;
	}

	/* Fold the constant operators */
	if(l->type == OCONST)
	if(r->type == OCONST)
		foldop(n);
}


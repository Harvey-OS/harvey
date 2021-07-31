#include "gc.h"

void
noretval(int n)
{

	if(n & 1) {
		gins(ANOP, Z, Z);
		p->to.type = REGRET;
	}
	if(n & 2) {
		gins(ANOP, Z, Z);
		p->to.type = FREGRET;
	}
	if((n&3) == 3)
	if(thisfn && thisfn->link && typefd[thisfn->link->etype])
		gins(AFLDZ, Z, Z);
}

/* welcome to commute */
static void
commute(Node *n)
{
	Node *l, *r;

	l = n->left;
	r = n->right;
	if(r->complex > l->complex) {
		n->left = r;
		n->right = l;
	}
}

void
indexshift(Node *n)
{
	int g;

	if(!typechlp[n->type->etype])
		return;
	simplifyshift(n);
	if(n->op == OASHL && n->right->op == OCONST){
		g = vconst(n->right);
		if(g >= 0 && g < 4)
			n->addable = 7;
	}
}

/*
 *	calculate addressability as follows
 *		NAME ==> 10/11		name+value(SB/SP)
 *		REGISTER ==> 12		register
 *		CONST ==> 20		$value
 *		*(20) ==> 21		value
 *		&(10) ==> 13		$name+value(SB)
 *		&(11) ==> 1		$name+value(SP)
 *		(13) + (20) ==> 13	fold constants
 *		(1) + (20) ==> 1	fold constants
 *		*(13) ==> 10		back to name
 *		*(1) ==> 11		back to name
 *
 *		(20) * (X) ==> 7	multiplier in indexing
 *		(X,7) + (13,1) ==> 8	adder in indexing (addresses)
 *		(8) ==> &9(OINDEX)	index, almost addressable
 *
 *	calculate complexity (number of registers)
 */
void
xcom(Node *n)
{
	Node *l, *r;
	int g;

	if(n == Z)
		return;
	l = n->left;
	r = n->right;
	n->complex = 0;
	n->addable = 0;
	switch(n->op) {
	case OCONST:
		n->addable = 20;
		break;

	case ONAME:
		n->addable = 10;
		if(n->class == CPARAM || n->class == CAUTO)
			n->addable = 11;
		break;

	case OEXREG:
		n->addable = 12;
		break;

	case OREGISTER:
		n->addable = 12;
		break;

	case OINDREG:
		n->addable = 12;
		break;

	case OADDR:
		xcom(l);
		if(l->addable == 10)
			n->addable = 13;
		else
		if(l->addable == 11)
			n->addable = 1;
		break;

	case OADD:
		xcom(l);
		xcom(r);
		if(n->type->etype != TIND)
			break;

		switch(r->addable) {
		case 20:
			switch(l->addable) {
			case 1:
			case 13:
			commadd:
				l->type = n->type;
				*n = *l;
				l = new(0, Z, Z);
				*l = *(n->left);
				l->xoffset += r->vconst;
				n->left = l;
				r = n->right;
				goto brk;
			}
			break;

		case 1:
		case 13:
		case 10:
		case 11:
			/* l is the base, r is the index */
			if(l->addable != 20)
				n->addable = 8;
			break;
		}
		switch(l->addable) {
		case 20:
			switch(r->addable) {
			case 13:
			case 1:
				r = n->left;
				l = n->right;
				n->left = l;
				n->right = r;
				goto commadd;
			}
			break;

		case 13:
		case 1:
		case 10:
		case 11:
			/* r is the base, l is the index */
			if(r->addable != 20)
				n->addable = 8;
			break;
		}
		if(n->addable == 8 && !side(n)) {
			indx(n);
			l = new1(OINDEX, idx.basetree, idx.regtree);
			l->scale = idx.scale;
			l->addable = 9;
			l->complex = l->right->complex;
			l->type = l->left->type;
			n->op = OADDR;
			n->left = l;
			n->right = Z;
			n->addable = 8;
			break;
		}
		break;

	case OINDEX:
		xcom(l);
		xcom(r);
		n->addable = 9;
		break;

	case OIND:
		xcom(l);
		if(l->op == OADDR) {
			l = l->left;
			l->type = n->type;
			*n = *l;
			return;
		}
		switch(l->addable) {
		case 20:
			n->addable = 21;
			break;
		case 1:
			n->addable = 11;
			break;
		case 13:
			n->addable = 10;
			break;
		}
		break;

	case OASHL:
		xcom(l);
		xcom(r);
		indexshift(n);
		break;

	case OMUL:
	case OLMUL:
		xcom(l);
		xcom(r);
		g = vlog(l);
		if(g >= 0) {
			n->left = r;
			n->right = l;
			l = r;
			r = n->right;
		}
		g = vlog(r);
		if(g >= 0) {
			n->op = OASHL;
			r->vconst = g;
			r->type = types[TINT];
			indexshift(n);
			break;
		}
commute(n);
		break;

	case OASLDIV:
		xcom(l);
		xcom(r);
		g = vlog(r);
		if(g >= 0) {
			n->op = OASLSHR;
			r->vconst = g;
			r->type = types[TINT];
		}
		break;

	case OLDIV:
		xcom(l);
		xcom(r);
		g = vlog(r);
		if(g >= 0) {
			n->op = OLSHR;
			r->vconst = g;
			r->type = types[TINT];
			indexshift(n);
			break;
		}
		break;

	case OASLMOD:
		xcom(l);
		xcom(r);
		g = vlog(r);
		if(g >= 0) {
			n->op = OASAND;
			r->vconst--;
		}
		break;

	case OLMOD:
		xcom(l);
		xcom(r);
		g = vlog(r);
		if(g >= 0) {
			n->op = OAND;
			r->vconst--;
		}
		break;

	case OASMUL:
	case OASLMUL:
		xcom(l);
		xcom(r);
		g = vlog(r);
		if(g >= 0) {
			n->op = OASASHL;
			r->vconst = g;
		}
		break;

	case OLSHR:
	case OASHR:
		xcom(l);
		xcom(r);
		indexshift(n);
		break;

	default:
		if(l != Z)
			xcom(l);
		if(r != Z)
			xcom(r);
		break;
	}
brk:
	if(n->addable >= 10)
		return;
	if(l != Z)
		n->complex = l->complex;
	if(r != Z) {
		if(r->complex == n->complex)
			n->complex = r->complex+1;
		else
		if(r->complex > n->complex)
			n->complex = r->complex;
	}
	if(n->complex == 0)
		n->complex++;

	if(com64(n))
		return;

	switch(n->op) {

	case OFUNC:
		n->complex = FNX;
		break;

	case OLMOD:
	case OMOD:
	case OLMUL:
	case OLDIV:
	case OMUL:
	case ODIV:
	case OASLMUL:
	case OASLDIV:
	case OASLMOD:
	case OASMUL:
	case OASDIV:
	case OASMOD:
		if(r->complex >= l->complex) {
			n->complex = l->complex + 3;
			if(r->complex > n->complex)
				n->complex = r->complex;
		} else {
			n->complex = r->complex + 3;
			if(l->complex > n->complex)
				n->complex = l->complex;
		}
		break;

	case OLSHR:
	case OASHL:
	case OASHR:
	case OASLSHR:
	case OASASHL:
	case OASASHR:
		if(r->complex >= l->complex) {
			n->complex = l->complex + 2;
			if(r->complex > n->complex)
				n->complex = r->complex;
		} else {
			n->complex = r->complex + 2;
			if(l->complex > n->complex)
				n->complex = l->complex;
		}
		break;

	case OADD:
	case OXOR:
	case OAND:
	case OOR:
		/*
		 * immediate operators, make const on right
		 */
		if(l->op == OCONST) {
			n->left = r;
			n->right = l;
		}
		break;

	case OEQ:
	case ONE:
	case OLE:
	case OLT:
	case OGE:
	case OGT:
	case OHI:
	case OHS:
	case OLO:
	case OLS:
		/*
		 * compare operators, make const on left
		 */
		if(r->op == OCONST) {
			n->left = r;
			n->right = l;
			n->op = invrel[relindex(n->op)];
		}
		break;
	}
}

void
indx(Node *n)
{
	Node *l, *r;

	if(debug['x'])
		prtree(n, "indx");

	l = n->left;
	r = n->right;
	if(l->addable == 1 || l->addable == 13 || r->complex > l->complex) {
		n->right = l;
		n->left = r;
		l = r;
		r = n->right;
	}
	if(l->addable != 7) {
		idx.regtree = l;
		idx.scale = 1;
	} else
	if(l->right->addable == 20) {
		idx.regtree = l->left;
		idx.scale = 1 << l->right->vconst;
	} else
	if(l->left->addable == 20) {
		idx.regtree = l->right;
		idx.scale = 1 << l->left->vconst;
	} else
		diag(n, "bad index");

	idx.basetree = r;
	if(debug['x']) {
		print("scale = %d\n", idx.scale);
		prtree(idx.regtree, "index");
		prtree(idx.basetree, "base");
	}
}

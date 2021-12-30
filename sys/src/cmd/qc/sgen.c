#include "gc.h"

void
noretval(int n)
{

	if(n & 1) {
		gins(ANOP, Z, Z);
		p->to.type = D_REG;
		p->to.reg = REGRET;
	}
	if(n & 2) {
		gins(ANOP, Z, Z);
		p->to.type = D_FREG;
		p->to.reg = FREGRET;
	}
}

/*
 *	calculate addressability as follows
 *		CONST ==> 20		$value
 *		NAME ==> 10		name
 *		REGISTER ==> 11		register
 *		INDREG ==> 12		*[(reg)+offset]
 *		&10 ==> 2		$name
 *		ADD(2, 20) ==> 2	$name+offset
 *		ADD(3, 20) ==> 3	$(reg)+offset
 *		&12 ==> 3		$(reg)+offset
 *		*11 ==> 11		??
 *		*2 ==> 10		name
 *		*3 ==> 12		*(reg)+offset
 *	calculate complexity (number of registers)
 */
void
xcom(Node *n)
{
	Node *l, *r;
	int v, nr;

	if(n == Z)
		return;
	l = n->left;
	r = n->right;
	n->addable = 0;
	n->complex = 0;
	switch(n->op) {
	case OCONST:
		n->addable = 20;
		return;

	case OREGISTER:
		n->addable = 11;
		return;

	case OINDREG:
		n->addable = 12;
		return;

	case ONAME:
		n->addable = 10;
		return;

	case OADDR:
		xcom(l);
		if(l->addable == 10)
			n->addable = 2;
		if(l->addable == 12)
			n->addable = 3;
		break;

	case OIND:
		xcom(l);
		if(l->addable == 11)
			n->addable = 12;
		if(l->addable == 3)
			n->addable = 12;
		if(l->addable == 2)
			n->addable = 10;
		break;

	case OADD:
		xcom(l);
		xcom(r);
		if(l->addable == 20) {
			if(r->addable == 2)
				n->addable = 2;
			if(r->addable == 3)
				n->addable = 3;
		}
		if(r->addable == 20) {
			if(l->addable == 2)
				n->addable = 2;
			if(l->addable == 3)
				n->addable = 3;
		}
		break;

	case OASMUL:
	case OASLMUL:
		xcom(l);
		xcom(r);
		v = vlog(r);
		if(v >= 0) {
			n->op = OASASHL;
			r->vconst = v;
			r->type = types[TINT];
		}
		break;

	case OMUL:
	case OLMUL:
		xcom(l);
		xcom(r);
		v = vlog(r);
		if(v >= 0) {
			n->op = OASHL;
			r->vconst = v;
			r->type = types[TINT];
		}
		v = vlog(l);
		if(v >= 0) {
			n->op = OASHL;
			n->left = r;
			n->right = l;
			r = l;
			l = n->left;
			r->vconst = v;
			r->type = types[TINT];
			simplifyshift(n);
		}
		break;

	case OASLDIV:
		xcom(l);
		xcom(r);
		v = vlog(r);
		if(v >= 0) {
			n->op = OASLSHR;
			r->vconst = v;
			r->type = types[TINT];
		}
		break;

	case OLDIV:
		xcom(l);
		xcom(r);
		v = vlog(r);
		if(v >= 0) {
			n->op = OLSHR;
			r->vconst = v;
			r->type = types[TINT];
			simplifyshift(n);
		}
		break;

	case OASLMOD:
		xcom(l);
		xcom(r);
		v = vlog(r);
		if(v >= 0) {
			n->op = OASAND;
			r->vconst--;
		}
		break;

	case OLMOD:
		xcom(l);
		xcom(r);
		v = vlog(r);
		if(v >= 0) {
			n->op = OAND;
			r->vconst--;
		}
		break;

	case OLSHR:
	case OASHL:
	case OASHR:
		xcom(l);
		xcom(r);
		simplifyshift(n);
		break;

	default:
		if(l != Z)
			xcom(l);
		if(r != Z)
			xcom(r);
		break;
	}
	if(n->addable >= 10)
		return;
	if(l != Z)
		n->complex = l->complex;
	if(r != Z) {
		nr = 1;
		if(r->type != T && typev[r->type->etype] || n->type != T && typev[n->type->etype]) {
			nr = 2;
			if(n->op == OMUL || n->op == OLMUL)
				nr += 3;
		}
		if(r->complex == n->complex)
			n->complex = r->complex+nr;
		else
		if(r->complex > n->complex)
			n->complex = r->complex;
	}
	if(n->complex == 0){
		n->complex++;
		if(n->type != T && typev[n->type->etype])
			n->complex++;
	}

	if(com64(n))
		return;

	switch(n->op) {

	case OFUNC:
		n->complex = FNX;
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
		 * immediate operators, make const on right
		 */
		if(l->op == OCONST) {
			n->left = r;
			n->right = l;
			n->op = invrel[relindex(n->op)];
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
	}
}


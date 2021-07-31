#include "gc.h"

void
codgen(Node *n, Node *nn)
{
	Prog *sp;
	Node *n1, nod, nod1;

	cursafe = 0;
	curarg = 0;
	maxargsafe = 0;

	/*
	 * isolate name
	 */
	for(n1 = nn;; n1 = n1->left) {
		if(n1 == Z) {
			diag(nn, "cant find function name");
			return;
		}
		if(n1->op == ONAME)
			break;
	}
	nearln = nn->lineno;
	gpseudo(ATEXT, n1->sym, nodconst(stkoff));
	sp = p;

	/*
	 * isolate first argument
	 */
	if(REGARG) {
		if(typesuv[thisfn->link->etype]) {
			nod1 = *nodret->left;
			nodreg(&nod, &nod1, REGARG);
			gopcode(OAS, &nod, Z, &nod1);
		} else
		if(firstarg && typechlp[firstargtype->etype]) {
			nod1 = *nodret->left;
			nod1.sym = firstarg;
			nod1.type = firstargtype;
			nod1.xoffset = align(0, firstargtype, Aarg1);
			nod1.etype = firstargtype->etype;
			nodreg(&nod, &nod1, REGARG);
			gopcode(OAS, &nod, Z, &nod1);
		}
	}

	retok = 0;
	gen(n);
	if(!retok)
		if(thisfn->link->etype != TVOID)
			warn(Z, "no return at end of function: %s", n1->sym->name);
	noretval(3);
	gbranch(ORETURN);

	if(!debug['N'] || debug['R'] || debug['P'])
		regopt(sp);

	sp->to.offset += maxargsafe;
}

void
gen(Node *n)
{
	Node *l, nod;
	Prog *sp, *spc, *spb;
	Case *cn;
	long sbc, scc;
	int o;

loop:
	if(n == Z)
		return;
	nearln = n->lineno;
	o = n->op;
	if(debug['G'])
		if(o != OLIST)
			print("%L %O\n", nearln, o);

	retok = 0;
	switch(o) {

	default:
		complex(n);
		cgen(n, Z);
		break;

	case OLIST:
		gen(n->left);

	rloop:
		n = n->right;
		goto loop;

	case ORETURN:
		retok = 1;
		complex(n);
		if(n->type == T)
			break;
		l = n->left;
		if(l == Z) {
			noretval(3);
			gbranch(ORETURN);
			break;
		}
		if(typesuv[n->type->etype]) {
			sugen(l, nodret, n->type->width);
			noretval(3);
			gbranch(ORETURN);
			break;
		}
		regret(&nod, n);
		cgen(l, &nod);
		regfree(&nod);
		if(typefd[n->type->etype])
			noretval(1);
		else
			noretval(2);
		gbranch(ORETURN);
		break;

	case OLABEL:
		l = n->left;
		if(l) {
			l->pc = pc;
			if(l->label)
				patch(l->label, pc);
		}
		gbranch(OGOTO);	/* prevent self reference in reg */
		patch(p, pc);
		goto rloop;

	case OGOTO:
		retok = 1;
		n = n->left;
		if(n == Z)
			return;
		if(n->complex == 0) {
			diag(Z, "label undefined: %s", n->sym->name);
			return;
		}
		gbranch(OGOTO);
		if(n->pc) {
			patch(p, n->pc);
			return;
		}
		if(n->label)
			patch(n->label, pc-1);
		n->label = p;
		return;

	case OCASE:
		l = n->left;
		if(cases == C)
			diag(n, "case/default outside a switch");
		if(l == Z) {
			cas();
			cases->val = 0;
			cases->def = 1;
			cases->label = pc;
			goto rloop;
		}
		complex(l);
		if(l->type == T)
			goto rloop;
		if(l->op == OCONST)
		if(typechl[l->type->etype]) {
			cas();
			cases->val = l->vconst;
			cases->def = 0;
			cases->label = pc;
			goto rloop;
		}
		diag(n, "case expression must be integer constant");
		goto rloop;

	case OSWITCH:
		l = n->left;
		complex(l);
		if(l->type == T)
			break;
		if(!typechl[l->type->etype]) {
			diag(n, "switch expression must be integer");
			break;
		}

		gbranch(OGOTO);		/* entry */
		sp = p;

		cn = cases;
		cases = C;
		cas();

		sbc = breakpc;
		breakpc = pc;
		gbranch(OGOTO);
		spb = p;

		gen(n->right);
		gbranch(OGOTO);
		patch(p, breakpc);

		patch(sp, pc);
		regalloc(&nod, l, Z);
		nod.type = types[TLONG];
		cgen(l, &nod);
		doswit(&nod);
		regfree(&nod);
		patch(spb, pc);

		cases = cn;
		breakpc = sbc;
		break;

	case OWHILE:
	case ODWHILE:
		l = n->left;
		gbranch(OGOTO);		/* entry */
		sp = p;

		scc = continpc;
		continpc = pc;
		gbranch(OGOTO);
		spc = p;

		sbc = breakpc;
		breakpc = pc;
		gbranch(OGOTO);
		spb = p;

		patch(spc, pc);
		if(n->op == OWHILE)
			patch(sp, pc);
		bcomplex(l);		/* test */
		patch(p, breakpc);

		if(n->op == ODWHILE)
			patch(sp, pc);
		gen(n->right);		/* body */
		gbranch(OGOTO);
		patch(p, continpc);

		patch(spb, pc);
		continpc = scc;
		breakpc = sbc;
		break;

	case OFOR:
		l = n->left;
		gen(l->right->left);	/* init */
		gbranch(OGOTO);		/* entry */
		sp = p;

		scc = continpc;
		continpc = pc;
		gbranch(OGOTO);
		spc = p;

		sbc = breakpc;
		breakpc = pc;
		gbranch(OGOTO);
		spb = p;

		patch(spc, pc);
		gen(l->right->right);	/* inc */
		patch(sp, pc);	
		if(l->left != Z) {	/* test */
			bcomplex(l->left);
			patch(p, breakpc);
		}
		gen(n->right);		/* body */
		gbranch(OGOTO);
		patch(p, continpc);

		patch(spb, pc);
		continpc = scc;
		breakpc = sbc;
		break;

	case OCONTINUE:
		if(continpc < 0) {
			diag(n, "continue not in a loop");
			break;
		}
		gbranch(OGOTO);
		patch(p, continpc);
		break;

	case OBREAK:
		if(breakpc < 0) {
			diag(n, "break not in a loop");
			break;
		}
		gbranch(OGOTO);
		patch(p, breakpc);
		break;

	case OIF:
		l = n->left;
		bcomplex(l);
		sp = p;
		if(n->right->left != Z)
			gen(n->right->left);
		if(n->right->right != Z) {
			gbranch(OGOTO);
			patch(sp, pc);
			sp = p;
			gen(n->right->right);
		}
		patch(sp, pc);
		break;

	case OSET:
	case OUSED:
		usedset(n->left, o);
		break;
	}
}

void
usedset(Node *n, int o)
{
	if(n->op == OLIST) {
		usedset(n->left, o);
		usedset(n->right, o);
		return;
	}
	complex(n);
	switch(n->op) {
	case OADDR:	/* volatile */
		gins(ANOP, n, Z);
		break;
	case ONAME:
		if(o == OSET)
			gins(ANOP, Z, n);
		else
			gins(ANOP, n, Z);
		break;
	}
}

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

void
testshift(Node *n)
{
	ulong c3;
	int o, s1, s2, c1, c2;

	if(!typechlp[n->type->etype])
		return;
	switch(n->op) {
	default:
		return;
	case OASHL:
		s1 = 0;
		break;
	case OLSHR:
		s1 = 1;
		break;
	case OASHR:
		s1 = 2;
		break;
	}
	if(n->right->op != OCONST)
		return;
	if(n->left->op != OAND)
		return;
	if(n->left->right->op != OCONST)
		return;
	switch(n->left->left->op) {
	default:
		return;
	case OASHL:
		s2 = 0;
		break;
	case OLSHR:
		s2 = 1;
		break;
	case OASHR:
		s2 = 2;
		break;
	}
	if(n->left->left->right->op != OCONST)
		return;

	c1 = n->right->vconst;
	c2 = n->left->left->right->vconst;
	c3 = n->left->right->vconst;

/*
	if(debug['h'])
		print("%.3o %ld %ld %d #%.lux\n",
			(s1<<3)|s2, c1, c2, topbit(c3), c3);
*/

	o = n->op;
	switch((s1<<3)|s2) {
	case 000:	/* (((e <<u c2) & c3) <<u c1) */
		c3 >>= c2;
		c1 += c2;
		if(c1 >= 32)
			break;
		goto rewrite1;

	case 002:	/* (((e >>s c2) & c3) <<u c1) */
		if(topbit(c3) >= (32-c2))
			break;
	case 001:	/* (((e >>u c2) & c3) <<u c1) */
		if(c1 > c2) {
			c3 <<= c2;
			c1 -= c2;
			o = OASHL;
			goto rewrite1;
		}
		c3 <<= c1;
		if(c1 == c2)
			goto rewrite0;
		c1 = c2-c1;
		o = OLSHR;
		goto rewrite2;

	case 022:	/* (((e >>s c2) & c3) >>s c1) */
		if(c2 <= 0)
			break;
	case 012:	/* (((e >>s c2) & c3) >>u c1) */
		if(topbit(c3) >= (32-c2))
			break;
		goto s11;
	case 021:	/* (((e >>u c2) & c3) >>s c1) */
		if(topbit(c3) >= 31 && c2 <= 0)
			break;
		goto s11;
	case 011:	/* (((e >>u c2) & c3) >>u c1) */
	s11:
		c3 <<= c2;
		c1 += c2;
		if(c1 >= 32)
			break;
		o = OLSHR;
		goto rewrite1;

	case 020:	/* (((e <<u c2) & c3) >>s c1) */
		if(topbit(c3) >= 31)
			break;
	case 010:	/* (((e <<u c2) & c3) >>u c1) */
		c3 >>= c1;
		if(c1 == c2)
			goto rewrite0;
		if(c1 > c2) {
			c1 -= c2;
			goto rewrite2;
		}
		c1 = c2 - c1;
		o = OASHL;
		goto rewrite2;
	}
	return;

rewrite0:	/* get rid of both shifts */
	*n = *n->left;
	n->left = n->left->left;
	n->right->vconst = c3;
	return;
rewrite1:	/* get rid of lower shift */
	n->left->left = n->left->left->left;
	n->left->right->vconst = c3;
	n->right->vconst = c1;
	n->op = o;
	return;
rewrite2:	/* get rid of upper shift */
	*n = *n->left;
	n->right->vconst = c3;
	n->left->right->vconst = c1;
	n->left->op = o;
	return;
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
	int t;

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

	case OASLMUL:
	case OASMUL:
		xcom(l);
		xcom(r);
		t = vlog(r);
		if(t >= 0) {
			n->op = OASASHL;
			r->vconst = t;
			r->type = types[TINT];
		}
		break;

	case OMUL:
	case OLMUL:
		xcom(l);
		xcom(r);
		t = vlog(r);
		if(t >= 0) {
			n->op = OASHL;
			r->vconst = t;
			r->type = types[TINT];
			goto shift;
		}
		t = vlog(l);
		if(t >= 0) {
			n->op = OASHL;
			n->left = r;
			n->right = l;
			r = l;
			l = n->left;
			r->vconst = t;
			r->type = types[TINT];
			goto shift;
		}
		break;

	case OASLDIV:
		xcom(l);
		xcom(r);
		t = vlog(r);
		if(t >= 0) {
			n->op = OASLSHR;
			r->vconst = t;
			r->type = types[TINT];
		}
		break;

	case OLDIV:
		xcom(l);
		xcom(r);
		t = vlog(r);
		if(t >= 0) {
			n->op = OLSHR;
			r->vconst = t;
			r->type = types[TINT];
			goto shift;
		}
		break;

	case OASLMOD:
		xcom(l);
		xcom(r);
		t = vlog(r);
		if(t >= 0) {
			n->op = OASAND;
			r->vconst--;
		}
		break;

	case OLMOD:
		xcom(l);
		xcom(r);
		t = vlog(r);
		if(t >= 0) {
			n->op = OAND;
			r->vconst--;
		}
		break;

	case OLSHR:
	case OASHL:
	case OASHR:
		xcom(l);
		xcom(r);
	shift:
		testshift(n);
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

	case OADD:
	case OXOR:
	case OAND:
	case OOR:
	case OEQ:
	case ONE:
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

void
bcomplex(Node *n)
{

	complex(n);
	if(n->type != T)
	if(tcompat(n, T, n->type, tnot))
		n->type = T;
	if(n->type != T) {
		bool64(n);
		boolgen(n, 1, Z);
	} else
		gbranch(OGOTO);
}

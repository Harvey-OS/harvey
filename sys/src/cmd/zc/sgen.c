#include "gc.h"

void
codgen(Node *n, Node *nn)
{
	Prog *sp;
	Node *n1;

	curreg = SZ_LONG;
	maxreg = 0;
	cursafe = 0;
	maxsafe = 0;
	for(n1 = nn;; n1 = n1->left) {
		if(n1 == Z) {
			diag(Z, "cant find function name");
			return;
		}
		if(n1->op == ONAME)
			break;
	}
	nearln = nn->lineno;
	gpseudo(ATEXT, n1->sym, nodconst(stkoff));
	sp = p;
	retok = 0;
	gen(n);
	if(!retok)
		if(thisfn->link->etype != TVOID)
			warn(Z, "no return at end of function: %s", n1->sym->name);
	gbranch(ORETURN);
	sp->to.offset += maxreg+maxsafe;
}

void
gen(Node *n)
{
	Node *l, nod, nod1;
	Prog *sp, *spc, *spb;
	Case *cn;
	long sbc, scc;

loop:
	if(n == Z)
		return;
	nearln = n->lineno;
	if(debug['G'])
		if(n->op != OLIST)
			print("%L %O\n", nearln, n->op);
	retok = 0;
	switch(n->op) {

	default:
		complex(n);
		doinc(n, PRE);
		cgen(n, Z);
		doinc(n, APOST);
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
			gbranch(ORETURN);
			break;
		}
		doinc(l, PRE);
		if(retalias(l)) {
			if(typesu[l->type->etype]) {
				sugen(l, nodrat, l->type->width);
				doinc(l, APOST);
				sugen(nodrat, nodret, l->type->width);
				gbranch(ORETURN);
				break;
			}
			regalloc(&nod1, l);
			cgen(l, &nod1);
			doinc(l, APOST);
			nod = *(nodret->left);
			nod.type = n->type;
			cgen(&nod1, &nod);
			gbranch(ORETURN);
			break;
		}
		if(typesu[l->type->etype]) {
			sugen(l, nodret, l->type->width);
			doinc(l, APOST);
			gbranch(ORETURN);
			break;
		}
		nod = *(nodret->left);
		nod.type = n->type;
		cgen(l, &nod);
		doinc(l, APOST);
		gbranch(ORETURN);
		break;

	case OLABEL:
		l = n->left;
		if(l) {
			l->offset = pc;
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
		if(n->offset) {
			patch(p, n->offset);
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
			cases->val = l->offset;
			cases->def = 0;
			cases->label = pc;
			goto rloop;
		}
		diag(n, "case expression must be integer constant");
		goto rloop;

	case OSWITCH:
		l = n->left;
		complex(l);
		doinc(l, PRE);
		if(l->type == T)
			break;
		if(!typechl[l->type->etype]) {
			diag(n, "switch expression must be integer");
			break;
		}
		regalloc(&nod, l);
		nod.type = types[TLONG];
		cgen(l, &nod);
		doinc(l, APOST);
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
		doswit(&nod);
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
		break;

	case OUSED:
		break;	/* no optimization, no problem */
		n = n->left;
		for(;;) {
			if(n->op == OLIST) {
				l = n->right;
				n = n->left;
				complex(l);
				if(l->type != T && !typesu[l->type->etype]) {
					regalloc(&nod, l);
					cgen(l, &nod);
				}
			} else {
				complex(n);
				if(n->type != T && !typesu[n->type->etype]) {
					regalloc(&nod, n);
					cgen(n, &nod);
				}
				break;
			}
		}
		break;
	}
}

int
retalias(Node *n)
{

	switch(n->op) {
	case ONAME:
		if(n->class == CPARAM)
		if(n->offset == 0)
			return 1;

	case OSTRING:
	case OREGISTER:
	case OINDREG:
	case OCONST:
		return 0;

	default:
		if(n->left)
		if(retalias(n->left))
			return 1;
		if(n->right)
		if(retalias(n->right))
			return 1;
		return 0;
	}
}

/*
 *	calculate addressability as follows
 *		CONST ==> 20		$value
 *		x-NAME ==> 10		name(SB)
 *		ADDR(10) ==> 12		$name(SB)
 *		ADD(12,20) ==> 12	$name+value(SB)
 *		IND(12) ==> 10		name+value(SB)
 *		s-NAME ==> 11		name(SP)
 *		IND(11) ==> 13		*name(SP)
 *		ADDR(11) ==> 2
 *		ADD(2,20) ==> 2
 *		IND(2) ==> 11		name+value(SP)
 *		IND(20) ==> 14		*$value
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
	switch(n->op) {
	case OCONST:
		n->addable = 20;
		break;

	case OREGISTER:
		n->addable = 11;
		break;

	case ONAME:
		n->addable = 11;
		if(n->class == CSTATIC ||
		   n->class == CLOCAL ||
		   n->class == CEXTERN ||
		   n->class == CGLOBL)
			n->addable = 10;
		break;

	case OASLMOD:
	case OASMOD:
	case OASXOR:
	case OASOR:
	case OASADD:
	case OASSUB:
	case OASLSHR:
	case OASASHL:
	case OASASHR:
	case OASAND:
	case OAS:
		xcom(r);

	case OPOSTINC:
	case OPREINC:
	case OPOSTDEC:
	case OPREDEC:
		xcom(l);
		if(l->addable > INDEXED)
			n->addable = l->addable;
		break;

	case OADDR:
		xcom(l);
		if(l->addable == 10)
			n->addable = 12;
		if(l->addable == 11)
			n->addable = 2;
		break;

	case OADD:
		xcom(l);
		xcom(r);
		if(l->addable == 20) {
			if(r->addable == 12)
				n->addable = 12;
			if(r->addable == 2)
				n->addable = 2;
		}
		if(r->addable == 20) {
			if(l->addable == 12)
				n->addable = 12;
			if(l->addable == 2)
				n->addable = 2;
		}
		break;

	case OIND:
		xcom(l);
		if(l->addable == 2)
			n->addable = 11;
		if(l->addable == 12)
			n->addable = 10;
		if(l->addable == 11)
			n->addable = 13;
		if(l->addable == 20)
			n->addable = 14;
		break;

	case OMUL:
	case OLMUL:
	case ODIV:
	case OLDIV:
		xcom(l);
		xcom(r);
		t = vlog(r);
		if(t >= 0) {
			if(n->op == ODIV)
				n->op = OASHR; else
			if(n->op == OLDIV)
				n->op = OLSHR; else
				n->op = OASHL;
			r->offset = t;
		}
		break;

	default:
		if(l != Z)
			xcom(l);
		if(r != Z)
			xcom(r);
		break;
	}
	n->complex = 0;
	if(n->addable >= 10)
		return;
	if(l != Z)
		n->complex = l->complex;
	if(r != Z) {
		if(r->complex == n->complex)
			n->complex = r->complex+1; else
		if(r->complex > n->complex)
			n->complex = r->complex;
	}
	if(n->complex == 0)
		n->complex++;
	switch(n->op) {

	case OFUNC:
		n->complex = FNX;
		break;

	case OADD:
	case OMUL:
	case OLMUL:
	case OXOR:
	case OAND:
	case OOR:
	case OEQ:
	case ONE:
		/*
		 * symmetric operators, make right side simple
		 */
		if(r->complex > l->complex) {
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
		doinc(n, PRE);
		boolgen(n, 1, Z);
	} else
		gbranch(OGOTO);
}

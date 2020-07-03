/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "gc.h"

void
codgen(Node *n, Node *nn)
{
	Prog *sp;
	Node *n1, nod, nod1;

	cursafe = 0;
	curarg = 0;
	maxargsafe = 0;
	hasdoubled = 0;

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

	if(typecmplx[thisfn->link->etype]) {
		if(nodret == nil) {
			nodret = new(ONAME, Z, Z);
			nodret->sym = slookup(".ret");
			nodret->class = CPARAM;
			nodret->type = types[TIND];
			nodret->etype = TIND;
			nodret = new(OIND, nodret, Z);
		}
		n1 = nodret->left;
		if(n1->type == T || n1->type->link != thisfn->link) {
			n1->type = typ(TIND, thisfn->link);
			n1->etype = n1->type->etype;
			nodret = new(OIND, n1, Z);
			complex(nodret);
		}
	}

	/*
	 * isolate first argument
	 */
	if(REGARG >= 0) {	
		if(typecmplx[thisfn->link->etype]) {
			nod1 = *nodret->left;
			nodreg(&nod, &nod1, REGARG);
			gmove(&nod, &nod1);
		} else
		if(firstarg && typeword[firstargtype->etype]) {
			nod1 = znode;
			nod1.op = ONAME;
			nod1.sym = firstarg;
			nod1.type = firstargtype;
			nod1.class = CPARAM;
			nod1.xoffset = align(0, firstargtype, Aarg1);
			nod1.etype = firstargtype->etype;
			xcom(&nod1);
			nodreg(&nod, &nod1, REGARG);
			gmove(&nod, &nod1);
		}
	}

	canreach = 1;
	warnreach = 1;
	gen(n);
	if(canreach && thisfn->link->etype != TVOID){
		if(debug['B'])
			warn(Z, "no return at end of function: %s", n1->sym->name);
		else
			diag(Z, "no return at end of function: %s", n1->sym->name);
	}
	noretval(3);
	gbranch(ORETURN);

	if(!debug['N'] || debug['R'] || debug['P'])
		regopt(sp);
	
	if(thechar=='6' || thechar=='7' || thechar=='9' || hasdoubled)	/* [sic] */
		maxargsafe = round(maxargsafe, 8);
	sp->to.offset += maxargsafe;
}

void
supgen(Node *n)
{
	int owarn;
	long spc;
	Prog *sp;

	if(n == Z)
		return;
	suppress++;
	owarn = warnreach;
	warnreach = 0;
	spc = pc;
	sp = lastp;
	gen(n);
	lastp = sp;
	pc = spc;
	sp->link = nil;
	suppress--;
	warnreach = owarn;
}

Node*
uncomma(Node *n)
{
	while(n != Z && n->op == OCOMMA) {
		cgen(n->left, Z);
		n = n->right;
	}
	return n;
}

void
gen(Node *n)
{
	Node *l, nod, rn;
	Prog *sp, *spc, *spb;
	Case *cn;
	long sbc, scc;
	int snbreak, sncontin;
	int f, o, oldreach;

loop:
	if(n == Z)
		return;
	nearln = n->lineno;
	o = n->op;
	if(debug['G'])
		if(o != OLIST)
			print("%L %O\n", nearln, o);

	if(!canreach) {
		switch(o) {
		case OLABEL:
		case OCASE:
		case OLIST:
		case OCOMMA:
		case OBREAK:
		case OFOR:
		case OWHILE:
		case ODWHILE:
			/* all handled specially - see switch body below */
			break;
		default:
			if(warnreach) {
				warn(n, "unreachable code %O", o);
				warnreach = 0;
			}
		}
	}

	switch(o) {

	default:
		complex(n);
		cgen(n, Z);
		break;

	case OLIST:
	case OCOMMA:
		gen(n->left);

	rloop:
		n = n->right;
		goto loop;

	case ORETURN:
		canreach = 0;
		warnreach = !suppress;
		complex(n);
		if(n->type == T)
			break;
		l = uncomma(n->left);
		if(l == Z) {
			noretval(3);
			gbranch(ORETURN);
			break;
		}
		if(typecmplx[n->type->etype]) {
			nod = znode;
			nod.op = OAS;
			nod.left = nodret;
			nod.right = l;
			nod.type = n->type;
			nod.complex = l->complex;
			cgen(&nod, Z);
			noretval(3);
			gbranch(ORETURN);
			break;
		}
		if(newvlongcode && !typefd[n->type->etype]){
			regret(&rn, n);
			regfree(&rn);
			nod = znode;
			nod.op = OAS;
			nod.left = &rn;
			nod.right = l;
			nod.type = n->type;
			nod.complex = l->complex;
			cgen(&nod, Z);
			noretval(2);
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
		canreach = 1;
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
		canreach = 0;
		warnreach = !suppress;
		n = n->left;
		if(n == Z)
			return;
		if(n->complex == 0) {
			diag(Z, "label undefined: %s", n->sym->name);
			return;
		}
		if(suppress)
			return;
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
		canreach = 1;
		l = n->left;
		if(cases == C)
			diag(n, "case/default outside a switch");
		if(l == Z) {
			casf();
			cases->val = 0;
			cases->def = 1;
			cases->label = pc;
			cases->isv = 0;
			goto rloop;
		}
		complex(l);
		if(l->type == T)
			goto rloop;
		if(l->op != OCONST || !typeswitch[l->type->etype]) {
			diag(n, "case expression must be integer constant");
			goto rloop;
		}
		casf();
		cases->val = l->vconst;
		cases->def = 0;
		cases->label = pc;
		cases->isv = typev[l->type->etype];
		goto rloop;

	case OSWITCH:
		l = n->left;
		complex(l);
		if(l->type == T)
			break;
		if(!typeswitch[l->type->etype]) {
			diag(n, "switch expression must be integer");
			break;
		}

		gbranch(OGOTO);		/* entry */
		sp = p;

		cn = cases;
		cases = C;
		casf();

		sbc = breakpc;
		breakpc = pc;
		snbreak = nbreak;
		nbreak = 0;
		gbranch(OGOTO);
		spb = p;

		gen(n->right);		/* body */
		if(canreach){
			gbranch(OGOTO);
			patch(p, breakpc);
			nbreak++;
		}

		patch(sp, pc);
		regalloc(&nod, l, Z);
		/* always signed */
		if(typev[l->type->etype])
			nod.type = types[TVLONG];
		else
			nod.type = types[TLONG];
		cgen(l, &nod);
		doswit(&nod);
		regfree(&nod);
		patch(spb, pc);

		cases = cn;
		breakpc = sbc;
		canreach = nbreak!=0;
		if(canreach == 0)
			warnreach = !suppress;
		nbreak = snbreak;
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
		snbreak = nbreak;
		nbreak = 0;
		gbranch(OGOTO);
		spb = p;

		patch(spc, pc);
		if(n->op == OWHILE)
			patch(sp, pc);
		bcomplex(l, Z);		/* test */
		patch(p, breakpc);
		if(l->op != OCONST || vconst(l) == 0)
			nbreak++;

		if(n->op == ODWHILE)
			patch(sp, pc);
		gen(n->right);		/* body */
		gbranch(OGOTO);
		patch(p, continpc);

		patch(spb, pc);
		continpc = scc;
		breakpc = sbc;
		canreach = nbreak!=0;
		if(canreach == 0)
			warnreach = !suppress;
		nbreak = snbreak;
		break;

	case OFOR:
		l = n->left;
		if(!canreach && l->right->left && warnreach) {
			warn(n, "unreachable code FOR");
			warnreach = 0;
		}
		gen(l->right->left);	/* init */
		gbranch(OGOTO);		/* entry */
		sp = p;

		/* 
		 * if there are no incoming labels in the 
		 * body and the top's not reachable, warn
		 */
		if(!canreach && warnreach && deadheads(n)) {
			warn(n, "unreachable code %O", o);
			warnreach = 0;
		}

		scc = continpc;
		continpc = pc;
		gbranch(OGOTO);
		spc = p;

		sbc = breakpc;
		breakpc = pc;
		snbreak = nbreak;
		nbreak = 0;
		sncontin = ncontin;
		ncontin = 0;
		gbranch(OGOTO);
		spb = p;

		patch(spc, pc);
		gen(l->right->right);	/* inc */
		patch(sp, pc);	
		if(l->left != Z) {	/* test */
			bcomplex(l->left, Z);
			patch(p, breakpc);
			if(l->left->op != OCONST || vconst(l->left) == 0)
				nbreak++;
		}
		canreach = 1;
		gen(n->right);		/* body */
		if(canreach){
			gbranch(OGOTO);
			patch(p, continpc);
			ncontin++;
		}
		if(!ncontin && l->right->right && warnreach) {
			warn(l->right->right, "unreachable FOR inc");
			warnreach = 0;
		}

		patch(spb, pc);
		continpc = scc;
		breakpc = sbc;
		canreach = nbreak!=0;
		if(canreach == 0)
			warnreach = !suppress;
		nbreak = snbreak;
		ncontin = sncontin;
		break;

	case OCONTINUE:
		if(continpc < 0) {
			diag(n, "continue not in a loop");
			break;
		}
		gbranch(OGOTO);
		patch(p, continpc);
		ncontin++;
		canreach = 0;
		warnreach = !suppress;
		break;

	case OBREAK:
		if(breakpc < 0) {
			diag(n, "break not in a loop");
			break;
		}
		/*
		 * Don't complain about unreachable break statements.
		 * There are breaks hidden in yacc's output and some people
		 * write return; break; in their switch statements out of habit.
		 * However, don't confuse the analysis by inserting an 
		 * unreachable reference to breakpc either.
		 */
		if(!canreach)
			break;
		gbranch(OGOTO);
		patch(p, breakpc);
		nbreak++;
		canreach = 0;
		warnreach = !suppress;
		break;

	case OIF:
		l = n->left;
		if(bcomplex(l, n->right)) {
			if(typefd[l->type->etype])
				f = !l->fconst;
			else
				f = !l->vconst;
			if(debug['c'])
				print("%L const if %s\n", nearln, f ? "false" : "true");
			if(f) {
				canreach = 1;
				supgen(n->right->left);
				oldreach = canreach;
				canreach = 1;
				gen(n->right->right);
				/*
				 * treat constant ifs as regular ifs for 
				 * reachability warnings.
				 */
				if(!canreach && oldreach && debug['w'] < 2)
					warnreach = 0;
			}
			else {
				canreach = 1;
				gen(n->right->left);
				oldreach = canreach;
				canreach = 1;
				supgen(n->right->right);
				/*
				 * treat constant ifs as regular ifs for 
				 * reachability warnings.
				 */
				if(!oldreach && canreach && debug['w'] < 2)
					warnreach = 0;
				canreach = oldreach;
			}
		}
		else {
			sp = p;
			canreach = 1;
			if(n->right->left != Z)
				gen(n->right->left);
			oldreach = canreach;
			canreach = 1;
			if(n->right->right != Z) {
				gbranch(OGOTO);
				patch(sp, pc);
				sp = p;
				gen(n->right->right);
			}
			patch(sp, pc);
			canreach = canreach || oldreach;
			if(canreach == 0)
				warnreach = !suppress;
		}
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

int
bcomplex(Node *n, Node *c)
{
	Node *b, nod;


	complex(n);
	if(n->type != T)
	if(tcompat(n, T, n->type, tnot))
		n->type = T;
	if(n->type == T) {
		gbranch(OGOTO);
		return 0;
	}
	if(c != Z && n->op == OCONST && deadheads(c))
		return 1;
	/* this is not quite right yet, so ignore it for now */
	if(0 && newvlongcode && typev[n->type->etype] && machcap(Z)) {
		b = &nod;
		b->op = ONE;
		b->left = n;
		b->right = new(0, Z, Z);
		*b->right = *nodconst(0);
		b->right->type = n->type;
		b->type = types[TLONG];
		cgen(b, Z);
		return 0;
	}
	bool64(n);
	boolgen(n, 1, Z);
	return 0;
}

#include "gc.h"

static void cmpv(Node*, int, Node*);
static void testv(Node*, int);
static void cgen64(Node*, Node*);
static int isvconstable(int, vlong);

void
cgen(Node *n, Node *nn)
{
	Node *l, *r;
	Prog *p1;
	Node nod, nod1, nod2, nod3, nod4;
	int o;
	long v, curs;

	if(debug['g']) {
		prtree(nn, "cgen lhs");
		prtree(n, "cgen");
	}
	if(n == Z || n->type == T)
		return;
	if(typesu[n->type->etype]) {
		sugen(n, nn, n->type->width);
		return;
	}
	if(typev[n->type->etype]) {
		switch(n->op) {
		case OCONST:
		case OFUNC:
			cgen64(n, nn);
			return;
		}
	}
	l = n->left;
	r = n->right;
	o = n->op;
	if(n->addable >= INDEXED) {
		if(nn == Z) {
			switch(o) {
			default:
				nullwarn(Z, Z);
				break;
			case OINDEX:
				nullwarn(l, r);
				break;
			}
			return;
		}
		gmove(n, nn);
		return;
	}
	curs = cursafe;

	if(n->complex >= FNX)
	if(l->complex >= FNX)
	if(r != Z && r->complex >= FNX)
	switch(o) {
	default:
		if(!typev[r->type->etype]) {
			regret(&nod, r);
			cgen(r, &nod);
			regsalloc(&nod1, r);
			gmove(&nod, &nod1);
			regfree(&nod);
		} else {
			regsalloc(&nod1, r);
			cgen(r, &nod1);
		}

		nod = *n;
		nod.right = &nod1;
		cgen(&nod, nn);
		return;

	case OFUNC:
	case OCOMMA:
	case OANDAND:
	case OOROR:
	case OCOND:
	case ODOT:
		break;
	}

	switch(o) {
	default:
		diag(n, "unknown op in cgen: %O", o);
		break;

	case ONEG:
	case OCOM:
		if(nn == Z) {
			nullwarn(l, Z);
			break;
		}
		regalloc(&nod, l, nn);
		cgen(l, &nod);
		gopcode(o, &nod, Z, &nod);
		gmove(&nod, nn);
		regfree(&nod);
		break;

	case OAS:
		if(l->op == OBIT)
			goto bitas;
		if(l->addable >= INDEXED) {
			if(nn != Z || r->addable < INDEXED) {
				regalloc(&nod, r, nn);
				cgen(r, &nod);
				gmove(&nod, l);
				regfree(&nod);
			} else
				gmove(r, l);
			break;
		}
		if(l->complex >= r->complex) {
			reglcgen(&nod1, l, Z);
			if(r->addable >= INDEXED) {
				gmove(r, &nod1);
				if(nn != Z)
					gmove(r, nn);
				regfree(&nod1);
				break;
			}
			regalloc(&nod, r, nn);
			cgen(r, &nod);
		} else {
			regalloc(&nod, r, nn);
			cgen(r, &nod);
			reglcgen(&nod1, l, Z);
		}
		gmove(&nod, &nod1);
		regfree(&nod);
		regfree(&nod1);
		break;

	bitas:
		n = l->left;
		regalloc(&nod, r, nn);
		if(l->complex >= r->complex) {
			reglcgen(&nod1, n, Z);
			cgen(r, &nod);
		} else {
			cgen(r, &nod);
			reglcgen(&nod1, n, Z);
		}
		regalloc(&nod2, n, Z);
		gopcode(OAS, &nod1, Z, &nod2);
		bitstore(l, &nod, &nod1, &nod2, nn);
		break;

	case OBIT:
		if(nn == Z) {
			nullwarn(l, Z);
			break;
		}
		bitload(n, &nod, Z, Z, nn);
		gopcode(OAS, &nod, Z, nn);
		regfree(&nod);
		break;

	case OXOR:
		if(nn != Z)
		if(r->op == OCONST && r->vconst == -1){
			regalloc(&nod, l, nn);
			cgen(l, &nod);
			gopcode(OCOM, &nod, Z, &nod);
			gmove(&nod, nn);
			regfree(&nod);
			break;
		}

	case OADD:
	case OSUB:
	case OAND:
	case OOR:
	case OLSHR:
	case OASHL:
	case OASHR:
		/*
		 * immediate operands
		 */
		if(nn != Z && r->op == OCONST && !typefd[n->type->etype] &&
		    (!typev[n->type->etype] || isvconstable(o, r->vconst))) {
			regalloc(&nod, l, nn);
			cgen(l, &nod);
			if(o == OAND || r->vconst != 0)
				gopcode(o, r, Z, &nod);
			gmove(&nod, nn);
			regfree(&nod);
			break;
		}

	case OMUL:
	case OLMUL:
	case OLDIV:
	case OLMOD:
	case ODIV:
	case OMOD:
		if(nn == Z) {
			nullwarn(l, r);
			break;
		}
		if((o == OMUL || o == OLMUL) && !typev[n->type->etype]) {
			if(mulcon(n, nn))
				break;
			if(debug['M'])
				print("%L multiply\n", n->lineno);
		}
		if(l->complex >= r->complex) {
			regalloc(&nod, l, nn);
			cgen(l, &nod);
			if(o != OMUL || typev[n->type->etype] || !sconst(r)) {
				regalloc(&nod1, r, Z);
				cgen(r, &nod1);
				gopcode(o, &nod1, Z, &nod);
				regfree(&nod1);
			} else
				gopcode(o, r, Z, &nod);
		} else {
			regalloc(&nod1, r, nn);
			cgen(r, &nod1);
			regalloc(&nod, l, Z);
			cgen(l, &nod);
			gopcode(o, &nod1, Z, &nod);
			regfree(&nod1);
		}
		gopcode(OAS, &nod, Z, nn);
		regfree(&nod);
		break;

	case OASLSHR:
	case OASASHL:
	case OASASHR:
	case OASAND:
	case OASADD:
	case OASSUB:
	case OASXOR:
	case OASOR:
		if(l->op == OBIT)
			goto asbitop;
		if(r->op == OCONST && !typefd[r->type->etype] && !typefd[n->type->etype] &&
		   (!typev[n->type->etype] || isvconstable(o, r->vconst))) {
			if(l->addable < INDEXED)
				reglcgen(&nod2, l, Z);
			else
				nod2 = *l;
			regalloc(&nod, l, nn);
			gopcode(OAS, &nod2, Z, &nod);
			gopcode(o, r, Z, &nod);
			gopcode(OAS, &nod, Z, &nod2);
	
			regfree(&nod);
			if(l->addable < INDEXED)
				regfree(&nod2);
			break;
		}

	case OASLMUL:
	case OASLDIV:
	case OASLMOD:
	case OASMUL:
	case OASDIV:
	case OASMOD:
		if(l->op == OBIT)
			goto asbitop;
		if(l->complex >= r->complex) {
			if(l->addable < INDEXED)
				reglcgen(&nod2, l, Z);
			else
				nod2 = *l;
			regalloc(&nod, r, Z);
			cgen(r, &nod);
		} else {
			regalloc(&nod, r, Z);
			cgen(r, &nod);
			if(l->addable < INDEXED)
				reglcgen(&nod2, l, Z);
			else
				nod2 = *l;
		}
		regalloc(&nod1, n, nn);
		gopcode(OAS, &nod2, Z, &nod1);
		gopcode(o, &nod, Z, &nod1);
		gopcode(OAS, &nod1, Z, &nod2);
		if(nn != Z)
			gopcode(OAS, &nod1, Z, nn);
		regfree(&nod);
		regfree(&nod1);
		if(l->addable < INDEXED)
			regfree(&nod2);
		break;

	asbitop:
		regalloc(&nod4, n, nn);
		regalloc(&nod3, r, Z);
		if(l->complex >= r->complex) {
			bitload(l, &nod, &nod1, &nod2, &nod4);
			cgen(r, &nod3);
		} else {
			cgen(r, &nod3);
			bitload(l, &nod, &nod1, &nod2, &nod4);
		}
		gmove(&nod, &nod4);
		gopcode(n->op, &nod3, Z, &nod4);
		regfree(&nod3);
		gmove(&nod4, &nod);
		regfree(&nod4);
		bitstore(l, &nod, &nod1, &nod2, nn);
		break;

	case OADDR:
		if(nn == Z) {
			nullwarn(l, Z);
			break;
		}
		lcgen(l, nn);
		break;

	case OFUNC:
		if(l->complex >= FNX) {
			if(l->op != OIND)
				diag(n, "bad function call");

			regret(&nod, l->left);
			cgen(l->left, &nod);
			regsalloc(&nod1, l->left);
			gopcode(OAS, &nod, Z, &nod1);
			regfree(&nod);

			nod = *n;
			nod.left = &nod2;
			nod2 = *l;
			nod2.left = &nod1;
			nod2.complex = 1;
			cgen(&nod, nn);

			return;
		}
		o = reg[REGARG];
		gargs(r, &nod, &nod1);
		if(l->addable < INDEXED) {
			reglcgen(&nod, l, Z);
			gopcode(OFUNC, Z, Z, &nod);
			regfree(&nod);
		} else
			gopcode(OFUNC, Z, Z, l);
		if(REGARG)
			if(o != reg[REGARG])
				reg[REGARG]--;
		if(nn != Z) {
			regret(&nod, n);
			gopcode(OAS, &nod, Z, nn);
			regfree(&nod);
		}
		break;

	case OIND:
		if(nn == Z) {
			cgen(l, nn);
			break;
		}
		regialloc(&nod, n, nn);
		r = l;
		while(r->op == OADD)
			r = r->right;
		if(sconst(r)) {
			v = r->vconst;
			r->vconst = 0;
			cgen(l, &nod);
			nod.xoffset += v;
			r->vconst = v;
		} else
			cgen(l, &nod);
		regind(&nod, n);
		gmove(&nod, nn);
		regfree(&nod);
		break;

	case OEQ:
	case ONE:
	case OLE:
	case OLT:
	case OGE:
	case OGT:
	case OLO:
	case OLS:
	case OHI:
	case OHS:
		if(nn == Z) {
			nullwarn(l, r);
			break;
		}
		boolgen(n, 1, nn);
		break;

	case OANDAND:
	case OOROR:
		boolgen(n, 1, nn);
		if(nn == Z)
			patch(p, pc);
		break;

	case ONOT:
		if(nn == Z) {
			nullwarn(l, Z);
			break;
		}
		boolgen(n, 1, nn);
		break;

	case OCOMMA:
		cgen(l, Z);
		cgen(r, nn);
		break;

	case OCAST:
		if(nn == Z) {
			nullwarn(l, Z);
			break;
		}
		/*
		 * convert from types l->n->nn
		 */
		if(nocast(l->type, n->type) && nocast(n->type, nn->type)) {
			/* both null, gen l->nn */
			cgen(l, nn);
			break;
		}
		if(typev[l->type->etype] || typev[n->type->etype]) {
			cgen64(n, nn);
			break;
		}
		regalloc(&nod, l, nn);
		cgen(l, &nod);
		regalloc(&nod1, n, &nod);
		gmove(&nod, &nod1);
		gmove(&nod1, nn);
		regfree(&nod1);
		regfree(&nod);
		break;

	case ODOT:
		sugen(l, nodrat, l->type->width);
		if(nn != Z) {
			warn(n, "non-interruptable temporary");
			nod = *nodrat;
			if(!r || r->op != OCONST) {
				diag(n, "DOT and no offset");
				break;
			}
			nod.xoffset += (long)r->vconst;
			nod.type = n->type;
			cgen(&nod, nn);
		}
		break;

	case OCOND:
		bcgen(l, 1);
		p1 = p;
		cgen(r->left, nn);
		gbranch(OGOTO);
		patch(p1, pc);
		p1 = p;
		cgen(r->right, nn);
		patch(p1, pc);
		break;

	case OPOSTINC:
	case OPOSTDEC:
		v = 1;
		if(l->type->etype == TIND)
			v = l->type->link->width;
		if(o == OPOSTDEC)
			v = -v;
		if(l->op == OBIT)
			goto bitinc;
		if(nn == Z)
			goto pre;

		if(l->addable < INDEXED)
			reglcgen(&nod2, l, Z);
		else
			nod2 = *l;

		regalloc(&nod, l, nn);
		gopcode(OAS, &nod2, Z, &nod);
		regalloc(&nod1, l, Z);
		if(typefd[l->type->etype]) {
			regalloc(&nod3, l, Z);
			if(v < 0) {
				gopcode(OAS, nodfconst(-v), Z, &nod3);
				gopcode(OSUB, &nod3, &nod, &nod1);
			} else {
				gopcode(OAS, nodfconst(v), Z, &nod3);
				gopcode(OADD, &nod3, &nod, &nod1);
			}
			regfree(&nod3);
		} else
			gopcode(OADD, nodconst(v), &nod, &nod1);
		gopcode(OAS, &nod1, Z, &nod2);

		regfree(&nod);
		regfree(&nod1);
		if(l->addable < INDEXED)
			regfree(&nod2);
		break;

	case OPREINC:
	case OPREDEC:
		v = 1;
		if(l->type->etype == TIND)
			v = l->type->link->width;
		if(o == OPREDEC)
			v = -v;
		if(l->op == OBIT)
			goto bitinc;

	pre:
		if(l->addable < INDEXED)
			reglcgen(&nod2, l, Z);
		else
			nod2 = *l;

		regalloc(&nod, l, nn);
		gopcode(OAS, &nod2, Z, &nod);
		if(typefd[l->type->etype]) {
			regalloc(&nod3, l, Z);
			if(v < 0) {
				gopcode(OAS, nodfconst(-v), Z, &nod3);
				gopcode(OSUB, &nod3, Z, &nod);
			} else {
				gopcode(OAS, nodfconst(v), Z, &nod3);
				gopcode(OADD, &nod3, Z, &nod);
			}
			regfree(&nod3);
		} else
			gopcode(OADD, nodconst(v), Z, &nod);
		gopcode(OAS, &nod, Z, &nod2);
		if(nn && l->op == ONAME)	/* in x=++i, emit USED(i) */
			gins(ANOP, l, Z);

		regfree(&nod);
		if(l->addable < INDEXED)
			regfree(&nod2);
		break;

	bitinc:
		if(nn != Z && (o == OPOSTINC || o == OPOSTDEC)) {
			bitload(l, &nod, &nod1, &nod2, Z);
			gopcode(OAS, &nod, Z, nn);
			gopcode(OADD, nodconst(v), Z, &nod);
			bitstore(l, &nod, &nod1, &nod2, Z);
			break;
		}
		bitload(l, &nod, &nod1, &nod2, nn);
		gopcode(OADD, nodconst(v), Z, &nod);
		bitstore(l, &nod, &nod1, &nod2, nn);
		break;
	}
	cursafe = curs;
}

void
reglcgen(Node *t, Node *n, Node *nn)
{
	Node *r;
	long v;

	regialloc(t, n, nn);
	if(n->op == OIND) {
		r = n->left;
		while(r->op == OADD)
			r = r->right;
		if(sconst(r)) {
			v = r->vconst;
			r->vconst = 0;
			lcgen(n, t);
			t->xoffset += v;
			r->vconst = v;
			regind(t, n);
			return;
		}
	}
	lcgen(n, t);
	regind(t, n);
}

void
lcgen(Node *n, Node *nn)
{
	Prog *p1;
	Node nod;

	if(debug['g']) {
		prtree(nn, "lcgen lhs");
		prtree(n, "lcgen");
	}
	if(n == Z || n->type == T)
		return;
	if(nn == Z) {
		nn = &nod;
		regalloc(&nod, n, Z);
	}
	switch(n->op) {
	default:
		if(n->addable < INDEXED) {
			diag(n, "unknown op in lcgen: %O", n->op);
			break;
		}
		nod = *n;
		nod.op = OADDR;
		nod.left = n;
		nod.right = Z;
		nod.type = types[TIND];
		gopcode(OAS, &nod, Z, nn);
		break;

	case OCOMMA:
		cgen(n->left, n->left);
		lcgen(n->right, nn);
		break;

	case OIND:
		cgen(n->left, nn);
		break;

	case OCOND:
		bcgen(n->left, 1);
		p1 = p;
		lcgen(n->right->left, nn);
		gbranch(OGOTO);
		patch(p1, pc);
		p1 = p;
		lcgen(n->right->right, nn);
		patch(p1, pc);
		break;
	}
}

void
bcgen(Node *n, int true)
{

	if(n->type == T)
		gbranch(OGOTO);
	else
		boolgen(n, true, Z);
}

void
boolgen(Node *n, int true, Node *nn)
{
	int o, uns;
	Prog *p1, *p2;
	Node *l, *r, nod, nod1;
	long curs;

	if(debug['g']) {
		prtree(nn, "boolgen lhs");
		prtree(n, "boolgen");
	}
	uns = 0;
	curs = cursafe;
	l = n->left;
	r = n->right;
	switch(n->op) {

	default:
		if(n->op == OCONST) {
			o = vconst(n);
			if(!true)
				o = !o;
			gbranch(OGOTO);
			if(o) {
				p1 = p;
				gbranch(OGOTO);
				patch(p1, pc);
			}
			goto com;
		}
		if(typev[n->type->etype]) {
			testv(n, true);
			goto com;
		}
		regalloc(&nod, n, nn);
		cgen(n, &nod);
		o = ONE;
		if(true)
			o = comrel[relindex(o)];
		if(typefd[n->type->etype]) {
			nodreg(&nod1, n, NREG+FREGZERO);
			gopcode(o, &nod, Z, &nod1);
		} else
			gopcode(o, &nod, Z, nodconst(0));
		regfree(&nod);
		goto com;

	case OCOMMA:
		cgen(l, Z);
		boolgen(r, true, nn);
		break;

	case ONOT:
		boolgen(l, !true, nn);
		break;

	case OCOND:
		bcgen(l, 1);
		p1 = p;
		bcgen(r->left, true);
		p2 = p;
		gbranch(OGOTO);
		patch(p1, pc);
		p1 = p;
		bcgen(r->right, !true);
		patch(p2, pc);
		p2 = p;
		gbranch(OGOTO);
		patch(p1, pc);
		patch(p2, pc);
		goto com;

	case OANDAND:
		if(!true)
			goto caseor;

	caseand:
		bcgen(l, true);
		p1 = p;
		bcgen(r, !true);
		p2 = p;
		patch(p1, pc);
		gbranch(OGOTO);
		patch(p2, pc);
		goto com;

	case OOROR:
		if(!true)
			goto caseand;

	caseor:
		bcgen(l, !true);
		p1 = p;
		bcgen(r, !true);
		p2 = p;
		gbranch(OGOTO);
		patch(p1, pc);
		patch(p2, pc);
		goto com;

	case OHI:
	case OHS:
	case OLO:
	case OLS:
		uns = 1;
		/* fall through */
	case OEQ:
	case ONE:
	case OLE:
	case OLT:
	case OGE:
	case OGT:
		if(typev[l->type->etype]){
			cmpv(n, true, Z);
			goto com;
		}
		o = n->op;
		if(true)
			o = comrel[relindex(o)];
		if(l->complex >= FNX && r->complex >= FNX) {
			regret(&nod, r);
			cgen(r, &nod);
			regsalloc(&nod1, r);
			gopcode(OAS, &nod, Z, &nod1);
			regfree(&nod);
			nod = *n;
			nod.right = &nod1;
			boolgen(&nod, true, nn);
			break;
		}
		if(!uns && sconst(r) || (uns || o == OEQ || o == ONE) && uconst(r)) {
			regalloc(&nod, l, nn);
			cgen(l, &nod);
			gopcode(o, &nod, Z, r);
			regfree(&nod);
			goto com;
		}
		if(l->complex >= r->complex) {
			regalloc(&nod1, l, nn);
			cgen(l, &nod1);
			regalloc(&nod, r, Z);
			cgen(r, &nod);
		} else {
			regalloc(&nod, r, nn);
			cgen(r, &nod);
			regalloc(&nod1, l, Z);
			cgen(l, &nod1);
		}
		gopcode(o, &nod1, Z, &nod);
		regfree(&nod);
		regfree(&nod1);

	com:
		if(nn != Z) {
			p1 = p;
			gopcode(OAS, nodconst(1L), Z, nn);
			gbranch(OGOTO);
			p2 = p;
			patch(p1, pc);
			gopcode(OAS, nodconst(0L), Z, nn);
			patch(p2, pc);
		}
		break;
	}
	cursafe = curs;
}

void
sugen(Node *n, Node *nn, long w)
{
	Prog *p1;
	Node nod0, nod1, nod2, nod3, nod4, *l, *r;
	Type *t;
	long pc1;
	int i, m, c;

	if(n == Z || n->type == T)
		return;
	if(nn == nodrat)
		if(w > nrathole)
			nrathole = w;
	if(debug['g']) {
		prtree(nn, "sugen lhs");
		prtree(n, "sugen");
	}
	if(typev[n->type->etype]) {
		diag(n, "old vlong sugen: %O", n->op);
		return;
	}
	switch(n->op) {
	case OIND:
		if(nn == Z) {
			nullwarn(n->left, Z);
			break;
		}

	default:
		goto copy;

	case ODOT:
		l = n->left;
		sugen(l, nodrat, l->type->width);
		if(nn != Z) {
			warn(n, "non-interruptable temporary");
			nod1 = *nodrat;
			r = n->right;
			if(!r || r->op != OCONST) {
				diag(n, "DOT and no offset");
				break;
			}
			nod1.xoffset += (long)r->vconst;
			nod1.type = n->type;
			sugen(&nod1, nn, w);
		}
		break;

	case OSTRUCT:
		/*
		 * rewrite so lhs has no side effects
		 */
		if(nn != Z && side(nn)) {
			nod1 = *n;
			nod1.type = typ(TIND, n->type);
			regalloc(&nod2, &nod1, Z);
			lcgen(nn, &nod2);
			regsalloc(&nod0, &nod1);
			gopcode(OAS, &nod2, Z, &nod0);
			regfree(&nod2);

			nod1 = *n;
			nod1.op = OIND;
			nod1.left = &nod0;
			nod1.right = Z;
			nod1.complex = 1;

			sugen(n, &nod1, w);
			return;
		}

		r = n->left;
		for(t = n->type->link; t != T; t = t->down) {
			l = r;
			if(r->op == OLIST) {
				l = r->left;
				r = r->right;
			}
			if(nn == Z) {
				cgen(l, nn);
				continue;
			}
			/*
			 * hand craft *(&nn + o) = l
			 */
			nod0 = znode;
			nod0.op = OAS;
			nod0.type = t;
			nod0.left = &nod1;
			nod0.right = l;

			nod1 = znode;
			nod1.op = OIND;
			nod1.type = t;
			nod1.left = &nod2;

			nod2 = znode;
			nod2.op = OADD;
			nod2.type = typ(TIND, t);
			nod2.left = &nod3;
			nod2.right = &nod4;

			nod3 = znode;
			nod3.op = OADDR;
			nod3.type = nod2.type;
			nod3.left = nn;

			nod4 = znode;
			nod4.op = OCONST;
			nod4.type = nod2.type;
			nod4.vconst = t->offset;

			ccom(&nod0);
			acom(&nod0);
			xcom(&nod0);
			nod0.addable = 0;

			/* prtree(&nod0, "hand craft"); /* */
			cgen(&nod0, Z);
		}
		break;

	case OAS:
		if(nn == Z) {
			if(n->addable < INDEXED)
				sugen(n->right, n->left, w);
			break;
		}
		/* BOTCH -- functions can clobber rathole */
		sugen(n->right, nodrat, w);
		warn(n, "non-interruptable temporary");
		sugen(nodrat, n->left, w);
		sugen(nodrat, nn, w);
		break;

	case OFUNC:
		/* this transformation should probably be done earlier */
		if(nn == Z) {
			sugen(n, nodrat, w);
			break;
		}
		if(nn->op != OIND) {
			nn = new1(OADDR, nn, Z);
			nn->type = types[TIND];
			nn->addable = 0;
		} else
			nn = nn->left;
		n = new(OFUNC, n->left, new(OLIST, nn, n->right));
		n->complex = FNX;
		n->type = types[TVOID];
		n->left->type = types[TVOID];
		cgen(n, Z);
		break;

	case OCOND:
		bcgen(n->left, 1);
		p1 = p;
		sugen(n->right->left, nn, w);
		gbranch(OGOTO);
		patch(p1, pc);
		p1 = p;
		sugen(n->right->right, nn, w);
		patch(p1, pc);
		break;

	case OCOMMA:
		cgen(n->left, Z);
		sugen(n->right, nn, w);
		break;
	}
	return;

copy:
	if(nn == Z)
		return;
	if(n->complex >= FNX && nn->complex >= FNX) {
		t = nn->type;
		nn->type = types[TLONG];
		regialloc(&nod1, nn, Z);
		lcgen(nn, &nod1);
		regsalloc(&nod2, nn);
		nn->type = t;

		gmove(&nod1, &nod2);
		regfree(&nod1);

		nod2.type = typ(TIND, t);

		nod1 = nod2;
		nod1.op = OIND;
		nod1.left = &nod2;
		nod1.right = Z;
		nod1.complex = 1;
		nod1.type = t;

		sugen(n, &nod1, w);
		return;
	}

	if(n->complex > nn->complex) {
		t = n->type;
		n->type = types[TLONG];
		reglcgen(&nod1, n, Z);
		n->type = t;

		t = nn->type;
		nn->type = types[TLONG];
		reglcgen(&nod2, nn, Z);
		nn->type = t;
	} else {
		t = nn->type;
		nn->type = types[TLONG];
		reglcgen(&nod2, nn, Z);
		nn->type = t;

		t = n->type;
		n->type = types[TLONG];
		reglcgen(&nod1, n, Z);
		n->type = t;
	}

	w /= SZ_LONG;
	if(w <= 5) {
		layout(&nod1, &nod2, w, 0, Z);
		goto out;
	}

	/*
	 * minimize space for unrolling loop
	 * 3,4,5 times. (6 or more is never minimum)
	 * if small structure, try 2 also.
	 */
	c = 0; /* set */
	m = 100;
	i = 3;
	if(w <= 15)
		i = 2;
	for(; i<=5; i++)
		if(i + w%i <= m) {
			c = i;
			m = c + w%c;
		}

	regalloc(&nod3, &regnode, Z);
	layout(&nod1, &nod2, w%c, w/c, &nod3);
	
	pc1 = pc;
	layout(&nod1, &nod2, c, 0, Z);

	gopcode(OSUB, nodconst(1L), Z, &nod3);
	nod1.op = OREGISTER;
	gopcode(OADD, nodconst(c*SZ_LONG), Z, &nod1);
	nod2.op = OREGISTER;
	gopcode(OADD, nodconst(c*SZ_LONG), Z, &nod2);
	
	gopcode(OGT, &nod3, Z, nodconst(0));
	patch(p, pc1);

	regfree(&nod3);
out:
	regfree(&nod1);
	regfree(&nod2);
}

void
layout(Node *f, Node *t, int c, int cv, Node *cn)
{
	Node t1, t2;

	while(c > 3) {
		layout(f, t, 2, 0, Z);
		c -= 2;
	}

	regalloc(&t1, &regnode, Z);
	regalloc(&t2, &regnode, Z);
	if(c > 0) {
		gopcode(OAS, f, Z, &t1);
		f->xoffset += SZ_LONG;
	}
	if(cn != Z)
		gopcode(OAS, nodconst(cv), Z, cn);
	if(c > 1) {
		gopcode(OAS, f, Z, &t2);
		f->xoffset += SZ_LONG;
	}
	if(c > 0) {
		gopcode(OAS, &t1, Z, t);
		t->xoffset += SZ_LONG;
	}
	if(c > 2) {
		gopcode(OAS, f, Z, &t1);
		f->xoffset += SZ_LONG;
	}
	if(c > 1) {
		gopcode(OAS, &t2, Z, t);
		t->xoffset += SZ_LONG;
	}
	if(c > 2) {
		gopcode(OAS, &t1, Z, t);
		t->xoffset += SZ_LONG;
	}
	regfree(&t1);
	regfree(&t2);
}

/*
 * is the vlong's value directly addressible?
 */
int
isvdirect(Node *n)
{
	return n->op == ONAME || n->op == OCONST || n->op == OINDREG;
}

/*
 * can the constant be used with given vlong op?
 */
static int
isvconstable(int o, vlong v)
{
	switch(o) {
	case OADD:
	case OASADD:
		/* there isn't an immediate form for ADDE/SUBE, but there are special ADDME/ADDZE etc */
		return v == 0 || v == -1;
	case OAND:
	case OOR:
	case OXOR:
	case OLSHR:
	case OASHL:
	case OASHR:
	case OASLSHR:
	case OASASHL:
	case OASASHR:
		return 1;
	}
	return 0;
}

/*
 * most 64-bit operations: cgen into a register pair, then operate.
 * 64-bit comparisons are handled a little differently because the two underlying
 * comparisons can be compiled separately, since the calculations don't interact.
 */

static void
vcgen(Node *n, Node *o, int *f)
{
	*f = 0;
	if(!isvdirect(n)) {
		if(n->complex >= FNX) {
			regsalloc(o, n);
			cgen(n, o);
			return;
		}
		*f = 1;
		if(n->addable < INDEXED && n->op != OIND && n->op != OINDEX) {
			regalloc(o, n, Z);
			cgen(n, o);
		} else
			reglcgen(o, n, Z);
	} else
		*o = *n;
}

static int
isuns(int op)
{
	switch(op){
	case OLO:
	case OLS:
	case OHI:
	case OHS:
		return 1;
	default:
		return 0;
	}
}

static void
gcmpv(Node *l, Node *r, void (*mov)(Node*, Node*, int), int op)
{
	Node vl, vr;

	regalloc(&vl, &regnode, Z);
	mov(l, &vl, 0);
	regalloc(&vr, &regnode, Z);
	mov(r, &vr, 1+isuns(op));
	gopcode(op, &vl, Z, &vr);
	if(vl.op == OREGISTER)
		regfree(&vl);
	if(vr.op == OREGISTER)
		regfree(&vr);
}

static void
brcondv(Node *l, Node *r, int chi, int clo)
{
	Prog *p1, *p2, *p3, *p4;

	gcmpv(l, r, gloadhi, chi);
	p1 = p;
	gins(ABNE, Z, Z);
	p2 = p;
	gcmpv(l, r, gloadlo, clo);
	p3 = p;
	gbranch(OGOTO);
	p4 = p;
	patch(p1, pc);
	patch(p3, pc);
	gbranch(OGOTO);
	patch(p2, pc);
	patch(p4, pc);
}

static void
testv(Node *n, int true)
{
	Node nod;

	nod = znode;
	nod.op = ONE;
	nod.left = n;
	nod.right = new1(0, Z, Z);
	*nod.right = *nodconst(0);
	nod.right->type = n->type;
	nod.type = types[TLONG];
	cmpv(&nod, true, Z);
}

/*
 * comparison for vlong does high and low order parts separately,
 * which saves loading the latter if the high order comparison suffices
 */
static void
cmpv(Node *n, int true, Node *nn)
{
	Node *l, *r, nod, nod1;
	int o, f1, f2;
	Prog *p1, *p2;
	long curs;

	if(debug['g']) {
		if(nn != nil)
			prtree(nn, "cmpv lhs");
		prtree(n, "cmpv");
	}
	curs = cursafe;
	l = n->left;
	r = n->right;
	if(l->complex >= FNX && r->complex >= FNX) {
		regsalloc(&nod1, r);
		cgen(r, &nod1);
		nod = *n;
		nod.right = &nod1;
		cmpv(&nod, true, nn);
		cursafe = curs;
		return;
	}
	if(l->complex >= r->complex) {
		vcgen(l, &nod1, &f1);
		vcgen(r, &nod, &f2);
	} else {
		vcgen(r, &nod, &f2);
		vcgen(l, &nod1, &f1);
	}
	nod.type = types[TLONG];
	nod1.type = types[TLONG];
	o = n->op;
	if(true)
		o = comrel[relindex(o)];
	switch(o){
	case OEQ:
		gcmpv(&nod1, &nod, gloadhi, ONE);
		p1 = p;
		gcmpv(&nod1, &nod, gloadlo, ONE);
		p2 = p;
		gbranch(OGOTO);
		patch(p1, pc);
		patch(p2, pc);
		break;
	case ONE:
		gcmpv(&nod1, &nod, gloadhi, ONE);
		p1 = p;
		gcmpv(&nod1, &nod, gloadlo, OEQ);
		p2 = p;
		patch(p1, pc);
		gbranch(OGOTO);
		patch(p2, pc);
		break;
	case OLE:
		brcondv(&nod1, &nod, OLT, OLS);
		break;
	case OGT:
		brcondv(&nod1, &nod, OGT, OHI);
		break;
	case OLS:
		brcondv(&nod1, &nod, OLO, OLS);
		break;
	case OHI:
		brcondv(&nod1, &nod, OHI, OHI);
		break;
	case OLT:
		brcondv(&nod1, &nod, OLT, OLO);
		break;
	case OGE:
		brcondv(&nod1, &nod, OGT, OHS);
		break;
	case OLO:
		brcondv(&nod1, &nod, OLO, OLO);
		break;
	case OHS:
		brcondv(&nod1, &nod, OHI, OHS);
		break;
	default:
		diag(n, "bad cmpv");
		return;
	}
	if(f1)
		regfree(&nod1);
	if(f2)
		regfree(&nod);
	cursafe = curs;
}

static void
cgen64(Node *n, Node *nn)
{
	Node *l, *r, *d;
	Node nod, nod1;
	long curs;
	Type *t;
	int o, m;

	curs = cursafe;
	l = n->left;
	r = n->right;
	o = n->op;
	switch(o) {

	case OCONST:
		if(nn == Z) {
			nullwarn(n->left, Z);
			break;
		}
		if(nn->op != OREGPAIR) {
//prtree(n, "cgen64 const");
			t = nn->type;
			nn->type = types[TLONG];
			reglcgen(&nod1, nn, Z);
			nn->type = t;

			if(align(0, types[TCHAR], Aarg1))	/* isbigendian */
				gmove(nod32const(n->vconst>>32), &nod1);
			else
				gmove(nod32const(n->vconst), &nod1);
			nod1.xoffset += SZ_LONG;
			if(align(0, types[TCHAR], Aarg1))	/* isbigendian */
				gmove(nod32const(n->vconst), &nod1);
			else
				gmove(nod32const(n->vconst>>32), &nod1);

			regfree(&nod1);
		} else
			gmove(n, nn);
		break;

	case OCAST:
		/*
		 * convert from types l->n->nn
		 */
		if(typev[l->type->etype]){
			/* vlong to non-vlong */
			if(!isvdirect(l)) {
				if(l->addable < INDEXED && l->op != OIND && l->op != OINDEX) {
					regalloc(&nod, l, l);
					cgen(l, &nod);
					regalloc(&nod1, n, nn);
					gmove(nod.right, &nod1);
				} else {
					reglcgen(&nod, l, Z);
					regalloc(&nod1, n, nn);
					gloadlo(&nod, &nod1, 0);	/* TO DO: not correct for typefd */
				}
				regfree(&nod);
			} else {
				regalloc(&nod1, n, nn);
				gloadlo(l, &nod1, 0);	/* TO DO: not correct for typefd */
			}
		}else{
			/* non-vlong to vlong */
			regalloc(&nod, l, Z);
			cgen(l, &nod);
			regalloc(&nod1, n, nn);
			gmove(&nod, nod1.right);
			if(typeu[l->type->etype])
				gmove(nodconst(0), nod1.left);
			else
				gopcode(OASHR, nodconst(31), nod1.right, nod1.left);
			regfree(&nod);
		}
		gmove(&nod1, nn);
		regfree(&nod1);
		break;

	case OFUNC:
		/* this transformation should probably be done earlier */
		if(nn == Z) {
			regsalloc(&nod1, n);
			nn = &nod1;
		}
		m = 0;
		if(nn->op != OIND) {
			if(nn->op == OREGPAIR) {
				m = 1;
				regsalloc(&nod1, nn);
				d = &nod1;
			}else
				d = nn;
			d = new1(OADDR, d, Z);
			d->type = types[TIND];
			d->addable = 0;
		} else
			d = nn->left;
		n = new(OFUNC, l, new(OLIST, d, r));
		n->complex = FNX;
		n->type = types[TVOID];
		n->left->type = types[TVOID];
		cgen(n, Z);
		if(m)
			gmove(&nod1, nn);
		break;

	default:
		diag(n, "bad cgen64 %O", o);
		break;
	}
	cursafe = curs;
}

#include "gc.h"

void
cgen(Node *n, Node *nn)
{
	Node *l, *r;
	Node nod, nod1, nod2, nod3, nod4;
	long regs, regc, v;
	int o;
	Prog *p1;

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
	if(n->addable > INDEXED) {
		if(nn != Z)
			gopcode(OAS, n, nn);
		return;
	}
	l = n->left;
	r = n->right;
	regs = cursafe;
	regc = curreg;

	o = n->op;
	switch(o) {
	default:
		diag(n, "unknown op in cgen: %O", n->op);
		break;

	case OAS:
		/*
		 * recursive use of result
		 */
		if(l->op == OBIT)
			goto bitas;
		if(nn == Z)
		if(l->addable > INDEXED)
		if(l->complex < FNX) {
			cgen(r, l);
			break;
		}
		if(l->complex >= FNX && r->complex >= FNX) {
			regsalloc(&nod1, n);
			cgen(r, &nod1);
			regialloc(&nod, l);
			lcgen(l, &nod);
			regind(&nod, l);
			gopcode(OAS, &nod1, &nod);
			if(nn != Z)
				gopcode(OAS, &nod1, nn);
			break;
		}
		if(l->complex > r->complex) {
			regialloc(&nod, l);
			lcgen(l, &nod);
			regind(&nod, l);
			curreg += SZ_IND;
			gprep(&nod1, r);
		} else {
			gprep(&nod1, r);
			regialloc(&nod, l);
			lcgen(l, &nod);
			regind(&nod, l);
		}
		gopcode(OAS, &nod1, &nod);
		if(nn != Z)
			gopcode(OAS, &nod1, nn);
		break;

	bitas:
		n = l->left;
		regalloc(&nod, r);
		curreg += SZ_LONG;
		if(l->complex >= r->complex) {
			regialloc(&nod1, n);
			lcgen(n, &nod1);
			regind(&nod1, n);
			curreg += SZ_IND;
			cgen(r, &nod);
		} else {
			cgen(r, &nod);
			regialloc(&nod1, n);
			lcgen(n, &nod1);
			regind(&nod1, n);
			curreg += SZ_IND;
		}
		regalloc(&nod2, n);
		curreg += (n->type->width+SZ_LONG-1) & ~(SZ_LONG-1);
		gopcode(OAS, &nod1, &nod2);
		bitstore(l, &nod, &nod1, &nod2, nn);
		break;

	case OBIT:
		if(nn == Z) {
			nullwarn(l, Z);
			break;
		}
		bitload(n, &nod, Z, Z);
		gopcode(OAS, &nod, nn);
		break;

	case OADD:
	case OLMUL:
	case OLDIV:
	case OLMOD:
	case OMUL:
	case ODIV:
	case OMOD:
	case OSUB:
	case OAND:
	case OOR:
	case OXOR:
	case OLSHR:
	case OASHL:
	case OASHR:
		if(nn == Z) {
			nullwarn(l, r);
			break;
		}
		if(l->complex >= FNX && r->complex >= FNX) {
			regsalloc(&nod, n);
			cgen(l, &nod);
			regalloc(&nod1, n);
			cgen(r, &nod1);
			gopcode(o, &nod1, &nod);
			gopcode(OAS, &nod, nn);
			break;
		}

		regalloc(&nod, n);
		if(acc(&nod))
		if(o != OOR && o != OLDIV && o != OLMOD) {
			if(l->complex >= r->complex) {
				gprep(&nod1, l);
				gprep(&nod2, r);
			} else {
				gprep(&nod2, r);
				gprep(&nod1, l);
			}
			gopcode3(o, &nod2, &nod1);
			gopcode(OAS, &nod, nn);
			break;
		}
		if(l->complex >= r->complex) {
			cgen(l, &nod);
			curreg += (l->type->width+SZ_LONG) & ~(SZ_LONG-1);
			gprep(&nod2, r);
		} else {
			gprep(&nod2, r);
			regalloc(&nod, n);
			cgen(l, &nod);
		}
		gopcode(o, &nod2, &nod);
		gopcode(OAS, &nod, nn);
		break;

	case OASLMUL:
	case OASLDIV:
	case OASLMOD:
	case OASMUL:
	case OASDIV:
	case OASMOD:
	case OASXOR:
	case OASOR:
	case OASADD:
	case OASSUB:
	case OASLSHR:
	case OASASHL:
	case OASASHR:
	case OASAND:
		if(l->op == OCAST)
			l = l->left;
		if(l->op == OBIT)
			goto asbitop;
		if(typefdv[n->type->etype] && (typec[l->type->etype] || typeh[l->type->etype])) {
			regalloc(&nod1, n);
			cgen(r, &nod1);
			curreg += SZ_DOUBLE;

			regialloc(&nod, l);
			lcgen(l, &nod);
			regind(&nod, l);
			curreg += SZ_IND;

			regalloc(&nod2, n);
			cgen(&nod, &nod2);
			gopcode(o, &nod1, &nod2);
			gopcode(OAS, &nod2, &nod);
			if(nn != Z)
				gopcode(OAS, &nod2, nn);
			break;
		}
		if(l->addable > INDEXED) {
			gprep(&nod1, r);
			gopcode(o, &nod1, l);
			if(nn != Z)
				gopcode(OAS, l, nn);
			break;
		}
		if(l->complex >= FNX && r->complex >= FNX) {
			regsalloc(&nod1, n);
			cgen(r, &nod1);
			regialloc(&nod, l);
			lcgen(l, &nod);
			regind(&nod, l);
			curreg += SZ_IND;
			gopcode(o, &nod1, &nod);
			if(nn != Z)
				gopcode(OAS, &nod, nn);
			break;
		}
		if(l->complex >= r->complex) {
			regialloc(&nod, l);
			lcgen(l, &nod);
			regind(&nod, l);
			curreg += SZ_IND;
			gprep(&nod1, r);
			gopcode(o, &nod1, &nod);
		} else {
			gprep(&nod1, r);
			regialloc(&nod, l);
			lcgen(l, &nod);
			regind(&nod, l);
			curreg += SZ_IND;
			gopcode(o, &nod1, &nod);
		}
		if(nn != Z)
			gopcode(OAS, &nod, nn);
		break;

	asbitop:
		regalloc(&nod4, n);
		curreg += (n->type->width+SZ_LONG) & ~(SZ_LONG-1);
		if(l->complex >= r->complex) {
			bitload(l, &nod, &nod1, &nod2);
			regalloc(&nod3, r);
			curreg += (r->type->width+SZ_LONG) & ~(SZ_LONG-1);
			cgen(r, &nod3);
		} else {
			regalloc(&nod3, r);
			curreg += (r->type->width+SZ_LONG) & ~(SZ_LONG-1);
			cgen(r, &nod3);
			bitload(l, &nod, &nod1, &nod2);
		}
		gopcode(OAS, &nod, &nod4);
		gopcode(o, &nod3, &nod4);
		gopcode(OAS, &nod4, &nod);
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
		curreg = SZ_LONG;
		if(SZ_LONG*2 > maxreg)
			maxreg = SZ_LONG*2;	/* one for pc; one for acc */
		gargs(r, &nod);
		if(l->addable < INDEXED) {
			regialloc(&nod1, l);
			lcgen(l, &nod1);
			regind(&nod1, l);
			gopcode(OFUNC, Z, &nod1);
		} else
			gopcode(OFUNC, Z, l);
		if(nn != Z) {
			regret(&nod, n);
			gopcode(OAS, &nod, nn);
		}
		break;

	case OIND:
		if(nn == Z) {
			nullwarn(l, Z);
			break;
		}
		regialloc(&nod, l);
		cgen(l, &nod);
		regind(&nod, n);
		gopcode(OAS, &nod, nn);
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
		doinc(l, APOST);
		doinc(r, PRE);
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
		if(nocast(l->type, n->type)) {
			if(nocast(n->type, nn->type)) {
				cgen(l, nn);
				break;
			}
		}
		regalloc(&nod, n);
		cgen(l, &nod);
		gopcode(OAS, &nod, nn);
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
			nod.offset += r->offset;
			nod.type = n->type;
			cgen(&nod, nn);
		}
		break;

	case OCOND:
		doinc(l, PRE);
		bcgen(l, 1);
		p1 = p;

		doinc(r->left, PRE);
		cgen(r->left, nn);
		doinc(r->left, APOST);
		gbranch(OGOTO);
		patch(p1, pc);
		p1 = p;

		doinc(r->right, PRE);
		cgen(r->right, nn);
		doinc(r->right, APOST);
		patch(p1, pc);
		break;

	case OPOSTINC:
	case OPOSTDEC:
		v = 1;
		if(l->type->etype == TIND)
			v = l->type->link->width;
		if(nn != Z)
			curreg += SZ_DOUBLE;	/* smells */
		if(l->op == OBIT)
			goto bitinc;
		regialloc(&nod, l);
		lcgen(l, &nod);
		regind(&nod, l);
		if(nn != Z)
			gopcode(OAS, &nod, nn);
		if(typefdv[l->type->etype])
			gopcode(o, nodfconst(v), &nod);
		else
			gopcode(o, nodconst(v), &nod);
		break;

	case OPREINC:
	case OPREDEC:
		v = 1;
		if(l->type->etype == TIND)
			v = l->type->link->width;
		if(nn != Z)
			curreg += SZ_DOUBLE;	/* smells */
		if(l->op == OBIT)
			goto bitinc;
		regialloc(&nod, l);
		lcgen(l, &nod);
		regind(&nod, l);
		if(typefdv[l->type->etype])
			gopcode(o, nodfconst(v), &nod);
		else
			gopcode(o, nodconst(v), &nod);
		if(nn != Z)
			gopcode(OAS, &nod, nn);
		break;

	bitinc:
		if(nn != Z && (o == OPOSTINC || o == OPOSTDEC)) {
			bitload(l, &nod, &nod1, &nod2);
			gopcode(OAS, &nod, nn);
			gopcode(o, nodconst(v), &nod);
			bitstore(l, &nod, &nod1, &nod2, Z);
			break;
		}
		bitload(l, &nod, &nod1, &nod2);
		gopcode(o, nodconst(v), &nod);
		bitstore(l, &nod, &nod1, &nod2, nn);
		break;
	}
	cursafe = regs;
	curreg = regc;
	return;

bad:
	diag(n, "%O not implemented", n->op);
	cursafe = regs;
	curreg = regc;
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
		regalloc(&nod, n);
	}
	switch(n->op) {
	default:
		if(n->addable < INDEXED) {
			diag(n, "unknown op in lcgen: %O", n->op);
			break;
		}
		gopcode(OADDR, n, nn);
		break;

	case OCOMMA:
		cgen(n->left, n->left);
		doinc(n->left, APOST);
		doinc(n->right, PRE);
		lcgen(n->right, nn);
		break;

	case OIND:
		cgen(n->left, nn);
		break;

	case OCOND:
		doinc(n->left, PRE);
		bcgen(n->left, 1);
		p1 = p;

		doinc(n->right->left, PRE);
		lcgen(n->right->left, nn);
		doinc(n->right->left, APOST);
		gbranch(OGOTO);
		patch(p1, pc);
		p1 = p;

		doinc(n->right->right, PRE);
		lcgen(n->right->right, nn);
		doinc(n->right->right, APOST);
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
	int o;
	Prog *p1, *p2;
	Node *l, *r, *tmp, nod, nod1;
	long regs, regc;

	if(debug['g']) {
		prtree(nn, "boolgen lhs");
		prtree(n, "boolgen");
	}
	regs = cursafe;
	regc = curreg;
	l = n->left;
	r = n->right;
	switch(n->op) {

	default:
		if(typefdv[n->type->etype]) {
			regalloc(&nod1, n);
			gopcode(OAS, nodconst(0), &nod1);
			gprep(&nod, n);
			gopcode(ONE, &nod1, &nod);
			o = ONE;
			goto genbool;
		}
		gprep(&nod, n);
		gopcode(ONE, nodconst(0), &nod);
		o = ONE;
		goto genbool;

	case OCONST:
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

	case OCOMMA:
		cgen(l, Z);
		doinc(l, APOST);
		doinc(r, PRE);
		boolgen(r, true, nn);
		break;

	case ONOT:
		boolgen(l, !true, nn);
		break;

	case OCOND:
		doinc(l, PRE);
		bcgen(l, 1);
		p1 = p;

		doinc(r->left, PRE);
		bcgen(r->left, true);
		p2 = p;
		gbranch(OGOTO);
		patch(p1, pc);
		p1 = p;

		doinc(r->right, PRE);
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
		doinc(r, PRE);
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
		doinc(r, PRE);
		bcgen(r, !true);
		p2 = p;
		gbranch(OGOTO);
		patch(p1, pc);
		patch(p2, pc);
		goto com;

	case OLT:
	case OGE:
	case OLO:
	case OHS:
		/* crisp only has cmp.lt */
		o = invrel[relindex(n->op)];
		tmp = l;
		l = r;
		r = tmp;
		goto rel;

	case OEQ:
	case ONE:
	case OLE:
	case OGT:
	case OHI:
	case OLS:
		o = n->op;

	rel:
		if(l->complex >= FNX && r->complex >= FNX) {
			regsalloc(&nod, l);
			cgen(l, &nod);
			regalloc(&nod1, r);
			cgen(r, &nod1);
			gopcode(o, &nod, &nod1);
			goto genbool;
		}
		if(l->complex >= r->complex) {
			gprep(&nod, l);
			gprep(&nod1, r);
			gopcode(o, &nod, &nod1);
			goto genbool;
		}
		gprep(&nod, r);
		gprep(&nod1, l);
		gopcode(o, &nod1, &nod);

	genbool:
		doinc(n, APOST);
		if(true)
			o = comrel[relindex(o)];
		gbranch(o);

	com:
		if(nn != Z) {
			p1 = p;
			gopcode(OAS, nodconst(1), nn);
			gbranch(OGOTO);
			p2 = p;
			patch(p1, pc);
			gopcode(OAS, nodconst(0), nn);
			patch(p2, pc);
		}
		break;
	}
	cursafe = regs;
	curreg = regc;
}

void
sugen(Node *n, Node *nn, long w)
{
	Prog *p1;
	Node nod0, nod1, nod2, nod3, nod4, *l, *r;
	Type *t;
	long regs, regc, pc1;
	int f, i;

	if(n == Z || n->type == T)
		return;
	if(debug['g']) {
		prtree(nn, "sugen lhs");
		prtree(n, "sugen");
	}
	if(nn == nodrat)
		if(w > nrathole)
			nrathole = w;
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
			nod1.offset += r->offset;
			nod1.type = n->type;
			sugen(&nod1, nn, w);
		}
		break;

	case OSTRUCT:
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
			nod4.offset = t->offset;

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
		sugen(n->right, nodrat, w);
		warn(n, "non-interruptable temporary");
		sugen(nodrat, n->left, w);
		sugen(nodrat, nn, w);
		break;

	case OFUNC:
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
		n->type = tint;
		cgen(n, Z);
		break;

	case OCOND:
		doinc(n->left, PRE);
		bcgen(n->left, 1);
		p1 = p;
		doinc(n->right->left, PRE);
		sugen(n->right->left, nn, w);
		doinc(n->right->left, APOST);
		gbranch(OGOTO);
		patch(p1, pc);
		p1 = p;
		doinc(n->right->right, PRE);
		sugen(n->right->right, nn, w);
		doinc(n->right->right, APOST);
		patch(p1, pc);
		break;

	case OCOMMA:
		cgen(n->left, Z);
		doinc(n->left, APOST);
		doinc(n->right, PRE);
		sugen(n->right, nn, w);
		break;
	}
	return;

copy:
	if(nn == Z)
		return;
	regs = cursafe;
	regc = curreg;
	if(n->complex >= FNX && nn->complex >= FNX) {
		diag(n, "ah come on");
		return;
	}
	f = 0;
	nod1 = *n;
	nod2 = *nn;
	/* note, there should be an operator to test/alter offsets
	   the following code does not get *(&external + const)
	 */
	if(n->complex >= nn->complex) {
		if(w > INLINE || (n->op != ONAME && n->op != OREGISTER)) {
			regialloc(&nod1, n);
			lcgen(n, &nod1);
			curreg += SZ_LONG;
			f |= 1;		/* pointer to n is in nod1 */
		}
		if(w > INLINE || (nn->op != ONAME && nn->op != OREGISTER)) {
			regialloc(&nod2, nn);
			lcgen(nn, &nod2);
			f |= 2;		/* pointer to nn is in nod2 */
			curreg += SZ_LONG;
		}
	} else {
		if(w > INLINE || (nn->op != ONAME && nn->op != OREGISTER)) {
			regialloc(&nod2, nn);
			lcgen(nn, &nod2);
			f |= 2;
			curreg += SZ_LONG;
		}
		if(w > INLINE || (n->op != ONAME && n->op != OREGISTER)) {
			regialloc(&nod1, n);
			lcgen(n, &nod1);
			curreg += SZ_LONG;
			f |= 1;
		}
	}
	nod1.type = types[TLONG];
	nod2.type = types[TLONG];
	if(f & 1)
		nod1.op = OINDREG;
	if(f & 2)
		nod2.op = OINDREG;
	if(w > INLINE)
		goto copyloop;
	gopcode(OAS, &nod1, &nod2);
	for(i=SZ_LONG; i<w; i+=SZ_LONG) {
		if(f & 1) {
			nod1.op = OREGISTER;
			gopcode(OADD, nodconst(SZ_LONG), &nod1);
			nod1.op = OINDREG;
		} else
			nod1.offset += SZ_LONG;
		if(f & 2) {
			nod2.op = OREGISTER;
			gopcode(OADD, nodconst(SZ_LONG), &nod2);
			nod2.op = OINDREG;
		} else
			nod2.offset += SZ_LONG;
		gopcode(OAS, &nod1, &nod2);
	}
	goto out;

copyloop:
	regialloc(&nod3, n);
	nod3.type = types[TLONG];
	gopcode(OAS, nodconst(w/SZ_LONG), &nod3);

		pc1 = pc;
		gopcode(OAS, &nod1, &nod2);

		nod1.op = OREGISTER;
		gopcode(OADD, nodconst(SZ_LONG), &nod1);
		nod2.op = OREGISTER;
		gopcode(OADD, nodconst(SZ_LONG), &nod2);

	gopcode(OADD, nodconst(-1), &nod3);
	gopcode(OLT, &nod3, nodconst(0));
	gbranch(OEQ);
	patch(p, pc1);

out:
	cursafe = regs;
	curreg = regc;
}

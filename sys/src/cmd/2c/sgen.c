#include "gc.h"

void
codgen(Node *n, Node *nn)
{
	Prog *sp;

	argoff = 0;
	inargs = 0;
	for(;; nn = nn->left) {
		if(nn == Z) {
			diag(Z, "cant find function name");
			return;
		}
		if(nn->op == ONAME)
			break;
	}
	nearln = nn->lineno;
	gpseudo(ATEXT, nn->sym, D_CONST, stkoff);
	sp = p;

	retok = 0;
	gen(n);
	if(!retok)
		if(thisfn->link->etype != TVOID)
			warn(Z, "no return at end of function: %s", nn->sym->name);

	noretval(3);
	gbranch(ORETURN);
	if(!debug['N'] || debug['R'] || debug['P'])
		regopt(sp);
}

void
gen(Node *n)
{
	Node *l;
	Prog *sp, *spc, *spb;
	Case *cn;
	long sbc, scc;
	int g, o;

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
		doinc(n, PRE);
		cgen(n, D_NONE, n);
		doinc(n, POST);
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
		doinc(l, PRE);
		if(typesuv[n->type->etype]) {
			sugen(l, D_TREE, nodret, n->type->width);
			doinc(l, POST);
			noretval(3);
			gbranch(ORETURN);
			break;
		}
		g = regalloc(n->type, regret(n->type));
		cgen(l, g, n);
		doinc(l, POST);
		if(typefd[n->type->etype])
			noretval(1);
		else
			noretval(2);
		gbranch(ORETURN);
		regfree(g);
		break;

	case OLABEL:
		l = n->left;
		if(l) {
			l->xoffset = pc;
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
		if(n->xoffset) {
			patch(p, n->xoffset);
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
			setsp();;
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
			setsp();
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
		g = regalloc(types[TLONG], D_NONE);
		n->type = types[TLONG];
		cgen(l, g, n);
		regfree(g);
		doinc(l, POST);
		setsp();
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
		doswit(g, l);

		patch(spb, pc);
		cases = cn;
		breakpc = sbc;
		setsp();
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
		bcomplex(l);	/* test */
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
		gbranch(OGOTO);			/* entry */
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
		gopcode(OTST, types[TINT], D_TREE, n, D_NONE, Z);
		p->as = ANOP;
		break;
	case ONAME:
		if(o == OSET)
			gopcode(OTST, types[TINT], D_NONE, Z, D_TREE, n);
		else
			gopcode(OTST, types[TINT], D_TREE, n, D_NONE, Z);
		p->as = ANOP;
		break;
	}
}

void
noretval(int n)
{

	if(n & 1) {
		gopcode(OTST, types[TINT], D_NONE, Z, regret(types[TLONG]), Z);
		p->as = ANOP;
	}
	if(n & 2) {
		gopcode(OTST, types[TINT], D_NONE, Z, regret(types[TDOUBLE]), Z);
		p->as = ANOP;
	}
}

/*
 *	calculate addressability as follows
 *		REGISTER ==> 12		register
 *		NAME ==> 10/11		name+value(SB/SP)
 *		CONST ==> 20		$value
 *		*(20) ==> 21		value
 *		&(10) ==> 12		$name+value(SB)
 *		&(11) ==> 1		$name+value(SP)
 *		(12) + (20) ==> 12	fold constants
 *		(1) + (20) ==> 1	fold constants
 *		*(12) ==> 10		back to name
 *		*(1) ==> 11		back to name
 *
 *		(2,10,11) + (20) ==> 2	indirect w offset
 *		(2) ==> &13
 *		*(10,11) ==> 13		indirect, no index
 *
 *		(20) * (X) ==> 7	multiplier in indexing
 *		(X,7) + (12,1) ==> 8	adder in indexing (addresses)
 *		(X,7) + (10,11,2) ==> 8	adder in indexing (names)
 *		(8) ==> &9		index, almost addressable
 *
 *		(X)++ ==> X		fake addressability
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

	case OREGISTER:
		n->addable = 12;
		break;

	case OADDR:
		xcom(l);
		if(l->addable == 10)
			n->addable = 12;
		else
		if(l->addable == 11)
			n->addable = 1;
		break;

	case OADD:
		xcom(l);
		xcom(r);
		if(n->type->etype != TIND)
			break;

		if(l->addable == 20)
		switch(r->addable) {
		case 12:
		case 1:
			n->addable = r->addable;
			goto brk;
		case 10:
		case 11:
		case 2:
			goto addr13;
		}
		if(r->addable == 20)
		switch(l->addable) {
		case 12:
		case 1:
			n->addable = l->addable;
			goto brk;
		case 10:
		case 11:
		case 2:
		addr13:
			n->addable = 2;
			l = new1(OXXX, Z, Z);
			*l = *n;
			n->op = OIND;
			n->left = l;
			n->right = Z;
			n->addable = 13;
			l = new1(OXXX, Z, Z);
			*l = *n;
			n->op = OADDR;
			n->left = l;
			n->right = Z;
			n->addable = 2;
			goto brk;
		}

		switch(r->addable) {
		case 10:
		case 11:
		case 12:
		case 1:
			n->addable = 8;
		}
		switch(l->addable) {
		case 10:
		case 11:
		case 12:
		case 1:
			n->addable = 8;
		}
		if(n->addable == 8) {
			indx(n);
			l = new1(OINDEX, idx.basetree, idx.regtree);
			l->scale = idx.scale;
			l->addable = 9;
			l->complex = l->right->complex;
			l->type = l->left->type;
			n->op = OADDR;
			n->left = l;
			n->right = Z;
			n->addable = 0;
			break;
		}
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
		case 12:
			n->addable = 10;
			break;
		case 10:
		case 11:
		case 2:
			n->addable = 13;
			break;
		}
		break;

	case OASHL:
		xcom(l);
		xcom(r);
		if(typev[n->type->etype])
			break;
		g = vconst(r);
		if(g >= 0 && g < 4)
			n->addable = 7;
		break;

	case OMUL:
	case OLMUL:
		xcom(l);
		xcom(r);
		if(typev[n->type->etype])
			break;
		g = vlog(r);
		if(g >= 0) {
			n->op = OASHL;
			r->vconst = g;
			if(g < 4)
				n->addable = 7;
			break;
		}
		g = vlog(l);
		if(g >= 0) {
			n->left = r;
			n->right = l;
			l = r;
			r = n->right;
			n->op = OASHL;
			r->vconst = g;
			if(g < 4)
				n->addable = 7;
			break;
		}
		break;

	case ODIV:
	case OLDIV:
		xcom(l);
		xcom(r);
		if(typev[n->type->etype])
			break;
		g = vlog(r);
		if(g >= 0) {
			if(n->op == ODIV)
				n->op = OASHR;
			else
				n->op = OLSHR;
			r->vconst = g;
		}
		break;

	case OSUB:
		xcom(l);
		xcom(r);
		if(typev[n->type->etype])
			break;
		if(vconst(l) == 0) {
			n->op = ONEG;
			n->left = r;
			n->right = Z;
		}
		break;

	case OXOR:
		xcom(l);
		xcom(r);
		if(typev[n->type->etype])
			break;
		if(vconst(l) == -1) {
			n->op = OCOM;
			n->left = r;
			n->right = Z;
		}
		break;

	case OASMUL:
	case OASLMUL:
		xcom(l);
		xcom(r);
		if(typev[n->type->etype])
			break;
		g = vlog(r);
		if(g >= 0) {
			n->op = OASASHL;
			r->vconst = g;
		}
		goto aseae;

	case OASDIV:
	case OASLDIV:
		xcom(l);
		xcom(r);
		if(typev[n->type->etype])
			break;
		g = vlog(r);
		if(g >= 0) {
			if(n->op == OASDIV)
				n->op = OASASHR;
			else
				n->op = OASLSHR;
			r->vconst = g;
		}
		goto aseae;

	case OASLMOD:
	case OASMOD:
		xcom(l);
		xcom(r);
		if(typev[n->type->etype])
			break;

	aseae:		/* hack that there are no byte/short mul/div operators */
		if(n->type->etype == TCHAR || n->type->etype == TSHORT) {
			n->right = new1(OCAST, n->right, Z);
			n->right->type = types[TLONG];
			n->type = types[TLONG];
		}
		if(n->type->etype == TUCHAR || n->type->etype == TUSHORT) {
			n->right = new1(OCAST, n->right, Z);
			n->right->type = types[TULONG];
			n->type = types[TULONG];
		}
		goto asop;

	case OASXOR:
	case OASOR:
	case OASADD:
	case OASSUB:
	case OASLSHR:
	case OASASHR:
	case OASASHL:
	case OASAND:
	case OAS:
		xcom(l);
		xcom(r);
		if(typev[n->type->etype])
			break;

	asop:
		if(l->addable > INDEXED &&
		   l->complex < FNX &&
		   r && r->complex < FNX)
			n->addable = l->addable;
		break;

	case OPOSTINC:
	case OPREINC:
	case OPOSTDEC:
	case OPREDEC:
		xcom(l);
		if(typev[n->type->etype])
			break;
		if(l->addable > INDEXED &&
		   l->complex < FNX)
			n->addable = l->addable;
		break;

	default:
		if(l != Z)
			xcom(l);
		if(r != Z)
			xcom(r);
		break;
	}

brk:
	n->complex = 0;
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
	case OMUL:
	case OLMUL:
	case OXOR:
	case OAND:
	case OOR:
		/*
		 * symmetric operators, make right side simple
		 * if same, put constant on left to get movq
		 */
		if(r->complex > l->complex ||
		  (r->complex == l->complex && r->addable == 20)) {
			n->left = r;
			n->right = l;
		}
		break;

	case OLE:
	case OLT:
	case OGE:
	case OGT:
	case OEQ:
	case ONE:
		/*
		 * relational operators, make right side simple
		 * if same, put constant on left to get movq
		 */
		if(r->complex > l->complex || r->addable == 20) {
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
	int t;

	if(debug['x'])
		prtree(n, "indx");
	t = 0;

loop:
	l = n->left;
	r = n->right;
	switch(r->addable) {
	default:
		if(t) {
			diag(n, "bad indx");
			break;
		}
		n->right = l;
		n->left = r;
		t++;
		goto loop;

	case 10:
	case 11:
		if(l->op == ONAME && r->op == ONAME)
		if(l->etype == TIND)
		if(r->etype != TIND) {
			n->right = l;
			n->left = r;
			goto loop;
		}
		if(l->addable == 1 || l->addable == 12) {
			n->right = l;
			n->left = r;
			goto loop;
		}

	case 1:
	case 12:
		break;
	}
	if(l->addable != 7) {
		idx.regtree = l;
		idx.scale = 0;
	} else
	if(l->right->addable == 20) {
		idx.regtree = l->left;
		idx.scale = l->right->vconst;
	} else {
		idx.regtree = l->right;
		idx.scale = l->left->vconst;
	}
	t = ewidth[idx.regtree->type->etype];
	if(t == SZ_LONG)
		idx.scale += 4;
	else
	if(t != SZ_SHORT)
		diag(n, "index not W or L");

	idx.basetree = r;
	if(debug['x']) {
		print("scale = %d\n", idx.scale);
		prtree(idx.regtree, "index");
		prtree(idx.basetree, "base");
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
		doinc(n, PRE);
		boolgen(n, 1, D_NONE, Z, n);
	} else
		gbranch(OGOTO);
}

Node*
nodconst(long v)
{

	return (Node*)v;
}

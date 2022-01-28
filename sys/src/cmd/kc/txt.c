#include "gc.h"

void
ginit(void)
{
	int i;
	Type *t;

	thechar = 'k';
	thestring = "sparc";
	exregoffset = REGEXT;
	exfregoffset = FREGEXT;
	listinit();
	nstring = 0;
	mnstring = 0;
	nrathole = 0;
	pc = 0;
	breakpc = -1;
	continpc = -1;
	cases = C;
	firstp = P;
	lastp = P;
	tfield = types[TLONG];

	zprog.link = P;
	zprog.as = AGOK;
	zprog.reg = NREG;
	zprog.from.type = D_NONE;
	zprog.from.name = D_NONE;
	zprog.from.reg = NREG;
	zprog.to = zprog.from;

	regnode.op = OREGISTER;
	regnode.class = CEXREG;
	regnode.reg = 0;
	regnode.complex = 0;
	regnode.addable = 11;
	regnode.type = types[TLONG];

	constnode.op = OCONST;
	constnode.class = CXXX;
	constnode.complex = 0;
	constnode.addable = 20;
	constnode.type = types[TLONG];

	fconstnode.op = OCONST;
	fconstnode.class = CXXX;
	fconstnode.complex = 0;
	fconstnode.addable = 20;
	fconstnode.type = types[TDOUBLE];

	nodsafe = new(ONAME, Z, Z);
	nodsafe->sym = slookup(".safe");
	nodsafe->type = types[TINT];
	nodsafe->etype = types[TINT]->etype;
	nodsafe->class = CAUTO;
	complex(nodsafe);

	t = typ(TARRAY, types[TCHAR]);
	symrathole = slookup(".rathole");
	symrathole->class = CGLOBL;
	symrathole->type = t;

	nodrat = new(ONAME, Z, Z);
	nodrat->sym = symrathole;
	nodrat->type = types[TIND];
	nodrat->etype = TVOID;
	nodrat->class = CGLOBL;
	complex(nodrat);
	nodrat->type = t;

	nodret = new(ONAME, Z, Z);
	nodret->sym = slookup(".ret");
	nodret->type = types[TIND];
	nodret->etype = TIND;
	nodret->class = CPARAM;
	nodret = new(OIND, nodret, Z);
	complex(nodret);

	com64init();

	memset(reg, 0, sizeof(reg));
	reg[REGZERO] = 1;
	reg[REGLINK] = 1;
	reg[REGTMP] = 1;
	for(i=NREG; i<NREG+NREG; i+=2)
		reg[i+1] = 1;
}

void
gclean(void)
{
	int i;
	Sym *s;

	for(i=0; i<NREG; i++)
		if(i != REGZERO && i != REGTMP && i != REGLINK)
		if(reg[i])
			diag(Z, "reg %d left allocated", i);
	for(i=NREG; i<NREG+NREG; i+=2)
		if(reg[i])
			diag(Z, "freg %d left allocated", i-NREG);
	while(mnstring)
		outstring("", 1L);
	symstring->type->width = nstring;
	symrathole->type->width = nrathole;
	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		if(s->type == T)
			continue;
		if(s->type->width == 0)
			continue;
		if(s->class != CGLOBL && s->class != CSTATIC)
			continue;
		if(s->type == types[TENUM])
			continue;
		gpseudo(AGLOBL, s, nodconst(s->type->width));
	}
	nextpc();
	p->as = AEND;
	outcode();
}

void
nextpc(void)
{

	p = alloc(sizeof(*p));
	*p = zprog;
	p->lineno = nearln;
	pc++;
	if(firstp == P) {
		firstp = p;
		lastp = p;
		return;
	}
	lastp->link = p;
	lastp = p;
}

void
gargs(Node *n, Node *tn1, Node *tn2)
{
	long regs;
	Node fnxargs[20], *fnxp;

	regs = cursafe;

	fnxp = fnxargs;
	garg1(n, tn1, tn2, 0, &fnxp);	/* compile fns to temps */

	curarg = 0;
	fnxp = fnxargs;
	garg1(n, tn1, tn2, 1, &fnxp);	/* compile normal args and temps */

	cursafe = regs;
}

void
garg1(Node *n, Node *tn1, Node *tn2, int f, Node **fnxp)
{
	Node nod;

	if(n == Z)
		return;
	if(n->op == OLIST) {
		garg1(n->left, tn1, tn2, f, fnxp);
		garg1(n->right, tn1, tn2, f, fnxp);
		return;
	}
	if(f == 0) {
		if(n->complex >= FNX) {
			regsalloc(*fnxp, n);
			nod = znode;
			nod.op = OAS;
			nod.left = *fnxp;
			nod.right = n;
			nod.type = n->type;
			cgen(&nod, Z);
			(*fnxp)++;
		}
		return;
	}
	if(typesuv[n->type->etype]) {
		regaalloc(tn2, n);
		if(n->complex >= FNX) {
			sugen(*fnxp, tn2, n->type->width);
			(*fnxp)++;
		} else
			sugen(n, tn2, n->type->width);
		return;
	}
	if(REGARG && curarg == 0 && typechlp[n->type->etype]) {
		regaalloc1(tn1, n);
		if(n->complex >= FNX) {
			cgen(*fnxp, tn1);
			(*fnxp)++;
		} else
			cgen(n, tn1);
		return;
	}
	if(vconst(n) == 0) {
		regaalloc(tn2, n);
		gopcode(OAS, n, Z, tn2);
		return;
	}
	regalloc(tn1, n, Z);
	if(n->complex >= FNX) {
		cgen(*fnxp, tn1);
		(*fnxp)++;
	} else
		cgen(n, tn1);
	regaalloc(tn2, n);
	gopcode(OAS, tn1, Z, tn2);
	regfree(tn1);
}

Node*
nod32const(vlong v)
{
	constnode.vconst = v & MASK(32);
	return &constnode;
}

Node*
nodconst(long v)
{
	constnode.vconst = v;
	return &constnode;
}

Node*
nodfconst(double d)
{
	fconstnode.fconst = d;
	return &fconstnode;
}

void
nodreg(Node *n, Node *nn, int reg)
{
	*n = regnode;
	n->reg = reg;
	n->type = nn->type;
	n->lineno = nn->lineno;
}

void
regret(Node *n, Node *nn)
{
	int r;

	r = REGRET;
	if(typefd[nn->type->etype])
		r = FREGRET+NREG;
	nodreg(n, nn, r);
	reg[r]++;
}

void
regalloc(Node *n, Node *tn, Node *o)
{
	int i, j;
	static int lasti;

	switch(tn->type->etype) {
	case TCHAR:
	case TUCHAR:
	case TSHORT:
	case TUSHORT:
	case TINT:
	case TUINT:
	case TLONG:
	case TULONG:
	case TIND:
		if(o != Z && o->op == OREGISTER) {
			i = o->reg;
			if(i > 0 && i < NREG)
				goto out;
		}
		j = lasti + REGRET+1;
		for(i=REGRET+1; i<NREG; i++) {
			if(j >= NREG)
				j = REGRET+1;
			if(reg[j] == 0) {
				i = j;
				goto out;
			}
			j++;
		}
		diag(tn, "out of fixed registers");
		goto err;

	case TFLOAT:
	case TDOUBLE:
	case TVLONG:
		if(o != Z && o->op == OREGISTER) {
			i = o->reg;
			if(i >= NREG && i < NREG+NREG)
				goto out;
		}
		j = lasti*2 + NREG;
		for(i=NREG; i<NREG+NREG; i+=2) {
			if(j >= NREG+NREG)
				j = NREG;
			if(reg[j] == 0) {
				i = j;
				goto out;
			}
			j += 2;
		}
		diag(tn, "out of float registers");
		goto err;
	}
	diag(tn, "unknown type in regalloc: %T", tn->type);
err:
	i = 0;
out:
	if(i)
		reg[i]++;
	lasti++;
	if(lasti >= 5)
		lasti = 0;
	nodreg(n, tn, i);
}

void
regialloc(Node *n, Node *tn, Node *o)
{
	Node nod;

	nod = *tn;
	nod.type = types[TIND];
	regalloc(n, &nod, o);
}

void
regfree(Node *n)
{
	int i;

	i = 0;
	if(n->op != OREGISTER && n->op != OINDREG)
		goto err;
	i = n->reg;
	if(i < 0 || i >= sizeof(reg))
		goto err;
	if(reg[i] <= 0)
		goto err;
	reg[i]--;
	return;
err:
	diag(n, "error in regfree: %d", i);
}

void
regsalloc(Node *n, Node *nn)
{
	cursafe = align(cursafe, nn->type, Aaut3);
	maxargsafe = maxround(maxargsafe, cursafe+curarg);
	*n = *nodsafe;
	n->xoffset = -(stkoff + cursafe);
	n->type = nn->type;
	n->etype = nn->type->etype;
	n->lineno = nn->lineno;
}

void
regaalloc1(Node *n, Node *nn)
{
	nodreg(n, nn, REGARG);
	reg[REGARG]++;
	curarg = align(curarg, nn->type, Aarg1);
	curarg = align(curarg, nn->type, Aarg2);
	maxargsafe = maxround(maxargsafe, cursafe+curarg);
}

void
regaalloc(Node *n, Node *nn)
{
	curarg = align(curarg, nn->type, Aarg1);
	*n = *nn;
	n->op = OINDREG;
	n->reg = REGSP;
	n->xoffset = curarg + SZ_LONG;
	n->complex = 0;
	n->addable = 20;
	curarg = align(curarg, nn->type, Aarg2);
	maxargsafe = maxround(maxargsafe, cursafe+curarg);
}

void
regind(Node *n, Node *nn)
{

	if(n->op != OREGISTER) {
		diag(n, "regind not OREGISTER");
		return;
	}
	n->op = OINDREG;
	n->type = nn->type;
}

void
raddr(Node *n, Prog *p)
{
	Adr a;

	naddr(n, &a);
	if(a.type == D_CONST && a.offset == 0) {
		a.type = D_REG;
		a.reg = 0;
	}
	if(a.type != D_REG && a.type != D_FREG) {
		if(n)
			diag(n, "bad in raddr: %O", n->op);
		else
			diag(n, "bad in raddr: <null>");
		p->reg = NREG;
	} else
		p->reg = a.reg;
}

void
naddr(Node *n, Adr *a)
{
	long v;

	a->type = D_NONE;
	if(n == Z)
		return;
	switch(n->op) {
	default:
	bad:
		diag(n, "bad in naddr: %O", n->op);
		break;

	case OREGISTER:
		a->type = D_REG;
		a->sym = S;
		a->reg = n->reg;
		if(a->reg >= NREG) {
			a->type = D_FREG;
			a->reg -= NREG;
		}
		break;

	case OIND:
		naddr(n->left, a);
		if(a->type == D_REG) {
			a->type = D_OREG;
			break;
		}
		if(a->type == D_CONST) {
			a->type = D_OREG;
			break;
		}
		goto bad;

	case OINDREG:
		a->type = D_OREG;
		a->sym = S;
		a->offset = n->xoffset;
		a->reg = n->reg;
		break;

	case ONAME:
		a->etype = n->etype;
		a->type = D_OREG;
		a->name = D_STATIC;
		a->sym = n->sym;
		a->offset = n->xoffset;
		if(n->class == CSTATIC)
			break;
		if(n->class == CEXTERN || n->class == CGLOBL) {
			a->name = D_EXTERN;
			break;
		}
		if(n->class == CAUTO) {
			a->name = D_AUTO;
			break;
		}
		if(n->class == CPARAM) {
			a->name = D_PARAM;
			break;
		}
		goto bad;

	case OCONST:
		a->sym = S;
		a->reg = NREG;
		if(typefd[n->type->etype]) {
			a->type = D_FCONST;
			a->dval = n->fconst;
		} else {
			a->type = D_CONST;
			a->offset = n->vconst;
		}
		break;

	case OADDR:
		naddr(n->left, a);
		if(a->type == D_OREG) {
			a->type = D_CONST;
			break;
		}
		goto bad;

	case OADD:
		if(n->left->op == OCONST) {
			naddr(n->left, a);
			v = a->offset;
			naddr(n->right, a);
		} else {
			naddr(n->right, a);
			v = a->offset;
			naddr(n->left, a);
		}
		a->offset += v;
		break;

	}
}

void
fop(int as, int f1, int f2, Node *t)
{
	Node nod1, nod2, nod3;

	nodreg(&nod1, t, NREG+f1);
	nodreg(&nod2, t, NREG+f2);
	regalloc(&nod3, t, t);
	gopcode(as, &nod1, &nod2, &nod3);
	gmove(&nod3, t);
	regfree(&nod3);
}

void
gmove(Node *f, Node *t)
{
	int ft, tt, a;
	Node nod;
	Prog *p1;
	double d;

	ft = f->type->etype;
	tt = t->type->etype;

	if(ft == TDOUBLE && f->op == OCONST) {
		d = f->fconst;
		if(d == 0.0) {
			a = FREGZERO;
			goto ffreg;
		}
		if(d == 0.5) {
			a = FREGHALF;
			goto ffreg;
		}
		if(d == 1.0) {
			a = FREGONE;
			goto ffreg;
		}
		if(d == 2.0) {
			a = FREGTWO;
			goto ffreg;
		}
		if(d == -.5) {
			fop(OSUB, FREGHALF, FREGZERO, t);
			return;
		}
		if(d == -1.0) {
			fop(OSUB, FREGONE, FREGZERO, t);
			return;
		}
		if(d == -2.0) {
			fop(OSUB, FREGTWO, FREGZERO, t);
			return;
		}
		if(d == 1.5) {
			fop(OADD, FREGONE, FREGHALF, t);
			return;
		}
		if(d == 2.5) {
			fop(OADD, FREGTWO, FREGHALF, t);
			return;
		}
		if(d == 3.0) {
			fop(OADD, FREGTWO, FREGONE, t);
			return;
		}
	}
	if(ft == TFLOAT && f->op == OCONST) {
		d = f->fconst;
		if(d == 0) {
			a = FREGZERO;
		ffreg:
			nodreg(&nod, f, NREG+a);
			gmove(&nod, t);
			return;
		}
	}
	/*
	 * a load --
	 * put it into a register then
	 * worry what to do with it.
	 */
	if(f->op == ONAME || f->op == OINDREG || f->op == OIND) {
		switch(ft) {
		default:
			if(typefd[tt]) {
				/* special case can load mem to Freg */
				regalloc(&nod, t, t);
				gins(AMOVW, f, &nod);
				a = AFMOVWD;
				if(tt == TFLOAT)
					a = AFMOVWF;
				gins(a, &nod, &nod);
				gmove(&nod, t);
				regfree(&nod);
				return;
			}
			a = AMOVW;
			break;
		case TFLOAT:
			a = AFMOVF;
			break;
		case TDOUBLE:
			a = AFMOVD;
			break;
		case TCHAR:
			a = AMOVB;
			break;
		case TUCHAR:
			a = AMOVBU;
			break;
		case TSHORT:
			a = AMOVH;
			break;
		case TUSHORT:
			a = AMOVHU;
			break;
		}
		regalloc(&nod, f, t);
		gins(a, f, &nod);
		gmove(&nod, t);
		regfree(&nod);
		return;
	}

	/*
	 * a store --
	 * put it into a register then
	 * store it.
	 */
	if(t->op == ONAME || t->op == OINDREG || t->op == OIND) {
		switch(tt) {
		default:
			if(typefd[ft]) {
				/* special case can store mem from Freg */
				regalloc(&nod, f, Z);
				a = AFMOVDW;
				if(ft == TFLOAT)
					a = AFMOVFW;
				gins(a, f, &nod);
				gins(AMOVW, &nod, t);
				regfree(&nod);
				return;
			}
			a = AMOVW;
			break;
		case TUCHAR:
			a = AMOVBU;
			break;
		case TCHAR:
			a = AMOVB;
			break;
		case TUSHORT:
			a = AMOVHU;
			break;
		case TSHORT:
			a = AMOVH;
			break;
		case TFLOAT:
			a = AFMOVF;
			break;
		case TVLONG:
		case TDOUBLE:
			a = AFMOVD;
			break;
		}
		if(!typefd[ft] && vconst(f) == 0) {
			gins(a, f, t);
			return;
		}
		if(ft == tt)
			regalloc(&nod, t, f);
		else
			regalloc(&nod, t, Z);
		gmove(f, &nod);
		gins(a, &nod, t);
		regfree(&nod);
		return;
	}

	/*
	 * type x type cross table
	 */
	a = AGOK;
	switch(ft) {
	case TDOUBLE:
	case TVLONG:
	case TFLOAT:
		switch(tt) {
		case TDOUBLE:
		case TVLONG:
			a = AFMOVD;
			if(ft == TFLOAT)
				a = AFMOVFD;
			break;
		case TFLOAT:
			a = AFMOVDF;
			if(ft == TFLOAT)
				a = AFMOVF;
			break;
		case TLONG:
		case TULONG:
		case TIND:
		case TINT:
		case TUINT:
		case TSHORT:
		case TUSHORT:
		case TCHAR:
		case TUCHAR:
			regalloc(&nod, f, Z);	/* should be type float */
			a = AFMOVDW;
			if(ft == TFLOAT)
				a = AFMOVFW;
			gins(a, f, &nod);
			gins(AFMOVF, &nod, nodrat);
			regfree(&nod);
			gins(AMOVW, nodrat, t);
			gmove(t, t);
			if(nrathole < SZ_LONG)
				nrathole = SZ_LONG;
			return;
		}
		break;
	case TINT:
	case TUINT:
	case TLONG:
	case TULONG:
	case TIND:
		switch(tt) {
		case TDOUBLE:
		case TVLONG:
		case TFLOAT:
			goto fxtofl;
		case TLONG:
		case TULONG:
		case TINT:
		case TUINT:
		case TIND:
		case TSHORT:
		case TUSHORT:
		case TCHAR:
		case TUCHAR:
			a = AMOVW;
			break;
		}
		break;
	case TSHORT:
		switch(tt) {
		case TDOUBLE:
		case TVLONG:
		case TFLOAT:
			goto fxtofl;
		case TUINT:
		case TINT:
		case TULONG:
		case TLONG:
		case TIND:
			a = AMOVH;
			break;
		case TSHORT:
		case TUSHORT:
		case TCHAR:
		case TUCHAR:
			a = AMOVW;
			break;
		}
		break;
	case TUSHORT:
		switch(tt) {
		case TDOUBLE:
		case TVLONG:
		case TFLOAT:
			goto fxtofl;
		case TINT:
		case TUINT:
		case TLONG:
		case TULONG:
		case TIND:
			a = AMOVHU;
			break;
		case TSHORT:
		case TUSHORT:
		case TCHAR:
		case TUCHAR:
			a = AMOVW;
			break;
		}
		break;
	case TCHAR:
		switch(tt) {
		case TDOUBLE:
		case TVLONG:
		case TFLOAT:
			goto fxtofl;
		case TINT:
		case TUINT:
		case TLONG:
		case TULONG:
		case TIND:
		case TSHORT:
		case TUSHORT:
			a = AMOVB;
			break;
		case TCHAR:
		case TUCHAR:
			a = AMOVW;
			break;
		}
		break;
	case TUCHAR:
		switch(tt) {
		case TDOUBLE:
		case TVLONG:
		case TFLOAT:
		fxtofl:
			regalloc(&nod, t, t);	/* should be type float */
			gins(AMOVW, f, nodrat);
			gins(AFMOVF, nodrat, &nod);
			a = AFMOVWD;
			if(tt == TFLOAT)
				a = AFMOVWF;
			gins(a, &nod, t);
			regfree(&nod);
			if(nrathole < SZ_LONG)
				nrathole = SZ_LONG;
			if(ft == TULONG) {
				regalloc(&nod, t, Z);
				if(tt == TFLOAT) {
					gins(AFCMPF, t, Z);
					p->to.type = D_FREG;
					p->to.reg = FREGZERO;
					gins(AFBGE, Z, Z);
					p1 = p;
					gins(AFMOVF, nodfconst(4294967296.), &nod);
					gins(AFADDF, &nod, t);
				} else {
					gins(AFCMPD, t, Z);
					p->to.type = D_FREG;
					p->to.reg = FREGZERO;
					gins(AFBGE, Z, Z);
					p1 = p;
					gins(AFMOVD, nodfconst(4294967296.), &nod);
					gins(AFADDD, &nod, t);
				}
				patch(p1, pc);
				regfree(&nod);
			}
			return;
		case TINT:
		case TUINT:
		case TLONG:
		case TULONG:
		case TIND:
		case TSHORT:
		case TUSHORT:
			a = AMOVBU;
			break;
		case TCHAR:
		case TUCHAR:
			a = AMOVW;
			break;
		}
		break;
	}
	if(a == AGOK)
		diag(Z, "bad opcode in gmove %T -> %T", f->type, t->type);
	if(a == AMOVW || a == AFMOVF || a == AFMOVD)
	if(samaddr(f, t))
		return;
	gins(a, f, t);
}

void
gins(int a, Node *f, Node *t)
{

	nextpc();
	p->as = a;
	if(f != Z)
		naddr(f, &p->from);
	if(t != Z)
		naddr(t, &p->to);
	if(debug['g'])
		print("%P\n", p);
}

void
gopcode(int o, Node *f1, Node *f2, Node *t)
{
	int a, et;
	Adr ta;

	et = TLONG;
	if(f1 != Z && f1->type != T)
		et = f1->type->etype;
	a = AGOK;
	switch(o) {
	case OAS:
		gmove(f1, t);
		return;

	case OASADD:
	case OADD:
		a = AADD;
		if(et == TFLOAT)
			a = AFADDF;
		else
		if(et == TDOUBLE || et == TVLONG)
			a = AFADDD;
		break;

	case OASSUB:
	case OSUB:
		a = ASUB;
		if(et == TFLOAT)
			a = AFSUBF;
		else
		if(et == TDOUBLE || et == TVLONG)
			a = AFSUBD;
		break;

	case OASOR:
	case OOR:
		a = AOR;
		break;

	case OASAND:
	case OAND:
		a = AAND;
		break;

	case OASXOR:
	case OXOR:
		a = AXOR;
		break;

	case OASLSHR:
	case OLSHR:
		a = ASRL;
		break;

	case OASASHR:
	case OASHR:
		a = ASRA;
		break;

	case OASASHL:
	case OASHL:
		a = ASLL;
		break;

	case OFUNC:
		a = AJMPL;
		break;

	case OASLMUL:
	case OLMUL:
	case OASMUL:
	case OMUL:
		if(et == TFLOAT) {
			a = AFMULF;
			break;
		} else
		if(et == TDOUBLE || et == TVLONG) {
			a = AFMULD;
			break;
		}
		a = AMUL;
		break;

	case OASDIV:
	case ODIV:
		if(et == TFLOAT) {
			a = AFDIVF;
			break;
		} else
		if(et == TDOUBLE || et == TVLONG) {
			a = AFDIVD;
			break;
		}
		a = ADIV;
		break;

	case OASMOD:
	case OMOD:
		a = AMOD;
		break;

	case OASLMOD:
	case OLMOD:
		a = AMODL;
		break;

	case OASLDIV:
	case OLDIV:
		a = ADIVL;
		break;

	case OEQ:
		a = ABE;
		if(typefd[et])
			a = AFBE;
		goto cmp;

	case ONE:
		a = ABNE;
		if(typefd[et])
			a = AFBLG;
		goto cmp;

	case OLT:
		a = ABL;
		if(typefd[et])
			a = AFBL;
		goto cmp;

	case OLE:
		a = ABLE;
		if(typefd[et])
			a = AFBLE;
		goto cmp;

	case OGE:
		a = ABGE;
		if(typefd[et])
			a = AFBGE;
		goto cmp;

	case OGT:
		a = ABG;
		if(typefd[et])
			a = AFBG;
		goto cmp;

	case OLO:
		a = ABCS;
		goto cmp;

	case OLS:
		a = ABLEU;
		goto cmp;

	case OHS:
		a = ABCC;
		goto cmp;

	case OHI:
		a = ABGU;
		goto cmp;

	cmp:
		nextpc();
		p->as = ACMP;
		if(et == TFLOAT)
			p->as = AFCMPF;
		else
		if(et == TDOUBLE || et == TVLONG)
			p->as = AFCMPD;
		if(f1 != Z)
			naddr(f1, &p->from);
		if(t != Z)
			naddr(t, &p->to);
		if(f1 == Z || t == Z || f2 != Z)
			diag(Z, "bad cmp in gopcode %O", o);
		if(debug['g'])
			print("%P\n", p);
		f1 = Z;
		f2 = Z;
		t = Z;
		break;
	}
	if(a == AGOK)
		diag(Z, "bad in gopcode %O", o);
	nextpc();
	p->as = a;
	if(f1 != Z)
		naddr(f1, &p->from);
	if(f2 != Z) {
		naddr(f2, &ta);
		p->reg = ta.reg;
		if(ta.type == D_CONST && ta.offset == 0)
			p->reg = REGZERO;
	}
	if(t != Z)
		naddr(t, &p->to);
	if(debug['g'])
		print("%P\n", p);
}

samaddr(Node *f, Node *t)
{

	if(f->op != t->op)
		return 0;
	switch(f->op) {

	case OREGISTER:
		if(f->reg != t->reg)
			break;
		return 1;
	}
	return 0;
}

void
gbranch(int o)
{
	int a;

	a = AGOK;
	switch(o) {
	case ORETURN:
		a = ARETURN;
		break;
	case OGOTO:
		a = AJMP;
		break;
	}
	nextpc();
	if(a == AGOK) {
		diag(Z, "bad in gbranch %O",  o);
		nextpc();
	}
	p->as = a;
}

void
patch(Prog *op, long pc)
{

	op->to.offset = pc;
	op->to.type = D_BRANCH;
}

void
gpseudo(int a, Sym *s, Node *n)
{

	nextpc();
	p->as = a;
	p->from.type = D_OREG;
	p->from.sym = s;
	if(a == ATEXT)
		p->reg = (profileflg ? 0 : NOPROF);
	p->from.name = D_EXTERN;
	if(s->class == CSTATIC)
		p->from.name = D_STATIC;
	naddr(n, &p->to);
	if(a == ADATA || a == AGLOBL)
		pc--;
}

int
sval(long v)
{

	if(v >= -(1<<12) && v < (1<<12))
		return 1;
	return 0;
}

int
sconst(Node *n)
{
	vlong vv;

	if(n->op == OCONST) {
		if(!typefd[n->type->etype]) {
			vv = n->vconst;
			if(vv >= -(vlong)(1<<12) && vv < (vlong)(1<<12))
				return 1;
		}
	}
	return 0;
}

long
exreg(Type *t)
{
	long o;

	if(typechlp[t->etype]) {
		if(exregoffset <= 3)
			return 0;
		o = exregoffset;
		exregoffset--;
		return o;
	}
	if(typefd[t->etype]) {
		if(exfregoffset <= 16)
			return 0;
		o = exfregoffset + NREG;
		exfregoffset--;
		return o;
	}
	return 0;
}

schar	ewidth[NTYPE] =
{
	-1,		/* [TXXX] */
	SZ_CHAR,	/* [TCHAR] */
	SZ_CHAR,	/* [TUCHAR] */
	SZ_SHORT,	/* [TSHORT] */
	SZ_SHORT,	/* [TUSHORT] */
	SZ_INT,		/* [TINT] */
	SZ_INT,		/* [TUINT] */
	SZ_LONG,	/* [TLONG] */
	SZ_LONG,	/* [TULONG] */
	SZ_VLONG,	/* [TVLONG] */
	SZ_VLONG,	/* [TUVLONG] */
	SZ_FLOAT,	/* [TFLOAT] */
	SZ_DOUBLE,	/* [TDOUBLE] */
	SZ_IND,		/* [TIND] */
	0,		/* [TFUNC] */
	-1,		/* [TARRAY] */
	0,		/* [TVOID] */
	-1,		/* [TSTRUCT] */
	-1,		/* [TUNION] */
	SZ_INT,		/* [TENUM] */
};

long	ncast[NTYPE] =
{
	0,				/* [TXXX] */
	BCHAR|BUCHAR,			/* [TCHAR] */
	BCHAR|BUCHAR,			/* [TUCHAR] */
	BSHORT|BUSHORT,			/* [TSHORT] */
	BSHORT|BUSHORT,			/* [TUSHORT] */
	BINT|BUINT|BLONG|BULONG|BIND,	/* [TINT] */
	BINT|BUINT|BLONG|BULONG|BIND,	/* [TUINT] */
	BINT|BUINT|BLONG|BULONG|BIND,	/* [TLONG] */
	BINT|BUINT|BLONG|BULONG|BIND,	/* [TULONG] */
	BVLONG|BUVLONG,			/* [TVLONG] */
	BVLONG|BUVLONG,			/* [TUVLONG] */
	BFLOAT,				/* [TFLOAT] */
	BDOUBLE,			/* [TDOUBLE] */
	BLONG|BULONG|BIND,		/* [TIND] */
	0,				/* [TFUNC] */
	0,				/* [TARRAY] */
	0,				/* [TVOID] */
	BSTRUCT,			/* [TSTRUCT] */
	BUNION,				/* [TUNION] */
	0,				/* [TENUM] */
};

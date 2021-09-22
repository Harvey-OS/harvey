#include "gc.h"

static	char	resvreg[nelem(reg)];

#define	isv(et)	((et) == TVLONG || (et) == TUVLONG || (et) == TIND)

void
ginit(void)
{
	Type *t;

	thechar = '7';
	thestring = "arm64";
	exregoffset = REGEXT;
	exfregoffset = FREGEXT;
	newvlongcode = 1;
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
	tfield = types[TVLONG];		/* was TLONG */

	typeswitch = typechlv;
	typeword = typechlvp;
	typecmplx = typesu;
	/* TO DO */
	memmove(typechlpv, typechlp, sizeof(typechlpv));
	typechlpv[TVLONG] = 1;
	typechlpv[TUVLONG] = 1;

	zprog.link = P;
	zprog.as = AGOK;
	zprog.reg = NREG;
	zprog.from.type = D_NONE;
	zprog.from.name = D_NONE;
	zprog.from.reg = NREG;
	zprog.to = zprog.from;

	regnode.op = OREGISTER;
	regnode.class = CEXREG;
	regnode.reg = REGTMP;
	regnode.complex = 0;
	regnode.addable = 11;
	regnode.type = types[TLONG];

	qregnode = regnode;
	qregnode.type = types[TVLONG];

	constnode.op = OCONST;
	constnode.class = CXXX;
	constnode.complex = 0;
	constnode.addable = 20;
	constnode.type = types[TLONG];

	vconstnode = constnode;
	vconstnode.type = types[TVLONG];

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
	/* don't allocate */
	reg[REGTMP] = 1;
	reg[REGSB] = 1;
	reg[REGSP] = 1;
	reg[REGZERO] = 1;
	/* keep two external registers */
	reg[REGEXT] = 1;
	reg[REGEXT-1] = 1;
	memmove(resvreg, reg, sizeof(reg));
}

void
gclean(void)
{
	int i;
	Sym *s;

	for(i=0; i<NREG; i++)
		if(reg[i] && !resvreg[i])
			diag(Z, "reg %d left allocated", i);
	for(i=NREG; i<NREG+NFREG; i++)
		if(reg[i] && !resvreg[i])
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
	if(typesu[n->type->etype]) {
		regaalloc(tn2, n);
		if(n->complex >= FNX) {
			sugen(*fnxp, tn2, n->type->width);
			(*fnxp)++;
		} else
			sugen(n, tn2, n->type->width);
		return;
	}
	if(REGARG >= 0 && curarg == 0 && typechlpv[n->type->etype]) {
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
nodgconst(vlong v, Type *t)
{
	if(!typev[t->etype])
		return nodconst((long)v);
	vconstnode.vconst = v;
	return &vconstnode;
}

Node*
nodconst(long v)
{
	constnode.vconst = v;
	return &constnode;
}

Node*
nod32const(vlong v)
{
	constnode.vconst = v & MASK(32);
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
	*n = qregnode;
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
	case TVLONG:
	case TUVLONG:
	case TIND:
		if(o != Z && o->op == OREGISTER) {
			i = o->reg;
			if(i >= 0 && i < NREG)
				goto out;
		}
		j = lasti + REGRET+1;
		for(i=REGRET+1; i<NREG; i++) {
			if(j >= NREG)
				j = REGRET+1;
			if(reg[j] == 0 && resvreg[j] == 0) {
				i = j;
				goto out;
			}
			j++;
		}
		diag(tn, "out of fixed registers");
		goto err;

	case TFLOAT:
	case TDOUBLE:
		if(o != Z && o->op == OREGISTER) {
			i = o->reg;
			if(i >= NREG && i < NREG+NFREG)
				goto out;
		}
		j = lasti + NREG;
		for(i=NREG; i<NREG+NFREG; i++) {
			if(j >= NREG+NFREG)
				j = NREG;
			if(reg[j] == 0) {
				i = j;
				goto out;
			}
			j++;
		}
		diag(tn, "out of float registers");
		goto err;
	}
	diag(tn, "unknown type in regalloc: %T", tn->type);
err:
	nodreg(n, tn, 0);
	return;
out:
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
	n->xoffset = curarg + SZ_VLONG;
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
		a.reg = REGZERO;
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
print("bad addr %D\n", a);
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

	ft = f->type->etype;
	tt = t->type->etype;

	if(ft == TDOUBLE && f->op == OCONST) {
	}
	if(ft == TFLOAT && f->op == OCONST) {
	}

	/*
	 * a load --
	 * put it into a register then
	 * worry what to do with it.
	 */
	if(f->op == ONAME || f->op == OINDREG || f->op == OIND) {
		switch(ft) {
		default:
			if(ewidth[ft] == 4){
				if(typeu[ft])
					a = AMOVWU;
				else
					a = AMOVW;
			}else
				a = AMOV;
			break;
		case TINT:
			a = AMOVW;
			break;
		case TUINT:
			a = AMOVWU;
			break;
		case TFLOAT:
			a = AFMOVS;
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
			if(ewidth[tt] == 4)
				a = AMOVW;
			else
				a = AMOV;
			break;
		case TINT:
			a = AMOVW;
			break;
		case TUINT:
			a = AMOVWU;
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
			a = AFMOVS;
			break;
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
	case TFLOAT:
		switch(tt) {
		case TDOUBLE:
			a = AFMOVD;
			if(ft == TFLOAT)
				a = AFCVTSD;
			break;
		case TFLOAT:
			a = AFMOVS;
			if(ft == TDOUBLE)
				a = AFCVTDS;
			break;
		case TCHAR:
		case TSHORT:
		case TINT:
		case TLONG:
			a = AFCVTZSDW;
			if(ft == TFLOAT)
				a = AFCVTZSSW;
			break;
		case TUCHAR:
		case TUSHORT:
		case TUINT:
		case TULONG:
			a = AFCVTZUDW;
			if(ft == TFLOAT)
				a = AFCVTZUSW;
			break;
		case TVLONG:
			a = AFCVTZSD;
			if(ft == TFLOAT)
				a = AFCVTZSS;
			break;
		case TUVLONG:
		case TIND:
			a = AFCVTZUD;
			if(ft == TFLOAT)
				a = AFCVTZUS;
			break;
		}
		break;
	case TUINT:
	case TULONG:
	case TINT:
	case TLONG:
		switch(tt) {
		case TDOUBLE:
			if(ft == TUINT || ft == TULONG)
				gins(AUCVTFWD, f, t);
			else
				gins(ASCVTFWD, f, t);
			return;
		case TFLOAT:
			if(ft == TUINT || ft == TULONG)
				gins(AUCVTFWS, f, t);
			else
				gins(ASCVTFWS, f, t);
			return;
		case TINT:
		case TUINT:
		case TLONG:
		case TULONG:
		case TSHORT:
		case TUSHORT:
		case TCHAR:
		case TUCHAR:
			if(typeu[tt])
				a = AMOVWU;
			else
				a = AMOVW;
			break;
		case TVLONG:
		case TUVLONG:
		case TIND:
			a = AMOV;
			break;
		}
		break;
	case TVLONG:
	case TUVLONG:
	case TIND:
		switch(tt) {
		case TDOUBLE:
			if(ft == TVLONG)
				gins(ASCVTFD, f, t);
			else
				gins(AUCVTFD, f, t);
			return;
		case TFLOAT:
			if(ft == TVLONG)
				gins(ASCVTFS, f, t);
			else
				gins(AUCVTFS, f, t);
			return;
		case TINT:
		case TUINT:
		case TLONG:
		case TULONG:
		case TVLONG:
		case TUVLONG:
		case TIND:
		case TSHORT:
		case TUSHORT:
		case TCHAR:
		case TUCHAR:
			a = AMOV;	/* TO DO: conversion done? */
			break;
		}
		break;
	case TSHORT:
		switch(tt) {
		case TDOUBLE:
			regalloc(&nod, f, Z);
			gins(AMOVH, f, &nod);
			gins(ASCVTFWD, &nod, t);
			regfree(&nod);
			return;
		case TFLOAT:
			regalloc(&nod, f, Z);
			gins(AMOVH, f, &nod);
			gins(ASCVTFWS, &nod, t);
			regfree(&nod);
			return;
		case TINT:
		case TUINT:
		case TLONG:
		case TULONG:
		case TVLONG:
		case TUVLONG:
		case TIND:
			a = AMOVH;
			break;
		case TSHORT:
		case TUSHORT:
		case TCHAR:
		case TUCHAR:
			a = AMOV;
			break;
		}
		break;
	case TUSHORT:
		switch(tt) {
		case TDOUBLE:
			regalloc(&nod, f, Z);
			gins(AMOVHU, f, &nod);
			gins(AUCVTFWD, &nod, t);
			regfree(&nod);
			return;
		case TFLOAT:
			regalloc(&nod, f, Z);
			gins(AMOVHU, f, &nod);
			gins(AUCVTFWS, &nod, t);
			regfree(&nod);
			return;
		case TINT:
		case TUINT:
		case TLONG:
		case TULONG:
		case TVLONG:
		case TUVLONG:
		case TIND:
			a = AMOVHU;
			break;
		case TSHORT:
		case TUSHORT:
		case TCHAR:
		case TUCHAR:
			a = AMOV;
			break;
		}
		break;
	case TCHAR:
		switch(tt) {
		case TDOUBLE:
			regalloc(&nod, f, Z);
			gins(AMOVB, f, &nod);
			gins(ASCVTFWD, &nod, t);
			regfree(&nod);
			return;
		case TFLOAT:
			regalloc(&nod, f, Z);
			gins(AMOVB, f, &nod);
			gins(ASCVTFWS, &nod, t);
			regfree(&nod);
			return;
		case TINT:
		case TUINT:
		case TLONG:
		case TULONG:
		case TIND:
		case TSHORT:
		case TUSHORT:
		case TVLONG:
		case TUVLONG:
			a = AMOVB;
			break;
		case TCHAR:
		case TUCHAR:
			a = AMOV;
			break;
		}
		break;
	case TUCHAR:
		switch(tt) {
		case TDOUBLE:
			regalloc(&nod, f, Z);
			gins(AMOVBU, f, &nod);
			gins(AUCVTFWD, &nod, t);
			regfree(&nod);
			return;
		case TFLOAT:
			regalloc(&nod, f, Z);
			gins(AMOVBU, f, &nod);
			gins(AUCVTFWS, &nod, t);
			regfree(&nod);
			return;
		case TINT:
		case TUINT:
		case TLONG:
		case TULONG:
		case TIND:
		case TSHORT:
		case TUSHORT:
		case TVLONG:
		case TUVLONG:
			a = AMOVBU;
			break;
		case TCHAR:
		case TUCHAR:
			a = AMOV;
			break;
		}
		break;
	}
	if(a == AGOK)
		diag(Z, "bad opcode in gmove %T -> %T", f->type, t->type);
	if(a == AMOV || (AMOVW || a == AMOVWU) && ewidth[ft] == ewidth[tt] || a == AFMOVS || a == AFMOVD)
	if(samaddr(f, t))
		return;
	gins(a, f, t);
}

void
gmover(Node *f, Node *t)
{
	int ft, tt, a;

	ft = f->type->etype;
	tt = t->type->etype;
	a = AGOK;
	if(typechlp[ft] && typechlp[tt] && ewidth[ft] >= ewidth[tt]){
		switch(tt){
		case TSHORT:
			a = AMOVH;
			break;
		case TUSHORT:
			a = AMOVHU;
			break;
		case TCHAR:
			a = AMOVB;
			break;
		case TUCHAR:
			a = AMOVBU;
			break;
		case TINT:
			a = AMOVW;
			break;
		case TUINT:
			a = AMOVWU;
			break;
		}
	}
	if(a == AGOK)
		gmove(f, t);
	else
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
	int a, et, true;
	Adr ta;

	et = TLONG;
	if(f1 != Z && f1->type != T) {
		et = f1->type->etype;
		if(f1->op == OCONST){
			if(t != Z && t->type != T)
				et = t->type->etype;
			else if(f2 != Z && f2->type != T && ewidth[f2->type->etype] > ewidth[et])
				et = f2->type->etype;
		}
	}
	true = o & BTRUE;
	o &= ~BTRUE;
	a = AGOK;
	switch(o) {
	case OAS:
		gmove(f1, t);
		return;

	case OASADD:
	case OADD:
		a = AADDW;
		if(isv(et))
			a = AADD;
		else if(et == TFLOAT)
			a = AFADDS;
		else if(et == TDOUBLE)
			a = AFADDD;
		break;

	case OASSUB:
	case OSUB:
		a = ASUBW;
		if(isv(et))
			a = ASUB;
		else if(et == TFLOAT)
			a = AFSUBS;
		else if(et == TDOUBLE)
			a = AFSUBD;
		break;

	case OASOR:
	case OOR:
		a = AORRW;
		if(isv(et))
			a = AORR;
		break;

	case OASAND:
	case OAND:
		a = AANDW;
		if(isv(et))
			a = AAND;
		break;

	case OASXOR:
	case OXOR:
		a = AEORW;
		if(isv(et))
			a = AEOR;
		break;

	case OASLSHR:
	case OLSHR:
		a = ALSRW;
		if(isv(et))
			a = ALSR;
		break;

	case OASASHR:
	case OASHR:
		a = AASRW;
		if(isv(et))
			a = AASR;
		break;

	case OASASHL:
	case OASHL:
		a = ALSLW;
		if(isv(et))
			a = ALSL;
		break;

	case OFUNC:
		a = ABL;
		break;

	case OASMUL:
	case OMUL:
		a = AMULW;
		if(isv(et))
			a = AMUL;
		else if(et == TFLOAT)
			a = AFMULS;
		else if(et == TDOUBLE)
			a = AFMULD;
		break;

	case OASDIV:
	case ODIV:
		a = ASDIVW;
		if(isv(et))
			a = ASDIV;
		else if(et == TFLOAT)
			a = AFDIVS;
		else if(et == TDOUBLE)
			a = AFDIVD;
		break;

	case OASMOD:
	case OMOD:
		a = AREMW;
		if(isv(et))
			a = AREM;
		break;

	case OASLMUL:
	case OLMUL:
		a = AUMULL;
		if(isv(et))
			a = AMUL;
		break;

	case OASLMOD:
	case OLMOD:
		a = AUREMW;
		if(isv(et))
			a = AUREM;
		break;

	case OASLDIV:
	case OLDIV:
		a = AUDIVW;
		if(isv(et))
			a = AUDIV;
		break;

	case OCOM:
		a = AMVNW;
		if(isv(et))
			a = AMVN;
		break;

	case ONEG:
		a = ANEGW;
		if(isv(et))
			a = ANEG;
		break;

	case OCASE:
		a = ACASE;	/* ACASEW? */
		break;

	case OEQ:
	case ONE:
	case OLT:
	case OLE:
	case OGE:
	case OGT:
	case OLO:
	case OLS:
	case OHS:
	case OHI:
		a = ACMPW;
		if(isv(et))
			a = ACMP;
		if(et == TFLOAT)
			a = AFCMPS;
		else if(et == TDOUBLE)
			a = AFCMPD;
		nextpc();
		p->as = a;
		naddr(f1, &p->from);
		if(f1->op == OCONST && p->from.offset < 0){
			if(a == ACMPW && (ulong)p->from.offset != 0x80000000UL) {
				p->as = ACMNW;
				p->from.offset = -p->from.offset;
			}else if(a == ACMP && p->from.offset != 0x8000000000000000LL){
				p->as = ACMN;
				p->from.offset = -p->from.offset;
			}
		}
		raddr(f2, p);
		switch(o) {
		case OEQ:
			a = ABEQ;
			break;
		case ONE:
			a = ABNE;
			break;
		case OLT:
			a = ABLT;
			/* ensure NaN comparison is always false */
			if(typefd[et] && !true)
				a = ABMI;
			break;
		case OLE:
			a = ABLE;
			if(typefd[et] && !true)
				a = ABLS;
			break;
		case OGE:
			a = ABGE;
			if(typefd[et] && true)
				a = ABPL;
			break;
		case OGT:
			a = ABGT;
			if(typefd[et] && true)
				a = ABHI;
			break;
		case OLO:
			a = ABLO;
			break;
		case OLS:
			a = ABLS;
			break;
		case OHS:
			a = ABHS;
			break;
		case OHI:
			a = ABHI;
			break;
		}
		f1 = Z;
		f2 = Z;
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
	}
	if(t != Z)
		naddr(t, &p->to);
	if(debug['g'])
		print("%P\n", p);
}

int
samaddr(Node *f, Node *t)
{
	return f->op == OREGISTER && t->op == OREGISTER && f->reg == t->reg;
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
		a = AB;
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
	p->from.name = D_EXTERN;
	if(a == ATEXT)
		p->reg = (profileflg ? 0 : NOPROF);
	if(s->class == CSTATIC)
		p->from.name = D_STATIC;
	naddr(n, &p->to);
	if(a == ADATA || a == AGLOBL)
		pc--;
}

int
sconst(Node *n)
{
	vlong vv;

	if(n->op == OCONST) {
		if(!typefd[n->type->etype]) {
			vv = n->vconst;
			if(vv >= (vlong)(-32766) && vv < (vlong)32766)
				return 1;
			/*
			 * should be specialised for constant values which will
			 * fit in different instructionsl; for now, let 5l
			 * sort it out
			 */
			return 1;
		}
	}
	return 0;
}

static int
isaddcon(vlong v)
{
	/* uimm12 or uimm24? */
	if(v < 0)
		return 0;
	if((v & 0xFFF) == 0)
		v >>= 12;
	return v <= 0xFFF;
}

int
sval(long v)
{
	return isaddcon(v) || isaddcon(-v);
}

int
usableoffset(Node *n, vlong o, Node *v)
{
	int s, et;

	et = v->type->etype;
	if(v->op != OCONST || typefd[et])
		return 0;
	s = n->type->width;
	if(s > 16)
		s = 16;
	o += v->vconst;
	return o >= -256 || o < 4095*s;
}

long
exreg(Type *t)
{
	long o;

	if(typechlpv[t->etype]) {
		if(exregoffset <= REGEXT-2)
			return 0;
		o = exregoffset;
		if(reg[o] && !resvreg[o])
			return 0;
		resvreg[o] = reg[o] = 1;
		exregoffset--;
		return o;
	}
	if(typefd[t->etype]) {
		if(exfregoffset <= NFREG-1)
			return 0;
		o = exfregoffset + NREG;
		if(reg[o] && !resvreg[o])
			return 0;
		resvreg[o] = reg[o] = 1;
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
	BINT|BUINT|BLONG|BULONG,	/* [TINT] */
	BINT|BUINT|BLONG|BULONG,	/* [TUINT] */
	BINT|BUINT|BLONG|BULONG,	/* [TLONG] */
	BINT|BUINT|BLONG|BULONG,	/* [TULONG] */
	BVLONG|BUVLONG|BIND,			/* [TVLONG] */
	BVLONG|BUVLONG|BIND,			/* [TUVLONG] */
	BFLOAT,				/* [TFLOAT] */
	BDOUBLE,			/* [TDOUBLE] */
	BVLONG|BUVLONG|BIND,		/* [TIND] */
	0,				/* [TFUNC] */
	0,				/* [TARRAY] */
	0,				/* [TVOID] */
	BSTRUCT,			/* [TSTRUCT] */
	BUNION,				/* [TUNION] */
	0,				/* [TENUM] */
};

#include "gc.h"

static long ncast64[];

extern void ccmain(int, char**);

void
main(int argc, char **argv)
{
	char *p;
	int oargc;
	char **oargv;

	thechar = 'i';
	p = strrchr(argv[0], '/');
	if(p == nil)
		p = argv[0];
	else
		p++;
	if(*p == 'j')
		thechar = 'j';
	oargc = argc;
	oargv = argv;
	ARGBEGIN {
	case 'j':
		thechar = 'j';
		break;
	case 'o':
	case 'D':
	case 'I':
		p = ARGF();
	} ARGEND
	USED(p);

	if(thechar == 'j'){
		thestring = "riscv64";
		ewidth[TIND] = 8;
	}else{
		thestring = "riscv";
		ewidth[TIND] = 4;
	}
	ccmain(oargc, oargv);
}

void
ginit(void)
{
	int i;
	Type *t;

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

	if(thechar == 'j'){
		typeword = typechlvp;
		typeswitch = typechlv;
		typecmplx = typesu;
	}

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

	vregnode = regnode;
	vregnode.type = types[TVLONG];

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

	if(thechar == 'i'){
		com64init();
	}else{
		memmove(ncast, ncast64, NTYPE*sizeof(long));
	}

	for(i=0; i<nelem(reg); i++) {
		reg[i] = 0;
		if(i == REGZERO)
			reg[i] = 1;
	}
}

void
gclean(void)
{
	int i;
	Sym *s;

	for(i=0; i<NREG; i++)
		if(i != REGZERO)
			if(reg[i])
				diag(Z, "reg %d left allocated", i);
	for(i=NREG; i<NREG+NREG; i++)
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
	if(typecmplx[n->type->etype]) {
		regaalloc(tn2, n);
		if(n->complex >= FNX) {
			sugen(*fnxp, tn2, n->type->width);
			(*fnxp)++;
		} else
			sugen(n, tn2, n->type->width);
		return;
	}
	if(REGARG && curarg == 0 && typeword[n->type->etype]) {
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

int
tmpreg(void)
{
	int i;

	for(i=REGRET+1; i<REGALLOC; i++)
		if(reg[i] == 0)
			return i;
	diag(Z, "out of fixed registers");
	return 0;
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
			if(i > 0 && i < NREG)
				goto out;
		}
		j = lasti + REGRET+1;
		for(i=REGRET+1; i<=REGALLOC; i++) {
			if(j > REGALLOC)
				j = REGRET+1;
			if(reg[j] == 0) {
				i = j;
				goto out0;
			}
			j++;
		}
		diag(tn, "out of fixed registers");
		goto err;

	case TFLOAT:
	case TDOUBLE:
		if(o != Z && o->op == OREGISTER) {
			i = o->reg;
			if(i >= NREG && i < NREG+NREG)
				goto out;
		}
		for(j=NREG; j<NREG+16; j++) {
			if(reg[j] == 0) {
				i = j;
				goto out;
			}
		}
		diag(tn, "out of float registers");
		goto err;
	}
	diag(tn, "unknown type in regalloc: %T", tn->type);
err:
	nodreg(n, tn, 0);
	return;
out0:
	lasti++;
	if(lasti >= 5)
		lasti = 0;
out:
	reg[i]++;
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
	cursafe = align(cursafe + stkoff, nn->type, Aaut3) - stkoff;
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
	n->xoffset = curarg + ewidth[TIND];
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
			v = (long)n->vconst;
			if(thechar == 'j' && (vlong)v != n->vconst) {
				a->type = D_VCONST;
				*(vlong*)a->sval = n->vconst;
			} else {
				a->type = D_CONST;
				a->offset = n->vconst;
			}
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
	double d;

	ft = f->type->etype;
	tt = t->type->etype;

	if(debug['O'])
		print("gmove: %O[%T],%O[%T]\n",
			f->op, f->type, t->op, t->type);
	if(ft == TDOUBLE && f->op == OCONST) {
		d = f->fconst;
		if(d == 0) {
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
			a = AMOV;
			break;
		case TFLOAT:
			a = AMOVF;
			break;
		case TDOUBLE:
			a = AMOVD;
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
		case TINT:
		case TLONG:
			a = AMOVW;
			break;
		case TUINT:
		case TULONG:
			a = AMOVW;	/* sic */
			break;
		case TIND:
			a = thechar == 'j' ? AMOV : AMOVW;
			break;
		}
		if(typechlp[ft] && typeilp[tt])
			regalloc(&nod, t, t);
		else
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
			a = AMOVW;
			break;
		case TUCHAR:
			if(!debug['N'] || debug['R'] || debug['P']){
				a = AMOVBU;
				break;
			}
		case TCHAR:
			a = AMOVB;
			break;
		case TUSHORT:
			if(!debug['N'] || debug['R'] || debug['P']){
				a = AMOVHU;
				break;
			}
		case TSHORT:
			a = AMOVH;
			break;
		case TFLOAT:
			a = AMOVF;
			break;
		case TDOUBLE:
			a = AMOVD;
			break;
		case TVLONG:
		case TUVLONG:
			a = AMOV;
			break;
		case TIND:
			a = thechar == 'j' ? AMOV : AMOVW;
			break;
		}
		if(!typefd[ft] && vconst(f) == 0) {
#ifdef notyet
			nodreg(&nod, f, REGZERO);
			gins(a, &nod, t);
#else
			gins(a, f, t);
#endif
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
			a = AMOVD;
			if(ft == TFLOAT)
				a = AMOVFD;
			break;
		case TFLOAT:
			a = AMOVDF;
			if(ft == TFLOAT)
				a = AMOVF;
			break;
		case TVLONG:
		case TUVLONG:
			a = AMOVDV;
			if(ft == TFLOAT)
				a = AMOVFV;
			break;
		case TINT:
		case TUINT:
		case TLONG:
		case TULONG:
		case TIND:
		case TSHORT:
		case TUSHORT:
		case TCHAR:
		case TUCHAR:
			a = AMOVDW;
			if(ft == TFLOAT)
				a = AMOVFW;
			break;
		}
		break;

	case TVLONG:
	case TUVLONG:
	fvlong:
		switch(tt) {
		case TDOUBLE:
			a = ft == TUVLONG ? AMOVUVD : AMOVVD;
			break;
		case TFLOAT:
			a = ft == TUVLONG ? AMOVUVF : AMOVVF;
			break;
		case TVLONG:
		case TUVLONG:
		case TIND:
			a = AMOV;
			break;
		case TINT:
		case TUINT:
		case TLONG:
		case TULONG:
		case TSHORT:
		case TUSHORT:
		case TCHAR:
		case TUCHAR:
			a = AMOVW;
			break;
		}
		break;

	case TINT:
	case TLONG:
		switch(tt) {
		case TDOUBLE:
			gins(AMOVWD, f, t);
			return;
		case TFLOAT:
			gins(AMOVWF, f, t);
			return;
		case TVLONG:
		case TUVLONG:
			a = AMOVW;
			break;
		case TINT:
		case TUINT:
		case TLONG:
		case TULONG:
		case TIND:
		case TSHORT:
		case TUSHORT:
		case TCHAR:
		case TUCHAR:
			a = AMOV;
			break;
		}
		break;

	case TIND:
		if(thechar == 'j')
			goto fvlong;
	/* fall through */
	case TUINT:
	case TULONG:
		switch(tt) {
		case TDOUBLE:
			gins(AMOVUD, f, t);
			return;
		case TFLOAT:
			gins(AMOVUF, f, t);
			return;
		case TVLONG:
		case TUVLONG:
			a = AMOVWU;
			break;
		case TINT:
		case TUINT:
		case TLONG:
		case TULONG:
		case TIND:
		case TSHORT:
		case TUSHORT:
		case TCHAR:
		case TUCHAR:
			if(thechar == 'j' && f->op == OCONST)
				a = AMOVW;
			else
				a = AMOV;
			break;
		}
		break;

	case TSHORT:
		switch(tt) {
		case TDOUBLE:
			regalloc(&nod, f, Z);
			gins(AMOVH, f, &nod);
			gins(AMOVWD, &nod, t);
			regfree(&nod);
			return;
		case TFLOAT:
			regalloc(&nod, f, Z);
			gins(AMOVH, f, &nod);
			gins(AMOVWF, &nod, t);
			regfree(&nod);
			return;
		case TINT:
		case TUINT:
		case TLONG:
		case TULONG:
		case TIND:
		case TVLONG:
		case TUVLONG:
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
			gins(AMOVWD, &nod, t);
			regfree(&nod);
			return;
		case TFLOAT:
			regalloc(&nod, f, Z);
			gins(AMOVHU, f, &nod);
			gins(AMOVWF, &nod, t);
			regfree(&nod);
			return;
		case TINT:
		case TUINT:
		case TLONG:
		case TULONG:
		case TIND:
		case TVLONG:
		case TUVLONG:
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
			gins(AMOVWD, &nod, t);
			regfree(&nod);
			return;
		case TFLOAT:
			regalloc(&nod, f, Z);
			gins(AMOVB, f, &nod);
			gins(AMOVWF, &nod, t);
			regfree(&nod);
			return;
		case TINT:
		case TUINT:
		case TLONG:
		case TULONG:
		case TIND:
		case TVLONG:
		case TUVLONG:
		case TSHORT:
		case TUSHORT:
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
			gins(AMOVWD, &nod, t);
			regfree(&nod);
			return;
		case TFLOAT:
			regalloc(&nod, f, Z);
			gins(AMOVBU, f, &nod);
			gins(AMOVWF, &nod, t);
			regfree(&nod);
			return;
		case TINT:
		case TUINT:
		case TLONG:
		case TULONG:
		case TIND:
		case TVLONG:
		case TUVLONG:
		case TSHORT:
		case TUSHORT:
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
	if(a == AMOV || (thechar == 'i' && (a == AMOVW || a == AMOVWU)) || a == AMOVF || a == AMOVD)
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
	int a, ab, et, tword;
	Adr ta;

	tword = 0;
	if(thechar == 'j')
		if(t != Z && t->type != T)
			if((et = t->type->etype) != TIND && !typev[et])
				tword = 1;
	et = TLONG;
	if(f1 != Z && f1->type != T)
		et = f1->type->etype;
	if(debug['O']) {
		if(f1 != Z && f1->type != T)
			print("gop: %O %O[%s],", o, f1->op, tnames[et]);
		else
			print("gop: %O Z,", o);
		if(f2 != Z && f2->type != T)
			print("%O[%s],", f2->op, tnames[f2->type->etype]);
		if(t != Z && t->type != T)
			print("%O[%s]\n", t->op, tnames[t->type->etype]);
		else
			print("Z\n");
	}
	a = AGOK;
	switch(o) {
	case OAS:
		gmove(f1, t);
		return;

	case OASADD:
	case OADD:
		a = tword ? AADDW : AADD;
		if(et == TFLOAT)
			a = AADDF;
		else
		if(et == TDOUBLE)
			a = AADDD;
		break;

	case OASSUB:
	case OSUB:
		a = tword ? ASUBW : ASUB;
		if(et == TFLOAT)
			a = ASUBF;
		else
		if(et == TDOUBLE)
			a = ASUBD;
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
		a = tword ? ASRLW : ASRL;
		break;

	case OASASHR:
	case OASHR:
		a = tword ? ASRAW : ASRA;
		break;

	case OASASHL:
	case OASHL:
		a = tword ? ASLLW : ASLL;
		break;

	case OFUNC:
		a = AJAL;
		break;

	case OCOND:
		a = ASLTU;
		break;

	case OCOMMA:
		a = ASLT;
		break;

	case OASMUL:
	case OMUL:
		if(et == TFLOAT) {
			a = AMULF;
			break;
		} else
		if(et == TDOUBLE) {
			a = AMULD;
			break;
		}
		a = tword ? AMULW : AMUL;
		break;

	case OASDIV:
	case ODIV:
		if(et == TFLOAT) {
			a = ADIVF;
			break;
		} else
		if(et == TDOUBLE) {
			a = ADIVD;
			break;
		}
		a = tword ? ADIVW : ADIV;
		break;

	case OASMOD:
	case OMOD:
		a = tword ? AREMW : AREM;
		break;

	case OASLMUL:
	case OLMUL:
		a = tword ? AMULW : AMUL;
		break;

	case OASLMOD:
	case OLMOD:
		a = tword ? AREMUW : AREMU;
		break;

	case OASLDIV:
	case OLDIV:
		a = tword ? ADIVUW : ADIVU;
		break;

	case OEQ:
		if(!typefd[et]) {
			a = ABEQ;
			break;
		}
		a = et == TFLOAT? ACMPEQF : ACMPEQD;
		ab = ABNE;
		goto cmpfloat;
	case ONE:
		if(!typefd[et]) {
			a = ABNE;
			break;
		}
		a = et == TFLOAT? ACMPEQF : ACMPEQD;
		ab = ABEQ;
		goto cmpfloat;
	case OLT:
		if(!typefd[et]) {
			a = ABLT;
			break;
		}
		a = et == TFLOAT? ACMPLTF : ACMPLTD;
		ab = ABNE;
		goto cmpfloat;
	case OLE:
		if(!typefd[et]) {
			a = ABLE;	/* pseudo op */
			break;
		}
		a = et == TFLOAT? ACMPLEF : ACMPLED;
		ab = ABNE;
		goto cmpfloat;
	case OGE:
		if(!typefd[et]) {
			a = ABGE;
			break;
		}
		a = et == TFLOAT? ACMPLTF : ACMPLTD;
		ab = ABEQ;
		goto cmpfloat;
	case OGT:
		if(!typefd[et]) {
			a = ABGT;	/* pseudo op */
			break;
		}
		a = et == TFLOAT? ACMPLEF : ACMPLED;
		ab = ABEQ;
		goto cmpfloat;
	case OLO:
		a = ABLTU;
		break;
	case OLS:
		a = ABLEU;	/* pseudo op */
		break;
	case OHS:
		a = ABGEU;
		break;
	case OHI:
		a = ABGTU;	/* pseudo op */
		break;

	cmpfloat:
		nextpc();
		p->as = a;
		naddr(f1, &p->from);
		raddr(f2, p);
		if(t != Z)
			naddr(t, &p->to);
		else {
			naddr(&regnode, &p->to);
			p->to.reg = tmpreg();
		}
		if(debug['g'])
			print("%P\n", p);
		if(t == Z) {
			nextpc();
			p->as = ab;
			naddr(&regnode, &p->from);
			p->from.reg = tmpreg();
			if(debug['g'])
				print("%P\n", p);
		} else if (ab == ABNE) {
			nextpc();
			p->as = AXOR;
			naddr(nodconst(1), &p->from);
			raddr(t, p);
			naddr(t, &p->to);
			if(debug['g'])
				print("%P\n", p);
		}
		return;
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

int
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
		a = ARET;
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
sconst(Node *n)
{
	vlong vv;

	if(n->op == OCONST) {
		if(!typefd[n->type->etype]) {
			vv = n->vconst;
			if(vv >= (vlong)(-2048) && vv < (vlong)2048)
				return 1;
		}
	}
	return 0;
}

int
sval(long v)
{
	if(v >= -32766L && v < 32766L)
		return 1;
	return 0;
}

long
exreg(Type *t)
{
	long o;

	if(typechlp[t->etype]) {
		if(exregoffset <= REGTMP)
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
	0,		/* [TIND] - set to 4 or 8 in main */
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

static long ncast64[NTYPE] =
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
	BVLONG|BUVLONG|BIND,		/* [TVLONG] */
	BVLONG|BUVLONG|BIND,		/* [TUVLONG] */
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

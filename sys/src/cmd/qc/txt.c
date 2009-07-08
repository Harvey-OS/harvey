#include "gc.h"

static	int	resvreg[nelem(reg)];

static	void	gopcode64(int, Node*, Node*, Node*);
static	void	gori64(int, Node*, Node*, Node*);
static	void	gandi64(int, Node*, Node*, Node*);

void
ginit(void)
{
	Type *t;

	thechar = 'q';
	thestring = "power";
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
	zprog.from3 = zprog.from;
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

	com64init();

	memset(reg, 0, sizeof(reg));
	reg[REGZERO] = 1;	/* don't use */
	reg[REGTMP] = 1;
	reg[FREGCVI+NREG] = 1;
	reg[FREGZERO+NREG] = 1;
	reg[FREGHALF+NREG] = 1;
	reg[FREGONE+NREG] = 1;
	reg[FREGTWO+NREG] = 1;
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
	for(i=NREG; i<NREG+NREG; i++)
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
	if(typesuv[n->type->etype]) {
		regaalloc(tn2, n);
		if(n->complex >= FNX) {
			cgen(*fnxp, tn2);
			(*fnxp)++;
		} else
			cgen(n, tn2);
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
		if(o != Z && o->op == OREGISTER) {
			i = o->reg;
			if(i >= NREG && i < NREG+NREG)
				goto out;
		}
		j = lasti + NREG;
		for(i=NREG; i<NREG+NREG; i++) {
			if(j >= NREG+NREG)
				j = NREG;
			if(reg[j] == 0) {
				i = j;
				goto out;
			}
			j++;
		}
		diag(tn, "out of float registers");
		goto err;

	case TVLONG:
	case TUVLONG:
		n->op = OREGPAIR;
		n->complex = 0;	/* already in registers */
		n->addable = 11;
		n->type = tn->type;
		n->lineno = nearln;
		n->left = alloc(sizeof(Node));
		n->right = alloc(sizeof(Node));
		if(o != Z && o->op == OREGPAIR) {
			regalloc(n->left, &regnode, o->left);
			regalloc(n->right, &regnode, o->right);
		} else {
			regalloc(n->left, &regnode, Z);
			regalloc(n->right, &regnode, Z);
		}
		n->right->type = types[TULONG];
		if(tn->type->etype == TUVLONG)
			n->left->type = types[TULONG];	/* TO DO: is this a bad idea? */
		return;
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

	if(n->op == OREGPAIR) {
		regfree(n->left);
		regfree(n->right);
		return;
	}
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
	diag(n, "error in regfree: %d [%d]", i, reg[i]);
	prtree(n, "regfree");
}

void
regsalloc(Node *n, Node *nn)
{
	cursafe = align(cursafe+stkoff, nn->type, Aaut3)-stkoff;
	maxargsafe = maxround(maxargsafe, cursafe+curarg);
//	if(nn->type->etype == TDOUBLE || nn->type->etype == TVLONG){
//		extern int hasdoubled;
//		fprint(2, "stkoff=%ld cursafe=%ld curarg=%ld %d\n", stkoff, cursafe, curarg, hasdoubled);
//	}
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
	if(R0ISZERO && a.type == D_CONST && a.offset == 0) {
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
		a->offset += n->xoffset;	/* little hack for reglcgenv */
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
gloadhi(Node *f, Node *t, int c)
{
	Type *ot;

	if(f->op == OCONST){
		f = nodconst((long)(f->vconst>>32));
		if(c==1 && sconst(f) || c==2 && uconst(f)){
			if(t->op == OREGISTER)
				regfree(t);
			*t = *f;
			return;
		}
	}
	if(f->op == OREGPAIR) {
		gmove(f->left, t);
		return;
	}
	ot = f->type;
	f->type = types[TLONG];
	gmove(f, t);
	f->type = ot;
}

void
gloadlo(Node *f, Node *t, int c)
{
	Type *ot;

	if(f->op == OCONST){
		f = nodconst((long)f->vconst);
		if(c && uconst(f)){
			if(t->op == OREGISTER)
				regfree(t);
			*t = *f;
			return;
		}
	}
	if(f->op == OREGPAIR) {
		gmove(f->right, t);
		return;
	}
	ot = f->type;
	f->type = types[TLONG];
	f->xoffset += SZ_LONG;
	if(0){
		prtree(f, "gloadlo f"); prtree(t, "gloadlo t");
	}
	gmove(f, t);
	f->xoffset -= SZ_LONG;
	f->type = ot;
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
	Node nod, fxc0, fxc1, fxc2, fxrat;
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
	if((ft == TVLONG || ft == TUVLONG) && f->op == OCONST && t->op == OREGPAIR) {
		if(align(0, types[TCHAR], Aarg1))	/* isbigendian */
			gmove(nod32const(f->vconst>>32), t->left);
		else
			gmove(nod32const(f->vconst), t->left);
		if(align(0, types[TCHAR], Aarg1))	/* isbigendian */
			gmove(nod32const(f->vconst), t->right);
		else
			gmove(nod32const(f->vconst>>32), t->right);
		return;
	}
	/*
	 * a load --
	 * put it into a register then
	 * worry what to do with it.
	 */
	if(f->op == ONAME || f->op == OINDREG || f->op == OIND) {
		switch(ft) {
		default:
			a = AMOVW;
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
			a = AMOVBZ;
			break;
		case TSHORT:
			a = AMOVH;
			break;
		case TUSHORT:
			a = AMOVHZ;
			break;
		}
		if(typev[ft]) {
			if(typev[tt]) {
				regalloc(&nod, f, t);
				/* low order first, because its value will be used first */
				f->xoffset += SZ_LONG;
				gins(AMOVW, f, nod.right);
				f->xoffset -= SZ_LONG;
				gins(AMOVW, f, nod.left);
			} else {
				/* assumed not float or double */
				regalloc(&nod, &regnode, t);
				f->xoffset += SZ_LONG;
				gins(AMOVW, f, &nod);
				f->xoffset -= SZ_LONG;
			}
		} else {
			regalloc(&nod, f, t);
			gins(a, f, &nod);
		}
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
			a = AMOVBZ;
			break;
		case TCHAR:
			a = AMOVB;
			break;
		case TUSHORT:
			a = AMOVHZ;
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
		if(R0ISZERO && !typefd[ft] && vconst(f) == 0) {
			gins(a, f, t);
			if(typev[tt]) {
				t->xoffset += SZ_LONG;
				gins(a, f, t);
				t->xoffset -= SZ_LONG;
			}
			return;
		}
		if(ft == tt)
			regalloc(&nod, t, f);
		else
			regalloc(&nod, t, Z);
		gmove(f, &nod);
		if(typev[tt]) {
			t->xoffset += SZ_LONG;
			gins(a, nod.right, t);
			t->xoffset -= SZ_LONG;
			gins(a, nod.left, t);
		} else
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
				a = AFMOVS;	/* AFMOVSD */
			break;
		case TFLOAT:
			a = AFRSP;
			if(ft == TFLOAT)
				a = AFMOVS;
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
			/* BUG: not right for unsigned long */
			regalloc(&nod, f, Z);	/* should be type float */
			regsalloc(&fxrat, &fconstnode);
			gins(AFCTIWZ, f, &nod);
			gins(AFMOVD, &nod, &fxrat);
			regfree(&nod);
			fxrat.type = nodrat->type;
			fxrat.etype = nodrat->etype;
			fxrat.xoffset += 4;
			gins(AMOVW, &fxrat, t);
			gmove(t, t);
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
		case TFLOAT:
			goto fxtofl;
		case TINT:
		case TUINT:
		case TLONG:
		case TULONG:
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
		case TFLOAT:
			goto fxtofl;
		case TINT:
		case TUINT:
		case TLONG:
		case TULONG:
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
		case TFLOAT:
			goto fxtofl;
		case TINT:
		case TUINT:
		case TLONG:
		case TULONG:
		case TIND:
			a = AMOVHZ;
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
		case TFLOAT:
		fxtofl:
			/*
			 * rat[0] = 0x43300000; rat[1] = f^0x80000000;
			 * t = *(double*)rat - FREGCVI;
			 * is-unsigned(t) => if(t<0) t += 2^32;
			 * could be streamlined for int-to-float
			 */
			regalloc(&fxc0, f, Z);
			regalloc(&fxc2, f, Z);
			regsalloc(&fxrat, &fconstnode);	/* should be type float */
			gins(AMOVW, nodconst(0x43300000L), &fxc0);
			gins(AMOVW, f, &fxc2);
			gins(AMOVW, &fxc0, &fxrat);
			gins(AXOR, nodconst(0x80000000L), &fxc2);
			fxc1 = fxrat;
			fxc1.type = nodrat->type;
			fxc1.etype = nodrat->etype;
			fxc1.xoffset += SZ_LONG;
			gins(AMOVW, &fxc2, &fxc1);
			regfree(&fxc2);
			regfree(&fxc0);
			regalloc(&nod, t, t);	/* should be type float */
			gins(AFMOVD, &fxrat, &nod);
			nodreg(&fxc1, t, NREG+FREGCVI);
			gins(AFSUB, &fxc1, &nod);
			a = AFMOVD;
			if(tt == TFLOAT)
				a = AFRSP;
			gins(a, &nod, t);
			regfree(&nod);
			if(ft == TULONG) {
				regalloc(&nod, t, Z);
				if(tt == TFLOAT) {
					gins(AFCMPU, t, Z);
					p->to.type = D_FREG;
					p->to.reg = FREGZERO;
					gins(ABGE, Z, Z);
					p1 = p;
					gins(AFMOVS, nodfconst(4294967296.), &nod);
					gins(AFADDS, &nod, t);
				} else {
					gins(AFCMPU, t, Z);
					p->to.type = D_FREG;
					p->to.reg = FREGZERO;
					gins(ABGE, Z, Z);
					p1 = p;
					gins(AFMOVD, nodfconst(4294967296.), &nod);
					gins(AFADD, &nod, t);
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
			a = AMOVBZ;
			break;
		case TCHAR:
		case TUCHAR:
			a = AMOVW;
			break;
		}
		break;
	case TVLONG:
	case TUVLONG:
		switch(tt) {
		case TVLONG:
		case TUVLONG:
			a = AMOVW;
			break;
		}
		break;
	}
	if(a == AGOK)
		diag(Z, "bad opcode in gmove %T -> %T", f->type, t->type);
	if(a == AMOVW || a == AFMOVS || a == AFMOVD)
	if(samaddr(f, t))
		return;
	if(typev[ft]) {
		if(f->op != OREGPAIR || t->op != OREGPAIR)
			diag(Z, "bad vlong in gmove (%O->%O)", f->op, t->op);
		gins(a, f->left, t->left);
		gins(a, f->right, t->right);
	} else
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
gins3(int a, Node *f1, Node *f2, Node *t)
{
	Adr ta;

	nextpc();
	p->as = a;
	if(f1 != Z)
		naddr(f1, &p->from);
	if(f2 != Z && (f2->op != OREGISTER || !samaddr(f2, t))) {
		ta = zprog.from;	/* TO DO */
		naddr(f2, &ta);
		p->reg = ta.reg;
		if(ta.type == D_CONST && ta.offset == 0) {
			if(R0ISZERO)
				p->reg = REGZERO;
			else
				diag(Z, "REGZERO in gins3 %A", a);
		}else if(ta.type == D_CONST)
			p->from3 = ta;
	}
	if(t != Z)
		naddr(t, &p->to);
	if(debug['g'])
		print("%P\n", p);
}

void
gins4(int a, Node *f1, Node *f2, Node *f3, Node *t)
{
	Adr ta;

	nextpc();
	p->as = a;
	naddr(f1, &p->from);
	if(f2->op != OREGISTER && (f2->op != OCONST || vconst(f2) != 0))
		diag(f2, "invalid gins4");
	naddr(f2, &ta);
	p->reg = ta.reg;
	if(ta.type == D_CONST && ta.offset == 0)
		p->reg = REGZERO;
	naddr(f3, &p->from3);
	naddr(t, &p->to);
	if(debug['g'])
		print("%P\n", p);
}

void
gopcode(int o, Node *f1, Node *f2, Node *t)
{
	int a, et, uns;

	if(o == OAS) {
		gmove(f1, t);
		return;
	}
	et = TLONG;
	if(f1 != Z && f1->type != T) {
		if(f1->op == OCONST && t != Z && t->type != T)
			et = t->type->etype;
		else
			et = f1->type->etype;
	}
	if((typev[et] || t->type != T && typev[t->type->etype]) && o != OFUNC) {
		gopcode64(o, f1, f2, t);
		return;
	}
	uns = 0;
	a = AGOK;
	switch(o) {

	case OASADD:
	case OADD:
		a = AADD;
		if(et == TFLOAT)
			a = AFADDS;
		else
		if(et == TDOUBLE)
			a = AFADD;
		break;

	case OASSUB:
	case OSUB:
		a = ASUB;
		if(et == TFLOAT)
			a = AFSUBS;
		else
		if(et == TDOUBLE)
			a = AFSUB;
		break;

	case OASOR:
	case OOR:
		a = AOR;
		break;

	case OASAND:
	case OAND:
		a = AAND;
		if(f1->op == OCONST)
			a = AANDCC;
		break;

	case OASXOR:
	case OXOR:
		a = AXOR;
		break;

	case OASLSHR:
	case OLSHR:
		a = ASRW;
		break;

	case OASASHR:
	case OASHR:
		a = ASRAW;
		break;

	case OASASHL:
	case OASHL:
		a = ASLW;
		break;

	case OFUNC:
		a = ABL;
		break;

	case OASLMUL:
	case OLMUL:
	case OASMUL:
	case OMUL:
		if(et == TFLOAT) {
			a = AFMULS;
			break;
		} else
		if(et == TDOUBLE) {
			a = AFMUL;
			break;
		}
		a = AMULLW;
		break;

	case OASDIV:
	case ODIV:
		if(et == TFLOAT) {
			a = AFDIVS;
			break;
		} else
		if(et == TDOUBLE) {
			a = AFDIV;
			break;
		}
		a = ADIVW;
		break;

	case OASMOD:
	case OMOD:
		a = AREM;
		break;

	case OASLMOD:
	case OLMOD:
		a = AREMU;
		break;

	case OASLDIV:
	case OLDIV:
		a = ADIVWU;
		break;

	case OCOM:
		a = ANOR;
		break;

	case ONEG:
		a = ANEG;
		if(et == TFLOAT || et == TDOUBLE)
			a = AFNEG;
		break;

	case OEQ:
		a = ABEQ;
		if(t->op == OCONST && t->vconst >= (1<<15))
			goto cmpu;
		goto cmp;

	case ONE:
		a = ABNE;
		if(t->op == OCONST && t->vconst >= (1<<15))
			goto cmpu;
		goto cmp;

	case OLT:
		a = ABLT;
		goto cmp;

	case OLE:
		a = ABLE;
		goto cmp;

	case OGE:
		a = ABGE;
		goto cmp;

	case OGT:
		a = ABGT;
		goto cmp;

	case OLO:
		a = ABLT;
		goto cmpu;

	case OLS:
		a = ABLE;
		goto cmpu;

	case OHS:
		a = ABGE;
		goto cmpu;

	case OHI:
		a = ABGT;
		goto cmpu;

	cmpu:
		uns = 1;
	cmp:
		nextpc();
		p->as = uns? ACMPU: ACMP;
		if(et == TFLOAT)
			p->as = AFCMPU;
		else
		if(et == TDOUBLE)
			p->as = AFCMPU;
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
	gins3(a, f1, f2, t);
}

static void
gopcode64(int o, Node *f1, Node *f2, Node *t)
{
	int a1, a2;
	Node nod, nod1, nod2, sh;
	ulong m;
	Prog *p1;

	if(t->op != OREGPAIR || f2 != Z && f2->op != OREGPAIR) {
		diag(Z, "bad f2/dest in gopcode64 %O", o);
		return;
	}
	if(f1->op != OCONST &&
	   (typev[f1->type->etype] && f1->op != OREGPAIR || !typev[f1->type->etype] && f1->op != OREGISTER)) {
		diag(Z, "bad f1[%O] in gopcode64 %O", f1->op, o);
		return;
	}
	/* a1 for low-order, a2 for high-order */
	a1 = AGOK;
	a2 = AGOK;
	switch(o) {
	case OASADD:
	case OADD:
		if(f1->op == OCONST && sconst(f1)) {
			if(f2 == Z)
				f2 = t;
			gins3(AADDC, f1, f2->right, t->right);
			if((f1->vconst>>32) == 0)
				gins(AADDZE, f2->left, t->left);
			else if((f1->vconst>>32) == -1)
				gins(AADDME, f2->left, t->left);
			else
				diag(t, "odd vlong ADD: %lld", f1->vconst);
			return;
		}
		a1 = AADDC;
		a2 = AADDE;
		break;

	case OASSUB:
	case OSUB:
		a1 = ASUBC;
		a2 = ASUBE;
		break;

	case OASOR:
	case OOR:
		if(f1->op == OCONST) {
			gori64(AOR, f1, f2, t);
			return;
		}
		a1 = a2 = AOR;
		break;

	case OASAND:
	case OAND:
		if(f1->op == OCONST) {
			gandi64(AANDCC, f1, f2, t);
			return;
		}
		a1 = a2 = AAND;
		break;

	case OASXOR:
	case OXOR:
		if(f1->op == OCONST) {
			gori64(AXOR, f1, f2, t);
			return;
		}
		a1 = a2 = AXOR;
		break;

	case OASLSHR:
	case OLSHR:
		if(f2 == Z)
			f2 = t;
		if(f1->op == OCONST) {
			if(f1->vconst >= 32) {
				if(f1->vconst == 32)
					gmove(f2->left, t->right);
				else if(f1->vconst < 64)
					gins3(ASRW, nodconst(f1->vconst-32), f2->left, t->right);
				else
					gmove(nodconst(0), t->right);
				gmove(nodconst(0), t->left);
				return;
			}
			if(f1->vconst <= 0) {
				if(f2 != t)
					gmove(f2, t);
				return;
			}
			sh = *nodconst(32 - f1->vconst);
			m = 0xFFFFFFFFUL >> f1->vconst;
			gins4(ARLWNM, &sh, f2->right, nodconst(m), t->right);
			gins4(ARLWMI, &sh, f2->left, nodconst(~m), t->right);
			gins4(ARLWNM, &sh, f2->left, nodconst(m), t->left);
			return;
		}
		regalloc(&nod, &regnode, Z);
		gins3(ASUBC, f1, nodconst(32), &nod);
		gins3(ASRW, f1, f2->right, t->right);
		regalloc(&nod1, &regnode, Z);
		gins3(ASLW, &nod, f2->left, &nod1);
		gins(AOR, &nod1, t->right);
		gins3(AADD, nodconst(-32), f1, &nod);
		gins3(ASRW, &nod, f2->left, &nod1);
		gins(AOR, &nod1, t->right);
		gins3(ASRW, f1, f2->left, t->left);
		regfree(&nod);
		regfree(&nod1);
		return;

	case OASASHR:
	case OASHR:
		if(f2 == Z)
			f2 = t;
		if(f1->op == OCONST) {
			if(f1->vconst >= 32) {
				if(f1->vconst == 32)
					gmove(f2->left, t->right);
				else if(f1->vconst < 64)
					gins3(ASRAW, nodconst(f1->vconst-32), f2->left, t->right);
				gins3(ASRAW, nodconst(31), f2->left, t->left);
				if(f1->vconst >= 64) {
					gmove(t->left, t->right);
					return;
				}
				return;
			}
			if(f1->vconst <= 0) {
				if(f2 != t)
					gmove(f2, t);
				return;
			}
			sh = *nodconst(32 - f1->vconst);
			m = 0xFFFFFFFFUL >> f1->vconst;
			gins4(ARLWNM, &sh, f2->right, nodconst(m), t->right);
			gins4(ARLWMI, &sh, f2->left, nodconst(~m), t->right);
			gins3(ASRAW, &sh, f2->left, t->left);
			return;
		}
		regalloc(&nod, &regnode, Z);
		gins3(ASUBC, f1, nodconst(32), &nod);
		gins3(ASRW, f1, f2->right, t->right);
		regalloc(&nod1, &regnode, Z);
		gins3(ASLW, &nod, f2->left, &nod1);
		gins(AOR, &nod1, t->right);
		gins3(AADDCCC, nodconst(-32), f1, &nod);
		gins3(ASRAW, &nod, f2->left, &nod1);
		gins(ABLE, Z, Z);
		p1 = p;
		gins(AMOVW, &nod1, t->right);
		patch(p1, pc);
		gins3(ASRAW, f1, f2->left, t->left);
		regfree(&nod);
		regfree(&nod1);
		return;

	case OASASHL:
	case OASHL:
		if(f2 == Z)
			f2 = t;
		if(f1->op == OCONST) {
			if(f1->vconst >= 32) {
				if(f1->vconst == 32)
					gmove(f2->right, t->left);
				else if(f1->vconst >= 64)
					gmove(nodconst(0), t->left);
				else
					gins3(ASLW, nodconst(f1->vconst-32), f2->right, t->left);
				gmove(nodconst(0), t->right);
				return;
			}
			if(f1->vconst <= 0) {
				if(f2 != t)
					gmove(f2, t);
				return;
			}
			m = 0xFFFFFFFFUL << f1->vconst;
			gins4(ARLWNM, f1, f2->left, nodconst(m), t->left);
			gins4(ARLWMI, f1, f2->right, nodconst(~m), t->left);
			gins4(ARLWNM, f1, f2->right, nodconst(m), t->right);
			return;
		}
		regalloc(&nod, &regnode, Z);
		gins3(ASUBC, f1, nodconst(32), &nod);
		gins3(ASLW, f1, f2->left, t->left);
		regalloc(&nod1, &regnode, Z);
		gins3(ASRW, &nod, f2->right, &nod1);
		gins(AOR, &nod1, t->left);
		gins3(AADD, nodconst(-32), f1, &nod);
		gins3(ASLW, &nod, f2->right, &nod1);
		gins(AOR, &nod1, t->left);
		gins3(ASLW, f1, f2->right, t->right);
		regfree(&nod);
		regfree(&nod1);
		return;

	case OASLMUL:
	case OLMUL:
	case OASMUL:
	case OMUL:
		if(f2 == Z)
			f2 = t;
		regalloc(&nod, &regnode, Z);
		gins3(AMULLW, f1->right, f2->right, &nod);	/* lo(f2.low*f1.low) */
		a1 = AMULHW;
		if(o == OLMUL || o == OASLMUL)
			a1 = AMULHWU;
		regalloc(&nod1, &regnode, Z);
		gins3(a1, f1->right, f2->right, &nod1);		/* hi(f2.low*f1.low) */
		regalloc(&nod2, &regnode, Z);
		gins3(AMULLW, f2->right, f1->left, &nod2);	/* lo(f2.low*f1.high) */
		gins(AADD, &nod2, &nod1);
		gins3(AMULLW, f1->right, f2->left, &nod2);	/* hi(f2.high*f1.low) */
		gins(AADD, &nod2, &nod1);
		regfree(&nod2);
		gmove(&nod, t->right);
		gmove(&nod1, t->left);
		regfree(&nod);
		regfree(&nod1);
		return;

	case OCOM:
		a1 = a2 = ANOR;
		break;

	case ONEG:
		gins3(ASUBC, t->right, nodconst(0), t->right);
		gins(ASUBZE, t->left, t->left);
		return;
	}
	if(a1 == AGOK || a2 == AGOK)
		diag(Z, "bad in gopcode64 %O", o);
	if(f1->op == OCONST) {
		if(f2 != Z & f2 != t)
			diag(Z, "bad const in gopcode64 %O", o);
		gins(a1, nod32const(f1->vconst), t->right);
		gins(a2, nod32const(f1->vconst>>32), t->left);
	} else {
		if(f2 != Z && f2 != t) {
			gins3(a1, f1->right, f2->right, t->right);
			gins3(a2, f1->left, f2->left, t->left);
		} else {
			gins(a1, f1->right, t->right);
			gins(a2, f1->left, t->left);
		}
	}
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

	case OREGPAIR:
		return samaddr(f->left, t->left) && samaddr(f->right, t->right);
	}
	return 0;
}

static void
gori64(int a, Node *f1, Node *f2, Node *t)
{
	ulong lo, hi;

	if(f2 == Z)
		f2 = t;
	lo = f1->vconst & MASK(32);
	hi = (f1->vconst >> 32) & MASK(32);
	if(lo & 0xFFFF)
		gins3(a, nodconst(lo & 0xFFFF), f2->right, t->right);
	if((lo >> 16) != 0)
		gins3(a, nodconst(lo & 0xFFFF0000UL), f2->right, t->right);
	if(hi & 0xFFFF)
		gins3(a, nodconst(hi & 0xFFFF), f2->left, t->left);
	if((hi >> 16) != 0)
		gins3(a, nodconst(hi & 0xFFFF0000UL), f2->left, t->left);
}

static void
gandi64(int a, Node *f1, Node *f2, Node *t)
{
	ulong lo, hi;

	if(f2 == Z)
		f2 = t;
	lo = f1->vconst & MASK(32);
	hi = (f1->vconst >> 32) & MASK(32);
	if(lo == 0)
		gins(AMOVW, nodconst(0), t->right);
	else
		gins3(a, nodconst(lo), f2->right, t->right);
	if(hi == 0)
		gins(AMOVW, nodconst(0), t->left);
	else
		gins3(a, nodconst(hi), f2->left, t->left);
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
		a = ABR;
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

	if(v >= -(1<<15) && v < (1<<15))
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
			if(vv >= -(((vlong)1)<<15) && vv < (((vlong)1)<<15))
				return 1;
		}
	}
	return 0;
}

int
uconst(Node *n)
{
	vlong vv;

	if(n->op == OCONST) {
		if(!typefd[n->type->etype]) {
			vv = n->vconst;
			if(vv >= 0 && vv < (((vlong)1)<<16))
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

#include "gc.h"

void
ginit(void)
{
	Type *t;

	thechar = 'x';
	thestring = "3210";
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
	tfield = types[TFIELD];
	if(TINT == TSHORT) {
		tint = types[TSHORT];
		tuint = types[TUSHORT];
		types[TFUNC]->link = tint;
	}
	ewidth[TENUM] = ewidth[TINT];
	types[TENUM]->width = ewidth[TENUM];
	suround = SU_ALLIGN;
	supad = SU_PAD;

	zprog.link = P;
	zprog.as = AGOK;
	zprog.reg = NREG;
	zprog.from.type = D_NONE;
	zprog.from.name = D_NONE;
	zprog.from.reg = NREG;
	zprog.from1 = zprog.from;
	zprog.from2 = zprog.from;
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
	nodsafe->type = tint;
	nodsafe->etype = tint->etype;
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

	memset(reg, 0, sizeof(reg));
	reg[REGZERO] = 1;
	reg[REGLINK] = 1;
	reg[REGSP] = 1;
	reg[REGTMP] = 1;
}

void
gclean(void)
{
	int i;
	Sym *s;

	for(i=0; i<NREG; i++)
		if(i != REGZERO && i != REGTMP && i != REGLINK && i != REGSP)
		if(reg[i])
			diag(Z, "reg %d left allocated", i);
	for(i=NREG; i<NREG+NFREG; i++)
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

	ALLOC(p, Prog);
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

/*
 * generate assignments to arguments for a function call
 */
void
gargs(Node *n, Node *tn1, Node *tn2)
{
	long regs;
	Node fnxargs[20], *fnxp;

	regs = cursafe;

	fnxp = fnxargs;
	garg1(n, tn1, tn2, 0, &fnxp, 1);	/* compile fns to temps */

	curarg = 0;
	fnxp = fnxargs;
	garg1(n, tn1, tn2, 1, &fnxp, 1);	/* compile normal args and temps */

	cursafe = regs;
}

void
garg1(Node *n, Node *tn1, Node *tn2, int f, Node **fnxp, int first)
{
	Node nod;
	long w;

	if(n == Z)
		return;
	if(n->op == OLIST) {
		garg1(n->right, tn1, tn2, f, fnxp, 0);
		garg1(n->left, tn1, tn2, f, fnxp, first);
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
	w = n->type->width;
	if(typesu[n->type->etype]) {
		if(w > tint->width)
			adjsp(w);
		regaalloc(tn2, n);
		if(n->complex >= FNX) {
			sugen(*fnxp, tn2, n->type->width);
			(*fnxp)++;
		} else
			sugen(n, tn2, n->type->width);
		if(w <= tint->width)
			adjsp(w);
		return;
	}
	if(w > tint->width)
		diag(Z, "arg not su size > 4: %d\n", w);
	if(REGARG && first && typechlp[n->type->etype]) {
		regaalloc1(tn1, n);
		if(n->complex >= FNX) {
			cgen(*fnxp, tn1);
			(*fnxp)++;
		} else
			cgen(n, tn1);
		adjsp(w);
		return;
	}
	if(vconst(n) == 0) {
		regaalloc(tn2, n);
		gopcode(OAS, n, Z, tn2);
		adjsp(w);
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
	adjsp(w);
	regfree(tn1);
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

/*
 * make n a node for register r with the attributes of nn
 */
void
nodreg(Node *n, Node *nn, int reg)
{
	*n = regnode;
	n->reg = reg;
	n->type = nn->type;
	n->lineno = nn->lineno;
}

/*
 * make n the return register with attributes of nn
 */
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

/*
 * make n a register node with same attributes as tn
 * o should be used if it's a register
 */
void
regalloc(Node *n, Node *tn, Node *o)
{
	int i, j;
	static lasti;

	switch(tn->type->etype) {
	case TCHAR:
	case TUCHAR:
	case TSHORT:
	case TUSHORT:
	case TLONG:
	case TULONG:
		if(o != Z && o->op == OREGISTER) {
			i = o->reg;
			if(i > 0 && i < NREG)
				goto out;
		}
		j = lasti + REGPTR+1;
		for(i=REGPTR+1; i<20; i++) {
			if(j > 19)
				j = REGPTR+1;
			if(reg[j] == 0) {
				i = j;
				goto out;
			}
			j++;
		}
	case TIND:
		if(o != Z && o->op == OREGISTER) {
			i = o->reg;
			if(i > 0 && i < REGMAX)
				goto out;
		}
		j = REGRET+1;
		for(i=REGRET+1; i<NREG; i++) {
			if(j > REGMAX)
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
			if(i >= NREG && i < NREG+NFREG)
				goto out;
		}
		j = NREG;
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
	i = -1;
out:
	if(i > 0)
		reg[i]++;
	lasti++;
	if(lasti >= 5)
		lasti = 0;
	nodreg(n, tn, i);
}

/*
 * make n a register node with
 * lineno of tn and type of TIND
 */
void
regialloc(Node *n, Node *tn, Node *o)
{
	Node nod;

	nod = *tn;
	nod.type = types[TIND];
	regalloc(n, &nod, o);
}

/*
 * routines to temporarily release and then
 * reaquire a register
 */
void
regrelease(Node *n)
{
	int i;

	if(n == Z)
		return;
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
	diag(n, "error in regrelease: %d", i);
}

void
regrealloc(Node *n)
{
	int i;

	if(n == Z)
		return;
	i = 0;
	if(n->op != OREGISTER && n->op != OINDREG)
		goto err;
	i = n->reg;
	if(i < 0 || i >= sizeof(reg))
		goto err;
	reg[i]++;
	return;
err:
	diag(n, "error in regrealloc: %d", i);
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

/*
 * make n a cell in the stack safe with same
 * attributes as nn
 */
void
regsalloc(Node *n, Node *nn)
{
	long o;

	o = nn->type->width;
	o += round(o, tint->width);
	cursafe += o;
	if(cursafe+curarg > maxargsafe)
		maxargsafe = cursafe+curarg;
	*n = *nodsafe;
	n->xoffset = -(stkoff+cursafe);
	n->type = nn->type;
	n->etype = nn->type->etype;
	n->lineno = nn->lineno;
}

/*
 * make n the first argument register with attributes of nn
 * allocate the next argument location for it
 */
void
regaalloc1(Node *n, Node *nn)
{
	int r;

	r = REGARG;
	nodreg(n, nn, r);
	reg[r]++;
	curarg += 4;
}

/*
 * make n a pointer to the next argument location
 */
void
regaalloc(Node *n, Node *nn)
{
	long o;

	o = nn->type->width;
	o += round(o, tint->width);
	curarg += o;
	*n = *nn;
	n->op = OINDREG;
	n->reg = REGSP;
	n->xoffset = 0;
	if(o > tint->width)
		n->xoffset = 4;
	n->complex = 0;
	n->addable = 20;
}

void
adjsp(long w)
{
	if(!w)
		return;
	nextpc();
	p->as = AADD;
	p->from.type = D_CONST;
	w += round(w, tint->width);
	p->from.offset = -w;
	p->to.type = D_REG;
	p->to.reg = REGSP;
}

/*
 * convert n from an address to a pointer of type nn
 * to that address
 */
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

/*
 * reverse a regind
 */
void
regaddr(Node *n, Node *nn)
{
	*n = *nn;

	if(n->op != OINDREG) {
		diag(n, "regaddr not OINDREG");
		return;
	}
	n->op = OREGISTER;
}

/*
 * set p's reg field to the register n represents
 */
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
	}else
		p->reg = a.reg;
}

/*
 * make address from 'addressable' node n
 */
void
naddr(Node *n, Adr *a)
{
	long v;

	a->type = D_NONE;
	if(n == Z)
		return;
	switch(n->op) {
	case OREGISTER:
		a->type = D_REG;
		a->sym = S;
		a->reg = n->reg;
		if(a->reg >= NREG) {
			a->type = D_FREG;
			a->reg -= NREG;
		}
		return;

	case OIND:
		naddr(n->left, a);
		if(a->type == D_REG) {
			a->type = D_INDREG;
			return;
		}
		if(a->type == D_CONST) {
			a->type = D_NAME;
			return;
		}
		break;

	case OINDREG:
		a->type = D_INDREG;
		a->sym = S;
		a->offset = n->xoffset;
		a->reg = n->reg;
		return;

	case ONAME:
		a->etype = n->etype;
		a->type = D_NAME;
		a->name = D_STATIC;
		a->sym = n->sym;
		a->offset = n->xoffset;
		if(n->class == CSTATIC)
			return;
		if(n->class == CEXTERN || n->class == CGLOBL) {
			a->name = D_EXTERN;
			return;
		}
		if(n->class == CAUTO) {
			a->name = D_AUTO;
			return;
		}
		if(n->class == CPARAM) {
			a->name = D_PARAM;
			return;
		}
		break;

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
		return;

	case OADDR:
		naddr(n->left, a);
		if(a->type == D_INDREG) {
			a->type = D_REG;
			return;
		}
		if(a->type == D_NAME) {
			a->type = D_CONST;
			return;
		}
		break;

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
		return;

	}
	diag(n, "bad in naddr: %O", n->op);
}

/*
 * make n a real address; nn is addressable by the compiler
 */
int
gaddr(Node *n, Node *nn)
{
	Adr a;

	*n = *nn;
	naddr(n, &a);
	switch(a.type){
	default:
		diag(n, "unknown in gaddr: %d\n", a.type);
		return 0;
	case D_FREG:
	case D_REG:
	case D_CONST:
	case D_NAME:
		return 0;
	case D_INDREG:
		if(!a.offset)
			return 0;
		break;
	}
	regialloc(n, nn, Z);
	nextpc();
	p->as = AADD;
	p->reg = a.reg;
	p->cc = 0;
	a.type = D_CONST;
	a.name = D_NONE;
	a.reg = NREG;
	p->from = a;
	p->to.type = D_REG;
	p->to.reg = n->reg;
	regind(n, nn);
	return 1;
}

/*
 * move f to t, doing any type conversions
 */
void
gmove(Node *f, Node *t)
{
	int ft, tt, a, aisreg;
	Node nod, adr;
	Prog *p1;

	ft = f->type->etype;
	tt = t->type->etype;

	/*
	 * a load --
	 * put it into a register then
	 * worry what to do with it.
	 */
	if(f->op == ONAME || f->op == OINDREG || f->op == OIND) {
		aisreg = gaddr(&adr, f);
		switch(ft) {
		default:
			if(typefd[tt]) {
				/* special case can load mem to Freg */
				regalloc(&nod, t, t);
				ginsf(AFMOVWF, &adr, Z, &nod, Z);
				gmove(&nod, t);
				regfree(&nod);
				if(aisreg)
					regfree(&adr);
				return;
			}
			a = AMOVW;
			break;
		case TFLOAT:
		case TDOUBLE:
		case TVLONG:
			a = AFMOVF;
			break;
		case TCHAR:
			a = AMOVB;
			break;
		case TUCHAR:
			a = AMOVBU;
			break;
		case TSHORT:
			if(typefd[tt]) {
				/* special case can load mem to Freg */
				regalloc(&nod, t, t);
				ginsf(AFMOVHF, &adr, Z, &nod, Z);
				gmove(&nod, t);
				regfree(&nod);
				if(aisreg)
					regfree(&adr);
				return;
			}
			a = AMOVH;
			break;
		case TUSHORT:
			a = AMOVHU;
			break;
		}
		regalloc(&nod, f, t);
		if(typefd[ft])
			ginsf(a, &adr, Z, &nod, Z);
		else
			gins(a, &adr, &nod);
		gmove(&nod, t);
		regfree(&nod);
		if(aisreg)
			regfree(&adr);
		return;
	}

	/*
	 * a store --
	 * put it into a register then
	 * store it.
	 */
	if(t->op == ONAME || t->op == OINDREG || t->op == OIND) {
		aisreg = gaddr(&adr, t);
		switch(tt) {
		default:
			if(typefd[ft]) {
				/* special case can store mem from Freg */
				regalloc(&nod, f, Z);
				ginsf(AFMOVFW, f, Z, &nod, t);
				regfree(&nod);
				return;
			}
			a = AMOVW;
			break;
		case TUCHAR:
		case TCHAR:
			a = AMOVB;
			break;
		case TSHORT:
			if(typefd[ft]) {
				/* special case can store mem from Freg */
				regalloc(&nod, f, Z);
				ginsf(AFMOVFH, f, Z, &nod, t);
				regfree(&nod);
				return;
			}
			a = AMOVH;
			break;
		case TUSHORT:
			a = AMOVH;
			break;
		case TFLOAT:
		case TDOUBLE:
		case TVLONG:
			a = AFMOVF;
			break;
		}
		/*
		 * test for move of zero
		 */
		if(!typefd[ft] && vconst(f) == 0) {
			gins(a, f, &adr);
			if(aisreg)
				regfree(&adr);
			return;
		}
		if(ft == tt)
			regalloc(&nod, t, f);
		else
			regalloc(&nod, t, Z);
		gmove(f, &nod);
		if(typefd[tt])
			ginsf(a, &nod, Z, Z, &adr);
		else
			gins(a, &nod, &adr);
		regfree(&nod);
		if(aisreg)
			regfree(&adr);
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
		case TFLOAT:
			a = AFMOVF;
			break;
		case TLONG:
		case TULONG:
		case TIND:
		case TSHORT:
		case TUSHORT:
		case TCHAR:
		case TUCHAR:
			regalloc(&nod, f, Z);	/* should be type float */
			ginsf(AFMOVFW, f, Z, &nod, nodrat);
			gins(AMOVW, nodrat, t);
			if(nrathole < SZ_LONG)
				nrathole = SZ_LONG;
			regfree(&nod);
			gmove(t, t);
			return;
		}
		break;
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
			gins(AMOVW, f, nodrat);
			ginsf(AFMOVWF, nodrat, Z, t, Z);
			if(nrathole < SZ_LONG)
				nrathole = SZ_LONG;
			if(ft == TULONG) {
				regalloc(&nod, t, Z);
				gins(ACMP, f, nodconst(0));
				gins(ABRA, Z, Z);
				p->cc = CCGE;
				p1 = p;
				ginsf(AFMOVF, nodfconst(4294967296.), Z, &nod, Z);
				ginsf(AFADD, &nod, t, t, Z);
				patch(p1, pc);
				regfree(&nod);
			}
			return;
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
	if(a == AMOVW || a == AFMOVF)
	if(samaddr(f, t))
		return;
	if(a == AFMOVF)
		ginsf(a, f, Z, t, Z);
	else
		gins(a, f, t);
}

/*
 * generate instruction a f,t
 */
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

/*
 * generate floating instruction a f,f1,r,t
 */
void
ginsf(int a, Node *f, Node *f1, Node *r, Node *t)
{

	nextpc();
	p->as = a;
	p->isf = 1;
	if(f != Z)
		naddr(f, &p->from);
	if(f1 != Z)
		naddr(f1, &p->from1);
	if(r != Z)
		raddr(r, p);
	if(t != Z)
		naddr(t, &p->to);
	if(debug['g'])
		print("%P\n", p);
}

/*
 * generate the instruction for operation o
 * with f1 and f2 as sources and store in t
 */
void
gopcode(int o, Node *f1, Node *f2, Node *t)
{
	Node nod;
	int a, cc, et;

	et = TLONG;
	if(f1 != Z && f1->type != T)
		et = f1->type->etype;
	a = AGOK;
	cc = 0;
	switch(o) {
	case OAS:
		gmove(f1, t);
		return;

	case OASADD:
	case OADD:
		a = AADD;
		if(et == TFLOAT || et == TDOUBLE || et == TVLONG)
			a = AFADD;
		break;

	case OASSUB:
	case OSUB:
		a = ASUB;
		if(et == TFLOAT || et == TDOUBLE || et == TVLONG){
			a = AFSUB;
			if(f2 && vconst(f2) == 0){
				ginsf(AFMOVFN, f1, Z, t, Z);
				return;
			}
		}
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
		a = ACALL;
		break;

	case OASLMUL:
	case OLMUL:
	case OASMUL:
	case OMUL:
		a = AMUL;
		if(et == TFLOAT || et == TDOUBLE || et == TVLONG)
			a = AFMUL;
		break;

	case OASDIV:
	case ODIV:
		a = ADIV;
		if(et == TFLOAT || et == TDOUBLE || et == TVLONG)
			a = AFDIV;
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
		cc = CCEQ;
		if(typefd[et])
			cc = CCFEQ;
		goto cmp;

	case ONE:
		cc = CCNE;
		if(typefd[et])
			cc = CCFNE;
		goto cmp;

	case OLT:
		cc = CCLT;
		if(typefd[et])
			cc = CCFLT;
		goto cmp;

	case OLE:
		cc = CCLE;
		if(typefd[et])
			cc = CCFLE;
		goto cmp;

	case OGE:
		cc = CCGE;
		if(typefd[et])
			cc = CCFGE;
		goto cmp;

	case OGT:
		cc = CCGT;
		if(typefd[et])
			cc = CCFGT;
		goto cmp;

	case OLO:
		cc = CCCS;
		goto cmp;

	case OLS:
		cc = CCLS;
		goto cmp;

	case OHS:
		cc = CCCC;
		goto cmp;

	case OHI:
		cc = CCHI;
		goto cmp;

	cmp:
		a = ABRA;
		if(f1 == Z || t == Z || f2 != Z)
			diag(Z, "bad cmp in gopcode %O", o);
		if(et == TFLOAT || et == TDOUBLE || et == TVLONG){
			regalloc(&nod, f1, Z);
			ginsf(AFSUB, t, f1, &nod, Z);
			regfree(&nod);
		}else
			gins(ACMP, f1, t);
		if(debug['g'])
			print("%P\n", p);
		f1 = Z;
		f2 = Z;
		t = Z;
		et = TLONG;
		break;
	}
	if(a == AGOK)
		diag(Z, "bad in gopcode %O", o);
	nextpc();
	p->as = a;
	p->cc = cc;
	if(f1 != Z)
		naddr(f1, &p->from);
	if(et == TFLOAT || et == TDOUBLE || et == TVLONG){
		p->isf = 1;
		if(f2 != Z)
			naddr(f2, &p->from1);
		else if(t != Z)
			naddr(t, &p->from1);
		if(t != Z)
			raddr(t, p);
	}else{
		if(f2 != Z)
			raddr(f2, p);
		if(t != Z)
			naddr(t, &p->to);
	}
	if(debug['g'])
		print("%P\n", p);
}

/*
 * do f and t represtent the same address?
 */
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

/*
 * generate a branch instruction;
 * patch the destination later
 */
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

/*
 * make pc the desitionation of branch instruction op
 */
void
patch(Prog *op, long pc)
{

	op->to.offset = pc;
	op->to.type = D_BRANCH;
}

/*
 * generate a pseudo instruction
 */
void
gpseudo(int a, Sym *s, Node *n)
{

	nextpc();
	p->as = a;
	p->from.type = D_NAME;
	p->from.sym = s;
	p->from.name = D_EXTERN;
	if(s->class == CSTATIC)
		p->from.name = D_STATIC;
	naddr(n, &p->to);
	if(a == ADATA || a == AGLOBL)
		pc--;
}

/*
 * value comparable in branch?
 * used in swt.c
 */
int
sval(long v)
{

	if(v >= -(1<<16) && v < (1<<16))
		return 1;
	return 0;
}

/*
 * is n a constant usable as an offset from a register
 * always false; adresses like this are always synthesized
 */
int
sconst(Node *n)
{
	long v;

	if(n->op == OCONST && !typefd[n->type->etype]) {
		v = n->vconst;
		if(v >= -32768 && v < 32768)
			return 1;
	}
	return 0;
}

/*
 * return the next external register for t,
 * or 0 if none are available
 */
long
exreg(Type *t)
{
	long o;


	if(typechlp[t->etype]) {
		if(exregoffset <= REGMAX)
			return 0;
		o = exregoffset;
		exregoffset--;
		return o;
	}
	return 0;
}

schar	ewidth[XTYPE] =
{
	-1,				/* TXXX */
	SZ_CHAR,	SZ_CHAR,	/* TCHAR	TUCHAR */
	SZ_SHORT,	SZ_SHORT,	/* TSHORT	TUSHORT */
	SZ_LONG,	SZ_LONG,	/* TLONG	TULONG */
	SZ_VLONG,	SZ_VLONG,	/* TVLONG	TUVLONG */
	SZ_FLOAT,	SZ_DOUBLE,	/* TFLOAT	TDOUBLE */
	SZ_IND,		0,		/* TIND		TFUNC */
	-1,		0,		/* TARRAY	TVOID */
	-1,		-1,		/* TSTRUCT	TUNION */
	-1				/* TENUM */
};
long	ncast[XTYPE] =
{
	/* TXXX */	0,
	/* TCHAR */	BCHAR|BUCHAR,
	/* TUCHAR */	BCHAR|BUCHAR,
	/* TSHORT */	BSHORT|BUSHORT,
	/* TUSHORT */	BSHORT|BUSHORT,
	/* TLONG */	BLONG|BULONG|BIND,
	/* TULONG */	BLONG|BULONG|BIND,
	/* TVLONG */	BVLONG|BUVLONG,
	/* TUVLONG */	BVLONG|BUVLONG,
	/* TFLOAT */	BFLOAT|BDOUBLE,
	/* TDOUBLE */	BDOUBLE|BFLOAT,
	/* TIND */	BLONG|BULONG|BIND,
	/* TFUNC */	0,
	/* TARRAY */	0,
	/* TVOID */	0,
	/* TSTRUCT */	BSTRUCT,
	/* TUNION */	BUNION,
	/* TENUM */	0,
};

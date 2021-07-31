#include "gc.h"

void
ginit(void)
{
	Type *t;

	thechar = 'z';
	thestring = "hobbit";
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
	zprog.from.type = D_NONE;
	zprog.from.width = W_NONE;
	zprog.to = zprog.from;

	regnode.op = OREGISTER;
	regnode.class = CEXREG;
	regnode.complex = 0;
	regnode.addable = 11;

	constnode.op = OCONST;
	constnode.type = tint;
	constnode.class = CXXX;
	constnode.complex = 0;
	constnode.addable = 20;

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
}

void
gclean(void)
{
	int i;
	Sym *s;

	while(mnstring)
		outstring("", 1);
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

void
gargs(Node *n, Node *nn)
{
	long regs, regc;
	Node nod;

	regs = cursafe;
	regc = curreg;
	garg1(n, nn, &nod, 0);	/* compile fns to temps */
	cursafe = regs;
	curreg = regc;
	garg1(n, nn, &nod, 1);	/* compile normal args and temps */
}

void
garg1(Node *n, Node *nn, Node *nnn, int f)
{
	long o;

	if(n == Z)
		return;
	if(n->op == OLIST) {
		garg1(n->left, nn, nnn, f);
		garg1(n->right, nn, nnn, f);
		return;
	}
	regalloc(nnn, n);
	if(typesu[n->type->etype]) {
		o = n->type->width;
		o += round(o, SZ_LONG);
		curreg += o;
		if(curreg > maxreg)
			maxreg = curreg;
	}
	if(n->complex >= FNX) {
		regsalloc(nn, n);
		if(f) {
			cgen(nn, nnn);
			if(!typesu[n->type->etype])
				curreg += n->type->width;
		} else
			cgen(n, nn);
		return;
	}
	if(f) {
		cgen(n, nnn);
		if(!typesu[n->type->etype])
			curreg += n->type->width;
	}
}

Node*
nodconst(long v)
{
	constnode.offset = v;
	return &constnode;
}

Node*
nodfconst(double d)
{
	fconstnode.ud = d;
	return &fconstnode;
}

void
regalloc(Node *n, Node *nn)
{
	long creg;

	*n = regnode;
	n->offset = curreg;
	creg = curreg + nn->type->width;
	if(creg > maxreg)
		maxreg = creg;
	n->type = nn->type;
	n->lineno = nn->lineno;
}

void
regialloc(Node *n, Node *nn)
{
	long creg;

	*n = regnode;
	n->offset = curreg;
	creg = curreg + SZ_IND;
	if(creg > maxreg)
		maxreg = creg;
	n->type = types[TIND];
	n->lineno = nn->lineno;
}

void
regsalloc(Node *n, Node *nn)
{
	long o;

	o = nn->type->width;
	o += round(o, tint->width);
	cursafe += o;
	if(cursafe > maxsafe)
		maxsafe = cursafe;
	*n = *nodsafe;
	n->offset = -(stkoff + cursafe);
	n->type = nn->type;
	n->lineno = nn->lineno;
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
regret(Node *n, Node *nn)
{

	regalloc(n, nn);
	n->offset = SZ_LONG;
}

void
gprep(Node *nod, Node *n)
{
	Node *l;
	long v;

	if(n->addable > INDEXED) {
		*nod = *n;
		return;
	}
	switch(n->op) {
	case OCAST:
		l = n->left;
		if(nilcast(l->type, n->type)) {
			gprep(nod, l);
			return;
		}

	default:
		regalloc(nod, n);
		cgen(n, nod);
		v = n->type->width;
		if(v > SZ_IND)
			curreg += v-SZ_IND;
		break;

	case OPREINC:
	case OPREDEC:
		l = n->left;
		regialloc(nod, l);
		lcgen(n->left, nod);
		regind(nod, l);
		v = 1;
		if(l->type->etype == TIND)
			v = l->type->link->width;
		gopcode(n->op, nodconst(v), nod);
		break;

	case OIND:
		regialloc(nod, n);
		lcgen(n, nod);
		regind(nod, n);
		break;
	}
	curreg += SZ_IND;
	nod->lineno = n->lineno;
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

	case OPREINC:
	case OPREDEC:
	case OPOSTINC:
	case OPOSTDEC:

	case OAS:
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
		naddr(n->left, a);
		break;

	case OREGISTER:
		a->type = D_REG;
		a->sym = S;
		a->offset = n->offset;
		break;

	case OINDREG:
		a->type = D_INDIR | D_REG;
		a->sym = S;
		a->offset = n->offset;
		break;

	case ONAME:
		a->type = D_STATIC;
		a->sym = n->sym;
		a->offset = n->offset;
		if(n->class == CSTATIC)
			break;
		if(n->class == CEXTERN || n->class == CGLOBL) {
			a->type = D_EXTERN;
			break;
		}
		if(n->class == CAUTO) {
			a->type = D_AUTO;
			break;
		}
		if(n->class == CPARAM) {
			a->type = D_PARAM;
			break;
		}
		goto bad;

	case OCONST:
		if(!n || !n->type) {
			diag(n, "no type on const");
			break;
		}
		a->width = W_NONE;
		if(typefdv[n->type->etype]) {
			a->type = D_FCONST;
			a->dval = n->ud;
			break;
		}
		a->type = D_CONST;
		a->offset = n->offset;
		break;

	case OADDR:
		naddr(n->left, a);
		a->width = a->type - D_NONE;
		a->type = D_ADDR;
		break;

	case OIND:
		v = a->width;
		naddr(n->left, a);
		if(a->type == D_ADDR) {
			a->type = a->width + D_NONE;
			a->width = v;
			break;
		}
		if(a->type == D_CONST)
			a->width = v;
		a->type |= D_INDIR;
		break;

	case OADD:
		if(n->left->addable == 20) {
			naddr(n->left, a);
			v = a->offset;
			naddr(n->right, a);
			a->offset += v;
			break;
		}
		if(n->right->addable == 20) {
			naddr(n->right, a);
			v = a->offset;
			naddr(n->left, a);
			a->offset += v;
			break;
		}
		goto bad;
	}
}

void
gopcode(int o, Node *f, Node *t)
{
	Node nod;
	int fp, a;

	fp = 0;
	if(f != Z)
	if(typefdv[f->type->etype])
		fp = 1;
	if(t != Z)
	if(typefdv[t->type->etype])
		fp = 1;

	a = AGOK;
	switch(o) {
	case OAS:
		a = AFMOV;
		if(!fp) {
			a = AMOV;
			break;
		}
		if(f && !typefdv[f->type->etype]) {
			if(twidth(f->type) != W_W)
				goto trulong;
			if(f->op == OCONST)
				goto trulong;
		}
		if(t && !typefdv[t->type->etype])
			if(twidth(t->type) != W_W)
				goto trulong;
		break;
	trulong:
		regialloc(&nod, f);
		nod.type = types[TLONG];
		gopcode(OAS, f, &nod);
		gopcode(OAS, &nod, t);
		return;
	case OPREINC:
	case OPOSTINC:
	case OASADD:
	case OADD:
		a = AADD;
		if(fp) {
			a = AFADD;
			if(typec[t->type->etype] || typeh[t->type->etype])
				diag(f, "illegal opcode");
		}
		break;
	case OPREDEC:
	case OPOSTDEC:
	case OASSUB:
	case OSUB:
		a = ASUB;
		if(fp)
			a = AFSUB;
		break;
	case OASMUL:
	case OMUL:
	case OASLMUL:
	case OLMUL:
		a = AMUL;
		if(fp)
			a = AFMUL;
		break;
	case OASDIV:
	case ODIV:
		a = ADIV;
		if(fp)
			a = AFDIV;
		break;
	case OASLDIV:
	case OLDIV:
		a = AUDIV;
		break;
	case OASMOD:
	case OMOD:
		a = AREM;
		break;
	case OASLMOD:
	case OLMOD:
		a = AUREM;
		break;
	case OASASHR:
	case OASHR:
		a = ASHR;
		break;
	case OASASHL:
	case OASHL:
		a = ASHL;
		break;
	case OASLSHR:
	case OLSHR:
		a = AUSHR;
		break;
	case OADDR:
		a = AMOVA;
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
	case OEQ:
	case ONE:
		a = ACMPEQ;
		if(fp)
			a = AFCMPEQ;
		break;
	case OLT:
	case OGT:
	case OLE:
	case OGE:
		a = ACMPGT;
		if(fp)
			a = AFCMPGT;
		break;
	case OLO:
	case OHI:
	case OLS:
	case OHS:
		a = ACMPHI;
		break;
	case OFUNC:
		a = ACALL;
		break;
	}
	if(a == AGOK)
		diag(t, "bad in gopcode %O", o);
	if(a == AMOV || a == AFMOV)
		if(samaddr(f, t))
			return;
	nextpc();
	p->as = a;
	if(f != Z) {
		if(a == AMOVA)
			p->from.width = W_NONE;
		else
			p->from.width = twidth(f->type);
		naddr(f, &p->from);
	}
	if(t != Z) {
		p->to.width = twidth(t->type);
		naddr(t, &p->to);
	}
	if(debug['g'])
		print("%ld %P\n", pc-1, p);
}

void
gopcode3(int o, Node *f, Node *t)
{
	int a;

	a = AGOK;
	switch(o) {
	case OADD:	a = AADD3; break;
	case OSUB:	a = ASUB3; break;
	case OLMUL:
	case OMUL:	a = AMUL3; break;
	case ODIV:	a = ADIV3; break;
	case OASHL:	a = ASHL3; break;
	case OASHR:	a = ASHR3; break;
	case OLSHR:	a = AUSHR3; break;
	case OMOD:	a = AREM3; break;
	case OOR:	a = AOR3; break;
	case OAND:	a = AAND3; break;
	case OXOR:	a = AXOR3; break;
	}
	if(a == AGOK)
		diag(Z, "bad in gopcode3 %O", o);
	nextpc();
	p->as = a;
	p->from.width = twidth(f->type);
	naddr(f, &p->from);
	p->to.width = twidth(t->type);
	naddr(t, &p->to);
	if(debug['g'])
		print("%ld %P\n", pc-1, p);
}

int
samaddr(Node *f, Node *t)
{

	if(f->op != t->op)
		goto no;
	if(twidth(f->type) != twidth(t->type))
		goto no;
	switch(f->op) {

	case OREGISTER:
		if(f->offset != t->offset)
			break;
		return 1;
	}
no:
	return 0;
}

int
acc(Node *n)
{

	if(n->op == OREGISTER)
	if(n->offset == SZ_LONG)
	if(typelp[n->type->etype])
		return 1;
	return 0;
}

void
gbranch(int o)
{
	int a;

	a = AGOK;
	switch(o) {
	case ORETURN: a = ARETURN; break;
	case OGOTO: a = AJMP; break;
	case OGT:
	case OHI:
	case OEQ: a = AJMPT; break;
	case OLE:
	case OLS:
	case ONE: a = AJMPF; break;
	}
	if(a == AGOK)
		diag(Z, "bad in gbranch %O",  o);
	nextpc();
	p->as = a;
	p->from.type = D_NONE;
	p->to.type = D_NONE;
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
	p->from.sym = s;
	p->from.type = D_EXTERN;
	if(s->class == CSTATIC)
		p->from.type = D_STATIC;
	naddr(n, &p->to);
	if(a == ADATA || a == AGLOBL)
		pc--;
}

int
twidth(Type *t)
{
	int et;

	if(t == T)
		return W_NONE;
	et = t->etype;
	switch(et) {

	case TCHAR:
		return W_B;

	case TUCHAR:
		return W_UB;

	case TSHORT:
		return W_H;

	case TUSHORT:
		return W_UH;

	case TLONG:
	case TULONG:
	case TIND:
		return W_W;

	case TFLOAT:
		return W_F;

	case TDOUBLE:
		return W_D;

	case TFUNC:
	case TSTRUCT:
		return W_NONE;
	}
	diag(Z, "bad in twidth %d", t->etype);
	return W_GOK;
}

long
exreg(Type *t)
{

	USED(t);
	return 0;
}

schar	ewidth[XTYPE] =
{
	-1,				/* TXXX */
	SZ_CHAR,	SZ_CHAR,	/* TCHAR	TUCHAR */
	SZ_SHORT,	SZ_SHORT,	/* TSHORT	TUSHORT */
	SZ_LONG,	SZ_LONG,	/* TLONG	TULONG */
	SZ_VLONG,			/* TVLONG */
	SZ_FLOAT,	SZ_DOUBLE,	/* TFLOAT	TDOUBLE */
	SZ_IND,		0,		/* TIND		TFUNC */
	-1,		0,		/* TARRAY	TVOID */
	-1,		-1,		/* TSTRUCT	TUNION */
	-1				/* TENUM */
};

/*
 * conversions that generate no code
 */
long	ncast[XTYPE] =
{
	/* TXXX */	0,
	/* TCHAR */	BCHAR|BUCHAR,
	/* TUCHAR */	BCHAR|BUCHAR,
	/* TSHORT */	BSHORT|BUSHORT,
	/* TUSHORT */	BSHORT|BUSHORT,
	/* TLONG */	BLONG|BULONG|BIND,
	/* TULONG */	BLONG|BULONG|BIND,
	/* TVLONG */	BVLONG|BDOUBLE,
	/* TFLOAT */	BFLOAT,
	/* TDOUBLE */	BDOUBLE,
	/* TIND */	BLONG|BULONG|BIND,
	/* TFUNC */	0,
	/* TARRAY */	0,
	/* TVOID */	0,
	/* TSTRUCT */	BSTRUCT,
	/* TUNION */	BUNION,
	/* TENUM */	0,
};

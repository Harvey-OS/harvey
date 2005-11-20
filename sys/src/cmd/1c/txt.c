#include "gc.h"

void
tindex(Type *tf, Type *tt)
{
	int i, j;

	j = 0;
	if(tt != T) {
		j = tt->etype;
		if(j >= NTYPE)
			j = 0;
	}
	i = 0;
	if(tf != T) {
		i = tf->etype;
		if(i >= NTYPE)
			if(typesu[i])
				i = j;
			else
				i = 0;
	}
	txtp = &txt[i][j];
}

void
ginit(void)
{
	int i, j, si, sj;

	thestring = "68000";
	thechar = '1';
	exregoffset = 7;
	exaregoffset = 5;
	exfregoffset = 7;
	listinit();
	for(i=0; i<NREG; i++) {
		regused[i] = 0;
		fregused[i] = 0;
		aregused[i] = 0;
	}
	regaddr(D_A0+6);
	regaddr(D_A0+7);
	for(i=0; i<sizeof(regbase); i++)
		regbase[i] = D_NONE;
	for(i=0; i<NREG; i++) {
		regbase[D_R0+i] = D_R0+i;
		regbase[D_A0+i] = D_A0+i;
		regbase[D_F0+i] = D_F0+i;
	}
	regbase[D_TOS] = D_TOS;

	for(i=0; i<NTYPE; i++)
	for(j=0; j<NTYPE; j++) {
		txtp = &txt[i][j];
		txtp->movas = AGOK;
		txtp->preclr = 0;
		txtp->postext = AGOK;
		if(!(typechlp[i] && typechlp[j]))
			continue;
		si = types[i]->width;
		sj = types[j]->width;
		if(sj < si)
			txtp->preclr = -1;
		if(sj > si) {
			if(typeu[i]) {
				txtp->preclr = 1;
			} else {
				if(sj == 2)
					txtp->postext = AEXTBW;
				else
				if(sj == 4)
					if(si == 1)
						txtp->postext = AEXTBL;
					else
						txtp->postext = AEXTWL;
			}
			sj = si;
		}
		if(sj == 1)
			txtp->movas = AMOVB;
		if(sj == 2)
			txtp->movas = AMOVW;
		if(sj == 4)
			txtp->movas = AMOVL;
	}

	for(i=0; i<ALLOP; i++)
		for(j=0; j<NTYPE; j++)
			opxt[i][j] = AGOK;
	oinit(OFUNC, ABSR, ATRAP, AGOK, AGOK, AGOK);

	oinit(OAS, AMOVB, AMOVW, AMOVL, AFMOVEF, AFMOVED);
	oinit(OFAS, AFMOVEB, AFMOVEW, AFMOVEL, AFMOVEF, AFMOVED);
	oinit(OADDR, AGOK, APEA, ALEA, AGOK, AGOK);
	oinit(OPREINC, AADDB, AADDW, AADDL, AFADDF, AFADDD);
	oinit(OPOSTINC, AADDB, AADDW, AADDL, AFADDF, AFADDD);
	oinit(OPREDEC, ASUBB, ASUBW, ASUBL, AFSUBF, AFSUBD);
	oinit(OPOSTDEC, ASUBB, ASUBW, ASUBL, AFSUBF, AFSUBD);
	oinit(OADD, AADDB, AADDW, AADDL, AFADDF, AFADDD);
	oinit(OASADD, AADDB, AADDW, AADDL, AFADDF, AFADDD);
	oinit(OSUB, ASUBB, ASUBW, ASUBL, AFSUBF, AFSUBD);
	oinit(OASSUB, ASUBB, ASUBW, ASUBL, AFSUBF, AFSUBD);
	oinit(OMUL, AGOK, AMULSW, AMULSL, AFMULF, AFMULD);
	oinit(OLMUL, AGOK, AMULSW, AMULSL, AFMULF, AFMULD);
	oinit(OASMUL, AGOK, AMULSW, AMULSL, AFMULF, AFMULD);
	oinit(OASLMUL, AGOK, AMULSW, AMULSL, AFMULF, AFMULD);
	oinit(ODIV, AGOK, ADIVSW, ADIVSL, AFDIVF, AFDIVD);
	oinit(OLDIV, AGOK, ADIVUW, ADIVUL, AFDIVF, AFDIVD);
	oinit(OASDIV, AGOK, ADIVSW, ADIVSL, AFDIVF, AFDIVD);
	oinit(OASLDIV, AGOK, ADIVUW, ADIVUL, AFDIVF, AFDIVD);
	oinit(OMOD, AGOK, ADIVSW, ADIVSL, AFMODF, AFMODD);
	oinit(OASMOD, AGOK, ADIVSW, ADIVSL, AGOK, AGOK);
	oinit(OLMOD, AGOK, ADIVUW, ADIVUL, AGOK, AGOK);
	oinit(OASLMOD, AGOK, ADIVUW, ADIVUL, AGOK, AGOK);
	oinit(OAND, AANDB, AANDW, AANDL, AGOK, AGOK);
	oinit(OASAND, AANDB, AANDW, AANDL, AGOK, AGOK);
	oinit(OOR, AORB, AORW, AORL, AGOK, AGOK);
	oinit(OASOR, AORB, AORW, AORL, AGOK, AGOK);
	oinit(OXOR, AEORB, AEORW, AEORL, AGOK, AGOK);
	oinit(OASXOR, AEORB, AEORW, AEORL, AGOK, AGOK);
	oinit(ONEG, ANEGB, ANEGW, ANEGL, AFNEGF, AFNEGD);
	oinit(OCOM, ANOTB, ANOTW, ANOTL, AGOK, AGOK);
	oinit(OTST, ATSTB, ATSTW, ATSTL, AFTSTF, AFTSTD);
	oinit(OEQ, ACMPB, ACMPW, ACMPL, AFCMPF, AFCMPD);
	oinit(ONE, ACMPB, ACMPW, ACMPL, AFCMPF, AFCMPD);
	oinit(OGE, ACMPB, ACMPW, ACMPL, AFCMPF, AFCMPD);
	oinit(OGT, ACMPB, ACMPW, ACMPL, AFCMPF, AFCMPD);
	oinit(OLT, ACMPB, ACMPW, ACMPL, AFCMPF, AFCMPD);
	oinit(OLE, ACMPB, ACMPW, ACMPL, AFCMPF, AFCMPD);
	oinit(OLS, ACMPB, ACMPW, ACMPL, AFCMPF, AFCMPD);
	oinit(OLO, ACMPB, ACMPW, ACMPL, AFCMPF, AFCMPD);
	oinit(OHS, ACMPB, ACMPW, ACMPL, AFCMPF, AFCMPD);
	oinit(OHI, ACMPB, ACMPW, ACMPL, AFCMPF, AFCMPD);
	oinit(OASHR, AASRB, AASRW, AASRL, AGOK, AGOK);
	oinit(OASASHR, AASRB, AASRW, AASRL, AGOK, AGOK);
	oinit(OLSHR, ALSRB, ALSRW, ALSRL, AGOK, AGOK);
	oinit(OASLSHR, ALSRB, ALSRW, ALSRL, AGOK, AGOK);
	oinit(OASHL, AASLB, AASLW, AASLL, AGOK, AGOK);
	oinit(OASASHL, AASLB, AASLW, AASLL, AGOK, AGOK);
	oinit(OBIT, ABFEXTU, AGOK, AGOK, AGOK, AGOK);

	nstring = 0;
	mnstring = 0;
	nrathole = 0;
	nstatic = 0;
	pc = 0;
	breakpc = -1;
	continpc = -1;
	cases = C;
	firstp = P;
	lastp = P;
	tfield = types[TLONG];

	zprog.link = P;
	zprog.as = AGOK;
	zprog.from.type = D_NONE;
	zprog.to = zprog.from;

	nodret = new(ONAME, Z, Z);
	nodret->sym = slookup(".ret");
	nodret->type = types[TIND];
	nodret->etype = types[TIND]->etype;
	nodret->class = CPARAM;
	nodret = new(OIND, nodret, Z);
	complex(nodret);

	symrathole = slookup(".rathole");
	symrathole->class = CGLOBL;
	symrathole->type = typ(TARRAY, types[TCHAR]);
	nodrat = new(ONAME, Z, Z);
	nodrat->sym = symrathole;
	nodrat->type = types[TIND];
	nodrat->etype = TVOID;
	nodrat->class = CGLOBL;
	complex(nodrat);
	nodrat->type = symrathole->type;

	com64init();

	symstatic = slookup(".static");
	symstatic->class = CSTATIC;
	symstatic->type = typ(TARRAY, types[TLONG]);
}

void
gclean(void)
{
	int i;
	Sym *s;

	regfree(D_A0+6);
	regfree(D_A0+7);
	for(i=0; i<NREG; i++) {
		if(regused[i])
			diag(Z, "missing R%d", i);
		if(aregused[i])
			diag(Z, "missing A%d", i);
		if(fregused[i])
			diag(Z, "missing F%d", i);
	}

	while(mnstring)
		outstring("", 1L);
	symstring->type->width = nstring;
	symstatic->type->width = nstatic;
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
		gpseudo(AGLOBL, s, D_CONST, s->type->width);
		pc--;
	}
	nextpc();
	p->as = AEND;
	outcode();
}

void
oinit(int o, int ab, int aw, int al, int af, int ad)
{
	int i;

	i = o;
	if(i >= ALLOP) {
		diag(Z, "op(%d) >= ALLOP(%d)", i, ALLOP);
		errorexit();
	}
	opxt[i][TCHAR] = ab;
	opxt[i][TUCHAR] = ab;
	opxt[i][TSHORT] = aw;
	opxt[i][TUSHORT] = aw;
	opxt[i][TINT] = al;
	opxt[i][TUINT] = al;
	opxt[i][TLONG] = al;
	opxt[i][TULONG] = al;
	opxt[i][TIND] = al;
	opxt[i][TFLOAT] = af;
	opxt[i][TDOUBLE] = ad;
}

Prog*
prg(void)
{
	Prog *p;

	p = alloc(sizeof(*p));
	*p = zprog;
	return p;
}

void
nextpc(void)
{

	p = prg();
	pc++;
	p->lineno = nearln;
	if(firstp == P) {
		firstp = p;
		lastp = p;
		return;
	}
	lastp->link = p;
	lastp = p;
}

void
gargs(Node *n)
{
	long s;

loop:
	if(n == Z)
		return;
	if(n->op == OLIST) {
		gargs(n->right);
		n = n->left;
		goto loop;
	}
	s = argoff;
	cgen(n, D_TOS, n);
	argoff = s + n->type->width;
}

void
naddr(Node *n, Adr *a, int x)
{
	Node *l;
	long v;

	switch(n->op) {
	default:
	bad:
		diag(n, "bad in naddr: %O", n->op);
		break;

	case OADDR:
	case OIND:
		naddr(n->left, a, x);
		goto noadd;

	case OREGISTER:
		a->sym = S;
		a->type = n->reg;
		a->offset = n->xoffset;
		a->displace = 0;
		break;

	case ONAME:
		a->etype = n->etype;
		a->displace = 0;
		a->sym = n->sym;
		a->offset = n->xoffset;
		a->type = D_STATIC;
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
		a->displace = 0;
		if(typefd[n->type->etype]) {
			a->type = D_FCONST;
			a->dval = n->fconst;
			break;
		}
		a->type = D_CONST;
		a->offset = n->vconst;
		break;

	case OADD:
		l = n->left;
		if(l->addable == 20) {
			v = l->vconst;
			naddr(n->right, a, x);
			goto add;
		}
		l = n->right;
		if(l->addable == 20) {
			v = l->vconst;
			naddr(n->left, a, x);
			goto add;
		}
		goto bad;

	noadd:
		v = 0;
	add:
		switch(n->addable) {
		default:
			goto bad;
		case 2:
			a->displace += v;
			break;
		case 21:
			a->type &= D_MASK;
			a->type |= I_INDIR;
			break;
		case 1:
		case 12:
			a->offset += v;
			a->type &= D_MASK;
			a->type |= I_ADDR;
			break;
		case 10:
		case 11:
		case 20:
			a->type &= D_MASK;
			a->type |= I_DIR;
			break;
		}
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
	case OASASHR:
	case OASASHL:
	case OASAND:
		naddr(n->left, a, x);
		break;
	}
}

int
regalloc(Type *t, int g)
{

	if(t == T)
		return D_NONE;
	g &= D_MASK;
	if(typefd[t->etype]) {
		if(g >= D_F0 && g < D_F0+NREG) {
			fregused[g-D_F0]++;
			return g;
		}
		for(g=0; g<NREG; g++)
			if(fregused[g] == 0) {
				fregused[g]++;
				return g + D_F0;
			}
	} else {
		if(g >= D_R0 && g < D_R0+NREG) {
			regused[g-D_R0]++;
			return g;
		}
		for(g=0; g<NREG; g++)
			if(regused[g] == 0) {
				regused[g]++;
				return g + D_R0;
			}
	}
	diag(Z, "out of registers");
	return D_TOS;
}

int
regaddr(int g)
{

	if(g >= D_A0 && g < D_A0+NREG) {
		aregused[g-D_A0]++;
		return g;
	}
	for(g=0; g<NREG; g++)
		if(aregused[g] == 0) {
			aregused[g]++;
			return g + D_A0;
		}
	diag(Z, "out of addr registers");
	return D_TOS;
}

int
regpair(int g)
{

	if(g >= D_R0+1 && g < D_R0+NREG)
		if(!regused[g-D_R0-1]) {
			regused[g-D_R0-1]++;
			regused[g-D_R0]++;
			return g-1;
		}
	if(g >= D_R0 && g < D_R0+NREG-1)
		if(!regused[g-D_R0+1]) {
			regused[g-D_R0+1]++;
			regused[g-D_R0]++;
			return g;
		}
	for(g = 0; g < NREG-1; g++)
		if(!regused[g])
		if(!regused[g+1]) {
			regused[g]++;
			regused[g+1]++;
			return g + D_R0;
		}
	diag(Z, "out of register pairs");
	return D_TOS;
}

int
regret(Type *t)
{

	if(t == T)
		return D_NONE;
	if(typefd[t->etype])
		return D_F0;
	return D_R0;
}

void
regfree(int g)
{

	g &= D_MASK;
	if(g == D_TOS || g == D_TREE || g == D_NONE)
		return;
	if(g >= D_R0 && g < D_R0+NREG) {
		regused[g-D_R0]--;
		return;
	}
	if(g >= D_A0 && g < D_A0+NREG) {
		aregused[g-D_A0]--;
		return;
	}
	if(g >= D_F0 && g < D_F0+NREG) {
		fregused[g-D_F0]--;
		return;
	}
	diag(Z, "bad in regfree: %d", g);
}

void
gmove(Type *tf, Type *tt, int gf, Node *f, int gt, Node *t)
{
	int g, a, b;
	Prog *p1;

	tindex(tf, tt);
	if(txtp->preclr) {
		if(gf >= D_R0 && gf < D_R0+NREG)
		if(txtp->preclr < 0) {
			gmove(tt, tt, gf, f, gt, t);
			return;
		}
		g = regalloc(types[TLONG], gt);
		if(g == gf) {
			g = regalloc(types[TLONG], D_NONE);
			regfree(gf);
		}
		if(txtp->preclr > 0)
			gopcode(OAS, types[TLONG], D_CONST, nodconst(0), g, Z);
		gopcode(OAS, tf, gf, f, g, Z);
		if(g != gt)
			gopcode(OAS, tt, g, Z, gt, t);
		regfree(g);
		return;
	}
	a = txtp->postext;
	if(a != AGOK) {
		if(gf >= D_R0 && gf < D_R0+NREG)
			g = regalloc(types[TLONG], gf);
		else
			g = regalloc(types[TLONG], gt);
		if(g != gf)
			gopcode(OAS, tf, gf, f, g, Z);
		nextpc();
		p->as = a;
		p->to.type = g;
		if(debug['g'])
			print("%P\n", p);
		if(g != gt)
			gopcode(OAS, tt, g, Z, gt, t);
		regfree(g);
		return;
	}
	if((regbase[gf] != D_NONE && regbase[gf] == regbase[gt]) ||
	   (gf == D_TREE && gt == D_TREE && f == t))
		return;
	if(typefd[tf->etype] || typefd[tt->etype]) {
		if(typeu[tf->etype] && typefd[tt->etype]) {	/* unsign->float */
			a = regalloc(types[TLONG], D_NONE);
			gmove(tf, types[TLONG], gf, f, a, t);
			if(tf->etype == TULONG) {
				b = regalloc(types[TDOUBLE], D_NONE);
				gmove(types[TLONG], tt, a, t, b, t);
				gopcode(OTST, types[TLONG], D_NONE, Z, a, t);
				gbranch(OGE);
				p1 = p;
				gopcode(OASADD, types[TDOUBLE],
					D_CONST, nodconst(100), b, t);
				p->from.dval = 4294967296.;
				patch(p1, pc);
				gmove(types[TDOUBLE], tt, b, t, gt, t);
				regfree(b);
			} else
				gmove(types[TLONG], tt, a, t, gt, t);
			regfree(a);
			return;
		}
		if(typefd[tf->etype] && !typefd[tt->etype]) {	/* float->fix */
			a = regalloc(types[TLONG], D_NONE);
			gopcode(OAS, types[TLONG], D_FPCR, t, a, t);
			gopcode(OAS, types[TLONG], D_CONST, nodconst(16), D_FPCR, t);
		}
		if(gf < D_F0 || gf >= D_F0+NREG) {
			g = regalloc(types[TDOUBLE], gt);
			gopcode(OFAS, tf, gf, f, g, t);
			if(g != gt)
				gopcode(OFAS, tt, g, t, gt, t);
			regfree(g);
		} else
			gopcode(OFAS, tt, gf, f, gt, t);
		if(typefd[tf->etype] && !typefd[tt->etype]) {	/* float->fix */
			gopcode(OAS, types[TLONG], a, t, D_FPCR, t);
			regfree(a);
		}
		return;
	}
	gopcode(OAS, tt, gf, f, gt, t);
}

void
gopcode(int o, Type *ty, int gf, Node *f, int gt, Node *t)
{
	int i, fidx, tidx;

	if(o == OAS)
		if(gf == gt)
			if(gf != D_TREE || f == t)
				return;

	fidx = D_NONE;
	tidx = D_NONE;
	i = 0;
	if(ty != T) {
		i = ty->etype;
		if(i >= NTYPE)
			i = 0;
	}
	nextpc();
	if(gf == D_TREE) {
		naddr(f, &p->from, fidx);
	} else {
		p->from.type = gf;
		if(gf == D_CONST) {
			p->from.offset = (long)(uintptr)f;
			if(typefd[i]) {
				p->from.type = D_FCONST;
				p->from.dval = (long)(uintptr)f;
			}
		}
	}
	p->as = opxt[o][i];
	if(gt == D_TREE) {
		naddr(t, &p->to, tidx);
	} else {
		p->to.type = gt;
		if(gt == D_CONST)
			p->to.offset = (long)(uintptr)t;
	}
	if(o == OBIT) {
		p->from.field = f->type->nbits;
		p->to.field = f->type->shift;
		if(p->from.field == 0)
			diag(Z, "BIT zero width bit field");
	}
	if(p->as == AMOVL || p->as == AMOVW || p->as == AMOVB)
		asopt();
	if(debug['g'])
		print("%P\n", p);
	if(p->as == AGOK)
		diag(Z, "GOK in gopcode: %s", onames[o]);
	if(fidx != D_NONE)
		regfree(fidx);
	if(tidx != D_NONE)
		regfree(tidx);
}

void
asopt(void)
{
	long v;
	int g;
	Prog *q;

	/*
	 * mov $0, ...
	 * ==>
	 * clr , ...
	 */
	v = 0;
	if(p->from.type == D_CONST) {
		v = p->from.offset;
		if(v == 0) {
			p->from.type = D_NONE;
			if(p->as == AMOVL)
				p->as = ACLRL;
			if(p->as == AMOVW)
				p->as = ACLRW;
			if(p->as == AMOVB)
				p->as = ACLRB;
			return;
		}
	}
	/*
	 * mov ..., TOS
	 * ==>
	 * pea (...)
	 */
	if(p->as == AMOVL && p->to.type == D_TOS)
	switch(p->from.type) {
	case D_CONST:
		p->from.type |= I_INDIR;
		p->to = p->from;
		p->from = zprog.from;
		p->as = APEA;
		return;

	case I_ADDR|D_EXTERN:
	case I_ADDR|D_STATIC:
		p->from.type &= ~I_ADDR;
		p->to = p->from;
		p->from = zprog.from;
		p->as = APEA;
		return;
	}
	/*
	 * movL $Qx, ...
	 * ==>
	 * movL $Qx,R
	 * movL R, ...
	 */
	if(p->as == AMOVL && p->from.type == D_CONST)
	if(v >= -128 && v < 128)
	if(p->to.type < D_R0 || p->to.type >= D_R0+NREG) {
		g = regalloc(types[TLONG], D_NONE);
		q = p;
		nextpc();
		p->as = AMOVL;
		p->from.type = g;
		p->to = q->to;
		q->to = p->from;
		regfree(g);
		if(debug['g'])
			print("%P\n", q);
		return;
	}
}

void
gbranch(int o)
{
	int a;

	a = ABNE;
	switch(o) {
	case ORETURN: a = ARTS; break;
	case OGOTO: a = ABRA; break;
	case OEQ: a = ABEQ; break;
	case ONE: a = ABNE; break;
	case OLE: a = ABLE; break;
	case OLS: a = ABLS; break;
	case OLT: a = ABLT; break;
	case OLO: a = ABCS; break;
	case OGE: a = ABGE; break;
	case OHS: a = ABCC; break;
	case OGT: a = ABGT; break;
	case OHI: a = ABHI; break;
	case OBIT: a = ABCS; break;
	case OCASE: a = ABCASE; break;
	}
	nextpc();
	p->from.type = D_NONE;
	p->to.type = D_NONE;
	p->as = a;
}

void
fpbranch(void)
{
	int a;

	a = p->as;
	switch(a) {
	case ABEQ: a = AFBEQ; break;
	case ABNE: a = AFBNE; break;
	case ABLE: a = AFBLE; break;
	case ABLT: a = AFBLT; break;
	case ABGE: a = AFBGE; break;
	case ABGT: a = AFBGT; break;
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
gpseudo(int a, Sym *s, int g, long v)
{

	nextpc();
	if(a == ADATA)
		pc--;
	p->as = a;
	p->to.type = g;
	p->to.offset = v;
	p->from.sym = s;
	p->from.type = D_EXTERN;
	if(s->class == CSTATIC)
		p->from.type = D_STATIC;
}

void
gpseudotree(int a, Sym *s, Node *n)
{

	nextpc();
	if(a == ADATA)
		pc--;
	p->as = a;
	naddr(n, &p->to, D_NONE);
	p->from.sym = s;
	p->from.type = D_EXTERN;
	if(s->class == CSTATIC)
		p->from.type = D_STATIC;
}

long
exreg(Type *t)
{
	long o;

	if(typechl[t->etype]) {
		if(exregoffset <= 5)
			return 0;
		o = exregoffset + D_R0;
		exregoffset--;
		return o;
	}
	if(t->etype == TIND) {
		if(exaregoffset <= 3)
			return 0;
		o = exaregoffset + D_A0;
		exaregoffset--;
		return o;
	}
	if(typefd[t->etype]) {
		if(exfregoffset <= 5)
			return 0;
		o = exfregoffset + D_F0;
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

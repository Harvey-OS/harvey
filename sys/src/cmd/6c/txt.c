#include "gc.h"

void
ginit(void)
{
	int i;
	Type *t;

	thechar = '6';
	thestring = "960";
	exregoffset = 0;
	exfregoffset = 0;
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
	zprog.type = D_NONE;
	zprog.from.type = D_NONE;
	zprog.from.index = D_NONE;
	zprog.from.scale = 0;
	zprog.to = zprog.from;

	regnode.op = OREGISTER;
	regnode.class = CEXREG;
	regnode.reg = REGTMP;
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

	memset(reg, 1, sizeof(reg));
	for(i=D_R0; i<D_R0+32; i++)
		reg[i] = 0;
	reg[REGSP]++;
	reg[REGZERO]++;
	reg[REGBAD1]++;
	reg[REGBAD2]++;
	reg[REGBAD3]++;
	reg[REGBAD4]++;
}

void
gclean(void)
{
	int i;
	Sym *s;

	reg[REGBAD4]--;
	reg[REGBAD3]--;
	reg[REGBAD2]--;
	reg[REGBAD1]--;
	reg[REGZERO]--;
	reg[REGSP]--;
	for(i=D_R0; i<D_R0+32; i++)
		if(reg[i])
			diag(Z, "reg %R left allocated", i);
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
	if(REGARG && curarg == 0 && typelp[n->type->etype]) {
		regaalloc1(tn1, n);
		if(n->complex >= FNX) {
			cgen(*fnxp, tn1);
			(*fnxp)++;
		} else
			cgen(n, tn1);
		return;
	}

	nod = znode;
	nod.op = OAS;
	regaalloc(tn2, n);
	nod.left = tn2;
	nod.right = n;
	if(n->complex >= FNX) {
		nod.right = *fnxp;
		(*fnxp)++;
	}
	nod.type = n->type;
	cgen(&nod, Z);
}

Node*
nodconst(long v)
{
	constnode.vconst = v;
	return &constnode;
}

int
nodreg(Node *n, Node *nn, int r)
{
	*n = regnode;
	n->reg = r;
	if(reg[r] == 0)
		return 0;
	if(nn != Z) {
		n->type = nn->type;
		n->lineno = nn->lineno;
		if(nn->op == OREGISTER)
		if(nn->reg == r)
			return 0;
	}
	return 1;
}

void
regret(Node *n, Node *nn)
{
	int r;

	r = REGRET;
	if(typefd[nn->type->etype])
		diag(nn, "no floating");
	nodreg(n, nn, r);
	reg[r]++;
}

int
tmpreg(void)
{
	int i;

	for(i=D_R0; i<D_R0+32; i++)
		if(reg[i] == 0)
			return i;
	diag(Z, "out of fixed registers");
	return 0;
}

void
regalloc(Node *n, Node *tn, Node *o)
{
	int i;

	switch(tn->type->etype) {
	case TCHAR:
	case TUCHAR:
	case TSHORT:
	case TUSHORT:
	case TLONG:
	case TULONG:
	case TIND:
		if(o != Z && o->op == OREGISTER) {
			i = o->reg;
			if(i >= D_R0 && i < D_R0+32)
				goto out;
		}
		for(i=D_R0; i<D_R0+32; i++)
			if(reg[i] == 0)
				goto out;
		diag(tn, "out of fixed registers");
		goto err;

	case TFLOAT:
	case TDOUBLE:
	case TVLONG:
		goto err;
	}
	diag(tn, "unknown type in regalloc: %T", tn->type);
err:
	i = D_R0;
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
	diag(n, "error in regfree: %O %d", n->op, i);
}

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
	n->xoffset = -(stkoff + cursafe);
	n->type = nn->type;
	n->etype = nn->type->etype;
	n->lineno = nn->lineno;
}

void
regaalloc1(Node *n, Node *nn)
{
	int r;

	r = REGARG;
	nodreg(n, nn, r);
	reg[r]++;
	curarg += 4;
}

void
regaalloc(Node *n, Node *nn)
{
	long o;

	*n = *nn;
	n->op = OINDREG;
	n->reg = REGSP;
	n->xoffset = curarg + 4;
	n->complex = 0;
	n->addable = 20;
	o = nn->type->width;
	o += round(o, tint->width);
	curarg += o;
	if(cursafe+curarg > maxargsafe)
		maxargsafe = cursafe+curarg;
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
naddr(Node *n, Adr *a)
{
	long v;

	a->type = D_NONE;
	if(n == Z)
		return;
	switch(n->op) {
	default:
	bad:
		diag(n, "bad in naddr: %O %D", n->op, a);
		break;

	case OREGISTER:
		a->type = n->reg;
		a->sym = S;
		break;


	case OINDEX:
	case OIND:
		naddr(n->left, a);
		if(a->type >= D_R0 && a->type < D_R0+32)
			a->type += D_INDIR;
		else
		if(a->type == D_CONST)
			a->type = D_NONE+D_INDIR;
		else
		if(a->type == D_ADDR) {
			a->type = a->index;
			a->index = D_NONE;
		} else
			goto bad;
		if(n->op == OINDEX) {
			a->index = idx.reg;
			a->scale = n->scale;
		}
		break;

	case OINDREG:
		a->type = n->reg+D_INDIR;
		a->sym = S;
		a->offset = n->xoffset;
		break;

	case ONAME:
		a->etype = n->etype;
		a->type = D_STATIC;
		a->sym = n->sym;
		a->offset = n->xoffset;
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
		if(typefd[n->type->etype]) {
			diag(n, "no floating");
			break;
		}
		a->sym = S;
		a->type = D_CONST;
		a->offset = n->vconst;
		break;

	case OADDR:
		naddr(n->left, a);
		if(a->type >= D_INDIR) {
			a->type -= D_INDIR;
			break;
		}
		if(a->type == D_EXTERN || a->type == D_STATIC ||
		   a->type == D_AUTO || a->type == D_PARAM)
			if(a->index == D_NONE) {
				a->index = a->type;
				a->type = D_ADDR;
				break;
			}
		goto bad;

	case OADD:
		if(n->right->op == OCONST) {
			v = n->right->vconst;
			naddr(n->left, a);
		} else
		if(n->left->op == OCONST) {
			v = n->left->vconst;
			naddr(n->right, a);
		} else
			goto bad;
		a->offset += v;
		break;

	}
}

#define	CASE(a,b)	((a<<8)|(b<<0))

void
gmove(Node *f, Node *t)
{
	int ft, tt, a;
	Node nod;

	ft = f->type->etype;
	tt = t->type->etype;
	if(debug['M'])
		print("gop: %O %O[%s],%O[%s]\n", OAS,
			f->op, tnames[ft], t->op, tnames[tt]);
	if(typefd[ft] && f->op == OCONST) {
		diag(f, "no floating");
		return;
	}
/*
 * load
 */
	if(f->op == ONAME || f->op == OINDREG || f->op == OIND || f->op == OINDEX)
	switch(ft) {
	case TCHAR:
		a = AMOVIB;
		goto ld;
	case TUCHAR:
		a = AMOVOB;
		goto ld;
	case TSHORT:
		if(typefd[tt]) {
			diag(t, "no floating");
			return;
		}
		a = AMOVIS;
		goto ld;
	case TUSHORT:
		a = AMOVOS;
		goto ld;
	case TLONG:
	case TULONG:
	case TIND:
		if(typefd[tt]) {
			diag(t, "no floating");
			return;
		}
		a = AMOV;
		goto ld;

	ld:
		regalloc(&nod, f, t);
		nod.type = types[TLONG];
		gins(a, f, &nod);
		gmove(&nod, t);
		regfree(&nod);
		return;

	case TFLOAT:
	case TDOUBLE:
	case TVLONG:
		diag(t, "no floating");
		return;
	}

/*
 * store
 */
	if(t->op == ONAME || t->op == OINDREG || t->op == OIND || t->op == OINDEX)
	switch(tt) {
	case TCHAR:
		a = AMOVIB;
		goto st;
	case TUCHAR:
		a = AMOVOB;
		goto st;
	case TSHORT:
		a = AMOVIS;
		goto st;
	case TUSHORT:
		a = AMOVOS;
		goto st;
	case TLONG:
	case TULONG:
	case TIND:
		a = AMOV;
		goto st;

	st:
		if(f->op == OCONST) {
			gins(a, f, t);
			return;
		}
		regalloc(&nod, t, f);
		gmove(f, &nod);
		gins(a, &nod, t);
		regfree(&nod);
		return;

	case TFLOAT:
	case TDOUBLE:
	case TVLONG:
		diag(f, "no floating");
		return;
	}

/*
 * convert
 */
	switch(CASE(ft,tt)) {
	default:
/*
 * integer to integer
 ********
		a = AGOK;	break;

	case CASE(	TCHAR,	TCHAR):
	case CASE(	TUCHAR,	TCHAR):
	case CASE(	TSHORT,	TCHAR):
	case CASE(	TUSHORT,TCHAR):
	case CASE(	TLONG,	TCHAR):
	case CASE(	TULONG,	TCHAR):
	case CASE(	TIND,	TCHAR):

	case CASE(	TCHAR,	TUCHAR):
	case CASE(	TUCHAR,	TUCHAR):
	case CASE(	TSHORT,	TUCHAR):
	case CASE(	TUSHORT,TUCHAR):
	case CASE(	TLONG,	TUCHAR):
	case CASE(	TULONG,	TUCHAR):
	case CASE(	TIND,	TUCHAR):

	case CASE(	TCHAR,	TSHORT):
	case CASE(	TUCHAR,	TSHORT):
	case CASE(	TSHORT,	TSHORT):
	case CASE(	TUSHORT,TSHORT):
	case CASE(	TLONG,	TSHORT):
	case CASE(	TULONG,	TSHORT):
	case CASE(	TIND,	TSHORT):

	case CASE(	TCHAR,	TUSHORT):
	case CASE(	TUCHAR,	TUSHORT):
	case CASE(	TSHORT,	TUSHORT):
	case CASE(	TUSHORT,TUSHORT):
	case CASE(	TLONG,	TUSHORT):
	case CASE(	TULONG,	TUSHORT):
	case CASE(	TIND,	TUSHORT):

	case CASE(	TCHAR,	TLONG):
	case CASE(	TUCHAR,	TLONG):
	case CASE(	TSHORT,	TLONG):
	case CASE(	TUSHORT,TLONG):
	case CASE(	TLONG,	TLONG):
	case CASE(	TULONG,	TLONG):
	case CASE(	TIND,	TLONG):

	case CASE(	TCHAR,	TULONG):
	case CASE(	TUCHAR,	TULONG):
	case CASE(	TSHORT,	TULONG):
	case CASE(	TUSHORT,TULONG):
	case CASE(	TLONG,	TULONG):
	case CASE(	TULONG,	TULONG):
	case CASE(	TIND,	TULONG):

	case CASE(	TCHAR,	TIND):
	case CASE(	TUCHAR,	TIND):
	case CASE(	TSHORT,	TIND):
	case CASE(	TUSHORT,TIND):
	case CASE(	TLONG,	TIND):
	case CASE(	TULONG,	TIND):
	case CASE(	TIND,	TIND):
 *****/
		a = AMOV;	break;

/*
 * float to fix
 */
	case CASE(	TFLOAT,	TCHAR):
	case CASE(	TFLOAT,	TUCHAR):
	case CASE(	TFLOAT,	TSHORT):
	case CASE(	TFLOAT,	TUSHORT):
	case CASE(	TFLOAT,	TLONG):
	case CASE(	TFLOAT,	TULONG):
	case CASE(	TFLOAT,	TIND):

	case CASE(	TDOUBLE,TCHAR):
	case CASE(	TDOUBLE,TUCHAR):
	case CASE(	TDOUBLE,TSHORT):
	case CASE(	TDOUBLE,TUSHORT):
	case CASE(	TDOUBLE,TLONG):
	case CASE(	TDOUBLE,TULONG):
	case CASE(	TDOUBLE,TIND):

	case CASE(	TVLONG,	TCHAR):
	case CASE(	TVLONG,	TUCHAR):
	case CASE(	TVLONG,	TSHORT):
	case CASE(	TVLONG,	TUSHORT):
	case CASE(	TVLONG,	TLONG):
	case CASE(	TVLONG,	TULONG):
	case CASE(	TVLONG,	TIND):
		diag(f, "no floating");
		return;

/*
 * fix to float
 */
	case CASE(	TCHAR,	TFLOAT):
	case CASE(	TUCHAR,	TFLOAT):
	case CASE(	TSHORT,	TFLOAT):
	case CASE(	TUSHORT,TFLOAT):
	case CASE(	TLONG,	TFLOAT):
	case CASE(	TULONG,	TFLOAT):
	case CASE(	TIND,	TFLOAT):

	case CASE(	TCHAR,	TDOUBLE):
	case CASE(	TUCHAR,	TDOUBLE):
	case CASE(	TSHORT,	TDOUBLE):
	case CASE(	TUSHORT,TDOUBLE):
	case CASE(	TLONG,	TDOUBLE):
	case CASE(	TULONG,	TDOUBLE):
	case CASE(	TIND,	TDOUBLE):

	case CASE(	TCHAR,	TVLONG):
	case CASE(	TUCHAR,	TVLONG):
	case CASE(	TSHORT,	TVLONG):
	case CASE(	TUSHORT,TVLONG):
	case CASE(	TLONG,	TVLONG):
	case CASE(	TULONG,	TVLONG):
	case CASE(	TIND,	TVLONG):
		diag(f, "no floating");
		return;

/*
 * float to float
 */
	case CASE(	TFLOAT,	TFLOAT):
	case CASE(	TDOUBLE,TFLOAT):
	case CASE(	TVLONG,	TFLOAT):

	case CASE(	TFLOAT,	TDOUBLE):
	case CASE(	TDOUBLE,TDOUBLE):
	case CASE(	TVLONG,	TDOUBLE):

	case CASE(	TFLOAT,	TVLONG):
	case CASE(	TDOUBLE,TVLONG):
	case CASE(	TVLONG,	TVLONG):
		diag(f, "no floating");
		return;
	}
	if(a == AMOV)
		if(samaddr(f, t))
			return;
	gins(a, f, t);
}

void
gins(int a, Node *f, Node *t)
{
	Node nod;
	long v;

	if(f != Z && f->op == OINDEX) {
		regalloc(&nod, &regnode, Z);
		v = constnode.vconst;
		cgen(f->right, &nod);
		constnode.vconst = v;
		idx.reg = nod.reg;
		regfree(&nod);
	}
	if(t != Z && t->op == OINDEX) {
		regalloc(&nod, &regnode, Z);
		v = constnode.vconst;
		cgen(t->right, &nod);
		constnode.vconst = v;
		idx.reg = nod.reg;
		regfree(&nod);
	}
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
fgopcode(int o, Node *f, Node *t, int pop, int rev)
{

	USED(o);
	USED(t);
	USED(pop);
	USED(rev);
	diag(f, "no floating");
}

void
gopcode(int o, Node *f, Node *f1, Node *t)
{
	int a;
	Adr adr;

	a = AGOK;
	switch(o) {
	case OADDR:
		a = AMOVA;
		break;

	case OASADD:
	case OADD:
		a = AADDO;
		break;

	case OASSUB:
	case OSUB:
		a = ASUBO;
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
		a = ASHRO;
		break;

	case OASASHR:
	case OASHR:
		a = ASHRI;
		break;

	case OASASHL:
	case OASHL:
		a = ASHLI;
		break;

	case OFUNC:
		a = ABAL;
		break;

	case OASMUL:
	case OMUL:
		a = AMULI;
		break;

	case OASMOD:
	case OMOD:
		a = AREMI;
		break;

	case OASDIV:
	case ODIV:
		a = ADIVI;
		break;

	case OASLMUL:
	case OLMUL:
		a = AMULO;
		break;

	case OASLMOD:
	case OLMOD:
		a = AREMO;
		break;

	case OASLDIV:
	case OLDIV:
		a = ADIVO;
		break;

	case OEQ:
	case ONE:
	case OLT:
	case OLE:
	case OGE:
	case OGT:
		a = ACMPI;
		goto cmp;

	case OLO:
	case OLS:
	case OHS:
	case OHI:
		a = ACMPO;
	cmp:
		if(f1 != Z)
			diag(f1, "middle operand for cmp");
		gins(a, f, t);
		switch(o) {
		case OEQ:	a = ABE; break;
		case ONE:	a = ABNE; break;
		case OLT:	a = ABL; break;
		case OLE:	a = ABLE; break;
		case OGE:	a = ABGE; break;
		case OGT:	a = ABG; break;
		case OLO:	a = ABL; break;
		case OLS:	a = ABLE; break;
		case OHS:	a = ABGE; break;
		case OHI:	a = ABG; break;
		}
		gins(a, Z, Z);
		return;
	}
	if(a == AGOK)
		diag(Z, "bad in gopcode %O", o);
	gins(a, f, t);
	if(f1 != Z) {
		naddr(f1, &adr);
		if(adr.type < D_R0 || adr.type >= D_R0+32) {
			if(adr.type != D_CONST) {
				diag(f1, "middle operand must be reg/const");
				return;
			}
			if(adr.offset >= 32) {
				diag(f1, "middle const must be < 32");
				return;
			}
		}
		p->type = adr.type;
		p->offset = adr.offset;
	}
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
		a = ARTS;
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
	p->from.type = D_EXTERN;
	p->from.sym = s;
	if(s->class == CSTATIC)
		p->from.type = D_STATIC;
	naddr(n, &p->to);
	if(a == ADATA || a == AGLOBL)
		pc--;
}

int
sconst(Node *n)
{
	long v;

	if(n->op == OCONST && !typefd[n->type->etype]) {
		v = n->vconst;
		if(v >= 0 && v < 4096)
			return 1;
	}
	return 0;
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

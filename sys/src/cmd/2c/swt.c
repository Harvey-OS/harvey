#include "gc.h"

int
swcmp(const void *a1, const void *a2)
{
	C1 *p1, *p2;

	p1 = (C1*)a1;
	p2 = (C1*)a2;
	if(p1->val < p2->val)
		return -1;
	return p1->val > p2->val;
}

void
doswit(int g, Node *n)
{
	Case *c;
	C1 *q, *iq;
	long def, nc, i;

	def = 0;
	nc = 0;
	for(c = cases; c->link != C; c = c->link) {
		if(c->def) {
			if(def)
				diag(n, "more than one default in switch");
			def = c->label;
			continue;
		}
		nc++;
	}

	iq = alloc(nc*sizeof(C1));
	q = iq;
	for(c = cases; c->link != C; c = c->link) {
		if(c->def)
			continue;
		q->label = c->label;
		q->val = c->val;
		q++;
	}
	qsort(iq, nc, sizeof(C1), swcmp);
	if(def == 0)
		def = breakpc;
	for(i=0; i<nc-1; i++)
		if(iq[i].val == iq[i+1].val)
			diag(n, "duplicate cases in switch %ld", iq[i].val);
	swit1(iq, nc, def, g, n);
}

#define	N1	4	/* ncase: always linear */
#define	N2	5	/* min ncase: direct */
#define	N3	4	/* range/ncase: direct */
			/* else binary */
void
swit1(C1 *q, int nc, long def, int g, Node *n)
{
	C1 *r, *s;
	int i, l, m, y;
	long v, range;
	Prog *sp1, *sp2;

	/* note that g and g+1 are not allocated */
	if(nc <= N1)
		goto linear;
	y = 23*nc/100 + 5;	/* number of cases needed to make */
	if(y < N2)		/* direct switch worthwile */
		y = N2;				/* try to do better than n**2 here */
	for(m=nc; m>=y; m--) {			/* m is number of cases */
		s = q+nc;
		r = s-m;
		for(l=nc-m; l>=0; l--) {	/* l is base of contig cases */
			s--;
			range = s->val - r->val;
			if(range > 0 && range <= N3*m)
				goto direct;
			r--;
		}
	}

	/*
	 * divide and conquer
	 */
	i = nc / 2;
	r = q+i;
	v = r->val;
	/* compare median */
	if(v >= -128 && v < 128) {
		gopcode(OAS, n->type, D_CONST, nodconst(v), g+1, n);
		gopcode(OEQ, n->type, g, n, g+1, n);
	} else
		gopcode(OEQ, n->type, g, n, D_CONST, nodconst(v));
	gbranch(OLT);
	sp1 = p;
	gbranch(OGT);
	sp2 = p;
	gbranch(OGOTO);
	patch(p, r->label);

	patch(sp1, pc);
	swit1(q, i, def, g, n);

	patch(sp2, pc);
	swit1(r+1, nc-i-1, def, g, n);
	return;

direct:
	/* compare low bound */
	v = r->val;
	if(v >= -128 && v < 128) {
		gopcode(OAS, n->type, D_CONST, nodconst(v), g+1, n);
		gopcode(OEQ, n->type, g, n, g+1, n);
	} else
		gopcode(OEQ, n->type, g, n, D_CONST, nodconst(v));
	gbranch(OLT);
	sp1 = p;

	/* compare high bound */
	v = s->val;
	if(v >= -128 && v < 128) {
		gopcode(OAS, n->type, D_CONST, nodconst(v), g+1, n);
		gopcode(OEQ, n->type, g, n, g+1, n);
	} else
		gopcode(OEQ, n->type, g, n, D_CONST, nodconst(v));
	gbranch(OGT);
	sp2 = p;

	/* switch */
	v = r->val;
	gpseudo(AMOVW, symstatic, D_R0, 0L);
	p->from.offset = nstatic - v*2;
	p->from.index = g|I_INDEX1;
	p->from.scale = 5;
	nextpc();
	p->as = ACASEW;

	/* table */
	for(i=0; i<=range; i++) {
		gbranch(OCASE);
		if(v == r->val) {
			patch(p, r->label);
			r++;
		} else
			patch(p, def);
		p->from.type = D_STATIC;
		p->from.sym = symstatic;
		p->from.offset = nstatic;
		nstatic += types[TSHORT]->width;
		v++;
	}
	gbranch(OGOTO);
	patch(p, def);
	if(r != s+1)
		print("smelly direct switch\n");

	if(l > 0) {
		patch(sp1, pc);
		swit1(q, l, def, g, n);
	} else
		patch(sp1, def);

	m += l;
	if(m < nc) {
		patch(sp2, pc);
		swit1(q+m, nc-m, def, g, n);
	} else
		patch(sp2, def);
	return;


linear:
	for(i=0; i<nc; i++) {
		v = q->val;
		if(v >= -128 && v < 128) {
			gopcode(OAS, n->type, D_CONST, nodconst(v), g+1, n);
			gopcode(OEQ, n->type, g+1, n, g, n);
		} else
			gopcode(OEQ, n->type, g, n, D_CONST, nodconst(v));
		gbranch(OEQ);
		patch(p, q->label);
		q++;
	}
	gbranch(OGOTO);
	patch(p, def);
}

void
cas(void)
{
	Case *c;

	c = alloc(sizeof(*c));
	c->link = cases;
	cases = c;
}


int
bitload(Node *b, int n1, int n2, int n3, Node *nn)
{
	int sh, g, gs;
	long v;
	Node *l;
	Type *t;

	/*
	 * n1 gets adjusted/masked value
	 * n2 gets address of cell
	 * n3 gets contents of cell
	 */
	gs = 0;
	t = tfield;

	l = b->left;
	g = regalloc(t, n3);
	if(n2 != D_NONE) {
		lcgen(l, n2, Z);
		n2 |= I_INDIR;
		gmove(t, t, n2, l, g, l);
		gmove(t, t, g, l, n1, l);
	} else
		cgen(l, g, nn);
	if(b->type->shift == 0 && typeu[b->type->etype]) {
		v = ~0 + (1L << b->type->nbits);
		gopcode(OAND, t, D_CONST, nodconst(v), g, l);
	} else {
		sh = 32 - b->type->shift - b->type->nbits;
		if(sh > 0)
			if(sh >= 8) {
				gs = regalloc(t, D_NONE);
				gmove(t, t, D_CONST, nodconst(sh), gs, l);
				gopcode(OASHL, t, gs, l, g, l);
				if(b->type->shift)
					regfree(gs);
			} else
				gopcode(OASHL, t, D_CONST, nodconst(sh), g, l);
		sh += b->type->shift;
		if(sh > 0) {
			if(sh >= 8) {
				if(b->type->shift) {
					gs = regalloc(t, D_NONE);
					gmove(t, t, D_CONST, nodconst(sh), gs, l);
				}
				if(typeu[b->type->etype])
					gopcode(OLSHR, t, gs, l, g, l);
				else
					gopcode(OASHR, t, gs, l, g, l);
				regfree(gs);
			} else {
				if(typeu[b->type->etype])
					gopcode(OLSHR, t, D_CONST, nodconst(sh), g, l);
				else
					gopcode(OASHR, t, D_CONST, nodconst(sh), g, l);
			}
		}
	}
	return g;
}

void
bitstore(Node *b, int n1, int n2, int n3, int result, Node *nn)
{
	long v;
	Node *l;
	Type *t;
	int sh, g, gs;

	/*
	 * n1 has adjusted/masked value
	 * n2 has address of cell
	 * n3 has contents of cell
	 */
	t = tfield;

	l = b->left;
	g = regalloc(t, D_NONE);
	v = ~0 + (1L << b->type->nbits);
	gopcode(OAND, t, D_CONST, nodconst(v), n1, l);
	gmove(t, t, n1, l, g, l);
	if(result != D_NONE)
		gmove(t, nn->type, n1, l, result, nn);
	sh = b->type->shift;
	if(sh > 0) {
		if(sh >= 8) {
			gs = regalloc(t, D_NONE);
			gmove(t, t, D_CONST, nodconst(sh), gs, l);
			gopcode(OASHL, t, gs, l, g, l);
			regfree(gs);
		} else
			gopcode(OASHL, t, D_CONST, nodconst(sh), g, l);
	}
	v <<= sh;
	gopcode(OAND, t, D_CONST, nodconst(~v), n3, l);
	gopcode(OOR, t, n3, l, g, l);
	gmove(t, t, g, l, n2|I_INDIR, l);

	regfree(g);
	regfree(n1);
	regfree(n2);
	regfree(n3);
}

long
outstring(char *s, long n)
{
	long r;

	r = nstring;
	while(n) {
		string[mnstring] = *s++;
		mnstring++;
		nstring++;
		if(mnstring >= NSNAME) {
			gpseudo(ADATA, symstring, D_SCONST, 0L);
			memmove(p->to.sval, string, NSNAME);
			p->from.offset = nstring - NSNAME;
			p->from.displace = NSNAME;
			mnstring = 0;
		}
		n--;
	}
	return r;
}

long
outlstring(ushort *s, long n)
{
	char buf[2];
	int c;
	long r;

	while(nstring & 1)
		outstring("", 1);
	r = nstring;
	while(n > 0) {
		c = *s++;
		if(align(0, types[TCHAR], Aarg1)) {
			buf[0] = c>>8;
			buf[1] = c;
		} else {
			buf[0] = c;
			buf[1] = c>>8;
		}
		outstring(buf, 2);
		n -= sizeof(ushort);
	}
	return r;
}

int
doinc(Node *n, int f)
{
	Node *l;
	int a;

loop:
	if(n == Z)
		return 0;
	l = n->left;
	switch(n->op) {

	case OPOSTINC:
	case OPOSTDEC:
		if(f & POST) {
			a = n->addable;
			if(a >= INDEXED) {
				if(f & TEST)
					return 1;
				n->addable = 0;
				cgen(n, D_NONE, n);
				n->addable = a;
			}
		}
		break;

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

	case OPREINC:
	case OPREDEC:
		if(f & PRE) {
			a = n->addable;
			if(a >= INDEXED) {
				if(f & TEST)
					return 1;
				n->addable = 0;
				doinc(n, PRE);
				cgen(n, D_NONE, n);
				n->addable = a;
				return 0;
			}
		}
		break;

	case OFUNC:
		if(f & PRE)
			break;
		return 0;

	case ONAME:
	case OREGISTER:
	case OSTRING:
	case OCONST:

	case OANDAND:
	case OOROR:
		return 0;

	case OCOND:
		return 0;

	case OCOMMA:
		n = n->right;
		if(f & PRE)
			n = l;
		goto loop;
	}
	if(l != Z)
		if(doinc(l, f))
			return 1;
	n = n->right;
	goto loop;
}

void
setsp(void)
{

	nextpc();
	p->as = AADJSP;
	p->from.type = D_CONST;
	p->from.offset = 0;
}

void
adjsp(long o)
{

	if(o != 0) {
		nextpc();
		p->as = AADJSP;
		p->from.type = D_CONST;
		p->from.offset = o;
		argoff += o;
	}
}

int
simplv(Node *n)
{

	if(n->addable <= INDEXED)
		return 0;
	while(n->op == OIND)
		n = n->left;
	if(n->op == ONAME)
		return 1;
	return 0;
}

int
eval(Node *n, int g)
{

	if(n->addable >= INDEXED)
		return D_TREE;
	g = regalloc(n->type, g);
	cgen(n, g, n);
	return g;
}

void	outhist(Biobuf*);
void	zname(Biobuf*, Sym*, int);
void	zaddr(Biobuf*, Adr*, int);
void	zwrite(Biobuf*, Prog*, int, int);

void
outcode(void)
{
	struct { Sym *sym; short type; } h[NSYM];
	Prog *p;
	Sym *s;
	int f, sf, st, t, sym;
	Biobuf b;

	if(debug['S']) {
		for(p = firstp; p != P; p = p->link)
			if(p->as != ADATA && p->as != AGLOBL)
				pc--;
		for(p = firstp; p != P; p = p->link) {
			print("%P\n", p);
			if(p->as != ADATA && p->as != AGLOBL)
				pc++;
		}
	}
	f = open(outfile, OWRITE);
	if(f < 0) {
		diag(Z, "cant open %s", outfile);
		errorexit();
	}
	Binit(&b, f, OWRITE);
	Bseek(&b, 0L, 2);
	outhist(&b);
	for(sym=0; sym<NSYM; sym++) {
		h[sym].sym = S;
		h[sym].type = 0;
	}
	sym = 1;
	for(p = firstp; p != P; p = p->link) {
	jackpot:
		sf = 0;
		s = p->from.sym;
		while(s != S) {
			sf = s->sym;
			if(sf < 0 || sf >= NSYM)
				sf = 0;
			t = p->from.type & D_MASK;
			if(h[sf].type == t)
			if(h[sf].sym == s)
				break;
			s->sym = sym;
			zname(&b, s, t);
			h[sym].sym = s;
			h[sym].type = t;
			sf = sym;
			sym++;
			if(sym >= NSYM)
				sym = 1;
			break;
		}
		st = 0;
		s = p->to.sym;
		while(s != S) {
			st = s->sym;
			if(st < 0 || st >= NSYM)
				st = 0;
			t = p->to.type & D_MASK;
			if(h[st].type == t)
			if(h[st].sym == s)
				break;
			s->sym = sym;
			zname(&b, s, t);
			h[sym].sym = s;
			h[sym].type = t;
			st = sym;
			sym++;
			if(sym >= NSYM)
				sym = 1;
			if(st == sf)
				goto jackpot;
			break;
		}
		zwrite(&b, p, sf, st);
	}
	Bflush(&b);
	close(f);
	firstp = P;
	lastp = P;
}

void
zwrite(Biobuf *b, Prog *p, int sf, int st)
{
	long l;

	l = p->as;
	Bputc(b, l);
	Bputc(b, l>>8);
	l = p->lineno;
	Bputc(b, l);
	Bputc(b, l>>8);
	Bputc(b, l>>16);
	Bputc(b, l>>24);
	zaddr(b, &p->from, sf);
	zaddr(b, &p->to, st);
}

void
zname(Biobuf *b, Sym *s, int t)
{
	char *n;
	ulong sig;

	if(debug['T'] && t == D_EXTERN && s->sig != SIGDONE && s->type != types[TENUM] && s != symrathole){
		sig = sign(s);
		Bputc(b, ASIGNAME);
		Bputc(b, ASIGNAME>>8);
		Bputc(b, sig);
		Bputc(b, sig>>8);
		Bputc(b, sig>>16);
		Bputc(b, sig>>24);
		s->sig = SIGDONE;
	}
	else{
		Bputc(b, ANAME);	/* as */
		Bputc(b, ANAME>>8);	/* as */
	}
	Bputc(b, t);		/* type */
	Bputc(b, s->sym);		/* sym */
	n = s->name;
	while(*n) {
		Bputc(b, *n);
		n++;
	}
	Bputc(b, 0);
}

void
zaddr(Biobuf *b, Adr *a, int s)
{
	long l;
	int i, t;
	char *n;
	Ieee e;

	t = 0;
	if(a->field)
		t |= T_FIELD;
	if(a->index != D_NONE)
		t |= T_INDEX;
	if(s)
		t |= T_SYM;

	switch(a->type) {
	default:
		if(a->offset)
			t |= T_OFFSET;
		if(a->displace)
			t |= T_INDEX;
		if(a->type & ~0xff)
			t |= T_TYPE;
		break;
	case D_FCONST:
		t |= T_FCONST;
		break;
	case D_SCONST:
		t |= T_SCONST;
		break;
	}
	Bputc(b, t);

	if(t & T_FIELD) {	/* implies field */
		i = a->field;
		Bputc(b, i);
		Bputc(b, i>>8);
	}
	if(t & T_INDEX) {	/* implies index, scale, displace */
		i = a->index;
		Bputc(b, i);
		Bputc(b, i>>8);
		Bputc(b, a->scale);
		l = a->displace;
		Bputc(b, l);
		Bputc(b, l>>8);
		Bputc(b, l>>16);
		Bputc(b, l>>24);
	}
	if(t & T_OFFSET) {	/* implies offset */
		l = a->offset;
		Bputc(b, l);
		Bputc(b, l>>8);
		Bputc(b, l>>16);
		Bputc(b, l>>24);
	}
	if(t & T_SYM)		/* implies sym */
		Bputc(b, s);
	if(t & T_FCONST) {
		ieeedtod(&e, a->dval);
		l = e.l;
		Bputc(b, l);
		Bputc(b, l>>8);
		Bputc(b, l>>16);
		Bputc(b, l>>24);
		l = e.h;
		Bputc(b, l);
		Bputc(b, l>>8);
		Bputc(b, l>>16);
		Bputc(b, l>>24);
		return;
	}
	if(t & T_SCONST) {
		n = a->sval;
		for(i=0; i<NSNAME; i++) {
			Bputc(b, *n);
			n++;
		}
		return;
	}
	i = a->type;
	Bputc(b, i);
	if(t & T_TYPE)
		Bputc(b, i>>8);
}



void
outhist(Biobuf *b)
{
	Hist *h;
	char *p, *q, *op, c;
	Prog pg;
	int n;

	pg = zprog;
	pg.as = AHISTORY;
	c = pathchar();
	for(h = hist; h != H; h = h->link) {
		p = h->name;
		op = 0;
		/* on windows skip drive specifier in pathname */
		if(systemtype(Windows) && p && p[1] == ':'){
			p += 2;
			c = *p;
		}
		if(p && p[0] != c && h->offset == 0 && pathname){
			/* on windows skip drive specifier in pathname */
			if(systemtype(Windows) && pathname[1] == ':') {
				op = p;
				p = pathname+2;
				c = *p;
			} else if(pathname[0] == c){
				op = p;
				p = pathname;
			}
		}
		while(p) {
			q = utfrune(p, c);
			if(q) {
				n = q-p;
				if(n == 0){
					n = 1;	/* leading "/" */
					*p = '/';	/* don't emit "\" on windows */
				}
				q++;
			} else {
				n = strlen(p);
				q = 0;
			}
			if(n) {
				Bputc(b, ANAME);
				Bputc(b, ANAME>>8);
				Bputc(b, D_FILE);
				Bputc(b, 1);
				Bputc(b, '<');
				Bwrite(b, p, n);
				Bputc(b, 0);
			}
			p = q;
			if(p == 0 && op) {
				p = op;
				op = 0;
			}
		}
		pg.lineno = h->line;
		pg.to.type = zprog.to.type;
		pg.to.offset = h->offset;
		if(h->offset)
			pg.to.type = D_CONST;

		zwrite(b, &pg, 0, 0);
	}
}

void
ieeedtod(Ieee *ieee, double native)
{
	double fr, ho, f;
	int exp;

	if(native < 0) {
		ieeedtod(ieee, -native);
		ieee->h |= 0x80000000L;
		return;
	}
	if(native == 0) {
		ieee->l = 0;
		ieee->h = 0;
		return;
	}
	fr = frexp(native, &exp);
	f = 2097152L;		/* shouldnt use fp constants here */
	fr = modf(fr*f, &ho);
	ieee->h = ho;
	ieee->h &= 0xfffffL;
	ieee->h |= (exp+1022L) << 20;
	f = 65536L;
	fr = modf(fr*f, &ho);
	ieee->l = ho;
	ieee->l <<= 16;
	ieee->l |= (long)(fr*f);
}

int
nodalloc(Type *t, int g, Node *n)
{

	n->type = t;
	n->op = OREGISTER;
	n->addable = 12;
	n->complex = 0;
	g = regaddr(g);
	n->reg = g | I_INDIR;
	n->xoffset = 0;
	return g;
}

int
mulcon(Node *n, Node *c, int result, Node *nn)
{
	long v;

	if(typefd[n->type->etype])
		return 0;
	v = c->vconst;
	if(mulcon1(n, v, result, nn))
		return 1;
	return 0;
}

int
shlcon(Node *n, Node *c, int result, Node *nn)
{
	long v;

	v = 1L << c->vconst;
	return mulcon1(n, v, result, nn);
}

int
mulcon1(Node *n, long v, int result, Node *nn)
{
	int g, g1, a1, a2, neg;
	int o;
	char code[10], *p;

	if(result == D_NONE)
		return 0;
	neg = 0;
	if(v < 0) {
		v = -v;
		neg++;
	}
	a1 = 0;
	a2 = multabsize;
	for(;;) {
		if(a1 >= a2)
			return 0;
		g1 = (a2 + a1)/2;
		if(v < multab[g1].val) {
			a2 = g1;
			continue;
		}
		if(v > multab[g1].val) {
			a1 = g1+1;
			continue;
		}
		break;
	}
	strcpy(code, "0");
	strncat(code, multab[g1].code, sizeof(multab[0].code));
	p = code;
	if(p[1] == 'i')
		p += 2;
	g = regalloc(n->type, result);
	cgen(n, g, n);
	if(neg)
		gopcode(ONEG, n->type, D_NONE, n, g, n);
	g1 = regalloc(n->type, D_NONE);
loop:
	switch(*p) {
	case 0:
		regfree(g1);
		gmove(n->type, nn->type, g, n, result, nn);
		regfree(g);
		return 1;
	case '0':
		o = OAS;
		*p -= '0';
		goto com;
	case '1':
	case '2':
		o = OSUB;
		*p -= '1';
		goto com;
	case '3':
	case '4':
	case '5':
	case '6':
		o = OADD;
		*p -= '3';
	com:
		a1 = g;
		if(*p == 1 || *p == 3)
			a1 = g1;
		a2 = g;
		if(*p == 0 || *p == 3)
			a2 = g1;
		gopcode(o, n->type, a1, n, a2, n);
		p++;
		break;
	default:
		a1 = *p++ - 'a' + 1;
		a2 = g;
		if(a1 > 8) {
			a2 = g1;
			a1 -= 8;
		}
		gopcode(OASHL, n->type, D_CONST, nodconst(a1), a2, n);
		break;
	}
	goto loop;
}

void
nullwarn(Node *l, Node *r)
{
	warn(Z, "result of operation not used");
	if(l != Z)
		cgen(l, D_NONE, Z);
	if(r != Z)
		cgen(r, D_NONE, Z);
}

void
sextern(Sym *s, Node *a, long o, long w)
{
	long e, lw;

	for(e=0; e<w; e+=NSNAME) {
		lw = NSNAME;
		if(w-e < lw)
			lw = w-e;
		gpseudo(ADATA, s, D_SCONST, 0L);
		p->from.offset += o+e;
		p->from.displace = lw;
		memmove(p->to.sval, a->cstring+e, lw);
	}
}

void
gextern(Sym *s, Node *a, long o, long w)
{
	if(a->op == OCONST && typev[a->type->etype]) {
		gpseudo(ADATA, s, D_CONST, (long)(a->vconst>>32));
		p->from.offset += o;
		p->from.displace = 4;
		gpseudo(ADATA, s, D_CONST, (long)(a->vconst));
		p->from.offset += o + 4;
		p->from.displace = 4;
		return;
	}
	gpseudotree(ADATA, s, a);
	p->from.offset += o;
	p->from.displace = w;
}

long
align(long i, Type *t, int op)
{
	long o;
	Type *v;
	int w;

	o = i;
	w = 1;
	switch(op) {
	default:
		diag(Z, "unknown align opcode %d", op);
		break;

	case Asu2:	/* padding at end of a struct */
		w = SZ_LONG;
		if(packflg)
			w = packflg;
		break;

	case Ael1:	/* initial allign of struct element */
		for(v=t; v->etype==TARRAY; v=v->link)
			;
		w = ewidth[v->etype];
		if(w <= 0 || w >= SZ_SHORT)
			w = SZ_SHORT;
		if(packflg)
			w = packflg;
		break;

	case Ael2:	/* width of a struct element */
		o += t->width;
		break;

	case Aarg0:	/* initial passbyptr argument in arg list */
		if(typesuv[t->etype]) {
			o = align(o, types[TIND], Aarg1);
			o = align(o, types[TIND], Aarg2);
		}
		break;

	case Aarg1:	/* initial allign of parameter */
		w = ewidth[t->etype];
		if(w <= 0 || w >= SZ_LONG) {
			w = SZ_LONG;
			break;
		}
		o += SZ_LONG - w;	/* big endian adjustment */
		w = 1;
		break;

	case Aarg2:	/* width of a parameter */
		o += t->width;
		w = SZ_LONG;
		break;

	case Aaut3:	/* total allign of automatic */
		o = align(o, t, Ael1);
		o = align(o, t, Ael2);
		break;
	}
	o = round(o, w);
	if(debug['A'])
		print("align %s %ld %T = %ld\n", bnames[op], i, t, o);
	return o;
}

long
maxround(long max, long v)
{
	v += SZ_LONG-1;
	if(v > max)
		max = round(v, SZ_LONG);
	return max;
}

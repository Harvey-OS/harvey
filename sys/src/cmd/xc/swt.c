#include "gc.h"

int
swcmp(void *a1, void *a2)
{
	C1 *p1, *p2;

	p1 = a1;
	p2 = a2;
	if(p1->val < p2->val)
		return -1;
	return  p1->val > p2->val;
}

/*
 * compile a switch statement
 * n is the value switched on
 */
void
doswit(Node *n)
{
	Case *c;
	C1 *q, *iq;
	long def, nc, i;
	Node tn;

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

	i = nc*sizeof(C1);
	while(nhunk < i)
		gethunk();
	iq = (C1*)hunk;
	nhunk -= i;
	hunk += i;

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
	regalloc(&tn, &regnode, Z);
	swit1(iq, nc, def, n, &tn);
	regfree(&tn);
}

/*
 * q is an array of nc case labels, def is the default
 * n is the expression, and tn is a tmp register
 */
void
swit1(C1 *q, int nc, long def, Node *n, Node *tn)
{
	C1 *r;
	int i;
	Prog *sp;

	if(nc < 5) {
		for(i=0; i<nc; i++) {
			if(sval(q->val)) {
				gopcode(OEQ, n, Z, nodconst(q->val));
			} else {
				gmove(nodconst(q->val), tn);
				gopcode(OEQ, n, Z, tn);
			}
			patch(p, q->label);
			q++;
		}
		gbranch(OGOTO);
		patch(p, def);
		return;
	}
	i = nc / 2;
	r = q+i;
	if(sval(r->val)) {
		gopcode(OGT, n, Z, nodconst(r->val));
		sp = p;
	} else {
		gmove(nodconst(q->val), tn);
		gopcode(OGT, n, Z, tn);
		sp = p;
	}
	/*
	 * sleezy hack
	 * we know the condition codes are already set,
	 * so just emit the conditional jump
	 */
	gbranch(OGOTO);
	p->as = ABRA;
	p->cc = CCEQ;
	patch(p, r->label);
	swit1(q, i, def, n, tn);

	patch(sp, pc);
	swit1(r+1, nc-i-1, def, n, tn);
}

void
cas(void)
{
	Case *c;

	ALLOC(c, Case);
	c->link = cases;
	cases = c;
}

void
bitload(Node *b, Node *n1, Node *n2, Node *n3, Node *nn)
{
	int sh;
	long v;
	Node *l;

	/*
	 * n1 gets adjusted/masked value
	 * n2 gets address of cell
	 * n3 gets contents of cell
	 */
	l = b->left;
	regalloc(n1, l, nn);
	if(n2 != Z) {
		reglcgen(n2, l, Z);
		regalloc(n3, l, Z);
		gopcode(OAS, n2, Z, n3);
		gopcode(OAS, n3, Z, n1);
	} else {
		cgen(l, n1);
	}
	if(b->type->shift == 0 && typeu[b->type->etype]) {
		v = ~0 + (1L << b->type->nbits);
		gopcode(OAND, nodconst(v), Z, n1);
	} else {
		sh = 32 - b->type->shift - b->type->nbits;
		if(sh > 0)
			gopcode(OASHL, nodconst(sh), Z, n1);
		sh += b->type->shift;
		if(sh > 0)
			if(typeu[b->type->etype])
				gopcode(OLSHR, nodconst(sh), Z, n1);
			else
				gopcode(OASHR, nodconst(sh), Z, n1);
	}
}

void
bitstore(Node *b, Node *n1, Node *n2, Node *n3, Node *nn)
{
	long v;
	Node nod, *l;
	int sh;

	/*
	 * n1 has adjusted/masked value
	 * n2 has address of cell
	 * n3 has contents of cell
	 */
	l = b->left;
	regalloc(&nod, l, Z);
	v = ~0 + (1L << b->type->nbits);
	gopcode(OAND, nodconst(v), Z, n1);
	gopcode(OAS, n1, Z, &nod);
	if(nn != Z)
		gopcode(OAS, n1, Z, nn);
	sh = b->type->shift;
	if(sh > 0)
		gopcode(OASHL, nodconst(sh), Z, &nod);
	v <<= sh;
	gopcode(OAND, nodconst(~v), Z, n3);
	gopcode(OOR, n3, Z, &nod);
	gopcode(OAS, &nod, Z, n2);

	regfree(&nod);
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
			gpseudo(ADATA, symstring, nodconst(0L));
			p->from.offset += nstring - NSNAME;
			p->reg = NSNAME;
			p->to.type = D_SCONST;
			memmove(p->to.sval, string, NSNAME);
			mnstring = 0;
		}
		n--;
	}
	return r;
}

/*
 * output a wide character string
 */
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
		if(endian(0)) {
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

/*
 * return log(n) if n is a power of 2 constant
 */
int
vlog(Node *n)
{

	int s, i;
	ulong m, v;

	if(n->op != OCONST)
		goto bad;
	if(!typechlp[n->type->etype])
		goto bad;

	v = n->vconst;

	s = 0;
	m = MASK(64);
	for(i=32; i; i>>=1) {
		m >>= i;
		if(!(v & m)) {
			v >>= i;
			s += i;
		}
	}
	if(v == 1)
		return s;

bad:
	return -1;
}

int
asmulcon(Node *n, Node *nn)
{
	Node *l, *r, nod1, nod2;
	Multab *m;
	long v;

	if(typefd[n->type->etype])
		return 0;
	l = n->left;
	r = n->right;
	if(r->op != OCONST)
		return 0;
	v = r->vconst;
	m = mulcon0(v);
	if(!m)
		return 0;
	if(l->addable < ADDABLE)
		reglcgen(&nod2, l, nn);
	else
		nod2 = *l;
	regalloc(&nod1, n, nn);
	gopcode(OAS, &nod2, Z, &nod1);

	mulcon1(m, v, &nod1);

	gopcode(OAS, &nod1, Z, &nod2);
	regfree(&nod1);
	if(l->addable < ADDABLE)
		regfree(&nod2);
	return 1;
}

/*
 * n is a multiply expression, destination nn
 * compile it using shifts and adds if a constant
 */
int
mulcon(Node *n, Node *nn)
{
	Node *l, *r, nod1;
	Multab *m;
	long v;

	if(typefd[n->type->etype])
		return 0;
	l = n->left;
	r = n->right;
	if(l->op == OCONST) {
		l = r;
		r = n->left;
	}
	if(r->op != OCONST)
		return 0;
	v = r->vconst;
	m = mulcon0(v);
	if(!m)
		return 0;

	regalloc(&nod1, n, nn);
	cgen(l, &nod1);

	mulcon1(m, v, &nod1);

	gmove(&nod1, nn);
	regfree(&nod1);
	return 1;
}

void
mulcon1(Multab *m, long v, Node *nod1)
{
	Node *l, *r, *n, nod2;
	int o;
	char code[sizeof(m->code)+2], *p;

	memmove(code, m->code, sizeof(m->code));
	code[sizeof(m->code)] = 0;
	if(debug['M'] && debug['v'])
		print("%L multiply: %ld code %s\n", nod1->lineno, v, code);

	p = code;
	if(v < 0)
		gopcode(OSUB, nod1, nodconst(0), nod1);
	regalloc(&nod2, nod1, Z);

loop:
	switch(*p) {
	case 0:
		regfree(&nod2);
		return;
	case '+':
		o = OADD;
		goto addsub;
	case '-':
		o = OSUB;
	addsub:	/* number is r,n,l */
		v = p[1] - '0';
		r = nod1;
		if(v&4)
			r = &nod2;
		n = nod1;
		if(v&2)
			n = &nod2;
		l = nod1;
		if(v&1)
			l = &nod2;
		gopcode(o, l, n, r);
		break;
	default: /* op is shiftcount, number is r */
		v = p[1] - '0';
		r = nod1;
		if(v&1)
			r = &nod2;
		v = *p - 'a';
		if(v < 0 || v >= 32) {
			diag(nod1, "mulcon unknown op: %c%c", p[0], p[1]);
			break;
		}
		gopcode(OASHL, nodconst(v), Z, r);
		break;
	}
	p += 2;
	goto loop;
}

void
nullwarn(Node *l, Node *r)
{
	warn(Z, "result of operation not used");
	if(l != Z)
		cgen(l, Z);
	if(r != Z)
		cgen(r, Z);
}

void
sextern(Sym *s, Node *a, long o, long w)
{
	long e, lw;

	for(e=0; e<w; e+=NSNAME) {
		lw = NSNAME;
		if(w-e < lw)
			lw = w-e;
		gpseudo(ADATA, s, nodconst(0L));
		p->from.offset += o+e;
		p->reg = lw;
		p->to.type = D_SCONST;
		memmove(p->to.sval, a->cstring+e, lw);
	}
}

void
gextern(Sym *s, Node *a, long o, long w)
{
	gpseudo(ADATA, s, a);
	p->from.offset += o;
	p->reg = w;
	if(p->to.type == D_NAME)
		p->to.type = D_CONST;
}

static struct{
	Sym	*sym;
	short	type;
} symh[NSYM];
static int	sym;

void	zname(Biobuf*, char*, int, int);
void	zaddr(Biobuf*, Adr*, int);
void	zwrite(Biobuf*, Prog*, int, int, int, int);
void	outhist(Biobuf*);
int	outsim(Biobuf*, Sym*, int);

void
outcode(void)
{
	Prog *p;
	int f, sf, sf1, sf2, st;
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
		diag(Z, "cannot open %s", outfile);
		return;
	}
	Binit(&b, f, OWRITE);
	Bseek(&b, 0L, 2);
	outhist(&b);
	for(sym=0; sym<NSYM; sym++) {
		symh[sym].sym = S;
		symh[sym].type = 0;
	}
	sym = 1;
	for(p = firstp; p != P; p = p->link) {
		for(;;){
			sf1 = 0;
			sf2 = 0;
			sf = outsim(&b, p->from.sym, p->from.name);
			if(p->isf && p->from1.type != D_NONE)
				sf1 = outsim(&b, p->from1.sym, p->from1.name);
			if(p->isf && p->from2.type != D_NONE)
				sf1 = outsim(&b, p->from2.sym, p->from2.name);
			st = outsim(&b, p->to.sym, p->to.name);
			if(!sf || sf != sf1 && sf != sf2 && sf != st
			&& !sf1 || sf1 != sf2 && sf1 != st
			&& !sf2 || sf2 != st)
				break;
		}
		zwrite(&b, p, sf, sf1, sf2, st);
	}
	Bflush(&b);
	close(f);
	firstp = P;
	lastp = P;
}

void
zwrite(Biobuf *b, Prog *p, int sf, int sf1, int sf2, int st)
{
	long l;
	int n;

	Bputc(b, p->as);
	Bputc(b, p->reg);
	n = p->isf;
	if(n && p->from1.type != D_NONE || p->from2.type != D_NONE)
		n++;
	if(n && p->from2.type != D_NONE)
		n++;
	Bputc(b, p->cc | (n << 6));
	l = p->lineno;
	Bputc(b, l);
	Bputc(b, l>>8);
	Bputc(b, l>>16);
	Bputc(b, l>>24);
	zaddr(b, &p->from, sf);
	if(n > 1)
		zaddr(b, &p->from1, sf1);
	if(n > 2)
		zaddr(b, &p->from2, sf2);
	zaddr(b, &p->to, st);
}

void
outhist(Biobuf *b)
{
	Hist *h;
	char *p, *q, *op;
	Prog pg;
	int n;

	pg = zprog;
	pg.as = AHISTORY;
	for(h = hist; h != H; h = h->link) {
		p = h->name;
		op = 0;
		if(p && p[0] != '/' && h->offset == 0 && pathname && pathname[0] == '/') {
			op = p;
			p = pathname;
		}
		while(p) {
			q = utfrune(p, '/');
			if(q) {
				n = q-p;
				if(n == 0)
					n = 1;	/* leading "/" */
				q++;
			} else {
				n = strlen(p);
				q = 0;
			}
			if(n) {
				Bputc(b, ANAME);
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

		zwrite(b, &pg, 0, 0, 0, 0);
	}
}

void
zname(Biobuf *b, char *n, int t, int s)
{

	Bputc(b, ANAME);
	Bputc(b, t);	/* type */
	Bputc(b, s);	/* sym */
	while(*n) {
		Bputc(b, *n);
		n++;
	}
	Bputc(b, 0);
}

int
outsim(Biobuf *b, Sym *s, int t)
{
	int sno;

	if(s == S)
		return 0;
	sno = s->sym;
	if(sno < 0 || sno >= NSYM)
		sno = 0;
	if(symh[sno].type == t && symh[sno].sym == s)
		return sno;
	zname(b, s->name, t, sym);
	s->sym = sym;
	symh[sym].sym = s;
	symh[sym].type = t;
	sno = sym;
	sym++;
	if(sym >= NSYM)
		sym = 1;
	return sno;
}

void
zaddr(Biobuf *b, Adr *a, int s)
{
	Dsp e;
	long l;
	int i;
	char *n;

	Bputc(b, a->type);
	Bputc(b, a->reg);
	Bputc(b, s);
	Bputc(b, a->name);
	switch(a->type) {
	default:
		diag(Z, "unknown type %d in zaddr", a->type);

	case D_NONE:
	case D_REG:
	case D_INDREG:
	case D_FREG:
	case D_CREG:
	case D_INC:
	case D_DEC:
		break;

	case D_INCREG:
		Bputc(b, a->offset);
		break;

	case D_NAME:
	case D_OREG:
	case D_CONST:
	case D_BRANCH:
		l = a->offset;
		Bputc(b, l);
		Bputc(b, l>>8);
		Bputc(b, l>>16);
		Bputc(b, l>>24);
		break;

	case D_SCONST:
		n = a->sval;
		for(i=0; i<NSNAME; i++) {
			Bputc(b, *n);
			n++;
		}
		break;

	case D_FCONST:
		e = dtodsp(a->dval);
		Bputc(b, e);
		Bputc(b, e>>8);
		Bputc(b, e>>16);
		Bputc(b, e>>24);
		break;
	}
}

Dsp
dtodsp(double d)
{
	double f, in;
	long v;
	int exp, neg;

	f = d;
	neg = 0;
	if(d < 0.0){
		neg = 1;
		d = -d;
	}
	if(d == 0)
		return 0;

	/*
	 * frexp returns with d in [1/2, 1)
	 * we want Mbits bits in the mantissa,
	 * plus a bit to make d in [1, 2)
	 */
	d = frexp(d, &exp);
	exp--;
	d = modf(d * (1 << (Dspbits + 1)), &in);

	v = in;
	if(d >= 0.5)
		v++;
	if(v >= (1 << (Dspbits + 1))){
		v >>= 1;
		exp++;
	}
	if(neg){
		v = -v;
		/*
		 * normalize for hidden 0 bit
		 */
		if(v & (1 << Dspbits)){
			v <<= 1;
			exp--;
		}
	}
	if(v == 0 || exp < -(Dspbias-1)){
		diag(Z, "floating constant %g underflowed", f);
		return 0;
	}
	if(exp > Dspexp - Dspbias){
		diag(Z, "floating constant %g overflowed", f);
		if(neg)
			v = 0;
		else
			v = ~0;
		exp = Dspexp - Dspbias;
	}
	v &= Dspmask;
	v = (v << 8) | ((exp + Dspbias) & Dspexp);
	if(neg)
		v |= 0x80000000;
	return v;
}

char*
xOconv(int a)
{

	USED(a);
	return "**badO**";
}

int
endian(int w)
{

	USED(w);
	return 0;
}

int
passbypointer(int et)
{

	return typesuv[et];
}

int
argalign(long typewidth, long offset, int offsp)
{
	USED(typewidth,offset,offsp);
	return 0;
}

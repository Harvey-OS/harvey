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

void
doswit(Node *n)
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
	if(debug['W'])
	for(i=0; i<nc; i++)
		print("case %2d: = %.8lux\n", i, iq[i].val);
	if(def == 0)
		def = breakpc;
	for(i=0; i<nc-1; i++)
		if(iq[i].val == iq[i+1].val)
			diag(n, "duplicate cases in switch %ld", iq[i].val);
	swit1(iq, nc, def, n);
}

void
swit1(C1 *q, int nc, long def, Node *n)
{
	C1 *r;
	int i;
	Prog *sp;

	if(nc < 5) {
		for(i=0; i<nc; i++) {
			if(debug['W'])
				print("case = %.8lux\n", q->val);
			gopcode(OEQ, nodconst(q->val), Z, n);
			patch(p, q->label);
			q++;
		}
		gbranch(OGOTO);
		patch(p, def);
		return;
	}
	i = nc / 2;
	r = q+i;
	if(debug['W'])
		print("case > %.8lux\n", r->val);
	gopcode(OLT, nodconst(r->val), Z, n);

	sp = p;
	gbranch(OGOTO);
	p->as = ABE;
	patch(p, r->label);
	swit1(q, i, def, n);

	if(debug['W'])
		print("case < %.8lux\n", r->val);
	patch(sp, pc);
	swit1(r+1, nc-i-1, def, n);
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
	if(n2 != Z) {
		regalloc(n1, l, nn);
		reglcgen(n2, l, Z);
		regalloc(n3, l, Z);
		gopcode(OAS, n2, Z, n3);
		gopcode(OAS, n3, Z, n1);
	} else {
		regalloc(n1, l, nn);
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
	while(n > 0) {
		string[mnstring] = *s++;
		mnstring++;
		nstring++;
		if(mnstring >= NSNAME) {
			gpseudo(ADATA, symstring, nodconst(0L));
			p->from.offset += nstring - NSNAME;
			p->from.scale = NSNAME;
			p->to.type = D_SCONST;
			memmove(p->to.sval, string, NSNAME);
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

int
vlog(Node *n)
{
	ulong v;
	int i, l;

	if(n->op != OCONST)
		goto bad;
	if(!typechlp[n->type->etype])
		goto bad;
	i = 0;
	l = 0;
	for(v = n->offset; v; v >>= 1) {
		i++;
		if(v & 1) {
			if(l)
				goto bad;
			l = i;
		}
	}
	return l-1;

bad:
	return -1;
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
		p->from.scale = lw;
		p->to.type = D_SCONST;
		memmove(p->to.sval, a->us+e, lw);
	}
}

void
gextern(Sym *s, Node *a, long o, long w)
{
	gpseudo(ADATA, s, a);
	p->from.offset += o;
	p->from.scale = w;
	switch(p->to.type) {
	default:
		p->to.index = p->to.type;
		p->to.type = D_ADDR;
	case D_CONST:
	case D_FCONST:
	case D_ADDR:
		break;
	}
}

#include	<bio.h>
void	zname(Biobuf*, char*, int, int);
void	zaddr(Biobuf*, Adr*, int);
void	outhist(Biobuf*);

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
		diag(Z, "cannot open %s", outfile);
		return;
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
			t = p->from.type;
			if(t == D_ADDR)
				t = p->from.index;
			if(h[sf].type == t)
			if(h[sf].sym == s)
				break;
			zname(&b, s->name, t, sym);
			s->sym = sym;
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
			t = p->to.type;
			if(t == D_ADDR)
				t = p->to.index;
			if(h[st].type == t)
			if(h[st].sym == s)
				break;
			zname(&b, s->name, t, sym);
			s->sym = sym;
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
		Bputc(&b, p->as);
		Bputc(&b, p->lineno);
		Bputc(&b, p->lineno>>8);
		Bputc(&b, p->lineno>>16);
		Bputc(&b, p->lineno>>24);
		switch(t = p->type) {
		case D_NONE:
			t = 0;			/* 0 D_NONE */
			break;
		default:
			if(t < D_R0 || t >= D_R0+32) {
				diag(Z, "bad type in outcode");
				t = D_R0;
			}
			t = (t - D_R0) + 1;	/* 1-32 D_R0+ */
			break;
		case D_CONST:
			t = p->offset;
			if(t >= 32) {
				diag(Z, "bad offset in outcode");
				t = 0;
			}
			t = (t - 0) + 33;	/* 33-64 D_CONST+ */
			break;
		}
		Bputc(&b, t);
		zaddr(&b, &p->from, sf);
		zaddr(&b, &p->to, st);
	}
	Bflush(&b);
	close(f);
	firstp = P;
	lastp = P;
}

void
outhist(Biobuf *b)
{
	Hist *h;
	char name[NNAME], *p, *q;
	Prog pg;
	int n;

	pg = zprog;
	pg.as = AHISTORY;
	name[0] = '<';
	for(h = hist; h != H; h = h->link) {
		p = h->name;
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
			if(n >= NNAME-1)
				n = NNAME-2;
			if(n) {
				memmove(name+1, p, n);
				name[n+1] = 0;
				zname(b, name, D_FILE, 1);
			}
			p = q;
		}
		pg.lineno = h->line;
		pg.to.type = zprog.to.type;
		pg.to.offset = h->offset;
		if(h->offset)
			pg.to.type = D_CONST;

		Bputc(b, pg.as);
		Bputc(b, pg.lineno);
		Bputc(b, pg.lineno>>8);
		Bputc(b, pg.lineno>>16);
		Bputc(b, pg.lineno>>24);
		Bputc(b, 0);			/* reg */
		zaddr(b, &pg.from, 0);
		zaddr(b, &pg.to, 0);
	}
}

void
zname(Biobuf *b, char *n, int t, int s)
{


	Bputc(b, ANAME);	/* as */
	Bputc(b, t);		/* type */
	Bputc(b, s);		/* sym */
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
	if(a->index != D_NONE || a->scale != 0)
		t |= T_INDEX;
	if(s != 0)
		t |= T_SYM;

	switch(a->type) {
	default:
		t |= T_TYPE;
	case D_NONE:
		if(a->offset != 0)
			t |= T_OFFSET;
		break;
	case D_FCONST:
		t |= T_FCONST;
		break;
	case D_SCONST:
		t |= T_SCONST;
		break;
	}
	Bputc(b, t);

	if(t & T_INDEX) {	/* implies index, scale */
		Bputc(b, a->index);
		Bputc(b, a->scale);
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
	if(t & T_TYPE)
		Bputc(b, a->type);
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

char*	xonames[] =
{
	"INDEX",
	"XEND",
};

char*
xOconv(int a)
{
	if(a <= OEND || a > OXEND)
		return "**badO**";
	return xonames[a-OEND-1];
}

long
castto(long c, int f)
{

	switch(f) {
	case TCHAR:
		c &= 0xff;
		if(c & 0x80)
			c |= ~0xff;
		break;

	case TUCHAR:
		c &= 0xff;
		break;

	case TSHORT:
		c &= 0xffff;
		if(c & 0x8000)
			c |= ~0xffff;
		break;

	case TUSHORT:
		c &= 0xffff;
		break;
	}
	return c;
}

int
endian(int w)
{

	USED(w);
	return 0;
}

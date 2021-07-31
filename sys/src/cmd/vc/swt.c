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
	if(debug['W'])
	for(i=0; i<nc; i++)
		print("case %2d: = %.8lux\n", i, iq[i].val);
	if(def == 0)
		def = breakpc;
	for(i=0; i<nc-1; i++)
		if(iq[i].val == iq[i+1].val)
			diag(n, "duplicate cases in switch %ld", iq[i].val);
	regalloc(&tn, &regnode, Z);
	swit1(iq, nc, def, n, &tn);
	regfree(&tn);
}

void
swit1(C1 *q, int nc, long def, Node *n, Node *tn)
{
	C1 *r;
	int i;
	Prog *sp;

	if(nc < 5) {
		for(i=0; i<nc; i++) {
			if(debug['W'])
				print("case = %.8lux\n", q->val);
			gmove(nodconst(q->val), tn);
			gopcode(OEQ, n, tn, Z);
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
	gmove(nodconst(r->val), tn);
	gopcode(OLT, tn, n, Z);
	sp = p;
	gopcode(OEQ, n, tn, Z);
	patch(p, r->label);
	swit1(q, i, def, n, tn);

	if(debug['W'])
		print("case < %.8lux\n", r->val);
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
		p->reg = lw;
		p->to.type = D_SCONST;
		memmove(p->to.sval, a->us+e, lw);
	}
}

void
gextern(Sym *s, Node *a, long o, long w)
{
	gpseudo(ADATA, s, a);
	p->from.offset += o;
	p->reg = w;
	if(p->to.type == D_OREG)
		p->to.type = D_CONST;
}

#include	<bio.h>
void	zname(Biobuf*, char*, int, int);
char*	zaddr(char*, Adr*, int);
void	zwrite(Biobuf*, Prog*, int, int);
void	outhist(Biobuf*);

void
zwrite(Biobuf *b, Prog *p, int sf, int st)
{
	char bf[100], *bp;

	bf[0] = p->as;
	bf[1] = p->reg;
	bf[2] = p->lineno;
	bf[3] = p->lineno>>8;
	bf[4] = p->lineno>>16;
	bf[5] = p->lineno>>24;
	bp = zaddr(bf+6, &p->from, sf);
	bp = zaddr(bp, &p->to, st);
	Bwrite(b, bf, bp-bf);
}

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
			t = p->from.name;
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
			t = p->to.name;
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
		zwrite(&b, p, sf, st);
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

		zwrite(b, &pg, 0, 0);
	}
}

void
zname(Biobuf *b, char *n, int t, int s)
{
	char bf[NNAME+10], *bp;

	bp = bf;
	bp[0] = ANAME;
	bp[1] = t;	/* type */
	bp[2] = s;	/* sym */
	bp += 3;
	while(*n) {
		*bp++ = *n;
		n++;
	}
	*bp++ = 0;
	Bwrite(b, bf, bp-bf);
}

char*
zaddr(char *bp, Adr *a, int s)
{
	long l;
	Ieee e;

	bp[0] = a->type;
	bp[1] = a->reg;
	bp[2] = s;
	bp[3] = a->name;
	bp += 4;
	switch(a->type) {
	default:
		diag(Z, "unknown type %d in zaddr", a->type);

	case D_NONE:
	case D_REG:
	case D_FREG:
	case D_MREG:
	case D_FCREG:
	case D_LO:
	case D_HI:
		break;

	case D_OREG:
	case D_CONST:
	case D_BRANCH:
		l = a->offset;
		bp[0] = l;
		bp[1] = l>>8;
		bp[2] = l>>16;
		bp[3] = l>>24;
		bp += 4;
		break;

	case D_SCONST:
		memmove(bp, a->sval, NSNAME);
		bp += NSNAME;
		break;

	case D_FCONST:
		ieeedtod(&e, a->dval);
		l = e.l;
		bp[0] = l;
		bp[1] = l>>8;
		bp[2] = l>>16;
		bp[3] = l>>24;
		bp += 4;
		l = e.h;
		bp[0] = l;
		bp[1] = l>>8;
		bp[2] = l>>16;
		bp[3] = l>>24;
		bp += 4;
		break;
	}
	return bp;
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

char*
xOconv(int a)
{

	USED(a);
	return "**badO**";
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

	return tint->width - w;
}

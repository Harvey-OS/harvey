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
	long def;
	int nc, i;

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
			gopcode(OEQ, nodconst(q->val), n);
			gbranch(OEQ);
			patch(p, q->label);
			q++;
		}
		gbranch(OGOTO);
		patch(p, def);
		return;
	}
	i = nc / 2;
	r = q+i;
	gopcode(OGT, nodconst(r->val), n);
	gbranch(OGT);
	sp = p;
	swit1(r, nc-i, def, n);
	patch(sp, pc);
	swit1(q, i, def, n);
}

void
cas(void)
{
	Case *c;

	while(nhunk < sizeof(Case))
		gethunk();
	c = (Case *)hunk;
	nhunk -= sizeof(Case);
	hunk += sizeof(Case);
	c->link = cases;
	cases = c;
}

void
bitload(Node *b, Node *n1, Node *n2, Node *n3)
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
		regalloc(n1, l);
		curreg += SZ_LONG;
		regialloc(n2, l);
		curreg += SZ_LONG;
		lcgen(l, n2);
		regind(n2, l);
		regalloc(n3, l);
		curreg += SZ_LONG;
		gopcode(OAS, n2, n3);
		gopcode(OAS, n3, n1);
	} else {
		regalloc(n1, l);
		curreg += SZ_LONG;
		cgen(l, n1);
	}
	if(b->type->shift == 0 && typeu[b->type->etype]) {
		v = ~0 + (1L << b->type->nbits);
		gopcode(OAND, nodconst(v), n1);
	} else {
		sh = 32 - b->type->shift - b->type->nbits;
		if(sh > 0)
			gopcode(OASHL, nodconst(sh), n1);
		sh += b->type->shift;
		if(sh > 0)
			if(typeu[b->type->etype])
				gopcode(OLSHR, nodconst(sh), n1);
			else
				gopcode(OASHR, nodconst(sh), n1);
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
	regalloc(&nod, l);
	curreg += SZ_IND;
	v = ~0 + (1L << b->type->nbits);
	gopcode(OAND, nodconst(v), n1);
	gopcode(OAS, n1, &nod);
	if(nn != Z)
		gopcode(OAS, n1, nn);
	sh = b->type->shift;
	if(sh > 0)
		gopcode(OASHL, nodconst(sh), &nod);
	v <<= sh;
	gopcode(OAND, nodconst(~v), n3);
	gopcode(OOR, n3, &nod);
	gopcode(OAS, &nod, n2);
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
			p->from.offset = nstring - NSNAME;
			p->to.type = D_SCONST;
			p->to.width = NSNAME;
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

void
doinc(Node *n, int f)
{
	Node *l;
	long v;
	int a, o;

loop:
	if(n == Z)
		return;
	o = n->op;
	l = n->left;
	switch(o) {

	case OPREINC:
	case OPREDEC:
		if(n->addable >= INDEXED)
		if(f & PRE)
		if(l->type != T) {
			v = 1;
			if(l->type->etype == TIND)
				v = l->type->link->width;
			if(typefdv[l->type->etype])
				gopcode(o, nodfconst(v), l);
			else
				gopcode(o, nodconst(v), l);
		}
		break;

	case OPOSTINC:
	case OPOSTDEC:
		if(n->addable >= INDEXED)
		if(f & APOST)
		if(l->type != T) {
			v = 1;
			if(l->type->etype == TIND)
				v = l->type->link->width;
			if(typefdv[l->type->etype])
				gopcode(o, nodfconst(v), l);
			else
				gopcode(o, nodconst(v), l);
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
	case OASASHL:
	case OASASHR:
	case OASAND:
		a = n->addable;
		if(a >= INDEXED)
		if(f & PRE) {
			n->addable = 0;
			doinc(n, PRE);
			cgen(n, Z);
			n->addable = a;
			if(!(f & APOST))
				return;
			f &= ~PRE;
		}
		break;

	case ONAME:
	case OSTRING:
	case OCONST:
	case OREGISTER:
	case OINDREG:
		return;

	case OCOMMA:
		if(f & PRE)
			doinc(l, f);
		if(f & APOST) {
			n = n->right;
			goto loop;
		}
		return;

	case OCOND:
		return;

	case OANDAND:
	case OOROR:
		n = l;
		goto loop;
	}
	doinc(l, f);
	n = n->right;
	goto loop;
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
		p->to.type = D_SCONST;
		p->to.width = lw;
		memmove(p->to.sval, a->us+e, lw);
	}
}

void
gextern(Sym *s, Node *a, long o, long w)
{

	USED(w);
	gpseudo(ADATA, s, a);
	p->from.offset += o;
	p->from.width = twidth(a->type);
	if(p->from.width == W_NONE)
		p->from.width = W_W;
	if(p->to.type == D_SCONST)
		p->from.width = W_NONE;
	if(p->to.type == D_EXTERN || p->to.type == D_STATIC) {
		p->from.width = W_W;
		p->to.width = p->to.type - D_NONE;
		p->to.type = D_ADDR;
	}
}

#include	<bio.h>
void	zname(Biobuf*, char*, int, int);
void	zaddr(Biobuf*, Adr*, int);
void	zwrite(Biobuf*, Prog*, int, int);
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
			t = p->from.type & ~D_INDIR;
			if(t == D_ADDR)
				t = p->from.width + D_NONE;
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
			t = p->to.type & ~D_INDIR;
			if(t == D_ADDR)
				t = p->to.width + D_NONE;
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
zwrite(Biobuf *b, Prog *p, int sf, int st)
{
	long l;

	Bputc(b, p->as);
	l = p->lineno;
	Bputc(b, l);
	Bputc(b, l>>8);
	Bputc(b, l>>16);
	Bputc(b, l>>24);
	zaddr(b, &p->from, sf);
	zaddr(b, &p->to, st);
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

	Bputc(b, ANAME);
	Bputc(b, t);	/* type */
	Bputc(b, s);	/* sym */
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
	int i;
	char *n;
	Ieee e;

	Bputc(b, a->type);
	Bputc(b, a->width);
	Bputc(b, s);
	switch(a->type & ~D_INDIR) {
	default:
		diag(Z, "unknown type %d in zaddr", a->type);

	case D_NONE:
		break;

	case D_CONST:
	case D_BRANCH:
	case D_ADDR:
	case D_REG:
	case D_EXTERN:
	case D_STATIC:
	case D_AUTO:
	case D_PARAM:
		l = a->offset;
		Bputc(b, l);
		Bputc(b, l>>8);
		Bputc(b, l>>16);
		Bputc(b, l>>24);
		break;

	case D_SCONST:
		n = a->sval;
		l = a->width;
		Bputc(b, l);
		for(i=0; i<l; i++) {
			Bputc(b, *n);
			n++;
		}
		break;

	case D_FCONST:
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
		break;
	}
}

void
ieeedtod(Ieee *ieee, double native)
{
	double fr, ho, f;
	int exp;

	if(native < 0) {
		ieeedtod(ieee, -native);
		ieee->h |= 0x80000000UL;
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

	USED(w);
	return 0;
}

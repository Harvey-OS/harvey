#include <u.h>
#include <libc.h>
#include <bio.h>

enum {
	HistorySize=	32*1024,
	BufSize=	4*1024,
	MaxBits=	16,	/* maximum bits in a encoded code */
	Nlitlen=	288,	/* number of litlen codes */
	Noff=		32,	/* number of offset codes */
	Nclen=		19,	/* number of codelen codes */
	LenShift=	10,	/* code = len<<LenShift|code */
	LitlenBits=	9,	/* number of bits in litlen decode table */
	OffBits=	6,	/* number of bits in offset decode table */
	ClenBits=	6,	/* number of bits in code len decode table */
};

typedef struct Input	Input;
typedef struct History	History;
typedef struct Huff	Huff;

struct Input
{
	void	*wr;
	int	(*w)(void*, void*, int);
	void	*getr;
	int	(*get)(void*);
	ulong	sreg;
	int	nbits;
};

struct History
{
	uchar	his[HistorySize];
	uchar	*cp;	/* current pointer in history */
	int	full;	/* his has been filled up at least once */
};

struct Huff
{
	int	bits;	/* number of extra bits from last entry in the table */
	int	first;	/* first encoding at this bit length */
	int	last;	/* one past last encoding */
	short	*code;	/* last-first code entries */
};

void	inflateinit(void);
int	inflate(void *wr, int (*w)(void*, void*, int), void *getr, int (*get)(void*));

/* litlen code words 257-285 extra bits */
static int litlenextra[Nlitlen-257] =
{
/* 257 */	0, 0, 0,
/* 260 */	0, 0, 0, 0, 0, 1, 1, 1, 1, 2,
/* 270 */	2, 2, 2, 3, 3, 3, 3, 4, 4, 4,
/* 280 */	4, 5, 5, 5, 5, 0, 0, 0
};

static int litlenbase[Nlitlen-257];

/* offset code word extra bits */
static int offextra[Noff] =
{
	0,  0,  0,  0,  1,  1,  2,  2,  3,  3,
	4,  4,  5,  5,  6,  6,  7,  7,  8,  8,
	9,  9,  10, 10, 11, 11, 12, 12, 13, 13,
	0,  0,
};
static int offbase[Noff];

/* order code lengths */
static int clenorder[Nclen] =
{
        16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

/* for static huffman tables */
static	Huff	litlentab[MaxBits];
static	Huff	offtab[MaxBits];
static	uchar	revtab[256];

static int	uncblock(Input *in, History*);
static int	fixedblock(Input *in, History*);
static int	dynamicblock(Input *in, History*);
static int	sregfill(Input *in, int n);
static int	sregunget(Input *in);
static void	hufftabinit(Huff*, int*, int, int);
static int	decode(Input*, History*, Huff*, Huff*);
static void	mismatch(void);
static void	hufftabfree(Huff tab[MaxBits]);
static int	hufflookup(Input *in, Huff tab[]);

void
inflateinit(void)
{
	int litlen[Nlitlen];
	int off[Noff];
	int i, j, base;

	/* byte reverse table */
	for(i=0; i<256; i++)
		for(j=0; j<8; j++)
			if(i & (1<<j))
				revtab[i] |= 0x80 >> j;

	for(i=257,base=3; i<Nlitlen; i++) {
		litlenbase[i-257] = base;
		base += 1<<litlenextra[i-257];
	}
	/* strange table entry in spec... */
	litlenbase[285-257]--;

	for(i=0,base=1; i<Noff; i++) {
		offbase[i] = base;
		base += 1<<offextra[i];
	}

	/* static Litlen bit lengths */
	for(i=0; i<144; i++)
		litlen[i] = 8;
	for(i=144; i<256; i++)
		litlen[i] = 9;
	for(i=256; i<280; i++)
		litlen[i] = 7;
	for(i=280; i<Nlitlen; i++)
		litlen[i] = 8;

	hufftabinit(litlentab, litlen, Nlitlen, 7);

	/* static Offset bit lengths */
	for(i=0; i<Noff; i++)
		off[i] = 5;

	hufftabinit(offtab, off, Noff, OffBits);
}

int
inflate(void *wr, int (*w)(void*, void*, int), void *getr, int (*get)(void*))
{
	History his;
	Input in;
	int final, type;

	his.cp = his.his;
	his.full = 0;
	in.getr = getr;
	in.get = get;
	in.wr = wr;
	in.w = w;
	in.nbits = 0;
	in.sreg = 0;

	do {
		if(!sregfill(&in, 3))
			return 0;
		final = in.sreg & 0x1;
		type = (in.sreg>>1) & 0x3;
		in.sreg >>= 3;
		in.nbits -= 3;
		switch(type) {
		default:
			werrstr("illegal block type");
			return 0;
		case 0:
			/* uncompressed */
			if(!uncblock(&in, &his))
				return 0;
			break;
		case 1:
			/* fixed huffman */
			if(!fixedblock(&in, &his))
				return 0;
			break;
		case 2:
			/* dynamic huffman */
			if(!dynamicblock(&in, &his))
				return 0;
			break;
		}
	} while(!final);

	if(his.cp != his.his && (*w)(wr, his.his, his.cp - his.his) != his.cp - his.his)
		return 0;

	if(!sregunget(&in))
		return 0;

	return 1;
}

static int
uncblock(Input *in, History *his)
{
	int len, nlen, c;
	uchar *hs, *hp, *he;

	if(!sregunget(in))
		return 0;
	len = (*in->get)(in->getr);
	len |= (*in->get)(in->getr)<<8;
	nlen = (*in->get)(in->getr);
	nlen |= (*in->get)(in->getr)<<8;
	if(len != (~nlen&0xffff)) {
		werrstr("uncompressed block: bad len=%ux nlen=%ux", len, ~nlen&0xffff);
		return 0;
	}

	hp = his->cp;
	hs = his->his;
	he = hs + HistorySize;

	while(len > 0) {
		c = (*in->get)(in->getr);
		if(c < 0)
			return 0;
		*hp++ = c;
		if(hp == he) {
			his->full = 1;
			if((*in->w)(in->wr, hs, HistorySize) != HistorySize)
				return 0;
			hp = hs;
		}
		len--;
	}

	his->cp = hp;

	return 1;
}

static int
fixedblock(Input *in, History *his)
{
	return decode(in, his, litlentab, offtab);
}

static int
dynamicblock(Input *in, History *his)
{
	int i, j, n, c, code, nlit, ndist, nclen;
	int litlen[Nlitlen], off[Noff], clen[Nclen];
	Huff litlentab[MaxBits], offtab[MaxBits], clentab[MaxBits];
	int len[Nlitlen+Noff];
	int res;
	int nb;

	/* huff table header */
	memset(litlen, 0, sizeof(litlen));
	memset(off, 0, sizeof(off));
	memset(clen, 0, sizeof(clen));

	if(!sregfill(in, 14))
		return 0;
	nlit = (in->sreg&0x1f) + 257;
	ndist = ((in->sreg>>5) & 0x1f) + 1;
	nclen = ((in->sreg>>10) & 0xf) + 4;
	in->sreg >>= 14;
	in->nbits -= 14;

	if(nlit > Nlitlen || ndist > Noff || nlit < 257) {
		werrstr("bad huffman table header");
		return 0;
	}

	for(i=0; i<nclen; i++) {
		if(!sregfill(in, 3))
			return 0;
		clen[clenorder[i]] = in->sreg & 0x7;
		in->sreg >>= 3;
		in->nbits -= 3;
	}

	hufftabinit(clentab, clen, Nclen, ClenBits);

	n = nlit+ndist;
	for(i=0; i<n;) {
		nb = clentab[0].bits;
		if(in->nbits<nb && !sregfill(in, nb))
			return 0;
		c = clentab[0].code[in->sreg&((1<<nb)-1)];
		if(c == 0)
			c = hufflookup(in, clentab);
		if(c == 0)
			return 0;
		code = c & ((1<<LenShift)-1);
		nb = c>>LenShift;
		in->sreg >>= nb;
		in->nbits -= nb;

		if(code < 16) {
			j = 1;
			c = code;
		} else if(code == 16) {
			if(in->nbits<2 && !sregfill(in, 2))
				return 0;
			j = (in->sreg&0x3)+3;
			in->sreg >>= 2;
			in->nbits -= 2;
			if(i == 0) {
				werrstr("bad code len code 16");
				return 0;
			}
			c = len[i-1];
		} else if(code == 17) {
			if(in->nbits<3 && !sregfill(in, 3))
				return 0;
			j = (in->sreg&0x7)+3;
			in->sreg >>= 3;
			in->nbits -= 3;
			c = 0;
		} else if(code == 18) {
			if(in->nbits<7 && !sregfill(in, 7))
				return 0;
			j = (in->sreg&0x7f)+11;
			in->sreg >>= 7;
			in->nbits -= 7;
			c = 0;
		} else {
			werrstr("bad code len code");
			return 0;
		}

		if(i+j > n) {
			werrstr("bad code len code");
			return 0;
		}

		while(j) {
			len[i] = c;
			i++;
			j--;
		}
	}

	for(i=0; i<nlit; i++)
		litlen[i] = len[i];
	for(i=0; i<ndist; i++)
		off[i] = len[i+nlit];

	nb = litlen[256];
	if(nb > LitlenBits)
		nb = LitlenBits;

	hufftabinit(litlentab, litlen, nlit, nb);
	hufftabinit(offtab, off, ndist, OffBits);
	res = decode(in, his, litlentab, offtab);
	hufftabfree(litlentab);
	hufftabfree(offtab);
	hufftabfree(clentab);

	return res;
}

static int
decode(Input *in, History *his, Huff *litlentab, Huff *offtab)
{
	int len, off;
	uchar *hs, *hp, *hq, *he;
	int c, code;
	int nb;

	hs = his->his;
	he = hs + HistorySize;
	hp = his->cp;

	for(;;) {
		nb = litlentab[0].bits;
		if(in->nbits<nb && !sregfill(in, nb))
			return 0;
		c = litlentab[0].code[in->sreg&((1<<nb)-1)];
		if(c == 0)
			c = hufflookup(in, litlentab);
		if(c == 0)
			return 0;
		code = c & ((1<<LenShift)-1);
		nb = c>>LenShift;
		in->sreg >>= nb;
		in->nbits -= nb;

		if(code < 256) {
			/* literal */
			*hp++ = code;
			if(hp == he) {
				his->full = 1;
				if((*in->w)(in->wr, hs, HistorySize) != HistorySize)
					return 0;
				hp = hs;
			}
			continue;
		}

		if(code == 256)
			break;

		if(code > 285) {
			werrstr("illegal lit/len code");
			return 0;
		}

		code -= 257;
		nb = litlenextra[code];
		if(in->nbits < nb && !sregfill(in, nb))
			return 0;
		len = litlenbase[code] + (in->sreg & ((1<<nb)-1));
		in->sreg >>= nb;
		in->nbits -= nb;

		/* get offset */
		nb = offtab[0].bits;
		if(in->nbits<nb && !sregfill(in, nb))
			return 0;
		c = offtab[0].code[in->sreg&((1<<nb)-1)];
		if(c == 0)
			c = hufflookup(in, offtab);
		if(c == 0)
			return 0;
		code = c & ((1<<LenShift)-1);
		nb = c>>LenShift;
		in->sreg >>= nb;
		in->nbits -= nb;

		if(code > 29) {
			werrstr("illegal lit/len code");
			return 0;
		}

		nb = offextra[code];
		if(in->nbits < nb && !sregfill(in, nb))
			return 0;

		off = offbase[code] + (in->sreg & ((1<<nb)-1));
		in->sreg >>= nb;
		in->nbits -= nb;

		hq = hp - off;
		if(hq < hs) {
			if(!his->full) {
				werrstr("access before begining of history");
				return 0;
			}
			hq += HistorySize;
		}

		/* slow but correct */
		while(len) {
			*hp = *hq;
			hq++;
			hp++;
			if(hq >= he)
				hq = hs;
			if(hp == he) {
				his->full = 1;
				if((*in->w)(in->wr, hs, HistorySize) != HistorySize)
					return 0;
				hp = hs;
			}
			len--;
		}

	}

	his->cp = hp;

	return 1;
}

static void
hufftabinit(Huff *tab, int *bitlen, int n, int bits)
{
	int i, j, k, nc, bl, v, ec;
	int count[MaxBits];
	ulong encode[MaxBits];
	short *code[MaxBits];

	nc = 1<<bits;
	memset(tab, 0, sizeof(Huff)*MaxBits);
	memset(count, 0, sizeof(count));
	memset(encode, 0, sizeof(encode));
	memset(code, 0, sizeof(code));

	for(i=0; i<n; i++)
		count[bitlen[i]]++;
	/* ignore codes with 0 bits */
	count[0] = 0;
	for(i=1; i<MaxBits; i++)
		encode[i] = (encode[i-1] + count[i-1]) << 1;

	code[bits] = malloc(sizeof(short)*nc);
	memset(code[bits], 0, sizeof(short)*nc);
	for(i=bits+1; i<MaxBits; i++)
		if(count[i])
			code[i] = malloc(sizeof(short)*count[i]);
	memset(count, 0, sizeof(count));
	for(i=0; i<n; i++) {
		bl = bitlen[i];
		if(bl == 0)
			continue;
		v = i | (bl<<LenShift);
		if(bl <= bits) {
			/* shift encode up so it starts on bit 15 then reverse */
			ec = encode[bl] + count[bl];
			ec <<= (16-bl);
			ec = revtab[ec>>8] | (revtab[ec&0xff]<<8);
			k = 1<<bl;
			for(j=ec; j<nc; j+=k)
				code[bits][j] = (short)v;
		} else {
			code[bl][count[bl]] = (short)v;
		}
		count[bl]++;
	}
	/* pack into a the huff table and return */
	tab[0].bits = bits;
	tab[0].first = 0;
	tab[0].last = nc;
	tab[0].code = code[bits];
	for(i=bits+1,j=1; i<MaxBits; i++) {
		if(count[i] == 0)
			continue;
		tab[j].bits = i;
		tab[j].first = encode[i];
		tab[j].last = encode[i]+count[i];
		tab[j].code = code[i];
		j++;
	}
}

static int
hufflookup(Input *in, Huff tab[])
{
	int nb, ec;
	Huff *h;

	ec = 0;
	for(h=tab+1; h->bits; h++) {
		nb = h->bits;
		if(in->nbits<nb && !sregfill(in, nb))
			return 0;
		ec = revtab[in->sreg&0xff]<<8;
		ec |= revtab[(in->sreg>>8)&0xff];
		ec >>= (16-nb);
		if(ec < h->last)
			return h->code[ec-h->first];
	}
	werrstr("unknown huff code %x", ec);
	return 0;
}

static void
hufftabfree(Huff tab[MaxBits])
{
	int i;

	for(i=0; tab[i].bits>0; i++)
		free(tab[i].code);
}

static int
sregfill(Input *in, int n)
{
	int c;

	while(n > in->nbits) {
		c = (*in->get)(in->getr);
		if(c < 0){
			werrstr("unexpected end of input");
			return 0;
		}
		in->sreg |= c<<in->nbits;
		in->nbits += 8;
	}
	return 1;
}

static int
sregunget(Input *in)
{
	if(in->nbits >= 8) {
		werrstr("internal inflate error: too many bits read");
		return 0;
	}

	/* throw other bits on the floor */
	in->nbits = 0;
	in->sreg = 0;
	return 1;
}

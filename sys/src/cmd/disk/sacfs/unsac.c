#include <u.h>
#include <libc.h>
#include "sac.h"

typedef struct Huff	Huff;
typedef struct Mtf	Mtf;
typedef struct Decode	Decode;

enum
{
	ZBase		= 2,			/* base of code to encode 0 runs */
	LitBase		= ZBase-1,		/* base of literal values */
	MaxLit		= 256,

	MaxLeaf		= MaxLit+LitBase,
	MaxHuffBits	= 16,			/* max bits in a huffman code */
	MaxFlatbits	= 5,			/* max bits decoded in flat table */

	CombLog		= 4,
	CombSpace	= 1 << CombLog,		/* mtf speedup indices spacing */
	CombMask	= CombSpace - 1,
};

struct Mtf
{
	int	maxcomb;		/* index of last valid comb */
	uchar	prev[MaxLit];
	uchar	next[MaxLit];
	uchar	comb[MaxLit / CombSpace + 1];
};

struct Huff
{
	int	maxbits;
	int	flatbits;
	ulong	flat[1<<MaxFlatbits];
	ulong	maxcode[MaxHuffBits];
	ulong	last[MaxHuffBits];
	ulong	decode[MaxLeaf];
};

struct Decode{
	Huff	tab;
	Mtf	mtf;
	int	nbits;
	ulong	bits;
	int	nzero;
	int	base;
	ulong	maxblocksym;

	jmp_buf	errjmp;

	uchar	*src;				/* input buffer */
	uchar	*smax;				/* limit */
};

static	void	fatal(Decode *dec, char*);

static	int	hdec(Decode*);
static	void	recvtab(Decode*, Huff*, int, ushort*);
static	ulong	bitget(Decode*, int);
static	int	mtf(uchar*, int);

#define FORWARD 0

static void
mtflistinit(Mtf *m, uchar *front, int n)
{
	int last, me, f, i, comb;

	if(n == 0)
		return;

	/*
	 * add all entries to free list
	 */
	last = MaxLit - 1;
	for(i = 0; i < MaxLit; i++){
		m->prev[i] = last;
		m->next[i] = i + 1;
		last = i;
	}
	m->next[last] = 0;
	f = 0;

	/*
	 * pull valid entries off free list and enter into mtf list
	 */
	comb = 0;
	last = front[0];
	for(i = 0; i < n; i++){
		me = front[i];

		f = m->next[me];
		m->prev[f] = m->prev[me];
		m->next[m->prev[f]] = f;

		m->next[last] = me;
		m->prev[me] = last;
		last = me;
		if((i & CombMask) == 0)
			m->comb[comb++] = me;
	}

	/*
	 * pad out the list with dummies to the next comb,
	 * using free entries
	 */
	for(; i & CombMask; i++){
		me = f;

		f = m->next[me];
		m->prev[f] = m->prev[me];
		m->next[m->prev[f]] = f;

		m->next[last] = me;
		m->prev[me] = last;
		last = me;
	}
	me = front[0];
	m->next[last] = me;
	m->prev[me] = last;
	m->comb[comb] = me;
	m->maxcomb = comb;
}

static int
mtflist(Mtf *m, int pos)
{
	uchar *next, *prev, *mycomb;
	int c, c0, pc, nc, off;

	if(pos == 0)
		return m->comb[0];

	next = m->next;
	prev = m->prev;
	mycomb = &m->comb[pos >> CombLog];
	off = pos & CombMask;
	if(off >= CombSpace / 2){
		c = mycomb[1];
		for(; off < CombSpace; off++)
			c = prev[c];
	}else{
		c = *mycomb;
		for(; off; off--)
			c = next[c];
	}

	nc = next[c];
	pc = prev[c];
	prev[nc] = pc;
	next[pc] = nc;

	for(; mycomb > m->comb; mycomb--)
		*mycomb = prev[*mycomb];
	c0 = *mycomb;
	*mycomb = c;
	mycomb[m->maxcomb] = c;

	next[c] = c0;
	pc = prev[c0];
	prev[c] = pc;
	prev[c0] = c;
	next[pc] = c;
	return c;
}

static void
hdecblock(Decode *dec, ulong n, ulong I, uchar *buf, ulong *sums, ulong *prev)
{
	ulong i, nn, sum;
	int m, z, zz, c;

	nn = I;
	n--;
	i = 0;
again:
	for(; i < nn; i++){
		while((m = hdec(dec)) == 0 && i + dec->nzero < n)
			;
		if(z = dec->nzero){
			dec->nzero = 0;
			c = dec->mtf.comb[0];
			sum = sums[c];
			sums[c] = sum + z;

			z += i;
			zz = z;
			if(i < I && z > I){
				zz = I;
				z++;
			}

		zagain:
			for(; i < zz; i++){
				buf[i] = c;
				prev[i] = sum++;
			}
			if(i != z){
				zz = z;
				nn = ++n;
				i++;
				goto zagain;
			}
			if(i == nn){
				if(i == n)
					return;
				nn = ++n;
				i++;
			}
		}

		c = mtflist(&dec->mtf, m);

		buf[i] = c;
		sum = sums[c];
		prev[i] = sum++;
		sums[c] = sum;

	}
	if(i == n)
		return;
	nn = ++n;
	i++;
	goto again;
}

int
unsac(uchar *dst, uchar *src, int n, int nsrc)
{
	Decode *dec;
	uchar *buf, *front;
	ulong *prev, *sums;
	ulong sum, i, I;
	int m, j, c;

	dec = malloc(sizeof *dec);
	buf = malloc(n+2);
	prev = malloc((n+2) * sizeof *prev);
	front = malloc(MaxLit * sizeof *front);
	sums = malloc(MaxLit * sizeof *sums);

	if(dec == nil || buf == nil || prev == nil || front == nil || sums == nil || setjmp(dec->errjmp)){
		free(dec);
		free(buf);
		free(prev);
		free(front);
		free(sums);
		return -1;
	}

	dec->src = src;
	dec->smax = src + nsrc;

	dec->nbits = 0;
	dec->bits = 0;
	dec->nzero = 0;
	for(i = 0; i < MaxLit; i++)
		front[i] = i;

	n++;
	I = bitget(dec, 16);
	if(I >= n)
		fatal(dec, "corrupted input");

	/*
	 * decode the character usage map
	 */
	for(i = 0; i < MaxLit; i++)
		sums[i] = 0;
	c = bitget(dec, 1);
	for(i = 0; i < MaxLit; ){
		m = bitget(dec, 8) + 1;
		while(m--){
			if(i >= MaxLit)
				fatal(dec, "corrupted char map");
			front[i++] = c;
		}
		c = c ^ 1;
	}

	/*
	 * initialize mtf state
	 */
	c = 0;
	for(i = 0; i < MaxLit; i++)
		if(front[i])
			front[c++] = i;
	mtflistinit(&dec->mtf, front, c);
	dec->maxblocksym = c + LitBase;

	/*
	 * huffman decoding, move to front decoding,
	 * along with character counting
	 */
	dec->base = 1;
	recvtab(dec, &dec->tab, MaxLeaf, nil);
	hdecblock(dec, n, I, buf, sums, prev);

	sum = 1;
	for(i = 0; i < MaxLit; i++){
		c = sums[i];
		sums[i] = sum;
		sum += c;
	}

	i = 0;
	for(j = n - 2; j >= 0; j--){
		if(i > n || i < 0 || i == I)
			fatal(dec, "corrupted data");
		c = buf[i];
		dst[j] = c;
		i = prev[i] + sums[c];
	}

	free(dec);
	free(buf);
	free(prev);
	free(front);
	free(sums);
	return n;
}

static ulong
bitget(Decode *dec, int nb)
{
	int c;

	while(dec->nbits < nb){
		if(dec->src >= dec->smax)
			fatal(dec, "premature eof 1");
		c = *dec->src++;
		dec->bits <<= 8;
		dec->bits |= c;
		dec->nbits += 8;
	}
	dec->nbits -= nb;
	return (dec->bits >> dec->nbits) & ((1 << nb) - 1);
}

static void
fillbits(Decode *dec)
{
	int c;

	while(dec->nbits < 24){
		if(dec->src >= dec->smax)
			fatal(dec, "premature eof 2");
		c = *dec->src++;
		dec->bits <<= 8;
		dec->bits |= c;
		dec->nbits += 8;
	}
}

/*
 * decode one symbol
 */
static int
hdecsym(Decode *dec, Huff *h, int b)
{
	long c;
	ulong bits;
	int nbits;

	bits = dec->bits;
	nbits = dec->nbits;
	for(; (c = bits >> (nbits - b)) > h->maxcode[b]; b++)
		;
	if(b > h->maxbits)
		fatal(dec, "too many bits consumed");
	dec->nbits = nbits - b;
	return h->decode[h->last[b] - c];
}

static int
hdec(Decode *dec)
{
	ulong c;
	int nbits, nb;

	if(dec->nbits < dec->tab.maxbits)
		fillbits(dec);
	nbits = dec->nbits;
	dec->bits &= (1 << nbits) - 1;
	c = dec->tab.flat[dec->bits >> (nbits - dec->tab.flatbits)];
	nb = c & 0xff;
	c >>= 8;
	if(nb == 0xff)
		c = hdecsym(dec, &dec->tab, c);
	else
		dec->nbits = nbits - nb;

	/*
	 * reverse funny run-length coding
	 */
	if(c < ZBase){
		dec->nzero += dec->base << c;
		dec->base <<= 1;
		return 0;
	}

	dec->base = 1;
	c -= LitBase;
	return c;
}

static void
hufftab(Decode *dec, Huff *h, char *hb, ulong *bitcount, int maxleaf, int maxbits, int flatbits)
{
	ulong c, mincode, code, nc[MaxHuffBits];
	int i, b, ec;

	h->maxbits = maxbits;
	if(maxbits < 0)
		return;

	code = 0;
	c = 0;
	for(b = 0; b <= maxbits; b++){
		h->last[b] = c;
		c += bitcount[b];
		mincode = code << 1;
		nc[b] = mincode;
		code = mincode + bitcount[b];
		if(code > (1 << b))
			fatal(dec, "corrupted huffman table");
		h->maxcode[b] = code - 1;
		h->last[b] += code - 1;
	}
	if(code != (1 << maxbits))
		fatal(dec, "huffman table not full");
	if(flatbits > maxbits)
		flatbits = maxbits;
	h->flatbits = flatbits;

	b = 1 << flatbits;
	for(i = 0; i < b; i++)
		h->flat[i] = ~0;

	/*
	 * initialize the flat table to include the minimum possible
	 * bit length for each code prefix
	 */
	for(b = maxbits; b > flatbits; b--){
		code = h->maxcode[b];
		if(code == -1)
			break;
		mincode = code + 1 - bitcount[b];
		mincode >>= b - flatbits;
		code >>= b - flatbits;
		for(; mincode <= code; mincode++)
			h->flat[mincode] = (b << 8) | 0xff;
	}

	for(i = 0; i < maxleaf; i++){
		b = hb[i];
		if(b == -1)
			continue;
		c = nc[b]++;
		if(b <= flatbits){
			code = (i << 8) | b;
			ec = (c + 1) << (flatbits - b);
			if(ec > (1<<flatbits))
				fatal(dec, "flat code too big");
			for(c <<= (flatbits - b); c < ec; c++)
				h->flat[c] = code;
		}else{
			c = h->last[b] - c;
			if(c >= maxleaf)
				fatal(dec, "corrupted huffman table");
			h->decode[c] = i;
		}
	}
}

static void
elimBit(int b, char *tmtf, int maxbits)
{
	int bb;

	for(bb = 0; bb < maxbits; bb++)
		if(tmtf[bb] == b)
			break;
	while(++bb <= maxbits)
		tmtf[bb - 1] = tmtf[bb];
}

static int
elimBits(int b, ulong *bused, char *tmtf, int maxbits)
{
	int bb, elim;

	if(b < 0)
		return 0;

	elim = 0;

	/*
	 * increase bits counts for all descendants
	 */
	for(bb = b + 1; bb < maxbits; bb++){
		bused[bb] += 1 << (bb - b);
		if(bused[bb] == (1 << bb)){
			elim++;
			elimBit(bb, tmtf, maxbits);
		}
	}

	/*
	 * steal bits from parent & check for fullness
	 */
	for(; b >= 0; b--){
		bused[b]++;
		if(bused[b] == (1 << b)){
			elim++;
			elimBit(b, tmtf, maxbits);
		}
		if((bused[b] & 1) == 0)
			break;
	}
	return elim;
}

static void
recvtab(Decode *dec, Huff *tab, int maxleaf, ushort *map)
{
	ulong bitcount[MaxHuffBits+1], bused[MaxHuffBits+1];
	char tmtf[MaxHuffBits+1], *hb;
	int i, b, ttb, m, maxbits, max, elim;

	hb = malloc(MaxLeaf * sizeof *hb);
	if(hb == nil)
		fatal(dec, "out of memory");

	/*
	 * read the tables for the tables
	 */
	max = 8;
	for(i = 0; i <= MaxHuffBits; i++){
		bitcount[i] = 0;
		tmtf[i] = i;
		bused[i] = 0;
	}
	tmtf[0] = -1;
	tmtf[max] = 0;
	elim = 0;
	maxbits = -1;
	for(i = 0; i <= MaxHuffBits && elim != max; i++){
		ttb = 4;
		while(max - elim < (1 << (ttb-1)))
			ttb--;
		b = bitget(dec, ttb);
		if(b > max - elim)
			fatal(dec, "corrupted huffman table table");
		b = tmtf[b];
		hb[i] = b;
		bitcount[b]++;
		if(b > maxbits)
			maxbits = b;

		elim += elimBits(b, bused, tmtf, max);
	}
	if(elim != max)
		fatal(dec, "incomplete huffman table table");
	hufftab(dec, tab, hb, bitcount, i, maxbits, MaxFlatbits);
	for(i = 0; i <= MaxHuffBits; i++){
		tmtf[i] = i;
		bitcount[i] = 0;
		bused[i] = 0;
	}
	tmtf[0] = -1;
	tmtf[MaxHuffBits] = 0;
	elim = 0;
	maxbits = -1;
	for(i = 0; i < maxleaf && elim != MaxHuffBits; i++){
		if(dec->nbits <= tab->maxbits)
			fillbits(dec);
		dec->bits &= (1 << dec->nbits) - 1;
		m = tab->flat[dec->bits >> (dec->nbits - tab->flatbits)];
		b = m & 0xff;
		m >>= 8;
		if(b == 0xff)
			m = hdecsym(dec, tab, m);
		else
			dec->nbits -= b;
		b = tmtf[m];
		for(; m > 0; m--)
			tmtf[m] = tmtf[m-1];
		tmtf[0] = b;

		if(b > MaxHuffBits)
			fatal(dec, "bit length too big");
		m = i;
		if(map != nil)
			m = map[m];
		hb[m] = b;
		bitcount[b]++;
		if(b > maxbits)
			maxbits = b;
		elim += elimBits(b, bused, tmtf, MaxHuffBits);
	}
	if(elim != MaxHuffBits && elim != 0)
		fatal(dec, "incomplete huffman table");
	if(map != nil)
		for(; i < maxleaf; i++)
			hb[map[i]] = -1;

	hufftab(dec, tab, hb, bitcount, i, maxbits, MaxFlatbits);

	free(hb);
}

static void
fatal(Decode *dec, char *msg)
{
	print("%s: %s\n", argv0, msg);
	longjmp(dec->errjmp, 1);
}

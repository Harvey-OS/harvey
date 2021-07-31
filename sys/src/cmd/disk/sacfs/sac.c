#include <u.h>
#include <libc.h>
#include <bio.h>
#include "sac.h"
#include "ssort.h"

typedef struct Chain	Chain;
typedef struct Chains	Chains;
typedef struct Huff	Huff;

enum
{
	ZBase		= 2,			/* base of code to encode 0 runs */
	LitBase		= ZBase-1,		/* base of literal values */
	MaxLit		= 256,

	MaxLeaf		= MaxLit+LitBase,

	MaxHuffBits	= 16,			/* limit of bits in a huffman code */
	ChainMem	= 2 * (MaxHuffBits - 1) * MaxHuffBits,
};

/*
 * huffman code table
 */
struct Huff
{
	short	bits;				/* length of the code */
	ulong	encode;				/* the code */
};

struct Chain
{
	ulong	count;				/* occurances of everything in the chain */
	ushort	leaf;				/* leaves to the left of chain, or leaf value */
	char	col;				/* ref count for collecting unused chains */
	char	gen;				/* need to generate chains for next lower level */
	Chain	*up;				/* Chain up in the lists */
};

struct Chains
{
	Chain	*lists[(MaxHuffBits - 1) * 2];
	ulong	leafcount[MaxLeaf];		/* sorted list of leaf counts */
	ushort	leafmap[MaxLeaf];		/* map to actual leaf number */
	int	nleaf;				/* number of leaves */
	Chain	chains[ChainMem];
	Chain	*echains;
	Chain	*free;
	char	col;
	int	nlists;
};

static	jmp_buf	errjmp;

static	uchar	*bout;
static	int	bmax;
static	int	bpos;

static	ulong	leafcount[MaxLeaf];
static	ulong	nzero;
static	ulong	*hbuf;
static	int	hbpos;
static	ulong	nzero;
static	ulong	maxblocksym;

static	void	henc(int);
static	void	hbflush(void);

static	void	mkprecode(Huff*, ulong *, int, int);
static	ulong	sendtab(Huff *tab, int maxleaf, ushort *map);
static	int	elimBits(int b, ulong *bused, char *tmtf, int maxbits);
static	void	elimBit(int b, char *tmtf, int maxbits);
static	void	nextchain(Chains*, int);
static	void	hufftabinit(Huff*, int, ulong*, int);
static	void	leafsort(ulong*, ushort*, int, int);

static	void	bitput(int, int);

static	int	mtf(uchar*, int);
static	void	fatal(char*, ...);

static	ulong	headerbits;
static	ulong	charmapbits;
static	ulong	hufftabbits;
static	ulong	databits;
static	ulong	nbits;
static	ulong	bits;

int
sac(uchar *dst, uchar *src, int n)
{
	uchar front[256];
	ulong *perm, cmap[256];
	long i, c, rev;

	rev = 0;

	perm = nil;

	if(setjmp(errjmp)){
		free(perm);
		if(rev){
			for(i = 0; i < n/2; i++){
				c = src[i];
				src[i] = src[n-i-1];
				src[n-i-1] = c;
			}
		}
		return -1;
	}

	/*
	 * set up the output buffer
	 */
	bout = dst;
	bmax = n;
	bpos = 0;

	perm = malloc((n+1)*sizeof *perm);
	if(perm == nil)
		return -1;

	hbuf = perm;
	hbpos = 0;

	nbits = 0;
	nzero = 0;
	for(i = 0; i < MaxLeaf; i++)
		leafcount[i] = 0;

	if(rev){
		for(i = 0; i < n/2; i++){
			c = src[i];
			src[i] = src[n-i-1];
			src[n-i-1] = c;
		}
	}

	i = ssortbyte(src, (int*)perm, nil, n);

	headerbits += 16;
	bitput(i, 16);

	/*
	 * send the map of chars which occur in this block
	 */
	for(i = 0; i < 256; i++)
		cmap[i] = 0;
	for(i = 0; i < n; i++)
		cmap[src[i]] = 1;
	charmapbits += 1;
	bitput(cmap[0] != 0, 1);
	for(i = 0; i < 256; i = c){
		for(c = i + 1; c < 256 && (cmap[c] != 0) == (cmap[i] != 0); c++)
			;
		charmapbits += 8;
		bitput(c - i - 1, 8);
	}

	/*
	 * initialize mtf state
	 */
	c = 0;
	for(i = 0; i < 256; i++){
		if(cmap[i]){
			cmap[i] = c;
			front[c] = i;
			c++;
		}
	}
	maxblocksym = c + LitBase;

	for(i = 0; i <= n; i++){
		c = perm[i] - 1;
		if(c < 0)
			continue;
		henc(mtf(front, src[c]));
	}

	hbflush();
	bitput(0, 24);
	bitput(0, 8 - nbits);

	free(perm);
	return bpos;
}

static void
bitput(int c, int n)
{
	bits <<= n;
	bits |= c;
	for(nbits += n; nbits >= 8; nbits -= 8){
		c = bits << (32 - nbits);

		if(bpos >= bmax)
			longjmp(errjmp, 1);
		bout[bpos++] = c >> 24;
	}
}

static int
mtf(uchar *front, int c)
{
	int i, v;

	for(i = 0; front[i] != c; i++)
		;
	for(v = i; i > 0; i--)
		front[i] = front[i - 1];
	front[0] = c;
	return v;
}

/*
 * encode a run of zeros
 * convert to funny run length coding:
 * add one, omit implicit top 1 bit.
 */
static void
zenc(void)
{
	int m;

	nzero++;
	while(nzero != 1){
		m = nzero & 1;
		hbuf[hbpos++] = m;
		leafcount[m]++;
		nzero >>= 1;
	}
	nzero = 0;
}

/*
 * encode one symbol
 */
static void
henc(int c)
{
	if(c == 0){
		nzero++;
		return;
	}

	if(nzero)
		zenc();
	c += LitBase;
	leafcount[c]++;

	hbuf[hbpos++] = c;
}

static void
hbflush(void)
{
	Huff tab[MaxLeaf];
	int i, b, s;

	if(nzero)
		zenc();
	if(hbpos == 0)
		return;

	mkprecode(tab, leafcount, maxblocksym, MaxHuffBits);

	hufftabbits += sendtab(tab, maxblocksym, nil);

	/*
	 * send the data
	 */
	for(i = 0; i < hbpos; i++){
		s = hbuf[i];
		b = tab[s].bits;
		if(b == 0)
			fatal("bad tables %d", i);
		databits += b;
		bitput(tab[s].encode, b);
	}
}

static ulong
sendtab(Huff *tab, int maxleaf, ushort *map)
{
	ulong ccount[MaxHuffBits+1], bused[MaxHuffBits+1];
	Huff codetab[MaxHuffBits+1];
	char tmtf[MaxHuffBits+1];
	ulong bits;
	int i, b, max, ttb, s, elim;

	/*
	 * make up the table table
	 * over cleverness: the data is known to be a huffman table
	 * check for fullness at each bit level, and wipe it out from
	 * the possibilities;  when nothing exists except 0, stop.
	 */
	for(i = 0; i <= MaxHuffBits; i++){
		tmtf[i] = i;
		ccount[i] = 0;
		bused[i] = 0;
	}
	tmtf[0] = -1;
	tmtf[MaxHuffBits] = 0;

	elim = 0;
	for(i = 0; i < maxleaf && elim != MaxHuffBits; i++){
		b = i;
		if(map)
			b = map[b];
		b = tab[b].bits;
		for(s = 0; tmtf[s] != b; s++)
			if(s >= MaxHuffBits)
				fatal("bitlength not found");
		ccount[s]++;
		for(; s > 0; s--)
			tmtf[s] = tmtf[s-1];
		tmtf[0] = b;

		elim += elimBits(b, bused, tmtf, MaxHuffBits);
	}
	if(elim != MaxHuffBits && elim != 0)
		fatal("incomplete huffman table");

	/*
	 * make up and send the table table
	 */
	max = 8;
	mkprecode(codetab, ccount, MaxHuffBits+1, max);
	for(i = 0; i <= max; i++){
		tmtf[i] = i;
		bused[i] = 0;
	}
	tmtf[0] = -1;
	tmtf[max] = 0;
	elim = 0;
	bits = 0;
	for(i = 0; i <= MaxHuffBits && elim != max; i++){
		b = codetab[i].bits;
		for(s = 0; tmtf[s] != b; s++)
			if(s > max)
				fatal("bitlength not found");
		ttb = 4;
		while(max - elim < (1 << (ttb-1)))
			ttb--;
		bits += ttb;
		bitput(s, ttb);

		elim += elimBits(b, bused, tmtf, max);
	}
	if(elim != max)
		fatal("incomplete huffman table table");

	/*
	 * send the table
	 */
	for(i = 0; i <= MaxHuffBits; i++){
		tmtf[i] = i;
		bused[i] = 0;
	}
	tmtf[0] = -1;
	tmtf[MaxHuffBits] = 0;
	elim = 0;
	for(i = 0; i < maxleaf && elim != MaxHuffBits; i++){
		b = i;
		if(map)
			b = map[b];
		b = tab[b].bits;
		for(s = 0; tmtf[s] != b; s++)
			if(s >= MaxHuffBits)
				fatal("bitlength not found");
		ttb = codetab[s].bits;
		if(ttb < 0)
			fatal("bad huffman table table");
		bits += ttb;
		bitput(codetab[s].encode, ttb);
		for(; s > 0; s--)
			tmtf[s] = tmtf[s-1];
		tmtf[0] = b;

		elim += elimBits(b, bused, tmtf, MaxHuffBits);
	}
	if(elim != MaxHuffBits && elim != 0)
		fatal("incomplete huffman table");

	return bits;
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
elimBit(int b, char *tmtf, int maxbits)
{
	int bb;

	for(bb = 0; bb < maxbits; bb++)
		if(tmtf[bb] == b)
			break;
	if(tmtf[bb] != b)
		fatal("elim bit not found");
	while(++bb <= maxbits)
		tmtf[bb - 1] = tmtf[bb];
}

/*
 * fast?, in-place huffman codes
 *
 * A. Moffat, J. Katajainen, "In-Place Calculation of Minimum-Redundancy Codes",
 *
 * counts must be sorted, and may be aliased to bitlens
 */
static int
fmkprecode(ulong *bitcount, ulong *bitlen, ulong *counts, int n, int maxbits)
{
	int leaf, tree, treet, depth, nodes, internal;

	/*
	 * form the internal tree structure:
	 * [0, tree) are parent pointers,
	 * [tree, treet) are weights of internal nodes,
	 * [leaf, n) are weights of remaining leaves.
	 *
	 * note that at the end, there are n-1 internal nodes
	 */
	leaf = 0;
	tree = 0;
	for(treet = 0; treet != n - 1; treet++){
		if(leaf >= n || tree < treet && bitlen[tree] < counts[leaf]){
			bitlen[treet] = bitlen[tree];
			bitlen[tree++] = treet;
		}else
			bitlen[treet] = counts[leaf++];
		if(leaf >= n || tree < treet && bitlen[tree] < counts[leaf]){
			bitlen[treet] += bitlen[tree];
			bitlen[tree++] = treet;
		}else
			bitlen[treet] += counts[leaf++];
	}
	if(tree != treet - 1)
		fatal("bad fast mkprecode");

	/*
	 * calculate depth of internal nodes
	 */
	bitlen[n - 2] = 0;
	for(tree = n - 3; tree >= 0; tree--)
		bitlen[tree] = bitlen[bitlen[tree]] + 1;

	/*
	 * calculate depth of leaves:
	 * at each depth, leaves(depth) = nodes(depth) - internal(depth)
	 * and nodes(depth+1) = 2 * internal(depth)
	 */
	leaf = n;
	tree = n - 2;
	depth = 0;
	for(nodes = 1; nodes > 0; nodes = 2 * internal){
		internal = 0;
		while(tree >= 0 && bitlen[tree] == depth){
			tree--;
			internal++;
		}
		nodes -= internal;
		if(depth < maxbits)
			bitcount[depth] = nodes;
		while(nodes-- > 0)
			bitlen[--leaf] = depth;
		depth++;
	}
	if(leaf != 0 || tree != -1)
		fatal("bad leaves in fast mkprecode");

	return depth - 1;
}

/*
 * fast, low space overhead algorithm for max depth huffman type codes
 *
 * J. Katajainen, A. Moffat and A. Turpin, "A fast and space-economical
 * algorithm for length-limited coding," Proc. Intl. Symp. on Algorithms
 * and Computation, Cairns, Australia, Dec. 1995, Lecture Notes in Computer
 * Science, Vol 1004, J. Staples, P. Eades, N. Katoh, and A. Moffat, eds.,
 * pp 12-21, Springer Verlag, New York, 1995.
 */
static void
mkprecode(Huff *tab, ulong *count, int n, int maxbits)
{
	Chains cs;
	Chain *c;
	ulong bitcount[MaxHuffBits];
	int i, m, em, bits;

	/*
	 * set up the sorted list of leaves
	 */
	m = 0;
	for(i = 0; i < n; i++){
		tab[i].bits = -1;
		tab[i].encode = 0;
		if(count[i] != 0){
			cs.leafcount[m] = count[i];
			cs.leafmap[m] = i;
			m++;
		}
	}
	if(m < 2){
		if(m != 0){
			m = cs.leafmap[0];
			tab[m].bits = 0;
			tab[m].encode = 0;
		}
		return;
	}
	cs.nleaf = m;
	leafsort(cs.leafcount, cs.leafmap, 0, m);

	/*
	 * try fast code
	 */
	bits = fmkprecode(bitcount, cs.leafcount, cs.leafcount, m, maxbits);
	if(bits < maxbits){
		for(i = 0; i < m; i++)
			tab[cs.leafmap[i]].bits = cs.leafcount[i];
		bitcount[0] = 0;
	}else{
		for(i = 0; i < m; i++)
			cs.leafcount[i] = count[cs.leafmap[i]];

		/*
		 * set up free list
		 */
		cs.free = &cs.chains[2];
		cs.echains = &cs.chains[ChainMem];
		cs.col = 1;

		/*
		 * initialize chains for each list
		 */
		c = &cs.chains[0];
		c->count = cs.leafcount[0];
		c->leaf = 1;
		c->col = cs.col;
		c->up = nil;
		c->gen = 0;
		cs.chains[1] = cs.chains[0];
		cs.chains[1].leaf = 2;
		cs.chains[1].count = cs.leafcount[1];
		for(i = 0; i < maxbits-1; i++){
			cs.lists[i * 2] = &cs.chains[0];
			cs.lists[i * 2 + 1] = &cs.chains[1];
		}

		cs.nlists = 2 * (maxbits - 1);
		m = 2 * m - 2;
		for(i = 2; i < m; i++)
			nextchain(&cs, cs.nlists - 2);

		bits = 0;
		bitcount[0] = cs.nleaf;
		for(c = cs.lists[cs.nlists - 1]; c != nil; c = c->up){
			m = c->leaf;
			bitcount[bits++] -= m;
			bitcount[bits] = m;
		}
		m = 0;
		for(i = bits; i >= 0; i--)
			for(em = m + bitcount[i]; m < em; m++)
				tab[cs.leafmap[m]].bits = i;
	}
	hufftabinit(tab, n, bitcount, bits);
}

/*
 * calculate the next chain on the list
 * we can always toss out the old chain
 */
static void
nextchain(Chains *cs, int list)
{
	Chain *c, *oc;
	int i, nleaf, sumc;

	oc = cs->lists[list + 1];
	cs->lists[list] = oc;
	if(oc == nil)
		return;

	/*
	 * make sure we have all chains needed to make sumc
	 * note it is possible to generate only one of these,
	 * use twice that value for sumc, and then generate
	 * the second if that preliminary sumc would be chosen.
	 * however, this appears to be slower on current tests
	 */
	if(oc->gen){
		nextchain(cs, list - 2);
		nextchain(cs, list - 2);
		oc->gen = 0;
	}

	/*
	 * pick up the chain we're going to add;
	 * collect unused chains no free ones are left
	 */
	for(c = cs->free; ; c++){
		if(c >= cs->echains){
			cs->col++;
			for(i = 0; i < cs->nlists; i++)
				for(c = cs->lists[i]; c != nil; c = c->up)
					c->col = cs->col;
			c = cs->chains;
		}
		if(c->col != cs->col)
			break;
	}

	/*
	 * pick the cheapest of
	 * 1) the next package from the previous list
	 * 2) the next leaf
	 */
	nleaf = oc->leaf;
	sumc = 0;
	if(list > 0 && cs->lists[list-1] != nil)
		sumc = cs->lists[list-2]->count + cs->lists[list-1]->count;
	if(sumc != 0 && (nleaf >= cs->nleaf || cs->leafcount[nleaf] > sumc)){
		c->count = sumc;
		c->leaf = oc->leaf;
		c->up = cs->lists[list-1];
		c->gen = 1;
	}else if(nleaf >= cs->nleaf){
		cs->lists[list + 1] = nil;
		return;
	}else{
		c->leaf = nleaf + 1;
		c->count = cs->leafcount[nleaf];
		c->up = oc->up;
		c->gen = 0;
	}
	cs->free = c + 1;

	cs->lists[list + 1] = c;
	c->col = cs->col;
}

static void
hufftabinit(Huff *tab, int n, ulong *bitcount, int nbits)
{
	ulong code, nc[MaxHuffBits];
	int i, bits;

	code = 0;
	for(bits = 1; bits <= nbits; bits++){
		code = (code + bitcount[bits-1]) << 1;
		nc[bits] = code;
	}

	for(i = 0; i < n; i++){
		bits = tab[i].bits;
		if(bits > 0)
			tab[i].encode = nc[bits]++;
	}
}

static int
pivot(ulong *c, int a, int n)
{
	int j, pi, pj, pk;

	j = n/6;
	pi = a + j;	/* 1/6 */
	j += j;
	pj = pi + j;	/* 1/2 */
	pk = pj + j;	/* 5/6 */
	if(c[pi] < c[pj]){
		if(c[pi] < c[pk]){
			if(c[pj] < c[pk])
				return pj;
			return pk;
		}
		return pi;
	}
	if(c[pj] < c[pk]){
		if(c[pi] < c[pk])
			return pi;
		return pk;
	}
	return pj;
}

static	void
leafsort(ulong *leafcount, ushort *leafmap, int a, int n)
{
	ulong t;
	int j, pi, pj, pn;

	while(n > 1){
		if(n > 10){
			pi = pivot(leafcount, a, n);
		}else
			pi = a + (n>>1);

		t = leafcount[pi];
		leafcount[pi] = leafcount[a];
		leafcount[a] = t;
		t = leafmap[pi];
		leafmap[pi] = leafmap[a];
		leafmap[a] = t;
		pi = a;
		pn = a + n;
		pj = pn;
		for(;;){
			do
				pi++;
			while(pi < pn && (leafcount[pi] < leafcount[a] || leafcount[pi] == leafcount[a] && leafmap[pi] > leafmap[a]));
			do
				pj--;
			while(pj > a && (leafcount[pj] > leafcount[a] || leafcount[pj] == leafcount[a] && leafmap[pj] < leafmap[a]));
			if(pj < pi)
				break;
			t = leafcount[pi];
			leafcount[pi] = leafcount[pj];
			leafcount[pj] = t;
			t = leafmap[pi];
			leafmap[pi] = leafmap[pj];
			leafmap[pj] = t;
		}
		t = leafcount[a];
		leafcount[a] = leafcount[pj];
		leafcount[pj] = t;
		t = leafmap[a];
		leafmap[a] = leafmap[pj];
		leafmap[pj] = t;
		j = pj - a;

		n = n-j-1;
		if(j >= n){
			leafsort(leafcount, leafmap, a, j);
			a += j+1;
		}else{
			leafsort(leafcount, leafmap, a + (j+1), n);
			n = j;
		}
	}
}

static void
fatal(char *fmt, ...)
{
	char buf[512];
	va_list arg;

	va_start(arg, fmt);
	doprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);

	longjmp(errjmp, 1);
}

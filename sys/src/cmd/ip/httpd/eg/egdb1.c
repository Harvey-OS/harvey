#include "all.h"

typedef struct Huff	Huff;
typedef struct Mtf	Mtf;
typedef struct Decode	Decode;

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

struct Decode
{
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

static	uchar	*egsacinbuf;
static	uchar	*egsacdbuf;

#define EGMAGIC	"chess endgame sac 1\n"

static	ulong	bitget(Decode*, int);
extern	void	egmapb(EgMap *map, uchar *dst, uchar *src);
extern	void	egmapbi(EgMap *map, uchar *dst, uchar *src);
extern	ulong	egmapindex(EgMap *map, ulong i);
extern	EgMap*	egmapinit(char *perm, char *square, char *same);
extern	int	egprobe(EgMap *map, int infd, ulong pos);
static	void	fatal(Decode *dec, char*);
static	int	hdec(Decode*);
static	void	invertmap(uchar *map, ulong *spread, ulong sh);
static	int	mtf(uchar*, int);
extern	EgMap*	readIndex(int infd);
static	void	recvtab(Decode*, Huff*, int, ushort*);
extern	long	sac(uchar *dst, uchar *src, long nsrc);
static	void	spreadmap(uchar *map, ulong *spread, ulong sh);
extern	long	unsac(uchar *dst, uchar *src, long n, long nsrc);

static	uchar	mapself[64];
static	uchar	mapdiagr[64];
static	uchar	mapdiagl[64];
static	uchar	mapspiral[64];
static	uchar	mapquads[64];

static	uchar	*maps[NMaps] =
{
	mapself,
	mapdiagr,
	mapdiagl,
	mapspiral,
	mapquads,
};

static long
getd(char **lp, int base)
{

	long n;
	int c;

	for(;;) {
		c = *(*lp)++;
		if(c != '.' || c != ' ' || c != '\t')
			break;
	}
	if(c < '0' || c > '9')
		if(base==10 || c < 'a' || c > 'f')
			return -1;
	n = 0;
	for(;;) {
		if(c < '0' || c > '9')
			if(base==10 || c < 'a' || c > 'f')
				break;
		n = n*base + (c-'0');
		if(c < '0' || c > '9')
			n += 10 + ('0'-'a');
		c = *(*lp)++;
	}
	return n;
}

static int
parse(Ecor *c, char *l)
{
	long k, p;
	int n;

	if(l == nil)
		return 1;
	k = getd(&l, 16);
	p = getd(&l, 16);
	n = getd(&l, 10);
	if(k < 0 || p < 0 || n < 0)
		return 1;
	c->k = k;
	c->p = p;
	c->n = n;
	return 0;
}

static void
coropen(char *name, Egdb *e)
{
	int i;
	Biobuf *b;
	char *l;
	Ecor cor;

	e->ncor = 0;
	b = Bopen(name, OREAD);
	if(b == nil)
		return;
	for(i=0;; i++) {
		l = Brdline(b, '\n');
		if(l == nil)
			break;
		if(parse(&cor, l)) {
			print("error parsing excepthin list\n");
			Bterm(b);
			return;
		}
	}

	e->cor = malloc(i * sizeof(*e->cor));
	if(e->cor == nil)
		return;
	e->ncor = i;

	Bseek(b, 0, 0);
	for(i=0; i<e->ncor; i++) {
		l = Brdline(b, '\n');
		if(parse(&e->cor[i], l))
			break;
	}
	Bterm(b);
}

static int
exception(Egdb *e, long k, long p)
{
	int n, m;
	Ecor *c, *mc;

	n = e->ncor;
	c = e->cor;

loop:
	if(n <= 0)
		return -1;
	m = n/2;
	mc = c+m;
	if(mc->k > k || (mc->k == k && mc->p >= p)) {
		if(mc->k == k && mc->p == p)
			return mc->n;
		n = m;
		goto loop;
	}
	c = mc+1;
	n = n-m-1;
	goto loop;
}

Egdb*
egdbinit(char *file, int mode)
{
	Egdb *e;
	char name[1000];
	int i;

	e = malloc(sizeof(*e));
	if(e == nil)
		return nil;
	egsacinbuf = malloc(BlockSize);
	egsacdbuf = malloc(BlockSize);
	if(egsacinbuf == nil || egsacdbuf == nil)
		return nil;
	for(i=0; i<nelem(e->egmap); i++)
		e->egmap[i] = nil;
	e->nfd = 0;

	/* compressed open */
	if(!(mode & 2)) {
		for(i=nelem(e->egmap)-1; i>=0; i--) {
			sprint(name, "/usr/web/eg-hidden/cdir/%s.k%d", file, i);
			if(mode & 1)
				sprint(name, "/usr/web/eg-hidden/cdir/%s.mk%d", file, i);
			e->infd[i] = open(name, OREAD);
			if(e->infd[i] < 0)
				break;
			e->egmap[i] = readIndex(e->infd[i]);
			if(e->egmap[i] == nil)
				break;
		}
		if(e->egmap[0] != nil) {
			sprint(name, "/usr/web/eg-hidden/cdir/%s.kc", file);
			if(mode & 1)
				sprint(name, "/usr/web/eg-hidden/cdir/%s.mkc", file);
			coropen(name, e);
		}
	}

	/* uncompressed open */
	for(i=0; i<nelem(e->fd); i++) {
		sprint(name, "/usr/web/eg-hidden/xdir/%s.r%d", file, i);
		if(mode & 1)
			sprint(name, "/usr/web/eg-hidden/xdir/%s.mr%d", file, i);
		e->fd[i] = open(name, OREAD);
		if(e->fd[i] < 0)
			break;
	}
	e->nfd = i;

	if(e->nfd == 0 && e->egmap[nelem(e->egmap)-1] == nil)
		return nil;

	return e;
}

int
egdbread(Egdb *e, ulong kgird, ulong pgird)
{
	int i, n1;
	ulong pos;

	ulong o;
	int n2, bit;
	uchar buf;

	/* compressed read */
	n1 = -1;
	if(e->egmap[nelem(e->egmap)-1] != nil) {
		n1 = exception(e, kgird, pgird);
		if(n1 < 0) {
			i = kgird / 66;
			if(i < 0 || i >= nelem(e->egmap))
				return -1;
			pos = ((kgird % 66) << 24) | pgird;
			n1 = egprobe(e->egmap[i], e->infd[i], pos);
		}
	}

	/* uncompressed read */
	n2 = -1;
	if(e->nfd != 0) {
		o = (kgird<<21) + (pgird>>3);
		bit = 1 << (pgird & 7);
		n2 = 0;
		for(i=0; i<e->nfd; i++) {
			seek(e->fd[i], o, 0);
			if(read(e->fd[i], &buf, 1) != 1)
				return -1;
			if(buf & bit)
				n2 |= 1<<i;
		}
	}

	if(n1 == -1)
		return n2;
	if(n2 == -1)
		return n1;

	if(n2 != n1)
		print("%lud.%lux: compressed = %d; uncompressed = %d\n", kgird, pgird, n1, n2);
	return n1;
}

/*
 * make up a board mapping
 * perm is a permutation of the pieces
 * squares is a mapping of the board for each piece
 * same gives an identical piece for piece
 */
EgMap*
egmapinit(char *perm, char *squares, char *same)
{
	EgMap *map;
	int sh[NPiece], ish[NPiece], sq[NPiece], sa[NPiece];
	long i;

	/*
	 * swap order of pieces
	 */
	if(perm == nil)
		perm = "3210";
	if(strlen(perm) != NPiece)
		return nil;

	for(i = 0; i < NPiece; i++)
		ish[i] = -1;
	for(i = 0; i < NPiece; i++){
		sh[i] = perm[NPiece-1 - i] - '0';
		if(sh[i] < 0 || sh[i] >= NPiece)
			return nil;
		ish[sh[i]] = i;
	}
	for(i = 0; i < NPiece; i++){
		if(ish[i] < 0)
			return nil;
	}

	/*
	 * alter ordering of squares
	 */
	if(squares == nil)
		squares = "0000";
	if(strlen(squares) != NPiece)
		return nil;
	for(i = 0; i < NPiece; i++){
		sq[i] = squares[NPiece-1-i] - '0';
		if(sq[i] < 0 || sq[i] >= NMaps)
			return nil;
	}

	/*
	 * pieces that are the same
	 */
	if(same == nil)
		same = "3210";
	if(strlen(same) != NPiece)
		return nil;
	for(i = 0; i < NPiece; i++){
		sa[i] = same[NPiece-1-i] - '0';
		if(sa[i] < 0 || sa[i] >= NPiece)
			return nil;
	}
	for(i = 0; i < NPiece; i++){
		if(sa[i] > i){
			if(sa[sa[i]] != i)
				return nil;
			sa[i] = i;
		}
	}

	map = malloc(sizeof(EgMap));
	if(map == nil)
		return nil;

	for(i = 0; i < NPiece; i++){
		spreadmap(maps[sq[i]], map->spread[i], sh[i] * PieceLog);
		map->same[i] = sa[i];
	}

	return map;
}

ulong
egmapindex(EgMap *map, ulong pos)
{
	int slice, p, sp, pi, spi, pv[NPiece];

	slice = pos >> (NPiece * PieceLog);
	for(pi = 0; pi < NPiece; pi++){
		p = pos & PieceMask;
		pos >>= PieceLog;
		spi = map->same[pi];
		pv[pi] = p;
		sp = pv[spi];
		if(p < sp){
			pv[spi] = p;
			pv[pi] = sp;
		}
	}
	pos = 0;
	for(pi = 0; pi < NPiece; pi++)
		pos |= map->spread[pi][pv[pi]];
	return pos | ((ulong)slice << (NPiece * PieceLog));
}

void
egmapb(EgMap *map, uchar *dst, uchar *src)
{
	ulong i, pos;
	int p, sp, pi, spi, pv[NPiece];

	/*
	 * zero the destination since we won't write all values
	 * if some of the pieces are the same
	 */
	memset(dst, 0, SliceSize);
	for(i = 0; i < SliceSize; i++){
		pos = i;
		for(pi = 0; pi < NPiece; pi++){
			p = pos & PieceMask;
			pos >>= PieceLog;
			spi = map->same[pi];
			pv[pi] = p;
			sp = pv[spi];
			if(p < sp){
				pv[spi] = p;
				pv[pi] = sp;
			}
		}

		pos = 0;
		for(pi = 0; pi < NPiece; pi++)
			pos |= map->spread[pi][pv[pi]];

		dst[pos] = src[i];
	}
}

void
egmapbi(EgMap *map, uchar *dst, uchar *src)
{
	ulong i, pos;
	int p, sp, pi, spi, pv[NPiece];

	for(i = 0; i < SliceSize; i++){
		pos = i;
		for(pi = 0; pi < NPiece; pi++){
			p = pos & PieceMask;
			pos >>= PieceLog;
			spi = map->same[pi];
			pv[pi] = p;
			sp = pv[spi];
			if(p < sp){
				pv[spi] = p;
				pv[pi] = sp;
			}
		}
		pos = 0;
		for(pi = 0; pi < NPiece; pi++)
			pos |= map->spread[pi][pv[pi]];

		dst[i] = src[pos];
	}
}

static void
spreadmap(uchar *map, ulong *spread, ulong sh)
{
	int i;

	for(i = 0; i < 64; i++)
		spread[i] = (ulong)map[i] << sh;
}

static void
invertmap(uchar *map, ulong *spread, ulong sh)
{
	ulong i;

	for(i = 0; i < 64; i++)
		spread[map[i]] = i << sh;
}

static uchar mapself[64] = {
	 0,  1,  2,  3,  4,  5,  6,  7,
	 8,  9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23,
	24, 25, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39,
	40, 41, 42, 43, 44, 45, 46, 47,
	48, 49, 50, 51, 52, 53, 54, 55,
	56, 57, 58, 59, 60, 61, 62, 63,
};

static uchar mapdiagr[64] = {
	 0, 32,  8, 46, 20, 56, 28, 62,
	39,  1, 33,  9, 47, 21, 57, 29,
	14, 40,  2, 34, 10, 48, 22, 58,
	51, 15, 41,  3, 35, 11, 49, 23,
	24, 52, 16, 42,  4, 36, 12, 50,
	59, 25, 53, 17, 43,  5, 37, 13,
	30, 60, 26, 54, 18, 44,  6, 38,
	63, 31, 61, 27, 55, 19, 45,  7,
};

static uchar mapdiagl[64] = {
	62, 28, 56, 20, 46,  8, 32,  0,
	29, 57, 21, 47,  9, 33,  1, 39,
	58, 22, 48, 10, 34,  2, 40, 14,
	23, 49, 11, 35,  3, 41, 15, 51,
	50, 12, 36,  4, 42, 16, 52, 24,
	13, 37,  5, 43, 17, 53, 25, 59,
	38,  6, 44, 18, 54, 26, 60, 30,
	 7, 45, 19, 55, 27, 61, 31, 63,
};

static uchar mapspiral[64] = {
	56, 57, 58, 59, 60, 61, 62, 63,
	55, 30, 31, 32, 33, 34, 35, 36,
	54, 29, 12, 13, 14, 15, 16, 37,
	53, 28, 11,  2,  3,  4, 17, 38,
	52, 27, 10,  1,  0,  5, 18, 39,
	51, 26,  9,  8,  7,  6, 19, 40,
	50, 25, 24, 23, 22, 21, 20, 41,
	49, 48, 47, 46, 45, 44, 43, 42,
};

static uchar mapquads[64] = {
	31, 30, 29, 28, 60, 61, 62, 63,
	27, 26, 25, 24, 56, 57, 58, 59,
	23, 22, 21, 20, 52, 53, 54, 55,
	19, 18, 17, 16, 48, 49, 50, 51,
	 3,  2,  1,  0, 32, 33, 34, 35,
	 7,  6,  5,  4, 36, 37, 38, 39,
	11, 10,  9,  8, 40, 41, 42, 43,
	15, 14, 13, 12, 44, 45, 46, 47,
};

int
egprobe(EgMap *egm, int infd, ulong pos)
{
	uchar *lo, *hi;
	ulong nsac, nhi, mpos, hpos;
	long i, h;
	int v;

	mpos = egmapindex(egm, pos);

	h = mpos/BlockSize;
	if(seek(infd, egm->index[h].start, 0) != egm->index[h].start)
		sysfatal("bad seek");
	mpos %= BlockSize;

	/*
	 * uncompress the low parts
	 */
	nsac = egm->index[h].losac;
	if(read(infd, egsacinbuf, nsac) != nsac)
		sysfatal("can't read compressed data");
	lo = egsacdbuf;
	if(nsac == BlockSize/4)
		lo = egsacinbuf;
	else if(unsac(lo, egsacinbuf, BlockSize/4, nsac) != 0)
		sysfatal("bad uncompress low");

	/*
	 * check for low answer
	 */
	v = lo[mpos >> 2] >> (6 - 2 * (mpos & 3));
	if((v & 3) != 3)
		return v & 3;

	/*
	 * missed: find index of high entry
	 */
	hpos = 0;
	while(v = v >> 2)
		if((v & 3) == 3)
			hpos++;
	mpos >>= 2;
	for(i = 0; i < mpos; i++)
		for(v = lo[i]; v; v = v >> 2)
			if((v & 3) == 3)
				hpos++;

	/*
	 * uncompress the high parts
	 */
	nsac = egm->index[h].hisac;
	nhi = egm->index[h].nhi;
	if(read(infd, egsacinbuf, nsac) != nsac)
		sysfatal("can't read compressed data");
	hi = egsacdbuf;
	if(nsac == nhi)
		hi = egsacinbuf;
	else if(unsac(hi, egsacinbuf, nhi, nsac) != 0)
		sysfatal("bad uncompress low");

	if(hpos >= nhi)
		sysfatal("not enough hi part");

	/*
	 * probe high part
	 */
	return hi[hpos] + 3;
}

EgMap *
readIndex(int infd)
{
	EgMap *egm;
	char *perm;
	uchar header[HeaderSize];
	ulong nhi, nsrc, start;
	long i, h;

	if(read(infd, header, HeaderSize) != HeaderSize)
		sysfatal("can't read input: %r");
	if(strncmp((char*)header, EGMAGIC, strlen(EGMAGIC)) != 0)
		sysfatal("not an endgame file");
	perm = strchr((char*)header, '\n');
	if(perm == nil || perm + 10 >= (char*)header + HeaderId
	|| perm[5] != ' ' || perm[10] != ' ' || perm[15] != '\n')
		sysfatal("can't find permutation");
	perm++;

	perm[4] = '\0';
	perm[9] = '\0';
	perm[14] = '\0';
	egm = egmapinit(perm, &perm[5], &perm[10]);
	if(egm == nil)
		sysfatal("can't initialize piece mapping");

	h = HeaderId;
	start = HeaderSize;
	for(i = 0; i < IndexEnts; i++){
		egm->index[i].start = start;
		nsrc = header[h+0] << 8;
		nsrc |= header[h+1];
		if(nsrc > BlockSize)
			sysfatal("bad low index");
		egm->index[i].losac = nsrc;
		start += nsrc;

		nhi = header[h+2] << 16;
		nhi |= header[h+3] << 8;
		nhi |= header[h+4];
		nsrc = header[h+5] << 16;
		nsrc |= header[h+6] << 8;
		nsrc |= header[h+7];

		h += IndexEntSize;
		if(h > HeaderSize)
			sysfatal("header overflow");
		if(nsrc > nhi)
			sysfatal("bad high index");
		egm->index[i].nhi = nhi;
		egm->index[i].hisac = nsrc;
		start += nsrc;
	}
	return egm;
}

void
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

int
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

long
unsac(uchar *dst, uchar *src, long n, long nsrc)
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
	I = bitget(dec, 24);
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
		if(i > n || i == I)
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
	return dec->smax - dec->src - 4 + ((dec->nbits + 7) >> 3);
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
hdecsym(Decode *dec, Huff *h)
{
	long c;
	ulong bits;
	int b, nbits;

	bits = dec->bits;
	b = h->flatbits;
	nbits = dec->nbits - b;
	for(; (c = bits >> nbits) > h->maxcode[b]; b++)
		nbits--;
	if(b > h->maxbits)
		fatal(dec, "too many bits consumed");
	dec->nbits = nbits;
	return h->decode[h->last[b] - c];
}

static int
hdec(Decode *dec)
{
	ulong c;
	int nbits;

	if(dec->nbits < dec->tab.maxbits)
		fillbits(dec);
	nbits = dec->nbits;
	dec->bits &= (1 << nbits) - 1;
	c = dec->tab.flat[dec->bits >> (nbits - dec->tab.flatbits)];
	if(c == ~0)
		c = hdecsym(dec, &dec->tab);
	else{
		dec->nbits = nbits - (c & 0xff);
		c >>= 8;
	}

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
	ulong c, code, nc[MaxHuffBits];
	int i, b, ec;

	h->maxbits = maxbits;
	if(maxbits < 0)
		return;

	code = 0;
	c = 0;
	for(b = 0; b <= maxbits; b++){
		h->last[b] = c;
		c += bitcount[b];
		nc[b] = code << 1;
		code = (code << 1) + bitcount[b];
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
		if(m == ~0)
			m = hdecsym(dec, tab);
		else{
			dec->nbits -= m & 0xff;
			m >>= 8;
		}
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

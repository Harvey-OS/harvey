#include <u.h>
#include <libc.h>
#include <libg.h>
#include <gnot.h>

/*
 * bitblt operates a 'word' at a time.
 * WBITS is the number of bits in a word
 * LWBITS=log2(WBITS),
 * W2L is the number of words in a long
 * WMASK has bits set for the low order word of a long
 * WType is a pointer to a word
 */
#ifndef WBITS
#define WBITS	32
#define LWBITS	5
#define	W2L	1
#define WMASK	~0UL
typedef ulong	*WType;
#endif

#define DEBUG 

#ifdef TEST
/*
 * globals used for testing
 */
int	FORCEFORW;
int	FORCEBAKW;
GBitmap	*curdm, *cursm;
Point	curpt;
Rectangle curr;
Fcode	curf;
void	*mem;
#endif

static void
gbitexplode(ulong sw, ulong *buf, int sdep, int x)
{
	int i, j, o, q, n, nw, inc, qinc;
	ulong s, dw, pix;

	inc = 1 << sdep;
	pix = (1 << inc) - 1;
	nw = 1 << x;
	n = 32 >> x;
	qinc = (nw << sdep) - inc;
	for(o = 32 - n; o >= 0; o -= n){
		dw = 0;
		s = sw >> o;
		q = 0;
		for(j = 0; j < n; j += inc){
			dw |= (s & (pix << j)) << q;
			q += qinc;
		}
		for(j = 0; j < x; j++)
			dw |= dw << (inc << j);
		*buf++ = dw;
	}
}

/*
void
main(void)
{
	ulong buf[128];

	gbitexplode(0x7777, buf, 0, 3);
	exits(0);
}
*/

void
gbitblt(GBitmap *dm, Point pt, GBitmap *sm, Rectangle r, Fcode fcode)
{
	int	width;		/* width in bits of dst */
	int	wwidth;		/* floor width in words */
	int	height;		/* height in pixels minus 1 */
	int	sdep;		/* src ldepth */
	int 	ddep;		/* dst ldepth */
	int	deltadep;	/* diff between ldepths */
	int	sspan;		/* words between scanlines in src */
	int	dspan;		/* words between scanlines in dst */
	int	soff;		/* bit offset of src start point */
	int	sdest;		/* bit offset of src start point that matches doff when expanded */
	int	doff;		/* bit offset of dst start point */
	int	delta;		/* amount to shift src by */
	int	sign;		/* of delta */
	ulong	*saddr;
	ulong	*daddr;
	ulong	*s;
	ulong	*d;
	ulong	mask;
	ulong	tmp;		/* temp storage source word */
	ulong	sw;		/* source word constructed */
	ulong	dw;		/* dest word fetched */
	ulong	lmask;		/* affected pixels in leftmost dst word */
	ulong	rmask;		/* affected pixels in rightmost dst word */
	int	i;
	int	j;
	int	x;
	ulong	buf[32];	/* for expanding a source */
	ulong	*p;		/* pointer into buf */
	int	spare;		/* number of words already converted */


#ifdef TEST
	curdm = dm;
	cursm = sm;
	curpt = pt;
	curr = r;
	curf = fcode;
#endif

	gbitbltclip(&dm);

	width = r.max.x - r.min.x;
	if(width <= 0)
		return;
	height = r.max.y - r.min.y - 1;
	if(height < 0)
		return;

	ddep = dm->ldepth;
	pt.x <<= ddep;
	width <<= ddep;

	sdep = sm->ldepth;
	r.min.x <<= sdep;
	r.max.x <<= sdep;

	dspan = dm->width * W2L;
	sspan = sm->width * W2L;

	daddr = (ulong*)((WType)dm->base
			+ dm->zero*W2L + pt.y*dspan
			+ (pt.x >> LWBITS));
	saddr = (ulong*)((WType)sm->base
			+ sm->zero*W2L + r.min.y*sspan
			+ (r.min.x >> LWBITS));

	doff = pt.x & (WBITS - 1);
	lmask = WMASK >> doff;
	rmask = (WMASK << (WBITS - ((doff+width) & (WBITS-1))))&WMASK;
	soff = r.min.x & (WBITS-1);
	wwidth = ((pt.x+width-1)>>LWBITS) - (pt.x>>LWBITS);

	if(sm == dm){
#ifdef TEST
		if(!FORCEBAKW &&
		   (FORCEFORW || sm != dm || saddr > daddr ||
		    (saddr == daddr && soff > doff)))
			;
		else{
			daddr += height * dspan;
			saddr += height * sspan;
			sspan -= 2 * W2L * sm->width;
			dspan -= 2 * W2L * dm->width;
		}
#else
		if(r.min.y < pt.y){	/* bottom to top */
			daddr += height * dspan;
			saddr += height * sspan;
			sspan -= 2 * W2L * sm->width;
			dspan -= 2 * W2L * dm->width;
		}else if(r.min.y == pt.y && r.min.x < pt.x)
			abort()/*goto right*/;
#endif
	}
	if(wwidth == 0)		/* collapse masks for narrow cases */
		lmask &= rmask;
	fcode &= F;

	deltadep = ddep - sdep;
	sdest = doff >> deltadep;
	delta = soff - sdest;
	sign = 0;
	if(delta < 0){
		sign = 1;
		delta = -delta;
	}

	p = 0;
	for(j = 0; j <= height; j++){
		d = daddr;
		s = saddr;
		mask = lmask;
		tmp = 0;
		if(!sign)
			tmp = *s++;
		spare = 0;
		for(i = wwidth; i >= 0; i--){
			if(spare)
				sw = *p++;
			else{
				if(sign){
					sw = tmp << (WBITS-delta);
					tmp = *s++;
					sw |= tmp >> delta;
				}else{
					sw = tmp << delta;
					tmp = *s++;
					if(delta)
						sw |= tmp >> (WBITS-delta);
				}
				spare = 1 << deltadep;
				if(deltadep >= 1){
					gbitexplode(sw, buf, sdep, deltadep);
					p = buf;
					sw = *p++;
				}
			}

			dw = *d;
			switch(fcode){		/* ltor bit aligned */
			case Zero:	*d = dw & ~mask;		break;
			case DnorS:	*d = dw ^ ((~sw | dw) & mask);	break;
			case DandnotS:	*d = dw ^ ((sw & dw) & mask);	break;
			case notS:	*d = dw ^ ((~sw ^ dw) & mask);	break;
			case notDandS:	*d = dw ^ ((sw | dw) & mask);	break;
			case notD:	*d = dw ^ mask;			break;
			case DxorS:	*d = dw ^ (sw & mask);		break;
			case DnandS:	*d = dw ^ ((sw | ~dw) & mask);	break;
			case DandS:	*d = dw ^ ((~sw & dw) & mask);	break;
			case DxnorS:	*d = dw ^ (~sw & mask);		break;
			case D:						break;
			case DornotS:	*d = dw | (~sw & mask);		break;
			case S:		*d = dw ^ ((sw ^ dw) & mask);	break;
			case notDorS:	*d = dw ^ (~(sw & dw) & mask);	break;
			case DorS:	*d = dw | (sw & mask);		break;
			case F:		*d = dw | mask;			break;
			}
			d++;

			mask = WMASK;
			if(i == 1)
				mask = rmask;
			spare--;
		}
		saddr += sspan;
		daddr += dspan;
	}
}

#ifdef TEST
void	prprog(void);
GBitmap *bb1, *bb2;
ulong	*src, *dst, *xdst, *xans;
int	swds, dwds;
long	ticks;
int	timeit;

long
func(int f, long s, int sld, long d, int dld)
{
	long a;
	int sh, i, db, sb;

	db = 1 << dld;
	sb = 1 << sld;
	sh = db - sb;
	if(sh > 0) {
		a = s;
		for(i = sb; i<db; i += sb){
			a <<= sb;
			s |= a;
		}
	} else if(sh < 0)
		s >>= -sh;

	switch(f){
	case Zero:	d = 0;			break;
	case DnorS:	d = ~(d|s);		break;
	case DandnotS:	d = d & ~s;		break;
	case notS:	d = ~s;			break;
	case notDandS:	d = ~d & s;		break;
	case notD:	d = ~d;			break;
	case DxorS:	d = d ^ s;		break;
	case DnandS:	d = ~(d&s);		break;
	case DandS:	d = d & s;		break;
	case DxnorS:	d = ~(d^s);		break;
	case S:		d = s;			break;
	case DornotS:	d = d | ~s;		break;
	case D:		d = d;			break;
	case notDorS:	d = ~d | s;		break;
	case DorS:	d = d | s;		break;
	case F:		d = ~0;			break;
	}

	d &= ((1<<db)-1);
	return d;
}

void
run(int fr, int to, int w, int op)
{
	int i, j, f, t, fy, ty;
	extern long *_clock;

	fr += bb2->r.min.x;
	to += bb1->r.min.x;
	fy = bb2->r.min.y + 1;
	ty = bb1->r.min.y + 1;
	if(timeit) {
		memcpy(dst, xdst, dwds * sizeof(long));
		ticks -= *_clock;
		gbitblt(bb1, Pt(to,ty), bb2, Rect(fr,fy,fr+w,fy+2), op);
		ticks += *_clock;
		return;
	}
	f = fr;
	t = to;
	memcpy(dst, xdst, dwds * sizeof(long));
	for(i=0; i<w; i++) {
		gbitblt(bb1, Pt(t,ty), bb2, Rect(f,fy,f+1,fy+1), op);
		gbitblt(bb1, Pt(t,ty+1), bb2, Rect(f,fy+1,f+1,fy+2), op);
		f++;
		t++;
	}
	memcpy(xans, dst, dwds * sizeof(long));

	memcpy(dst, xdst, dwds * sizeof(long));
	gbitblt(bb1, Pt(to,ty), bb2, Rect(fr,fy,fr+w,fy+2), op);

	if(memcmp(xans, dst, dwds * sizeof(long))) {
		/*
		 * print src and dst row offset, width in bits, and forw/back
		 * then print for each of the four rows: the source (s),
		 * the dest (d), the good value of the answer (g),
		 * and the actual bad value of the answer (b)
		 */
		print("fr=%d to=%d w=%d fb=%d%d\n",
			fr, to, w, FORCEFORW, FORCEBAKW);
		print("dst bitmap b %#lux, z %d, w %d, ld %d, r [%d,%d][%d,%d]\n",
			bb1->base, bb1->zero, bb1->width, bb1->ldepth,
			bb1->r.min.x, bb1->r.min.y, bb1->r.max.x, bb1->r.max.y);
		print("src bitmap b %#lux, z %d, w %d, ld %d, r [%d,%d][%d,%d]\n",
			bb2->base, bb2->zero, bb2->width, bb2->ldepth,
			bb2->r.min.x, bb2->r.min.y, bb2->r.max.x, bb2->r.max.y);
		for(j=0; 7*j < dwds; j++) {
			print("\ns");
			for(i=0; i<7 && 7*j+i < dwds; i++)
				print(" %.8lux", src[7*j + i]);
			print("\nd");
			for(i=0; i<7 && 7*j+i < dwds; i++)
				print(" %.8lux", xdst[7*j + i]);
			print("\ng");
			for(i=0; i<7 && 7*j+i < dwds; i++)
				print(" %.8lux", xans[7*j + i]);
			print("\nb");
			for(i=0; i<7 && 7*j+i < dwds; i++)
				print(" %.8lux", dst[7*j + i]);
			print("\n");
		}
		prprog();
	}
}

void
prprog(void)
{
	exits(0);
}

int
main(int argc, char *argv[])
{
	int f, t, w, i, sld, dld, op, iters, simple;
	ulong s, d, spix, dpix, apix, fpix, m, *ps, *pd;
	Point sorg, dorg;
	GBitmap *bs, *bd;
	long seed;
	char *ct;

	sld = 0;
	dld = 0;
	timeit = 0;
	iters = 200;
	simple = 0;
	ARGBEGIN {
	case 'i':
		iters = atoi(ARGF());
		break;
	case 's':
		simple = 1;
		break;
	case 't':
		timeit = 1;
		ct = ARGF();
		if(ct)
			iters = atoi(ct);
		break;
	} ARGEND
	if(argc > 0)
		sld = atoi(argv[0]);
	if(argc > 1)
		dld = atoi(argv[1]);
	if(!timeit && !simple) {
		seed = time(0);
		print("seed %lux\n", seed); srand(seed);	/**/
	}

	print("sld %d dld %d\n", sld, dld);
	op = 1;

	/* bitmaps for 1-bit tests */
	bd = gballoc(Rect(0,0,32,1), dld);
	bs = gballoc(Rect(0,0,32,1), sld);
	for(i=0; i<bs->width; i++)
		bs->base[i] = lrand();

	/* bitmaps for rect tests */
	if(simple) {
		dorg = Pt(0,0);
		sorg = Pt(0,0);
	} else {
		dorg = Pt(nrand(63)-31,nrand(63)-31);
		sorg = Pt(nrand(63)-31,nrand(63)-31);
	}
	bb1 = gballoc(Rpt(dorg,add(dorg,Pt(200,4))), dld);
	bb2 = gballoc(Rpt(sorg,add(sorg,Pt(200,4))), sld);
	dwds = bb1->width * Dy(bb1->r);
	swds = bb2->width * Dy(bb2->r);
	dst = bb1->base;
	src = bb2->base;
	xdst = malloc(dwds * sizeof(long));
	xans =  malloc(dwds * sizeof(long));
	for(i=0; i<swds; i++)
		src[i] = lrand();
	for(i=0; i<dwds; i++)
		xdst[i] = lrand();

loop:
	print("Op %d\n", op);
	if(!timeit) {
		print("one pixel\n");
		ps = bs->base;
		pd = bd->base;
		FORCEFORW = 1;
		FORCEBAKW = 0;
		for(i=0; i<1000; i++, FORCEFORW = !FORCEFORW, FORCEBAKW = !FORCEBAKW) {
			f = nrand(32 >> sld);
			t = nrand(32 >> dld);
			s = lrand();
			d = lrand();
			ps[0] = s;
			pd[0] = d;
#ifdef T386
			spix = (byterev(s) >> (32 - ((f+1)<<sld))) & ((1 << (1<<sld)) - 1);
			dpix = (byterev(d) >> (32 - ((t+1)<<dld))) & ((1 << (1<<dld)) - 1);
#else
			spix = (s >> (32 - ((f+1)<<sld))) & ((1 << (1<<sld)) - 1);
			dpix = (d >> (32 - ((t+1)<<dld))) & ((1 << (1<<dld)) - 1);
#endif
#ifdef T386
			apix = byterev(func(op, spix, sld, dpix, dld) << (32 - ((t+1)<<dld)));
#else
			apix = func(op, spix, sld, dpix, dld) << (32 - ((t+1)<<dld));
#endif
			gbitblt(bd, Pt(t,0), bs, Rect(f,0,f+1,1), op);
			if(ps[0] != s) {
				print("bb src %.8lux %.8lux %d %d\n", ps[0], s, f, t);
				exits("error");
			}
			m = ((1 << (1<<dld)) - 1) << (32 - ((t+1)<<dld));
#ifdef T386
			m = byterev(m);
#endif
			if((pd[0] & ~m) != (d & ~m)) {
					print("bb dst1 %.8lux %.8lux\n",
						s, d);
					print("bb      %.8lux %.8lux %d %d\n",
						ps[0], pd[0], f, t);
					prprog();
					exits("error");
			}
			if((pd[0] & m) != apix) {
				spix <<= 32 - ((f+1)<<sld);
				dpix <<= 32 - ((t+1)<<dld);
#ifdef T386
				spix = byterev(spix);
				dpix = byterev(dpix);
#endif
				print("bb dst2 %.8lux %.8lux\n",
					s, d);
				print("bb      %.8lux %.8lux %d %d\n",
					ps[0], pd[0], f, t);
				print("bb      %.8lux %.8lux %.8lux %.8lux\n",
					spix, dpix, apix, pd[0] & m);
				prprog();
				exits("error");
			}
		}
	}

	print("for\n");
	FORCEFORW = 1;
	FORCEBAKW = 0;

	for(i=0; i<iters; i++) {
		f = nrand(64);
		t = nrand(64);
		w = nrand(130);
		run(f, t, w, op);
	}

	if(sld == dld) {
		print("bak\n");
		FORCEFORW = 0;
		FORCEBAKW = 1;
	
		for(i=0; i<iters; i++) {
			f = nrand(64);
			t = nrand(64);
			w = nrand(130);
			run(f, t, w, op);
		}
	}

	if(op < F) {
		op++;
		goto loop;
	}
	if(timeit)
		print("time: %d ticks\n", ticks);
	exits(0);
}


#endif

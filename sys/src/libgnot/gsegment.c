#include <u.h>
#include <libg.h>
#include <gnot.h>

#define	PSHIFT(x, y)	{if(LENDIAN) x <<= y; else x >>= y;}

int
_setdda(int x, int y, Linedesc *l)
{
	int e;

	/*
	 * Unfold all the symmetry folded out in gsetline()
	 */
	if(l->slopeneg == 0)
		e = ((2*(x-l->x0)+2)*l->dminor
			-(2*(y-l->y0)+1)*l->dmajor);
	else{
		if(l->xmajor)
			e = ((2*(x-l->x0)+2)*l->dminor
				+(2*(l->y0+y)-1)*l->dmajor);
		else
			e = ((2*(l->x0-x)+2)*l->dminor
				-(2*(l->y0+y)+1)*l->dmajor)-1;
	}
	return e;
}

/*
 * Strange, usually fast line code.
 * Based on the observation that given src and f, src is constant
 * for the call, so we can reduce f by that symmetry.  Then we can
 * reduce it again by choosing the src we actually run with; thus
 *	src, f -> src', f'
 * In the code, src is replicated through the register m.
 * The f's we choose (they are somewhat arbitrary) are
 * 	f(0) = S & D
 * and
 * 	f(1) = S | ~D,
 * chosen by the bits of the incoming f.  In particular, if
 * 	src == 0
 * we select
 * 	f' = f&1, src' = (f&2) >> 1
 * else
 * 	f' = (f>>2)&1, src' = ((f>>2)&2) >> 1
 * If src is all 1's or all 0's (guaranteed true on a 1-bit display),
 * this is all we need; otherwise (the slower case) we insert into the dest
 * twice: once for the 1 bits in src, once for the 0 bits, using f' and src'
 * each computed as above.
 */

void
gsegment(GBitmap *b, Point p, Point q, int src, Fcode f)
{
	int i, dw, dx, dy;
	int e, e1, e2;
	uchar m, m1, k, k0, k1, f0, f1, lom, him, him0, him1, t, *dest;
	int ld, bpp, xshift;
	int m0, lo, lo0, lo1, kk, kk0, kk1;
	Linedesc l;

	if(!_clipline(b->clipr, &p, &q, &l))
		return;
	/* line is now clipped and closed */
	dx = q.x - p.x;
	dy = q.y - p.y;
	if(dx < 0)	/* can't happen, but breaks code if it does */
		return;

	ld = b->ldepth;
	bpp = 8>>(3-ld);
	lom = ((1<<bpp)-1);
	him = lom<<(8-bpp);
	xshift = (p.x&(7>>ld))<<ld;
	if(LENDIAN)
		xshift = 8-bpp - xshift;
	k = him>>xshift;	/* mask */
	lo = src&lom;

	if(lo==lom || lo==0){
		/*
		 * set new f, new src
		 */
		if(lo)
			f >>= 2;
		f &= 3;
		if(f & 2)
			lo = lom;
		else
			lo = 0;
		f &= 1;
		/*
		 * develop m as a register full of pixels containing 'lo1'
		 */
		m = lo;
		for(i=1<<ld; i<8; i+=i)
			m |= m<<i;
	}else{
		/*
		 * set *1 for 1 bits; *0 for 0 bits
		 */
		f1 = (f>>2)&3;
		if(f1 & 2)
			lo1 = lom & lo;
		else
			lo1 = 0;
		f1 &= 1;
		him1 = (lom&lo)<<(8-bpp);
		k1 = him1>>xshift;	/* mask */
		f0 = f&3;
		if(f0 & 2)
			lo0 = lom & ~lo;
		else
			lo0 = 0;
		f0 &= 1;
		him0 = (lom&~lo)<<(8-bpp);
		k0 = him0>>xshift;	/* mask */
		/*
		 * develop m0, m1 as a register full of pixels containing 'lo1'
		 */
		m0 = lo0;
		m1 = lo1;
		for(i=1<<ld; i<8; i+=i){
			m0 |= m0<<i;
			m1 |= m1<<i;
		}
		f = 2;
	}

	if(LENDIAN){
		him >>= 8-bpp;
		him0 >>= 8-bpp;
		him1 >>= 8-bpp;
	}
	
	dw = b->width*sizeof(ulong);
	if(dy < 0){
		dw = -dw;
		dy = -dy;
	}
	dest = (uchar*)gaddr(b, p) + ((p.x&(31>>ld))>>(3-ld));
	e1 = 2*l.dminor;
	e2 = e1 - 2*l.dmajor;
	if(l.xmajor == 0)
		goto majory;

majorx:
	e = _setdda(p.x, p.y, &l);
	switch(f){
	case 0:
		kk = 0;
		for(i=dx; i>=0; i--){
			kk |= k;
			PSHIFT(k, bpp);
			if(e >= 0){
				*dest &= m|~kk;
				dest += dw;
				e += e2;
				kk = 0;
				if(k == 0){
					k = him;
					dest++;
				}
				continue;
			}
			e += e1;
			if(k == 0){
				k = him;
				*dest &= m|~kk;
				dest++;
				kk = 0;
			}
		}
		if(kk)
			*dest &= m|~kk;
		return;

	case 1:
		kk = 0;
		for(i=dx; i>=0; i--){
			kk |= k;
			PSHIFT(k, bpp);
			if(e >= 0){
				t = *dest;
				*dest = (kk ^ t) | (t & m);
				dest += dw;
				e += e2;
				kk = 0;
				if(k == 0){
					k = him;
					dest++;
				}
				continue;
			}
			e += e1;
			if(k == 0){
				t = *dest;
				*dest = (kk ^ t) | (t & m);
				dest++;
				k = him;
				kk = 0;
			}
		}
		if(kk){
			t = *dest;
			*dest = (kk ^ t) | (t & m);
		}
		return;

	case 2:
		kk = 0;
		kk0 = 0;
		kk1 = 0;
		for(i=dx; i>=0; i--){
			kk |= k;
			kk0 |= k0;
			kk1 |= k1;
			PSHIFT(k, bpp);
			PSHIFT(k0, bpp);
			PSHIFT(k1, bpp);
			if(e >= 0){
				t = *dest;
				if(f0 == 0)
					t &= m0|~kk0;
				else
					t = (kk0 ^ t) | (t & m0);
				if(f1 == 0)
					*dest = t & (m1|~kk1);
				else
					*dest = (kk1 ^ t) | (t & m1);
				dest += dw;
				e += e2;
				kk = 0;
				kk0 = 0;
				kk1 = 0;
				if(k == 0){
					k = him;
					k0 = him0;
					k1 = him1;
					dest++;
				}
				continue;
			}
			e += e1;
			if(k == 0){
				t = *dest;
				if(f0 == 0)
					t &= m0|~kk0;
				else
					t = (kk0 ^ t) | (t & m0);
				if(f1 == 0)
					*dest = t & (m1|~kk1);
				else
					*dest = (kk1 ^ t) | (t & m1);
				dest++;
				k = him;
				k0 = him0;
				k1 = him1;
				kk = 0;
				kk0 = 0;
				kk1 = 0;
			}
		}
		if(kk){
			t = *dest;
			if(f0 == 0)
				t &= m0|~kk0;
			else
				t = (kk0 ^ t) | (t & m0);
			if(f1 == 0)
				*dest = t & (m1|~kk1);
			else
				*dest = (kk1 ^ t) | (t & m1);
		}
		return;
	}
	return;

majory:
	e = _setdda(p.y, p.x, &l);
	switch (f){
	case 0:
		for(i=dy; i>=0; --i){
			*dest &= m|~k;
			dest += dw;
			if(e >= 0){
				PSHIFT(k, bpp);
				if(k == 0){
					dest++;
					k = him;
				}
				e += e2;
			}else
				e += e1;
		}
		return;

	case 1:
		for(i=dy; i>=0; --i){
			t = *dest;
			*dest = (k ^ t) | (t & m);
			dest += dw;
			if(e >= 0){
				PSHIFT(k, bpp);
				if(k == 0){
					dest++;
					k = him;
				}
				e += e2;
			}else
				e += e1;
		}
		return;

	case 2:
		for(i=dy; i>=0; --i){
			t = *dest;
			if(f0 == 0)
				t &= m0|~k0;
			else
				t = (k0 ^ t) | (t & m0);
			if(f1 == 0)
				*dest = t & (m1|~k1);
			else
				*dest = (k1 ^ t) | (t & m1);
			dest += dw;
			if(e >= 0){
				PSHIFT(k, bpp);
				PSHIFT(k0, bpp);
				PSHIFT(k1, bpp);
				if(k == 0){
					dest++;
					k = him;
					k0 = him0;
					k1 = him1;
				}
				e += e2;
			}else
				e += e1;
		}
	}
	return;
}

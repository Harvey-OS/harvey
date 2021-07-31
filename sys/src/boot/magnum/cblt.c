#include <u.h>
#include <libg.h>
#include <gnot.h>

#define WSHIFT	5
#define WMASK	0x1F
#define WSIZE	32

#define S	0xC
#define D	0xA
#define X	0xF

#define ltor(tag,expr)	case F&(tag):\
	for (j = 0; j < dy; j++){\
		mask = lmask;\
		a = *s++;\
		for (i = nw+1; i > 0; i--){\
			b = *s++;\
			m = (a<<ls) | (b>>rs);\
			a = b;\
			expr;\
			d++;\
			mask = ONES;\
			if(i == 2)\
				mask = rmask;\
		}\
		s += sw;\
		d += dw;\
	}\
	break

#define wltor(tag,expr)	case F&(tag):\
	for (j = 0; j < dy; j++){\
		mask = lmask;\
		for (i = nw+1; i > 0; i--){\
			m = *s++;\
			expr;\
			d++;\
			mask = ONES;\
			if(i == 2)\
				mask = rmask;\
		}\
		s += sw+1;\
		d += dw;\
	}\
	break

void
gbitblt(GBitmap *dm,Point p,GBitmap *sm,Rectangle r,Fcode f)
{
	ulong *s, *d;
	ulong mask, a, b, m, ls, rs;
	ulong lmask, rmask;
	int i, j, qx, sw, dw, dy, nw;

	gbitbltclip(&dm);
	qx = p.x + (r.max.x-r.min.x) - 1;
	lmask = ONES >> (p.x&WMASK);
	rmask = ONES << (WMASK-qx&WMASK);
	ls = (r.min.x-p.x)&WMASK;
	rs = WSIZE - ls;
	nw = (qx>>WSHIFT) - (p.x>>WSHIFT);
	dy = r.max.y - r.min.y;
	sw = sm->width - nw - 2;
	dw = dm->width - nw - 1;

	if(sm == dm){
		if(r.min.y < p.y){	/* bottom to top */
			r.min.y += dy-1;
			p.y += dy-1;
			sw -= 2*sm->width;
			dw -= 2*dm->width;
		}
		else if(r.min.y == p.y && r.min.x < p.x)
			goto right;
	}
	r.min.x -= p.x&WMASK;	/* adjust for first destination word */
	s = gaddr(sm,r.min);
	d = gaddr(dm,p);
	if(nw == 0)		/* collapse masks for narrow cases */
		lmask &= rmask;
	f &= F;

	if(ls&WMASK) switch (f){		/* ltor bit aligned */
		ltor(0,		*d &= ~mask);
		ltor(~(D|S),	*d ^= ((~m |  *d) & mask));
		ltor(D&~S,	*d ^= (( m &  *d) & mask));
		ltor(~S,	*d ^= ((~m ^  *d) & mask));
		ltor(~D&S,	*d ^= (( m |  *d) & mask));
		ltor(~D,	*d ^= mask);
		ltor(D^S,	*d ^= (m & mask));
		ltor(~(D&S),	*d ^= (( m | ~*d) & mask));
		ltor(D&S,	*d ^= ((~m &  *d) & mask));
		ltor(~(D^S),	*d ^= (~m & mask));
		ltor(D,		*d);
		ltor(D|~S,	*d |= (~m & mask));
		ltor(S,		*d ^= (( m ^ *d) & mask));
		ltor(~D|S,	*d ^= (~(m & *d) & mask));
		ltor(D|S,	*d |= (m & mask));
		ltor(X,		*d |= mask);
	}
	else switch (f){		/* word aligned */
		wltor(0,	*d &= ~mask);
		wltor(~(D|S),	*d ^= ((~m |  *d) & mask));
		wltor(D&~S,	*d ^= (( m &  *d) & mask));
		wltor(~S,	*d ^= ((~m ^  *d) & mask));
		wltor(~D&S,	*d ^= (( m |  *d) & mask));
		wltor(~D,	*d ^= mask);
		wltor(D^S,	*d ^= (m & mask));
		wltor(~(D&S),	*d ^= (( m | ~*d) & mask));
		wltor(D&S,	*d ^= ((~m &  *d) & mask));
		wltor(~(D^S),	*d ^= (~m & mask));
		wltor(D,	*d);
		wltor(D|~S,	*d |= (~m & mask));
		wltor(S,	*d ^= (( m ^ *d) & mask));	/* scrolling! */
		wltor(~D|S,	*d ^= (~(m & *d) & mask));
		wltor(D|S,	*d |= (m & mask));
		wltor(X,	*d |= mask);
	}

	return;
}

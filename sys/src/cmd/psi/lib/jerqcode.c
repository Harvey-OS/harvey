#include <u.h>
#include <libc.h>
#include "system.h"
#ifndef Plan9
#include "stdio.h"
#include "njerq.h"
#include "object.h"

char *beg_mem;

Bitmap *
balloc(Rectangle r, int i)
{
	register Bitmap *b;
	int j = i+1;
	b = (Bitmap *) vm_alloc(sizeof(Bitmap));
	if(b == (Bitmap *)NULL){
		fprintf(stderr,"failed to allocate bitmap\n");
		pserror("big problems", "balloc");
	}
	b->r = r;
	b->width = ((r.max.x + 31) >> 5) - (r.min.x >> 5);
	b->base = (Word *) vm_alloc(j*b->width*(r.max.y-r.min.y)*4);
	if(b->base == (Word *)NULL){
		fprintf(stderr,"failed to allocate bitmap base %d\n",
			b->width*(r.max.y-r.min.y)*4);
		pserror("big problems", "balloc");
	}
	rectf(b, b->r, Zero);
	return(b);
}
static int fcount;
static int nomore=0;
Bitmap *
cachealloc(Rectangle r)
{
	register Bitmap *b;
	fcount++;
	if(nomore)
		return(0);
	b = (Bitmap *) malloc(sizeof(Bitmap));
	if((char *)b >=beg_mem){
		fprintf(stderr,"ran out of cache memory %d fonts\n",fcount);
		nomore = 1;
		return(0);
	}
	b->r = r;
	b->width = ((r.max.x + 31) >> 5) - (r.min.x >> 5);
	b->base = (Word *) malloc(b->width*(r.max.y-r.min.y)*4);
	if((char *)b->base >= beg_mem){
		fprintf(stderr,"ran out of cache memory %d\n",b->width*(r.max.y-r.min.y)*4);
		nomore = 1;
		return(0);
	}
	rectf(b, b->r, Zero);
	return(b);
}

#define addr(b,p) b->base + (b->width*(p.y-b->r.min.y) + (((p.x>>5)<<5)/32) - (b->r.min.x>>5))

#ifdef AHMDAL
#define DOIT(src,roff,xmask,shift) ((*src<<roff)|((*(src+1)>>shift)&xmask))
#endif
#ifdef SUN
#define DOIT(src,roff,xmask,shift) ((*src<<roff)|((*(src+1)>>shift)&xmask))
#endif
#ifdef I386
		/* deal with inability to shift by 32 */
#define DOIT(src,roff,xmask,shift) (shift==32?((*src>>roff)&xmask):(roff==32?(*(src+1)<<shift):(((*src>>roff)&xmask)|(*(src+1)<<shift))))
#endif
#ifndef DOIT
#define DOIT(src,roff,xmask,shift) (((*src>>roff)&xmask)|(*(src+1)<<shift))
#endif

#ifndef VAX
Rmask[33] = {
	 037777777777,	      020000000000, 030000000000,
	034000000000, 036000000000, 037000000000,
	037400000000, 037600000000, 037700000000,
	037740000000, 037760000000, 037770000000,
	037774000000, 037776000000, 037777000000,
	037777400000, 037777600000, 037777700000,
	037777740000, 037777760000, 037777770000,
	037777774000, 037777776000, 037777777000,
	037777777400, 037777777600, 037777777700,
	037777777740, 037777777760, 037777777770,
	037777777774, 037777777776, 037777777777 };
Lmask[33] = {
	037777777777, 017777777777, 007777777777,
	003777777777, 001777777777, 000777777777,
	000377777777, 000177777777, 000077777777,
	000037777777, 000017777777, 000007777777,
	000003777777, 000001777777, 000000777777,
	000000377777, 000000177777, 000000077777,
	000000037777, 000000017777, 000000007777,
	000000003777, 000000001777, 000000000777,
	000000000377, 000000000177, 000000000077,
	000000000037, 000000000017, 000000000007,
	000000000003, 1, 0};
#else
long Lmask[] = {
	0xffffffff, 0xfffffffe, 0xfffffffc, 0xfffffff8,
	0xfffffff0, 0xffffffe0, 0xffffffc0, 0xffffff80,
	0xffffff00, 0xfffffe00, 0xfffffc00, 0xfffff800,
	0xfffff000, 0xffffe000, 0xffffc000, 0xffff8000,
	0xffff0000, 0xfffe0000, 0xfffc0000, 0xfff80000,
	0xfff00000, 0xffe00000, 0xffc00000, 0xff800000,
	0xff000000, 0xfe000000, 0xfc000000, 0xf8000000,
	0xf0000000, 0xe0000000, 0xc0000000, 0x80000000
};
long Rmask[] = {
	0xffffffff, 0x00000001, 0x00000003, 0x00000007,
	0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
	0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
	0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
	0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
	0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
	0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
	0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff
};
mask[32] = {037777777777,017777777777, 07777777777,
	03777777777, 01777777777,0777777777,
	0377777777, 0177777777,077777777,
	037777777, 017777777,07777777,
	03777777, 01777777,0777777,
	0377777, 0177777,077777,
	037777, 017777,07777,
	03777, 01777,0777,
	0377, 0177,077,
	037, 017,07,
	03, 01};
#endif
void
bitblt(Bitmap *dm, Point p, Bitmap *sm, Rectangle r, Fcode f)
{
	register i,j,roff,m;	/* r11, r10, r9, r8 */
	register long *source,*dest;	/* r7, r6 */
	register long lmask,rmask;
	register long xmask;
	int shift;
	int nw,w,h,sw,dw,left=0;
	long *lsource,*ldest;
	register Bitmap *b = dm;
	Point dp;
	w = r.max.x - r.min.x;
	h = r.max.y - r.min.y;
	if(w <= 0 || h <= 0){
		if(mdebug)fprintf(stderr,"bad bitblt (%d %d) to (%d %d)\n",
			r.min.x,r.min.y,r.max.x,r.max.y);
		return;
	}
	dp = Pt(p.x+w,p.y+h);
	/* clip to destination bitmap */
	if ((i = p.x - b->r.min.x) < 0) {
		p.x -= i;
		r.min.x -= i;
	}
	if ((i = p.y - b->r.min.y) < 0) {
		p.y -= i;
		r.min.y -= i;
	}
	if ((i = b->r.max.x - dp.x) < 0)
		r.max.x += i;
	if ((i = b->r.max.y - dp.y) < 0)
		r.max.y += i;
	/* recompute loop numbers */
	w = r.max.x - r.min.x;
	h = r.max.y - r.min.y;
	nw = ((p.x+w-1)>>5) - (p.x>>5) - 1;
	lmask = Lmask[p.x & 0x1f];
	rmask = Rmask[(p.x+w) & 0x1f];
	sw = sm->width;
	dw = dm->width;
	if (sm == dm) {		/* may have to change loop order */
		if (r.min.y < p.y) {
			r.min.y += h-1;
			p.y += h-1;
			sw = -sw;
			dw = -dw;
		}
		if (r.min.x < p.x) {
			left = 1;
			r.min.x += w-1;
			p.x += w-1;
		}
	}
	roff = (r.min.x&0x1f) - (p.x&0x1f);
	if (roff < 0) {
		roff += 32;
		r.min.x -= 32;
	}
	shift = 32-roff;
#ifndef VAX
	xmask = Lmask[shift];
#else
	xmask = mask[roff];
#endif
	if (sm != dm && f == (S|D)) {
		if (nw < 0) {
			lmask &= rmask;
			source = addr(sm,r.min);
			dest = addr(dm,p);
			for (i = 0; i < h; i++) {
				m = DOIT(source,roff,xmask,shift);
				*dest |= lmask&m;
				dest += dw;
				source += sw;
			}
		}
		else {
			lsource = addr(sm,r.min);
			ldest = addr(dm,p);
			for (i = 0; i < h; i++, lsource += sw, ldest += dw) {
				source = lsource;
				dest = ldest;
				m = DOIT(source,roff,xmask,shift);
				source++;
				*dest |= lmask&m;
				dest++;
				for (j = 0; j < nw; j++) {
				m = DOIT(source,roff,xmask,shift);
					source++;
					*dest++ |= m;
				}
				m = DOIT(source,roff,xmask,shift);
				*dest |= rmask&m;
			}
		}
		return;
	}
	if (nw < 0) {
		lmask &= rmask;
		rmask = ~lmask;
		source = addr(sm,r.min);
		dest = addr(dm,p);
		for (i = 0; i < h; i++) {
			m = DOIT(source,roff,xmask,shift);
			switch(f) {
			case (S|D):
				*dest |= lmask&m;
				break;
			case DandnotS:
				*dest &= ~(lmask&m);
				break;
			case S:
				*dest = lmask&m | rmask&*dest;
			}
			dest += dw;
			source += sw;
		}
	}
	else if (left == 0) {
		lsource = addr(sm,r.min);
		ldest = addr(dm,p);
		for (i = 0; i < h; i++, lsource += sw, ldest += dw) {
			source = lsource;
			dest = ldest;
			m = DOIT(source,roff,xmask,shift);
			source++;
			switch(f) {
			case (S|D):
				*dest |= lmask&m;
				break;
			case DandnotS:
				*dest &= ~(lmask&m);
				break;
			case S:
				*dest = lmask&m | (~lmask)&*dest;
			}
			dest++;
			for (j = 0; j < nw; j++) {
				m = DOIT(source,roff,xmask,shift);
				source++;
				switch(f) {
				case (S|D):
					*dest++ |= m;
					break;
				case DandnotS:
					*dest++ &= ~m;
					break;
				case S:
					*dest++ = m;
				}
			}
			m = DOIT(source,roff,xmask,shift);
			switch(f) {
			case (S|D):
				*dest |= rmask&m;
				break;
			case DandnotS:
				*dest &= ~(rmask&m);
				break;
			case S:
				*dest = rmask&m | (~rmask)&*dest;
			}
		}
	}
	else {
		lsource = addr(sm,r.min);
		ldest = addr(dm,p);
		for (i = 0; i < h; i++, lsource += sw, ldest += dw) {
			source = lsource;
			dest = ldest;
			m = DOIT(source,roff,xmask,shift);
			source--;
			switch(f) {
			case (S|D):
				*dest |= rmask&m;
				break;
			case DandnotS:
				*dest &= ~(rmask&m);
				break;
			case S:
				*dest = rmask&m | (~rmask)&*dest;
			}
			for (j = 0; j < nw; j++) {
				m = DOIT(source,roff,xmask,shift);
				source--;
				switch(f) {
				case (S|D):
					*--dest |= m;
					break;
				case DandnotS:
					*--dest &= ~m;
					break;
				case S:
					*--dest = m;
				}
			}
			m = DOIT(source,roff,xmask,shift);
			dest--;
			switch(f) {
			case (S|D):
				*dest |= lmask&m;
				break;
			case DandnotS:
				*dest &= ~(lmask&m);
				break;
			case S:
				*dest = lmask&m | (~lmask)&*dest;
			}
		}
	}
}

Texture zeros;
Texture ones = {
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
};
Texture	grey[] =
{

	0xFFFFFFFF , 0xFFFFFFFF , 0xFFFFFFFF , 0xFFFFFFFF ,
	0xFFFFFFFF , 0xFFFFFFFF , 0xFFFFFFFF , 0xFFFFFFFF ,
	0xFFFFFFFF , 0xFFFFFFFF , 0xFFFFFFFF , 0xFFFFFFFF ,
	0xFFFFFFFF , 0xFFFFFFFF , 0xFFFFFFFF , 0xFFFFFFFF ,
	0xFFFFFFFF , 0xFFFFFFFF , 0xFFFFFFFF , 0xFFFFFFFF ,
	0xFFFFFFFF , 0xFFFFFFFF , 0xFFFFFFFF , 0xFFFFFFFF ,
	0xFFFFFFFF , 0xFFFFFFFF , 0xFFFFFFFF , 0xFFFFFFFF ,
	0xFFFFFFFF , 0xFFFFFFFF , 0xFFFFFFFF , 0xFFFFFFFF ,

	0xFFFFFFFF , 0x55555555 , 0xFFFFFFFF , 0x55555555 ,
	0xFFFFFFFF , 0x55555555 , 0xFFFFFFFF , 0x55555555 ,
	0xFFFFFFFF , 0x55555555 , 0xFFFFFFFF , 0x55555555 ,
	0xFFFFFFFF , 0x55555555 , 0xFFFFFFFF , 0x55555555 ,
	0xFFFFFFFF , 0x55555555 , 0xFFFFFFFF , 0x55555555 ,
	0xFFFFFFFF , 0x55555555 , 0xFFFFFFFF , 0x55555555 ,
	0xFFFFFFFF , 0x55555555 , 0xFFFFFFFF , 0x55555555 ,
	0xFFFFFFFF , 0x55555555 , 0xFFFFFFFF , 0x55555555 ,

	0xAAAAAAAA , 0x55555555 , 0xAAAAAAAA , 0x55555555 ,
	0xAAAAAAAA , 0x55555555 , 0xAAAAAAAA , 0x55555555 ,
	0xAAAAAAAA , 0x55555555 , 0xAAAAAAAA , 0x55555555 ,
	0xAAAAAAAA , 0x55555555 , 0xAAAAAAAA , 0x55555555 ,
	0xAAAAAAAA , 0x55555555 , 0xAAAAAAAA , 0x55555555 ,
	0xAAAAAAAA , 0x55555555 , 0xAAAAAAAA , 0x55555555 ,
	0xAAAAAAAA , 0x55555555 , 0xAAAAAAAA , 0x55555555 ,
	0xAAAAAAAA , 0x55555555 , 0xAAAAAAAA , 0x55555555 ,

	0xAAAAAAAA , 0x00000000 , 0xAAAAAAAA , 0x00000000 ,
	0xAAAAAAAA , 0x00000000 , 0xAAAAAAAA , 0x00000000 ,
	0xAAAAAAAA , 0x00000000 , 0xAAAAAAAA , 0x00000000 ,
	0xAAAAAAAA , 0x00000000 , 0xAAAAAAAA , 0x00000000 ,
	0xAAAAAAAA , 0x00000000 , 0xAAAAAAAA , 0x00000000 ,
	0xAAAAAAAA , 0x00000000 , 0xAAAAAAAA , 0x00000000 ,
	0xAAAAAAAA , 0x00000000 , 0xAAAAAAAA , 0x00000000 ,
	0xAAAAAAAA , 0x00000000 , 0xAAAAAAAA , 0x00000000 ,

	0x00000000 , 0x00000000 , 0x00000000 , 0x00000000 ,
	0x00000000 , 0x00000000 , 0x00000000 , 0x00000000 ,
	0x00000000 , 0x00000000 , 0x00000000 , 0x00000000 ,
	0x00000000 , 0x00000000 , 0x00000000 , 0x00000000 ,
	0x00000000 , 0x00000000 , 0x00000000 , 0x00000000 ,
	0x00000000 , 0x00000000 , 0x00000000 , 0x00000000 ,
	0x00000000 , 0x00000000 , 0x00000000 , 0x00000000 ,
	0x00000000 , 0x00000000 , 0x00000000 , 0x00000000 ,
};

void	/*general modified to only do STORE or CLEAR*/
texture(Bitmap *b, Rectangle r, Texture t, Fcode f)
{
	register i,j;
	register long lmask,rmask;
	register long *dest,*tp;
	int w,h;
	long *ldest;
	lmask = Lmask[r.min.x & 0x1f];
	rmask = Rmask[r.max.x & 0x1f];
	i = r.max.x-r.min.x;
	w = ((r.max.x-1)>>5) - (r.min.x>>5) - 1;
	h = r.max.y - r.min.y;
	if(i <= 0 || h <= 0 ){
		if(mdebug)fprintf(stderr,"bad texture (%d %d) (%d %d)\n",
			r.min.x,r.min.y,r.max.x,r.max.y);
		return;
	}
	if(f == (S|D))f = S;
	tp = &t[r.min.y & 0x1f];
	if (w < 0) {
		lmask &= rmask;
		rmask = ~lmask;
		dest = addr(b,r.min);
		for (i = 0; i < h; i++) {
			if(f == S)*dest = lmask&*tp++ | rmask&*dest;
			else *dest &= ~(lmask&*tp++);
			dest += b->width;
			if (tp == &t[32])
				tp = t;
		}
	}
	else {
		ldest = addr(b,r.min);
		for (i = 0; i < h; i++, ldest += b->width) {
			dest = ldest;
			if(f == S)*dest = lmask&*tp | (~lmask)&*dest;
			else *dest &= ~(lmask&*tp);
			dest++;
			for (j = 0; j < w; j++)
				if(f == S)*dest++ = *tp;
				else *dest++ &= ~*tp;
			if(f == S)*dest = rmask&*tp | (~rmask)&*dest;
			else *dest &= ~(rmask&*tp);
			if (++tp == &t[32])
				tp = t;
		}
	}
}

Rectangle
Rect(int x, int y, int u, int v)
{
	Rectangle r;
	r.min.x = x;
	r.min.y = y;
	r.max.x = u;
	r.max.y = v;
	return(r);
}
Point
Pt(int x, int y)
{
	Point p;
	p.x = x;
	p.y = y;
	return(p);
}
#ifdef VAX
long bit[] = {
	0x00000001, 0x00000002, 0x00000004, 0x00000008,
	0x00000010, 0x00000020, 0x00000040, 0x00000080,
	0x00000100, 0x00000200, 0x00000400, 0x00000800,
	0x00001000, 0x00002000, 0x00004000, 0x00008000,
	0x00010000, 0x00020000, 0x00040000, 0x00080000,
	0x00100000, 0x00200000, 0x00400000, 0x00800000,
	0x01000000, 0x02000000, 0x04000000, 0x08000000,
	0x10000000, 0x20000000, 0x40000000, 0x80000000,
};
#endif
#ifdef AHMDAL
long bit[] = {
	0x80000000, 0x40000000, 0x20000000, 0x10000000,
	0x08000000, 0x04000000, 0x02000000, 0x01000000,
	0x00800000, 0x00400000, 0x00200000, 0x00100000,
	0x00080000, 0x00040000, 0x00020000, 0x00010000,
	0x00008000, 0x00004000, 0x00002000, 0x00001000,
	0x00000800, 0x00000400, 0x00000200, 0x00000100,
	0x00000080, 0x00000040, 0x00000020, 0x00000010,
	0x00000008, 0x00000004, 0x00000002, 0x00000001
};
#endif
#ifdef SUN
long bit[] = {
	0x01000000, 0x02000000, 0x04000000, 0x08000000,
	0x10000000, 0x20000000, 0x40000000, 0x80000000,
	0x00010000, 0x00020000, 0x00040000, 0x00080000,
	0x00100000, 0x00200000, 0x00400000, 0x00800000,
	0x00000100, 0x00000200, 0x00000400, 0x00000800,
	0x00001000, 0x00002000, 0x00004000, 0x00008000,
	0x00000001, 0x00000002, 0x00000004, 0x00000008,
	0x00000010, 0x00000020, 0x00000040, 0x00000080,
};
#endif
long *
faddr(register Bitmap *b, Point p)
{
	return(b->base + (b->width*(p.y-b->r.min.y) + (((p.x>>5)<<5)/32) - (b->r.min.x>>5)));
}
void
point(Bitmap *b, Point p, int i, Fcode f)
{
	switch (f) {
	case S:
	case D:
		*faddr(b,p) |= bit[p.x&31];
		break;
	case Zero:		/*XOR*/
		*faddr(b,p) ^= bit[p.x&31];
		break;
	case DandnotS:
		*faddr(b,p) &= ~bit[p.x&31];
		break;
	}
}
#endif

#include <u.h>
#include <libg.h>
#include <gnot.h>
#include "tabs.h"

#define swizbytes(a) (a)
#ifdef T386
#define LENDIAN
#endif
#ifdef Thobbit
#define LENDIAN
#endif

/*
 * texture - uses bitblt, logarithmically if f doesn't involve D
 * but special case some textures whose rows can be packed in 32 bits
 */

void
gtexture(GBitmap *dm, Rectangle r, GBitmap *sm, Fcode f)	
{
	Point p, dp;
	int x, y, d, w, hm, wneed, sld, dld;
	ulong a, b, lmask, rmask, *ps, *pe, *pp, s[32];
	Rectangle savdr;

	if(rectclip(&r, dm->clipr) == 0)
		return;
	dp = sub(sm->clipr.max, sm->clipr.min);
	if(dp.x==0 || dp.y==0)
		return;
	p.x = r.min.x - (r.min.x % dp.x);
	p.y = r.min.y - (r.min.y % dp.y);
	f &= 0xF;
	if(f==S || f==notS){
		sld = sm->ldepth;
		dld = dm->ldepth;
		w = dp.x << sld;
		wneed = (dld == sld) ? 32 : 32 >> dld;
		if((sld == 0 || sld == dld) &&
		   sm->clipr.min.x == 0 && sm->r.min.x == 0 &&
		   sm->clipr.min.y == 0 && sm->r.min.y == 0 &&
		   w <= wneed && wneed%w == 0 &&
		   dp.y <= 32 && (dp.y&(dp.y-1))==0) {
			/* 32-bit word tiling; replicate/convert rows into s[] */
			for(y = 0; y < dp.y; y++) {
				a = swizbytes(sm->base[y]);	/* we know sm rows fit in a word */
				if(w < wneed) {
#ifdef LENDIAN
					b = a;
					for(x = w; x < 32; x += w)
						a |= b << x;
#else
					b = a >> (32-w);
					for(x = w; x < 32; x += w)
						a |= b << (32-x-w);
#endif
				}
				if(sld != dld)
					switch(dld) {
					case 1:
						a = (tab01[a>>24]<<16) |
						    tab01[(a>>16)&0xFF];
						break;
					case 2:
						a = tab02[a>>24];
						break;
					case 3:
						a = tab03[a>>28];
						break;
					default:
						return;	/* don't handle ldepth > 3 yet */
					}
				if(f == notS)
					a = ~a;
				s[y] = swizbytes(a);
			}
			a = (r.min.x<<dld)&31;
#ifdef LENDIAN
			lmask = ~0UL << a;
			rmask = ~0UL >> (32-((r.max.x<<dld))&31);
#else
			lmask = ~0UL >> a;
			rmask = ~0UL << (32-((r.max.x<<dld))&31);
#endif
			if(!rmask)
				rmask = ~0;
			ps = gaddr(dm, r.min);
			pe = gaddr(dm, Pt(r.max.x-1, r.min.y));
			lmask = swizbytes(lmask);
			rmask = swizbytes(rmask);
			if(pe == ps)
				lmask &= rmask;
			w = dm->width;
			hm = dp.y-1;	/* we know dp.y is a power of 2 */
			for(y = r.min.y; y < r.max.y; y++) {
				a = s[y&hm];
				b = *ps;
				*ps = ((a^b)&lmask)^b;
				if(ps < pe) {
					for(pp = ps+1; pp<pe; )
						*pp++ = a;
					b = *pe;
					*pe = ((a^b)&rmask)^b;
				}
				ps += w;
				pe += w;
			}
		} else {
			/* logarithmic tiling */
			savdr = dm->clipr;
			rectclip(&dm->clipr, r);
			if(!eqpt(p, r.min)){
				gbitblt(dm, p, sm, sm->clipr, f);
				d = dp.x;
				gbitblt(dm, add(p, Pt(d, 0)), sm, sm->clipr, f);
				for(x=p.x+d; x+d<r.max.x; d+=d)
					gbitblt(dm, Pt(x+d, p.y), dm, Rect(x, p.y, x+d, p.y+dp.y), S);
				d = dp.y;
				gbitblt(dm, add(p, Pt(0, d)), sm, sm->clipr, f);
				for(y=p.y+d; y+d<r.max.y; d+=d)
					gbitblt(dm, Pt(p.x, y+d), dm, Rect(p.x, y, p.x+dp.x, y+d), S);
				p = add(p, dp);
			}
			gbitblt(dm, p, sm, sm->clipr, f);
			for(d=dp.x; p.x+d<r.max.x; d+=d)
				gbitblt(dm, Pt(p.x+d, p.y), dm, Rect(p.x, p.y, p.x+d, p.y+dp.y), S);
			for(d=dp.y; p.y+d<r.max.y; d+=d)
				gbitblt(dm, Pt(p.x, p.y+d), dm, Rect(p.x, p.y, r.max.x, p.y+d), S);
			dm->clipr = savdr;
		}
	}else{
		savdr = dm->clipr;
		rectclip(&dm->clipr, r);
		for(y=p.y; y<r.max.y; y+=dp.y)
			for(x=p.x; x<r.max.x; x+=dp.x)
				gbitblt(dm, Pt(x, y), sm, sm->clipr, f);
		dm->clipr = savdr;
	}
}

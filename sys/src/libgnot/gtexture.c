#include <u.h>
#include <libg.h>
#include <gnot.h>
#include "tabs.h"

#define swizbytes(a) (a)

/* a = s (func(f)) d, where f involves d */
#define fcodefun(a,s,d,f) switch(f) {\
	case DnorS:	a = ~((d)|(s));	break; \
	case DandnotS:	a = (d)&~(s);	break; \
	case notDandS:	a = ~(d)&(s);	break; \
	case notD:	a = ~(d);	break; \
	case DxorS:	a = (d)^(s);	break; \
	case DnandS:	a = ~((d)&(s));	break; \
	case DandS:	a = (d)&(s);	break; \
	case DxnorS:	a = ~((d)^(s));	break; \
	case D:		a = d;		break; \
	case DornotS:	a = (d)|~(s);	break; \
	case notDorS:	a = ~(d)|(s);	break; \
	case DorS:	a = (d)|(s);	break; \
	default:	a = d;		break; \
	}
/*
 * texture - uses bitblt, logarithmically if f doesn't involve D
 * but special case some textures whose rows can be packed in 32 bits
 */

void
gtexture(GBitmap *dm, Rectangle r, GBitmap *sm, Fcode f)	
{
	Point p, dp;
	int x, y, d, w, hm, wneed, sld, dld, dunused, sunused, fconst;
	ulong a, b, c, lmask, rmask, *ps, *pe, *pp, s[32];
	Rectangle savdr;

	if(rectclip(&r, dm->clipr) == 0)
		return;
	dp = sub(sm->clipr.max, sm->clipr.min);
	if(dp.x==0 || dp.y==0 || Dx(r)==0 || Dy(r)==0)
		return;
	p.x = r.min.x - (r.min.x % dp.x);
	p.y = r.min.y - (r.min.y % dp.y);
	f &= 0xF;
	sld = sm->ldepth;
	dld = dm->ldepth;
	w = dp.x << sld;
	wneed = (dld == sld) ? 32 : 32 >> dld;
	fconst = (f==F || f==Zero);
	sunused = (f==D || f==notD || fconst);
	dunused = (f==S || f==notS || fconst);
	if(sunused ||
	   ((sld == 0 && !LENDIAN || sld == dld) &&
	   sm->clipr.min.x == 0 && sm->r.min.x == 0 &&
	   sm->clipr.min.y == 0 && sm->r.min.y == 0 &&
	   w <= wneed && wneed%w == 0 &&
	   dp.y <= 32 && (dp.y&(dp.y-1))==0)) {
		/* 32-bit word tiling; replicate/convert rows into s[] */
		if (sunused) {
			dp.y = 1;
			s[0] = (f==F)? ~0 : 0;
		} else for(y = 0; y < dp.y; y++) {
			/* we know sm rows fit in a word */
			a = swizbytes(sm->base[y]);
			if(w < wneed) {
				if(LENDIAN){
					b = a;
					for(x = w; x < 32; x += w)
						a |= b << x;
				}else{
					b = a >> (32-w);
					for(x = w; x < 32; x += w)
						a |= b << (32-x-w);
				}
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
					/* don't do ldepth > 3 yet */
					return;
				}
			if(f == notS)
				a = ~a;
			if(!dunused) {
				/* rotate right to get aligned */
				b = (p.x << dld) % 32;
				if(b)
					a = (a >> b) | (a << (32-b));
			}
			s[y] = swizbytes(a);
		}
		a = (r.min.x<<dld)&31;
		if(LENDIAN){
			lmask = ~0UL << a;
			rmask = ~0UL >> (32-((r.max.x<<dld))&31);
		}else{
			lmask = ~0UL >> a;
			rmask = ~0UL << (32-((r.max.x<<dld))&31);
		}
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
		if (dunused) {
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
			for(y = r.min.y; y < r.max.y; y++) {
				a = s[y&hm];
				b = *ps;
				fcodefun(c,a,b,f)
				*ps = ((c^b)&lmask)^b;
				if(ps < pe) {
					for(pp = ps+1; pp<pe; ) {
						b = *pp;
						fcodefun(c,a,b,f)
						*pp++ = c;
					}
					b = *pe;
					fcodefun(c,a,b,f)
					*pe = ((c^b)&rmask)^b;
				}
				ps += w;
				pe += w;
			}
		}
	} else if(dunused){
		/* logarithmic tiling */
		savdr = dm->clipr;
		rectclip(&dm->clipr, r);
		if(!eqpt(p, r.min)){
			gbitblt(dm, p, sm, sm->clipr, f);
			d = dp.x;
			x = p.x+d;
			if(x<r.max.x){
				gbitblt(dm, Pt(x, p.y), sm, sm->clipr, f);
				for(; x+d<r.max.x; d+=d)
					gbitblt(dm, Pt(x+d, p.y), dm,
						Rect(x, p.y, x+d, p.y+dp.y), S);
			}
			d = dp.y;
			y = p.y+d;
			if(y<r.max.y){
				gbitblt(dm, Pt(p.x,y), sm, sm->clipr, f);
				for(; y+d<r.max.y; d+=d)
					gbitblt(dm, Pt(p.x, y+d), dm,
						Rect(p.x, y, p.x+dp.x, y+d), S);
			}
			p = add(p, dp);
		}
		if(p.x<r.max.x && p.y<r.max.y){
			gbitblt(dm, p, sm, sm->clipr, f);
			for(d=dp.x; p.x+d<r.max.x; d+=d)
				gbitblt(dm, Pt(p.x+d, p.y), dm,
					Rect(p.x, p.y, p.x+d, p.y+dp.y), S);
			for(d=dp.y; p.y+d<r.max.y; d+=d)
				gbitblt(dm, Pt(p.x, p.y+d), dm,
					Rect(p.x, p.y, r.max.x, p.y+d), S);
		}
		dm->clipr = savdr;
	}else{
		savdr = dm->clipr;
		rectclip(&dm->clipr, r);
		for(y=p.y; y<r.max.y; y+=dp.y)
			for(x=p.x; x<r.max.x; x+=dp.x)
				gbitblt(dm, Pt(x, y), sm, sm->clipr, f);
		dm->clipr = savdr;
	}
}

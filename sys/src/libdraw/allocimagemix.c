#include <u.h>
#include <libc.h>
#include <draw.h>

Image*
allocimagemix(Display *d, ulong color1, ulong color3)
{
	Image *t, *b;
	static Image *qmask;

	if(qmask == nil)
		qmask = allocimage(d, Rect(0,0,1,1), GREY8, 1, 0x3F3F3FFF);
		
	if(d->depth <= 8){	/* create a 2×2 texture */
		t = allocimage(d, Rect(0,0,1,1), display->chan, 0, color1);
		if(t == nil)
			return nil;

		b = allocimage(d, Rect(0,0,2,2), display->chan, 1, color3);
		if(b == nil){
			freeimage(t);
			return nil;
		}

		draw(b, Rect(0,0,1,1), t, nil, ZP);
		freeimage(t);
		return b;
	}else{	/* use a solid color, blended using alpha */
		t = allocimage(d, Rect(0,0,1,1), display->chan, 1, color1);
		if(t == nil)
			return nil;

		b = allocimage(d, Rect(0,0,1,1), display->chan, 1, color3);
		if(b == nil){
			freeimage(t);
			return nil;
		}

		draw(b, b->r, t, qmask, ZP);
		freeimage(t);
		return b;
	}
}

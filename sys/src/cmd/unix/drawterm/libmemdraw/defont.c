#include "../lib9.h"

#include "../libdraw/draw.h"
#include "../libmemdraw/memdraw.h"

Memsubfont*
getmemdefont(void)
{
	char *hdr, *p;
	int n;
	Fontchar *fc;
	Memsubfont *f;
	int ld;
	Rectangle r;
	Memimage *i;

	/*
	 * make sure data is word-aligned.  this is true with Plan 9 compilers
	 * but not in general.  the byte order is right because the data is
	 * declared as char*, not ulong*.
	 */
	p = (char*)defontdata;
	n = (ulong)p & 3;
	if(n != 0){
		memmove(p+(4-n), p, sizeofdefont-n);
		p += 4-n;
	}
	ld = atoi(p+0*12);
	r.min.x = atoi(p+1*12);
	r.min.y = atoi(p+2*12);
	r.max.x = atoi(p+3*12);
	r.max.y = atoi(p+4*12);

	p += 5*12;

	i = allocmemimage(r, drawld2chan[ld]);
	if(i == nil)
		return nil;
		
	loadmemimage(i, i->r, (uchar*)p, Dy(r)*i->width*sizeof(ulong));
	
	hdr = p+Dy(r)*i->width*sizeof(ulong);
	n = atoi(hdr);
	p = hdr+3*12;
	fc = malloc(sizeof(Fontchar)*(n+1));
	if(fc == 0){
		freememimage(i);
		return 0;
	}
	_unpackinfo(fc, (uchar*)p, n);
	f = allocmemsubfont("*default*", n, atoi(hdr+12), atoi(hdr+24), fc, i);
	if(f == 0){
		freememimage(i);
		free(fc);
		return 0;
	}
	return f;
}

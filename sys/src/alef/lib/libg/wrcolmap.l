#include <alef.h>
#include <libg.h>

/*
 * This code (and the devbit interface) will have to change
 * if we ever get bitmaps with ldepth > 3, because the
 * colormap will have to be written in chunks
 */

void
wrcolmap(Bitmap *b, RGB *m)
{
	byte *buf;
	int i, n;

	n = 1<<(1<<b->ldepth);
	if(n > 256)
		berror("wrcolmap bitmap too deep");
	buf = bneed(3 + 12*n);
	buf[0] = 'z';
	BPSHORT(buf+1, b->id);
	buf += 3;
	for(i = 0; i < n; i++){
		BPLONG(buf, m->red);
		BPLONG(buf+4, m->green);
		BPLONG(buf+8, m->blue);
		buf += 12;
		m++;
	}
	bneed(0);
}

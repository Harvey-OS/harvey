#include <stdlib.h>
#include <unistd.h>
#include <libg.h>

typedef unsigned char uchar;
typedef unsigned long ulong;

/*
 * This code (and the devbit interface) will have to change
 * if we ever get bitmaps with ldepth > 3, because the
 * colormap will have to be read in chunks
 */

void
rdcolmap(Bitmap *b, RGB *m)
{
	uchar *buf, *p;
	uchar ans[256*12];
	int i, n;

	bneed(0);
	buf = bneed(3);
	buf[0] = 'm';
	BPSHORT(buf+1, b->id);
	n = 1<<(1<<b->ldepth);
	if(n > 256)
		berror("rdcolmap bitmap too deep");
	if(!bwrite())
		berror("bad bitmap for colormap read");
	if(read(bitbltfd, (char *)ans, 12*n) != 12*n)
		berror("rdcolmap read");
	p = ans;
	for(i = 0; i < n; i++){
		m->red = BGLONG(p);
		m->green = BGLONG(p+4);
		m->blue = BGLONG(p+8);
		m++;
		p += 12;
	}
}

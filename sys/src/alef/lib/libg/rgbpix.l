#include <alef.h>
#include <libg.h>

intern int
cdiff(RGB c, RGB d)
{
	int ans, rd, gd, bd;

	rd = ((c.red >> 24) & 0xFF) - ((d.red >> 24) & 0xFF);
	gd = ((c.green >> 24) & 0xFF) - ((d.green >> 24) & 0xFF);
	bd = ((c.blue >> 24) & 0xFF) - ((d.blue >> 24) & 0xFF);
	ans = rd*rd + gd*gd + bd*bd;

	return ans;
}

uint
rgbpix(Bitmap *b, RGB col)
{
	RGB map[256];
	int i, n, besti;
	uint d, bestd;

	n = 1<<(1<<b->ldepth);
	if(n > 256)
		berror("rdcolmap bitmap too deep");

	rdcolmap(b, map);
	besti = 0;
	bestd = cdiff(col, map[0]);
	for(i = 1; i < n; i++) {
		d = cdiff(col, map[i]);
		if(d < bestd) {
			bestd = d;
			besti = i;
		}
	}
	return besti;
}

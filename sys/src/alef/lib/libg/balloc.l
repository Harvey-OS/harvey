#include <alef.h>
#include <graphics.h>

Bitmap*
balloc(Rectangle r, int ldepth)
{
	int id;
	Bitmap *b;
	char *buf, xbuf[3];

	bneed(0);		/* flush so if there's an error we know it's our fault */
	buf = bneed(18);
	buf[0] = 'a';
	buf[1] = ldepth;

	BPLONG(buf+2, r.min.x);
	BPLONG(buf+6, r.min.y);
	BPLONG(buf+10, r.max.x);
	BPLONG(buf+14, r.max.y);

	if(!bwrite())
		return nil;	/* unfatal case: no free bitmap memory */

	if(read(bitbltfd, xbuf, 3) != 3 || xbuf[0]!='A')
		berror("balloc read");

	id = xbuf[1] | (xbuf[2]<<8);
	b = malloc(sizeof(Bitmap));
	if(b == nil) {
		buf[0] = 'f';
		write(bitbltfd, xbuf, 3);
		berror("balloc malloc");
	}

	b->ldepth = ldepth;
	b->r = r;
	b->clipr = r;
	b->id = id;
	b->cache = nil;
	return b;
}

void
bfree(Bitmap *b)
{
	char *buf;

	buf = bneed(3);
	buf[0] = 'f';
	buf[1] = b->id;
	buf[2] = b->id>>8;
	free(b);
	bneed(0);	/* make sure the memory's freed before continuing */
}

#include <u.h>
#include <libc.h>
#include <libg.h>

#define	CHUNK	6000
void
wrbitmap(Bitmap *b, int miny, int maxy, uchar *data)
{
	long dy, px;
	ulong l, t, n;
	uchar *buf;

	px = 1<<(3-b->ldepth);	/* pixels per byte */
	/* set l to number of bytes of data per scan line */
	if(b->r.min.x >= 0)
		l = (b->r.max.x+px-1)/px - b->r.min.x/px;
	else{	/* make positive before divide */
		t = (-b->r.min.x)+px-1;
		t = (t/px)*px;
		l = (t+b->r.max.x+px-1)/px;
	}
	while(maxy > miny){
		dy = maxy - miny;
		if(dy*l > CHUNK)
			dy = CHUNK/l;
		n = dy*l;
		buf = bneed(11+n);
		buf[0] = 'w';
		BPSHORT(buf+1, b->id);
		BPLONG(buf+3, miny);
		BPLONG(buf+7, miny+dy);
		memmove(buf+11, data, n);
		data += n;
		miny += dy;
	}
}

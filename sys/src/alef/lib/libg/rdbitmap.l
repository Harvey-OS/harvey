#include <alef.h>
#include <libg.h>

#define	CHUNK	6000

void
rdbitmap(Bitmap *b, int miny, int maxy, byte *data)
{
	int dy, px;
	uint l, t, n;
	byte hdr[11];

	bneed(0);
	hdr[0] = 'r';
	BPSHORT(hdr+1, b->id);
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
		BPLONG(hdr+3, miny);
		BPLONG(hdr+7, miny+dy);
		n = dy*l;
		if(write(bitbltfd, hdr, 11) != 11)
			berror("rdbitmap write");
		if(read(bitbltfd, data, n) != n)
			berror("rdbitmap read");
		data += n;
		miny += dy;
	}
}

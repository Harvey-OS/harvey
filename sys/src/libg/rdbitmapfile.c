#include <u.h>
#include <libc.h>
#include <libg.h>

#define	CHUNK	6000

Bitmap*
rdbitmapfile(int fd)
{
	char hdr[5*12+1];
	long dy, px;
	ulong l, t, n;
	long miny, maxy;
	Rectangle r;
	int ld;
	Bitmap *b;

	if(readn(fd, hdr, 5*12)!=5*12)
		berror("rdbitmapfile read");
	ld = atoi(hdr+0*12);
	r.min.x = atoi(hdr+1*12);
	r.min.y = atoi(hdr+2*12);
	r.max.x = atoi(hdr+3*12);
	r.max.y = atoi(hdr+4*12);
	if(ld<0 || ld>3)
		berror("rdbitmapfile ldepth");
	if(r.min.x>r.max.x || r.min.y>r.max.y)
		berror("rdbitmapfile rectangle");

	miny = r.min.y;
	maxy = r.max.y;
	px = 1<<(3-ld);	/* pixels per byte */
	/* set l to number of bytes of data per scan line */
	if(r.min.x >= 0)
		l = (r.max.x+px-1)/px - r.min.x/px;
	else{	/* make positive before divide */
		t = (-r.min.x)+px-1;
		t = (t/px)*px;
		l = (t+r.max.x+px-1)/px;
	}
	b = balloc(r, ld);
	if(b == 0)
		return 0;
	while(maxy > miny){
		dy = maxy - miny;
		if(dy*l > CHUNK)
			dy = CHUNK/l;
		n = dy*l;
		if(readn(fd, _btmp, n) != n){
			bfree(b);
			berror("rdbitmapfile read");
		}
		wrbitmap(b, miny, miny+dy, _btmp);
		miny += dy;
	}
	return b;
}

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <libg.h>

#define	CHUNK	4096

void
wrbitmapfile(int fd, Bitmap *b)
{
	char hdr[5*12+1];
	unsigned char *data;
	long dy, px;
	unsigned long l, t, n;
	long miny, maxy;

	sprintf(hdr, "%11d %11d %11d %11d %11d ",
		b->ldepth, b->r.min.x, b->r.min.y, b->r.max.x, b->r.max.y);
	if(write(fd, hdr, 5*12) != 5*12)
		berror("wrbitmapfile write");

	px = 1<<(3-b->ldepth);	/* pixels per byte */
	/* set l to number of bytes of data per scan line */
	if(b->r.min.x >= 0)
		l = (b->r.max.x+px-1)/px - b->r.min.x/px;
	else{	/* make positive before divide */
		t = (-b->r.min.x)+px-1;
		t = (t/px)*px;
		l = (t+b->r.max.x+px-1)/px;
	}
	miny = b->r.min.y;
	maxy = b->r.max.y;
	data = malloc(CHUNK);
	if(data == 0)
		berror("wrbitmapfile malloc");
	while(maxy > miny){
		dy = maxy - miny;
		if(dy*l > CHUNK)
			dy = CHUNK/l;
		rdbitmap(b, miny, miny+dy, data);
		n = dy*l;
		if(write(fd, (char *)data, n) != n){
			free(data);
			berror("wrbitmapfile write");
		}
		miny += dy;
	}
	free(data);
}

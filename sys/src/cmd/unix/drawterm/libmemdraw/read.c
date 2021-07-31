#include "../lib9.h"

#include "../libdraw/draw.h"
#include "../libmemdraw/memdraw.h"

#define	CHUNK	8000

Memimage*
readmemimage(int fd)
{
	char hdr[5*12+1];
	int dy;
	ulong chan;
	uint l, m, n;
	int j;
	int new, miny, maxy;
	Rectangle r;
	uchar *tmp;
	int ldepth;
	Memimage *i;

	if(readn(fd, hdr, 11) != 11)
		return nil;
	if(memcmp(hdr, "compressed\n", 11) == 0)
		return creadmemimage(fd);
	if(readn(fd, hdr+11, 5*12-11) != 5*12-11)
		return nil;

	/*
	 * distinguish new channel descriptor from old ldepth.
	 * channel descriptors have letters as well as numbers,
	 * while ldepths are a single digit formatted as %-11d.
	 */
	new = 0;
	for(m=0; m<10; m++){
		if(hdr[m] != ' '){
			new = 1;
			break;
		}
	}
	if(hdr[11] != ' '){
		werrstr("readimage: bad format");
		return nil;
	}
	if(new){
		hdr[11] = '\0';
		if((chan = strtochan(hdr)) == 0){
			werrstr("readimage: bad channel string %s", hdr);
			return nil;
		}
	}else{
		ldepth = ((int)hdr[10])-'0';
		if(ldepth<0 || ldepth>3){
			werrstr("readimage: bad ldepth %d", ldepth);
			return nil;
		}
		chan = drawld2chan[ldepth];
	}

	r.min.x = atoi(hdr+1*12);
	r.min.y = atoi(hdr+2*12);
	r.max.x = atoi(hdr+3*12);
	r.max.y = atoi(hdr+4*12);
	if(r.min.x>r.max.x || r.min.y>r.max.y){
		werrstr("readimage: bad rectangle");
		return nil;
	}

	miny = r.min.y;
	maxy = r.max.y;

	l = bytesperline(r, chantodepth(chan));
	i = allocmemimage(r, chan);
	if(i == nil)
		return nil;
	tmp = malloc(CHUNK);
	if(tmp == nil)
		goto Err;
	while(maxy > miny){
		dy = maxy - miny;
		if(dy*l > CHUNK)
			dy = CHUNK/l;
		if(dy <= 0){
			werrstr("readmemimage: image too wide for buffer");
			goto Err;
		}
		n = dy*l;
		m = readn(fd, tmp, n);
		if(m != n){
			werrstr("readmemimage: read count %d not %d: %r", m, n);
   Err:
 			freememimage(i);
			free(tmp);
			return nil;
		}
		if(!new)	/* an old image: must flip all the bits */
			for(j=0; j<CHUNK; j++)
				tmp[j] ^= 0xFF;

		if(loadmemimage(i, Rect(r.min.x, miny, r.max.x, miny+dy), tmp, CHUNK) <= 0)
			goto Err;
		miny += dy;
	}
	free(tmp);
	return i;
}

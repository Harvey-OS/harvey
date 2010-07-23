/*
 * readV210.c - read single uncompressed Quicktime YUV image.
 * http://developer.apple.com/quicktime/icefloe/dispatch019.html#v210
 * Steve Simon, 2009
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include <ctype.h>
#include "imagefile.h"

enum {
	Pixels = 720,
	R601pal = 576,
	R601ntsc = 486,
	Shift = 13
};

static int
looksize(char *file, vlong size, int *pixels, int *lines, int *chunk)
{
	Biobuf *bp;
	uvlong l, p, c;
	char *s, *a[12];

	/*
	 * This may not always work, there could be an alias between file
	 * sizes of different standards stored in 8bits and 10 bits.
	 */
	if((bp = Bopen(file, OREAD)) == nil)
		return -1;
	while((s = Brdstr(bp, '\n', 1)) != nil){
		if(tokenize(s, a, nelem(a)) < 3)
			continue;
		if(a[0][0] == '#')
			continue;
		p = atoll(a[3]);
		l = atoll(a[5]);
		l += atoll(a[7]);
		c = 128 * ceil(p/48);
		if(l*c == size){
			*pixels = p;
			*lines = l;
			*chunk = c;
			break;
		}
	}
	Bterm(bp);
	if(s == nil)
		return -1;
	return 0;
}

static int
clip(int x)
{
	x >>= Shift + 2;	/* +2 as we assume all input images are 10 bit */
	if(x > 255)
		return 0xff;
	if(x <= 0)
		return 0;
	return x;
}

Rawimage**
BreadV210(Biobuf *bp, int colourspace)
{
	Dir *d;
	uvlong sz;
	Rawimage *a, **array;
	ushort *mux, *end, *frm, *wr;
	uchar *buf, *r, *g, *b;
	uint i, t;
	int y1, y2, cb, cr, c, l, rd;
	int chunk, lines, pixels;
	int F1, F2, F3, F4;

	buf = nil;
	if(colourspace != CYCbCr){
		werrstr("BreadV210: unknown colour space %d", colourspace);
		return nil;
	}

	if((d = dirfstat(Bfildes(bp))) != nil){
		sz = d->length;
		free(d);
	}
	else {
		fprint(2, "cannot stat input, assuming pixelsx576x10bit\n");
		sz = Pixels * R601pal * 2L + (pixels * R601pal / 2L);
	}

	if(looksize("/lib/video.specs", sz, &pixels, &lines, &chunk) == -1){
		werrstr("file spec not in /lib/video.specs\n");
		return nil;
	}

	if((a = calloc(sizeof(Rawimage), 1)) == nil)
		sysfatal("no memory");

	if((array = calloc(sizeof(Rawimage * ), 2)) == nil)
		sysfatal("no memory");
	array[0] = a;
	array[1] = nil;

	a->nchans = 3;
	a->chandesc = CRGB;
	a->chanlen = pixels * lines;
	a->r = Rect(0, 0, pixels, lines);

	if((frm = malloc(pixels*2*lines*sizeof(ushort))) == nil)
		goto Error;

	for(c = 0; c  < 3; c++)
		if((a->chans[c] = malloc(pixels*lines)) == nil)
			goto Error;

	if((buf = malloc(chunk)) == nil)
		goto Error;

	for(l = 0; l < lines; l++){
		if(Bread(bp, buf, chunk) == -1)
			goto Error;

		rd = 0;
		wr = &frm[l*pixels*2];
		end = &frm[(l+1)*pixels*2];
		while(wr < end){
			t = 0;
			for(i = 0; i < 4; i++)
				t += buf[rd+i] << 8*i;
			*wr++ = t & 0x3ff;
			*wr++ = t>>10 & 0x3ff;
			*wr++ = t>>20 & 0x3ff;
			rd += 4;
		}
	}

	mux = frm;
	end = frm + pixels * lines * 2;
	r = a->chans[0];
	g = a->chans[1];
	b = a->chans[2];

	if(pixels == Pixels && lines != R601pal){	// 625
		F1 = floor(1.402 * (1 << Shift));
		F2 = floor(0.34414 * (1 << Shift));
		F3 = floor(0.71414 * (1 << Shift));
		F4 = floor(1.772 * (1 << Shift));
	}
	else{						// 525 and HD
		F1 = floor(1.5748 * (1 << Shift));
		F2 = floor(0.1874 * (1 << Shift));
		F3 = floor(0.4681 * (1 << Shift));
		F4 = floor(1.8560 * (1 << Shift));
	}

	/*
	 * Fixme: fixed colourspace conversion at present
	 */
	while(mux < end){

		cb = *mux++ - 512;
		y1 = (int)*mux++ << Shift;
		cr = *mux++ - 512;
		y2 = (int)*mux++ << Shift;

		*r++ = clip(y1 + F1*cr);
		*g++ = clip(y1 - F2*cb - F3*cr);
		*b++ = clip((y1 + F4*cb));

		*r++ = clip(y2 + F1*cr);
		*g++ = clip(y2 - F2*cb - F3*cr);
		*b++ = clip((y2 + F4*cb));
	}
	free(frm);
	free(buf);
	return array;

Error:
	for(c = 0; c < 3; c++)
		free(a->chans[c]);
	free(a->cmap);
	free(array[0]);
	free(array);
	free(frm);
	free(buf);
	return nil;
}

Rawimage**
readV210(int fd, int colorspace)
{
	Rawimage * *a;
	Biobuf b;

	if(Binit(&b, fd, OREAD) < 0)
		return nil;
	a = BreadV210(&b, colorspace);
	Bterm(&b);
	return a;
}

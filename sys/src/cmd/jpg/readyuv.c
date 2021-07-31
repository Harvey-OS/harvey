/* readyuv.c - read an Abekas A66 style image file.   Steve Simon, 2003 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include <ctype.h>
#include "imagefile.h"

/*
 * ITU/CCIR Rec601 states:
 *
 * R = y + 1.402 * Cr
 * B = Y + 1.77305 * Cb
 * G = Y - 0.72414 * Cr - 0.34414 * Cb
 *
 *	using 8 bit traffic
 * Y = 16 + 219 * Y
 * Cr = 128 + 224 * Cr
 * Cb = 128 + 224 * Cb
 * 	or, if 10bit is used
 * Y = 64 + 876 * Y
 * Cr = 512 + 896 * Cr
 * Cb = 512 + 896 * Cb
 */

enum {
	pixels = 720,
	r601pal = 576,
	r601ntsc = 486
};


static int lsbtab[] = { 6, 4, 2, 0};

int
looksize(char *file, vlong size, int *pixels, int *lines, int *bits)
{
	Biobuf *bp;
	uvlong l, p;
	char *s, *a[12];

	/*
	 * This may not always work, there could be an alias between file
	 * sizes of different standards stored in 8bits and 10 bits.
	 */
	if ((bp = Bopen(file, OREAD)) == nil)
		return -1;
	while((s = Brdstr(bp, '\n', 1)) != nil){
		if (tokenize(s, a, nelem(a)) < 3)
			continue;
		if (a[0][0] == '#')
			continue;
		p = atoll(a[3]);
		l = atoll(a[5]);
		if (l*p*2 == size){
			*pixels = p;
			*lines = l;
			*bits = 8;
			break;
		}
		if ((l*p*20)/8 == size){
			*pixels = p;
			*lines = l;
			*bits = 10;
			break;
		}
	}
	Bterm(bp);
	if (s == nil)
		return -1;
	return 0;
}


static int 
clip(int x)
{
	x >>= 18;

	if (x > 255)
		return 0xff;
	if (x <= 0)
		return 0;
	return x;
}

Rawimage**
Breadyuv(Biobuf *bp, int colourspace)
{
	Dir *d;
	uvlong sz;
	Rawimage *a, **array;
	char *e, ebuf[128];
	ushort * mux, *end, *frm;
	uchar *buf, *r, *g, *b;
	int y1, y2, cb, cr, c, l, w, base;
	int bits, lines, pixels;

	frm = nil;
	buf = nil;
	if (colourspace != CYCbCr) {
		errstr(ebuf, sizeof ebuf);	/* throw it away */
		werrstr("ReadYUV: unknown colour space %d", colourspace);
		return nil;
	}

	if ((a = calloc(sizeof(Rawimage), 1)) == nil)
		sysfatal("no memory");

	if ((array = calloc(sizeof(Rawimage * ), 2)) == nil)
		sysfatal("no memory");
	array[0] = a;
	array[1] = nil;

	if ((d = dirfstat(Bfildes(bp))) != nil) {
		sz = d->length;
		free(d);
	} else {
		fprint(2, "cannot stat input, assuming pixelsx576x10bit\n");
		sz = pixels * r601pal * 2L + (pixels * r601pal / 2L);
	}

	if (looksize("/lib/video.specs", sz, &pixels, &lines, &bits) == -1){
		e = "file size not listed in /lib/video.specs";
		goto Error;
	}


	a->nchans = 3;
	a->chandesc = CRGB;
	a->chanlen = pixels * lines;
	a->r = Rect(0, 0, pixels, lines);

	e = "no memory";
	if ((frm = malloc(pixels*2*lines*sizeof(ushort))) == nil)
		goto Error;

	for (c = 0; c  < 3; c++)
		if ((a->chans[c] = malloc(pixels*lines)) == nil)
			goto Error;

	if ((buf = malloc(pixels*2)) == nil)
		goto Error;

	e = "read file";
	for (l = 0; l < lines; l++) {
		if (Bread(bp, buf, pixels *2) == -1)
			goto Error;

		base = l*pixels*2;
		for (w = 0; w < pixels *2; w++)
			frm[base + w] = ((ushort)buf[w]) << 2;
	}


	if (bits == 10)
		for (l = 0; l < lines; l++) {
			if (Bread(bp, buf, pixels / 2) == -1)
				goto Error;


			base = l * pixels * 2;
			for (w = 0; w < pixels * 2; w++)
				frm[base + w] |= buf[w / 4] >> lsbtab[w % 4];
		}

	mux = frm;
	end = frm + pixels * lines * 2;
	r = a->chans[0];
	g = a->chans[1];
	b = a->chans[2];

	/*
	 * Fixme: fixed colourspace conversion at present
	 */
	while (mux < end) {
		cb = *mux++ - 512;
		y1 = (*mux++ - 64) * 76310;
		cr = *mux++ - 512;
		y2 = (*mux++ - 64) * 76310;

		*r++ = clip((104635 * cr) + y1);
		*g++ = clip((-25690 * cb + -53294 * cr) + y1);
		*b++ = clip((132278 * cb) + y1);

		*r++ = clip((104635 * cr) + y2);
		*g++ = clip((-25690 * cb + -53294 * cr) + y2);
		*b++ = clip((132278 * cb) + y2);
	}
	free(frm);
	free(buf);
	return array;

Error:

	errstr(ebuf, sizeof ebuf);
//	if (ebuf[0] == 0)
		strcpy(ebuf, e);
	errstr(ebuf, sizeof ebuf);

	for (c = 0; c < 3; c++)
		free(a->chans[c]);
	free(a->cmap);
	free(array[0]);
	free(array);
	free(frm);
	free(buf);
	return nil;
}


Rawimage**
readyuv(int fd, int colorspace)
{
	Rawimage * *a;
	Biobuf b;

	if (Binit(&b, fd, OREAD) < 0)
		return nil;
	a = Breadyuv(&b, colorspace);
	Bterm(&b);
	return a;
}



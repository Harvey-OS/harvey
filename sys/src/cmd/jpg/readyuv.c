/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* readyuv.c - read an Abekas A66 style image file.   Steve Simon, 2003 */
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


static int lsbtab[] = { 6, 4, 2, 0};

static int
looksize(char *file, int64_t size, int *pixels, int *lines, int *bits)
{
	Biobuf *bp;
	uint64_t l, p;
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
		l += atoll(a[7]);
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
	x >>= (Shift+2); // +2 as we assume all input images are 10 bit

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
	uint64_t sz;
	Rawimage *a, **array;
	uint16_t * mux, *end, *frm;
	uint8_t *buf, *r, *g, *b;
	int y1, y2, cb, cr, c, l, w, base;
	int bits, lines, pixels;
	int F1, F2, F3, F4;

	if ((d = dirfstat(Bfildes(bp))) != nil){
		sz = d->length;
		free(d);
	}
	else{
		fprint(2, "cannot stat input, assuming pixelsx576x10bit\n");
		sz = Pixels * R601pal * 2L + (Pixels * R601pal / 2L);
	}

	if (looksize("/lib/video.specs", sz, &pixels, &lines, &bits) == -1){
		werrstr("file size not listed in /lib/video.specs");
		return nil;
	}

	buf = nil;
	if (colourspace != CYCbCr) {
		werrstr("ReadYUV: unknown colour space %d", colourspace);
		return nil;
	}

	if ((a = calloc(sizeof(Rawimage), 1)) == nil)
		sysfatal("no memory");

	if ((array = calloc(sizeof(Rawimage * ), 2)) == nil)
		sysfatal("no memory");
	array[0] = a;
	array[1] = nil;

	a->nchans = 3;
	a->chandesc = CRGB;
	a->chanlen = pixels * lines;
	a->r = Rect(0, 0, pixels, lines);

	if ((frm = malloc(pixels*2*lines*sizeof(uint16_t))) == nil)
		goto Error;

	for (c = 0; c  < 3; c++)
		if ((a->chans[c] = malloc(pixels*lines)) == nil)
			goto Error;

	if ((buf = malloc(pixels*2)) == nil)
		goto Error;

	for (l = 0; l < lines; l++) {
		if (Bread(bp, buf, pixels *2) == -1)
			goto Error;

		base = l*pixels*2;
		for (w = 0; w < pixels *2; w++)
			frm[base + w] = ((uint16_t)buf[w]) << 2;
	}


	if (bits == 10)
		for (l = 0; l < lines; l++) {
			if (Bread(bp, buf, pixels / 2) == -1)
				goto Error;


			base = l * pixels * 2;
			for (w = 0; w < pixels * 2; w++)
				frm[base + w] |= (buf[w / 4] >> lsbtab[w % 4]) & 3;
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
	else{				// 525
		F1 = floor(1.5748 * (1 << Shift));
		F2 = floor(0.1874 * (1 << Shift));
		F3 = floor(0.4681 * (1 << Shift));
		F4 = floor(1.8560 * (1 << Shift));
	}

	/*
	 * Fixme: fixed colourspace conversion at present
	 */
	while (mux < end) {

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

/*
 * TGA is a fairly dead standard, however in the video industry
 * it is still used a little for test patterns and the like.
 *
 * Thus we ignore any alpha channels, and colour mapped images.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include <ctype.h>
#include "imagefile.h"

enum {
	HdrLen = 18,
};

typedef struct {
	int idlen;		/* length of string after header */
	int cmaptype;		/* 1 =>  datatype = 1 => colourmapped */
	int datatype;		/* see below */
	int cmaporigin;		/* index of first entry in colour map */
	int cmaplen;		/* length of olour map */
	int cmapbpp;		/* bips per pixel of colour map: 16, 24, or 32 */
	int xorigin;		/* source image origin */
	int yorigin;
	int width;
	int height;
	int bpp;		/* bits per pixel of image: 16, 24, or 32 */
	int descriptor;
	uchar *cmap;		/* colour map (optional) */
} Tga;

/*
 * descriptor:
 * d0-3 = number of attribute bits per pixel
 * d4 	= reserved, always zero
 * d6-7	= origin: 0=lower left, 1=upper left, 2=lower right, 3=upper right
 * d8-9 = interleave: 0=progressive, 1=2 way, 3 = 4 way, 4 = reserved.
 */

char *datatype[] = {
	[0]	"No image data",
	[1]	"color mapped",
	[2]	"RGB",
	[3]	"B&W",
	[9]	"RLE color-mapped",
	[10]	"RLE RGB",
	[11]	"Compressed B&W",
	[32]	"Compressed color",
	[33]	"Quadtree compressed color",
};

static int
Bgeti(Biobuf *bp)
{
	int x, y;

	if((x = Bgetc(bp)) < 0)
		return -1;
	if((y = Bgetc(bp)) < 0)
		return -1;
	return (y<<8)|x;
}

static Tga *
rdhdr(Biobuf *bp)
{
	int n;
	Tga *h;

	if((h = malloc(sizeof(Tga))) == nil)
		return nil;
	if((h->idlen = Bgetc(bp)) == -1)
		return nil;
	if((h->cmaptype = Bgetc(bp)) == -1)
		return nil;
	if((h->datatype = Bgetc(bp)) == -1)
		return nil;
	if((h->cmaporigin = Bgeti(bp)) == -1)
		return nil;
	if((h->cmaplen = Bgeti(bp)) == -1)
		return nil;
	if((h->cmapbpp = Bgetc(bp)) == -1)
		return nil;
	if((h->xorigin = Bgeti(bp)) == -1)
		return nil;
	if((h->yorigin = Bgeti(bp)) == -1)
		return nil;
	if((h->width = Bgeti(bp)) == -1)
		return nil;
	if((h->height = Bgeti(bp)) == -1)
		return nil;
	if((h->bpp = Bgetc(bp)) == -1)
		return nil;
	if((h->descriptor = Bgetc(bp)) == -1)
		return nil;

	/* skip over ID, usually empty anyway */
	if(Bseek(bp, h->idlen, 1) < 0){
		free(h);
		return nil;
	}

	if(h->cmaptype == 0){
		h->cmap = 0;
		return h;
	}

	n = (h->cmapbpp/8)*h->cmaplen;
	if((h->cmap = malloc(n)) == nil){
		free(h);
		return nil;
	}
	if(Bread(bp, h->cmap, n) != n){
		free(h);
		free(h->cmap);
		return nil;
	}
	return h;
}

static int
luma(Biobuf *bp, uchar *l, int num)
{
	return Bread(bp, l, num);
}

static int
luma_rle(Biobuf *bp, uchar *l, int num)
{
	uchar len;
	int i, got;

	for(got = 0; got < num; got += len){
		if(Bread(bp, &len, 1) != 1)
			break;
		if(len & 0x80){
			len &= 0x7f;
			len += 1;	/* run of zero is meaningless */
			if(luma(bp, l, 1) != 1)
				break;
			for(i = 0; i < len && got < num; i++)
				l[i+1] = *l;
		}
		else{
			len += 1;	/* raw block of zero is meaningless */
			if(luma(bp, l, len) != len)
				break;
		}
		l += len;
	}
	return got;
}


static int
rgba(Biobuf *bp, int bpp, uchar *r, uchar *g, uchar *b, int num)
{
	int i;
	uchar x, y, buf[4];

	switch(bpp){
	case 16:
		for(i = 0; i < num; i++){
			if(Bread(bp, buf, 2) != 2)
				break;
			x = buf[0];
			y = buf[1];
			*b++ = (x&0x1f)<<3;
			*g++ = ((y&0x03)<<6) | ((x&0xe0)>>2);
			*r++ = (y&0x1f)<<3;
		}
		break;
	case 24:
		for(i = 0; i < num; i++){
			if(Bread(bp, buf, 3) != 3)
				break;
			*b++ = buf[0];
			*g++ = buf[1];
			*r++ = buf[2];
		}
		break;
	case 32:
		for(i = 0; i < num; i++){
			if(Bread(bp, buf, 4) != 4)
				break;
			*b++ = buf[0];
			*g++ = buf[1];
			*r++ = buf[2];
		}
		break;
	default:
		i = 0;
		break;
	}
	return i;
}

static int
rgba_rle(Biobuf *bp, int bpp, uchar *r, uchar *g, uchar *b, int num)
{
	uchar len;
	int i, got;

	for(got = 0; got < num; got += len){
		if(Bread(bp, &len, 1) != 1)
			break;
		if(len & 0x80){
			len &= 0x7f;
			len += 1;	/* run of zero is meaningless */
			if(rgba(bp, bpp, r, g, b, 1) != 1)
				break;
			for(i = 0; i < len-1 && got < num; i++){
				r[i+1] = *r;
				g[i+1] = *g;
				b[i+1] = *b;
			}
		}
		else{
			len += 1;	/* raw block of zero is meaningless */
			if(rgba(bp, bpp, r, g, b, len) != len)
				break;
		}
		r += len;
		g += len;
		b += len;
	}
	return got;
}

int
flip(Rawimage *ar)
{
	int w, h, c, l;
	uchar *t, *s, *d;

	w = Dx(ar->r);
	h = Dy(ar->r);
	if((t = malloc(w)) == nil){
		werrstr("ReadTGA: no memory - %r\n");
		return -1;
	}

	for(c = 0; c < ar->nchans; c++){
		s = ar->chans[c];
		d = ar->chans[c] + ar->chanlen - w;
		for(l = 0; l < (h/2); l++){
			memcpy(t, s, w);
			memcpy(s, d, w);
			memcpy(d, t, w);
			s += w;
			d -= w;
		}
	}
	free(t);
	return 0;
}

int
reflect(Rawimage *ar)
{
	int w, h, c, l, p;
	uchar t, *sol, *eol, *s, *d;

	w = Dx(ar->r);
	h = Dy(ar->r);

	for(c = 0; c < ar->nchans; c++){
		sol = ar->chans[c];
		eol = ar->chans[c] +w -1;
		for(l = 0; l < h; l++){
			s = sol;
			d = eol;
			for(p = 0; p < w/2; p++){
				t = *s;
				*s = *d;
				*d = t;
				s++;
				d--;
			}
			sol += w;
			eol += w;
		}
	}
	return 0;
}


Rawimage**
Breadtga(Biobuf *bp)
{
	Tga *h;
	int n, c, num;
	uchar *r, *g, *b;
	Rawimage *ar, **array;

	if((h = rdhdr(bp)) == nil){
		werrstr("ReadTGA: bad header %r");
		return nil;
	}

	if(0){
		fprint(2, "idlen=%d\n", h->idlen);
		fprint(2, "cmaptype=%d\n", h->cmaptype);
		fprint(2, "datatype=%s\n", datatype[h->datatype]);
		fprint(2, "cmaporigin=%d\n", h->cmaporigin);
		fprint(2, "cmaplen=%d\n", h->cmaplen);
		fprint(2, "cmapbpp=%d\n", h->cmapbpp);
		fprint(2, "xorigin=%d\n", h->xorigin);
		fprint(2, "yorigin=%d\n", h->yorigin);
		fprint(2, "width=%d\n", h->width);
		fprint(2, "height=%d\n", h->height);
		fprint(2, "bpp=%d\n", h->bpp);
		fprint(2, "descriptor=%d\n", h->descriptor);
	}

	array = nil;
	if((ar = calloc(sizeof(Rawimage), 1)) == nil){
		werrstr("ReadTGA: no memory - %r\n");
		goto Error;
	}

	if((array = calloc(sizeof(Rawimage *), 2)) == nil){
		werrstr("ReadTGA: no memory - %r\n");
		goto Error;
	}
	array[0] = ar;
	array[1] = nil;

	if(h->datatype == 3){
		ar->nchans = 1;
		ar->chandesc = CY;
	}
	else{
		ar->nchans = 3;
		ar->chandesc = CRGB;
	}

	ar->chanlen = h->width*h->height;
	ar->r = Rect(0, 0, h->width, h->height);
	for (c = 0; c < ar->nchans; c++)
		if ((ar->chans[c] = malloc(h->width*h->height)) == nil){
			werrstr("ReadTGA: no memory - %r\n");
			goto Error;
		}
	r = ar->chans[0];
	g = ar->chans[1];
	b = ar->chans[2];

	num = h->width*h->height;
	switch(h->datatype){
	case 2:
		if(rgba(bp, h->bpp, r, g, b, num) != num){
			werrstr("ReadTGA: decode fail - %r\n");
			goto Error;
		}
		break;
	case 3:
		if(luma(bp, r, num) != num){
			werrstr("ReadTGA: decode fail - %r\n");
			goto Error;
		}
		break;
	case 10:
		if((n = rgba_rle(bp, h->bpp, r, g, b, num)) != num){
			werrstr("ReadTGA: decode fail (%d!=%d) - %r\n", n, num);
			goto Error;
		}
		break;
	case 11:
		if(luma_rle(bp, r, num) != num){
			werrstr("ReadTGA: decode fail - %r\n");
			goto Error;
		}
		break;
	default:
		werrstr("ReadTGA: type=%d (%s) unsupported\n", h->datatype, datatype[h->datatype]);
		goto Error;	
 	}

	if(h->xorigin != 0)
		reflect(ar);
	if(h->yorigin == 0)
		flip(ar);
	
	free(h->cmap);
	free(h);
	return array;
Error:

	if(ar)
		for (c = 0; c < ar->nchans; c++)
			free(ar->chans[c]);
	free(ar);
	free(array);
	free(h->cmap);
	free(h);
	return nil;
}

Rawimage**
readtga(int fd)
{
	Rawimage * *a;
	Biobuf b;

	if (Binit(&b, fd, OREAD) < 0)
		return nil;
	a = Breadtga(&b);
	Bterm(&b);
	return a;
}



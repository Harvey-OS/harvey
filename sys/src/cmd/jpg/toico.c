/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>

enum
{
	FileHdrLen=	6,
	IconDescrLen=	16,
	IconHdrLen=	40,
};

typedef struct Icon Icon;
struct Icon
{
	Icon	*next;
	char	*file;

	uint8_t	w;		/* icon width */
	uint8_t	h;		/* icon height */
	uint16_t	ncolor;		/* number of colors */
	uint16_t	nplane;		/* number of bit planes */
	uint16_t	bits;		/* bits per pixel */
	uint32_t	len;		/* length of data */
	uint32_t	offset;		/* file offset to data */
	uint8_t	map[4*256];	/* color map */

	Image	*img;

	uint8_t	*xor;
	int	xorlen;
	uint8_t	*and;
	int	andlen;
};

typedef struct Header Header;
struct Header
{
	uint	n;
	Icon	*first;
	Icon	*last;
};

void
Bputs(Biobuf *b, uint16_t x)
{
	Bputc(b, x&0xff);
	Bputc(b, x>>8);
}

void
Bputl(Biobuf *b, uint32_t x)
{
	Bputs(b, x&0xffff);
	Bputs(b, x>>16);
}

Header h;

void*	emalloc(int);
void	mk8bit(Icon*, int);
void	mkxorand(Icon*, int);
void	readicon(char*);

void
main(int argc, char **argv)
{
	int i;
	Biobuf *b, out;
	Icon *icon;
	uint32_t offset;
	uint32_t len;

	ARGBEGIN{
	}ARGEND;

	/* read in all the images */
	display = initdisplay(nil, nil, nil);
	if(argc < 1){
		readicon("/fd/0");
	} else {
		for(i = 0; i < argc; i++)
			readicon(argv[i]);
	}

	/* create the .ico file */
	b = &out;
	Binit(b, 1, OWRITE);

	/* offset to first icon */
	offset = FileHdrLen + h.n*IconDescrLen;

	/* file header is */
	Bputs(b, 0);
	Bputs(b, 1);
	Bputs(b, h.n);

	/* icon description */
	for(icon = h.first; icon != nil; icon = icon->next){
		Bputc(b, icon->w);
		Bputc(b, icon->h);
		Bputc(b, icon->ncolor);
		Bputc(b, 0);
		Bputs(b, icon->nplane);
		Bputs(b, icon->bits);
		len = IconHdrLen + icon->ncolor*4 + icon->xorlen + icon->andlen;
		Bputl(b, len);
		Bputl(b, offset);
		offset += len;
	}

	/* icons */
	for(icon = h.first; icon != nil; icon = icon->next){
		/* icon header (BMP like) */
		Bputl(b, IconHdrLen);
		Bputl(b, icon->w);
		Bputl(b, 2*icon->h);
		Bputs(b, icon->nplane);
		Bputs(b, icon->bits);
		Bputl(b, 0);	/* compression info */
		Bputl(b, 0);
		Bputl(b, 0);
		Bputl(b, 0);
		Bputl(b, 0);
		Bputl(b, 0);

		/* color map */
		if(Bwrite(b, icon->map, 4*icon->ncolor) < 0)
			sysfatal("writing color map: %r");

		/* xor bits */
		if(Bwrite(b, icon->xor, icon->xorlen) < 0)
			sysfatal("writing xor bits: %r");

		/* and bits */
		if(Bwrite(b, icon->and, icon->andlen) < 0)
			sysfatal("writing and bits: %r");
	}

	Bterm(b);
	exits(0);
}

void
readicon(char *file)
{
	int fd;
	Icon *icon;

	fd = open(file, OREAD);
	if(fd < 0)
		sysfatal("opening %s: %r", file);
	icon = emalloc(sizeof(Icon));
	icon->img = readimage(display, fd, 0);
	if(icon->img == nil)
		sysfatal("reading image %s: %r", file);
	close(fd);

	if(h.first)
		h.last->next = icon;
	else
		h.first = icon;
	h.last = icon;
	h.n++;

	icon->h = Dy(icon->img->r);
	icon->w = Dx(icon->img->r);
	icon->bits = 1<<icon->img->depth;
	icon->nplane = 1;

	/* convert to 8 bits per pixel */
	switch(icon->img->chan){
	case GREY8:
	case CMAP8:
		break;
	case GREY1:
	case GREY2:
	case GREY4:
		mk8bit(icon, 1);
		break;
	default:
		mk8bit(icon, 0);
		break;
	}
	icon->bits = 8;
	icon->file = file;

	/* create xor/and masks, minimizing bits per pixel */
	mkxorand(icon, icon->img->chan == GREY8);
}

void*
emalloc(int len)
{
	void *x;

	x = mallocz(len, 1);
	if(x == nil)
		sysfatal("memory: %r");
	return x;
}

/* convert to 8 bit */
void
mk8bit(Icon *icon, int grey)
{
	Image *img;

	img = allocimage(display, icon->img->r, grey ? GREY8 : CMAP8, 0, DNofill);
	if(img == nil)
		sysfatal("can't allocimage: %r");
	draw(img, img->r, icon->img, nil, ZP);
	freeimage(icon->img);
	icon->img = img;
}

/* make xor and and mask */
void
mkxorand(Icon *icon, int grey)
{
	int i, x, y, s, sa;
	uint8_t xx[256];
	uint8_t *data, *p, *e;
	int ndata;
	uint8_t *mp;
	int ncolor;
	uint32_t color;
	int bits;
	uint8_t andbyte, xorbyte;
	uint8_t *ato, *xto;
	int xorrl, andrl;

	ndata = icon->h * icon->w;
	data = emalloc(ndata);
	if(unloadimage(icon->img, icon->img->r, data, ndata) < 0)
		sysfatal("can't unload %s: %r", icon->file);
	e = data + ndata;

	/* find colors used */
	memset(xx, 0, sizeof xx);
	for(p = data; p < e; p++)
		xx[*p]++;

	/* count the colors and create a mapping from plan 9 */
	mp = icon->map;
	ncolor = 0;
	for(i = 0; i < 256; i++){
		if(xx[i] == 0)
			continue;
		if(grey){
			*mp++ = i;
			*mp++ = i;
			*mp++ = i;
			*mp++ = 0;
		} else {
			color = cmap2rgb(i);
			*mp++ = color;
			*mp++ = color>>8;
			*mp++ = color>>16;
			*mp++ = 0;
		}
		xx[i] = ncolor;
		ncolor++;
	}

	/* get minimum number of pixels per bit (with a color map) */
	if(ncolor <= 2){
		ncolor = 2;
		bits = 1;
	} else if(ncolor <= 4){
		ncolor = 4;
		bits = 2;
	} else if(ncolor <= 16){
		ncolor = 16;
		bits = 4;
	} else {
		ncolor = 256;
		bits = 8;
	}
	icon->bits = bits;
	icon->ncolor = ncolor;

	/* the xor mask rows are justified to a 32 bit boundary */
	/* the and mask is 1 bit grey */
	xorrl = 4*((bits*icon->w + 31)/32);
	andrl = 4*((icon->w + 31)/32);
	icon->xor = emalloc(xorrl * icon->h);
	icon->and = emalloc(andrl * icon->h);
	icon->xorlen = xorrl*icon->h;
	icon->andlen = andrl*icon->h;

	/* make both masks.  they're upside down relative to plan9 ones */
	p = data;
	for(y = 0; y < icon->h; y++){
		andbyte = 0;
		xorbyte = 0;
		sa = s = 0;
		xto = icon->xor + (icon->h-1-y)*xorrl;
		ato = icon->and + (icon->h-1-y)*andrl;
		for(x = 0; x < icon->w; x++){
			xorbyte <<= bits;
			xorbyte |= xx[*p];
			s += bits;
			if(s == 8){
				*xto++ = xorbyte;
				xorbyte = 0;
				s = 0;
			}
			andbyte <<= 1;
			if(*p == 0xff)
				andbyte |= 1;
			sa++;
			if(sa == 0){
				*ato++ = andbyte;
				sa = 0;
				andbyte = 0;
			}
			p++;
		}
	}
	free(data);
}

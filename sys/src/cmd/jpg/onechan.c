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
#include <draw.h>
#include <memdraw.h>
#include <bio.h>
#include "imagefile.h"

/* Convert image to a single channel, one byte per pixel */

static
int
notrans(uint32_t chan)
{
	switch(chan){
	case GREY1:
	case GREY2:
	case GREY4:
	case	CMAP8:
	case GREY8:
		return 1;
	}
	return 0;
}

static
int
easycase(uint32_t chan)
{
	switch(chan){
	case RGB16:
	case RGB24:
	case RGBA32:
	case ARGB32:
		return 1;
	}
	return 0;
}

/*
 * Convert to one byte per pixel, RGBV or grey, depending
 */

static
uint8_t*
load(Image *image, Memimage *memimage)
{
	uint8_t *data, *p, *q0, *q1, *q2;
	uint8_t *rgbv;
	int depth, ndata, dx, dy, i, v;
	uint32_t chan, pixel;
	Rectangle r;
	Rawimage ri, *nri;

	if(memimage == nil){
		r = image->r;
		depth = image->depth;
		chan = image->chan;
	}else{
		r = memimage->r;
		depth = memimage->depth;
		chan = memimage->chan;
	}
	dx = Dx(r);
	dy = Dy(r);

	/*
	 * Read image data into memory
	 * potentially one extra byte on each end of each scan line.
	 */
	ndata = dy*(2+bytesperline(r, depth));
	data = malloc(ndata);
	if(data == nil)
		return nil;
	if(memimage != nil)
		ndata = unloadmemimage(memimage, r, data, ndata);
	else
		ndata = unloadimage(image, r, data, ndata);
	if(ndata < 0){
		werrstr("onechan: %r");
		free(data);
		return nil;
	}

	/*
	 * Repack
	 */
	memset(&ri, 0, sizeof(ri));
	ri.r = r;
	ri.cmap = nil;
	ri.cmaplen = 0;
	ri.nchans = 3;
	ri.chanlen = dx*dy;
	ri.chans[0] = malloc(ri.chanlen);
	ri.chans[1] = malloc(ri.chanlen);
	ri.chans[2] = malloc(ri.chanlen);
	if(ri.chans[0]==nil || ri.chans[1]==nil || ri.chans[2]==nil){
    Err:
		free(ri.chans[0]);
		free(ri.chans[1]);
		free(ri.chans[2]);
		free(data);
		return nil;
	}
	ri.chandesc = CRGB;

	p = data;
	q0 = ri.chans[0];
	q1 = ri.chans[1];
	q2 = ri.chans[2];

	switch(chan){
	default:
		werrstr("can't handle image type 0x%lx", chan);
		goto Err;
	case RGB16:
		for(i=0; i<ri.chanlen; i++, p+=2){
			pixel = (p[1]<<8)|p[0];	/* rrrrrggg gggbbbbb */
			v = (pixel & 0xF800) >> 8;
			*q0++ = v | (v>>5);
			v = (pixel & 0x07E0) >> 3;
			*q1++ = v | (v>>6);
			v = (pixel & 0x001F) << 3;
			*q2++ = v | (v>>5);
		}
		break;
	case RGB24:
		for(i=0; i<ri.chanlen; i++){
			*q2++ = *p++;
			*q1++ = *p++;
			*q0++ = *p++;
		}
		break;
	case RGBA32:
		for(i=0; i<ri.chanlen; i++){
			*q2++ = *p++;
			*q1++ = *p++;
			*q0++ = *p++;
			p++;
		}
		break;
	case ARGB32:
		for(i=0; i<ri.chanlen; i++){
			p++;
			*q2++ = *p++;
			*q1++ = *p++;
			*q0++ = *p++;
		}
		break;
	}

	rgbv = nil;
	nri = torgbv(&ri, 1);
	if(nri != nil){
		rgbv = nri->chans[0];
		free(nri);
	}

	free(ri.chans[0]);
	free(ri.chans[1]);
	free(ri.chans[2]);
	free(data);
	return rgbv;
}

Image*
onechan(Image *i)
{
	uint8_t *data;
	Image *ni;

	if(notrans(i->chan))
		return i;

	if(easycase(i->chan))
		data = load(i, nil);
	else{
		ni = allocimage(display, i->r, RGB24, 0, DNofill);
		if(ni == nil)
			return ni;
		draw(ni, ni->r, i, nil, i->r.min);
		data = load(ni, nil);
		freeimage(ni);
	}

	if(data == nil)
		return nil;

	ni = allocimage(display, i->r, CMAP8, 0, DNofill);
	if(ni != nil)
		if(loadimage(ni, ni->r, data, Dx(ni->r)*Dy(ni->r)) < 0){
			freeimage(ni);
			ni = nil;
		}
	free(data);
	return ni;
}

Memimage*
memonechan(Memimage *i)
{
	uint8_t *data;
	Memimage *ni;

	if(notrans(i->chan))
		return i;

	if(easycase(i->chan))
		data = load(nil, i);
	else{
		ni = allocmemimage(i->r, RGB24);
		if(ni == nil)
			return ni;
		memimagedraw(ni, ni->r, i, i->r.min, nil, ZP, S);
		data = load(nil, ni);
		freememimage(ni);
	}

	if(data == nil)
		return nil;

	ni = allocmemimage(i->r, CMAP8);
	if(ni != nil)
		if(loadmemimage(ni, ni->r, data, Dx(ni->r)*Dy(ni->r)) < 0){
			freememimage(ni);
			ni = nil;
		}
	free(data);
	return ni;
}

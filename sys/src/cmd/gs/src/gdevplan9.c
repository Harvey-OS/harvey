/*
 *		Copyright (c) 1998 by Lucent Technologies.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR LUCENT TECHNOLOGIES MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */

/*
 * gdevplan9.c: gs device to generate plan9 bitmaps
 * Russ Cox <rsc@plan9.bell-labs.com>, 3/25/98 (gdevifno)
 * Updated to fit in the standard GS distribution, 5/14/98
 * Added support for true-color bitmaps, 6/7/02
 */

#include "gdevprn.h"
#include "gsparam.h"
#include "gxlum.h"
#include "gxstdio.h"
#include <stdlib.h>

#define nil ((void*)0)
enum {
	ERROR = -2
};

typedef struct WImage WImage;
typedef struct Rectangle Rectangle;
typedef struct Point Point;

struct Point {
	int x;
	int y;
};

struct Rectangle {
	Point min;
	Point max;
};
private Point ZP = { 0, 0 };

private WImage* initwriteimage(FILE *f, Rectangle r, char*, int depth);
private int writeimageblock(WImage *w, uchar *data, int ndata);
private int bytesperline(Rectangle, int);
private int rgb2cmap(int, int, int);
private long cmap2rgb(int);

#define X_DPI	100
#define Y_DPI	100

private dev_proc_map_rgb_color(plan9_rgb2cmap);
private dev_proc_map_color_rgb(plan9_cmap2rgb);
private dev_proc_open_device(plan9_open);
private dev_proc_close_device(plan9_close);
private dev_proc_print_page(plan9_print_page);
private dev_proc_put_params(plan9_put_params);
private dev_proc_get_params(plan9_get_params);

typedef struct plan9_device_s {
	gx_device_common;
	gx_prn_device_common;
	int dither;

	int ldepth;
	int lastldepth;
	int cmapcall;
} plan9_device;

enum {
	Nbits = 8,
	Bitmask = (1<<Nbits)-1,
};

private const gx_device_procs plan9_procs =
	prn_color_params_procs(plan9_open, gdev_prn_output_page, gdev_prn_close,
		plan9_rgb2cmap, plan9_cmap2rgb,
		gdev_prn_get_params, gdev_prn_put_params);
/*
		plan9_get_params, plan9_put_params);
*/


plan9_device far_data gs_plan9_device =
{ prn_device_body(plan9_device, plan9_procs, "plan9",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0,0,0,0,	/* margins */
	3,		/* 3 = RGB, 1 = gray, 4 = CMYK */
	Nbits*3,		/* # of bits per pixel */
	(1<<Nbits)-1,		/* # of distinct gray levels. */
	(1<<Nbits)-1,		/* # of distinct color levels. */
	1<<Nbits,		/* dither gray ramp size.  used in alpha? */
	1<<Nbits,    	/* dither color ramp size.  used in alpha? */
	plan9_print_page),
	1,
};

/*
 * ghostscript asks us how to convert between
 * rgb and color map entries
 */
private gx_color_index 
plan9_rgb2cmap(gx_device *dev, gx_color_value *rgb)
{
	gx_color_value r, g, b;
	int shift;
	plan9_device *idev;
	ulong red, green, blue;

	r = rgb[0];
	g = rgb[1];
	b = rgb[2];

	idev = (plan9_device*) dev;

	shift = gx_color_value_bits - Nbits;
	red = r >> shift;
	green = g >> shift;
	blue = b >> shift;

	/*
	 * we keep track of what ldepth bitmap this is by watching
	 * what colors gs asks for.
	 * 
	 * one catch: sometimes print_page gets called more than one
	 * per page (for multiple copies) without cmap calls inbetween.
	 * if idev->cmapcall is 0 when print_page gets called, it uses
	 * the ldepth of the last page.
	 */
	if(red == green && green == blue) {
		if(red == 0 || red == Bitmask)
			;
		else if(red == Bitmask/3 || red == 2*Bitmask/3) {
			if(idev->ldepth < 1)
				idev->ldepth = 1;
		} else {
			if(idev->ldepth < 2)
				idev->ldepth = 2;
		}
	} else
		idev->ldepth = 3;

	idev->cmapcall = 1;
	return (blue << (2*Nbits)) | (green << Nbits) | red;
}

private int 
plan9_cmap2rgb(gx_device *dev, gx_color_index color,
  gx_color_value rgb[3]) {
	int shift, i;
	plan9_device *idev;

	if((ulong)color > 0xFFFFFF)
		return_error(gs_error_rangecheck);

	idev = (plan9_device*) dev;
	shift = gx_color_value_bits - Nbits;

	rgb[2] = ((color >> (2*Nbits)) & Bitmask) << shift;
	rgb[1] = ((color >> Nbits) & Bitmask) << shift;
	rgb[0] = (color & Bitmask) << shift;

	return 0;
}

private int
plan9_put_param_int(gs_param_list *plist, gs_param_name pname, int *pv,
	int minval, int maxval, int ecode)
{
	int code, value;
	switch(code = param_read_int(plist, pname, &value)) {
	default:
		return code;

	case 1:
		return ecode;

	case 0:
		if(value < minval || value > maxval)
			param_signal_error(plist, pname, gs_error_rangecheck);
		*pv = value;
		return (ecode < 0 ? ecode : 1);
	}
}

private int
plan9_get_params(gx_device *pdev, gs_param_list *plist)
{
	int code;
	plan9_device *idev;

	idev = (plan9_device*) pdev;
//	printf("plan9_get_params dither %d\n", idev->dither);

	if((code = gdev_prn_get_params(pdev, plist)) < 0
	 || (code = param_write_int(plist, "Dither", &idev->dither)) < 0)
		return code;
//	printf("getparams: dither=%d\n", idev->dither);
	return code;
}

private int
plan9_put_params(gx_device * pdev, gs_param_list * plist)
{
	int code;
	int dither;
	plan9_device *idev;

//	printf("plan9_put_params\n");

	idev = (plan9_device*)pdev;
	dither = idev->dither;

	code = plan9_put_param_int(plist, "Dither", &dither, 0, 1, 0);
	if(code < 0)
		return code;

	idev->dither = dither;
	return 0;
}
/*
 * plan9_open() is supposed to initialize the device.
 * there's not much to do.
 */
extern void init_p9color(void);	/* in gdevifno.c */
private int
plan9_open(gx_device *dev)
{
	int code;
	plan9_device *idev;

	idev = (plan9_device*) dev;
	idev->cmapcall = 0;
	idev->ldepth = 0;

//	printf("plan9_open gs_plan9_device.dither = %d idev->dither = %d\n",
//		gs_plan9_device.dither, idev->dither);
	init_p9color();

	return gdev_prn_open(dev);
}

/*
 * plan9_print_page() is called once for each page
 * (actually once for each copy of each page, but we won't
 * worry about that).
 */
private int
plan9_print_page(gx_device_printer *pdev, FILE *f)
{
	char *chanstr;
	uchar *buf;	/* [8192*3*8/Nbits] BUG: malloc this */
	uchar *p;
	WImage *w;
	int bpl, y;
	int x, xmod;
	int ldepth;
	int ppb[] = {8, 4, 2, 1};	/* pixels per byte */
	int bpp[] = {1, 2, 4, 8};	/* bits per pixel */
	int gsbpl;
	int dither;
	int depth;
	ulong u;
	ushort us;
	Rectangle rect;
	plan9_device *idev;
	uchar *r;

	gsbpl = gdev_prn_raster(pdev);
	buf = gs_malloc(pdev->memory, gsbpl, 1, "plan9_print_page");

	if(buf == nil) {
		errprintf("out of memory\n");
		return_error(gs_error_Fatal);
	}

	idev = (plan9_device *) pdev;
	if(idev->cmapcall) {
		idev->lastldepth = idev->ldepth;
		idev->ldepth = 0;
		idev->cmapcall = 0;
	}
	ldepth = idev->lastldepth;
	dither = idev->dither;

	if(pdev->color_info.anti_alias.graphics_bits || pdev->color_info.anti_alias.text_bits)
		if(ldepth < 2)
			ldepth = 2;

	chanstr = nil;
	depth = 0;
	switch(ldepth){
	case 0:
		chanstr = "k1";
		depth = 1;
		break;
	case 1:
		return_error(gs_error_Fatal);
	case 2:
		chanstr = "k4";
		depth = 4;
		break;
	case 3:
		chanstr = "r8g8b8";
		depth = 24;
		break;
	}

//	printf("plan9_print_page dither %d ldepth %d idither %d\n", dither, ldepth, gs_plan9_device.dither);
	rect.min = ZP;
	rect.max.x = pdev->width;
	rect.max.y = pdev->height;
	bpl = bytesperline(rect, depth);
	w = initwriteimage(f, rect, chanstr, depth);
	if(w == nil) {
		errprintf("initwriteimage failed\n");
		return_error(gs_error_Fatal);
	}

	/*
	 * i wonder if it is faster to put the switch around the for loops
	 * to save all the ldepth lookups.
	 */
	for(y=0; y<pdev->height; y++) {
		gdev_prn_get_bits(pdev, y, buf, &p);
		r = p+2;
		switch(depth){
		default:
			return_error(gs_error_Fatal);
		case 1:
			for(x=0; x<pdev->width; x++){
				if((x%8) == 0)
					p[x/8] = (*r>>4)&1;
				else
					p[x/8] = (p[x/8]<<1) | (*r>>4)&1;
				r += 3;
			}
			break;
		case 4:
			for(x=0; x<pdev->width; x++){
				if((x%2) == 0)
					p[x/2] = (*r>>4) & 0xF;
				else
					p[x/2] = (p[x/2]<<4) | ((*r>>4)&0xF);
				r += 3;
			}
			break;
		case 24:
			break;
		}

		/* pad last byte over if we didn't fill it */
		xmod = pdev->width % ppb[ldepth];
		if(xmod && ldepth<3)
			p[(x-1)/ppb[ldepth]] <<= ((ppb[ldepth]-xmod)*bpp[ldepth]);

		if(writeimageblock(w, p, bpl) == ERROR) {
			gs_free(pdev->memory, buf, gsbpl, 1, "plan9_print_page");
			return_error(gs_error_Fatal);
		}
	}
	if(writeimageblock(w, nil, 0) == ERROR) {
		gs_free(pdev->memory, buf, gsbpl, 1, "plan9_print_page");
		return_error(gs_error_Fatal);
	}

	gs_free(pdev->memory, buf, gsbpl, 1, "plan9_print_page");
	return 0;
}

/*
 * this is a modified version of the image compressor
 * from fb/bit2enc.  it is modified only in that it 
 * now compiles as part of gs.
 */

/*
 * Compressed image file parameters
 */
#define	NMATCH	3		/* shortest match possible */
#define	NRUN	(NMATCH+31)	/* longest match possible */
#define	NMEM	1024		/* window size */
#define	NDUMP	128		/* maximum length of dump */
#define	NCBLOCK	6000		/* size of compressed blocks */

#define	HSHIFT	3	/* HSHIFT==5 runs slightly faster, but hash table is 64x bigger */
#define	NHASH	(1<<(HSHIFT*NMATCH))
#define	HMASK	(NHASH-1)
#define	hupdate(h, c)	((((h)<<HSHIFT)^(c))&HMASK)

typedef struct Dump	Dump;
typedef struct Hlist Hlist;

struct Hlist{
	ulong p;
	Hlist *next, *prev;
};

struct Dump {
	int ndump;
	uchar *dumpbuf;
	uchar buf[1+NDUMP];
};

struct WImage {
	FILE *f;

	/* image attributes */
	Rectangle origr, r;
	int bpl;

	/* output buffer */
	uchar outbuf[NCBLOCK], *outp, *eout, *loutp;

	/* sliding input window */
	/*
	 * ibase is the pointer to where the beginning of
	 * the input "is" in memory.  whenever we "slide" the
	 * buffer N bytes, what we are actually doing is 
	 * decrementing ibase by N.
	 * the ulongs in the Hlist structures are just
	 * pointers relative to ibase.
	 */
	uchar *inbuf;	/* inbuf should be at least NMEM+NRUN+NMATCH long */
	uchar *ibase;
	int minbuf;	/* size of inbuf (malloc'ed bytes) */
	int ninbuf;	/* size of inbuf (filled bytes) */
	ulong line;	/* the beginning of the line we are currently encoding,
			 * relative to inbuf (NOT relative to ibase) */

	/* raw dump buffer */
	Dump dump;

	/* hash tables */
	Hlist hash[NHASH];
	Hlist chain[NMEM], *cp;
	int h;
	int needhash;
};

private void
zerohash(WImage *w)
{
	memset(w->hash, 0, sizeof(w->hash));
	memset(w->chain, 0, sizeof(w->chain));
	w->cp=w->chain;
	w->needhash = 1;
}

private int
addbuf(WImage *w, uchar *buf, int nbuf)
{
	int n;
	if(buf == nil || w->outp+nbuf > w->eout) {
		if(w->loutp==w->outbuf){	/* can't really happen -- we checked line length above */
			errprintf("buffer too small for line\n");
			return ERROR;
		}
		n=w->loutp-w->outbuf;
		fprintf(w->f, "%11d %11d ", w->r.max.y, n);
		fwrite(w->outbuf, 1, n, w->f);
		w->r.min.y=w->r.max.y;
		w->outp=w->outbuf;
		w->loutp=w->outbuf;
		zerohash(w);
		return -1;
	}

	memmove(w->outp, buf, nbuf);
	w->outp += nbuf;
	return nbuf;
}

/* return 0 on success, -1 if buffer is full */
private int
flushdump(WImage *w)
{
	int n = w->dump.ndump;

	if(n == 0)
		return 0;

	w->dump.buf[0] = 0x80|(n-1);
	if((n=addbuf(w, w->dump.buf, n+1)) == ERROR)
		return ERROR;
	if(n < 0)
		return -1;
	w->dump.ndump = 0;
	return 0;
}

private void
updatehash(WImage *w, uchar *p, uchar *ep)
{
	uchar *q;
	Hlist *cp;
	Hlist *hash;
	int h;

	hash = w->hash;
	cp = w->cp;
	h = w->h;
	for(q=p; q<ep; q++) {
		if(cp->prev)
			cp->prev->next = cp->next;
		cp->next = hash[h].next;
		cp->prev = &hash[h];
		cp->prev->next = cp;
		if(cp->next)
			cp->next->prev = cp;
		cp->p = q - w->ibase;
		if(++cp == w->chain+NMEM)
			cp = w->chain;
		if(&q[NMATCH] < &w->inbuf[w->ninbuf])
			h = hupdate(h, q[NMATCH]);
	}
	w->cp = cp;
	w->h = h;
}

/*
 * attempt to process a line of input,
 * returning the number of bytes actually processed.
 *
 * if the output buffer needs to be flushed, we flush
 * the buffer and return 0.
 * otherwise we return bpl
 */
private int
gobbleline(WImage *w)
{
	int runlen, n, offs;
	uchar *eline, *es, *best, *p, *s, *t;
	Hlist *hp;
	uchar buf[2];
	int rv;

	if(w->needhash) {
		w->h = 0;
		for(n=0; n!=NMATCH; n++)
			w->h = hupdate(w->h, w->inbuf[w->line+n]);
		w->needhash = 0;
	}
	w->dump.ndump=0;
	eline=w->inbuf+w->line+w->bpl;
	for(p=w->inbuf+w->line;p!=eline;){
		es = (eline < p+NRUN) ? eline : p+NRUN;

		best=nil;
		runlen=0;
		/* hash table lookup */
		for(hp=w->hash[w->h].next;hp;hp=hp->next){
			/*
			 * the next block is an optimization of 
			 * for(s=p, t=w->ibase+hp->p; s<es && *s == *t; s++, t++)
			 * 	;
			 */

			{	uchar *ss, *tt;
				s = p+runlen;
				t = w->ibase+hp->p+runlen;
				for(ss=s, tt=t; ss>=p && *ss == *tt; ss--, tt--)
					;
				if(ss < p)
					while(s<es && *s == *t)
						s++, t++;
			}

			n = s-p;

			if(n > runlen) {
				runlen = n;
				best = w->ibase+hp->p;
				if(p+runlen == es)
					break;
			}
		}

		/*
		 * if we didn't find a long enough run, append to 
		 * the raw dump buffer
		 */
		if(runlen<NMATCH){
			if(w->dump.ndump==NDUMP) {
				if((rv = flushdump(w)) == ERROR)
					return ERROR;
				if(rv < 0)
					return 0;
			}
			w->dump.dumpbuf[w->dump.ndump++]=*p;
			runlen=1;
		}else{
		/*
		 * otherwise, assuming the dump buffer is empty,
		 * add the compressed rep.
		 */
			if((rv = flushdump(w)) == ERROR)
				return ERROR;
			if(rv < 0)
				return 0;
			offs=p-best-1;
			buf[0] = ((runlen-NMATCH)<<2)|(offs>>8);
			buf[1] = offs&0xff;
			if(addbuf(w, buf, 2) < 0)
				return 0;
		}

		/*
		 * add to hash tables what we just encoded
		 */
		updatehash(w, p, p+runlen);
		p += runlen;
	}

	if((rv = flushdump(w)) == ERROR)
		return ERROR;
	if(rv < 0)
		return 0;
	w->line += w->bpl;
	w->loutp=w->outp;
	w->r.max.y++;
	return w->bpl;
}

private uchar*
shiftwindow(WImage *w, uchar *data, uchar *edata)
{
	int n, m;

	/* shift window over */
	if(w->line > NMEM) {
		n = w->line-NMEM;
		memmove(w->inbuf, w->inbuf+n, w->ninbuf-n);
		w->line -= n;
		w->ibase -= n;
		w->ninbuf -= n;
	}

	/* fill right with data if available */
	if(w->minbuf > w->ninbuf && edata > data) {
		m = w->minbuf - w->ninbuf;
		if(edata-data < m)
			m = edata-data;
		memmove(w->inbuf+w->ninbuf, data, m);
		data += m;
		w->ninbuf += m;
	}

	return data;
}

private WImage*
initwriteimage(FILE *f, Rectangle r, char *chanstr, int depth)
{
	WImage *w;
	int n, bpl;

	bpl = bytesperline(r, depth);
	if(r.max.y <= r.min.y || r.max.x <= r.min.x || bpl <= 0) {
		errprintf("bad rectangle, ldepth");
		return nil;
	}

	n = NMEM+NMATCH+NRUN+bpl*2;
	w = malloc(n+sizeof(*w));
	if(w == nil)
		return nil;
	w->inbuf = (uchar*) &w[1];
	w->ibase = w->inbuf;
	w->line = 0;
	w->minbuf = n;
	w->ninbuf = 0;
	w->origr = r;
	w->r = r;
	w->r.max.y = w->r.min.y;
	w->eout = w->outbuf+sizeof(w->outbuf);
	w->outp = w->loutp = w->outbuf;
	w->bpl = bpl;
	w->f = f;
	w->dump.dumpbuf = w->dump.buf+1;
	w->dump.ndump = 0;
	zerohash(w);

	fprintf(f, "compressed\n%11s %11d %11d %11d %11d ",
		chanstr, r.min.x, r.min.y, r.max.x, r.max.y);
	return w;
}

private int
writeimageblock(WImage *w, uchar *data, int ndata)
{
	uchar *edata;

	if(data == nil) {	/* end of data, flush everything */
		while(w->line < w->ninbuf)
			if(gobbleline(w) == ERROR)
				return ERROR;
		addbuf(w, nil, 0);
		if(w->r.min.y != w->origr.max.y) {
			errprintf("not enough data supplied to writeimage\n");
		}
		free(w);
		return 0;
	}

	edata = data+ndata;
	data = shiftwindow(w, data, edata);
	while(w->ninbuf >= w->line+w->bpl+NMATCH) {
		if(gobbleline(w) == ERROR)
			return ERROR;
		data = shiftwindow(w, data, edata);
	}
	if(data != edata) {
		fprintf(w->f, "data != edata.  uh oh\n");
		return ERROR; /* can't happen */
	}
	return 0;
}

/*
 * functions from the Plan9/Brazil drawing libraries 
 */
static
int
unitsperline(Rectangle r, int d, int bitsperunit)
{
	ulong l, t;

	if(d <= 0 || d > 32)	/* being called wrong.  d is image depth. */
		abort();

	if(r.min.x >= 0){
		l = (r.max.x*d+bitsperunit-1)/bitsperunit;
		l -= (r.min.x*d)/bitsperunit;
	}else{			/* make positive before divide */
		t = (-r.min.x*d+bitsperunit-1)/bitsperunit;
		l = t+(r.max.x*d+bitsperunit-1)/bitsperunit;
	}
	return l;
}

int
wordsperline(Rectangle r, int d)
{
	return unitsperline(r, d, 8*sizeof(ulong));
}

int
bytesperline(Rectangle r, int d)
{
	return unitsperline(r, d, 8);
}

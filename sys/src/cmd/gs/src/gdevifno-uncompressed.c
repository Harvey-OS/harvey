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
 * gdevifno.c: gs device to generate inferno bitmaps
 * Russ Cox <rsc@plan9.bell-labs.com>, 3/25/98
 * Updated to fit in the standard GS distribution, 5/14/98
 */

#include "gdevprn.h"
#include "gsparam.h"
#include "gxlum.h"
#include <stdlib.h>
#undef printf

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

private WImage* initwriteimage(FILE *f, Rectangle r, int ldepth);
private int writeimageblock(WImage *w, uchar *data, int ndata);
private int bytesperline(Rectangle, int);
private int rgb2cmap(int, int, int);
private long cmap2rgb(int);

#define X_DPI	100
#define Y_DPI	100

private dev_proc_map_rgb_color(inferno_rgb2cmap);
private dev_proc_map_color_rgb(inferno_cmap2rgb);
private dev_proc_open_device(inferno_open);
private dev_proc_close_device(inferno_close);
private dev_proc_print_page(inferno_print_page);
private dev_proc_put_params(inferno_put_params);
private dev_proc_get_params(inferno_get_params);

typedef struct inferno_device_s {
	gx_device_common;
	gx_prn_device_common;
	int dither;

	int ldepth;
	int lastldepth;
	int cmapcall;
} inferno_device;

enum {
	Nbits = 8,
	Bitmask = (1<<Nbits)-1,
};

private const gx_device_procs inferno_procs =
	prn_color_params_procs(inferno_open, gdev_prn_output_page, gdev_prn_close,
		inferno_rgb2cmap, inferno_cmap2rgb,
		gdev_prn_get_params, gdev_prn_put_params);
/*
		inferno_get_params, inferno_put_params);
*/


inferno_device far_data gs_inferno_device =
{ prn_device_body(inferno_device, inferno_procs, "inferno",
	DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	X_DPI, Y_DPI,
	0,0,0,0,	/* margins */
	3,		/* 3 = RGB, 1 = gray, 4 = CMYK */
	Nbits*3,		/* # of bits per pixel */
	(1<<Nbits)-1,		/* # of distinct gray levels. */
	(1<<Nbits)-1,		/* # of distinct color levels. */
	1<<Nbits,		/* dither gray ramp size.  used in alpha? */
	1<<Nbits,    	/* dither color ramp size.  used in alpha? */
	inferno_print_page),
	1,
};

/*
 * ghostscript asks us how to convert between
 * rgb and color map entries
 */
private gx_color_index 
inferno_rgb2cmap(P4(gx_device *dev, gx_color_value r,
  gx_color_value g, gx_color_value b)) {
	int shift;
	inferno_device *idev;
	ulong red, green, blue;

	idev = (inferno_device*) dev;

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
inferno_cmap2rgb(P3(gx_device *dev, gx_color_index color,
  gx_color_value rgb[3])) {
	int shift, i;
	inferno_device *idev;

	if((ulong)color > 0xFFFFFF)
		return_error(gs_error_rangecheck);

	idev = (inferno_device*) dev;
	shift = gx_color_value_bits - Nbits;

	rgb[2] = ((color >> (2*Nbits)) & Bitmask) << shift;
	rgb[1] = ((color >> Nbits) & Bitmask) << shift;
	rgb[0] = (color & Bitmask) << shift;

	return 0;
}

private int
inferno_put_param_int(gs_param_list *plist, gs_param_name pname, int *pv,
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
inferno_get_params(gx_device *pdev, gs_param_list *plist)
{
	int code;
	inferno_device *idev;

	idev = (inferno_device*) pdev;
//	printf("inferno_get_params dither %d\n", idev->dither);

	if((code = gdev_prn_get_params(pdev, plist)) < 0
	 || (code = param_write_int(plist, "Dither", &idev->dither)) < 0)
		return code;
	printf("getparams: dither=%d\n", idev->dither);
	return code;
}

private int
inferno_put_params(gx_device * pdev, gs_param_list * plist)
{
	int code;
	int dither;
	inferno_device *idev;

	printf("inferno_put_params\n");

	idev = (inferno_device*)pdev;
	dither = idev->dither;

	code = inferno_put_param_int(plist, "Dither", &dither, 0, 1, 0);
	if(code < 0)
		return code;

	idev->dither = dither;
	return 0;
}

/*
 * dithering tables courtesy of john hobby
 */
/* The following constants and tables define the mapping from fine-grained RGB
   triplets to 8-bit values based on the standard Plan 9 color map.
*/
#define Rlevs	16		/* number of levels to cut red value into */
#define Glevs	16
#define Blevs	16
#define Mlevs	16
#define Rfactor 1		/* multiple of red level in p9color[] index */
#define Gfactor Rlevs
#define Bfactor	(Rlevs*Glevs)
			
ulong p9color[Rlevs*Glevs*Blevs];	/* index blue most sig, red least sig */

void init_p9color(void)		/* init at run time since p9color[] is so big */
{
	int r, g, b, o;
	ulong* cur = p9color;
	for (b=0; b<16; b++) {
	    for (g=0; g<16; g++) {
		int m0 = (b>g) ? b : g;
		for (r=0; r<16; r++) {
		    int V, M, rM, gM, bM, m;
		    int m1 = (r>m0) ? r : m0;
		    V=m1&3; M=(m1-V)<<1;
		    if (m1==0) m1=1;
		    m = m1 << 3;
		    rM=r*M; gM=g*M; bM=b*M;
		    *cur = 0;
		    for (o=7*m1; o>0; o-=2*m1) {
			int rr=(rM+o)/m, gg=(gM+o)/m, bb=(bM+o)/m;
			int ij = (rr<<6) + (V<<4) + ((V-rr+(gg<<2)+bb)&15);
			*cur = (*cur << 8) + 255-ij;
		    }
		    cur++;
		}
	    }
	}
}

/*
 * inferno_open() is supposed to initialize the device.
 * there's not much to do.
 */
private int
inferno_open(P1(gx_device *dev))
{
	int code;
	inferno_device *idev;

	idev = (inferno_device*) dev;
	idev->cmapcall = 0;
	idev->ldepth = 0;

//	printf("inferno_open gs_inferno_device.dither = %d idev->dither = %d\n",
//		gs_inferno_device.dither, idev->dither);
	setbuf(stderr, 0);
	init_p9color();

	return gdev_prn_open(dev);
}

/*
 * inferno_print_page() is called once for each page
 * (actually once for each copy of each page, but we won't
 * worry about that).
 */
private int
inferno_print_page(P2(gx_device_printer *pdev, FILE *f))
{
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
	ulong u;
	ushort us;
	Rectangle rect;
	inferno_device *idev;
	ulong r, g, b;

	setbuf(stderr, 0);
	gsbpl = gdev_prn_raster(pdev);
	buf = gs_malloc(gsbpl, 1, "inferno_print_page");

	if(buf == nil) {
		fprintf(stderr, "out of memory\n");
		return_error(gs_error_Fatal);
	}

	idev = (inferno_device *) pdev;
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

//	printf("inferno_print_page dither %d ldepth %d idither %d\n", dither, ldepth, gs_inferno_device.dither);
	rect.min = ZP;
	rect.max.x = pdev->width;
	rect.max.y = pdev->height;
	bpl = bytesperline(rect, ldepth);
	w = initwriteimage(f, rect, ldepth);
	if(w == nil) {
		fprintf(stderr, "initwriteimage failed\n");
		return_error(gs_error_Fatal);
	}

	/*
	 * i wonder if it is faster to put the switch around the for loops
	 * to save all the ldepth lookups.
	 */
	for(y=0; y<pdev->height; y++) {
		gdev_prn_get_bits(pdev, y, buf, &p);
		for(x=0; x<pdev->width; x++) {
			b = p[3*x];
			g = p[3*x+1];
			r = p[3*x+2];
			us = ((b>>4) << 8) | ((g>>4) << 4) | (r>>4);
			switch(ldepth) {
			case 3:
				if(1 || dither){
					u = p9color[us];
					/* the ulong in p9color is a 2x2 matrix.  pull the entry
					 * u[x%2][y%2], more or less.
					 */
					p[x] = u >> (8*((y%2)+2*(x%2)));
				} else {
					p[x] = rgb2cmap(r, g, b);
				}
				break;
			case 2:
				us = ~us;
				if((x%2) == 0)
					p[x/2] = us & 0xf;
				else
					p[x/2] = (p[x/2]<<4)|(us&0xf);
				break;
			case 1:
				return_error(gs_error_Fatal);
			case 0:
				us = ~us;
				if((x%8) == 0)
					p[x/8] = us & 0x1;
				else
					p[x/8] = (p[x/8]<<1)|(us&0x1);
				break;
			}
		}

		/* pad last byte over if we didn't fill it */
		xmod = pdev->width % ppb[ldepth];
		if(xmod)
			p[(x-1)/ppb[ldepth]] <<= ((ppb[ldepth]-xmod)*bpp[ldepth]);
		if(writeimageblock(w, p, bpl) == ERROR) {
			gs_free(buf, gsbpl, 1, "inferno_print_page");
			return_error(gs_error_Fatal);
		}
	}
	if(writeimageblock(w, nil, 0) == ERROR) {
		gs_free(buf, gsbpl, 1, "inferno_print_page");
		return_error(gs_error_Fatal);
	}

	gs_free(buf, gsbpl, 1, "inferno_print_page");
	return 0;
}

private WImage*
initwriteimage(FILE *f, Rectangle r, int ldepth)
{
	fprintf(f, "%11d %11d %11d %11d %11d ",
		ldepth, r.min.x, r.min.y, r.max.x, r.max.y);

	return (WImage*)f;
}

private int
writeimageblock(WImage *w, uchar *data, int ndata)
{
	fwrite(data, 1, ndata, (FILE*)w);
	return 0;
}

private int
bytesperline(Rectangle r, int ld)
{
	ulong ws, l, t;
	int bits = 8;

	ws = bits>>ld;	/* pixels per unit */
	if(r.min.x >= 0){
		l = (r.max.x+ws-1)/ws;
		l -= r.min.x/ws;
	}else{			/* make positive before divide */
		t = (-r.min.x)+ws-1;
		t = (t/ws)*ws;
		l = (t+r.max.x+ws-1)/ws;
	}
	return l;
}

private int
rgb2cmap(int cr, int cg, int cb)
{
	int r, g, b, v, cv;

	if(cr < 0)
		cr = 0;
	else if(cr > 255)
		cr = 255;
	if(cg < 0)
		cg = 0;
	else if(cg > 255)
		cg = 255;
	if(cb < 0)
		cb = 0;
	else if(cb > 255)
		cb = 255;
	r = cr>>6;
	g = cg>>6;
	b = cb>>6;
	cv = cr;
	if(cg > cv)
		cv = cg;
	if(cb > cv)
		cv = cb;
	v = (cv>>4)&3;
	return 255-((((r<<2)+v)<<4)+(((g<<2)+b+v-r)&15));
}

/*
 * go the other way; not currently used.
 *
private long
cmap2rgb(int c)
{
	int j, num, den, r, g, b, v, rgb;

	c = 255-c;
	r = c>>6;
	v = (c>>4)&3;
	j = (c-v+r)&15;
	g = j>>2;
	b = j&3;
	den=r;
	if(g>den)
		den=g;
	if(b>den)
		den=b;
	if(den==0) {
		v *= 17;
		rgb = (v<<16)|(v<<8)|v;
	}
	else{
		num=17*(4*den+v);
		rgb = ((r*num/den)<<16)|((g*num/den)<<8)|(b*num/den);
	}
	return rgb;
}
 *
 *
 */


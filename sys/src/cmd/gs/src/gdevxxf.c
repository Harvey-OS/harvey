/* Copyright (C) 1993, 2000 Aladdin Enterprises.  All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: gdevxxf.c,v 1.9 2004/08/04 19:36:12 stefan Exp $ */
/* External font (xfont) implementation for X11. */
#include "math_.h"
#include "memory_.h"
#include "x_.h"
#include "gx.h"
#include "gxdevice.h"
#include "gdevx.h"
#include "gsstruct.h"
#include "gsutil.h"
#include "gserrors.h"
#include "gxxfont.h"

/* Define the smallest point size that we trust X to render reasonably well. */
#define min_X_font_size 6
/* Define the largest point size where X will do a better job than we can. */
#define max_X_font_size 35

extern gx_device_X gs_x11_device;

extern const byte gs_map_std_to_iso[256];
extern const byte gs_map_iso_to_std[256];

/* Declare the xfont procedures */
private xfont_proc_lookup_font(x_lookup_font);
private xfont_proc_char_xglyph(x_char_xglyph);
private xfont_proc_char_metrics(x_char_metrics);
private xfont_proc_render_char(x_render_char);
private xfont_proc_release(x_release);
private const gx_xfont_procs x_xfont_procs =
{
    x_lookup_font,
    x_char_xglyph,
    x_char_metrics,
    x_render_char,
    x_release
};

/* Return the xfont procedure record. */
const gx_xfont_procs *
gdev_x_get_xfont_procs(gx_device * dev)
{
    return &x_xfont_procs;
}

/* Define a X11 xfont. */
typedef struct x_xfont_s x_xfont;
struct x_xfont_s {
    gx_xfont_common common;
    gx_device_X *xdev;
    XFontStruct *font;
    int encoding_index;
    int My;
    int angle;
};

gs_private_st_dev_ptrs1(st_x_xfont, x_xfont, "x_xfont",
			x_xfont_enum_ptrs, x_xfont_reloc_ptrs, xdev);

/* ---------------- Utilities ---------------- */

/* Search one set of font maps for a font with a given name. */
private x11fontmap *
find_fontmap(x11fontmap *fmps, const byte *fname, uint len)
{
    x11fontmap *fmp = fmps;

    while (fmp) {
	if (len == strlen(fmp->ps_name) &&
	    strncmp(fmp->ps_name, (const char *)fname, len) == 0)
	    break;
	fmp = fmp->next;
    }
    return fmp;
}

/* Find an X font with a given name, encoding, and size. */
private char *
find_x_font(gx_device_X *xdev, char x11template[256], x11fontmap *fmp,
	    const char *encoding_name, x11fontlist *fls, int xheight,
	    bool *scalable_font)
{
    int i;
    char *x11fontname = 0;
    int len1 = strlen(fmp->x11_name) + 1;

    if (fls->count == -1) {
	sprintf(x11template, "%s-*-*-*-*-*-*-%s", fmp->x11_name,
		encoding_name);
	fls->names = XListFonts(xdev->dpy, x11template, 32, &fls->count);
    }
    *scalable_font = false;
    for (i = 0; i < fls->count; i++) {
	const char *szp = fls->names[i] + len1;
	int size = 0;

	while (*szp >= '0' && *szp <= '9')
	    size = size * 10 + *szp++ - '0';
	if (size == 0) {
	    *scalable_font = true;
	    continue;
	}
	if (size == xheight)
	    return fls->names[i];
    }
    if (*scalable_font && xdev->useScalableFonts) {
	sprintf(x11template, "%s-%d-0-0-0-*-0-%s", fmp->x11_name,
		xheight, encoding_name);
	x11fontname = x11template;
    }
    return x11fontname;
}

/* ---------------- xfont procedures ---------------- */

/* Look up a font. */
private gx_xfont *
x_lookup_font(gx_device * dev, const byte * fname, uint len,
	    int encoding_index, const gs_uid * puid, const gs_matrix * pmat,
	      gs_memory_t * mem)
{
    gx_device_X *xdev = (gx_device_X *) dev;
    x_xfont *xxf;
    char x11template[256];
    char *x11fontname = NULL;
    XFontStruct *x11font;
    x11fontmap *fmp;
    double height;
    int xwidth, xheight, angle;
    Boolean My;
    bool scalable_font;

    if (!xdev->useXFonts)
	return NULL;

    if (pmat->xy == 0 && pmat->yx == 0) {
	xwidth = fabs(pmat->xx * 1000) + 0.5;
	xheight = fabs(pmat->yy * 1000) + 0.5;
	height = fabs(pmat->yy * 1000);
	angle = (pmat->xx > 0 ? 0 : 180);
	My = (pmat->xx > 0 && pmat->yy > 0) || (pmat->xx < 0 && pmat->yy < 0);
    } else if (pmat->xx == 0 && pmat->yy == 0) {
	xwidth = fabs(pmat->xy * 1000) + 0.5;
	xheight = fabs(pmat->yx * 1000) + 0.5;
	height = fabs(pmat->yx * 1000);
	angle = (pmat->yx < 0 ? 90 : 270);
	My = (pmat->yx > 0 && pmat->xy < 0) || (pmat->yx < 0 && pmat->xy > 0);
    } else {
	return NULL;
    }

    /* Don't do very small fonts, where font metrics are way off */
    /* due to rounding and the server does a very bad job of scaling, */
    /* or very large fonts, where we can do just as good a job and */
    /* the server may lock up the entire window system while rasterizing */
    /* the whole font. */
    if (xwidth < min_X_font_size || xwidth > max_X_font_size ||
	xheight < min_X_font_size || xheight > max_X_font_size
	)
	return NULL;

    if (!xdev->useFontExtensions && (My || angle != 0))
	return NULL;

    switch (encoding_index) {
    case 0:
	fmp = find_fontmap(xdev->regular_fonts, fname, len);
	if (fmp == NULL)
	    return NULL;
	x11fontname =
	    find_x_font(xdev, x11template, fmp, "Adobe-fontspecific",
			&fmp->std, xheight, &scalable_font);
	if (!x11fontname) {
	    x11fontname =
		find_x_font(xdev, x11template, fmp, "ISO8859-1",
			    &fmp->iso, xheight, &scalable_font);
	    encoding_index = 1;
	}
	break;
    case 1:
	fmp = find_fontmap(xdev->regular_fonts, fname, len);
	if (fmp == NULL)
	    return NULL;
	x11fontname =
	    find_x_font(xdev, x11template, fmp, "ISO8859-1",
			&fmp->iso, xheight, &scalable_font);
	if (!x11fontname) {
	    x11fontname =
		find_x_font(xdev, x11template, fmp, "Adobe-fontspecific",
			    &fmp->std, xheight, &scalable_font);
	    encoding_index = 0;
	}
	break;
    case 2:
	fmp = xdev->symbol_fonts;
	goto sym;
    case 3:
	fmp = xdev->dingbat_fonts;
sym:	fmp = find_fontmap(fmp, fname, len);
	if (fmp == NULL)
	    return NULL;
	x11fontname =
	    find_x_font(xdev, x11template, fmp, "Adobe-fontspecific",
			&fmp->std, xheight, &scalable_font);
    default:
	return NULL;
    }
    if (!x11fontname)
	return NULL;

    if (xwidth != xheight || angle != 0 || My) {
	if (!xdev->useScalableFonts || !scalable_font)
	    return NULL;
	sprintf(x11template, "%s%s+%d-%d+%d-0-0-0-*-0-%s",
		fmp->x11_name, (My ? "+My" : ""),
		angle * 64, xheight, xwidth,
		(encoding_index == 1 ? "ISO8859-1" : "Adobe-fontspecific"));
	x11fontname = x11template;
    }
    x11font = XLoadQueryFont(xdev->dpy, x11fontname);
    if (x11font == NULL)
	return NULL;
    /* Don't bother with 16-bit or 2 byte fonts yet */
    if (x11font->min_byte1 || x11font->max_byte1) {
	XFreeFont(xdev->dpy, x11font);
	return NULL;
    }
    xxf = gs_alloc_struct(mem, x_xfont, &st_x_xfont, "x_lookup_font");
    if (xxf == NULL)
	return NULL;
    xxf->common.procs = &x_xfont_procs;
    xxf->xdev = xdev;
    xxf->font = x11font;
    xxf->encoding_index = encoding_index;
    xxf->My = (My ? -1 : 1);
    xxf->angle = angle;
    if (xdev->logXFonts) {
	dprintf3("Using %s\n  for %s at %g pixels.\n", x11fontname,
		 fmp->ps_name, height);
	dflush();
    }
    return (gx_xfont *) xxf;
}

/* Convert a character name or index to an xglyph code. */
private gx_xglyph
x_char_xglyph(gx_xfont * xf, gs_char chr, int encoding_index,
	      gs_glyph glyph, const gs_const_string *glyph_name)
{
    const x_xfont *xxf = (x_xfont *) xf;

    if (chr == gs_no_char)
	return gx_no_xglyph;	/* can't look up names yet */
    if (encoding_index != xxf->encoding_index) {
	if (encoding_index == 0 && xxf->encoding_index == 1)
	    chr = gs_map_std_to_iso[chr];
	else if (encoding_index == 1 && xxf->encoding_index == 0)
	    chr = gs_map_iso_to_std[chr];
	else
	    return gx_no_xglyph;
	if (chr == 0)
	    return gx_no_xglyph;
    }
    if (chr < xxf->font->min_char_or_byte2 ||
	chr > xxf->font->max_char_or_byte2)
	return gx_no_xglyph;
    if (xxf->font->per_char) {
	int i = chr - xxf->font->min_char_or_byte2;
	const XCharStruct *xc = &xxf->font->per_char[i];

	if ((xc->lbearing == 0) && (xc->rbearing == 0) &&
	    (xc->ascent == 0) && (xc->descent == 0))
	    return gx_no_xglyph;
    }
    return (gx_xglyph) chr;
}

/* Get the metrics for a character. */
private int
x_char_metrics(gx_xfont * xf, gx_xglyph xg, int wmode,
	       gs_point * pwidth, gs_int_rect * pbbox)
{
    const x_xfont *xxf = (const x_xfont *) xf;
    int width;

    if (wmode != 0)
	return gs_error_undefined;
    if (xxf->font->per_char == NULL) {
	width = xxf->font->max_bounds.width;
	pbbox->p.x = xxf->font->max_bounds.lbearing;
	pbbox->q.x = xxf->font->max_bounds.rbearing;
	pbbox->p.y = -xxf->font->max_bounds.ascent;
	pbbox->q.y = xxf->font->max_bounds.descent;
    } else {
	int i = xg - xxf->font->min_char_or_byte2;
	const XCharStruct *xc = &xxf->font->per_char[i];

	width = xc->width;
	pbbox->p.x = xc->lbearing;
	pbbox->q.x = xc->rbearing;
	pbbox->p.y = -xc->ascent;
	pbbox->q.y = xc->descent;
    }
    switch (xxf->angle) {
    case 0:
	pwidth->x = width, pwidth->y = 0; break;
    case 90:
	pwidth->x = 0, pwidth->y = -xxf->My * width; break;
    case 180:
	pwidth->x = -width, pwidth->y = 0; break;
    case 270:
	pwidth->x = 0, pwidth->y = xxf->My * width; break;
    }
    return 0;
}

/* Render a character. */
private int
x_render_char(gx_xfont * xf, gx_xglyph xg, gx_device * dev,
	      int xo, int yo, gx_color_index color, int required)
{
    x_xfont *xxf = (x_xfont *) xf;
    char chr = (char)xg;
    gs_point wxy;
    gs_int_rect bbox;
    int x, y, w, h;
    int code;

    if (dev->dname == gs_x11_device.dname && !((gx_device_X *)dev)->is_buffered) {
	gx_device_X *xdev = (gx_device_X *)dev;

	code = (*xf->common.procs->char_metrics) (xf, xg, 0, &wxy, &bbox);
	if (code < 0)
	    return code;
	/* Buffer text for more efficient X interaction. */
	if (xdev->text.item_count == MAX_TEXT_ITEMS ||
	    xdev->text.char_count == MAX_TEXT_CHARS ||
	    (IN_TEXT(xdev) &&
	     (yo != xdev->text.origin.y || color != xdev->fore_color ||
	      xxf->font->fid != xdev->fid))
	    ) {
	    DRAW_TEXT(xdev);
	    xdev->text.item_count = xdev->text.char_count = 0;
	}
	if (xdev->text.item_count == 0) {
	    X_SET_FILL_STYLE(xdev, FillSolid);
	    X_SET_FORE_COLOR(xdev, color);
	    X_SET_FUNCTION(xdev, GXcopy);
	    xdev->text.origin.x = xdev->text.x = xo;
	    xdev->text.origin.y = yo;
	    xdev->text.items[0].font = xdev->fid = xxf->font->fid;
	}
	/*
	 * The following is wrong for rotated text, but it doesn't matter,
	 * because the next call of x_render_char will have a different Y.
	 */
	{
	    int index = xdev->text.item_count;
	    XTextItem *item = &xdev->text.items[index];
	    char *pchar = &xdev->text.chars[xdev->text.char_count++];
	    int delta = xo - xdev->text.x;

	    *pchar = chr;
	    if (index > 0 && delta == 0) {
		/* Continue the same item. */
		item[-1].nchars++;
	    } else {
		/* Start a new item. */
		item->chars = pchar;
		item->nchars = 1;
		item->delta = delta;
		if (index > 0)
		    item->font = None;
		xdev->text.item_count++;
	    }
	    xdev->text.x = xo + wxy.x;
	}
	if (xdev->bpixmap != (Pixmap) 0) {
	    x = xo + bbox.p.x;
	    y = yo + bbox.p.y;
	    w = bbox.q.x - bbox.p.x;
	    h = bbox.q.y - bbox.p.y;
	    fit_fill(dev, x, y, w, h);
	    x_update_add(xdev, x, y, w, h);
	}
	return 0;
    } else if (!required)
	return -1;		/* too hard */
    else {
	/* Display on an intermediate bitmap, then copy the bits. */
	gx_device_X *xdev = xxf->xdev;
	int wbm, raster;
	int i;
	XImage *xim;
	Pixmap xpm;
	GC fgc;
	byte *bits;

	dev_proc_copy_mono((*copy_mono)) = dev_proc(dev, copy_mono);

	code = (*xf->common.procs->char_metrics) (xf, xg, 0, &wxy, &bbox);
	if (code < 0)
	    return code;
	w = bbox.q.x - bbox.p.x;
	h = bbox.q.y - bbox.p.y;
	wbm = ROUND_UP(w, align_bitmap_mod * 8);
	raster = wbm >> 3;
	bits = (byte *) gs_malloc(xdev->memory, h, raster, "x_render_char");
	if (bits == 0)
	    return gs_error_limitcheck;
	xpm = XCreatePixmap(xdev->dpy, xdev->win, w, h, 1);
	fgc = XCreateGC(xdev->dpy, xpm, None, NULL);
	XSetForeground(xdev->dpy, fgc, 0);
	XFillRectangle(xdev->dpy, xpm, fgc, 0, 0, w, h);
	XSetForeground(xdev->dpy, fgc, 1);
	XSetFont(xdev->dpy, fgc, xxf->font->fid);
	XDrawString(xdev->dpy, xpm, fgc, -bbox.p.x, -bbox.p.y, &chr, 1);
	xim = XGetImage(xdev->dpy, xpm, 0, 0, w, h, 1, ZPixmap);
	i = 0;
	for (y = 0; y < h; y++) {
	    char b = 0;

	    for (x = 0; x < wbm; x++) {
		b = b << 1;
		if (x < w)
		    b += XGetPixel(xim, x, y);
		if ((x & 7) == 7)
		    bits[i++] = b;
	    }
	}
	code = (*copy_mono) (dev, bits, 0, raster, gx_no_bitmap_id,
			     xo + bbox.p.x, yo + bbox.p.y, w, h,
			     gx_no_color_index, color);
	gs_free(xdev->memory, (char *)bits, h, raster, "x_render_char");
	XFreePixmap(xdev->dpy, xpm);
	XFreeGC(xdev->dpy, fgc);
	XDestroyImage(xim);
	return (code < 0 ? code : 0);
    }
}

/* Release an xfont. */
private int
x_release(gx_xfont * xf, gs_memory_t * mem)
{
#if 0
    /* The device may not be open.  Cannot reliably free the font. */
    x_xfont *xxf = (x_xfont *) xf;

    XFreeFont(xxf->xdev->dpy, xxf->font);
#endif
    if (mem != NULL)
	gs_free_object(mem, xf, "x_release");
    return 0;
}

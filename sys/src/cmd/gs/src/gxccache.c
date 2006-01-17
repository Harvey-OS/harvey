/* Copyright (C) 1989, 1995, 1996, 1997, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxccache.c,v 1.34 2005/06/15 18:40:07 igor Exp $ */
/* Fast case character cache routines for Ghostscript library */
#include "memory_.h"
#include "gx.h"
#include "gpcheck.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gscencs.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gzstate.h"
#include "gzpath.h"
#include "gxdevice.h"
#include "gxdevmem.h"
#include "gzcpath.h"
#include "gxchar.h"
#include "gxfont.h"
#include "gxfcache.h"
#include "gxxfont.h"
#include "gscspace.h"		/* for gsimage.h */
#include "gsimage.h"
#include "gxhttile.h"

/* Forward references */
private byte *compress_alpha_bits(const cached_char *, gs_memory_t *);

/* Define a scale factor of 1. */
private const gs_log2_scale_point scale_log2_1 =
{0, 0};

void
gx_compute_char_matrix(const gs_matrix *char_tm, const gs_log2_scale_point *log2_scale, 
    float *mxx, float *mxy, float *myx, float *myy)
{
    int scale_x = 1 << log2_scale->x;
    int scale_y = 1 << log2_scale->y;

    *mxx = char_tm->xx * scale_x;
    *mxy = char_tm->xy * scale_x;
    *myx = char_tm->yx * scale_y;
    *myy = char_tm->yy * scale_y;
}

void
gx_compute_ccache_key(gs_font * pfont, const gs_matrix *char_tm, 
    const gs_log2_scale_point *log2_scale, bool design_grid,
    float *mxx, float *mxy, float *myx, float *myy)
{
    if (design_grid && 
	    (pfont->FontType == ft_TrueType || pfont->FontType == ft_CID_TrueType)) {
	/* 
	 * We need a special face for this case, because the TT interpreter
	 * can't generate both grid_fitted and non-grid-fitted outlines
	 * with a same face instance. This happens due to control
	 * values in 'cvt' must be different. 
	 * Since a single face satisfies all font sizes,
	 * we use a zero matrix as the cache entry key.
	 */
	*mxx = *mxy = *myx = *myy = 0;
    } else
	gx_compute_char_matrix(char_tm, log2_scale, mxx, mxy, myx, myy);
}

/* Look up, and if necessary add, a font/matrix pair in the cache */
int
gx_lookup_fm_pair(gs_font * pfont, const gs_matrix *char_tm, 
    const gs_log2_scale_point *log2_scale, bool design_grid, cached_fm_pair **ppair)
{
    float mxx, mxy, myx, myy;
    gs_font *font = pfont;
    register gs_font_dir *dir = font->dir;
    register cached_fm_pair *pair =
    dir->fmcache.mdata + dir->fmcache.mnext;
    int count = dir->fmcache.mmax;
    gs_uid uid;

    gx_compute_ccache_key(pfont, char_tm, log2_scale, design_grid,
			    &mxx, &mxy, &myx, &myy);
    if (font->FontType == ft_composite || font->PaintType != 0) {	/* We can't cache by UID alone. */
	uid_set_invalid(&uid);
    } else {
	uid = ((gs_font_base *) font)->UID;
	if (uid_is_valid(&uid))
	    font = 0;
    }
    while (count--) {
	if (pair == dir->fmcache.mdata)
	    pair += dir->fmcache.mmax;
	pair--;
	/* We have either a non-zero font and an invalid UID, */
	/* or a zero font and a valid UID. */
	/* We have to break up the test */
	/* because of a bug in the Zortech compiler. */
	if (font != 0) {
	    if (pair->font != font)
		continue;
	} else {
	    if (!uid_equal(&pair->UID, &uid) ||
		pair->FontType != pfont->FontType
		)
		continue;
	}
	if (pair->mxx == mxx && pair->mxy == mxy &&
	    pair->myx == myx && pair->myy == myy
	    && pair->design_grid == design_grid) {
	    if (pair->font == 0) {
		pair->font = pfont;
		if_debug2('k', "[k]updating pair 0x%lx with font 0x%lx\n",
			  (ulong) pair, (ulong) pfont);
	    } else {
		if_debug2('k', "[k]found pair 0x%lx: font=0x%lx\n",
			  (ulong) pair, (ulong) pair->font);
	    }
	    *ppair = pair;
	    return 0;
	}
    }
    return gx_add_fm_pair(dir, pfont, &uid, char_tm, log2_scale, design_grid, ppair);
}

/* Look up a glyph with the right depth in the cache. */
/* Return the cached_char or 0. */
cached_char *
gx_lookup_cached_char(const gs_font * pfont, const cached_fm_pair * pair,
		      gs_glyph glyph, int wmode, int depth, 
		      gs_fixed_point *subpix_origin)
{
    gs_font_dir *dir = pfont->dir;
    uint chi = chars_head_index(glyph, pair);
    register cached_char *cc;

    while ((cc = dir->ccache.table[chi & dir->ccache.table_mask]) != 0) {
	if (cc->code == glyph && cc_pair(cc) == pair &&
	    cc->subpix_origin.x == subpix_origin->x && 
	    cc->subpix_origin.y == subpix_origin->y &&
	    cc->wmode == wmode && cc_depth(cc) == depth
	    ) {
	    if_debug4('K', "[K]found 0x%lx (depth=%d) for glyph=0x%lx, wmode=%d\n",
		      (ulong) cc, cc_depth(cc), (ulong) glyph, wmode);
	    return cc;
	}
	chi++;
    }
    if_debug3('K', "[K]not found: glyph=0x%lx, wmode=%d, depth=%d\n",
	      (ulong) glyph, wmode, depth);
    return 0;
}

/* Look up a character in an external font. */
/* Return the cached_char or 0. */
cached_char *
gx_lookup_xfont_char(const gs_state * pgs, cached_fm_pair * pair,
		     gs_char chr, gs_glyph glyph, int wmode)
{
    gs_font *font = pair->font;
    int enc_index;
    gx_xfont *xf;
    gx_xglyph xg;
    gs_log2_scale_point log2_scale;
    gs_point wxy;
    gs_int_rect bbox;
    cached_char *cc;

    if (font == 0)
	return NULL;
    enc_index =
	(font->FontType == ft_composite ? -1 :
	 ((gs_font_base *) font)->nearest_encoding_index);
    if (!pair->xfont_tried) {	/* Look for an xfont now. */
	gx_lookup_xfont(pgs, pair, enc_index);
	pair->xfont_tried = true;
    }
    xf = pair->xfont;
    if (xf == 0)
	return NULL;
    {
	const gx_xfont_procs *procs = xf->common.procs;
	gs_const_string gstr;
	int code = font->procs.glyph_name(font, glyph, &gstr);

	if (code < 0)
	    return NULL;
	if (enc_index >= 0 && ((gs_font_base *)font)->encoding_index < 0) {
	    /*
	     * Use the registered encoding only if this glyph
	     * is the same as the one in the registered encoding.
	     */
	    gs_const_string kstr;

	    if (gs_c_glyph_name(gs_c_known_encode(chr, enc_index), &kstr) < 0 ||
		kstr.size != gstr.size ||
		memcmp(kstr.data, gstr.data, kstr.size)
		)
		enc_index = -1;
	}
	xg = procs->char_xglyph(xf, chr, enc_index, glyph, &gstr);
	if (xg == gx_no_xglyph)
	    return NULL;
	if ((*procs->char_metrics) (xf, xg, wmode, &wxy, &bbox) < 0)
	    return NULL;
    }
    log2_scale.x = log2_scale.y = 1;
    cc = gx_alloc_char_bits(font->dir, NULL, NULL, 
		(ushort)(bbox.q.x - bbox.p.x), (ushort)(bbox.q.y - bbox.p.y), 
		&log2_scale, 1);
    if (cc == 0)
	return NULL;
    /* Success.  Make the cache entry. */
    cc->code = glyph;
    cc->wmode = wmode;
    cc->xglyph = xg;
    cc->wxy.x = float2fixed(wxy.x);
    cc->wxy.y = float2fixed(wxy.y);
    cc->offset.x = int2fixed(-bbox.p.x);
    cc->offset.y = int2fixed(-bbox.p.y);
    cc_set_pair(cc, pair);
    if_debug5('k', "[k]xfont %s char %d/0x%x#0x%lx=>0x%lx\n",
	      font->font_name.chars, enc_index, (int)chr,
	      (ulong) glyph, (ulong) xg);
    if_debug6('k', "     wxy=(%g,%g) bbox=(%d,%d),(%d,%d)\n",
	      wxy.x, wxy.y, bbox.p.x, bbox.p.y, bbox.q.x, bbox.q.y);
    gx_add_cached_char(font->dir, NULL, cc, pair, &scale_log2_1);
    return cc;
}

/* Copy a cached character to the screen. */
/* Assume the caller has already done gx_color_load. */
/* Return 0 if OK, 1 if we couldn't do the operation but no error */
/* should be signalled, or a negative error code. */
int
gx_image_cached_char(register gs_show_enum * penum, register cached_char * cc)
{
    register gs_state *pgs = penum->pgs;
    gx_device_color *pdevc = pgs->dev_color;
    int x, y, w, h, depth;
    int code;
    gs_fixed_point pt;
    gx_device *dev = penum->dev;
    gx_device *imaging_dev = penum->imaging_dev ? penum->imaging_dev : dev;
    gx_device *orig_dev = imaging_dev;
    gx_device_clip cdev;
    gx_xglyph xg = cc->xglyph;
    gx_xfont *xf;
    byte *bits;

  top:code = gx_path_current_point_inline(pgs->path, &pt);
    if (code < 0)
	return code;
    /*
     * If the character doesn't lie entirely within the inner
     * clipping rectangle, we set up an intermediate clipping device.
     * Note that if the original device implements fill_mask, we may
     * never actually use the clipping device.
     */
    pt.x -= cc->offset.x + cc->subpix_origin.x;
    x = fixed2int_var_rounded(pt.x) + penum->ftx;
    pt.y -= cc->offset.y + cc->subpix_origin.y;
    y = fixed2int_var_rounded(pt.y) + penum->fty;
    w = cc->width;
    h = cc->height;
#ifdef DEBUG
    if (gs_debug_c('K')) {
	if (cc_has_bits(cc))
	    debug_dump_bitmap(cc_bits(cc), cc_raster(cc), h,
			      "[K]bits");
	else
	    dputs("[K]no bits\n");
	dlprintf3("[K]copying 0x%lx, offset=(%g,%g)\n", (ulong) cc,
		  fixed2float(-cc->offset.x),
		  fixed2float(-cc->offset.y));
	dlprintf6("   at (%g,%g)+(%d,%d)->(%d,%d)\n",
		  fixed2float(pt.x), fixed2float(pt.y),
		  penum->ftx, penum->fty, x, y);
    }
#endif
    if ((x < penum->ibox.p.x || x + w > penum->ibox.q.x ||
	 y < penum->ibox.p.y || y + h > penum->ibox.q.y) &&
	imaging_dev != (gx_device *) & cdev	/* might be 2nd time around */
	) {			/* Check for the character falling entirely outside */
	/* the clipping region. */
	gx_clip_path *pcpath;

	if (x >= penum->obox.q.x || x + w <= penum->obox.p.x ||
	    y >= penum->obox.q.y || y + h <= penum->obox.p.y
	    )
	    return 0;		/* nothing to do */
	code = gx_effective_clip_path(pgs, &pcpath);
	if (code < 0)
	    return code;
	gx_make_clip_device(&cdev, gx_cpath_list(pcpath));
	cdev.target = imaging_dev;
	imaging_dev = (gx_device *) & cdev;
	(*dev_proc(imaging_dev, open_device)) (imaging_dev);
	if_debug0('K', "[K](clipping)\n");
    }
    gx_set_dev_color(pgs);
    /* If an xfont can render this character, use it. */
    if (xg != gx_no_xglyph && (xf = cc_pair(cc)->xfont) != 0) {
	int cx = x + fixed2int(cc->offset.x);
	int cy = y + fixed2int(cc->offset.y);

	/*
	 * Note that we prefer a 1-bit xfont implementation over
	 * a multi-bit cached bitmap.  Eventually we should change
	 * the xfont interface so it can deliver multi-bit bitmaps,
	 * or else implement oversampling for xfonts.
	 */
	if (gs_color_writes_pure(pgs)) {
	    code = (*xf->common.procs->render_char) (xf, xg,
					imaging_dev, cx, cy,
					pdevc->colors.pure, 0);
	    if_debug8('K', "[K]render_char display: xfont=0x%lx, glyph=0x%lx\n\tdev=0x%lx(%s) x,y=%d,%d, color=0x%lx => %d\n",
		      (ulong) xf, (ulong) xg, (ulong) imaging_dev,
		      imaging_dev->dname, cx, cy,
		      (ulong) pdevc->colors.pure, code);
	    if (code == 0)
		return_check_interrupt(penum->memory, 0);
	}
	/* Can't render directly.  If we don't have a bitmap yet, */
	/* get it from the xfont now. */
	if (!cc_has_bits(cc)) {
	    gx_device_memory mdev;

	    gs_make_mem_mono_device(&mdev, dev->memory, imaging_dev);
	    gx_open_cache_device(&mdev, cc);
	    code = (*xf->common.procs->render_char) (xf, xg,
				       (gx_device *) & mdev, cx - x, cy - y,
						     (gx_color_index) 1, 1);
	    if_debug7('K', "[K]render_char to bits: xfont=0x%lx, glyph=0x%lx\n\tdev=0x%lx(%s) x,y=%d,%d => %d\n",
		      (ulong) xf, (ulong) xg, (ulong) & mdev,
		      mdev.dname, cx - x, cy - y, code);
	    if (code != 0)
		return_check_interrupt(penum->memory, 1);
	    gx_add_char_bits(cc_pair(cc)->font->dir,
			     cc, &scale_log2_1);
	    /* gx_add_char_bits may change width, height, */
	    /* raster, and/or offset.  It's easiest to */
	    /* start over from the top.  Clear xg so that */
	    /* we don't waste time trying render_char again. */
	    xg = gx_no_xglyph;
	    goto top;
	}
    }
    /*
     * No xfont.  Render from the cached bits.  If the cached bits
     * have more than 1 bit of alpha, and the color isn't pure or
     * the copy_alpha operation fails, construct a single-bit mask
     * by taking the high-order alpha bit.
     */
    bits = cc_bits(cc);
    /* With 4x2 scale, depth == 3. 
     * An example is -dTextAlphaBits=4 comparefiles/fonttest.pdf .
     * We need to map 4 bitmap bits to 2 alpha bits.
     */
    depth = (cc_depth(cc) == 3 ? 2 : cc_depth(cc));
    if (dev_proc(orig_dev, fill_mask) != gx_default_fill_mask ||
	!lop_no_S_is_T(pgs->log_op)
	) {
	gx_clip_path *pcpath;

	code = gx_effective_clip_path(pgs, &pcpath);
	if (code >= 0) {
	    code = (*dev_proc(orig_dev, fill_mask))
		(orig_dev, bits, 0, cc_raster(cc), cc->id,
		 x, y, w, h, pdevc, depth, pgs->log_op, pcpath);
	    if (code >= 0)
		goto done;
	}
    } else if (gs_color_writes_pure(pgs)) {
	gx_color_index color = pdevc->colors.pure;

	if (depth > 1) {
	    code = (*dev_proc(imaging_dev, copy_alpha))
		(imaging_dev, bits, 0, cc_raster(cc), cc->id,
		 x, y, w, h, color, depth);
	    if (code >= 0)
		return_check_interrupt(penum->memory, 0);
	    /* copy_alpha failed, construct a monobit mask. */
	    bits = compress_alpha_bits(cc, penum->memory->non_gc_memory);
	    if (bits == 0)
		return 1;	/* VMerror, but recoverable */
	}
	code = (*dev_proc(imaging_dev, copy_mono))
	    (imaging_dev, bits, 0, bitmap_raster(w), gs_no_id,
	     x, y, w, h, gx_no_color_index, color);
	goto done;
    }
    if (depth > 1) {		/* Complex color or fill_mask / copy_alpha failed, */
	/* construct a monobit mask. */
	bits = compress_alpha_bits(cc, penum->memory->non_gc_memory);
	if (bits == 0)
	    return 1;		/* VMerror, but recoverable */

    } {				/* Use imagemask to render the character. */
	gs_memory_t *mem = penum->memory->non_gc_memory;
	gs_image_enum *pie =
	    gs_image_enum_alloc(mem, "image_char(image_enum)");
	gs_image_t image;
	int iy;
	uint used, raster = (bits == cc_bits(cc) ? cc_raster(cc)
			     : bitmap_raster(cc->width) );
	int code1;

	if (pie == 0) {
	    if (bits != cc_bits(cc))
		gs_free_object(mem, bits,
			       "compress_alpha_bits");
	    return 1;		/* VMerror, but recoverable */
	}
	/* Make a matrix that will place the image */
	/* at (x,y) with no transformation. */
	gs_image_t_init_mask(&image, true);
#define mat image.ImageMatrix
	gs_make_translation((floatp) - x, (floatp) - y, &mat);
	gs_matrix_multiply(&ctm_only(pgs), &mat, &mat);
#undef mat
	image.Width = w;
	image.Height = h;
	image.adjust = false;
	code = gs_image_init(pie, &image, false, pgs);
	switch (code) {
	    case 1:		/* empty image */
		code = 0;
	    default:
		break;
	    case 0:
		for (iy = 0; iy < h && code >= 0; iy++)
		    code = gs_image_next(pie, bits + iy * raster,
					 (w + 7) >> 3, &used);
	}
	code1 = gs_image_cleanup_and_free_enum(pie);
	if (code >= 0 && code1 < 0)
	    code = code1;
    }
  done:if (bits != cc_bits(cc))
	gs_free_object(penum->memory->non_gc_memory, bits, "compress_alpha_bits");
    if (code > 0)
	code = 0;
    return_check_interrupt(penum->memory, code);
}

/* ------ Image manipulation ------ */

/*
 * Compress a mask with 2 or 4 bits of alpha to a monobit mask.
 * Allocate and return the address of the monobit mask.
 */
private byte *
compress_alpha_bits(const cached_char * cc, gs_memory_t * mem)
{
    const byte *data = cc_const_bits(cc);
    uint width = cc->width;
    uint height = cc->height;
    /* With 4x2 scale, depth == 3. 
     * An example is -dTextAlphaBits=4 comparefiles/fonttest.pdf .
     * We need to map 4 bitmap bits to 2 alpha bits.
     */
    int depth = (cc_depth(cc) == 3 ? 2 : cc_depth(cc));
    uint sraster = cc_raster(cc);
    uint sskip = sraster - ((width * depth + 7) >> 3);
    uint draster = bitmap_raster(width);
    uint dskip = draster - ((width + 7) >> 3);
    byte *mask = gs_alloc_bytes(mem, draster * height,
				"compress_alpha_bits");
    const byte *sptr = data;
    byte *dptr = mask;
    uint h;

    if (mask == 0)
	return 0;
    for (h = height; h; --h) {
	byte sbit = 0x80;
	byte d = 0;
	byte dbit = 0x80;
	uint w;

	for (w = width; w; --w) {
	    if (*sptr & sbit)
		d += dbit;
	    if (!(sbit >>= depth))
		sbit = 0x80, sptr++;
	    if (!(dbit >>= 1)) {
		*dptr++ = d;
		dbit = 0x80, d = 0;
	    }
	}
	if (dbit != 0x80)
	    *dptr++ = d;
	for (w = dskip; w != 0; --w)
	    *dptr++ = 0;
	if (sbit != 0x80)
	    ++sptr;
	sptr += sskip;
    }
    return mask;
}

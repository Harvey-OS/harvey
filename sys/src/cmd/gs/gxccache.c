/* Copyright (C) 1989, 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
  This file is part of Aladdin Ghostscript.
  
  Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
  License (the "License") for full details.
  
  Every copy of Aladdin Ghostscript must include a copy of the License,
  normally in a plain ASCII text file named PUBLIC.  The License grants you
  the right to copy, modify and redistribute Aladdin Ghostscript, but only
  under certain conditions described in the License.  Among other things, the
  License requires that the copyright notice and this notice be preserved on
  all copies.
*/

/* gxccache.c */
/* Fast case character cache routines for Ghostscript library */
#include "gx.h"
#include "gpcheck.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gzstate.h"
#include "gzpath.h"
#include "gzcpath.h"
#include "gxdevmem.h"
#include "gxchar.h"
#include "gxfont.h"
#include "gxfcache.h"
#include "gxxfont.h"
#include "gscspace.h"			/* for gsimage.h */
#include "gsimage.h"

/* Forward references */
private byte *compress_alpha_bits(P2(const cached_char *, gs_memory_t *));

/* Define a scale factor of 1. */
static const gs_log2_scale_point scale_log2_1 = { 0, 0 };

/* Look up, and if necessary add, a font/matrix pair in the cache */
cached_fm_pair *
gx_lookup_fm_pair(gs_font *pfont, register const gs_state *pgs)
{	float	mxx = pgs->char_tm.xx, mxy = pgs->char_tm.xy,
		myx = pgs->char_tm.yx, myy = pgs->char_tm.yy;
	gs_font *font = pfont;
	register gs_font_dir *dir = font->dir;
	register cached_fm_pair *pair =
		dir->fmcache.mdata + dir->fmcache.mnext;
	int count = dir->fmcache.mmax;
	gs_uid uid;
	if ( font->FontType == ft_composite )
	{	uid_set_invalid(&uid);
	}
	else
	{	uid = ((gs_font_base *)font)->UID;
		if ( uid_is_valid(&uid) )
			font = 0;
	}
	while ( count-- )
	{	if ( pair == dir->fmcache.mdata )
			pair += dir->fmcache.mmax;
		pair--;
		/* We have either a non-zero font and an invalid UID, */
		/* or a zero font and a valid UID. */
		/* We have to break up the test */
		/* because of a bug in the Zortech compiler. */
		if ( font != 0 )
		{	if ( pair->font != font ) continue;
		}
		else
		{	if ( !uid_equal(&pair->UID, &uid) ) continue;
		}
		if (	pair->mxx == mxx && pair->mxy == mxy &&
			pair->myx == myx && pair->myy == myy
		   )
		{	if ( pair->font == 0 )
			{	pair->font = pfont;
				if_debug2('k', "[k]updating pair 0x%lx with font 0x%lx\n",
					  (ulong)pair, (ulong)pfont);
			}
			else
			{	if_debug2('k', "[k]found pair 0x%lx: font=0x%lx\n",
					  (ulong)pair, (ulong)pair->font);
			}
			return pair;
		}
	}
	return gx_add_fm_pair(dir, pfont, &uid, pgs);
}

/* Look up a glyph in the cache. */
/* The character depth must be either 1 or alt_depth. */
/* Return the cached_char or 0. */
cached_char *
gx_lookup_cached_char(const gs_font *pfont, const cached_fm_pair *pair,
  gs_glyph glyph, int wmode, int alt_depth)
{	gs_font_dir *dir = pfont->dir;
	register cached_char *cc = *chars_head(dir, glyph, pair);
	while ( cc != 0 )
	{	if ( cc->code == glyph && cc->head.pair == pair &&
		     cc->wmode == wmode && (cc->depth == 1 || cc->depth == alt_depth)
		   )
		{	if_debug4('K', "[K]found 0x%lx (depth=%d) for glyph=0x%lx, wmode=%d\n",
				  (ulong)cc, cc->depth, (ulong)glyph, wmode);
			return cc;
		}
		cc = cc->next;
	}
	if_debug3('K', "[K]not found: glyph=0x%lx, wmode=%d, alt_depth=%d\n",
		  (ulong)glyph, wmode, alt_depth);
	return 0;
}

/* Look up a character in an external font. */
/* Return the cached_char or 0. */
cached_char *
gx_lookup_xfont_char(const gs_state *pgs, cached_fm_pair *pair,
  gs_char chr, gs_glyph glyph, gs_proc_glyph_name_t name_proc, int wmode)
{	gs_font *font = pair->font;
	int enc_index;
	gx_xfont *xf;
	gx_xglyph xg;
	gs_log2_scale_point log2_scale;
	gs_point wxy;
	gs_int_rect bbox;
	cached_char *cc;

	if ( font == 0 )
		return NULL;
	enc_index =
		(font->FontType == ft_composite ? -1 :
		 ((gs_font_base *)font)->nearest_encoding_index);
	if ( !pair->xfont_tried )
	{	/* Look for an xfont now. */
		gx_lookup_xfont(pgs, pair, enc_index);
		pair->xfont_tried = true;
	}
	xf = pair->xfont;
	if ( xf == 0 )
		return NULL;
	/***** The following is wrong.  We should only use the nearest *****/
	/***** registered encoding if the character is really the same. *****/
	xg = (*xf->common.procs->char_xglyph)(xf, chr, enc_index, glyph, name_proc);
	if ( xg == gx_no_xglyph )
		return NULL;
	if ( (*xf->common.procs->char_metrics)(xf, xg, wmode, &wxy, &bbox) < 0 )
		return NULL;
	log2_scale.x = log2_scale.y = 1;
	cc = gx_alloc_char_bits(font->dir, NULL, bbox.q.x - bbox.p.x,
				bbox.q.y - bbox.p.y, &log2_scale);
	if ( cc == 0 )
		return NULL;
	/* Success.  Make the cache entry. */
	cc->code = glyph;
	cc->wmode = wmode;
	cc->depth = 1;
	cc->xglyph = xg;
	cc->wxy.x = float2fixed(wxy.x);
	cc->wxy.y = float2fixed(wxy.y);
	cc->offset.x = int2fixed(-bbox.p.x);
	cc->offset.y = int2fixed(-bbox.p.y);
	if_debug5('k', "[k]xfont %s char %d/0x%x#0x%lx=>0x%lx\n",
		  font->font_name.chars, enc_index, (int)chr,
		  (ulong)glyph, (ulong)xg);
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
gx_image_cached_char(register gs_show_enum *penum, register cached_char *cc)
{	register gs_state *pgs = penum->pgs;
	int x, y, w, h;
	int code;
	gs_fixed_point pt;
	gx_device *dev = gs_currentdevice_inline(pgs);
	gx_device_clip cdev;
	gx_xglyph xg = cc->xglyph;
	gx_xfont *xf;
	byte *bits;

top:	code = gx_path_current_point_inline(pgs->path, &pt);
	if ( code < 0 ) return code;
	/* If the character doesn't lie entirely within the */
	/* quick-check clipping rectangle, we have to */
	/* set up an intermediate clipping device. */
	pt.x -= cc->offset.x;
	x = fixed2int_var_rounded(pt.x) + penum->ftx;
	pt.y -= cc->offset.y;
	y = fixed2int_var_rounded(pt.y) + penum->fty;
	w = cc->width;
	h = cc->height;
#ifdef DEBUG
	if ( gs_debug_c('K') )
	{	if ( cc_has_bits(cc) )
			debug_dump_bitmap(cc_bits(cc), cc->raster, h,
					  "[K]bits");
		else
			dputs("[K]no bits\n");
		dprintf3("[K]copying 0x%lx, offset=(%g,%g)\n", (ulong)cc,
			 fixed2float(-cc->offset.x),
			 fixed2float(-cc->offset.y));
		dprintf6("   at (%g,%g)+(%d,%d)->(%d,%d)\n",
			 fixed2float(pt.x), fixed2float(pt.y),
			 penum->ftx, penum->fty, x, y);
	}
#endif
	if (	(x < penum->ibox.p.x || x + w > penum->ibox.q.x ||
		 y < penum->ibox.p.y || y + h > penum->ibox.q.y) &&
		dev != (gx_device *)&cdev	/* might be 2nd time around */
	   )
	{	/* Check for the character falling entirely outside */
		/* the clipping region. */
		if ( x >= penum->obox.q.x || x + w <= penum->obox.p.x ||
		     y >= penum->obox.q.y || y + h <= penum->obox.p.y
		   )
			return 0;		/* nothing to do */
		gx_make_clip_device(&cdev, &cdev, &pgs->clip_path->list);
		cdev.target = dev;
		dev = (gx_device *)&cdev;
		(*dev_proc(dev, open_device))(dev);
		if_debug0('K', "[K](clipping)\n");
	}
	/* If an xfont can render this character, use it. */
	if ( xg != gx_no_xglyph && (xf = cc->head.pair->xfont) != 0 )
	{	int cx = x + fixed2int(cc->offset.x);
		int cy = y + fixed2int(cc->offset.y);
		/*
		 * Note that we prefer a 1-bit xfont implementation over
		 * a multi-bit cached bitmap.  Eventually we should change
		 * the xfont interface so it can deliver multi-bit bitmaps,
		 * or else implement oversampling for xfonts.
		 */
		if ( color_is_pure(pgs->dev_color) )
		{	code = (*xf->common.procs->render_char)(xf, xg,
				dev, cx, cy,
				pgs->dev_color->colors.pure, 0);
			if_debug8('K', "[K]render_char display: xfont=0x%lx, glyph=0x%lx\n\tdev=0x%lx(%s) x,y=%d,%d, color=0x%lx => %d\n",
				  (ulong)xf, (ulong)xg, (ulong)dev,
				  dev->dname, cx, cy,
				  (ulong)pgs->dev_color->colors.pure, code);
			if ( code == 0 )
				return_check_interrupt(0);
		}
		/* Can't render directly.  If we don't have a bitmap yet, */
		/* get it from the xfont now. */
		if ( !cc_has_bits(cc) )
		{	gx_device_memory mdev;
			gs_make_mem_mono_device(&mdev, 0, dev);
			gx_open_cache_device(&mdev, cc);
			code = (*xf->common.procs->render_char)(xf, xg,
				(gx_device *)&mdev, cx - x, cy - y,
				(gx_color_index)1, 1);
			if_debug7('K', "[K]render_char to bits: xfont=0x%lx, glyph=0x%lx\n\tdev=0x%lx(%s) x,y=%d,%d => %d\n",
				  (ulong)xf, (ulong)xg, (ulong)&mdev,
				  mdev.dname, cx - x, cy - y, code);
			if ( code != 0 )
				return_check_interrupt(1);
			gx_add_char_bits(cc->head.pair->font->dir,
					 &mdev, cc, &scale_log2_1);
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
	if ( color_is_pure(pgs->dev_color) )
	{	byte *bits = cc_bits(cc);
		gx_color_index color = pgs->dev_color->colors.pure;
		if ( cc->depth > 1 )
		  {	code = (*dev_proc(dev, copy_alpha))
					(dev, bits, 0, cc->raster, cc->id,
					 x, y, w, h, color, cc->depth);
			if ( code >= 0 )
			  return_check_interrupt(0);
			/* copy_alpha failed, construct a monobit mask. */
			bits = compress_alpha_bits(cc, &gs_memory_default);
			if ( bits == 0 )
			  return 1;	/* VMerror, but recoverable */
		  }
		code = (*dev_proc(dev, copy_mono))
				(dev, bits, 0, cc->raster, cc->id,
				 x, y, w, h, gx_no_color_index, color);
	}
	else
	{	/* Use imagemask to render the character. */
		gs_image_enum *pie = gs_image_enum_alloc(&gs_memory_default,
						"image_char(image_enum)");
		gs_matrix mat;
		int iy;
		uint used;

		if ( pie == 0 )
		  return 1;	/* VMerror, but recoverable */
		if ( cc->depth > 1 )
		  {	/* Construct a monobit mask. */
			bits = compress_alpha_bits(cc, &gs_memory_default);
			if ( bits == 0 )
			  {	gs_free_object(&gs_memory_default, pie,
					       "image_char(image_enum)");
				return 1;	/* VMerror, but recoverable */
			  }
		  }
		/* Make a matrix that will place the image */
		/* at (x,y) with no transformation. */
		gs_make_translation((floatp)-x, (floatp)-y, &mat);
		gs_matrix_multiply(&ctm_only(pgs), &mat, &mat);
		code = gs_imagemask_init(pie, pgs, w, h, false, false,
					 &mat, false);
		switch ( code )
		{
		case 1:			/* empty image */
			code = 0;
		default:
			break;
		case 0:
			for ( iy = 0; iy < h && code >= 0; iy++ )
			  code = gs_image_next(pie, bits + iy * cc->raster,
					       (w + 7) >> 3, &used);
			gs_image_cleanup(pie);
		}
		gs_free_object(&gs_memory_default, pie,
			       "image_char(image_enum)");
	}
	if ( bits != cc_bits(cc) )
	  gs_free_object(&gs_memory_default, bits, "compress_alpha_bits");
	if ( code > 0 )
	  code = 0;
	return_check_interrupt(code);
}

/* ------ Image manipulation ------ */

/*
 * Compress a mask with 2 or 4 bits of alpha to a monobit mask.
 * Allocate and return the address of the monobit mask.
 */
private byte *
compress_alpha_bits(const cached_char *cc, gs_memory_t *mem)
{	const byte *data = cc_bits(cc);
	uint width = cc->width;
	uint height = cc->height;
	int log2_scale = cc->depth;
	int scale = 1 << log2_scale;
	uint sraster = cc->raster;
	uint sskip = sraster - ((width * scale + 7) >> 3);
	uint draster = bitmap_raster(width);
	uint dskip = draster - ((width + 7) >> 3);
	byte *mask = gs_alloc_bytes(mem, draster * height,
				    "compress_alpha_bits");
	const byte *sptr = data;
	byte *dptr = mask;
	uint h;

	if ( mask == 0 )
	  return 0;
	for ( h = height; h; --h )
	{	byte sbit = 0x80;
		byte d = 0;
		byte dbit = 0x80;
		uint w;

		for ( w = width; w; --w )
		  {	if ( *sptr & sbit )
			  d += dbit;
			if ( !(sbit >>= log2_scale) )
			  sbit = 0x80, sptr++;
			if ( !(dbit >>= 1) )
			  dbit = 0x80, dptr++, d = 0;
		  }
		if ( dbit != 0x80 )
		  *dptr++ = d;
		for ( w = dskip; w != 0; --w )
		  *dptr++ = 0;
		sptr += sskip;
	}
	return mask;
}

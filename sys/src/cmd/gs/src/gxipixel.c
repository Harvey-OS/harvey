/* Copyright (C) 1989, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: gxipixel.c,v 1.2 2000/09/19 19:00:38 lpd Exp $ */
/* Common code for ImageType 1 and 4 initialization */
#include "gx.h"
#include "math_.h"
#include "memory_.h"
#include "gpcheck.h"
#include "gscdefs.h"		/* for image class table */
#include "gserrors.h"
#include "gsstruct.h"
#include "gsutil.h"
#include "gxfixed.h"
#include "gxfrac.h"
#include "gxarith.h"
#include "gxmatrix.h"
#include "gsccolor.h"
#include "gspaint.h"
#include "gzstate.h"
#include "gxdevice.h"
#include "gzpath.h"
#include "gzcpath.h"
#include "gxdevmem.h"
#include "gximage.h"
#include "gxiparam.h"
#include "gdevmrop.h"

/* Structure descriptors */
private_st_gx_image_enum();

/* Image class procedures */
extern_gx_image_class_table();

/* Enumerator procedures */
private const gx_image_enum_procs_t image1_enum_procs = {
    gx_image1_plane_data, gx_image1_end_image, gx_image1_flush
};

/* GC procedures */
private 
ENUM_PTRS_WITH(image_enum_enum_ptrs, gx_image_enum *eptr)
{
    int bps;
    gs_ptr_type_t ret;

    /* Enumerate the used members of clues.dev_color. */
    index -= gx_image_enum_num_ptrs;
    bps = eptr->unpack_bps;
    if (eptr->spp != 1)
	bps = 8;
    else if (bps > 8 || eptr->unpack == sample_unpack_copy)
	bps = 1;
    if (index >= (1 << bps) * st_device_color_max_ptrs)		/* done */
	return 0;
    ret = ENUM_USING(st_device_color,
		     &eptr->clues[(index / st_device_color_max_ptrs) *
				  (255 / ((1 << bps) - 1))].dev_color,
		     sizeof(eptr->clues[0].dev_color),
		     index % st_device_color_max_ptrs);
    if (ret == 0)		/* don't stop early */
	ENUM_RETURN(0);
    return ret;
}
#define e1(i,elt) ENUM_PTR(i,gx_image_enum,elt);
gx_image_enum_do_ptrs(e1)
#undef e1
ENUM_PTRS_END
private RELOC_PTRS_WITH(image_enum_reloc_ptrs, gx_image_enum *eptr)
{
    int i;

#define r1(i,elt) RELOC_PTR(gx_image_enum,elt);
    gx_image_enum_do_ptrs(r1)
#undef r1
    {
	int bps = eptr->unpack_bps;

	if (eptr->spp != 1)
	    bps = 8;
	else if (bps > 8 || eptr->unpack == sample_unpack_copy)
	    bps = 1;
	for (i = 0; i <= 255; i += 255 / ((1 << bps) - 1))
	    RELOC_USING(st_device_color,
			&eptr->clues[i].dev_color, sizeof(gx_device_color));
    }
}
RELOC_PTRS_END

/* Forward declarations */
private int color_draws_b_w(P2(gx_device * dev,
			       const gx_drawing_color * pdcolor));
private void image_init_map(P3(byte * map, int map_size, const float *decode));
private void image_init_colors(P9(gx_image_enum * penum, int bps, int spp,
				  gs_image_format_t format,
				  const float *decode,
				  const gs_imager_state * pis, gx_device * dev,
				  const gs_color_space * pcs, bool * pdcb));

/* Procedures for unpacking the input data into bytes or fracs. */
/*extern SAMPLE_UNPACK_PROC(sample_unpack_copy); *//* declared above */

/*
 * Do common initialization for processing an ImageType 1 or 4 image.
 * Allocate the enumerator and fill in the following members:
 *	rect
 */
int
gx_image_enum_alloc(const gs_image_common_t * pic,
		    const gs_int_rect * prect, gs_memory_t * mem,
		    gx_image_enum **ppenum)
{
    const gs_pixel_image_t *pim = (const gs_pixel_image_t *)pic;
    int width = pim->Width, height = pim->Height;
    int bpc = pim->BitsPerComponent;
    gx_image_enum *penum;

    if (width < 0 || height < 0)
	return_error(gs_error_rangecheck);
    switch (pim->format) {
    case gs_image_format_chunky:
    case gs_image_format_component_planar:
	switch (bpc) {
	case 1: case 2: case 4: case 8: case 12: break;
	default: return_error(gs_error_rangecheck);
	}
	break;
    case gs_image_format_bit_planar:
	if (bpc < 1 || bpc > 8)
	    return_error(gs_error_rangecheck);
    }
    if (prect) {
	if (prect->p.x < 0 || prect->p.y < 0 ||
	    prect->q.x < prect->p.x || prect->q.y < prect->p.y ||
	    prect->q.x > width || prect->q.y > height
	    )
	    return_error(gs_error_rangecheck);
    }
    penum = gs_alloc_struct(mem, gx_image_enum, &st_gx_image_enum,
			    "gx_default_begin_image");
    if (penum == 0)
	return_error(gs_error_VMerror);
    if (prect) {
	penum->rect.x = prect->p.x;
	penum->rect.y = prect->p.y;
	penum->rect.w = prect->q.x - prect->p.x;
	penum->rect.h = prect->q.y - prect->p.y;
    } else {
	penum->rect.x = 0, penum->rect.y = 0;
	penum->rect.w = width, penum->rect.h = height;
    }
#ifdef DEBUG
    if (gs_debug_c('b')) {
	dlprintf2("[b]Image: w=%d h=%d", width, height);
	if (prect)
	    dprintf4(" ((%d,%d),(%d,%d))",
		     prect->p.x, prect->p.y, prect->q.x, prect->q.y);
    }
#endif
    *ppenum = penum;
    return 0;
}

/*
 * Finish initialization for processing an ImageType 1 or 4 image.
 * Assumes the following members of *penum are set in addition to those
 * set by gx_image_enum_alloc:
 *	alpha, use_mask_color, mask_color (if use_mask_color is true),
 *	masked, adjust
 */
int
gx_image_enum_begin(gx_device * dev, const gs_imager_state * pis,
		    const gs_matrix *pmat, const gs_image_common_t * pic,
		const gx_drawing_color * pdcolor, const gx_clip_path * pcpath,
		gs_memory_t * mem, gx_image_enum *penum)
{
    const gs_pixel_image_t *pim = (const gs_pixel_image_t *)pic;
    gs_image_format_t format = pim->format;
    const int width = pim->Width;
    const int height = pim->Height;
    const int bps = pim->BitsPerComponent;
    bool masked = penum->masked;
    const float *decode = pim->Decode;
    gs_matrix mat;
    int index_bps;
    const gs_color_space *pcs = pim->ColorSpace;
    gs_logical_operation_t lop = (pis ? pis->log_op : lop_default);
    int code;
    int log2_xbytes = (bps <= 8 ? 0 : arch_log2_sizeof_frac);
    int spp, nplanes, spread;
    uint bsize;
    byte *buffer;
    fixed mtx, mty;
    gs_fixed_point row_extent, col_extent, x_extent, y_extent;
    bool device_color;
    gs_fixed_rect obox, cbox;

    if (pmat == 0)
	pmat = &ctm_only(pis);
    if ((code = gs_matrix_invert(&pim->ImageMatrix, &mat)) < 0 ||
	(code = gs_matrix_multiply(&mat, pmat, &mat)) < 0
	) {
	gs_free_object(mem, penum, "gx_default_begin_image");
	return code;
    }
    penum->matrix = mat;
    if_debug6('b', " [%g %g %g %g %g %g]\n",
	      mat.xx, mat.xy, mat.yx, mat.yy, mat.tx, mat.ty);
    /* following works for 1, 2, 4, 8, 12 */
    index_bps = (bps < 8 ? bps >> 1 : (bps >> 2) + 1);
    mtx = float2fixed(mat.tx);
    mty = float2fixed(mat.ty);
    row_extent.x = float2fixed(width * mat.xx + mat.tx) - mtx;
    row_extent.y =
	(is_fzero(mat.xy) ? fixed_0 :
	 float2fixed(width * mat.xy + mat.ty) - mty);
    col_extent.x =
	(is_fzero(mat.yx) ? fixed_0 :
	 float2fixed(height * mat.yx + mat.tx) - mtx);
    col_extent.y = float2fixed(height * mat.yy + mat.ty) - mty;
    gx_image_enum_common_init((gx_image_enum_common_t *)penum,
			      (const gs_data_image_t *)pim,
			      &image1_enum_procs, dev,
			      (masked ? 1 : cs_num_components(pcs)),
			      format);
    if (penum->rect.w == width && penum->rect.h == height) {
	x_extent = row_extent;
	y_extent = col_extent;
    } else {
	int rw = penum->rect.w, rh = penum->rect.h;

	x_extent.x = float2fixed(rw * mat.xx + mat.tx) - mtx;
	x_extent.y =
	    (is_fzero(mat.xy) ? fixed_0 :
	     float2fixed(rw * mat.xy + mat.ty) - mty);
	y_extent.x =
	    (is_fzero(mat.yx) ? fixed_0 :
	     float2fixed(rh * mat.yx + mat.tx) - mtx);
	y_extent.y = float2fixed(rh * mat.yy + mat.ty) - mty;
    }
    if (masked) {	/* This is imagemask. */
	if (bps != 1 || pcs != NULL || penum->alpha ||
	    !((decode[0] == 0.0 && decode[1] == 1.0) ||
	      (decode[0] == 1.0 && decode[1] == 0.0))
	    ) {
	    gs_free_object(mem, penum, "gx_default_begin_image");
	    return_error(gs_error_rangecheck);
	}
	/* Initialize color entries 0 and 255. */
	color_set_pure(&penum->icolor0, gx_no_color_index);
	penum->icolor1 = *pdcolor;
	memcpy(&penum->map[0].table.lookup4x1to32[0],
	       (decode[0] == 0 ? lookup4x1to32_inverted :
		lookup4x1to32_identity),
	       16 * 4);
	penum->map[0].decoding = sd_none;
	spp = 1;
	lop = rop3_know_S_0(lop);
    } else {			/* This is image, not imagemask. */
	const gs_color_space_type *pcst = pcs->type;
	int b_w_color;

	spp = cs_num_components(pcs);
	if (spp < 0) {		/* Pattern not allowed */
	    gs_free_object(mem, penum, "gx_default_begin_image");
	    return_error(gs_error_rangecheck);
	}
	if (penum->alpha)
	    ++spp;
	/* Use a less expensive format if possible. */
	switch (format) {
	case gs_image_format_bit_planar:
	    if (bps > 1)
		break;
	    format = gs_image_format_component_planar;
	case gs_image_format_component_planar:
	    if (spp == 1)
		format = gs_image_format_chunky;
	default:		/* chunky */
	    break;
	}
	device_color = (*pcst->concrete_space) (pcs, pis) == pcs;
	image_init_colors(penum, bps, spp, format, decode, pis, dev,
			  pcs, &device_color);
	/* Try to transform non-default RasterOps to something */
	/* that we implement less expensively. */
	if (!pim->CombineWithColor)
	    lop = rop3_know_T_0(lop) & ~lop_T_transparent;
	else {
	    if (rop3_uses_T(lop))
		switch (color_draws_b_w(dev, pdcolor)) {
		    case 0:
			lop = rop3_know_T_0(lop);
			break;
		    case 1:
			lop = rop3_know_T_1(lop);
			break;
		    default:
			;
		}
	}
	if (lop != rop3_S &&	/* if best case, no more work needed */
	    !rop3_uses_T(lop) && bps == 1 && spp == 1 &&
	    (b_w_color =
	     color_draws_b_w(dev, &penum->icolor0)) >= 0 &&
	    color_draws_b_w(dev, &penum->icolor1) == (b_w_color ^ 1)
	    ) {
	    if (b_w_color) {	/* Swap the colors and invert the RasterOp source. */
		gx_device_color dcolor;

		dcolor = penum->icolor0;
		penum->icolor0 = penum->icolor1;
		penum->icolor1 = dcolor;
		lop = rop3_invert_S(lop);
	    }
	    /*
	     * At this point, we know that the source pixels
	     * correspond directly to the S input for the raster op,
	     * i.e., icolor0 is black and icolor1 is white.
	     */
	    switch (lop) {
		case rop3_D & rop3_S:
		    /* Implement this as an inverted mask writing 0s. */
		    penum->icolor1 = penum->icolor0;
		    /* (falls through) */
		case rop3_D | rop3_not(rop3_S):
		    /* Implement this as an inverted mask writing 1s. */
		    memcpy(&penum->map[0].table.lookup4x1to32[0],
			   lookup4x1to32_inverted, 16 * 4);
		  rmask:	/* Fill in the remaining parameters for a mask. */
		    penum->masked = masked = true;
		    color_set_pure(&penum->icolor0, gx_no_color_index);
		    penum->map[0].decoding = sd_none;
		    lop = rop3_T;
		    break;
		case rop3_D & rop3_not(rop3_S):
		    /* Implement this as a mask writing 0s. */
		    penum->icolor1 = penum->icolor0;
		    /* (falls through) */
		case rop3_D | rop3_S:
		    /* Implement this as a mask writing 1s. */
		    memcpy(&penum->map[0].table.lookup4x1to32[0],
			   lookup4x1to32_identity, 16 * 4);
		    goto rmask;
		default:
		    ;
	    }
	}
    }
    penum->device_color = device_color;
    /*
     * Adjust width upward for unpacking up to 7 trailing bits in
     * the row, plus 1 byte for end-of-run, plus up to 7 leading
     * bits for data_x offset within a packed byte.
     */
    bsize = ((bps > 8 ? width * 2 : width) + 15) * spp;
    buffer = gs_alloc_bytes(mem, bsize, "image buffer");
    if (buffer == 0) {
	gs_free_object(mem, penum, "gx_default_begin_image");
	return_error(gs_error_VMerror);
    }
    penum->bps = bps;
    penum->unpack_bps = bps;
    penum->log2_xbytes = log2_xbytes;
    penum->spp = spp;
    switch (format) {
    case gs_image_format_chunky:
	nplanes = 1;
	spread = 1 << log2_xbytes;
	break;
    case gs_image_format_component_planar:
	nplanes = spp;
	spread = spp << log2_xbytes;
	break;
    case gs_image_format_bit_planar:
	nplanes = spp * bps;
	spread = spp << log2_xbytes;
	break;
    default:
	/* No other cases are possible (checked by gx_image_enum_alloc). */
	return_error(gs_error_Fatal);
    }
    penum->num_planes = nplanes;
    penum->spread = spread;
    /*
     * If we're asked to interpolate in a partial image, we have to
     * assume that the client either really only is interested in
     * the given sub-image, or else is constructing output out of
     * overlapping pieces.
     */
    penum->interpolate = pim->Interpolate;
    penum->x_extent = x_extent;
    penum->y_extent = y_extent;
    penum->posture =
	((x_extent.y | y_extent.x) == 0 ? image_portrait :
	 (x_extent.x | y_extent.y) == 0 ? image_landscape :
	 image_skewed);
    penum->pis = pis;
    penum->pcs = pcs;
    penum->memory = mem;
    penum->buffer = buffer;
    penum->buffer_size = bsize;
    penum->line = 0;
    penum->line_size = 0;
    penum->use_rop = lop != (masked ? rop3_T : rop3_S);
#ifdef DEBUG
    if (gs_debug_c('*')) {
	if (penum->use_rop)
	    dprintf1("[%03x]", lop);
	dprintf5("%c%d%c%dx%d ",
		 (masked ? (color_is_pure(pdcolor) ? 'm' : 'h') : 'i'),
		 bps,
		 (penum->posture == image_portrait ? ' ' :
		  penum->posture == image_landscape ? 'L' : 'T'),
		 width, height);
    }
#endif
    penum->slow_loop = 0;
    if (pcpath == 0) {
	(*dev_proc(dev, get_clipping_box)) (dev, &obox);
	cbox = obox;
	penum->clip_image = 0;
    } else
	penum->clip_image =
	    (gx_cpath_outer_box(pcpath, &obox) |	/* not || */
	     gx_cpath_inner_box(pcpath, &cbox) ?
	     0 : image_clip_region);
    penum->clip_outer = obox;
    penum->clip_inner = cbox;
    penum->log_op = rop3_T;	/* rop device takes care of this */
    penum->clip_dev = 0;	/* in case we bail out */
    penum->rop_dev = 0;		/* ditto */
    penum->scaler = 0;		/* ditto */
    /*
     * If all four extrema of the image fall within the clipping
     * rectangle, clipping is never required.  When making this check,
     * we must carefully take into account the fact that we only care
     * about pixel centers.
     */
    {
	fixed
	    epx = min(row_extent.x, 0) + min(col_extent.x, 0),
	    eqx = max(row_extent.x, 0) + max(col_extent.x, 0),
	    epy = min(row_extent.y, 0) + min(col_extent.y, 0),
	    eqy = max(row_extent.y, 0) + max(col_extent.y, 0);

	{
	    int hwx, hwy;

	    switch (penum->posture) {
		case image_portrait:
		    hwx = width, hwy = height;
		    break;
		case image_landscape:
		    hwx = height, hwy = width;
		    break;
		default:
		    hwx = hwy = 0;
	    }
	    /*
	     * If the image is only 1 sample wide or high,
	     * and is less than 1 device pixel wide or high,
	     * move it slightly so that it covers pixel centers.
	     * This is a hack to work around a bug in some old
	     * versions of TeX/dvips, which use 1-bit-high images
	     * to draw horizontal and vertical lines without
	     * positioning them properly.
	     */
	    if (hwx == 1 && eqx - epx < fixed_1) {
		fixed diff =
		arith_rshift_1(row_extent.x + col_extent.x);

		mtx = (((mtx + diff) | fixed_half) & -fixed_half) - diff;
	    }
	    if (hwy == 1 && eqy - epy < fixed_1) {
		fixed diff =
		arith_rshift_1(row_extent.y + col_extent.y);

		mty = (((mty + diff) | fixed_half) & -fixed_half) - diff;
	    }
	}
	if_debug5('b', "[b]Image: %sspp=%d, bps=%d, mt=(%g,%g)\n",
		  (masked? "masked, " : ""), spp, bps,
		  fixed2float(mtx), fixed2float(mty));
	if_debug9('b',
		  "[b]   cbox=(%g,%g),(%g,%g), obox=(%g,%g),(%g,%g), clip_image=0x%x\n",
		  fixed2float(cbox.p.x), fixed2float(cbox.p.y),
		  fixed2float(cbox.q.x), fixed2float(cbox.q.y),
		  fixed2float(obox.p.x), fixed2float(obox.p.y),
		  fixed2float(obox.q.x), fixed2float(obox.q.y),
		  penum->clip_image);
	dda_init(penum->dda.row.x, mtx, col_extent.x, height);
	dda_init(penum->dda.row.y, mty, col_extent.y, height);
	if (penum->rect.y) {
	    dda_advance(penum->dda.row.x, penum->rect.y);
	    dda_advance(penum->dda.row.y, penum->rect.y);
	}
	penum->cur.x = penum->prev.x = dda_current(penum->dda.row.x);
	penum->cur.y = penum->prev.y = dda_current(penum->dda.row.y);
	dda_init(penum->dda.strip.x, penum->cur.x, row_extent.x, width);
	dda_init(penum->dda.strip.y, penum->cur.y, row_extent.y, width);
	if (penum->rect.x) {
	    dda_advance(penum->dda.strip.x, penum->rect.x);
	    dda_advance(penum->dda.strip.y, penum->rect.x);
	} {
	    fixed ox = dda_current(penum->dda.strip.x);
	    fixed oy = dda_current(penum->dda.strip.y);

	    if (!penum->clip_image)	/* i.e., not clip region */
		penum->clip_image =
		    (fixed_pixround(ox + epx) < fixed_pixround(cbox.p.x) ?
		     image_clip_xmin : 0) +
		    (fixed_pixround(ox + eqx) >= fixed_pixround(cbox.q.x) ?
		     image_clip_xmax : 0) +
		    (fixed_pixround(oy + epy) < fixed_pixround(cbox.p.y) ?
		     image_clip_ymin : 0) +
		    (fixed_pixround(oy + eqy) >= fixed_pixround(cbox.q.y) ?
		     image_clip_ymax : 0);
	}
    }
    penum->y = 0;
    penum->used.x = 0;
    penum->used.y = 0;
    {
	static const sample_unpack_proc_t procs[4] = {
	    sample_unpack_1, sample_unpack_2,
	    sample_unpack_4, sample_unpack_8
	};
	int i;

	if (index_bps == 4) {
	    if ((penum->unpack = sample_unpack_12_proc) == 0) {		/* 12-bit samples are not supported. */
		gx_default_end_image(dev,
				     (gx_image_enum_common_t *) penum,
				     false);
		return_error(gs_error_rangecheck);
	    }
	} else {
	    penum->unpack = procs[index_bps];
	    if_debug1('b', "[b]unpack=%d\n", bps);
	}
	/* Set up pixel0 for image class procedures. */
	penum->dda.pixel0 = penum->dda.strip;
	for (i = 0; i < gx_image_class_table_count; ++i)
	    if ((penum->render = gx_image_class_table[i](penum)) != 0)
		break;
	if (i == gx_image_class_table_count) {
	    /* No available class can handle this image. */
	    gx_default_end_image(dev, (gx_image_enum_common_t *) penum,
				 false);
	    return_error(gs_error_rangecheck);
	}
    }
    if (penum->clip_image && pcpath) {	/* Set up the clipping device. */
	gx_device_clip *cdev =
	    gs_alloc_struct(mem, gx_device_clip,
			    &st_device_clip, "image clipper");

	if (cdev == 0) {
	    gx_default_end_image(dev,
				 (gx_image_enum_common_t *) penum,
				 false);
	    return_error(gs_error_VMerror);
	}
	gx_make_clip_translate_device(cdev, gx_cpath_list(pcpath), 0, 0, mem);
	gx_device_retain((gx_device *)cdev, true); /* will free explicitly */
	gx_device_set_target((gx_device_forward *)cdev, dev);
	(*dev_proc(cdev, open_device)) ((gx_device *) cdev);
	penum->clip_dev = cdev;
    }
    if (penum->use_rop) {	/* Set up the RasterOp source device. */
	gx_device_rop_texture *rtdev;

	code = gx_alloc_rop_texture_device(&rtdev, mem,
					   "image RasterOp");
	if (code < 0) {
	    gx_default_end_image(dev, (gx_image_enum_common_t *) penum,
				 false);
	    return code;
	}
	gx_make_rop_texture_device(rtdev,
				   (penum->clip_dev != 0 ?
				    (gx_device *) penum->clip_dev :
				    dev), lop, pdcolor);
	penum->rop_dev = rtdev;
    }
    return 0;
}

/* If a drawing color is black or white, return 0 or 1 respectively, */
/* otherwise return -1. */
private int
color_draws_b_w(gx_device * dev, const gx_drawing_color * pdcolor)
{
    if (color_is_pure(pdcolor)) {
	gx_color_value rgb[3];

	(*dev_proc(dev, map_color_rgb)) (dev, gx_dc_pure_color(pdcolor),
					 rgb);
	if (!(rgb[0] | rgb[1] | rgb[2]))
	    return 0;
	if ((rgb[0] & rgb[1] & rgb[2]) == gx_max_color_value)
	    return 1;
    }
    return -1;
}

/* Initialize the color mapping tables for a non-mask image. */
private void
image_init_colors(gx_image_enum * penum, int bps, int spp,
		  gs_image_format_t format, const float *decode /*[spp*2] */ ,
		  const gs_imager_state * pis, gx_device * dev,
		  const gs_color_space * pcs, bool * pdcb)
{
    int ci;
    static const float default_decode[] = {
	0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0
    };

    /* Initialize the color table */

#define ictype(i)\
  penum->clues[i].dev_color.type
    switch ((spp == 1 ? bps : 8)) {
	case 8:		/* includes all color images */
	    {
		register gx_image_clue *pcht = &penum->clues[0];
		register int n = 64;

		do {
		    pcht[0].dev_color.type =
			pcht[1].dev_color.type =
			pcht[2].dev_color.type =
			pcht[3].dev_color.type =
			gx_dc_type_none;
		    pcht[0].key = pcht[1].key =
			pcht[2].key = pcht[3].key = 0;
		    pcht += 4;
		}
		while (--n > 0);
		penum->clues[0].key = 1;	/* guarantee no hit */
		break;
	    }
	case 4:
	    ictype(17) = ictype(2 * 17) = ictype(3 * 17) =
		ictype(4 * 17) = ictype(6 * 17) = ictype(7 * 17) =
		ictype(8 * 17) = ictype(9 * 17) = ictype(11 * 17) =
		ictype(12 * 17) = ictype(13 * 17) = ictype(14 * 17) =
		gx_dc_type_none;
	    /* falls through */
	case 2:
	    ictype(5 * 17) = ictype(10 * 17) = gx_dc_type_none;
#undef ictype
    }

    /* Initialize the maps from samples to intensities. */

    for (ci = 0; ci < spp; ci++) {
	sample_map *pmap = &penum->map[ci];

	/* If the decoding is [0 1] or [1 0], we can fold it */
	/* into the expansion of the sample values; */
	/* otherwise, we have to use the floating point method. */

	const float *this_decode = &decode[ci * 2];
	const float *map_decode;	/* decoding used to */
					/* construct the expansion map */
	const float *real_decode;	/* decoding for expanded samples */

	bool no_decode;

	map_decode = real_decode = this_decode;
	if (map_decode[0] == 0.0 && map_decode[1] == 1.0)
	    no_decode = true;
	else if (map_decode[0] == 1.0 && map_decode[1] == 0.0 && bps <= 8) {
	    no_decode = true;
	    real_decode = default_decode;
	} else {
	    no_decode = false;
	    *pdcb = false;
	    map_decode = default_decode;
	}
	if (bps > 2 || format != gs_image_format_chunky) {
	    if (bps <= 8)
		image_init_map(&pmap->table.lookup8[0], 1 << bps,
			       map_decode);
	} else {		/* The map index encompasses more than one pixel. */
	    byte map[4];
	    register int i;

	    image_init_map(&map[0], 1 << bps, map_decode);
	    switch (bps) {
		case 1:
		    {
			register bits32 *p = &pmap->table.lookup4x1to32[0];

			if (map[0] == 0 && map[1] == 0xff)
			    memcpy((byte *) p, lookup4x1to32_identity, 16 * 4);
			else if (map[0] == 0xff && map[1] == 0)
			    memcpy((byte *) p, lookup4x1to32_inverted, 16 * 4);
			else
			    for (i = 0; i < 16; i++, p++)
				((byte *) p)[0] = map[i >> 3],
				    ((byte *) p)[1] = map[(i >> 2) & 1],
				    ((byte *) p)[2] = map[(i >> 1) & 1],
				    ((byte *) p)[3] = map[i & 1];
		    }
		    break;
		case 2:
		    {
			register bits16 *p = &pmap->table.lookup2x2to16[0];

			for (i = 0; i < 16; i++, p++)
			    ((byte *) p)[0] = map[i >> 2],
				((byte *) p)[1] = map[i & 3];
		    }
		    break;
	    }
	}
	pmap->decode_base /* = decode_lookup[0] */  = real_decode[0];
	pmap->decode_factor =
	    (real_decode[1] - real_decode[0]) /
	    (bps <= 8 ? 255.0 : (float)frac_1);
	pmap->decode_max /* = decode_lookup[15] */  = real_decode[1];
	if (no_decode) {
	    pmap->decoding = sd_none;
	    pmap->inverted = map_decode[0] != 0;
	} else if (bps <= 4) {
	    int step = 15 / ((1 << bps) - 1);
	    int i;

	    pmap->decoding = sd_lookup;
	    for (i = 15 - step; i > 0; i -= step)
		pmap->decode_lookup[i] = pmap->decode_base +
		    i * (255.0 / 15) * pmap->decode_factor;
	} else
	    pmap->decoding = sd_compute;
	if (spp == 1) {		/* and ci == 0 *//* Pre-map entries 0 and 255. */
	    gs_client_color cc;

	    cc.paint.values[0] = real_decode[0];
	    (*pcs->type->remap_color) (&cc, pcs, &penum->icolor0,
				       pis, dev, gs_color_select_source);
	    cc.paint.values[0] = real_decode[1];
	    (*pcs->type->remap_color) (&cc, pcs, &penum->icolor1,
				       pis, dev, gs_color_select_source);
	}
    }

}
/* Construct a mapping table for sample values. */
/* map_size is 2, 4, 16, or 256.  Note that 255 % (map_size - 1) == 0, */
/* so the division 0xffffL / (map_size - 1) is always exact. */
private void
image_init_map(byte * map, int map_size, const float *decode)
{
    float min_v = decode[0];
    float diff_v = decode[1] - min_v;

    if (diff_v == 1 || diff_v == -1) {	/* We can do the stepping with integers, without overflow. */
	byte *limit = map + map_size;
	uint value = min_v * 0xffffL;
	int diff = diff_v * (0xffffL / (map_size - 1));

	for (; map != limit; map++, value += diff)
	    *map = value >> 8;
    } else {			/* Step in floating point, with clamping. */
	int i;

	for (i = 0; i < map_size; ++i) {
	    int value = (int)((min_v + diff_v * i / (map_size - 1)) * 255);

	    map[i] = (value < 0 ? 0 : value > 255 ? 255 : value);
	}
    }
}

/*
 * Scale a pair of mask_color values to match the scaling of each sample to
 * a full byte, and complement and swap them if the map incorporates
 * a Decode = [1 0] inversion.
 */
void
gx_image_scale_mask_colors(gx_image_enum *penum, int component_index)
{
    uint scale = 255 / ((1 << penum->bps) - 1);
    uint *values = &penum->mask_color.values[component_index * 2];
    uint v0 = values[0] *= scale;
    uint v1 = values[1] *= scale;

    if (penum->map[component_index].decoding == sd_none &&
	penum->map[component_index].inverted
	) {
	values[0] = 255 - v1;
	values[1] = 255 - v0;
    }
}

/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gsptype1.c,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* PatternType 1 pattern implementation */
#include "math_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsrop.h"
#include "gsstruct.h"
#include "gsutil.h"		/* for gs_next_ids */
#include "gxarith.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gxcoord.h"		/* for gs_concat, gx_tr'_to_fixed */
#include "gxcspace.h"		/* for gscolor2.h */
#include "gxcolor2.h"
#include "gxdcolor.h"
#include "gxdevice.h"
#include "gxdevmem.h"
#include "gxclip2.h"
#include "gspath.h"
#include "gxpath.h"
#include "gxpcolor.h"
#include "gxp1impl.h"		/* requires gxpcolor.h */
#include "gzstate.h"
#include "gsimage.h"
#include "gsiparm4.h"

/* GC descriptors */
private_st_pattern1_template();
private_st_pattern1_instance();

/* GC procedures */
private ENUM_PTRS_BEGIN(pattern1_instance_enum_ptrs) {
    if (index < st_pattern1_template_max_ptrs) {
	gs_ptr_type_t ptype =
	    ENUM_SUPER_ELT(gs_pattern1_instance_t, st_pattern1_template,
			   template, 0);

	if (ptype)
	    return ptype;
	return ENUM_OBJ(NULL);	/* don't stop early */
    }
    ENUM_PREFIX(st_pattern_instance, st_pattern1_template_max_ptrs);
} ENUM_PTRS_END
private RELOC_PTRS_BEGIN(pattern1_instance_reloc_ptrs) {
    RELOC_PREFIX(st_pattern_instance);
    RELOC_SUPER(gs_pattern1_instance_t, st_pattern1_template, template);
} RELOC_PTRS_END

/* Define a PatternType 1 pattern. */
private pattern_proc_uses_base_space(gs_pattern1_uses_base_space);
private pattern_proc_make_pattern(gs_pattern1_make_pattern);
private pattern_proc_get_pattern(gs_pattern1_get_pattern);
private const gs_pattern_type_t gs_pattern1_type = {
    1, {
	gs_pattern1_uses_base_space, gs_pattern1_make_pattern,
	gs_pattern1_get_pattern, gs_pattern1_remap_color
    }
};

/*
 * Build a PatternType 1 Pattern color space.
 */
int
gs_cspace_build_Pattern1(gs_color_space ** ppcspace,
		    const gs_color_space * pbase_cspace, gs_memory_t * pmem)
{
    gs_color_space *pcspace = 0;
    int code;

    if (pbase_cspace != 0) {
	if (gs_color_space_num_components(pcspace) < 0)		/* Pattern space */
	    return_error(gs_error_rangecheck);
    }
    code = gs_cspace_alloc(&pcspace, &gs_color_space_type_Pattern, pmem);
    if (code < 0)
	return code;
    if (pbase_cspace != 0) {
	pcspace->params.pattern.has_base_space = true;
	gs_cspace_init_from((gs_color_space *) & (pcspace->params.pattern.base_space),
			    pbase_cspace
	    );
    } else
	pcspace->params.pattern.has_base_space = false;
    *ppcspace = pcspace;
    return 0;
}

/* Initialize a PatternType 1 pattern template. */
void
gs_pattern1_init(gs_pattern1_template_t * ppat)
{
    gs_pattern_common_init((gs_pattern_template_t *)ppat, &gs_pattern1_type);
}

/* Make an instance of a PatternType 1 pattern. */
private int compute_inst_matrix(P3(gs_pattern1_instance_t * pinst,
				   const gs_state * saved, gs_rect * pbbox));
int
gs_makepattern(gs_client_color * pcc, const gs_pattern1_template_t * pcp,
	       const gs_matrix * pmat, gs_state * pgs, gs_memory_t * mem)
{
    return gs_pattern1_make_pattern(pcc, (const gs_pattern_template_t *)pcp,
				    pmat, pgs, mem);
}
private int
gs_pattern1_make_pattern(gs_client_color * pcc,
			 const gs_pattern_template_t * ptemp,
			 const gs_matrix * pmat, gs_state * pgs,
			 gs_memory_t * mem)
{
    const gs_pattern1_template_t *pcp = (const gs_pattern1_template_t *)ptemp;
    gs_pattern1_instance_t inst;
    gs_pattern1_instance_t *pinst;
    gs_state *saved;
    gs_rect bbox;
    gs_fixed_rect cbox;
    int code = gs_make_pattern_common(pcc, (const gs_pattern_template_t *)pcp,
				      pmat, pgs, mem,
				      &st_pattern1_instance);

    if (code < 0)
	return code;
    if (mem == 0)
	mem = gs_state_memory(pgs);
    pinst = (gs_pattern1_instance_t *)pcc->pattern;
    *(gs_pattern_instance_t *)&inst = *(gs_pattern_instance_t *)pinst;
    saved = inst.saved;
    switch (pcp->PaintType) {
	case 1:		/* colored */
	    gs_set_logical_op(saved, lop_default);
	    break;
	case 2:		/* uncolored */
	    gx_set_device_color_1(saved);
	    break;
	default:
	    code = gs_note_error(gs_error_rangecheck);
	    goto fsaved;
    }
    inst.template = *pcp;
    code = compute_inst_matrix(&inst, saved, &bbox);
    if (code < 0)
	goto fsaved;
#define mat inst.step_matrix
    if_debug6('t', "[t]step_matrix=[%g %g %g %g %g %g]\n",
	      mat.xx, mat.xy, mat.yx, mat.yy, mat.tx, mat.ty);
    if_debug4('t', "[t]bbox=(%g,%g),(%g,%g)\n",
	      bbox.p.x, bbox.p.y, bbox.q.x, bbox.q.y);
    {
	float bbw = bbox.q.x - bbox.p.x;
	float bbh = bbox.q.y - bbox.p.y;

	/* If the step and the size agree to within 1/2 pixel, */
	/* make them the same. */
	inst.size.x = (int)(bbw + 0.8);		/* 0.8 is arbitrary */
	inst.size.y = (int)(bbh + 0.8);

	if (inst.size.x == 0 || inst.size.y == 0) {
	    /*
	     * The pattern is empty: the stepping matrix doesn't matter.
	     */
	    gs_make_identity(&mat);
	    bbox.p.x = bbox.p.y = bbox.q.x = bbox.q.y = 0;
	} else {
	    /* Check for singular stepping matrix. */
	    if (fabs(mat.xx * mat.yy - mat.xy * mat.yx) < 1.0e-6) {
		code = gs_note_error(gs_error_rangecheck);
		goto fsaved;
	    }
	    if (mat.xy == 0 && mat.yx == 0 &&
		fabs(fabs(mat.xx) - bbw) < 0.5 &&
		fabs(fabs(mat.yy) - bbh) < 0.5
		) {
		gs_scale(saved, fabs(inst.size.x / mat.xx),
			 fabs(inst.size.y / mat.yy));
		code = compute_inst_matrix(&inst, saved, &bbox);
		if (code < 0)
		    goto fsaved;
		if_debug2('t',
			  "[t]adjusted XStep & YStep to size=(%d,%d)\n",
			  inst.size.x, inst.size.y);
		if_debug4('t', "[t]bbox=(%g,%g),(%g,%g)\n",
			  bbox.p.x, bbox.p.y, bbox.q.x, bbox.q.y);
	    }
	}
    }
    if ((code = gs_bbox_transform_inverse(&bbox, &mat, &inst.bbox)) < 0)
	goto fsaved;
    if_debug4('t', "[t]ibbox=(%g,%g),(%g,%g)\n",
	      inst.bbox.p.x, inst.bbox.p.y, inst.bbox.q.x, inst.bbox.q.y);
    inst.is_simple = (fabs(mat.xx) == inst.size.x && mat.xy == 0 &&
		      mat.yx == 0 && fabs(mat.yy) == inst.size.y);
    if_debug6('t',
	      "[t]is_simple? xstep=(%g,%g) ystep=(%g,%g) size=(%d,%d)\n",
	      inst.step_matrix.xx, inst.step_matrix.xy,
	      inst.step_matrix.yx, inst.step_matrix.yy,
	      inst.size.x, inst.size.y);
    /* Absent other information, instances always require a mask. */
    inst.uses_mask = true;
    gx_translate_to_fixed(saved, float2fixed(mat.tx - bbox.p.x),
			  float2fixed(mat.ty - bbox.p.y));
    mat.tx = bbox.p.x;
    mat.ty = bbox.p.y;
#undef mat
    cbox.p.x = fixed_0;
    cbox.p.y = fixed_0;
    cbox.q.x = int2fixed(inst.size.x);
    cbox.q.y = int2fixed(inst.size.y);
    code = gx_clip_to_rectangle(saved, &cbox);
    if (code < 0)
	goto fsaved;
    inst.id = gs_next_ids(1);
    *pinst = inst;
    return 0;
#undef mat
  fsaved:gs_state_free(saved);
    gs_free_object(mem, pinst, "gs_makepattern");
    return code;
}
/* Compute the stepping matrix and device space instance bounding box */
/* from the step values and the saved matrix. */
private int
compute_inst_matrix(gs_pattern1_instance_t * pinst, const gs_state * saved,
		    gs_rect * pbbox)
{
    double xx = pinst->template.XStep * saved->ctm.xx;
    double xy = pinst->template.XStep * saved->ctm.xy;
    double yx = pinst->template.YStep * saved->ctm.yx;
    double yy = pinst->template.YStep * saved->ctm.yy;

    /* Adjust the stepping matrix so all coefficients are >= 0. */
    if (xx == 0 || yy == 0) {	/* We know that both xy and yx are non-zero. */
	double temp;

	temp = xx, xx = yx, yx = temp;
	temp = xy, xy = yy, yy = temp;
    }
    if (xx < 0)
	xx = -xx, xy = -xy;
    if (yy < 0)
	yx = -yx, yy = -yy;
    /* Now xx > 0, yy > 0. */
    pinst->step_matrix.xx = xx;
    pinst->step_matrix.xy = xy;
    pinst->step_matrix.yx = yx;
    pinst->step_matrix.yy = yy;
    pinst->step_matrix.tx = saved->ctm.tx;
    pinst->step_matrix.ty = saved->ctm.ty;
    return gs_bbox_transform(&pinst->template.BBox, &ctm_only(saved),
			     pbbox);
}

/* Test whether a PatternType 1 pattern uses a base space. */
private bool
gs_pattern1_uses_base_space(const gs_pattern_template_t *ptemp)
{
    return ((const gs_pattern1_template_t *)ptemp)->PaintType == 2;
}

/* getpattern for PatternType 1 */
/* This is only intended for the benefit of pattern PaintProcs. */
private const gs_pattern_template_t *
gs_pattern1_get_pattern(const gs_pattern_instance_t *pinst)
{
    return (const gs_pattern_template_t *)
	&((const gs_pattern1_instance_t *)pinst)->template;
}
const gs_pattern1_template_t *
gs_getpattern(const gs_client_color * pcc)
{
    const gs_pattern_instance_t *pinst = pcc->pattern;

    return (pinst == 0 || pinst->type != &gs_pattern1_type ? 0 :
	    &((const gs_pattern1_instance_t *)pinst)->template);
}

/*
 *  Code for generating patterns from bitmaps and pixmaps.
 */

/*
 *  The following structures are realized here only because this is the
 *  first location in which they were needed. Otherwise, there is nothing
 *  about them that is specific to patterns.
 */
public_st_gs_bitmap();
public_st_gs_tile_bitmap();
public_st_gs_depth_bitmap();
public_st_gs_tile_depth_bitmap();
public_st_gx_strip_bitmap();

/*
 *  Structure for holding a gs_depth_bitmap and the corresponding depth and
 *  colorspace information.
 *
 *  The free_proc pointer is needed to hold the original value of the pattern
 *  instance free structure. This pointer in the pattern instance will be
 *  overwritten with free_pixmap_pattern, which will free the pixmap info
 *  structure when it is freed.
 */
typedef struct pixmap_info_s {
    gs_depth_bitmap bitmap;	/* must be first */
    const gs_color_space *pcspace;
    uint white_index;
    void (*free_proc)(P3(gs_memory_t *, void *, client_name_t));
} pixmap_info;

gs_private_st_suffix_add1(st_pixmap_info,
			  pixmap_info,
			  "pixmap info. struct",
			  pixmap_enum_ptr,
			  pixmap_reloc_ptr,
			  st_gs_depth_bitmap,
			  pcspace
);

#define st_pixmap_info_max_ptrs (1 + st_tile_bitmap_max_ptrs)

/*
 *  Free routine for pattern instances created from pixmaps. This overwrites
 *  the free procedure originally stored in the pattern instance, and stores
 *  the pointer to that procedure in the pixmap_info structure. This procedure
 *  will call the original procedure, then free the pixmap_info structure. 
 *
 *  Note that this routine does NOT release the data in the original pixmap;
 *  that remains the responsibility of the client.
 */
private void
free_pixmap_pattern(
    gs_memory_t *           pmem,
    void *                  pvpinst,
    client_name_t           cname
)
{
    gs_pattern1_instance_t *pinst = (gs_pattern1_instance_t *)pvpinst;
    pixmap_info *ppmap = pinst->template.client_data;

    ppmap->free_proc(pmem, pvpinst, cname);
    gs_free_object(pmem, ppmap, cname);
}

/*
 *  PaintProcs for bitmap and pixmap patterns.
 */
private int bitmap_paint(P4(gs_image_enum * pen, gs_data_image_t * pim,
			  const gs_depth_bitmap * pbitmap, gs_state * pgs));
private int
mask_PaintProc(const gs_client_color * pcolor, gs_state * pgs)
{
    const pixmap_info *ppmap = gs_getpattern(pcolor)->client_data;
    const gs_depth_bitmap *pbitmap = &(ppmap->bitmap);
    gs_image_enum *pen =
    gs_image_enum_alloc(gs_state_memory(pgs), "mask_PaintProc");
    gs_image1_t mask;

    if (pen == 0)
	return_error(gs_error_VMerror);
    gs_image_t_init_mask(&mask, true);
    mask.Width = pbitmap->size.x;
    mask.Height = pbitmap->size.y;
    gs_image_init(pen, &mask, false, pgs);
    return bitmap_paint(pen, (gs_data_image_t *) & mask, pbitmap, pgs);
}
private int
image_PaintProc(const gs_client_color * pcolor, gs_state * pgs)
{
    const pixmap_info *ppmap = gs_getpattern(pcolor)->client_data;
    const gs_depth_bitmap *pbitmap = &(ppmap->bitmap);
    gs_image_enum *pen =
    gs_image_enum_alloc(gs_state_memory(pgs), "image_PaintProc");
    const gs_color_space *pcspace =
    (ppmap->pcspace == 0 ?
     gs_cspace_DeviceGray((const gs_imager_state *)pgs) :
     ppmap->pcspace);
    gx_image_enum_common_t *pie;
    gs_image4_t image;
    int code;

    if (pen == 0)
	return_error(gs_error_VMerror);
    gs_image4_t_init(&image, pcspace);
    image.Width = pbitmap->size.x;
    image.Height = pbitmap->size.y;
    image.MaskColor_is_range = false;
    image.MaskColor[0] = ppmap->white_index;
    image.Decode[0] = 0;
    image.Decode[1] = (1 << pbitmap->pix_depth) - 1;
    image.BitsPerComponent = pbitmap->pix_depth;
    /* backwards compatibility */
    if (ppmap->pcspace == 0) {
	image.Decode[0] = 1.0;
	image.Decode[1] = 0.0;
    }
    code = gs_image_begin_typed((const gs_image_common_t *)&image, pgs,
				false, &pie);
    if (code < 0)
	return code;
    code = gs_image_enum_init(pen, pie, (gs_data_image_t *)&image, pgs);
    if (code < 0)
	return code;
    return bitmap_paint(pen, (gs_data_image_t *) & image, pbitmap, pgs);
}
/* Finish painting any kind of bitmap pattern. */
private int
bitmap_paint(gs_image_enum * pen, gs_data_image_t * pim,
	     const gs_depth_bitmap * pbitmap, gs_state * pgs)
{
    uint raster = pbitmap->raster;
    uint nbytes = (pim->Width * pbitmap->pix_depth + 7) >> 3;
    uint used;
    const byte *dp = pbitmap->data;
    int n;
    int code = 0;

    if (nbytes == raster)
	code = gs_image_next(pen, dp, nbytes * pim->Height, &used);
    else
	for (n = pim->Height; n > 0 && code >= 0; dp += raster, --n)
	    code = gs_image_next(pen, dp, nbytes, &used);
    gs_image_cleanup(pen);
    gs_free_object(gs_state_memory(pgs), pen, "bitmap_paint");
    return code;
}

/*
 * Make a pattern from a bitmap or pixmap. The pattern may be colored or
 * uncolored, as determined by the mask operand. This code is intended
 * primarily for use by PCL.
 *
 * See the comment prior to the declaration of this function in gscolor2.h
 * for further information.
 */
int
gs_makepixmappattern(
			gs_client_color * pcc,
			const gs_depth_bitmap * pbitmap,
			bool mask,
			const gs_matrix * pmat,
			long id,
			const gs_color_space * pcspace,
			uint white_index,
			gs_state * pgs,
			gs_memory_t * mem
)
{

    gs_pattern1_template_t pat;
    pixmap_info *ppmap;
    gs_matrix mat, smat;
    int code;

    /* check that the data is legitimate */
    if ((mask) || (pcspace == 0)) {
	if (pbitmap->pix_depth != 1)
	    return_error(gs_error_rangecheck);
	pcspace = 0;
    } else if (gs_color_space_get_index(pcspace) != gs_color_space_index_Indexed)
	return_error(gs_error_rangecheck);
    if (pbitmap->num_comps != 1)
	return_error(gs_error_rangecheck);

    /* allocate and initialize a pixmap_info structure for the paint proc */
    if (mem == 0)
	mem = gs_state_memory(pgs);
    ppmap = gs_alloc_struct(mem,
			    pixmap_info,
			    &st_pixmap_info,
			    "makepximappattern"
	);
    if (ppmap == 0)
	return_error(gs_error_VMerror);
    ppmap->bitmap = *pbitmap;
    ppmap->pcspace = pcspace;
    ppmap->white_index = white_index;

    /* set up the client pattern structure */
    gs_pattern1_init(&pat);
    uid_set_UniqueID(&pat.uid, (id == no_UniqueID) ? gs_next_ids(1) : id);
    pat.PaintType = (mask ? 2 : 1);
    pat.TilingType = 1;
    pat.BBox.p.x = 0;
    pat.BBox.p.y = 0;
    pat.BBox.q.x = pbitmap->size.x;
    pat.BBox.q.y = pbitmap->size.y;
    pat.XStep = pbitmap->size.x;
    pat.YStep = pbitmap->size.y;
    pat.PaintProc = (mask ? mask_PaintProc : image_PaintProc);
    pat.client_data = ppmap;

    /* set the ctm to be the identity */
    gs_currentmatrix(pgs, &smat);
    gs_make_identity(&mat);
    gs_setmatrix(pgs, &mat);

    /* build the pattern, restore the previous matrix */
    if (pmat == NULL)
	pmat = &mat;
    if ((code = gs_makepattern(pcc, &pat, pmat, pgs, mem)) != 0)
	gs_free_object(mem, ppmap, "makebitmappattern_xform");
    else {
	/*
	 * If this is not a masked pattern and if the white pixel index
	 * is outside of the representable range, we don't need to go to
	 * the trouble of accumulating a mask that will just be all 1s.
	 */
	gs_pattern1_instance_t *pinst =
	    (gs_pattern1_instance_t *)pcc->pattern;

	if (!mask && (white_index >= (1 << pbitmap->pix_depth)))
	    pinst->uses_mask = false;

        /* overwrite the free procedure for the pattern instance */
        ppmap->free_proc = pinst->rc.free;
        pinst->rc.free = free_pixmap_pattern;

	/*
	 * Since the PaintProcs don't reference the saved color space or
	 * color, clear these so that there isn't an extra retained
	 * reference to the Pattern object.
	 */
	gs_setgray(pinst->saved, 0.0);

    }
    gs_setmatrix(pgs, &smat);
    return code;
}

/*
 *  Backwards compatibility.
 */
int
gs_makebitmappattern_xform(
			      gs_client_color * pcc,
			      const gx_tile_bitmap * ptile,
			      bool mask,
			      const gs_matrix * pmat,
			      long id,
			      gs_state * pgs,
			      gs_memory_t * mem
)
{
    gs_depth_bitmap bitmap;

    /* build the bitmap the size of one repetition */
    bitmap.data = ptile->data;
    bitmap.raster = ptile->raster;
    bitmap.size.x = ptile->rep_width;
    bitmap.size.y = ptile->rep_height;
    bitmap.id = ptile->id;	/* shouldn't matter */
    bitmap.pix_depth = 1;
    bitmap.num_comps = 1;

    return gs_makepixmappattern(pcc, &bitmap, mask, pmat, id, 0, 0, pgs, mem);
}


/* ------ Color space implementation ------ */

/*
 * Defined the Pattern device color types.  We need a masked analogue of
 * each of the non-pattern types, to handle uncolored patterns.  We use
 * 'masked_fill_rect' instead of 'masked_fill_rectangle' in order to limit
 * identifier lengths to 32 characters.
 */
private dev_color_proc_load(gx_dc_pattern_load);
/*dev_color_proc_fill_rectangle(gx_dc_pattern_fill_rectangle); *//*gxp1fill.h */
private dev_color_proc_equal(gx_dc_pattern_equal);
private dev_color_proc_load(gx_dc_pure_masked_load);

/*dev_color_proc_fill_rectangle(gx_dc_pure_masked_fill_rect); *//*gxp1fill.h */
private dev_color_proc_equal(gx_dc_pure_masked_equal);
private dev_color_proc_load(gx_dc_binary_masked_load);

/*dev_color_proc_fill_rectangle(gx_dc_binary_masked_fill_rect); *//*gxp1fill.h */
private dev_color_proc_equal(gx_dc_binary_masked_equal);
private dev_color_proc_load(gx_dc_colored_masked_load);

/*dev_color_proc_fill_rectangle(gx_dc_colored_masked_fill_rect); *//*gxp1fill.h */
private dev_color_proc_equal(gx_dc_colored_masked_equal);

/* The device color types are exported for gxpcmap.c. */
gs_private_st_composite(st_dc_pattern, gx_device_color, "dc_pattern",
			dc_pattern_enum_ptrs, dc_pattern_reloc_ptrs);
const gx_device_color_type_t gx_dc_pattern = {
    &st_dc_pattern,
    gx_dc_pattern_load, gx_dc_pattern_fill_rectangle,
    gx_dc_default_fill_masked, gx_dc_pattern_equal
};

extern_st(st_dc_ht_binary);
gs_private_st_composite(st_dc_pure_masked, gx_device_color, "dc_pure_masked",
			dc_masked_enum_ptrs, dc_masked_reloc_ptrs);
const gx_device_color_type_t gx_dc_pure_masked = {
    &st_dc_pure_masked,
    gx_dc_pure_masked_load, gx_dc_pure_masked_fill_rect,
    gx_dc_default_fill_masked, gx_dc_pure_masked_equal
};

gs_private_st_composite(st_dc_binary_masked, gx_device_color,
			"dc_binary_masked", dc_binary_masked_enum_ptrs,
			dc_binary_masked_reloc_ptrs);
const gx_device_color_type_t gx_dc_binary_masked = {
    &st_dc_binary_masked,
    gx_dc_binary_masked_load, gx_dc_binary_masked_fill_rect,
    gx_dc_default_fill_masked, gx_dc_binary_masked_equal
};

gs_private_st_composite_only(st_dc_colored_masked, gx_device_color,
			     "dc_colored_masked",
			     dc_masked_enum_ptrs, dc_masked_reloc_ptrs);
const gx_device_color_type_t gx_dc_colored_masked = {
    &st_dc_colored_masked,
    gx_dc_colored_masked_load, gx_dc_colored_masked_fill_rect,
    gx_dc_default_fill_masked, gx_dc_colored_masked_equal
};

#undef gx_dc_type_pattern
const gx_device_color_type_t *const gx_dc_type_pattern = &gx_dc_pattern;
#define gx_dc_type_pattern (&gx_dc_pattern)

/* GC procedures */
private 
ENUM_PTRS_WITH(dc_pattern_enum_ptrs, gx_device_color *cptr)
{
    return ENUM_USING(st_dc_pure_masked, vptr, size, index - 1);
}
case 0:
{
    gx_color_tile *tile = cptr->colors.pattern.p_tile;

    ENUM_RETURN((tile == 0 ? tile : tile - tile->index));
}
ENUM_PTRS_END
private RELOC_PTRS_WITH(dc_pattern_reloc_ptrs, gx_device_color *cptr)
{
    gx_color_tile *tile = cptr->colors.pattern.p_tile;

    if (tile != 0) {
	uint index = tile->index;

	RELOC_TYPED_OFFSET_PTR(gx_device_color, colors.pattern.p_tile, index);
    }
    RELOC_USING(st_dc_pure_masked, vptr, size);
}
RELOC_PTRS_END
private ENUM_PTRS_WITH(dc_masked_enum_ptrs, gx_device_color *cptr)
ENUM_SUPER(gx_device_color, st_client_color, ccolor, 1);
case 0:
{
    gx_color_tile *mask = cptr->mask.m_tile;

    ENUM_RETURN((mask == 0 ? mask : mask - mask->index));
}
ENUM_PTRS_END
private RELOC_PTRS_WITH(dc_masked_reloc_ptrs, gx_device_color *cptr)
{
    gx_color_tile *mask = cptr->mask.m_tile;

    RELOC_SUPER(gx_device_color, st_client_color, ccolor);
    if (mask != 0) {
	uint index = mask->index;

	RELOC_TYPED_OFFSET_PTR(gx_device_color, mask.m_tile, index);
    }
}
RELOC_PTRS_END
private ENUM_PTRS_BEGIN(dc_binary_masked_enum_ptrs)
{
    return ENUM_USING(st_dc_ht_binary, vptr, size, index - 2);
}
case 0:
case 1:
return ENUM_USING(st_dc_pure_masked, vptr, size, index);
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(dc_binary_masked_reloc_ptrs)
{
    RELOC_USING(st_dc_pure_masked, vptr, size);
    RELOC_USING(st_dc_ht_binary, vptr, size);
}
RELOC_PTRS_END

/* Macros for pattern loading */
#define FINISH_PATTERN_LOAD\
	while ( !gx_pattern_cache_lookup(pdevc, pis, dev, select) )\
	 { code = gx_pattern_load(pdevc, pis, dev, select);\
	   if ( code < 0 ) break;\
	 }\
	return code;

/* Ensure that a colored Pattern is loaded in the cache. */
private int
gx_dc_pattern_load(gx_device_color * pdevc, const gs_imager_state * pis,
		   gx_device * dev, gs_color_select_t select)
{
    int code = 0;

    FINISH_PATTERN_LOAD
}
/* Ensure that an uncolored Pattern is loaded in the cache. */
private int
gx_dc_pure_masked_load(gx_device_color * pdevc, const gs_imager_state * pis,
		       gx_device * dev, gs_color_select_t select)
{
    int code = (*gx_dc_type_data_pure.load) (pdevc, pis, dev, select);

    if (code < 0)
	return code;
    FINISH_PATTERN_LOAD
}
private int
gx_dc_binary_masked_load(gx_device_color * pdevc, const gs_imager_state * pis,
			 gx_device * dev, gs_color_select_t select)
{
    int code = (*gx_dc_type_data_ht_binary.load) (pdevc, pis, dev, select);

    if (code < 0)
	return code;
    FINISH_PATTERN_LOAD
}
private int
gx_dc_colored_masked_load(gx_device_color * pdevc, const gs_imager_state * pis,
			  gx_device * dev, gs_color_select_t select)
{
    int code = (*gx_dc_type_data_ht_colored.load) (pdevc, pis, dev, select);

    if (code < 0)
	return code;
    FINISH_PATTERN_LOAD
}

/* Look up a pattern color in the cache. */
bool
gx_pattern_cache_lookup(gx_device_color * pdevc, const gs_imager_state * pis,
			gx_device * dev, gs_color_select_t select)
{
    gx_pattern_cache *pcache = pis->pattern_cache;
    gx_bitmap_id id = pdevc->mask.id;

    if (id == gx_no_bitmap_id) {
	color_set_null_pattern(pdevc);
	return true;
    }
    if (pcache != 0) {
	gx_color_tile *ctile = &pcache->tiles[id % pcache->num_tiles];

	if (ctile->id == id &&
	    (pdevc->type != &gx_dc_pattern ||
	     ctile->depth == dev->color_info.depth)
	    ) {
	    int px = pis->screen_phase[select].x;
	    int py = pis->screen_phase[select].y;

	    if (pdevc->type == &gx_dc_pattern) {	/* colored */
		pdevc->colors.pattern.p_tile = ctile;
		color_set_phase_mod(pdevc, px, py,
				    ctile->tbits.rep_width,
				    ctile->tbits.rep_height);
	    }
	    pdevc->mask.m_tile =
		(ctile->tmask.data == 0 ? (gx_color_tile *) 0 :
		 ctile);
	    pdevc->mask.m_phase.x = -px;
	    pdevc->mask.m_phase.y = -py;
	    return true;
	}
    }
    return false;
}

#undef FINISH_PATTERN_LOAD

/* Compare two Pattern colors for equality. */
private bool
gx_dc_pattern_equal(const gx_device_color * pdevc1,
		    const gx_device_color * pdevc2)
{
    return pdevc2->type == pdevc1->type &&
	pdevc1->phase.x == pdevc2->phase.x &&
	pdevc1->phase.y == pdevc2->phase.y &&
	pdevc1->mask.id == pdevc2->mask.id;
}
private bool
gx_dc_pure_masked_equal(const gx_device_color * pdevc1,
			const gx_device_color * pdevc2)
{
    return (*gx_dc_type_pure->equal) (pdevc1, pdevc2) &&
	pdevc1->mask.id == pdevc2->mask.id;
}
private bool
gx_dc_binary_masked_equal(const gx_device_color * pdevc1,
			  const gx_device_color * pdevc2)
{
    return (*gx_dc_type_ht_binary->equal) (pdevc1, pdevc2) &&
	pdevc1->mask.id == pdevc2->mask.id;
}
private bool
gx_dc_colored_masked_equal(const gx_device_color * pdevc1,
			   const gx_device_color * pdevc2)
{
    return (*gx_dc_type_ht_colored->equal) (pdevc1, pdevc2) &&
	pdevc1->mask.id == pdevc2->mask.id;
}

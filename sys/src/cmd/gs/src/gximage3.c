/* Copyright (C) 1997, 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gximage3.c,v 1.15 2005/03/29 14:26:26 igor Exp $ */
/* ImageType 3 image implementation */
#include "math_.h"		/* for ceil, floor */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsbitops.h"
#include "gscspace.h"
#include "gsstruct.h"
#include "gxdevice.h"
#include "gxdevmem.h"
#include "gxclipm.h"
#include "gximage3.h"
#include "gxistate.h"

/* Forward references */
private dev_proc_begin_typed_image(gx_begin_image3);
private image_enum_proc_plane_data(gx_image3_plane_data);
private image_enum_proc_end_image(gx_image3_end_image);
private image_enum_proc_flush(gx_image3_flush);
private image_enum_proc_planes_wanted(gx_image3_planes_wanted);

/* GC descriptor */
private_st_gs_image3();

/* Define the image type for ImageType 3 images. */
const gx_image_type_t gs_image_type_3 = {
    &st_gs_image3, gx_begin_image3, gx_data_image_source_size,
    gx_image_no_sput, gx_image_no_sget, gx_image_default_release, 3
};
private const gx_image_enum_procs_t image3_enum_procs = {
    gx_image3_plane_data, gx_image3_end_image,
    gx_image3_flush, gx_image3_planes_wanted
};

/* Initialize an ImageType 3 image. */
void
gs_image3_t_init(gs_image3_t * pim, const gs_color_space * color_space,
		 gs_image3_interleave_type_t interleave_type)
{
    gs_pixel_image_t_init((gs_pixel_image_t *) pim, color_space);
    pim->type = &gs_image_type_3;
    pim->InterleaveType = interleave_type;
    gs_data_image_t_init(&pim->MaskDict, -1);
}

/*
 * We implement ImageType 3 images by interposing a mask clipper in
 * front of an ordinary ImageType 1 image.  Note that we build up the
 * mask row-by-row as we are processing the image.
 *
 * We export a generalized form of the begin_image procedure for use by
 * the PDF and PostScript writers.
 */
typedef struct gx_image3_enum_s {
    gx_image_enum_common;
    gx_device *mdev;		/* gx_device_memory in default impl. */
    gx_device *pcdev;		/* gx_device_mask_clip in default impl. */
    gx_image_enum_common_t *mask_info;
    gx_image_enum_common_t *pixel_info;
    gs_image3_interleave_type_t InterleaveType;
    int num_components;		/* (not counting mask) */
    int bpc;			/* BitsPerComponent */
    gs_memory_t *memory;
    int mask_width, mask_height, mask_full_height;
    int pixel_width, pixel_height, pixel_full_height;
    byte *mask_data;		/* (if chunky) */
    byte *pixel_data;		/* (if chunky) */
    /* The following are the only members that change dynamically. */
    int mask_y;
    int pixel_y;
    int mask_skip;		/* # of mask rows to skip, see below */
} gx_image3_enum_t;

extern_st(st_gx_image_enum_common);
gs_private_st_suffix_add6(st_image3_enum, gx_image3_enum_t, "gx_image3_enum_t",
  image3_enum_enum_ptrs, image3_enum_reloc_ptrs, st_gx_image_enum_common,
  mdev, pcdev, pixel_info, mask_info, pixel_data, mask_data);

/* Define the default implementation of ImageType 3 processing. */
private IMAGE3_MAKE_MID_PROC(make_mid_default); /* check prototype */
private int
make_mid_default(gx_device **pmidev, gx_device *dev, int width, int height,
		 gs_memory_t *mem)
{
    gx_device_memory *midev =
	gs_alloc_struct(mem, gx_device_memory, &st_device_memory,
			"make_mid_default");
    int code;

    if (midev == 0)
	return_error(gs_error_VMerror);
    gs_make_mem_mono_device(midev, mem, NULL);
    midev->bitmap_memory = mem;
    midev->width = width;
    midev->height = height;
    check_device_separable((gx_device *)midev);
    gx_device_fill_in_procs((gx_device *)midev);
    code = dev_proc(midev, open_device)((gx_device *)midev);
    if (code < 0) {
	gs_free_object(mem, midev, "make_mid_default");
	return code;
    }
    midev->is_open = true;
    dev_proc(midev, fill_rectangle)
	((gx_device *)midev, 0, 0, width, height, (gx_color_index)0);
    *pmidev = (gx_device *)midev;
    return 0;
}
private IMAGE3_MAKE_MCDE_PROC(make_mcde_default);  /* check prototype */
private int
make_mcde_default(gx_device *dev, const gs_imager_state *pis,
		  const gs_matrix *pmat, const gs_image_common_t *pic,
		  const gs_int_rect *prect, const gx_drawing_color *pdcolor,
		  const gx_clip_path *pcpath, gs_memory_t *mem,
		  gx_image_enum_common_t **pinfo,
		  gx_device **pmcdev, gx_device *midev,
		  gx_image_enum_common_t *pminfo,
		  const gs_int_point *origin)
{
    gx_device_memory *const mdev = (gx_device_memory *)midev;
    gx_device_mask_clip *mcdev =
	gs_alloc_struct(mem, gx_device_mask_clip, &st_device_mask_clip,
			"make_mcde_default");
    gx_strip_bitmap bits;	/* only gx_bitmap */
    int code;

    if (mcdev == 0)
	return_error(gs_error_VMerror);
    bits.data = mdev->base;
    bits.raster = mdev->raster;
    bits.size.x = mdev->width;
    bits.size.y = mdev->height;
    bits.id = gx_no_bitmap_id;
    code = gx_mask_clip_initialize(mcdev, &gs_mask_clip_device,
				   (const gx_bitmap *)&bits, dev,
				   origin->x, origin->y, mem);
    if (code < 0) {
	gs_free_object(mem, mcdev, "make_mcde_default");
	return code;
    }
    mcdev->tiles = bits;
    code = dev_proc(mcdev, begin_typed_image)
	((gx_device *)mcdev, pis, pmat, pic, prect, pdcolor, pcpath, mem,
	 pinfo);
    if (code < 0) {
	gs_free_object(mem, mcdev, "make_mcde_default");
	return code;
    }
    *pmcdev = (gx_device *)mcdev;
    return 0;
}
private int
gx_begin_image3(gx_device * dev,
		const gs_imager_state * pis, const gs_matrix * pmat,
		const gs_image_common_t * pic, const gs_int_rect * prect,
		const gx_drawing_color * pdcolor, const gx_clip_path * pcpath,
		gs_memory_t * mem, gx_image_enum_common_t ** pinfo)
{
    return gx_begin_image3_generic(dev, pis, pmat, pic, prect, pdcolor,
				   pcpath, mem, make_mid_default,
				   make_mcde_default, pinfo);
}

/*
 * Begin a generic ImageType 3 image, with client handling the creation of
 * the mask image and mask clip devices.
 */
private bool check_image3_extent(floatp mask_coeff, floatp data_coeff);
int
gx_begin_image3_generic(gx_device * dev,
			const gs_imager_state *pis, const gs_matrix *pmat,
			const gs_image_common_t *pic, const gs_int_rect *prect,
			const gx_drawing_color *pdcolor,
			const gx_clip_path *pcpath, gs_memory_t *mem,
			image3_make_mid_proc_t make_mid,
			image3_make_mcde_proc_t make_mcde,
			gx_image_enum_common_t **pinfo)
{
    const gs_image3_t *pim = (const gs_image3_t *)pic;
    gx_image3_enum_t *penum;
    gs_int_rect mask_rect, data_rect;
    gx_device *mdev = 0;
    gx_device *pcdev = 0;
    gs_image_t i_pixel, i_mask;
    gs_matrix mi_pixel, mi_mask, mat;
    gs_rect mrect;
    gs_int_point origin;
    int code;

    /* Validate the parameters. */
    if (pim->Height <= 0 || pim->MaskDict.Height <= 0)
	return_error(gs_error_rangecheck);
    switch (pim->InterleaveType) {
	default:
	    return_error(gs_error_rangecheck);
	case interleave_chunky:
	    if (pim->MaskDict.Width != pim->Width ||
		pim->MaskDict.Height != pim->Height ||
		pim->MaskDict.BitsPerComponent != pim->BitsPerComponent ||
		pim->format != gs_image_format_chunky
		)
		return_error(gs_error_rangecheck);
	    break;
	case interleave_scan_lines:
	    if (pim->MaskDict.Height % pim->Height != 0 &&
		pim->Height % pim->MaskDict.Height != 0
		)
		return_error(gs_error_rangecheck);
	    /* falls through */
	case interleave_separate_source:
	    if (pim->MaskDict.BitsPerComponent != 1)
		return_error(gs_error_rangecheck);
    }
    if (!check_image3_extent(pim->ImageMatrix.xx,
			     pim->MaskDict.ImageMatrix.xx) ||
	!check_image3_extent(pim->ImageMatrix.xy,
			     pim->MaskDict.ImageMatrix.xy) ||
	!check_image3_extent(pim->ImageMatrix.yx,
			     pim->MaskDict.ImageMatrix.yx) ||
	!check_image3_extent(pim->ImageMatrix.yy,
			     pim->MaskDict.ImageMatrix.yy)
	)
	return_error(gs_error_rangecheck);
    if ((code = gs_matrix_invert(&pim->ImageMatrix, &mi_pixel)) < 0 ||
	(code = gs_matrix_invert(&pim->MaskDict.ImageMatrix, &mi_mask)) < 0
	)
	return code;
    if (fabs(mi_pixel.tx - mi_mask.tx) >= 0.5 ||
	fabs(mi_pixel.ty - mi_mask.ty) >= 0.5
	)
	return_error(gs_error_rangecheck);
    {
	gs_point ep, em;

	if ((code = gs_point_transform(pim->Width, pim->Height, &mi_pixel,
				       &ep)) < 0 ||
	    (code = gs_point_transform(pim->MaskDict.Width,
				       pim->MaskDict.Height, &mi_mask,
				       &em)) < 0
	    )
	    return code;
	if (fabs(ep.x - em.x) >= 0.5 || fabs(ep.y - em.y) >= 0.5)
	    return_error(gs_error_rangecheck);
    }
    penum = gs_alloc_struct(mem, gx_image3_enum_t, &st_image3_enum,
			    "gx_begin_image3");
    if (penum == 0)
	return_error(gs_error_VMerror);
    penum->num_components =
	gs_color_space_num_components(pim->ColorSpace);
    gx_image_enum_common_init((gx_image_enum_common_t *) penum,
			      (const gs_data_image_t *)pim,
			      &image3_enum_procs, dev,
			      1 + penum->num_components,
			      pim->format);
    /* Initialize pointers now in case we bail out. */
    penum->mask_data = 0;
    penum->pixel_data = 0;
    if (prect) {
	long lmw = pim->MaskDict.Width, lmh = pim->MaskDict.Height;

	data_rect = *prect;
	mask_rect.p.x = (int)(data_rect.p.x * lmw / pim->Width);
	mask_rect.p.y = (int)(data_rect.p.y * lmh / pim->Height);
	mask_rect.q.x = (int)((data_rect.q.x + pim->Width - 1) * lmw /
			      pim->Width);
	mask_rect.q.y = (int)((data_rect.q.y + pim->Height - 1) * lmh /
			      pim->Height);
    } else {
	mask_rect.p.x = mask_rect.p.y = 0;
	mask_rect.q.x = pim->MaskDict.Width;
	mask_rect.q.y = pim->MaskDict.Height;
	data_rect.p.x = data_rect.p.y = 0;
	data_rect.q.x = pim->Width;
	data_rect.q.y = pim->Height;
    }
    penum->mask_width = mask_rect.q.x - mask_rect.p.x;
    penum->mask_height = mask_rect.q.y - mask_rect.p.y;
    penum->mask_full_height = pim->MaskDict.Height;
    penum->mask_y = 0;
    penum->mask_skip = 0;
    penum->pixel_width = data_rect.q.x - data_rect.p.x;
    penum->pixel_height = data_rect.q.y - data_rect.p.y;
    penum->pixel_full_height = pim->Height;
    penum->pixel_y = 0;
    penum->mask_info = 0;
    penum->pixel_info = 0;
    if (pim->InterleaveType == interleave_chunky) {
	/* Allocate row buffers for the mask and pixel data. */
	penum->pixel_data =
	    gs_alloc_bytes(mem,
			   (penum->pixel_width * pim->BitsPerComponent *
			    penum->num_components + 7) >> 3,
			   "gx_begin_image3(pixel_data)");
	penum->mask_data =
	    gs_alloc_bytes(mem, (penum->mask_width + 7) >> 3,
			   "gx_begin_image3(mask_data)");
	if (penum->pixel_data == 0 || penum->mask_data == 0) {
	    code = gs_note_error(gs_error_VMerror);
	    goto out1;
	}
    }
    penum->InterleaveType = pim->InterleaveType;
    penum->bpc = pim->BitsPerComponent;
    penum->memory = mem;
    mrect.p.x = mrect.p.y = 0;
    mrect.q.x = pim->MaskDict.Width;
    mrect.q.y = pim->MaskDict.Height;
    if (pmat == 0)
	pmat = &ctm_only(pis);
    if ((code = gs_matrix_multiply(&mi_mask, pmat, &mat)) < 0 ||
	(code = gs_bbox_transform(&mrect, &mat, &mrect)) < 0
	)
	return code;

    origin.x = (mrect.p.x < 0) ? (int)ceil(mrect.p.x) : (int)floor(mrect.p.x);
    origin.y = (mrect.p.y < 0) ? (int)ceil(mrect.p.y) : (int)floor(mrect.p.y);
    code = make_mid(&mdev, dev, (int)ceil(mrect.q.x) - origin.x,
		    (int)ceil(mrect.q.y) - origin.y, mem);
    if (code < 0)
	goto out1;
    penum->mdev = mdev;
    gs_image_t_init_mask(&i_mask, false);
    i_mask.adjust = false;
    {
	const gx_image_type_t *type1 = i_mask.type;

	*(gs_data_image_t *)&i_mask = pim->MaskDict;
	i_mask.type = type1;
	i_mask.BitsPerComponent = 1;
    }
    {
	gx_drawing_color dcolor;
	gs_matrix m_mat;

	set_nonclient_dev_color(&dcolor, 1);
	/*
	 * Adjust the translation for rendering the mask to include a
	 * negative translation by origin.{x,y} in device space.
	 */
	m_mat = *pmat;
	m_mat.tx -= origin.x;
	m_mat.ty -= origin.y;
	/*
	 * Note that pis = NULL here, since we don't want to have to
	 * create another imager state with default log_op, etc.
	 */
	code = gx_device_begin_typed_image(mdev, NULL, &m_mat,
					   (const gs_image_common_t *)&i_mask,
					   &mask_rect, &dcolor, NULL, mem,
					   &penum->mask_info);
	if (code < 0)
	    goto out2;
    }
    gs_image_t_init(&i_pixel, pim->ColorSpace);
    {
	const gx_image_type_t *type1 = i_pixel.type;
        const bool mask = i_pixel.ImageMask;

        /* On gcc 2.95.4 for Alpha all structures are padded to 8 byte
         * boundary but sizeof(bool) == 4. First member of the subclass
         * is restored because it is overwritten by padding data.
         */
	*(gs_pixel_image_t *)&i_pixel = *(const gs_pixel_image_t *)pim;
	i_pixel.ImageMask = mask;
	i_pixel.type = type1;
    }
    code = make_mcde(dev, pis, pmat, (const gs_image_common_t *)&i_pixel,
		     prect, pdcolor, pcpath, mem, &penum->pixel_info,
		     &pcdev, mdev, penum->mask_info, &origin);
    if (code < 0)
	goto out3;
    penum->pcdev = pcdev;
    /*
     * Set num_planes, plane_widths, and plane_depths from the values in the
     * enumerators for the mask and the image data.
     */
    switch (pim->InterleaveType) {
    case interleave_chunky:
	/* Add the mask data to the depth of the image data. */
	penum->num_planes = 1;
	penum->plane_widths[0] = pim->Width;
	penum->plane_depths[0] =
	    penum->pixel_info->plane_depths[0] *
	    (penum->num_components + 1) / penum->num_components;
	break;
    case interleave_scan_lines:
	/*
	 * There is only 1 plane, with dynamically changing width & depth.
	 * Initialize it for the mask data, since that is what will be
	 * read first.
	 */
	penum->num_planes = 1;
	penum->plane_depths[0] = 1;
	penum->plane_widths[0] = pim->MaskDict.Width;
	break;
    case interleave_separate_source:
	/* Insert the mask data as a separate plane before the image data. */
	penum->num_planes = penum->pixel_info->num_planes + 1;
	penum->plane_widths[0] = pim->MaskDict.Width;
	penum->plane_depths[0] = 1;
	memcpy(&penum->plane_widths[1], &penum->pixel_info->plane_widths[0],
	       (penum->num_planes - 1) * sizeof(penum->plane_widths[0]));
	memcpy(&penum->plane_depths[1], &penum->pixel_info->plane_depths[0],
	       (penum->num_planes - 1) * sizeof(penum->plane_depths[0]));
	break;
    }
    gx_device_retain(mdev, true); /* will free explicitly */
    gx_device_retain(pcdev, true); /* ditto */
    *pinfo = (gx_image_enum_common_t *) penum;
    return 0;
  out3:
    gx_image_end(penum->mask_info, false);
  out2:
    gs_closedevice(mdev);
    gs_free_object(mem, mdev, "gx_begin_image3(mdev)");
  out1:
    gs_free_object(mem, penum->mask_data, "gx_begin_image3(mask_data)");
    gs_free_object(mem, penum->pixel_data, "gx_begin_image3(pixel_data)");
    gs_free_object(mem, penum, "gx_begin_image3");
    return code;
}
private bool
check_image3_extent(floatp mask_coeff, floatp data_coeff)
{
    if (mask_coeff == 0)
	return data_coeff == 0;
    if (data_coeff == 0 || (mask_coeff > 0) != (data_coeff > 0))
	return false;
    return true;
}

/*
 * Return > 0 if we want more mask now, < 0 if we want more data now,
 * 0 if we want both.
 */
private int
planes_next(const gx_image3_enum_t *penum)
{
    /*
     * The invariant we need to maintain is that we always have at least as
     * much mask as pixel data, i.e., mask_y / mask_full_height >=
     * pixel_y / pixel_full_height, or, to avoid floating point,
     * mask_y * pixel_full_height >= pixel_y * mask_full_height.
     * We know this condition is true now;
     * return a value that indicates how to maintain it.
     */
    int mask_h = penum->mask_full_height;
    int pixel_h = penum->pixel_full_height;
    long current = penum->pixel_y * (long)mask_h -
	penum->mask_y * (long)pixel_h;

#ifdef DEBUG
    if (current > 0)
	lprintf4("planes_next invariant fails: %d/%d > %d/%d\n",
		 penum->pixel_y, penum->pixel_full_height,
		 penum->mask_y, penum->mask_full_height);
#endif
    return ((current += mask_h) <= 0 ? -1 :
	    current - pixel_h <= 0 ? 0 : 1);
}

/* Process the next piece of an ImageType 3 image. */
private int
gx_image3_plane_data(gx_image_enum_common_t * info,
		     const gx_image_plane_t * planes, int height,
		     int *rows_used)
{
    gx_image3_enum_t *penum = (gx_image3_enum_t *) info;
    int pixel_height = penum->pixel_height;
    int pixel_used = 0;
    int mask_height = penum->mask_height;
    int mask_used = 0;
    int h1 = max(pixel_height - penum->pixel_y, mask_height - penum->mask_y);
    int h = min(height, h1);
    const gx_image_plane_t *pixel_planes;
    gx_image_plane_t pixel_plane, mask_plane;
    int code = 0;

    /* Initialized rows_used in case we get an error. */
    *rows_used = 0;
    switch (penum->InterleaveType) {
	case interleave_chunky:
	    if (h <= 0)
		return 0;
	    if (h > 1) {
		/* Do the operation one row at a time. */
		int h_orig = h;

		mask_plane = planes[0];
		do {
		    code = gx_image3_plane_data(info, &mask_plane, 1,
						rows_used);
		    h -= *rows_used;
		    if (code)
			break;
		    mask_plane.data += mask_plane.raster;
		} while (h);
		*rows_used = h_orig - h;
		return code;
	    } {
		/* Pull apart the source data and the mask data. */
		int bpc = penum->bpc;
		int num_components = penum->num_components;
		int width = penum->pixel_width;

		/* We do this in the simplest (not fastest) way for now. */
		uint bit_x = bpc * (num_components + 1) * planes[0].data_x;

		sample_load_declare_setup(sptr, sbit,
					  planes[0].data + (bit_x >> 3),
					  bit_x & 7, bpc);
		sample_store_declare_setup(mptr, mbit, mbbyte,
					   penum->mask_data, 0, 1);
		sample_store_declare_setup(pptr, pbit, pbbyte,
					   penum->pixel_data, 0, bpc);
		int x;

		mask_plane.data = mptr;
		mask_plane.data_x = 0;
		/* raster doesn't matter */
		pixel_plane.data = pptr;
		pixel_plane.data_x = 0;
		/* raster doesn't matter */
		pixel_planes = &pixel_plane;
		for (x = 0; x < width; ++x) {
		    uint value;
		    int i;

		    sample_load_next12(value, sptr, sbit, bpc);
		    sample_store_next12(value != 0, mptr, mbit, 1, mbbyte);
		    for (i = 0; i < num_components; ++i) {
			sample_load_next12(value, sptr, sbit, bpc);
			sample_store_next12(value, pptr, pbit, bpc, pbbyte);
		    }
		}
		sample_store_flush(mptr, mbit, 1, mbbyte);
		sample_store_flush(pptr, pbit, bpc, pbbyte);
	    }
	    break;
	case interleave_scan_lines:
	    if (planes_next(penum) >= 0) {
		/* This is mask data. */
		mask_plane = planes[0];
		pixel_planes = &pixel_plane;
		pixel_plane.data = 0;
	    } else {
		/* This is pixel data. */
		mask_plane.data = 0;
		pixel_planes = planes;
	    }
	    break;
	case interleave_separate_source:
	    /*
	     * In order to be able to recover from interruptions, we must
	     * limit separate-source processing to 1 scan line at a time.
	     */
	    if (h > 1)
		h = 1;
	    mask_plane = planes[0];
	    pixel_planes = planes + 1;
	    break;
	default:		/* not possible */
	    return_error(gs_error_rangecheck);
    }
    /*
     * Process the mask data first, so it will set up the mask
     * device for clipping the pixel data.
     */
    if (mask_plane.data) {
	/*
	 * If, on the last call, we processed some mask rows successfully
	 * but processing the pixel rows was interrupted, we set rows_used
	 * to indicate the number of pixel rows processed (since there is
	 * no way to return two rows_used values).  If this happened, some
	 * mask rows may get presented again.  We must skip over them
	 * rather than processing them again.
	 */
	int skip = penum->mask_skip;

	if (skip >= h) {
	    penum->mask_skip = skip - (mask_used = h);
	} else {
	    int mask_h = h - skip;

	    mask_plane.data += skip * mask_plane.raster;
	    penum->mask_skip = 0;
	    code = gx_image_plane_data_rows(penum->mask_info, &mask_plane,
					    mask_h, &mask_used);
	    mask_used += skip;
	}
	*rows_used = mask_used;
	penum->mask_y += mask_used;
	if (code < 0)
	    return code;
    }
    if (pixel_planes[0].data) {
	/*
	 * If necessary, flush any buffered mask data to the mask clipping
	 * device.
	 */
	gx_image_flush(penum->mask_info);
	code = gx_image_plane_data_rows(penum->pixel_info, pixel_planes, h,
					&pixel_used);
	/*
	 * There isn't any way to set rows_used if different amounts of
	 * the mask and pixel data were used.  Fake it.
	 */
	*rows_used = pixel_used;
	/*
	 * Don't return code yet: we must account for the fact that
	 * some mask data may have been processed.
	 */
	penum->pixel_y += pixel_used;
	if (code < 0) {
	    /*
	     * We must prevent the mask data from being processed again.
	     * We rely on the fact that h > 1 is only possible if the
	     * mask and pixel data have the same Y scaling.
	     */
	    if (mask_used > pixel_used) {
		int skip = mask_used - pixel_used;

		penum->mask_skip = skip;
		penum->mask_y -= skip;
		mask_used = pixel_used;
	    }
	}
    }
    if_debug5('b', "[b]image3 h=%d %smask_y=%d %spixel_y=%d\n",
	      h, (mask_plane.data ? "+" : ""), penum->mask_y,
	      (pixel_planes[0].data ? "+" : ""), penum->pixel_y);
    if (penum->mask_y >= penum->mask_height &&
	penum->pixel_y >= penum->pixel_height)
	return 1;
    if (penum->InterleaveType == interleave_scan_lines) {
	/* Update the width and depth in the enumerator. */
	if (planes_next(penum) >= 0) {  /* want mask data next */
	    penum->plane_widths[0] = penum->mask_width;
	    penum->plane_depths[0] = 1;
	} else {		/* want pixel data next */
	    penum->plane_widths[0] = penum->pixel_width;
	    penum->plane_depths[0] = penum->pixel_info->plane_depths[0];
	}
    }
    /*
     * The mask may be complete (gx_image_plane_data_rows returned 1),
     * but there may still be pixel rows to go, so don't return 1 here.
     */
    return (code < 0 ? code : 0);
}

/* Flush buffered data. */
private int
gx_image3_flush(gx_image_enum_common_t * info)
{
    gx_image3_enum_t * const penum = (gx_image3_enum_t *) info;
    int code = gx_image_flush(penum->mask_info);

    if (code >= 0)
	code = gx_image_flush(penum->pixel_info);
    return code;
}

/* Determine which data planes are wanted. */
private bool
gx_image3_planes_wanted(const gx_image_enum_common_t * info, byte *wanted)
{
    const gx_image3_enum_t * const penum = (const gx_image3_enum_t *) info;

    switch (penum->InterleaveType) {
    case interleave_chunky:	/* only 1 plane */
	wanted[0] = 0xff;
	return true;
    case interleave_scan_lines:	/* only 1 plane, but varying width/depth */
	wanted[0] = 0xff;
	return false;
    case interleave_separate_source: {
	/*
	 * We always want at least as much of the mask to be filled as the
	 * pixel data.  next > 0 iff we've processed more data than mask.
	 * Plane 0 is the mask, planes [1 .. num_planes - 1] are pixel data.
	 */
	int next = planes_next(penum);

	wanted[0] = (next >= 0 ? 0xff : 0);
	memset(wanted + 1, (next <= 0 ? 0xff : 0), info->num_planes - 1);
	/*
	 * In principle, wanted will always be true for both mask and pixel
	 * data if the full_heights are equal.  Unfortunately, even in this
	 * case, processing may be interrupted after a mask row has been
	 * passed to the underlying image processor but before the data row
	 * has been passed, in which case pixel data will be 'wanted', but
	 * not mask data, for the next call.  Therefore, we must return
	 * false.
	 */
	return false
	    /*(next == 0 &&
	      penum->mask_full_height == penum->pixel_full_height)*/;
    }
    default:			/* can't happen */
	memset(wanted, 0, info->num_planes);
	return false;
    }
}

/* Clean up after processing an ImageType 3 image. */
private int
gx_image3_end_image(gx_image_enum_common_t * info, bool draw_last)
{
    gx_image3_enum_t *penum = (gx_image3_enum_t *) info;
    gs_memory_t *mem = penum->memory;
    gx_device *mdev = penum->mdev;
    int mcode = gx_image_end(penum->mask_info, draw_last);
    gx_device *pcdev = penum->pcdev;
    int pcode = gx_image_end(penum->pixel_info, draw_last);
    int code1 = gs_closedevice(pcdev);
    int code2 = gs_closedevice(mdev);

    gs_free_object(mem, penum->mask_data,
		   "gx_image3_end_image(mask_data)");
    gs_free_object(mem, penum->pixel_data,
		   "gx_image3_end_image(pixel_data)");
    gs_free_object(mem, pcdev, "gx_image3_end_image(pcdev)");
    gs_free_object(mem, mdev, "gx_image3_end_image(mdev)");
    gs_free_object(mem, penum, "gx_image3_end_image");
    return (pcode < 0 ? pcode : mcode < 0 ? mcode : code1 < 0 ? code1 : code2);
}

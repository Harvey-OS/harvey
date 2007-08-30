    /* Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpdfd.c,v 1.71 2005/10/12 08:16:50 leonardo Exp $ */
/* Path drawing procedures for pdfwrite driver */
#include "math_.h"
#include "memory_.h"
#include "gx.h"
#include "gxdevice.h"
#include "gxfixed.h"
#include "gxistate.h"
#include "gxpaint.h"
#include "gxcoord.h"
#include "gxdevmem.h"
#include "gxcolor2.h"
#include "gxhldevc.h"
#include "gsstate.h"
#include "gserrors.h"
#include "gsptype2.h"
#include "gzpath.h"
#include "gzcpath.h"
#include "gdevpdfx.h"
#include "gdevpdfg.h"
#include "gdevpdfo.h"
#include "gsutil.h"

/* ---------------- Drawing ---------------- */

/* Fill a rectangle. */
int
gdev_pdf_fill_rectangle(gx_device * dev, int x, int y, int w, int h,
			gx_color_index color)
{
    gx_device_pdf *pdev = (gx_device_pdf *) dev;
    int code;

    /* Make a special check for the initial fill with white, */
    /* which shouldn't cause the page to be opened. */
    if (color == pdev->white && !is_in_page(pdev))
	return 0;
    code = pdf_open_page(pdev, PDF_IN_STREAM);
    if (code < 0)
	return code;
    /* Make sure we aren't being clipped. */
    code = pdf_put_clip_path(pdev, NULL);
    if (code < 0)
	return code;
    pdf_set_pure_color(pdev, color, &pdev->saved_fill_color,
		       &pdev->fill_used_process_color,
		       &psdf_set_fill_color_commands);
    if (!pdev->HaveStrokeColor)
	pdev->saved_stroke_color = pdev->saved_fill_color;
    pprintd4(pdev->strm, "%d %d %d %d re f\n", x, y, w, h);
    return 0;
}

/* ---------------- Path drawing ---------------- */

/* ------ Vector device implementation ------ */

private int
pdf_setlinewidth(gx_device_vector * vdev, floatp width)
{
    /* Acrobat Reader doesn't accept negative line widths. */
    return psdf_setlinewidth(vdev, fabs(width));
}

private int
pdf_can_handle_hl_color(gx_device_vector * vdev, const gs_imager_state * pis, 
		 const gx_drawing_color * pdc)
{
    return pis != NULL;
}

private int
pdf_setfillcolor(gx_device_vector * vdev, const gs_imager_state * pis, 
		 const gx_drawing_color * pdc)
{
    gx_device_pdf *const pdev = (gx_device_pdf *)vdev;
    bool hl_color = (*vdev_proc(vdev, can_handle_hl_color)) (vdev, pis, pdc);
    const gs_imager_state *pis_for_hl_color = (hl_color ? pis : NULL);

    if (!pdev->HaveStrokeColor) {
	/* opdfread.ps assumes same color for stroking and non-stroking operations. */
	int code = pdf_set_drawing_color(pdev, pis_for_hl_color, pdc, &pdev->saved_stroke_color,
				    &pdev->stroke_used_process_color,
				    &psdf_set_stroke_color_commands);
	if (code < 0)
	    return code;
    }
    return pdf_set_drawing_color(pdev, pis_for_hl_color, pdc, &pdev->saved_fill_color,
				 &pdev->fill_used_process_color,
				 &psdf_set_fill_color_commands);
}

private int
pdf_setstrokecolor(gx_device_vector * vdev, const gs_imager_state * pis, 
                   const gx_drawing_color * pdc)
{
    gx_device_pdf *const pdev = (gx_device_pdf *)vdev;
    bool hl_color = (*vdev_proc(vdev, can_handle_hl_color)) (vdev, pis, pdc);
    const gs_imager_state *pis_for_hl_color = (hl_color ? pis : NULL);

    if (!pdev->HaveStrokeColor) {
	/* opdfread.ps assumes same color for stroking and non-stroking operations. */
	int code = pdf_set_drawing_color(pdev, pis_for_hl_color, pdc, &pdev->saved_fill_color,
				 &pdev->fill_used_process_color,
				 &psdf_set_fill_color_commands);
	if (code < 0)
	    return code;
    }
    return pdf_set_drawing_color(pdev, pis_for_hl_color, pdc, &pdev->saved_stroke_color,
				 &pdev->stroke_used_process_color,
				 &psdf_set_stroke_color_commands);
}

private int
pdf_dorect(gx_device_vector * vdev, fixed x0, fixed y0, fixed x1, fixed y1,
	   gx_path_type_t type)
{
    gx_device_pdf *pdev = (gx_device_pdf *)vdev;
    fixed xmax = int2fixed(vdev->width), ymax = int2fixed(vdev->height);
    int bottom = (pdev->ResourcesBeforeUsage ? 1 : 0);
    fixed xmin = (pdev->sbstack_depth > bottom ? -xmax : 0);
    fixed ymin = (pdev->sbstack_depth > bottom ? -ymax : 0);

    /*
     * If we're doing a stroke operation, expand the checking box by the
     * stroke width.
     */
    if (type & gx_path_type_stroke) {
	double w = vdev->state.line_params.half_width;
	double xw = w * (fabs(vdev->state.ctm.xx) + fabs(vdev->state.ctm.yx));
	int d = float2fixed(xw) + fixed_1;

	xmin -= d;
	xmax += d;
	ymin -= d;
	ymax += d;
    }
    if (!(type & gx_path_type_clip) &&
	(x0 > xmax || x1 < xmin || y0 > ymax || y1 < ymin ||
	 x0 > x1 || y0 > y1)
	)
	return 0;		/* nothing to fill or stroke */
    /*
     * Clamp coordinates to avoid tripping over Acrobat Reader's limit
     * of 32K on user coordinate values.
     */
    if (x0 < xmin)
	x0 = xmin;
    if (x1 > xmax)
	x1 = xmax;
    if (y0 < ymin)
	y0 = ymin;
    if (y1 > ymax)
	y1 = ymax;
    return psdf_dorect(vdev, x0, y0, x1, y1, type);
}

private int
pdf_endpath(gx_device_vector * vdev, gx_path_type_t type)
{
    return 0;			/* always handled by caller */
}

const gx_device_vector_procs pdf_vector_procs = {
	/* Page management */
    NULL,
	/* Imager state */
    pdf_setlinewidth,
    psdf_setlinecap,
    psdf_setlinejoin,
    psdf_setmiterlimit,
    psdf_setdash,
    psdf_setflat,
    psdf_setlogop,
	/* Other state */
    pdf_can_handle_hl_color,
    pdf_setfillcolor,
    pdf_setstrokecolor,
	/* Paths */
    psdf_dopath,
    pdf_dorect,
    psdf_beginpath,
    psdf_moveto,
    psdf_lineto,
    psdf_curveto,
    psdf_closepath,
    pdf_endpath
};

/* ------ Utilities ------ */

/* Store a copy of clipping path. */
int
pdf_remember_clip_path(gx_device_pdf * pdev, const gx_clip_path * pcpath)
{
    /* Used for skipping redundant clip paths. SF bug #624168. */
    if (pdev->clip_path != 0) {
	gx_path_free(pdev->clip_path, "pdf clip path");
    }
    if (pcpath == 0) {
	pdev->clip_path = 0;
	return 0;
    }
    pdev->clip_path = gx_path_alloc(pdev->pdf_memory, "pdf clip path");
    if (pdev->clip_path == 0)
	return_error(gs_error_VMerror);
    return gx_cpath_to_path((gx_clip_path *)pcpath, pdev->clip_path);
}

/* Check if same clipping path. */
private int
pdf_is_same_clip_path(gx_device_pdf * pdev, const gx_clip_path * pcpath)
{
    /* Used for skipping redundant clip paths. SF bug #624168. */
    gs_cpath_enum cenum;
    gs_path_enum penum;
    gs_fixed_point vs0[3], vs1[3];
    int code, pe_op;

    if ((pdev->clip_path != 0) != (pcpath != 0))
	return 0;
    code = gx_path_enum_init(&penum, pdev->clip_path);
    if (code < 0)
	return code;
    code = gx_cpath_enum_init(&cenum, (gx_clip_path *)pcpath);
    if (code < 0)
	return code;
    while ((code = gx_cpath_enum_next(&cenum, vs0)) > 0) {
	pe_op = gx_path_enum_next(&penum, vs1);
	if (pe_op < 0)
	    return pe_op;
	if (pe_op != code)
	    return 0;
	switch (pe_op) {
	    case gs_pe_curveto:
		if (vs0[1].x != vs1[1].x || vs0[1].y != vs1[1].y ||
		    vs0[2].x != vs1[2].x || vs0[2].y != vs1[2].y)
		    return 0;
	    case gs_pe_moveto:
	    case gs_pe_lineto:
		if (vs0[0].x != vs1[0].x || vs0[0].y != vs1[0].y)
		    return 0;
	}
    }
    if (code < 0)
	return code;
    code = gx_path_enum_next(&penum, vs1);
    if (code < 0)
	return code;
    return (code == 0);
}

/* Test whether we will need to put the clipping path. */
bool
pdf_must_put_clip_path(gx_device_pdf * pdev, const gx_clip_path * pcpath)
{
    if (pcpath == NULL) {
	if (pdev->clip_path_id == pdev->no_clip_path_id)
	    return false;
    } else { 
	if (pdev->clip_path_id == pcpath->id)
	    return false;
	if (gx_cpath_includes_rectangle(pcpath, fixed_0, fixed_0,
					int2fixed(pdev->width),
					int2fixed(pdev->height)))
	    if (pdev->clip_path_id == pdev->no_clip_path_id)
		return false;
	if (pdf_is_same_clip_path(pdev, pcpath) > 0) {
	    pdev->clip_path_id = pcpath->id;
	    return false;
	}
    }
    return true;
}

/* Put a single element of a clipping path list. */
private int
pdf_put_clip_path_list_elem(gx_device_pdf * pdev, gx_cpath_path_list *e, 
	gs_path_enum *cenum, gdev_vector_dopath_state_t *state,
	gs_fixed_point vs[3])
{   /* This recursive function provides a reverse order of the list elements. */
    int pe_op;

    if (e->next != NULL) {
	int code = pdf_put_clip_path_list_elem(pdev, e->next, cenum, state, vs);

	if (code != 0)
	    return code;
    }
    gx_path_enum_init(cenum, &e->path);
    while ((pe_op = gx_path_enum_next(cenum, vs)) > 0)
	gdev_vector_dopath_segment(state, pe_op, vs);
    pprints1(pdev->strm, "%s n\n", (e->rule <= 0 ? "W" : "W*"));
    if (pe_op < 0)
	return pe_op;
    return 0;
}

/* Put a clipping path on the output file. */
int
pdf_put_clip_path(gx_device_pdf * pdev, const gx_clip_path * pcpath)
{
    int code;
    stream *s = pdev->strm;
    gs_id new_id;

    /* Check for no update needed. */
    if (pcpath == NULL) {
	if (pdev->clip_path_id == pdev->no_clip_path_id)
	    return 0;
	new_id = pdev->no_clip_path_id;
    } else {
	if (pdev->clip_path_id == pcpath->id)
	    return 0;
	new_id = pcpath->id;
	if (gx_cpath_includes_rectangle(pcpath, fixed_0, fixed_0,
					int2fixed(pdev->width),
					int2fixed(pdev->height))
	    ) {
	    if (pdev->clip_path_id == pdev->no_clip_path_id)
		return 0;
	    new_id = pdev->no_clip_path_id;
	}
	code = pdf_is_same_clip_path(pdev, pcpath);
	if (code < 0)
	    return code;
	if (code) {
	    pdev->clip_path_id = new_id;
	    return 0;
	}
    }
    /*
     * The contents must be open already, so the following will only exit
     * text or string context.
     */
    code = pdf_open_contents(pdev, PDF_IN_STREAM);
    if (code < 0)
	return code;
    /* Use Q to unwind the old clipping path. */
    if (pdev->vgstack_depth > pdev->vgstack_bottom) {
	code = pdf_restore_viewer_state(pdev, s);
	if (code < 0)
	    return code;
    }
    if (new_id != pdev->no_clip_path_id) {
	gdev_vector_dopath_state_t state;
	gs_fixed_point vs[3];
	int pe_op;

	/* Use q to allow the new clipping path to unwind.  */
	code = pdf_save_viewer_state(pdev, s);
	if (code < 0)
	    return code;
	gdev_vector_dopath_init(&state, (gx_device_vector *)pdev,
				gx_path_type_fill, NULL);
	if (pcpath->path_list == NULL) {
	    gs_cpath_enum cenum;

	/*
	 * We have to break 'const' here because the clip path
	 * enumeration logic uses some internal mark bits.
	 * This is very unfortunate, but until we can come up with
	 * a better algorithm, it's necessary.
	 */
	gx_cpath_enum_init(&cenum, (gx_clip_path *) pcpath);
	while ((pe_op = gx_cpath_enum_next(&cenum, vs)) > 0)
	    gdev_vector_dopath_segment(&state, pe_op, vs);
	pprints1(s, "%s n\n", (pcpath->rule <= 0 ? "W" : "W*"));
	if (pe_op < 0)
	    return pe_op;
	} else {
	    gs_path_enum cenum;

	    code = pdf_put_clip_path_list_elem(pdev, pcpath->path_list, &cenum, &state, vs);
	    if (code < 0)
		return code;
	}
    }
    pdev->clip_path_id = new_id;
    return pdf_remember_clip_path(pdev, 
	    (pdev->clip_path_id == pdev->no_clip_path_id ? NULL : pcpath));
}

/*
 * Compute the scaling to ensure that user coordinates for a path are within
 * Acrobat's range.  Return true if scaling was needed.  In this case, the
 * CTM will be multiplied by *pscale, and all coordinates will be divided by
 * *pscale.
 */
private bool
make_rect_scaling(const gx_device_pdf *pdev, const gs_fixed_rect *bbox,
		  floatp prescale, double *pscale)
{
    double bmin, bmax;

    bmin = min(bbox->p.x / pdev->scale.x, bbox->p.y / pdev->scale.y) * prescale;
    bmax = max(bbox->q.x / pdev->scale.x, bbox->q.y / pdev->scale.y) * prescale;
    if (bmin <= int2fixed(-MAX_USER_COORD) ||
	bmax > int2fixed(MAX_USER_COORD)
	) {
	/* Rescale the path. */
	*pscale = max(bmin / int2fixed(-MAX_USER_COORD),
		      bmax / int2fixed(MAX_USER_COORD));
	return true;
    } else {
	*pscale = 1;
	return false;
    }
}

/*
 * Prepare a fill with a color anc a clipping path.
 * Return 1 if there is nothing to paint.
 */
private int
prepare_fill_with_clip(gx_device_pdf *pdev, const gs_imager_state * pis,
	      gs_fixed_rect *box, bool have_path,
	      const gx_drawing_color * pdcolor, const gx_clip_path * pcpath)
{
    bool new_clip;
    int code;

    /*
     * Check for an empty clipping path.
     */
    if (pcpath) {
	gx_cpath_outer_box(pcpath, box);
	if (box->p.x >= box->q.x || box->p.y >= box->q.y)
	    return 1;		/* empty clipping path */
    }
    if (gx_dc_is_pure(pdcolor)) {
	/*
	 * Make a special check for the initial fill with white,
	 * which shouldn't cause the page to be opened.
	 */
	if (gx_dc_pure_color(pdcolor) == pdev->white && 
		!is_in_page(pdev) && pdev->sbstack_depth == 0)
	    return 1;
    }
    new_clip = pdf_must_put_clip_path(pdev, pcpath);
    if (have_path || pdev->context == PDF_IN_NONE || new_clip) {
	if (new_clip)
	    code = pdf_unclip(pdev);
	else
	    code = pdf_open_page(pdev, PDF_IN_STREAM);
	if (code < 0)
	    return code;
    }
    code = pdf_prepare_fill(pdev, pis);
    if (code < 0)
	return code;
    return pdf_put_clip_path(pdev, pcpath);
}

/* -------------A local image converter device. -----------------------------*/

public_st_pdf_lcvd_t();

private int 
lcvd_fill_rectangle_shifted(gx_device *dev, int x, int y, int width, int height, gx_color_index color)
{
    pdf_lcvd_t *cvd = (pdf_lcvd_t *)dev;

    return cvd->std_fill_rectangle((gx_device *)&cvd->mdev, 
	x - cvd->mdev.mapped_x, y - cvd->mdev.mapped_y, width, height, color);
}
private int 
lcvd_fill_rectangle_shifted2(gx_device *dev, int x, int y, int width, int height, gx_color_index color)
{
    pdf_lcvd_t *cvd = (pdf_lcvd_t *)dev;
    int code;

    code = (*dev_proc(cvd->mask, fill_rectangle))((gx_device *)cvd->mask, 
	x - cvd->mdev.mapped_x, y - cvd->mdev.mapped_y, width, height, (gx_color_index)1);
    if (code < 0)
	return code;
    return cvd->std_fill_rectangle((gx_device *)&cvd->mdev, 
	x - cvd->mdev.mapped_x, y - cvd->mdev.mapped_y, width, height, color);
}
private int 
lcvd_fill_rectangle_shifted_from_mdev(gx_device *dev, int x, int y, int width, int height, gx_color_index color)
{
    pdf_lcvd_t *cvd = (pdf_lcvd_t *)dev;

    return cvd->std_fill_rectangle((gx_device *)&cvd->mdev, 
	x - cvd->mdev.mapped_x, y - cvd->mdev.mapped_y, width, height, color);
}
private void 
lcvd_get_clipping_box_from_target(gx_device *dev, gs_fixed_rect *pbox)
{
    gx_device_memory *mdev = (gx_device_memory *)dev;

    (*dev_proc(mdev->target, get_clipping_box))(mdev->target, pbox);
}
private int 
lcvd_pattern_manage(gx_device *pdev1, gx_bitmap_id id,
		gs_pattern1_instance_t *pinst, pattern_manage_t function)
{
    if (function == pattern_manage__shading_area)
	return 1; /* Request shading area. */
    return 0;
}
private int 
lcvd_close_device_with_writing(gx_device *pdev)
{
    /* Assuming 'mdev' is being closed before 'mask' - see gx_image3_end_image. */
    pdf_lcvd_t *cvd = (pdf_lcvd_t *)pdev;
    int code, code1;

    code = pdf_dump_converted_image(cvd->pdev, cvd);
    code1 = cvd->std_close_device((gx_device *)&cvd->mdev);
    return code < 0 ? code : code1;
}

private int
write_image(gx_device_pdf *pdev, gx_device_memory *mdev, gs_matrix *m)
{
    gs_image_t image;
    pdf_image_writer writer;
    const int sourcex = 0;
    int code;

    if (m != NULL)
	pdf_put_matrix(pdev, NULL, m, " cm\n");
    code = pdf_copy_color_data(pdev, mdev->base, sourcex,  
		mdev->raster, gx_no_bitmap_id, 0, 0, mdev->width, mdev->height,
		&image, &writer, 2);
    if (code == 1)
	code = 0; /* Empty image. */
    else if (code == 0)
	code = pdf_do_image(pdev, writer.pres, NULL, true);
    return code;
}
private int
write_mask(gx_device_pdf *pdev, gx_device_memory *mdev, gs_matrix *m)
{
    const int sourcex = 0;
    gs_id save_clip_id = pdev->clip_path_id;
    bool save_skip_color = pdev->skip_colors;
    int code;

    if (m != NULL)
	pdf_put_matrix(pdev, NULL, m, " cm\n");
    pdev->clip_path_id = pdev->no_clip_path_id;
    pdev->skip_colors = true;
    code = gdev_pdf_copy_mono((gx_device *)pdev, mdev->base, sourcex,  
		mdev->raster, gx_no_bitmap_id, 0, 0, mdev->width, mdev->height,
		gx_no_color_index, (gx_color_index)0);
    pdev->clip_path_id = save_clip_id;
    pdev->skip_colors = save_skip_color; 
    return code;
}

private void
max_subimage_width(int width, byte *base, int x0, long count1, int *x1, long *count)
{
    long c = 0, c1 = count1 - 1;
    int x = x0;
    byte p = 1; /* The inverse of the previous bit. */
    byte r;     /* The inverse of the current  bit. */
    byte *q = base + (x / 8), m = 0x80 >> (x % 8);

    for (; x < width; x++) {
	r = !(*q & m);
	if (p != r) {
	    if (c >= c1) {
		if (!r)
		    goto ex; /* stop before the upgrade. */
	    }
	    c++;
	}
	p = r;
	m >>= 1;
	if (!m) {
	    m = 0x80;
	    q++;
	}
    }
    if (p)
	c++; /* Account the last downgrade. */
ex:
    *count = c;
    *x1 = x;
}

private void
compute_subimage(int width, int height, int raster, byte *base, 
	    	 int x0, int y0, long MaxClipPathSize, int *x1, int *y1)
{
    /* Returns a semiopen range : [x0:x1)*[y0:y1). */
    if (x0 != 0) {
	long count;

	/* A partial single scanline. */
	max_subimage_width(width, base + y0 * raster, x0, MaxClipPathSize / 4, x1, &count);
	*y1 = y0;
    } else {
	int xx, y = y0, yy;
	long count, count1 = MaxClipPathSize / 4;

	for(; y < height && count1 > 0; ) {
	    max_subimage_width(width, base + y * raster, 0, count1, &xx, &count);
	    if (xx < width) {
		if (y == y0) {
		    /* Partial single scanline. */
		    *y1 = y + 1;
		    *x1 = xx;
		    return;
		} else {
		    /* Full lines before this scanline. */
		    break;
		}
	    }
	    count1 -= count;
	    yy = y + 1;
	    for (; yy < height; yy++)
		if (memcmp(base + raster * y, base + raster * yy, raster))
		    break;
	    y = yy;

	}
	*y1 = y;
	*x1 = width;
    }
}

private int
image_line_to_clip(gx_device_pdf *pdev, byte *base, int x0, int x1, int y0, int y1, bool started)
{   /* returns the number of segments or error code. */
    int x = x0, xx;
    byte *q = base + (x / 8), m = 0x80 >> (x % 8);
    long c = 0;

    for (;;) {
	/* Look for upgrade : */
	for (; x < x1; x++) {
	    if (*q & m)
		break;
	    m >>= 1;
	    if (!m) {
		m = 0x80;
		q++;
	    }
	}
	if (x == x1)
	    return c;
	xx = x;
	/* Look for downgrade : */
	for (; x < x1; x++) {
	    if (!(*q & m))
		break;
	    m >>= 1;
	    if (!m) {
		m = 0x80;
		q++;
	    }
	}
	/* Found the interval [xx:x). */
	if (!started) {
	    stream_puts(pdev->strm, "n\n");
	    started = true;
	}
	pprintld2(pdev->strm, "%ld %ld m ", xx, y0);
	pprintld2(pdev->strm, "%ld %ld l ", x, y0);
	pprintld2(pdev->strm, "%ld %ld l ", x, y1);
	pprintld2(pdev->strm, "%ld %ld l h\n", xx, y1);
	c += 4;
    }
}

private int
mask_to_clip(gx_device_pdf *pdev, int width, int height, 
	     int raster, byte *base, int x0, int y0, int x1, int y1)
{
    int y, yy, code = 0;
    bool has_segments = false;

    for (y = y0; y < y1 && code >= 0;) {
	yy = y + 1;
	if (x0 == 0) {
	for (; yy < y1; yy++)
	    if (memcmp(base + raster * y, base + raster * yy, raster))
		break;
	}
	code = image_line_to_clip(pdev, base + raster * y, x0, x1, y, yy, has_segments);
	if (code > 0)
	    has_segments = true;
	y = yy;
    }
    if (has_segments)
	stream_puts(pdev->strm, "W n\n");
    return code < 0 ? code : has_segments ? 1 : 0;
}

private int
write_subimage(gx_device_pdf *pdev, gx_device_memory *mdev, int x, int y, int x1, int y1)
{
    gs_image_t image;
    pdf_image_writer writer;
    /* expand in 1 pixel to provide a proper color interpolation */
    int X = max(0, x - 1);
    int Y = max(0, y - 1);
    int X1 = min(mdev->width, x1 + 1);
    int Y1 = min(mdev->height, y1 + 1);
    int code;

    code = pdf_copy_color_data(pdev, mdev->base + mdev->raster * Y, X,  
		mdev->raster, gx_no_bitmap_id, 
		X, Y, X1 - X, Y1 - Y,
		&image, &writer, 2);
    if (code < 0)
	return code;
    if (!writer.pres)
	return 0; /* inline image. */
    return pdf_do_image(pdev, writer.pres, NULL, true);
}

private int 
write_image_with_clip(gx_device_pdf *pdev, pdf_lcvd_t *cvd)
{
    int x = 0, y = 0;
    int code, code1;

    if (cvd->write_matrix)
	pdf_put_matrix(pdev, NULL, &cvd->m, " cm\n");
    for(;;) {
	int x1, y1;
	
	compute_subimage(cvd->mask->width, cvd->mask->height, 
			 cvd->mask->raster, cvd->mask->base, 
			 x, y, max(pdev->MaxClipPathSize, 100), &x1, &y1);
	code = mask_to_clip(pdev, 
			 cvd->mask->width, cvd->mask->height, 
			 cvd->mask->raster, cvd->mask->base,
			 x, y, x1, y1);
	if (code < 0)
	    return code;
	if (code > 0) {
	    code1 = write_subimage(pdev, &cvd->mdev, x, y, x1, y1);
	    if (code1 < 0)
		return code1;
	}
	if (x1 >= cvd->mdev.width && y1 >= cvd->mdev.height)
	    break;
	if (code > 0)
	    stream_puts(pdev->strm, "Q q\n");
	if (x1 == cvd->mask->width) {
	    x = 0;
	    y = y1;
	} else {
	    x = x1;
	    y = y1;
	}
    }
    return 0;
}

int
pdf_dump_converted_image(gx_device_pdf *pdev, pdf_lcvd_t *cvd)
{
    int code = 0;

    if (!cvd->path_is_empty || cvd->has_background) {
	if (!cvd->has_background)
	    stream_puts(pdev->strm, "W n\n");
	code = write_image(pdev, &cvd->mdev, (cvd->write_matrix ? &cvd->m : NULL));
	cvd->path_is_empty = true;
    } else if (!cvd->mask_is_empty && pdev->PatternImagemask) {
	/* Convert to imagemask with a pattern color. */
	/* See also use_image_as_pattern in gdevpdfi.c . */
	gs_imager_state s;
	gs_pattern1_instance_t inst;
	gs_id id = gs_next_ids(cvd->mdev.memory, 1);
	cos_value_t v;
	const pdf_resource_t *pres;

	memset(&s, 0, sizeof(s));
	s.ctm.xx = cvd->m.xx;
	s.ctm.xy = cvd->m.xy;
	s.ctm.yx = cvd->m.yx;
	s.ctm.yy = cvd->m.yy;
	s.ctm.tx = cvd->m.tx;
	s.ctm.ty = cvd->m.ty;
	memset(&inst, 0, sizeof(inst));
	inst.saved = (gs_state *)&s; /* HACK : will use s.ctm only. */
	inst.template.PaintType = 1;
	inst.template.TilingType = 1;
	inst.template.BBox.p.x = inst.template.BBox.p.y = 0;
	inst.template.BBox.q.x = cvd->mdev.width;
	inst.template.BBox.q.y = cvd->mdev.height;
	inst.template.XStep = (float)cvd->mdev.width;
	inst.template.YStep = (float)cvd->mdev.height;
	code = (*dev_proc(pdev, pattern_manage))((gx_device *)pdev, 
	    id, &inst, pattern_manage__start_accum);
	if (code >= 0) {
	    stream_puts(pdev->strm, "W n\n");
	    code = write_image(pdev, &cvd->mdev, NULL);
	}
	pres = pdev->accumulating_substream_resource;
	if (code >= 0)
	    code = (*dev_proc(pdev, pattern_manage))((gx_device *)pdev, 
		id, &inst, pattern_manage__finish_accum);
	if (code >= 0)
	    code = (*dev_proc(pdev, pattern_manage))((gx_device *)pdev, 
		id, &inst, pattern_manage__load);
	if (code >= 0)
	    code = pdf_cs_Pattern_colored(pdev, &v);
	if (code >= 0) {
	    cos_value_write(&v, pdev);
	    pprintld1(pdev->strm, " cs /R%ld scn ", pdf_resource_id(pres));
	}
	if (code >= 0)
	    code = write_mask(pdev, cvd->mask, (cvd->write_matrix ? &cvd->m : NULL));
	cvd->mask_is_empty = true;
    } else if (!cvd->mask_is_empty && !pdev->PatternImagemask) {
	/* Convert to image with a clipping path. */
	stream_puts(pdev->strm, "q\n");
	code = write_image_with_clip(pdev, cvd);
	stream_puts(pdev->strm, "Q\n");
    }
    if (code > 0)
	code = (*dev_proc(&cvd->mdev, fill_rectangle))((gx_device *)&cvd->mdev, 
		0, 0, cvd->mdev.width, cvd->mdev.height, (gx_color_index)0);
    return code;
}
private int
lcvd_handle_fill_path_as_shading_coverage(gx_device *dev,
    const gs_imager_state *pis, gx_path *ppath,
    const gx_fill_params *params,
    const gx_drawing_color *pdcolor, const gx_clip_path *pcpath)
{
    pdf_lcvd_t *cvd = (pdf_lcvd_t *)dev;
    gx_device_pdf *pdev = (gx_device_pdf *)cvd->mdev.target;
    int code;

    if (cvd->has_background)
	return 0;
    if (gx_path_is_null(ppath)) {
	/* use the mask. */
	if (!cvd->path_is_empty) {
	    code = pdf_dump_converted_image(pdev, cvd);
	    if (code < 0)
		return code;
	    stream_puts(pdev->strm, "Q q\n");
	    dev_proc(&cvd->mdev, fill_rectangle) = lcvd_fill_rectangle_shifted2;
	}
	if (!cvd->mask_is_clean || !cvd->path_is_empty) {
	    code = (*dev_proc(cvd->mask, fill_rectangle))((gx_device *)cvd->mask, 
			0, 0, cvd->mask->width, cvd->mask->height, (gx_color_index)0);
	    if (code < 0)
		return code;
	    cvd->mask_is_clean = true;
	}
	cvd->path_is_empty = true;
	cvd->mask_is_empty = false;
    } else {
	gs_matrix m;

	gs_make_translation(cvd->path_offset.x, cvd->path_offset.y, &m);
	/* use the clipping. */
	if (!cvd->mask_is_empty) {
	    code = pdf_dump_converted_image(pdev, cvd);
	    if (code < 0)
		return code;
	    stream_puts(pdev->strm, "Q q\n");
	    dev_proc(&cvd->mdev, fill_rectangle) = lcvd_fill_rectangle_shifted;
	    cvd->mask_is_empty = true;
	}
	code = gdev_vector_dopath((gx_device_vector *)pdev, ppath,
			    gx_path_type_fill | gx_path_type_optimize, &m);
	if (code < 0)
	    return code;
	stream_puts(pdev->strm, "h\n");
	cvd->path_is_empty = false;
    }
    return 0;
}

int
pdf_setup_masked_image_converter(gx_device_pdf *pdev, gs_memory_t *mem, const gs_matrix *m, pdf_lcvd_t **pcvd, 
				 bool need_mask, int x, int y, int w, int h, bool write_on_close)
{
    int code;
    gx_device_memory *mask = 0;
    pdf_lcvd_t *cvd = *pcvd;

    if (cvd == NULL) {
	cvd = gs_alloc_struct(mem, pdf_lcvd_t, &st_pdf_lcvd_t, "pdf_setup_masked_image_converter");
	if (cvd == NULL)
	    return_error(gs_error_VMerror);
	*pcvd = cvd;
    }
    cvd->pdev = pdev;
    gs_make_mem_device(&cvd->mdev, gdev_mem_device_for_bits(pdev->color_info.depth),
		mem, 0, (gx_device *)pdev);
    cvd->mdev.width  = w;
    cvd->mdev.height = h;
    cvd->mdev.mapped_x = x;
    cvd->mdev.mapped_y = y;
    cvd->mdev.bitmap_memory = mem;
    cvd->mdev.color_info = pdev->color_info;
    cvd->path_is_empty = true;
    cvd->mask_is_empty = true;
    cvd->mask_is_clean = false;
    cvd->has_background = false;
    cvd->mask = 0;
    cvd->write_matrix = true;
    code = (*dev_proc(&cvd->mdev, open_device))((gx_device *)&cvd->mdev);
    if (code < 0)
	return code;
    code = (*dev_proc(&cvd->mdev, fill_rectangle))((gx_device *)&cvd->mdev, 
		0, 0, cvd->mdev.width, cvd->mdev.height, (gx_color_index)0);
    if (code < 0)
	return code;
    if (need_mask) {
	mask = gs_alloc_struct(mem, gx_device_memory, &st_device_memory, "pdf_setup_masked_image_converter");
	if (mask == NULL)
	    return_error(gs_error_VMerror);
	cvd->mask = mask;
	gs_make_mem_mono_device(mask, mem, (gx_device *)pdev);
	mask->width = cvd->mdev.width;
	mask->height = cvd->mdev.height;
	mask->bitmap_memory = mem;
	code = (*dev_proc(mask, open_device))((gx_device *)mask);
	if (code < 0)
	    return code;
	if (write_on_close) {
	    code = (*dev_proc(mask, fill_rectangle))((gx_device *)mask, 
			0, 0, mask->width, mask->height, (gx_color_index)0);
	    if (code < 0)
		return code;
	}
    }
    cvd->std_fill_rectangle = dev_proc(&cvd->mdev, fill_rectangle);
    cvd->std_close_device = dev_proc(&cvd->mdev, close_device);
    if (!write_on_close) {
	/* Type 3 images will write to the mask directly. */
	dev_proc(&cvd->mdev, fill_rectangle) = (need_mask ? lcvd_fill_rectangle_shifted2 
							  : lcvd_fill_rectangle_shifted);
    } else
	dev_proc(&cvd->mdev, fill_rectangle) = lcvd_fill_rectangle_shifted_from_mdev;
    dev_proc(&cvd->mdev, get_clipping_box) = lcvd_get_clipping_box_from_target;
    dev_proc(&cvd->mdev, pattern_manage) = lcvd_pattern_manage;
    dev_proc(&cvd->mdev, fill_path) = lcvd_handle_fill_path_as_shading_coverage;
    cvd->m = *m;
    if (write_on_close) {
	cvd->mdev.is_open = true;
	mask->is_open = true;
	dev_proc(&cvd->mdev, close_device) = lcvd_close_device_with_writing;
    }
    return 0;
}

void
pdf_remove_masked_image_converter(gx_device_pdf *pdev, pdf_lcvd_t *cvd, bool need_mask)
{
    (*dev_proc(&cvd->mdev, close_device))((gx_device *)&cvd->mdev);
    if (cvd->mask) {
	(*dev_proc(cvd->mask, close_device))((gx_device *)cvd->mask);
	gs_free_object(cvd->mask->memory, cvd->mask, "pdf_remove_masked_image_converter");
    }
}

private int
path_scale(gx_path *path, double scalex, double scaley)
{
    segment *pseg = (segment *)path->first_subpath;

    for (;pseg != NULL; pseg = pseg->next) {
	pseg->pt.x = (fixed)floor(pseg->pt.x * scalex + 0.5);
	pseg->pt.y = (fixed)floor(pseg->pt.y * scaley + 0.5);
	if (pseg->type == s_curve) {
	    curve_segment *s = (curve_segment *)pseg;

	    s->p1.x = (fixed)floor(s->p1.x * scalex + 0.5);
	    s->p1.y = (fixed)floor(s->p1.y * scaley + 0.5);
	    s->p2.x = (fixed)floor(s->p2.x * scalex + 0.5);
	    s->p2.y = (fixed)floor(s->p2.y * scaley + 0.5);
	}
    }
    path->position.x = (fixed)floor(path->position.x * scalex + 0.5);
    path->position.y = (fixed)floor(path->position.y * scaley + 0.5);
    return 0;
}

/* ------ Driver procedures ------ */

/* Fill a path. */
int
gdev_pdf_fill_path(gx_device * dev, const gs_imager_state * pis, gx_path * ppath,
		   const gx_fill_params * params,
	      const gx_drawing_color * pdcolor, const gx_clip_path * pcpath)
{
    gx_device_pdf *pdev = (gx_device_pdf *) dev;
    int code;
    /*
     * HACK: we fill an empty path in order to set the clipping path
     * and the color for writing text.  If it weren't for this, we
     * could detect and skip empty paths before putting out the clip
     * path or the color.  We also clip with an empty path in order
     * to advance currentpoint for show operations without actually
     * drawing anything.
     */
    bool have_path;
    gs_fixed_rect box = {{0, 0}, {0, 0}};

    have_path = !gx_path_is_void(ppath);
    if (!have_path && !pdev->vg_initial_set) {
	/* See lib/gs_pdfwr.ps about "initial graphic state". */
	pdf_prepare_initial_viewer_state(pdev, pis);
	pdf_reset_graphics(pdev);
	return 0;
    }

    code = prepare_fill_with_clip(pdev, pis, &box, have_path, pdcolor, pcpath);
    if (code == gs_error_rangecheck) {
	/* Fallback to the default implermentation for handling 
	   a transparency with CompatibilityLevel<=1.3 . */
	return gx_default_fill_path((gx_device *)pdev, pis, ppath, params, pdcolor, pcpath);
    }
    if (code < 0)
	return code;
    if (code == 1)
	return 0; /* Nothing to paint. */
    code = pdf_setfillcolor((gx_device_vector *)pdev, pis, pdcolor);
    if (code == gs_error_rangecheck) {
	const bool convert_to_image = (pdev->CompatibilityLevel <= 1.2 && 
		gx_dc_is_pattern2_color(pdcolor));

	if (!convert_to_image) {
	    /* Fallback to the default implermentation for handling 
	    a shading with CompatibilityLevel<=1.2 . */
	    return gx_default_fill_path(dev, pis, ppath, params, pdcolor, pcpath);
	} else {
	    /* Convert a shading into a bitmap
	       with CompatibilityLevel<=1.2 . */
	    pdf_lcvd_t cvd, *pcvd = &cvd;
	    int sx, sy;
	    gs_fixed_rect bbox, bbox1;
	    bool need_mask = gx_dc_pattern2_can_overlap(pdcolor);
	    gs_matrix m, save_ctm = ctm_only(pis), ms, msi, mm;
	    gs_int_point rect_size;
	    /* double scalex = 1.9, scaley = 1.4; debug purpose only. */
	    double scale, scalex = 1.0, scaley = 1.0;
	    gx_drawing_color dc = *pdcolor;
	    gs_pattern2_instance_t pi = *(gs_pattern2_instance_t *)dc.ccolor.pattern;
	    gs_state *pgs = gs_state_copy(pi.saved, gs_state_memory(pi.saved));

	    if (pgs == NULL)
		return_error(gs_error_VMerror);
	    dc.ccolor.pattern = (gs_pattern_instance_t *)&pi;
	    pi.saved = pgs;
	    code = gx_path_bbox(ppath, &bbox);
	    if (code < 0)
		return code;
	    rect_intersect(bbox, box);
	    code = gx_dc_pattern2_get_bbox(pdcolor, &bbox1);
	    if (code < 0)
		return code;
	    if (code)
		rect_intersect(bbox, bbox1);
	    if (bbox.p.x >= bbox.q.x || bbox.p.y >= bbox.q.y)
		return 0;
	    sx = fixed2int(bbox.p.x);
	    sy = fixed2int(bbox.p.y);
	    gs_make_identity(&m);
	    rect_size.x = fixed2int(bbox.q.x + fixed_half) - sx;
	    rect_size.y = fixed2int(bbox.q.y + fixed_half) - sy;
	    if (rect_size.x == 0 || rect_size.y == 0)
		return 0;
	    m.tx = (float)sx;
	    m.ty = (float)sy;
	    cvd.path_offset.x = sx;
	    cvd.path_offset.y = sy;
	    scale = (double)rect_size.x * rect_size.y * pdev->color_info.num_components /
		    pdev->MaxShadingBitmapSize;
	    if (scale > 1) {
		/* This section (together with the call to 'path_scale' below)
		   sets up a downscaling when converting the shading into bitmap. 
		   We used floating point numbers to debug it, but in production
		   we prefer to deal only with integers being powers of 2
		   in order to avoid possible distorsions when scaling paths.
		*/
		scalex = ceil(sqrt(scale));
		scalex = scaley = 1 << ilog2((int)scalex);
		if (scalex * scaley < scale)
		    scalex *= 2;
		if (scalex * scaley < scale)
		    scaley *= 2;
		rect_size.x = (int)floor(rect_size.x / scalex + 0.5);
		rect_size.y = (int)floor(rect_size.y / scaley + 0.5);
		gs_make_scaling(1.0 / scalex, 1.0 / scaley, &ms);
		gs_make_scaling(scalex, scaley, &msi);
		gs_matrix_multiply(&msi, &m, &m);
		gs_matrix_multiply(&ctm_only(pis), &ms, &mm);
		gs_setmatrix((gs_state *)pis, &mm);
		gs_matrix_multiply(&ctm_only((gs_imager_state *)pgs), &ms, &mm);
		gs_setmatrix((gs_state *)pgs, &mm);
		sx = fixed2int(bbox.p.x / (int)scalex);
		sy = fixed2int(bbox.p.y / (int)scaley);
		cvd.path_offset.x = sx; /* m.tx / scalex */
		cvd.path_offset.y = sy;
	    }
	    code = pdf_setup_masked_image_converter(pdev, pdev->memory, &m, &pcvd, need_mask, sx, sy, 
			    rect_size.x, rect_size.y, false);
	    pcvd->has_background = gx_dc_pattern2_has_background(pdcolor);
	    stream_puts(pdev->strm, "q\n");
	    if (code >= 0) {
		code = gdev_vector_dopath((gx_device_vector *)pdev, ppath,
					gx_path_type_clip, NULL);
		if (code >= 0)
		    stream_puts(pdev->strm, "W n\n");
	    }
	    pdf_put_matrix(pdev, NULL, &cvd.m, " cm q\n");
	    cvd.write_matrix = false;
	    if (code >= 0) {
		/* See gx_default_fill_path. */
		gx_clip_path cpath_intersection;
		gx_path path_intersection, path1, *p = &path_intersection;

		gx_path_init_local(&path_intersection, pdev->memory);
		gx_path_init_local(&path1, pdev->memory);
		gx_cpath_init_local_shared(&cpath_intersection, pcpath, pdev->memory);
		if ((code = gx_cpath_intersect(&cpath_intersection, ppath, params->rule, (gs_imager_state *)pis)) >= 0)
		    code = gx_cpath_to_path(&cpath_intersection, &path_intersection);
		if (code >= 0 && scale > 1) {
		    code = gx_path_copy(&path_intersection, &path1);	
		    if (code > 0) {
			p = &path1;
			code = path_scale(&path1, scalex, scaley);
		    }
		}
		if (code >= 0)
		    code = gx_dc_pattern2_fill_path(&dc, p, NULL, (gx_device *)&cvd.mdev);
		gx_path_free(&path_intersection, "gdev_pdf_fill_path");
		gx_path_free(&path1, "gdev_pdf_fill_path");
		gx_cpath_free(&cpath_intersection, "gdev_pdf_fill_path");
	    }
	    if (code >= 0) {
		code = pdf_dump_converted_image(pdev, &cvd);
	    }
	    stream_puts(pdev->strm, "Q Q\n");
	    pdf_remove_masked_image_converter(pdev, &cvd, need_mask);
	    gs_setmatrix((gs_state *)pis, &save_ctm);
	    gs_state_free(pgs);
	    return code; 
	}
    }
    if (code < 0)
	return code;
    if (have_path) {
	stream *s = pdev->strm;
	double scale;
	gs_matrix smat;
	gs_matrix *psmat = NULL;
	gs_fixed_rect box1;

	code = gx_path_bbox(ppath, &box1);
	if (code < 0)
	    return code;
	if (pcpath) {
 	    rect_intersect(box1, box);
 	    if (box1.p.x > box1.q.x || box1.p.y > box1.q.y)
  		return 0;		/* outside the clipping path */
	}
	if (params->flatness != pdev->state.flatness) {
	    pprintg1(s, "%g i\n", params->flatness);
	    pdev->state.flatness = params->flatness;
	}
	if (make_rect_scaling(pdev, &box1, 1.0, &scale)) {
	    gs_make_scaling(pdev->scale.x * scale, pdev->scale.y * scale,
			    &smat);
            pdf_put_matrix(pdev, "q ", &smat, "cm\n");
	    psmat = &smat;
	}
	gdev_vector_dopath((gx_device_vector *)pdev, ppath,
			   gx_path_type_fill | gx_path_type_optimize,
			   psmat);
	stream_puts(s, (params->rule < 0 ? "f\n" : "f*\n"));
	if (psmat)
	    stream_puts(s, "Q\n");
    }
    return 0;
}

/* Stroke a path. */
int
gdev_pdf_stroke_path(gx_device * dev, const gs_imager_state * pis,
		     gx_path * ppath, const gx_stroke_params * params,
	      const gx_drawing_color * pdcolor, const gx_clip_path * pcpath)
{
    gx_device_pdf *pdev = (gx_device_pdf *) dev;
    stream *s;
    int code;
    double scale, path_scale;
    bool set_ctm;
    gs_matrix mat;
    double prescale = 1;
    gs_fixed_rect bbox;

    if (gx_path_is_void(ppath))
	return 0;		/* won't mark the page */
    if (pdf_must_put_clip_path(pdev, pcpath))
	code = pdf_unclip(pdev);
    else 
	code = pdf_open_page(pdev, PDF_IN_STREAM);
    if (code < 0)
	return code;
    code = pdf_prepare_stroke(pdev, pis);
    if (code == gs_error_rangecheck) {
	/* Fallback to the default implermentation for handling 
	   a transparency with CompatibilityLevel<=1.3 . */
	return gx_default_stroke_path((gx_device *)dev, pis, ppath, params, pdcolor, pcpath);
    }
    if (code < 0)
	return code;
    code = pdf_put_clip_path(pdev, pcpath);
    if (code < 0)
	return code;
    /*
     * If the CTM is not uniform, stroke width depends on angle.
     * We'd like to avoid resetting the CTM, so we check for uniform
     * CTMs explicitly.  Note that in PDF, unlike PostScript, it is
     * the CTM at the time of the stroke operation, not the CTM at
     * the time the path was constructed, that is used for transforming
     * the points of the path; so if we have to reset the CTM, we must
     * do it before constructing the path, and inverse-transform all
     * the coordinates.
     */
    set_ctm = (bool)gdev_vector_stroke_scaling((gx_device_vector *)pdev,
					       pis, &scale, &mat);
    if (set_ctm && (pis->ctm.xx == 0) + (pis->ctm.xy == 0) + 
	           (pis->ctm.yx == 0) + (pis->ctm.yy == 0) >= 3) {
 	/* Acrobat Reader 5 and Adobe Reader 6 issues 
 	   the "Wrong operand type" error with such matrices.
	   Besides that, we found that Acrobat Reader 4, Acrobat Reader 5 
	   and Adobe Reader 6 all store the current path in user space 
	   and apply CTM in the time of stroking - See the bug 687901. 
	   Therefore a precise conversion of Postscript to PDF isn't possible in this case.
	   Adobe viewers render a line with a constant width instead. */
	set_ctm = false;
	scale = fabs(pis->ctm.xx + pis->ctm.xy + pis->ctm.yx + pis->ctm.yy) /* Using the non-zero coeff. */
	        / sqrt(2); /* Empirically from Adobe. */
    }
    if (set_ctm) {
	/*
	 * We want a scaling factor that will bring the largest reasonable
	 * user coordinate within bounds.  We choose a factor based on the
	 * minor axis of the transformation.  Thanks to Raph Levien for
	 * the following formula.
	 */
	double a = mat.xx, b = mat.xy, c = mat.yx, d = mat.yy;
	double u = fabs(a * d - b * c);
	double v = a * a + b * b + c * c + d * d;
	double minor = (sqrt(v + 2 * u) - sqrt(v - 2 * u)) * 0.5;

	prescale = (minor == 0 || minor > 1 ? 1 : 1 / minor);
    }
    gx_path_bbox(ppath, &bbox);
    {
    	/* Check whether a painting appears inside the clipping box. 
	   Doing so after writing the clipping path due to /SP pdfmark
	   uses a special hack with painting outside the clipping box
	   for synchronizing the clipping path (see lib/gs_pdfwr.ps).
	   That hack appeared because there is no way to pass 
	   the imager state through gdev_pdf_put_params,
	   which pdfmark is implemented with.
	*/
	gs_fixed_rect clip_box, stroke_bbox = bbox;
	gs_point d0, d1;
	gs_fixed_point p0, p1;
	fixed bbox_expansion_x, bbox_expansion_y;

	gs_distance_transform(pis->line_params.half_width, 0, &ctm_only(pis), &d0);
	gs_distance_transform(0, pis->line_params.half_width, &ctm_only(pis), &d1);
	p0.x = float2fixed(any_abs(d0.x));
	p0.y = float2fixed(any_abs(d0.y));
	p1.x = float2fixed(any_abs(d1.x));
	p1.y = float2fixed(any_abs(d1.y));
	bbox_expansion_x = max(p0.x, p1.x) + fixed_1 * 2;
	bbox_expansion_y = max(p0.y, p1.y) + fixed_1 * 2;
	stroke_bbox.p.x -= bbox_expansion_x;
	stroke_bbox.p.y -= bbox_expansion_y;
	stroke_bbox.q.x += bbox_expansion_x;
	stroke_bbox.q.y += bbox_expansion_y;
	gx_cpath_outer_box(pcpath, &clip_box);
	rect_intersect(stroke_bbox, clip_box);
	if (stroke_bbox.q.x < stroke_bbox.p.x || stroke_bbox.q.y < stroke_bbox.p.y)
	    return 0;
    }
    if (make_rect_scaling(pdev, &bbox, prescale, &path_scale)) {
	scale /= path_scale;
	if (set_ctm)
	    gs_matrix_scale(&mat, path_scale, path_scale, &mat);
	else {
	    gs_make_scaling(path_scale, path_scale, &mat);
	    set_ctm = true;
	}
    }
    code = gdev_vector_prepare_stroke((gx_device_vector *)pdev, pis, params,
				      pdcolor, scale);
    if (code < 0)
	return gx_default_stroke_path(dev, pis, ppath, params, pdcolor,
				      pcpath);
    if (!pdev->HaveStrokeColor)
	pdev->saved_fill_color = pdev->saved_stroke_color;
    if (set_ctm)
  	pdf_put_matrix(pdev, "q ", &mat, "cm\n");
    code = gdev_vector_dopath((gx_device_vector *)pdev, ppath,
			      gx_path_type_stroke | gx_path_type_optimize,
			      (set_ctm ? &mat : (const gs_matrix *)0));
    if (code < 0)
	return code;
    s = pdev->strm;
    stream_puts(s, (code ? "s" : "S"));
    stream_puts(s, (set_ctm ? " Q\n" : "\n"));
    return 0;
}

/*
   The fill_rectangle_hl_color device method.
   See gxdevcli.h about return codes.
 */
int
gdev_pdf_fill_rectangle_hl_color(gx_device *dev, const gs_fixed_rect *rect, 
    const gs_imager_state *pis, const gx_drawing_color *pdcolor,
    const gx_clip_path *pcpath)
{
    int code;
    gs_fixed_rect box1 = *rect, box = {{0, 0}, {0, 0}};
    gx_device_pdf *pdev = (gx_device_pdf *) dev;
    double scale;
    gs_matrix smat;
    gs_matrix *psmat = NULL;
    const bool convert_to_image = (pdev->CompatibilityLevel <= 1.2 && 
	    gx_dc_is_pattern2_color(pdcolor));

    if (rect->p.x == rect->q.x)
	return 0;
    if (!convert_to_image) {
	code = prepare_fill_with_clip(pdev, pis, &box, true, pdcolor, pcpath);
	if (code < 0)
	    return code;
	if (code == 1)
	    return 0; /* Nothing to paint. */
	code = pdf_setfillcolor((gx_device_vector *)pdev, pis, pdcolor);
	if (code < 0)
	    return code;
	if (pcpath) 
	    rect_intersect(box1, box);
	if (box1.p.x > box1.q.x || box1.p.y > box1.q.y)
  	    return 0;		/* outside the clipping path */
	if (make_rect_scaling(pdev, &box1, 1.0, &scale)) {
	    gs_make_scaling(pdev->scale.x * scale, pdev->scale.y * scale, &smat);
	    pdf_put_matrix(pdev, "q ", &smat, "cm\n");
	    psmat = &smat;
	}
	pprintg4(pdev->strm, "%g %g %g %g re f\n",
		fixed2float(box1.p.x) * scale, fixed2float(box1.p.y) * scale,
		fixed2float(box1.q.x - box1.p.x) * scale, fixed2float(box1.q.y - box1.p.y) * scale);
	if (psmat)
	    stream_puts(pdev->strm, "Q\n");
	return 0;
    } else {
	gx_fill_params params;
	gx_path path;

	params.rule = 1; /* Not important because the path is a rectange. */
	params.adjust.x = params.adjust.y = 0;
        params.flatness = pis->flatness;
	params.fill_zero_width = false;
	gx_path_init_local(&path, pis->memory);
	gx_path_add_rectangle(&path, rect->p.x, rect->p.y, rect->q.x, rect->q.y);
	code = gdev_pdf_fill_path(dev, pis, &path, &params, pdcolor, pcpath);
	gx_path_free(&path, "gdev_pdf_fill_rectangle_hl_color");
	return code;

    }
}


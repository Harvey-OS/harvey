/* Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevpdfd.c,v 1.3 2000/03/16 01:21:24 lpd Exp $ */
/* Path drawing procedures for pdfwrite driver */
#include "math_.h"
#include "gx.h"
#include "gxdevice.h"
#include "gxfixed.h"
#include "gxistate.h"
#include "gxpaint.h"
#include "gzpath.h"
#include "gzcpath.h"
#include "gdevpdfx.h"

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
    pdf_put_clip_path(pdev, NULL);
    pdf_set_color(pdev, color, &pdev->fill_color,
		  &psdf_set_fill_color_commands);
    pprintd4(pdev->strm, "%d %d %d %d re f\n", x, y, w, h);
    return 0;
}

/* ---------------- Path drawing ---------------- */

/* ------ Vector device implementation ------ */
private int
pdf_dorect(gx_device_vector * vdev, fixed x0, fixed y0, fixed x1, fixed y1,
	   gx_path_type_t type)
{
    fixed xmax = int2fixed(vdev->width), ymax = int2fixed(vdev->height);
    fixed xmin = 0, ymin = 0;

    /*
     * If we're doing a stroke operation, expand the checking box by the
     * stroke width.
     */
    if (type & gx_path_type_stroke) {
	double w = vdev->state.line_params.half_width;
	double xw = w * (fabs(vdev->state.ctm.xx) + fabs(vdev->state.ctm.yx));
	double yw = w * (fabs(vdev->state.ctm.xy) + fabs(vdev->state.ctm.yy));

	xmin = -(float2fixed(xw) + fixed_1);
	xmax -= xmin;
	ymin = -(float2fixed(yw) + fixed_1);
	ymax -= ymin;
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
private const gx_device_vector_procs pdf_vector_procs =
{
	/* Page management */
    NULL,
	/* Imager state */
    psdf_setlinewidth,
    psdf_setlinecap,
    psdf_setlinejoin,
    psdf_setmiterlimit,
    psdf_setdash,
    psdf_setflat,
    psdf_setlogop,
	/* Other state */
    psdf_setfillcolor,
    psdf_setstrokecolor,
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

/* Test whether we will need to put the clipping path. */
bool
pdf_must_put_clip_path(gx_device_pdf * pdev, const gx_clip_path * pcpath)
{
    if (pcpath == NULL)
	return pdev->clip_path_id != pdev->no_clip_path_id;
    if (pdev->clip_path_id == pcpath->id)
	return false;
    if (gx_cpath_includes_rectangle(pcpath, fixed_0, fixed_0,
				    int2fixed(pdev->width),
				    int2fixed(pdev->height))
	)
	return pdev->clip_path_id != pdev->no_clip_path_id;
    return true;
}

/* Put a clipping path on the output file. */
int
pdf_put_clip_path(gx_device_pdf * pdev, const gx_clip_path * pcpath)
{
    stream *s = pdev->strm;

    pdev->vec_procs = &pdf_vector_procs;
    if (pcpath == NULL) {
	if (pdev->clip_path_id == pdev->no_clip_path_id)
	    return 0;
	pputs(s, "Q\nq\n");
	pdev->clip_path_id = pdev->no_clip_path_id;
    } else {
	if (pdev->clip_path_id == pcpath->id)
	    return 0;
	if (gx_cpath_includes_rectangle(pcpath, fixed_0, fixed_0,
					int2fixed(pdev->width),
					int2fixed(pdev->height))
	    ) {
	    if (pdev->clip_path_id == pdev->no_clip_path_id)
		return 0;
	    pputs(s, "Q\nq\n");
	    pdev->clip_path_id = pdev->no_clip_path_id;
	} else {
	    gdev_vector_dopath_state_t state;
	    gs_cpath_enum cenum;
	    gs_fixed_point vs[3];
	    int pe_op;

	    pprints1(s, "Q\nq\n%s\n", (pcpath->rule <= 0 ? "W" : "W*"));
	    gdev_vector_dopath_init(&state, (gx_device_vector *)pdev,
				    gx_path_type_fill, NULL);
	    /*
	     * We have to break 'const' here because the clip path
	     * enumeration logic uses some internal mark bits.
	     * This is very unfortunate, but until we can come up with
	     * a better algorithm, it's necessary.
	     */
	    gx_cpath_enum_init(&cenum, (gx_clip_path *) pcpath);
	    while ((pe_op = gx_cpath_enum_next(&cenum, vs)) > 0)
		gdev_vector_dopath_segment(&state, pe_op, vs);
	    pputs(s, "n\n");
	    if (pe_op < 0)
		return pe_op;
	    pdev->clip_path_id = pcpath->id;
	}
    }
    pdev->text.font = 0;
    if (pdev->context == PDF_IN_TEXT)
	pdev->context = PDF_IN_STREAM;
    pdf_reset_graphics(pdev);
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

    pdev->vec_procs = &pdf_vector_procs;
    /*
     * Check for an empty clipping path.
     */
    if (pcpath) {
	gs_fixed_rect box;

	gx_cpath_outer_box(pcpath, &box);
	if (box.p.x >= box.q.x || box.p.y >= box.q.y)
	    return 0;		/* empty clipping path */
    }
    if (!gx_dc_is_pure(pdcolor))
	return gx_default_fill_path(dev, pis, ppath, params, pdcolor,
				    pcpath);
    /*
     * Make a special check for the initial fill with white,
     * which shouldn't cause the page to be opened.
     */
    if (gx_dc_pure_color(pdcolor) == pdev->white && !is_in_page(pdev))
	return 0;
    have_path = !gx_path_is_void(ppath);
    if (have_path || pdev->context == PDF_IN_NONE ||
	pdf_must_put_clip_path(pdev, pcpath)
	) {
	code = pdf_open_page(pdev, PDF_IN_STREAM);
	if (code < 0)
	    return code;
    }
    pdf_put_clip_path(pdev, pcpath);
    pdf_set_color(pdev, gx_dc_pure_color(pdcolor), &pdev->fill_color,
		  &psdf_set_fill_color_commands);
    if (have_path) {
	stream *s = pdev->strm;

	if (params->flatness != pdev->state.flatness) {
	    pprintg1(s, "%g i\n", params->flatness);
	    pdev->state.flatness = params->flatness;
	}
	gdev_vector_dopath((gx_device_vector *)pdev, ppath,
			   gx_path_type_fill, NULL);
	pputs(s, (params->rule < 0 ? "f\n" : "f*\n"));
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
    double scale;
    bool set_ctm;
    gs_matrix mat;

    if (gx_path_is_void(ppath))
	return 0;		/* won't mark the page */
    if (!gx_dc_is_pure(pdcolor))
	return gx_default_stroke_path(dev, pis, ppath, params, pdcolor,
				      pcpath);
    pdev->vec_procs = &pdf_vector_procs;
    code = pdf_open_page(pdev, PDF_IN_STREAM);
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
    pdf_put_clip_path(pdev, pcpath);
    gdev_vector_prepare_stroke((gx_device_vector *)pdev, pis, params,
			       pdcolor, scale);
    if (set_ctm)
	pdf_put_matrix(pdev, "q ", &mat, "cm\n");
    code = gdev_vector_dopath((gx_device_vector *)pdev, ppath,
			      gx_path_type_stroke,
			      (set_ctm ? &mat : (const gs_matrix *)0));
    if (code < 0)
	return code;
    s = pdev->strm;
    pputs(s, (code ? "s" : "S"));
    pputs(s, (set_ctm ? " Q\n" : "\n"));
    return 0;
}

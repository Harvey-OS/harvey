/* Copyright (C) 1991, 1992, 1994, 1996, 1997, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsdps1.c,v 1.10 2003/09/15 10:04:01 igor Exp $ */
/* Display PostScript graphics additions for Ghostscript library */
#include "math_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsmatrix.h"		/* for gscoord.h */
#include "gscoord.h"
#include "gspaint.h"
#include "gxdevice.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gxhldevc.h"
#include "gspath.h"
#include "gspath2.h"		/* defines interface */
#include "gzpath.h"
#include "gzcpath.h"
#include "gzstate.h"

/*
 * Define how much rounding slop setbbox should leave,
 * in device coordinates.  Because of rounding in transforming
 * path coordinates to fixed point, the minimum realistic value is:
 *
 *      #define box_rounding_slop_fixed (fixed_epsilon)
 *
 * But even this isn't enough to compensate for cumulative rounding error
 * in rmoveto or rcurveto.  Instead, we somewhat arbitrarily use:
 */
#define box_rounding_slop_fixed (fixed_epsilon * 3)

/* ------ Graphics state ------ */

/* Set the bounding box for the current path. */
int
gs_setbbox(gs_state * pgs, floatp llx, floatp lly, floatp urx, floatp ury)
{
    gs_rect ubox, dbox;
    gs_fixed_rect obox, bbox;
    gx_path *ppath = pgs->path;
    int code;

    if (llx > urx || lly > ury)
	return_error(gs_error_rangecheck);
    /* Transform box to device coordinates. */
    ubox.p.x = llx;
    ubox.p.y = lly;
    ubox.q.x = urx;
    ubox.q.y = ury;
    if ((code = gs_bbox_transform(&ubox, &ctm_only(pgs), &dbox)) < 0)
	return code;
    /* Round the corners in opposite directions. */
    /* Because we can't predict the magnitude of the dbox values, */
    /* we add/subtract the slop after fixing. */
    if (dbox.p.x < fixed2float(min_fixed + box_rounding_slop_fixed) ||
	dbox.p.y < fixed2float(min_fixed + box_rounding_slop_fixed) ||
	dbox.q.x >= fixed2float(max_fixed - box_rounding_slop_fixed + fixed_epsilon) ||
	dbox.q.y >= fixed2float(max_fixed - box_rounding_slop_fixed + fixed_epsilon)
	)
	return_error(gs_error_limitcheck);
    bbox.p.x =
	(fixed) floor(dbox.p.x * fixed_scale) - box_rounding_slop_fixed;
    bbox.p.y =
	(fixed) floor(dbox.p.y * fixed_scale) - box_rounding_slop_fixed;
    bbox.q.x =
	(fixed) ceil(dbox.q.x * fixed_scale) + box_rounding_slop_fixed;
    bbox.q.y =
	(fixed) ceil(dbox.q.y * fixed_scale) + box_rounding_slop_fixed;
    if (gx_path_bbox(ppath, &obox) >= 0) {	/* Take the union of the bboxes. */
	ppath->bbox.p.x = min(obox.p.x, bbox.p.x);
	ppath->bbox.p.y = min(obox.p.y, bbox.p.y);
	ppath->bbox.q.x = max(obox.q.x, bbox.q.x);
	ppath->bbox.q.y = max(obox.q.y, bbox.q.y);
    } else {			/* empty path *//* Just set the bbox. */
	ppath->bbox = bbox;
    }
    ppath->bbox_set = 1;
    return 0;
}

/* ------ Rectangles ------ */

/* Append a list of rectangles to a path. */
int
gs_rectappend(gs_state * pgs, const gs_rect * pr, uint count)
{
    for (; count != 0; count--, pr++) {
	floatp px = pr->p.x, py = pr->p.y, qx = pr->q.x, qy = pr->q.y;
	int code;

	/* Ensure counter-clockwise drawing. */
	if ((qx >= px) != (qy >= py))
	    qx = px, px = pr->q.x;	/* swap x values */
	if ((code = gs_moveto(pgs, px, py)) < 0 ||
	    (code = gs_lineto(pgs, qx, py)) < 0 ||
	    (code = gs_lineto(pgs, qx, qy)) < 0 ||
	    (code = gs_lineto(pgs, px, qy)) < 0 ||
	    (code = gs_closepath(pgs)) < 0
	    )
	    return code;
    }
    return 0;
}

/* Clip to a list of rectangles. */
int
gs_rectclip(gs_state * pgs, const gs_rect * pr, uint count)
{
    int code;
    gx_path save;

    gx_path_init_local(&save, pgs->memory);
    gx_path_assign_preserve(&save, pgs->path);
    gs_newpath(pgs);
    if ((code = gs_rectappend(pgs, pr, count)) < 0 ||
	(code = gs_clip(pgs)) < 0
	) {
	gx_path_assign_free(pgs->path, &save);
	return code;
    }
    gx_path_free(&save, "gs_rectclip");
    gs_newpath(pgs);
    return 0;
}

/* Fill a list of rectangles. */
/* We take the trouble to do this efficiently in the simple cases. */
int
gs_rectfill(gs_state * pgs, const gs_rect * pr, uint count)
{
    const gs_rect *rlist = pr;
    gx_clip_path *pcpath;
    uint rcount = count;
    int code;
    gx_device * pdev = pgs->device;
    gx_device_color *pdc = pgs->dev_color;
    const gs_imager_state *pis = (const gs_imager_state *)pgs;
    bool hl_color_available = gx_hld_is_hl_color_available(pis, pdc);
    gs_fixed_rect empty = {{0, 0}, {0, 0}};
    bool hl_color = (hl_color_available && 
		dev_proc(pdev, fill_rectangle_hl_color)(pdev, 
		    	    &empty, pis, pdc, NULL) == 0);

    gx_set_dev_color(pgs);
    if ((is_fzero2(pgs->ctm.xy, pgs->ctm.yx) ||
	 is_fzero2(pgs->ctm.xx, pgs->ctm.yy)) &&
	gx_effective_clip_path(pgs, &pcpath) >= 0 &&
	clip_list_is_rectangle(gx_cpath_list(pcpath)) &&
	(hl_color ||
	 pdc->type == gx_dc_type_pure ||
	 pdc->type == gx_dc_type_ht_binary ||
	 pdc->type == gx_dc_type_ht_colored
	 /* DeviceN todo: add wts case */) &&
	gs_state_color_load(pgs) >= 0 &&
	(*dev_proc(pdev, get_alpha_bits)) (pdev, go_graphics)
	<= 1 &&
        (!pgs->overprint || !pgs->effective_overprint_mode)
	) {
	uint i;
	gs_fixed_rect clip_rect;

	gx_cpath_inner_box(pcpath, &clip_rect);
	for (i = 0; i < count; ++i) {
	    gs_fixed_point p, q;
	    gs_fixed_rect draw_rect;
	    
	    if (gs_point_transform2fixed(&pgs->ctm, pr[i].p.x, pr[i].p.y, &p) < 0 ||
		gs_point_transform2fixed(&pgs->ctm, pr[i].q.x, pr[i].q.y, &q) < 0
		) {		/* Switch to the slow algorithm. */
		goto slow;
	    }
	    draw_rect.p.x = min(p.x, q.x);
	    draw_rect.p.y = min(p.y, q.y);
	    draw_rect.q.x = max(p.x, q.x);
	    draw_rect.q.y = max(p.y, q.y);
	    if (hl_color) {
		rect_intersect(draw_rect, clip_rect);
		if (draw_rect.p.x < draw_rect.q.x &&
		    draw_rect.p.y < draw_rect.q.y) {
		    code = dev_proc(pdev, fill_rectangle_hl_color)(pdev,
			     &draw_rect, pis, pdc, pcpath);
		    if (code < 0)
			return code;
		}
	    } else {
		int x, y, w, h;

		draw_rect.p.x -= pgs->fill_adjust.x;
		draw_rect.p.y -= pgs->fill_adjust.x;
		draw_rect.q.x += pgs->fill_adjust.x;
		draw_rect.q.y += pgs->fill_adjust.x;
		rect_intersect(draw_rect, clip_rect);
		x = fixed2int_pixround(draw_rect.p.x);
		y = fixed2int_pixround(draw_rect.p.y);
		w = fixed2int_pixround(draw_rect.q.x) - x;
		h = fixed2int_pixround(draw_rect.q.y) - y;
		if (w > 0 && h > 0)
    		    if (gx_fill_rectangle(x, y, w, h, pdc, pgs) < 0)
			goto slow;
	    }
	}
	return 0;
      slow:rlist = pr + i;
	rcount = count - i;
    } {
	bool do_save = !gx_path_is_null(pgs->path);

	if (do_save) {
	    if ((code = gs_gsave(pgs)) < 0)
		return code;
	    gs_newpath(pgs);
	}
	if ((code = gs_rectappend(pgs, rlist, rcount)) < 0 ||
	    (code = gs_fill(pgs)) < 0
	    )
	    DO_NOTHING;
	if (do_save)
	    gs_grestore(pgs);
	else if (code < 0)
	    gs_newpath(pgs);
    }
    return code;
}

/* Stroke a list of rectangles. */
/* (We could do this a lot more efficiently.) */
int
gs_rectstroke(gs_state * pgs, const gs_rect * pr, uint count,
	      const gs_matrix * pmat)
{
    bool do_save = pmat != NULL || !gx_path_is_null(pgs->path);
    int code;

    if (do_save) {
	if ((code = gs_gsave(pgs)) < 0)
	    return code;
	gs_newpath(pgs);
    }
    if ((code = gs_rectappend(pgs, pr, count)) < 0 ||
	(pmat != NULL && (code = gs_concat(pgs, pmat)) < 0) ||
	(code = gs_stroke(pgs)) < 0
	)
	DO_NOTHING;
    if (do_save)
	gs_grestore(pgs);
    else if (code < 0)
	gs_newpath(pgs);
    return code;
}

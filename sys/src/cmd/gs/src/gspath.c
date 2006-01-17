/* Copyright (C) 1989, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gspath.c,v 1.10 2004/08/31 13:23:16 igor Exp $ */
/* Basic path routines for Ghostscript library */
#include "gx.h"
#include "math_.h"
#include "gserrors.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gscoord.h"		/* requires gsmatrix.h */
#include "gspath.h"		/* for checking prototypes */
#include "gzstate.h"
#include "gzpath.h"
#include "gxdevice.h"		/* for gxcpath.h */
#include "gxdevmem.h"		/* for gs_device_is_memory */
#include "gzcpath.h"

/* ------ Miscellaneous ------ */

int
gs_newpath(gs_state * pgs)
{
    pgs->current_point_valid = false;
    return gx_path_new(pgs->path);
}

int
gs_closepath(gs_state * pgs)
{
    gx_path *ppath = pgs->path;
    int code = gx_path_close_subpath(ppath);

    if (code < 0)
	return code;
    pgs->current_point = pgs->subpath_start;
    return code;
}

int
gs_upmergepath(gs_state * pgs)
{
    return gx_path_add_path(pgs->saved->path, pgs->path);
}

/* Get the current path (for internal use only). */
gx_path *
gx_current_path(const gs_state * pgs)
{
    return pgs->path;
}

/* ------ Points and lines ------ */

/*
 * Define clamped values for out-of-range coordinates.
 * Currently the path drawing routines can't handle values
 * close to the edge of the representable space.
 */
#define max_coord_fixed (max_fixed - int2fixed(1000))	/* arbitrary */
#define min_coord_fixed (-max_coord_fixed)
private inline void
clamp_point(gs_fixed_point * ppt, floatp x, floatp y)
{
#define clamp_coord(xy)\
  ppt->xy = (xy > fixed2float(max_coord_fixed) ? max_coord_fixed :\
	     xy < fixed2float(min_coord_fixed) ? min_coord_fixed :\
	     float2fixed(xy))
    clamp_coord(x);
    clamp_coord(y);
#undef clamp_coord
}

int
gs_currentpoint(gs_state * pgs, gs_point * ppt)
{
    if (!pgs->current_point_valid)
	return_error(gs_error_nocurrentpoint);
    return gs_itransform(pgs, pgs->current_point.x, 
			      pgs->current_point.y, ppt);
}

private inline int 
gs_point_transform_compat(floatp x, floatp y, const gs_matrix_fixed *m, gs_point *pt)
{
#if !PRECISE_CURRENTPOINT
    gs_fixed_point p;
    int code = gs_point_transform2fixed(m, x, y, &p);

    if (code < 0)
	return code;
    pt->x = fixed2float(p.x);
    pt->y = fixed2float(p.y);
    return 0;
#else
    return gs_point_transform(x, y, (const gs_matrix *)m, pt);
#endif
}

private inline int 
gs_distance_transform_compat(floatp x, floatp y, const gs_matrix_fixed *m, gs_point *pt)
{
#if !PRECISE_CURRENTPOINT
    gs_fixed_point p;
    int code = gs_distance_transform2fixed(m, x, y, &p);

    if (code < 0)
	return code;
    pt->x = fixed2float(p.x);
    pt->y = fixed2float(p.y);
    return 0;
#else
    return gs_distance_transform(x, y, (const gs_matrix *)m, pt);
#endif
}

private inline int
clamp_point_aux(bool clamp_coordinates, gs_fixed_point *ppt, floatp x, floatp y)
{
    if (!f_fits_in_bits(x, fixed_int_bits) || !f_fits_in_bits(y, fixed_int_bits)) {
	if (!clamp_coordinates)
	    return_error(gs_error_limitcheck);
	clamp_point(ppt, x, y);
    } else {
	/* 181-01.ps" fails with no rounding in 
	   "Verify as last element of a userpath and effect on setbbox." */
	ppt->x = float2fixed_rounded(x);
	ppt->y = float2fixed_rounded(y);
    }
    return 0;
}

int
gs_moveto_aux(gs_imager_state *pis, gx_path *ppath, floatp x, floatp y)
{
    gs_fixed_point pt;
    int code;

    code = clamp_point_aux(pis->clamp_coordinates, &pt, x, y);
    if (code < 0)
	return code;
    code = gx_path_add_point(ppath, pt.x, pt.y);
    if (code < 0)
	return code;
    ppath->start_flags = ppath->state_flags;
    gx_setcurrentpoint(pis, x, y);
    pis->subpath_start = pis->current_point;
    pis->current_point_valid = true;
    return 0;
}

int
gs_moveto(gs_state * pgs, floatp x, floatp y)
{
    gs_point pt;
    int code = gs_point_transform_compat(x, y, &pgs->ctm, &pt);

    if (code < 0)
	return code;
    return gs_moveto_aux((gs_imager_state *)pgs, pgs->path, pt.x, pt.y);
}

int
gs_rmoveto(gs_state * pgs, floatp x, floatp y)
{
    gs_point dd;
    int code; 

    if (!pgs->current_point_valid)
	return_error(gs_error_nocurrentpoint);
    code = gs_distance_transform_compat(x, y, &pgs->ctm, &dd);
    if (code < 0)
	return code;
    /* fixme : check in range. */
    return gs_moveto_aux((gs_imager_state *)pgs, pgs->path, 
		dd.x + pgs->current_point.x, dd.y + pgs->current_point.y);
}

private inline int
gs_lineto_aux(gs_state * pgs, floatp x, floatp y)
{
    gx_path *ppath = pgs->path;
    gs_fixed_point pt;
    int code;

    code = clamp_point_aux(pgs->clamp_coordinates, &pt, x, y);
    if (code < 0)
	return code;
    code = gx_path_add_line(ppath, pt.x, pt.y);
    if (code < 0)
	return code;
    gx_setcurrentpoint(pgs, x, y);
    return 0;
}

int
gs_lineto(gs_state * pgs, floatp x, floatp y)
{
    gs_point pt;
    int code = gs_point_transform_compat(x, y, &pgs->ctm, &pt);

    if (code < 0)
	return code;
    return gs_lineto_aux(pgs, pt.x, pt.y);
}

int
gs_rlineto(gs_state * pgs, floatp x, floatp y)
{
    gs_point dd;
    int code; 

    if (!pgs->current_point_valid)
	return_error(gs_error_nocurrentpoint);
    code = gs_distance_transform_compat(x, y, &pgs->ctm, &dd);
    if (code < 0)
	return code;
    /* fixme : check in range. */
    return gs_lineto_aux(pgs, dd.x + pgs->current_point.x, 
                              dd.y + pgs->current_point.y);
}

/* ------ Curves ------ */

private inline int
gs_curveto_aux(gs_state * pgs,
	   floatp x1, floatp y1, floatp x2, floatp y2, floatp x3, floatp y3)
{
    gs_fixed_point p1, p2, p3;
    int code;
    gx_path *ppath = pgs->path;

    code = clamp_point_aux(pgs->clamp_coordinates, &p1, x1, y1);
    if (code < 0)
	return code;
    code = clamp_point_aux(pgs->clamp_coordinates, &p2, x2, y2);
    if (code < 0)
	return code;
    code = clamp_point_aux(pgs->clamp_coordinates, &p3, x3, y3);
    if (code < 0)
	return code;
    code = gx_path_add_curve(ppath, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
    if (code < 0)
	return code;
    gx_setcurrentpoint(pgs, x3, y3);
    return 0;
}

int
gs_curveto(gs_state * pgs,
	   floatp x1, floatp y1, floatp x2, floatp y2, floatp x3, floatp y3)
{
    gs_point pt1, pt2, pt3;
    int code;

    code = gs_point_transform_compat(x1, y1, &pgs->ctm, &pt1);
    if (code < 0)
	return code;
    code = gs_point_transform_compat(x2, y2, &pgs->ctm, &pt2);
    if (code < 0)
	return code;
    code = gs_point_transform_compat(x3, y3, &pgs->ctm, &pt3);
    if (code < 0)
	return code;
    return gs_curveto_aux(pgs,   pt1.x, pt1.y,   pt2.x, pt2.y,   pt3.x, pt3.y);
}

int
gs_rcurveto(gs_state * pgs,
     floatp dx1, floatp dy1, floatp dx2, floatp dy2, floatp dx3, floatp dy3)
{
    gs_point dd1, dd2, dd3;
    int code; 

    if (!pgs->current_point_valid)
	return_error(gs_error_nocurrentpoint);
    code = gs_distance_transform_compat(dx1, dy1, &pgs->ctm, &dd1);
    if (code < 0)
	return code;
    code = gs_distance_transform_compat(dx2, dy2, &pgs->ctm, &dd2);
    if (code < 0)
	return code;
    code = gs_distance_transform_compat(dx3, dy3, &pgs->ctm, &dd3);
    if (code < 0)
	return code;
    /* fixme : check in range. */
    return gs_curveto_aux(pgs, dd1.x + pgs->current_point.x, dd1.y + pgs->current_point.y, 
			       dd2.x + pgs->current_point.x, dd2.y + pgs->current_point.y, 
			       dd3.x + pgs->current_point.x, dd3.y + pgs->current_point.y);
}

/* ------ Clipping ------ */

/* Forward references */
private int common_clip(gs_state *, int);

/*
 * Return the effective clipping path of a graphics state.  Sometimes this
 * is the intersection of the clip path and the view clip path; sometimes it
 * is just the clip path.  We aren't sure what the correct algorithm is for
 * this: for now, we use view clipping unless the current device is a memory
 * device.  This takes care of the most important case, where the current
 * device is a cache device.
 */
int
gx_effective_clip_path(gs_state * pgs, gx_clip_path ** ppcpath)
{
    gs_id view_clip_id =
	(pgs->view_clip == 0 || pgs->view_clip->rule == 0 ? gs_no_id :
	 pgs->view_clip->id);

    if (gs_device_is_memory(pgs->device)) {
	*ppcpath = pgs->clip_path;
	return 0;
    }
    if (pgs->effective_clip_id == pgs->clip_path->id &&
	pgs->effective_view_clip_id == view_clip_id
	) {
	*ppcpath = pgs->effective_clip_path;
	return 0;
    }
    /* Update the cache. */
    if (view_clip_id == gs_no_id) {
	if (!pgs->effective_clip_shared)
	    gx_cpath_free(pgs->effective_clip_path, "gx_effective_clip_path");
	pgs->effective_clip_path = pgs->clip_path;
	pgs->effective_clip_shared = true;
    } else {
	gs_fixed_rect cbox, vcbox;

	gx_cpath_inner_box(pgs->clip_path, &cbox);
	gx_cpath_outer_box(pgs->view_clip, &vcbox);
	if (rect_within(vcbox, cbox)) {
	    if (!pgs->effective_clip_shared)
		gx_cpath_free(pgs->effective_clip_path,
			      "gx_effective_clip_path");
	    pgs->effective_clip_path = pgs->view_clip;
	    pgs->effective_clip_shared = true;
	} else {
	    /* Construct the intersection of the two clip paths. */
	    int code;
	    gx_clip_path ipath;
	    gx_path vpath;
	    gx_clip_path *npath = pgs->effective_clip_path;

	    if (pgs->effective_clip_shared) {
		npath = gx_cpath_alloc(pgs->memory, "gx_effective_clip_path");
		if (npath == 0)
		    return_error(gs_error_VMerror);
	    }
	    gx_cpath_init_local(&ipath, pgs->memory);
	    code = gx_cpath_assign_preserve(&ipath, pgs->clip_path);
	    if (code < 0)
		return code;
	    gx_path_init_local(&vpath, pgs->memory);
	    code = gx_cpath_to_path(pgs->view_clip, &vpath);
	    if (code < 0 ||
		(code = gx_cpath_clip(pgs, &ipath, &vpath,
				      gx_rule_winding_number)) < 0 ||
		(code = gx_cpath_assign_free(npath, &ipath)) < 0
		)
		DO_NOTHING;
	    gx_path_free(&vpath, "gx_effective_clip_path");
	    gx_cpath_free(&ipath, "gx_effective_clip_path");
	    if (code < 0)
		return code;
	    pgs->effective_clip_path = npath;
	    pgs->effective_clip_shared = false;
	}
    }
    pgs->effective_clip_id = pgs->effective_clip_path->id;
    pgs->effective_view_clip_id = view_clip_id;
    *ppcpath = pgs->effective_clip_path;
    return 0;
}

#ifdef DEBUG
/* Note that we just set the clipping path (internal). */
private void
note_set_clip_path(const gs_state * pgs)
{
    if (gs_debug_c('P')) {
	dlprintf("[P]Clipping path:\n");
	gx_cpath_print(pgs->clip_path);
    }
}
#else
#  define note_set_clip_path(pgs) DO_NOTHING
#endif

int
gs_clippath(gs_state * pgs)
{
    gx_path cpath;
    int code;

    gx_path_init_local(&cpath, pgs->path->memory);
    code = gx_cpath_to_path(pgs->clip_path, &cpath);
    if (code >= 0)
	code = gx_path_assign_free(pgs->path, &cpath);
    if (code < 0)
	gx_path_free(&cpath, "gs_clippath");
    return code;
}

int
gs_initclip(gs_state * pgs)
{
    gs_fixed_rect box;
    int code = gx_default_clip_box(pgs, &box);

    if (code < 0)
	return code;
    return gx_clip_to_rectangle(pgs, &box);
}

int
gs_clip(gs_state * pgs)
{
    return common_clip(pgs, gx_rule_winding_number);
}

int
gs_eoclip(gs_state * pgs)
{
    return common_clip(pgs, gx_rule_even_odd);
}

private int
common_clip(gs_state * pgs, int rule)
{
    int code = gx_cpath_clip(pgs, pgs->clip_path, pgs->path, rule);
    if (code < 0)
	return code;
    pgs->clip_path->rule = rule;
    note_set_clip_path(pgs);
    return 0;
}

/* Establish a rectangle as the clipping path. */
/* Used by initclip and by the character and Pattern cache logic. */
int
gx_clip_to_rectangle(gs_state * pgs, gs_fixed_rect * pbox)
{
    int code = gx_cpath_from_rectangle(pgs->clip_path, pbox);

    if (code < 0)
	return code;
    pgs->clip_path->rule = gx_rule_winding_number;
    note_set_clip_path(pgs);
    return 0;
}

/* Set the clipping path to the current path, without intersecting. */
/* This is very inefficient right now. */
int
gx_clip_to_path(gs_state * pgs)
{
    gs_fixed_rect bbox;
    int code;

    if ((code = gx_path_bbox(pgs->path, &bbox)) < 0 ||
	(code = gx_clip_to_rectangle(pgs, &bbox)) < 0 ||
	(code = gs_clip(pgs)) < 0
	)
	return code;
    note_set_clip_path(pgs);
    return 0;
}

/* Get the default clipping box. */
int
gx_default_clip_box(const gs_state * pgs, gs_fixed_rect * pbox)
{
    register gx_device *dev = gs_currentdevice(pgs);
    gs_rect bbox;
    gs_matrix imat;
    int code;

    if (dev->ImagingBBox_set) {	/* Use the ImagingBBox, relative to default user space. */
	gs_defaultmatrix(pgs, &imat);
	bbox.p.x = dev->ImagingBBox[0];
	bbox.p.y = dev->ImagingBBox[1];
	bbox.q.x = dev->ImagingBBox[2];
	bbox.q.y = dev->ImagingBBox[3];
    } else {			/* Use the MediaSize indented by the HWMargins, */
	/* relative to unrotated user space adjusted by */
	/* the Margins.  (We suspect this isn't quite right, */
	/* but the whole issue of "margins" is such a mess that */
	/* we don't think we can do any better.) */
	(*dev_proc(dev, get_initial_matrix)) (dev, &imat);
	/* Adjust for the Margins. */
	imat.tx += dev->Margins[0] * dev->HWResolution[0] /
	    dev->MarginsHWResolution[0];
	imat.ty += dev->Margins[1] * dev->HWResolution[1] /
	    dev->MarginsHWResolution[1];
	bbox.p.x = dev->HWMargins[0];
	bbox.p.y = dev->HWMargins[1];
	bbox.q.x = dev->MediaSize[0] - dev->HWMargins[2];
	bbox.q.y = dev->MediaSize[1] - dev->HWMargins[3];
    }
    code = gs_bbox_transform(&bbox, &imat, &bbox);
    if (code < 0)
	return code;
    /* Round the clipping box so that it doesn't get ceilinged. */
    pbox->p.x = fixed_rounded(float2fixed(bbox.p.x));
    pbox->p.y = fixed_rounded(float2fixed(bbox.p.y));
    pbox->q.x = fixed_rounded(float2fixed(bbox.q.x));
    pbox->q.y = fixed_rounded(float2fixed(bbox.q.y));
    return 0;
}

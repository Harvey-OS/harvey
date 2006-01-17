/* Copyright (C) 1997, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsdps.c,v 1.5 2002/06/16 05:48:55 lpd Exp $ */
/* Display PostScript extensions */
#include "gx.h"
#include "gserrors.h"
#include "gsdps.h"
#include "gspath.h"		/* for newpath */
#include "gxdevice.h"		/* for gxcpath.h */
#include "gzpath.h"		/* for gzcpath.h */
#include "gzstate.h"
#include "gzcpath.h"

/* ---------------- View clipping ---------------- */

/* Forward references */
private int common_viewclip(gs_state *, int);

int
gs_initviewclip(gs_state * pgs)
{
    gx_clip_path *pcpath = pgs->view_clip;

    if (pcpath != 0 && pcpath->rule != 0) {
	gx_cpath_reset(pcpath);
	pcpath->rule = 0;
    }
    return 0;
}

int
gs_viewclip(gs_state * pgs)
{
    return common_viewclip(pgs, gx_rule_winding_number);
}

int
gs_eoviewclip(gs_state * pgs)
{
    return common_viewclip(pgs, gx_rule_even_odd);
}

/* This code is (almost) copied from common_clip in gspath.c. */
/* Someday we'll find a way to merge them. */
private int
common_viewclip(gs_state * pgs, int rule)
{
    gs_fixed_rect bbox;
    gx_clip_path rpath;
    int code;
    gx_clip_path *pcpath = pgs->view_clip;

    if (pcpath == 0) {
	pcpath = gx_cpath_alloc(pgs->memory, "gs_[eo]viewclip");
	if (pcpath == 0)
	    return_error(gs_error_VMerror);
	pgs->view_clip = pcpath;
    }
    if ((code = gx_path_bbox(pgs->path, &bbox)) < 0)
	return code;
    gx_cpath_init_local(&rpath, pgs->memory);
    code = gx_cpath_from_rectangle(&rpath, &bbox);
    if (code >= 0)
	code = gx_cpath_clip(pgs, &rpath, pgs->path, rule);
    if (code < 0) {
	gx_cpath_free(&rpath, "gs_[eo]viewclip");
	return code;
    }
    rpath.rule = rule;
    gx_cpath_assign_free(pcpath, &rpath);
    gs_newpath(pgs);
    return 0;
}

int
gs_viewclippath(gs_state * pgs)
{
    gx_path cpath;
    gx_clip_path *pcpath = pgs->view_clip;
    int code;

    gx_path_init_local(&cpath, pgs->memory);
    if (pcpath == 0 || pcpath->rule == 0) {
	/* No view clip path is active: fabricate one. */
	gs_fixed_rect box;

	code = gx_default_clip_box(pgs, &box);
	if (code < 0)
	    return code;
	code = gx_path_add_rectangle(&cpath, box.p.x, box.p.y,
				     box.q.x, box.q.y);
    } else {
	code = gx_cpath_to_path(pcpath, &cpath);
    }
    if (code < 0)
	return code;
    return gx_path_assign_free(pgs->path, &cpath);
}

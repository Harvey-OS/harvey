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

/* $Id: gscolor3.c,v 1.9 2004/09/15 07:59:18 igor Exp $ */
/* "Operators" for LanguageLevel 3 color facilities */
#include "gx.h"
#include "gserrors.h"
#include "gsmatrix.h"		/* for gscolor2.h */
#include "gscolor3.h"
#include "gsptype2.h"
#include "gxcolor2.h"		/* for gxpcolor.h */
#include "gxcspace.h"		/* for gs_cspace_init */
#include "gxdcolor.h"		/* for gxpcolor.h */
#include "gxpcolor.h"		/* for gs_color_space_type_Pattern */
#include "gzstate.h"
#include "gzpath.h"
#include "gxpaint.h"		/* (requires gx_path) */
#include "gxshade.h"

/* setsmoothness */
int
gs_setsmoothness(gs_state * pgs, floatp smoothness)
{
    pgs->smoothness =
	(smoothness < 0 ? 0 : smoothness > 1 ? 1 : smoothness);
    return 0;
}

/* currentsmoothness */
float
gs_currentsmoothness(const gs_state * pgs)
{
    return pgs->smoothness;
}

/* shfill */
int
gs_shfill(gs_state * pgs, const gs_shading_t * psh)
{
    /*
     * shfill is equivalent to filling the current clipping path (or, if
     * clipping, its bounding box) with the shading, disregarding the
     * Background if any.  In order to produce reasonable high-level output,
     * we must actually implement this by calling gs_fill rather than
     * gs_shading_fill_path.  However, filling with a shading pattern does
     * paint the Background, so if necessary, we construct a copy of the
     * shading with Background removed.
     */
    gs_pattern2_template_t pat;
    gx_path cpath;
    gs_matrix imat;
    gs_client_color cc;
    gs_color_space cs;
    gx_device_color devc;
    int code;

    gs_pattern2_init(&pat);
    pat.Shading = psh;
    gs_make_identity(&imat);
    code = gs_make_pattern(&cc, (gs_pattern_template_t *)&pat, &imat, pgs,
			   pgs->memory);
    if (code < 0)
	return code;
    code = gs_pattern2_set_shfill(&cc);
    if (code < 0)
	return code;
    gs_cspace_init(&cs, &gs_color_space_type_Pattern, pgs->memory, false);
    cs.params.pattern.has_base_space = false;
    code = cs.type->remap_color(&cc, &cs, &devc, (gs_imager_state *)pgs,
				pgs->device, gs_color_select_texture);
    if (code >= 0) {
	gx_path_init_local(&cpath, pgs->memory);
	code = gx_cpath_to_path(pgs->clip_path, &cpath);
	if (code >= 0)
	    code = gx_fill_path(&cpath, &devc, pgs, gx_rule_winding_number,
				fixed_0, fixed_0);
	gx_path_free(&cpath, "gs_shfill");
    }
    gs_pattern_reference(&cc, -1);
    return code;
}

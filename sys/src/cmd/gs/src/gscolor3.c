/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gscolor3.c,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* "Operators" for LanguageLevel 3 color facilities */
#include "gx.h"
#include "gserrors.h"
#include "gscspace.h"		/* for gscolor2.h */
#include "gsmatrix.h"		/* for gscolor2.h */
#include "gscolor2.h"
#include "gscolor3.h"
#include "gzstate.h"
#include "gzpath.h"
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
    gx_path cpath;
    int code;

    gx_path_init_local(&cpath, pgs->memory);
    code = gx_cpath_to_path(pgs->clip_path, &cpath);
    if (code < 0)
	return code;
    code = gs_shading_fill_path(psh, &cpath, NULL, gs_currentdevice(pgs),
				(gs_imager_state *)pgs, false);
    gx_path_free(&cpath, "gs_shfill");
    return code;
}

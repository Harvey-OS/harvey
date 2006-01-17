/* Copyright (C) 1992, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zimage2.c,v 1.7 2002/08/22 07:12:29 henrys Exp $ */
/* image operator extensions for Level 2 PostScript */
#include "memory_.h"
#include "ghost.h"
#include "gsimage.h"
#include "gxiparam.h"
#include "icstate.h"
#include "iimage2.h"
#include "igstate.h"		/* for igs */


/*
 * Process an image that has no explicit source data.  This isn't used by
 * standard Level 2, but it's a very small procedure and is needed by
 * both zdps.c and zdpnext.c.
 */
int
process_non_source_image(i_ctx_t *i_ctx_p, const gs_image_common_t * pic,
			 client_name_t cname)
{
    gx_image_enum_common_t *pie;
    int code = gs_image_begin_typed(pic, igs, false /****** WRONG ******/ ,
				    &pie);

    /* We didn't pass any data, so there's nothing to clean up. */
    return code;
}

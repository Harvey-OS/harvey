/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zcspixel.c,v 1.5 2004/08/04 19:36:13 stefan Exp $ */
/* DevicePixel color space support */
#include "ghost.h"
#include "oper.h"
#include "igstate.h"
#include "gscspace.h"
#include "gsmatrix.h"		/* for gscolor2.h */
#include "gscolor2.h"
#include "gscpixel.h"
#include "ialloc.h"

/* <array> .setdevicepixelspace - */
private int
zsetdevicepixelspace(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    ref depth;
    gs_color_space cs;
    int code;

    check_read_type(*op, t_array);
    if (r_size(op) != 2)
	return_error(e_rangecheck);
    array_get(imemory, op, 1L, &depth);
    check_type_only(depth, t_integer);
    code = gs_cspace_init_DevicePixel(imemory, &cs, (int)depth.value.intval);
    if (code < 0)
	return code;
    code = gs_setcolorspace(igs, &cs);
    if (code >= 0)
	pop(1);
    return code;
}

/* ------ Initialization procedure ------ */

const op_def zcspixel_op_defs[] =
{
    {"1.setdevicepixelspace", zsetdevicepixelspace},
    op_def_end(0)
};

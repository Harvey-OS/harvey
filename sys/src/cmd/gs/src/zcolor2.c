/* Copyright (C) 1992, 2000, 2001 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zcolor2.c,v 1.10 2002/08/22 07:12:29 henrys Exp $ */
/* Level 2 color operators */
#include "ghost.h"
#include "string_.h"
#include "oper.h"
#include "gsmatrix.h"
#include "gsstruct.h"
#include "gxcspace.h"
#include "gscolor2.h"
#include "igstate.h"
#include "store.h"


/*
 *  -   .useralternate   <bool>
 *
 * Push true if the current color space contains a base or alternate
 * color space and makes use of that color space (e.g.: a Separation
 * color space for a component not supported by the process color model.
 */
private int
zusealternate(i_ctx_t * i_ctx_p)
{
    os_ptr                  op = osp;
    const gs_color_space *  pcs = gs_currentcolorspace(igs);

    push(1);
    make_bool(op, cs_base_space(pcs) != 0);
    return 0;
}


/* ------ Initialization procedure ------ */

const op_def    zcolor2_l2_op_defs[] = {
    op_def_begin_level2(),
    { "0.usealternate", zusealternate },
    op_def_end(0)
};

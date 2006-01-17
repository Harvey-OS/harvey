/* Copyright (C) 2002 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: zcolor3.c,v 1.2 2002/09/06 18:42:16 dan Exp $*/
/* Level 3 color operators */
#include "ghost.h"
#include "oper.h"
#include "igstate.h"


/*
 *  <bool>   .setuseciecolor  -
 *
 * Set the use_cie_color parameter for the interpreter state, which
 * corresponds to the UseCIEColor page device parameter. This parameter
 * may be read at all language levels, but it may be set only for
 * language level 3. The parameter is handled separately from the page
 * device dictionary primarily for performance reasons (it may need to
 * be checked frequently), but also to ensure proper language level
 * specific behavior.
 *
 * This operator is accessible only during initialization and is called
 * only under controlled conditions. Hence, it does not do any operand
 * checking.
 */
private int
zsetuseciecolor(i_ctx_t * i_ctx_p)
{
    os_ptr  op = osp;

    istate->use_cie_color = *op;
    pop(1);
    return 0;
}


/*
 * Initialization procedure
 */

const op_def    zcolor3_l3_op_defs[] = {
    op_def_begin_ll3(),
    { "0.setuseciecolor", zsetuseciecolor },
    op_def_end(0)
};

/* Copyright (C) 1995, 1996, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsrop.c,v 1.4 2002/02/21 22:24:52 giles Exp $ */
/* RasterOp / transparency accessing for library */
#include "gx.h"
#include "gserrors.h"
#include "gzstate.h"
#include "gsrop.h"

/* setrasterop */
int
gs_setrasterop(gs_state * pgs, gs_rop3_t rop)
{
    if (pgs->in_cachedevice)
	return_error(gs_error_undefined);
    pgs->log_op = (rop & rop3_1) | (pgs->log_op & ~rop3_1);
    return 0;
}

/* currentrasterop */
gs_rop3_t
gs_currentrasterop(const gs_state * pgs)
{
    return lop_rop(pgs->log_op);
}

/* setsourcetransparent */
int
gs_setsourcetransparent(gs_state * pgs, bool transparent)
{
    if (pgs->in_cachedevice)
	return_error(gs_error_undefined);
    pgs->log_op =
	(transparent ? pgs->log_op | lop_S_transparent :
	 pgs->log_op & ~lop_S_transparent);
    return 0;
}

/* currentsourcetransparent */
bool
gs_currentsourcetransparent(const gs_state * pgs)
{
    return (pgs->log_op & lop_S_transparent) != 0;
}

/* settexturetransparent */
int
gs_settexturetransparent(gs_state * pgs, bool transparent)
{
    if (pgs->in_cachedevice)
	return_error(gs_error_undefined);
    pgs->log_op =
	(transparent ? pgs->log_op | lop_T_transparent :
	 pgs->log_op & ~lop_T_transparent);
    return 0;
}

/* currenttexturetransparent */
bool
gs_currenttexturetransparent(const gs_state * pgs)
{
    return (pgs->log_op & lop_T_transparent) != 0;
}

/* Save/restore logical operation.  (For internal use only.) */
int
gs_set_logical_op(gs_state * pgs, gs_logical_operation_t lop)
{
    pgs->log_op = lop;
    return 0;
}
gs_logical_operation_t
gs_current_logical_op(const gs_state * pgs)
{
    return pgs->log_op;
}

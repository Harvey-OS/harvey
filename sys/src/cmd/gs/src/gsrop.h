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

/* $Id: gsrop.h,v 1.5 2002/06/16 08:45:42 lpd Exp $ */
/* RasterOp / transparency procedure interface */

#ifndef gsrop_INCLUDED
#  define gsrop_INCLUDED

#include "gsropt.h"

/* Procedural interface */

int gs_setrasterop(gs_state *, gs_rop3_t);
gs_rop3_t gs_currentrasterop(const gs_state *);
int gs_setsourcetransparent(gs_state *, bool);
bool gs_currentsourcetransparent(const gs_state *);
int gs_settexturetransparent(gs_state *, bool);
bool gs_currenttexturetransparent(const gs_state *);

/* Save/restore the combined logical operation. */
gs_logical_operation_t gs_current_logical_op(const gs_state *);
int gs_set_logical_op(gs_state *, gs_logical_operation_t);

#endif /* gsrop_INCLUDED */

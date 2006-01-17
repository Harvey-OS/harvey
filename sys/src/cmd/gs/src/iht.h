/* Copyright (C) 1997 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: iht.h,v 1.6 2005/10/11 10:04:28 leonardo Exp $ */
/* Procedures exported by zht.c for zht1.c and zht2.c */

#ifndef iht_INCLUDED
#  define iht_INCLUDED

int zscreen_params(os_ptr op, gs_screen_halftone * phs);

int zscreen_enum_init(i_ctx_t *i_ctx_p, const gx_ht_order * porder,
		      gs_screen_halftone * phs, ref * pproc, int npop,
		      op_proc_t finish_proc, int space_index);

#endif /* iht_INCLUDED */

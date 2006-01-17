/* Copyright (C) 1989, 1995, 1996, 1997, 1999, 2000, 2002 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zchar42.h,v 1.1 2003/09/11 21:12:18 igor Exp $ */

#ifndef zchar42_INCLUDED
#  define zchar42_INCLUDED

/* Get a Type 42 character metrics and set the cache device. */
int zchar42_set_cache(i_ctx_t *i_ctx_p, gs_font_base *pbfont, ref *cnref, 
	    uint glyph_index, op_proc_t cont, op_proc_t *exec_cont, bool put_lsb);

#endif /* zchar42_INCLUDED */

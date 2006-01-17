/* Copyright (C) 2002 Artifex Software Inc.  All rights reserved.
  
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

/* $Id: zht2.h,v 1.3 2004/08/04 19:36:13 stefan Exp $ */
/* Level 2 sethalftone support */

#ifndef zht2_INCLUDED
#  define zht2_INCLUDED

#include "gscspace.h"            /* for gs_separation_name */

/*
 * This routine translates a gs_separation_name value into a character string
 * pointer and a string length.
 */
int gs_get_colorname_string(const gs_memory_t *mem, 
			    gs_separation_name colorname_index,
			    unsigned char **ppstr, 
			    unsigned int *pname_size);

#endif /* zht2_INCLUDED */

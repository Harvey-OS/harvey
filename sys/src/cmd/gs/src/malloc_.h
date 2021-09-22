/* Copyright (C) 1989, 1992, 1996, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: malloc_.h,v 1.5 2002/06/16 05:03:12 lpd Exp $ */
/* Generic substitute for Unix malloc.h */

#ifndef malloc__INCLUDED
#  define malloc__INCLUDED

/* We must include std.h before any file that includes sys/types.h. */
#include "std.h"
#include <stdlib.h>

#ifdef Tamd64
#define free(p)			/* for amd64 */
#endif
#ifdef Triscv64
#define free(p)
#endif
#define gs_realloc(ptr, old_size, new_size) realloc(ptr, new_size)
#endif

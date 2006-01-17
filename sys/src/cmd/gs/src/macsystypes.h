/* Copyright (C) 1994-7 artofcode LLC.  All rights reserved.
  
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

/* $Id: macsystypes.h,v 1.8 2003/01/25 22:50:31 giles Exp $ */

#ifndef __sys_types_h__
#define __sys_types_h__

#include <MacTypes.h>
#include <unix.h>
#define CHECK_INTERRUPTS

/* use a 64 bit type for color vectors. (from MacTypes.h)
   this is important for devicen support, but can be safely
   undef'd to fallback to a 32 bit representation  */
#define GX_COLOR_INDEX_TYPE UInt64

#define main gs_main

#if (0)
#define fprintf myfprintf
#define fputs myfputs
#define getenv mygetenv
int myfprintf(FILE *file, const char *fmt, ...);
int myfputs(const char *string, FILE *file);
#endif

/* Metrowerks CodeWarrior should define this */
#ifndef __MACOS__
#define __MACOS__
#endif

#endif /* __sys_types_h__ */

/* Copyright (C) 1993, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: ctype_.h,v 1.4 2002/02/21 22:24:51 giles Exp $ */
/* Wrapper for ctype.h */

#ifndef ctype__INCLUDED
#  define ctype__INCLUDED

/* We must include std.h before any file that includes sys/types.h. */
#include "std.h"

/* ... and that's the only reason for having this file at all. */
#include <ctype.h>

#endif /* ctype__INCLUDED */

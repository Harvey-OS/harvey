/* Copyright (C) 2003 Aladdin Enterprises.  All rights reserved.

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

/* $Id: ttfmemd.h,v 1.3 2003/10/07 15:26:04 igor Exp $ */
/* Memory structure descriptors for the TrueType instruction interpreter. */

#if !defined(ttfmemd_INCLUDED)
#  define ttfmemd_INCLUDED

#include "gsstype.h"

extern_st(st_TFace);
extern_st(st_TInstance);
extern_st(st_TExecution_Context);
extern_st(st_ttfFont);
extern_st(st_ttfInterpreter);

#endif /* !defined(ttfmemd_INCLUDED) */

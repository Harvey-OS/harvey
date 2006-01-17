/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: inobtokn.c,v 1.5 2003/09/03 03:22:59 giles Exp $ */
/* Dummy scan_binary_token for Level 1 systems */
#include "ghost.h"
#include "ierrors.h"
#include "stream.h"
#include "iscan.h"
#include "iscanbin.h"

int
scan_binary_token(i_ctx_t *i_ctx_p, stream *s, ref *pref,
		  scanner_state *pstate)
{
    return_error(e_unregistered);
}

/* Copyright (C) 1994, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: sdctc.c,v 1.6 2004/08/19 19:33:09 stefan Exp $ */
/* Code common to DCT encoding and decoding streams */
#include "stdio_.h"
#include "jpeglib_.h"
#include "gsmemory.h"
#include "gsmalloc.h"
#include "strimpl.h"
#include "sdct.h"

public_st_DCT_state();

/* Set the defaults for the DCT filters. */
void
s_DCT_set_defaults(stream_state * st)
{
    stream_DCT_state *const ss = (stream_DCT_state *) st;

    ss->jpeg_memory = (gs_memory_t *)gs_lib_ctx_get_non_gc_memory_t(); /* cast away const */
    ss->data.common = 0;
	/****************
	  ss->data.common->Picky = 0;
	  ss->data.common->Relax = 0;
	 ****************/
    ss->ColorTransform = -1;
    ss->QFactor = 1.0;
    /* Clear pointers */
    ss->Markers.data = 0;
    ss->Markers.size = 0;
}

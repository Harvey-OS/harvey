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

/* $Id: gxcllzw.c,v 1.4 2002/02/21 22:24:53 giles Exp $ */
/* LZW filter initialization for RAM-based band lists */
#include "std.h"
#include "gstypes.h"
#include "gsmemory.h"
#include "gxclmem.h"
#include "slzwx.h"

private stream_LZW_state cl_LZWE_state;
private stream_LZW_state cl_LZWD_state;

/* Initialize the states to be copied. */
void
gs_cl_lzw_init(gs_memory_t * mem)
{
    s_LZW_set_defaults((stream_state *) & cl_LZWE_state);
    cl_LZWE_state.template = &s_LZWE_template;
    s_LZW_set_defaults((stream_state *) & cl_LZWD_state);
    cl_LZWD_state.template = &s_LZWD_template;
}

/* Return the prototypes for compressing/decompressing the band list. */
const stream_state *
clist_compressor_state(void *client_data)
{
    return (const stream_state *)&cl_LZWE_state;
}
const stream_state *
clist_decompressor_state(void *client_data)
{
    return (const stream_state *)&cl_LZWD_state;
}

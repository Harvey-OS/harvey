/* Copyright (C) 1997 Aladdin Enterprises.  All rights reserved.

   This file is part of Aladdin Ghostscript.

   Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
   or distributor accepts any responsibility for the consequences of using it,
   or for whether it serves any particular purpose or works at all, unless he
   or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
   License (the "License") for full details.

   Every copy of Aladdin Ghostscript must include a copy of the License,
   normally in a plain ASCII text file named PUBLIC.  The License grants you
   the right to copy, modify and redistribute Aladdin Ghostscript, but only
   under certain conditions described in the License.  Among other things, the
   License requires that the copyright notice and this notice be preserved on
   all copies.
 */

/*$Id: gxcllzw.c,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
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

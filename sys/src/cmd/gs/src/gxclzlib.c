/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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

/* $Id: gxclzlib.c,v 1.4 2002/02/21 22:24:53 giles Exp $ */
/* zlib filter initialization for RAM-based band lists */
/* Must be compiled with -I$(ZSRCDIR) */
#include "std.h"
#include "gstypes.h"
#include "gsmemory.h"
#include "gxclmem.h"
#include "szlibx.h"

private stream_zlib_state cl_zlibE_state;
private stream_zlib_state cl_zlibD_state;

/* Initialize the states to be copied. */
void
gs_cl_zlib_init(gs_memory_t * mem)
{
    s_zlib_set_defaults((stream_state *) & cl_zlibE_state);
    cl_zlibE_state.no_wrapper = true;
    cl_zlibE_state.template = &s_zlibE_template;
    s_zlib_set_defaults((stream_state *) & cl_zlibD_state);
    cl_zlibD_state.no_wrapper = true;
    cl_zlibD_state.template = &s_zlibD_template;
}

/* Return the prototypes for compressing/decompressing the band list. */
const stream_state *
clist_compressor_state(void *client_data)
{
    return (const stream_state *)&cl_zlibE_state;
}
const stream_state *
clist_decompressor_state(void *client_data)
{
    return (const stream_state *)&cl_zlibD_state;
}

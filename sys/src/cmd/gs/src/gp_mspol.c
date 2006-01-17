/* Copyright (C) 2001 artofcode LLC.  All rights reserved.
  
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

/* $Id: gp_mspol.c,v 1.5 2004/08/05 17:02:36 stefan Exp $ */
/*
 * Microsoft Windows platform polling support for Ghostscript.
 *
 */

#include "gx.h"
#include "gp.h"
#include "gpcheck.h"
#include "iapi.h"
#include "iref.h"
#include "iminst.h"
#include "imain.h"

/* ------ Process message loop ------ */
/* 
 * Check messages and interrupts; return true if interrupted.
 * This is called frequently - it must be quick!
 */
#ifdef CHECK_INTERRUPTS
int
gp_check_interrupts(const gs_memory_t *mem)
{
    if(mem == NULL) {
	/* MAJOR HACK will NOT work in multithreaded environment */
	mem = gs_lib_ctx_get_non_gc_memory_t();
    }
    if (mem && mem->gs_lib_ctx && mem->gs_lib_ctx->poll_fn)
	return (*mem->gs_lib_ctx->poll_fn)(mem->gs_lib_ctx->caller_handle);
    return 0;
}
#endif

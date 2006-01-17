/* Copyright (C) 1989, 1995, 1996, 1997 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsinit.c,v 1.7 2004/08/10 12:59:53 stefan Exp $ */
/* Initialization for the imager */
#include "stdio_.h"
#include "memory_.h"
#include "gdebug.h"
#include "gscdefs.h"
#include "gsmemory.h"
#include "gsmalloc.h"
#include "gp.h"
#include "gslib.h"		/* interface definition */

/* Configuration information from gconfig.c. */
extern_gx_init_table();

/* Initialization to be done before anything else. */
int
gs_lib_init(FILE * debug_out)
{
    return gs_lib_init1(gs_lib_init0(debug_out));
}
gs_memory_t *
gs_lib_init0(FILE * debug_out)
{
    gs_memory_t *mem;

    mem = (gs_memory_t *) gs_malloc_init(NULL);

    /* Reset debugging flags */
    memset(gs_debug, 0, 128);
    gs_log_errors = 0;
    return mem;
}
int
gs_lib_init1(gs_memory_t * mem)
{
    /* Run configuration-specific initialization procedures. */
    init_proc((*const *ipp));
    int code;

    for (ipp = gx_init_table; *ipp != 0; ++ipp)
	if ((code = (**ipp)(mem)) < 0)
	    return code;
    return 0;
}

/* Clean up after execution. */
void
gs_lib_finit(int exit_status, int code, gs_memory_t *mem)
{
    /* Do platform-specific cleanup. */
    gp_exit(exit_status, code);

    /* NB: interface problem.
     * if gs_lib_init0 was called the we should
     *    gs_malloc_release(mem);
     * else
     *    someone else has control of mem so we can't free it.
     *    gs_view and iapi.h interface 
     */
}

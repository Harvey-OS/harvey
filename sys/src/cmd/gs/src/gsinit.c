/* Copyright (C) 1989, 1995, 1996, 1997 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gsinit.c,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Initialization for the imager */
#include "stdio_.h"
#include "memory_.h"
#include "gdebug.h"
#include "gscdefs.h"
#include "gsmemory.h"
#include "gsmalloc.h"
#include "gp.h"
#include "gslib.h"		/* interface definition */

/* Imported from gsmisc.c */
extern FILE *gs_debug_out;

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

    gs_debug_out = debug_out;
    mem = (gs_memory_t *) gs_malloc_init();
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
gs_lib_finit(int exit_status, int code)
{
    fflush(stderr);		/* in case of error exit */
    /* Do platform-specific cleanup. */
    gp_exit(exit_status, code);
    gs_malloc_release();
}

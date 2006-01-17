/* Copyright (C) 1996-2001 Ghostgum Software Pty Ltd.  All rights reserved.
  
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

/* $Id: iapi.c,v 1.12 2004/08/19 19:33:09 stefan Exp $ */

/* Public Application Programming Interface to Ghostscript interpreter */

#include "string_.h"
#include "ierrors.h"
#include "gscdefs.h"
#include "gstypes.h"
#include "iapi.h"	/* Public API */
#include "iref.h"
#include "iminst.h"
#include "imain.h"
#include "imainarg.h"
#include "gsmemory.h"
#include "gsmalloc.h"
#include "gslibctx.h"

/* number of threads to allow per process
 * currently more than 1 is guarenteed to fail 
 */
static int gsapi_instance_counter = 0;
static const int gsapi_instance_max = 1;

/* Return revision numbers and strings of Ghostscript. */
/* Used for determining if wrong GSDLL loaded. */
/* This may be called before any other function. */
GSDLLEXPORT int GSDLLAPI
gsapi_revision(gsapi_revision_t *pr, int rvsize)
{
    if (rvsize < sizeof(gsapi_revision_t))
	return sizeof(gsapi_revision_t);
    pr->product = gs_product;
    pr->copyright = gs_copyright;
    pr->revision = gs_revision;
    pr->revisiondate = gs_revisiondate;
    return 0;
}

/* Create a new instance of Ghostscript. 
 * First instance per process call with *pinstance == NULL
 * next instance in a proces call with *pinstance == copy of valid_instance pointer
 * *pinstance is set to a new instance pointer.
 */
GSDLLEXPORT int GSDLLAPI 
gsapi_new_instance(void **pinstance, void *caller_handle)
{
    gs_memory_t *mem = NULL;
    gs_main_instance *minst = NULL;

    if (pinstance == NULL)
	return e_Fatal;

    /* limited to 1 instance, till it works :) */
    if ( gsapi_instance_counter >= gsapi_instance_max ) 
	return e_Fatal;
    ++gsapi_instance_counter;

    if (*pinstance == NULL)
	/* first instance in this process */
	mem = gs_malloc_init(NULL);
    else {
	/* nothing different for second thread initialization 
	 * seperate memory, ids, only stdio is process shared.
	 */
	mem = gs_malloc_init(NULL);
	
    }
    minst = gs_main_alloc_instance(mem);
    mem->gs_lib_ctx->top_of_system = (void*) minst;
    mem->gs_lib_ctx->caller_handle = caller_handle;
    mem->gs_lib_ctx->stdin_fn = NULL;
    mem->gs_lib_ctx->stdout_fn = NULL;
    mem->gs_lib_ctx->stderr_fn = NULL;
    mem->gs_lib_ctx->poll_fn = NULL;

    *pinstance = (void*)(mem->gs_lib_ctx);
    return 0;
}

/* Destroy an instance of Ghostscript */
/* We do not support multiple instances, so make sure
 * we use the default instance only once.
 */
GSDLLEXPORT void GSDLLAPI 
gsapi_delete_instance(void *lib)
{
    gs_lib_ctx_t *ctx = (gs_lib_ctx_t *)lib;
    if ((ctx != NULL)) {
	ctx->caller_handle = NULL;
	ctx->stdin_fn = NULL;
	ctx->stdout_fn = NULL;
	ctx->stderr_fn = NULL;
	ctx->poll_fn = NULL;
	get_minst_from_memory(ctx->memory)->display = NULL;
	
	/* NB: notice how no deletions are occuring, good bet this isn't thread ready
	 */
	
	--gsapi_instance_counter;

	
    }
}

/* Set the callback functions for stdio */
GSDLLEXPORT int GSDLLAPI 
gsapi_set_stdio(void *lib,
    int(GSDLLCALL *stdin_fn)(void *caller_handle, char *buf, int len),
    int(GSDLLCALL *stdout_fn)(void *caller_handle, const char *str, int len),
    int(GSDLLCALL *stderr_fn)(void *caller_handle, const char *str, int len))
{
    gs_lib_ctx_t *ctx = (gs_lib_ctx_t *)lib;
    if (lib == NULL)
	return e_Fatal;
    ctx->stdin_fn = stdin_fn;
    ctx->stdout_fn = stdout_fn;
    ctx->stderr_fn = stderr_fn;
    return 0;
}

/* Set the callback function for polling */
GSDLLEXPORT int GSDLLAPI 
gsapi_set_poll(void *lib, 
    int(GSDLLCALL *poll_fn)(void *caller_handle))
{
    gs_lib_ctx_t *ctx = (gs_lib_ctx_t *)lib;
    if (lib == NULL)
	return e_Fatal;
    ctx->poll_fn = poll_fn;
    return 0;
}

/* Set the display callback structure */
GSDLLEXPORT int GSDLLAPI 
gsapi_set_display_callback(void *lib, display_callback *callback)
{
    gs_lib_ctx_t *ctx = (gs_lib_ctx_t *)lib;
    if (lib == NULL)
	return e_Fatal;
    get_minst_from_memory(ctx->memory)->display = callback;
    /* not in a language switched build */
    return 0;
}


/* Initialise the interpreter */
GSDLLEXPORT int GSDLLAPI 
gsapi_init_with_args(void *lib, int argc, char **argv)
{
    gs_lib_ctx_t *ctx = (gs_lib_ctx_t *)lib;
    if (lib == NULL)
	return e_Fatal;
    return gs_main_init_with_args(get_minst_from_memory(ctx->memory), argc, argv);
}



/* The gsapi_run_* functions are like gs_main_run_* except
 * that the error_object is omitted.
 * An error object in minst is used instead.
 */

/* Setup up a suspendable run_string */
GSDLLEXPORT int GSDLLAPI 
gsapi_run_string_begin(void *lib, int user_errors, 
	int *pexit_code)
{
    gs_lib_ctx_t *ctx = (gs_lib_ctx_t *)lib;
    if (lib == NULL)
	return e_Fatal;

    return gs_main_run_string_begin(get_minst_from_memory(ctx->memory), 
				    user_errors, pexit_code, 
				    &(get_minst_from_memory(ctx->memory)->error_object));
}


GSDLLEXPORT int GSDLLAPI 
gsapi_run_string_continue(void *lib, 
	const char *str, uint length, int user_errors, int *pexit_code)
{
    gs_lib_ctx_t *ctx = (gs_lib_ctx_t *)lib;
    if (lib == NULL)
	return e_Fatal;

    return gs_main_run_string_continue(get_minst_from_memory(ctx->memory),
				       str, length, user_errors, pexit_code, 
				       &(get_minst_from_memory(ctx->memory)->error_object));
}

GSDLLEXPORT int GSDLLAPI 
gsapi_run_string_end(void *lib, 
	int user_errors, int *pexit_code)
{
    gs_lib_ctx_t *ctx = (gs_lib_ctx_t *)lib;
    if (lib == NULL)
	return e_Fatal;

    return gs_main_run_string_end(get_minst_from_memory(ctx->memory),
				  user_errors, pexit_code, 
				  &(get_minst_from_memory(ctx->memory)->error_object));
}

GSDLLEXPORT int GSDLLAPI 
gsapi_run_string_with_length(void *lib, 
	const char *str, uint length, int user_errors, int *pexit_code)
{
    gs_lib_ctx_t *ctx = (gs_lib_ctx_t *)lib;
    if (lib == NULL)
	return e_Fatal;

    return gs_main_run_string_with_length(get_minst_from_memory(ctx->memory),
					  str, length, user_errors, pexit_code, 
					  &(get_minst_from_memory(ctx->memory)->error_object));
}

GSDLLEXPORT int GSDLLAPI 
gsapi_run_string(void *lib, 
	const char *str, int user_errors, int *pexit_code)
{
    gs_lib_ctx_t *ctx = (gs_lib_ctx_t *)lib;
    return gsapi_run_string_with_length(get_minst_from_memory(ctx->memory),
	str, (uint)strlen(str), user_errors, pexit_code);
}

GSDLLEXPORT int GSDLLAPI 
gsapi_run_file(void *lib, const char *file_name, 
	int user_errors, int *pexit_code)
{
    gs_lib_ctx_t *ctx = (gs_lib_ctx_t *)lib;
    if (lib == NULL)
	return e_Fatal;

    return gs_main_run_file(get_minst_from_memory(ctx->memory),
			    file_name, user_errors, pexit_code, 
			    &(get_minst_from_memory(ctx->memory)->error_object));
}


/* Exit the interpreter */
GSDLLEXPORT int GSDLLAPI 
gsapi_exit(void *lib)
{
    gs_lib_ctx_t *ctx = (gs_lib_ctx_t *)lib;
    if (lib == NULL)
	return e_Fatal;

    gs_to_exit(ctx->memory, 0);
    return 0;
}

/* Visual tracer : */
struct vd_trace_interface_s;
extern struct vd_trace_interface_s * vd_trace0;
GSDLLEXPORT void GSDLLAPI
gsapi_set_visual_tracer(struct vd_trace_interface_s *I)
{   vd_trace0 = I;
}

/* end of iapi.c */

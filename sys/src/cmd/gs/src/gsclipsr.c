/* Copyright (C) 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsclipsr.c,v 1.4 2002/02/21 22:24:52 giles Exp $ */
/* clipsave/cliprestore */
#include "gx.h"
#include "gserrors.h"
#include "gsclipsr.h"
#include "gsstruct.h"
#include "gxclipsr.h"
#include "gxfixed.h"		/* for gxpath.h */
#include "gxpath.h"
#include "gzstate.h"

/* Structure descriptors */
private_st_clip_stack();

/*
 * When we free a clip stack entry, free the associated clip path,
 * and iterate down the list.  We do this iteratively so that we don't
 * take a level of recursion for each node on the list.
 */
private void
rc_free_clip_stack(gs_memory_t * mem, void *vstack, client_name_t cname)
{
    gx_clip_stack_t *stack = (gx_clip_stack_t *)vstack;
    gx_clip_stack_t *next;

    do {
	gx_clip_path *pcpath = stack->clip_path;

	next = stack->next;
	gs_free_object(stack->rc.memory, stack, cname);
	gx_cpath_free(pcpath, "rc_free_clip_stack");
    } while ((stack = next) != 0 && !--(stack->rc.ref_count));
}

/* clipsave */
int
gs_clipsave(gs_state *pgs)
{
    gs_memory_t *mem = pgs->memory;
    gx_clip_path *copy =
	gx_cpath_alloc_shared(pgs->clip_path, mem, "gs_clipsave(clip_path)");
    gx_clip_stack_t *stack =
	gs_alloc_struct(mem, gx_clip_stack_t, &st_clip_stack,
			"gs_clipsave(stack)");

    if (copy == 0 || stack == 0) {
	gs_free_object(mem, stack, "gs_clipsave(stack)");
	gs_free_object(mem, copy, "gs_clipsave(clip_path)");
	return_error(gs_error_VMerror);
    }
    rc_init_free(stack, mem, 1, rc_free_clip_stack);
    stack->clip_path = copy;
    stack->next = pgs->clip_stack;
    pgs->clip_stack = stack;
    return 0;
}

/* cliprestore */
int
gs_cliprestore(gs_state *pgs)
{
    gx_clip_stack_t *stack = pgs->clip_stack;

    if (stack) {
	gx_clip_stack_t *next = stack->next;
	gx_clip_path *pcpath = stack->clip_path;
	int code;

	if (stack->rc.ref_count == 1) {
	    /* Use assign_free rather than assign_preserve. */
	    gs_free_object(stack->rc.memory, stack, "cliprestore");
	    code = gx_cpath_assign_free(pgs->clip_path, pcpath);
	} else {
	    code = gx_cpath_assign_preserve(pgs->clip_path, pcpath);
	    if (code < 0)
		return code;
	    --(stack->rc.ref_count);
	}
	pgs->clip_stack = next;
	return code;
    } else {
	return gx_cpath_assign_preserve(pgs->clip_path, pgs->saved->clip_path);
    }
}

/*
  Copyright (C) 2001 artofcode LLC.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.

  Author: Raph Levien <raph@artofcode.com>
*/
/*$Id: gsdfilt.c,v 1.3 2001/10/17 17:28:10 raph Exp $ */
/* Functions for managing the device filter stack */

#include "ctype_.h"
#include "memory_.h"		/* for memchr, memcpy */
#include "string_.h"
#include "gx.h"
#include "gp.h"
#include "gscdefs.h"		/* for gs_lib_device_list */
#include "gserrors.h"
#include "gsfname.h"
#include "gsstruct.h"
#include "gspath.h"		/* gs_initclip prototype */
#include "gspaint.h"		/* gs_erasepage prototype */
#include "gsmatrix.h"		/* for gscoord.h */
#include "gscoord.h"		/* for gs_initmatrix */
#include "gzstate.h"
#include "gxcmap.h"
#include "gxdevice.h"
#include "gxdevmem.h"
#include "gxiodev.h"

#include "gsdfilt.h"

struct gs_device_filter_stack_s {
    gs_device_filter_stack_t *next;
    gs_device_filter_t *df;
    gx_device *next_device;
};

gs_private_st_ptrs3(st_gs_device_filter_stack, gs_device_filter_stack_t,
		    "gs_device_filter_stack",
		    gs_device_filter_stack_enum_ptrs,
		    gs_device_filter_stack_reloc_ptrs,
		    next, df, next_device);

gs_public_st_simple(st_gs_device_filter, gs_device_filter_t,
		    "gs_device_filter");

#ifdef DFILTER_TEST

/* The test device filter installs a simple forwarding device which changes
   the behavior of map_rgb_color to "bleach" colors. It is only here for
   testing purposes, and isn't necessary to be compiled in production code.
*/

private gx_color_index
gs_test_device_filter_map_rgb_color(gx_device * dev,
				    gx_color_value r, gx_color_value g, gx_color_value b)
{
    gx_device_forward * const fdev = (gx_device_forward *)dev;
    gx_device *tdev = fdev->target;

    r += (gx_max_color_value - r) >> 1;
    g += (gx_max_color_value - g) >> 1;
    b += (gx_max_color_value - b) >> 1;
    return dev_proc(tdev, map_rgb_color)(tdev, r, g, b);
}

private const gx_device_forward gs_test_device_filter_device =
{std_device_std_body_open(gx_device_forward, 0,
			  "Device filter test device", 0, 0, 1, 1),
 {NULL,		/* open_device */
  NULL,		/* get_initial_matrix */
  NULL,		/* sync_output */
  NULL,		/* output_page */
  NULL,		/* close_device */
  gs_test_device_filter_map_rgb_color
 }
};

private int
gs_test_device_filter_push(gs_device_filter_t *self, gs_memory_t *mem,
			   gx_device **pdev, gx_device *target)
{
    gx_device_forward *fdev;

    fdev = gs_alloc_struct_immovable(mem, gx_device_forward,
				     &st_device_forward,
				     "gs_test_device_filter_push");
    if (fdev == 0)
	return_error(gs_error_VMerror);
    gx_device_init((gx_device *)fdev,
		   (const gx_device *)&gs_test_device_filter_device, mem,
		   false);
    gx_device_forward_fill_in_procs(fdev);
    gx_device_copy_params((gx_device *)fdev, target);
    gx_device_set_target(fdev, target);
    *pdev = (gx_device *)fdev;
    return 0;
}

private int
gs_test_device_filter_pop(gs_device_filter_t *self, gs_memory_t *mem,
			  gs_state *pgs, gx_device *dev)
{
    gx_device_set_target((gx_device_forward *)dev, NULL);
    gs_free_object(mem, self, "gs_test_device_filter_pop");
    return 0;
}

int
gs_test_device_filter(gs_device_filter_t **pdf, gs_memory_t *mem)
{
    gs_device_filter_t *df;

    df = gs_alloc_struct(mem, gs_device_filter_t,
			 &st_gs_device_filter, "gs_test_device_filter");
    if (df == 0)
	return_error(gs_error_VMerror);
    df->push = gs_test_device_filter_push;
    df->pop = gs_test_device_filter_pop;
    *pdf = df;
    return 0;
}
#endif

int
gs_push_device_filter(gs_memory_t *mem, gs_state *pgs, gs_device_filter_t *df)
{
    gs_device_filter_stack_t *dfs;
    gx_device *new_dev = NULL;
    int code;

    dfs = gs_alloc_struct(mem, gs_device_filter_stack_t,
			  &st_gs_device_filter_stack, "gs_push_device_filter");
    if (dfs == NULL)
	return_error(gs_error_VMerror);
    rc_increment(pgs->device);
    dfs->next_device = pgs->device;
    code = df->push(df, mem, &new_dev, pgs->device);
    if (code < 0) {
	return code;
	gs_free_object(mem, dfs, "gs_push_device_filter");
    }
    dfs->next = pgs->dfilter_stack;
    pgs->dfilter_stack = dfs;
    dfs->df = df;
    gs_setdevice_no_init(pgs, new_dev);
    rc_decrement_only(new_dev, "gs_push_device_filter");
    return code;
}

int
gs_pop_device_filter(gs_memory_t *mem, gs_state *pgs)
{
    gs_device_filter_stack_t *dfs_tos = pgs->dfilter_stack;
    gs_device_filter_t *df;
    int code;

    if (dfs_tos == NULL)
	return_error(gs_error_rangecheck);
    df = dfs_tos->df;
    code = df->pop(df, mem, pgs, pgs->device);
    pgs->dfilter_stack = dfs_tos->next;
    gs_setdevice_no_init(pgs, dfs_tos->next_device);
    rc_decrement_only(dfs_tos->next_device, "gs_pop_device_filter");
    gs_free_object(mem, dfs_tos, "gs_pop_device_filter");
    return code;
}

int
gs_clear_device_filters(gs_memory_t *mem, gs_state *pgs)
{
    int code;

    while (pgs->dfilter_stack != NULL) {
	if ((code = gs_pop_device_filter(mem, pgs)) < 0)
	    return code;
    }
    return 0;
}

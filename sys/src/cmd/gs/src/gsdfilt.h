/*
  Copyright (C) 2001 artofcode LLC.
  
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

  Author: Raph Levien <raph@artofcode.com>
*/
/* $Id: gsdfilt.h,v 1.9 2003/08/15 22:32:02 raph Exp $ */

#ifndef gsdfilt_INCLUDED
#  define gsdfilt_INCLUDED

/* The device filter stack lives in the gs_state structure. It represents
   a chained sequence of devices that filter device requests, each forwarding
   to its target. The last such target is the physical device as set by
   setpagedevice.

   There is a "shadow" gs_device_filter_stack_s object for each device in
   the chain. The stack management uses these objects to keep track of the
   chain.
*/

#ifndef gs_device_filter_stack_DEFINED
#  define gs_device_filter_stack_DEFINED
typedef struct gs_device_filter_stack_s gs_device_filter_stack_t;
#endif

#ifndef gs_device_filter_DEFINED
#  define gs_device_filter_DEFINED
typedef struct gs_device_filter_s gs_device_filter_t;
#endif

struct gs_device_filter_s {
    int (*push)(gs_device_filter_t *self, gs_memory_t *mem, gs_state *pgs,
		gx_device **pdev, gx_device *target);
    int (*prepop)(gs_device_filter_t *self, gs_memory_t *mem, gs_state *pgs,
		  gx_device *dev);
    int (*postpop)(gs_device_filter_t *self, gs_memory_t *mem, gs_state *pgs,
		   gx_device *dev);
};

extern_st(st_gs_device_filter);

/**
 * gs_push_device_filter: Push a device filter.
 * @mem: Memory for creating device filter.
 * @pgs: Graphics state.
 * @df: The device filter.
 *
 * Pushes a device filter, thereby becoming the first in the chain.
 *
 * Return value: 0 on success, or error code.
 **/
int gs_push_device_filter(gs_memory_t *mem, gs_state *pgs, gs_device_filter_t *df);

/**
 * gs_pop_device_filter: Pop a device filter.
 * @mem: Memory in which device filter was created, for freeing.
 * @pgs: Graphics state.
 *
 * Removes the topmost device filter (ie, first filter in the chain)
 * from the graphics state's device filter stack.
 *
 * Return value: 0 on success, or error code.
 **/
int gs_pop_device_filter(gs_memory_t *mem, gs_state *pgs);

/**
 * gs_clear_device_filters: Clear device filters from a graphics state.
 * @mem: Memory in which device filters were created, for freeing.
 * @pgs: Graphics state.
 *
 * Clears all device filters from the given graphics state.
 *
 * Return value: 0 on success, or error code.
 **/
int gs_clear_device_filters(gs_memory_t *mem, gs_state *pgs);


#endif /* gsdfilt_INCLUDED */

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
/*$Id: gsdfilt.h,v 1.2 2001/04/01 00:30:41 raph Exp $ */

/* The device filter stack lives in the gs_state structure. It represents
   a chained sequence of devices that filter device requests, each forwarding
   to its target. The last such target is the physical device as set by
   setpagedevice.

   There is a "shadow" gs_device_filter_stack_s object for each device in
   the chain. The stack management uses these objects to keep track of the
   chain.
*/

#define DFILTER_TEST

#ifndef gs_device_filter_stack_DEFINED
#  define gs_device_filter_stack_DEFINED
typedef struct gs_device_filter_stack_s gs_device_filter_stack_t;
#endif

/* This is the base structure from which device filters are derived. */
typedef struct gs_device_filter_s gs_device_filter_t;

struct gs_device_filter_s {
    int (*push)(gs_device_filter_t *self, gs_memory_t *mem,
		gx_device **pdev, gx_device *target);
    int (*pop)(gs_device_filter_t *self, gs_memory_t *mem, gs_state *pgs,
	       gx_device *dev);
};

extern_st(st_gs_device_filter);

#ifdef DFILTER_TEST
int gs_test_device_filter(gs_device_filter_t **pdf, gs_memory_t *mem);
#endif


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


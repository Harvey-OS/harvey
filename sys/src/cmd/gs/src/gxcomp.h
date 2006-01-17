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

/* $Id: gxcomp.h,v 1.7 2005/03/14 18:08:36 dan Exp $ */
/* Definitions for implementing compositing functions */

#ifndef gxcomp_INCLUDED
#  define gxcomp_INCLUDED

#include "gscompt.h"
#include "gsrefct.h"
#include "gxbitfmt.h"

/*
 * Because compositor information is passed through the command list,
 * individual compositors must be identified by some means that is
 * independent of address space. The address of the compositor method
 * array, gs_composite_type_t (see below), cannot be used, as it is
 * meaningful only within a single address space.
 *
 * In addition, it is desirable the keep the compositor identifier
 * size as small as possible, as this identifier must be passed through
 * the command list for all bands whenever the compositor is invoked.
 * Fortunately, compositor invocation is not so frequent as to warrant
 * byte-sharing techniques, which most likely would store the identifier
 * in some unused bits of a command code byte. Hence, the smallest
 * reasonable size for the identifier is one byte. This allows for up
 * to 255 compositors, which should be ample (as of this writing, there
 * are only two compositors, only one of which can be passed through
 * the command list).
 *
 * The following list is intended to enumerate all compositors. We
 * use definitions rather than an encoding to ensure a one-byte size.
 */
#define GX_COMPOSITOR_ALPHA        0x01   /* DPS/Next alpha compositor */
#define GX_COMPOSITOR_OVERPRINT    0x02   /* overprint/overprintmode compositor */
#define GX_COMPOSITOR_PDF14_TRANS  0x03   /* PDF 1.4 transparency compositor */


/*
 * Define the abstract superclass for all compositing function types.
 */
						   /*typedef struct gs_composite_s gs_composite_t; *//* in gscompt.h */

#ifndef gs_imager_state_DEFINED
#  define gs_imager_state_DEFINED
typedef struct gs_imager_state_s gs_imager_state;

#endif

#ifndef gx_device_DEFINED
#  define gx_device_DEFINED
typedef struct gx_device_s gx_device;

#endif

typedef struct gs_composite_type_procs_s {

    /*
     * Create the default compositor for a compositing function.
     */
#define composite_create_default_compositor_proc(proc)\
  int proc(const gs_composite_t *pcte, gx_device **pcdev,\
    gx_device *dev, gs_imager_state *pis, gs_memory_t *mem)
    composite_create_default_compositor_proc((*create_default_compositor));

    /*
     * Test whether this function is equal to another one.
     */
#define composite_equal_proc(proc)\
  bool proc(const gs_composite_t *pcte, const gs_composite_t *pcte2)
    composite_equal_proc((*equal));

    /*
     * Convert the representation of this function to a string
     * for writing in a command list.  *psize is the amount of space
     * available.  If it is large enough, the procedure sets *psize
     * to the amount used and returns 0; if it is not large enough,
     * the procedure sets *psize to the amount needed and returns a
     * rangecheck error; in the case of any other error, *psize is
     * not changed.
     */
#define composite_write_proc(proc)\
  int proc(const gs_composite_t *pcte, byte *data, uint *psize)
    composite_write_proc((*write));

    /*
     * Convert the string representation of a function back to
     * a structure, allocating the structure. Return the number of
     * bytes read, or < 0 in the event of an error.
     */
#define composite_read_proc(proc)\
  int proc(gs_composite_t **ppcte, const byte *data, uint size,\
    gs_memory_t *mem)
    composite_read_proc((*read));

    /*
     * Update the clist write device when a compositor device is created.
     */
#define composite_clist_write_update(proc)\
  int proc(const gs_composite_t * pcte, gx_device * dev, gx_device ** pcdev,\
			gs_imager_state * pis, gs_memory_t * mem)
    composite_clist_write_update((*clist_compositor_write_update));

    /*
     * Update the clist read device when a compositor device is created.
     */
#define composite_clist_read_update(proc)\
  int proc(gs_composite_t * pcte, gx_device * cdev, gx_device * tdev,\
			gs_imager_state * pis, gs_memory_t * mem)
    composite_clist_read_update((*clist_compositor_read_update));

} gs_composite_type_procs_t;

typedef struct gs_composite_type_s {
    byte comp_id;   /* to identify compositor passed through command list */
    gs_composite_type_procs_t procs;
} gs_composite_type_t;

/*
 * Default implementation for creating a compositor for clist writing.
 * The default does nothing.
 */
composite_clist_write_update(gx_default_composite_clist_write_update);

/*
 * Default implementation for adjusting the clist reader when a compositor
 * device is added.  The default does nothing.
 */
composite_clist_read_update(gx_default_composite_clist_read_update);

/*
 * Compositing objects are reference-counted, because graphics states will
 * eventually reference them.  Note that the common part has no
 * garbage-collectible pointers and is never actually instantiated, so no
 * structure type is needed for it.
 */
#define gs_composite_common\
	const gs_composite_type_t *type;\
	gs_id id;		/* see gscompt.h */\
	rc_header rc
struct gs_composite_s {
    gs_composite_common;
};

/* Replace a procedure with a macro. */
#define gs_composite_id(pcte) ((pcte)->id)

#endif /* gxcomp_INCLUDED */

/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gshtx.h,v 1.6 2002/08/22 07:12:29 henrys Exp $ */
/* High-level interface to stand-alone halftone/transfer objects */

#ifndef gshtx_INCLUDED
#  define gshtx_INCLUDED

#include "gsmemory.h"
#include "gsht1.h"
#include "gxtmap.h"
#include "gscspace.h"

/*
 * The stand-alone halftone structures are opaque, and are placed in an opaque
 * graphic state. 
 */

/* Alias type names */
#define gs_ht gs_halftone
#define gs_spot_ht gs_spot_halftone
#define gs_threshold_ht gs_threshold_halftone
#define gs_ht_component gs_halftone_component
#define gs_multiple_ht gs_multiple_halftone
/* Alias GC descriptors */
#define st_gs_ht st_halftone
#define st_ht_comp_element st_ht_component_element
/* Alias member names */
#define ht_spot spot
#define ht_threshold threshold
#define ht_multiple multiple

#ifndef gs_state_DEFINED
#  define gs_state_DEFINED
typedef struct gs_state_s gs_state;

#endif

/*
 * A "closure" form of gs_mapping_proc. This allows the procedure to access
 * client data for the purpose of filling in the transfer information.
 *
 * As with PostScript transfer functions, the operand will be in the range
 * [0, 1], and the result should be in the same range.
 */
typedef gs_mapping_closure_proc_t gs_ht_transfer_proc;	/* see gxtmap.h */

/*
 * Constructor, destructor, assign, and copy routines for a gs_ht
 * structure, and to install them in the graphic state.
 *
 * Notes:
 *
 *    Construction of a gs_ht halftone requires two steps: creating the
 *    overall halftone, and creating each of the components. Client data
 *    must be provided for each of the latter steps.
 *
 *    The type field of gs_ht halftones will always be ht_type_multiple;
 *    if only one component is required, this halftone will always be given
 *    the component name "Default".
 *
 *    The type fields of the gs_ht_component structures pointed to by the
 *    gs_multiple_ht structure will have the value ht_type_spot or
 *    ht_type_threshold; the constructor routines will not build any
 *    other types.
 *
 *    Individual component halftones of a gs_ht structure must always be
 *    provided with transfer functions.
 *
 *    Releasing the gs_ht structure will NOT release the client data 
 *    (the client must do that directly).
 */

extern int gs_ht_build(gs_ht ** ppht, uint num_comps, gs_memory_t * pmem);

extern int gs_ht_set_spot_comp(
				     gs_ht * pht,
				     int component_index,
				     floatp freq,
				     floatp angle,
				     float (*spot_func) (floatp, floatp),
				     bool accurate,
				     gs_ht_transfer_proc transfer,
				     const void *client_data
			       );

extern int gs_ht_set_threshold_comp(
					  gs_ht * pht,
					  int component_index,
					  int width,
					  int height,
					  const gs_const_string * thresholds,
					  gs_ht_transfer_proc transfer,
					  const void *client_data
				    );

/*
 * This procedure specifies a (possibly non-monotonic) halftone of size
 * width x height with num_levels different levels (including white, always
 * all 0s, but excluding black, always all 1s).  Each mask is in the form of
 * a gs_bitmap, except that there is no row padding -- the 'raster' is
 * ceil(width / 8).
 *
 * Note that the client is responsible for releasing the mask data.
 */
extern int gs_ht_set_mask_comp(
				     gs_ht * pht,
				     int component_index,
				     int width,
				     int height,
				     int num_levels,
				     const byte * masks,	/* width x height x num_levels */
				     gs_ht_transfer_proc transfer,
				     const void *client_data
			       );

extern void gs_ht_reference(gs_ht * pht);
extern void gs_ht_release(gs_ht * pht);

#define gs_ht_assign(pto, pfrom)    \
    BEGIN                           \
        gs_ht_reference(pfrom);     \
        if (pto != 0)               \
            gs_ht_release(pto);     \
        pto = pfrom;                \
    END

#define gs_ht_init_ptr(pto, pfrom)          \
    BEGIN gs_ht_reference(pfrom); pto = pfrom; END

extern int gs_ht_install(gs_state * pgs, gs_ht * pht);

#endif /* gshtx_INCLUDED */

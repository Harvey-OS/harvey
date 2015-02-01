/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Copyright (C) 1994, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxtmap.h,v 1.5 2002/06/16 08:45:43 lpd Exp $ */
/* Definition of transfer mapping function */
/* (also used for black generation and undercolor removal) */

#ifndef gxtmap_INCLUDED
#  define gxtmap_INCLUDED

/* Common definition for mapping procedures. */
/* These are used for transfer functions, black generation, */
/* and undercolor removal. */
/* gx_transfer_map should probably be renamed gx_mapping_cache.... */

/* Define an abstract type for a transfer map. */
typedef struct gx_transfer_map_s gx_transfer_map;

/*
 * Define the type of a mapping procedure.  There are two forms of this.
 * The original form passed only the transfer map itself as an argument:
 */
typedef float (*gs_mapping_proc) (floatp, const gx_transfer_map *);

/*
 * Later, we recognized that this procedure should really be a general
 * closure:
 */
typedef float (*gs_mapping_closure_proc_t) (floatp value,
					    const gx_transfer_map * pmap,
					    const void *proc_data);
typedef struct gs_mapping_closure_s {
    gs_mapping_closure_proc_t proc;
    const void *data;
} gs_mapping_closure_t;

#endif /* gxtmap_INCLUDED */

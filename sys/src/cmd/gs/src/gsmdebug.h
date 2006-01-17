/* Copyright (C) 1995, 1996, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsmdebug.h,v 1.6 2002/06/16 08:45:42 lpd Exp $ */
/* Allocator debugging definitions and interface */
/* Requires gdebug.h (for gs_debug) */

#ifndef gsmdebug_INCLUDED
#  define gsmdebug_INCLUDED

/* Define the fill patterns used for debugging the allocator. */
extern const byte
       gs_alloc_fill_alloc,	/* allocated but not initialized */
       gs_alloc_fill_block,	/* locally allocated block */
       gs_alloc_fill_collected,	/* garbage collected */
       gs_alloc_fill_deleted,	/* locally deleted block */
       gs_alloc_fill_free;	/* freed */

/* Define an alias for a specialized debugging flag */
/* that used to be a separate variable. */
#define gs_alloc_debug gs_debug['@']

/* Conditionally fill unoccupied blocks with a pattern. */
extern void gs_alloc_memset(void *, int /*byte */ , ulong);

#ifdef DEBUG
#  define gs_alloc_fill(ptr, fill, len)\
     BEGIN if ( gs_alloc_debug ) gs_alloc_memset(ptr, fill, (ulong)(len)); END
#else
#  define gs_alloc_fill(ptr, fill, len)\
     DO_NOTHING
#endif

#endif /* gsmdebug_INCLUDED */

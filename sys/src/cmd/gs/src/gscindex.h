/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gscindex.h,v 1.5 2002/06/16 08:45:42 lpd Exp $ */
/* Client interface to Indexed color facilities */

#ifndef gscindex_INCLUDED
#  define gscindex_INCLUDED

#include "gscspace.h"

/*
 * Indexed color spaces.
 *
 * If the color space will use a procedure rather than a byte table,
 * ptbl should be set to 0.
 *
 * Unlike most of the other color space constructors, this one initializes
 * some of the fields of the colorspace. In the case in which a string table
 * is used for mapping, it initializes the entire structure. Note that the
 * client is responsible for the table memory in that case; the color space
 * will not free it when the color space itself is released.
 *
 * For the case of an indexed color space based on a procedure, a default
 * procedure will be provided that simply echoes the color values already in
 * the palette; the client may override these procedures by use of
 * gs_cspace_indexed_set_proc. If the client wishes to insert values into
 * the palette, it should do so by using gs_cspace_indexed_value_array, and
 * directly inserting the desired values into the array.
 *
 * If the client does insert values into the palette directly, the default
 * procedures provided by the client are fairly efficient, and there are
 * few instances in which the client would need to replace them.
 */
extern int gs_cspace_build_Indexed(
				   gs_color_space ** ppcspace,
				   const gs_color_space * pbase_cspace,
				   uint num_entries,
				   const gs_const_string * ptbl,
				   gs_memory_t * pmem
				   );

/* Return the number of entries in the palette of an indexed color space. */
extern int gs_cspace_indexed_num_entries(
					 const gs_color_space * pcspace
					 );

/* In the case of a procedure-based indexed color space, get a pointer to */
/* the array of cached values. */
extern float *gs_cspace_indexed_value_array(
					    const gs_color_space * pcspace
					    );

/* Set the lookup procedure to be used for an Indexed color space. */
extern int gs_cspace_indexed_set_proc(
				      gs_color_space * pcspace,
				      int (*proc) (const gs_indexed_params *, int, float *)
				      );

/* Look up an index in an Indexed color space. */
int gs_cspace_indexed_lookup(const gs_indexed_params *, int,
			     gs_client_color *);

#endif /* gscindex_INCLUDED */

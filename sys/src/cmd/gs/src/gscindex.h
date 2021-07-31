/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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
*/

/*$Id: gscindex.h,v 1.2 2000/09/19 19:00:26 lpd Exp $ */
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
extern int gs_cspace_build_Indexed(P5(
				      gs_color_space ** ppcspace,
				      const gs_color_space * pbase_cspace,
				      uint num_entries,
				      const gs_const_string * ptbl,
				      gs_memory_t * pmem
				      ));

/* Return the number of entries in the palette of an indexed color space. */
extern int gs_cspace_indexed_num_entries(P1(
					       const gs_color_space * pcspace
					 ));

/* In the case of a procedure-based indexed color space, get a pointer to */
/* the array of cached values. */
extern float *gs_cspace_indexed_value_array(P1(
					      const gs_color_space * pcspace
					    ));

/* Set the lookup procedure to be used for an Indexed color space. */
extern int gs_cspace_indexed_set_proc(P2(
					    gs_color_space * pcspace,
		   int (*proc) (P3(const gs_indexed_params *, int, float *))
				      ));

/* Look up an index in an Indexed color space. */
int gs_cspace_indexed_lookup(P3(const gs_indexed_params *, int,
				gs_client_color *));

#endif /* gscindex_INCLUDED */

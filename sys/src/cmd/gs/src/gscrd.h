/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.

   This file is part of Aladdin Ghostscript.

   Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
   or distributor accepts any responsibility for the consequences of using it,
   or for whether it serves any particular purpose or works at all, unless he
   or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
   License (the "License") for full details.

   Every copy of Aladdin Ghostscript must include a copy of the License,
   normally in a plain ASCII text file named PUBLIC.  The License grants you
   the right to copy, modify and redistribute Aladdin Ghostscript, but only
   under certain conditions described in the License.  Among other things, the
   License requires that the copyright notice and this notice be preserved on
   all copies.
 */

/*$Id: gscrd.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Interface for CIE color rendering dictionary creation */

#ifndef gscrd_INCLUDED
#  define gscrd_INCLUDED

#include "gscie.h"

/*
 * Allocate and minimally initialize a CRD.  Note that this procedure sets
 * the reference count of the structure to 1, not 0.  gs_setcolorrendering
 * will increment the reference count again, so unless you want the
 * structure to stay allocated permanently (or until a garbage collection),
 * you should call rc_decrement(pcrd, "client name") *after* calling
 * gs_setcolorrendering.
 */
int
    gs_cie_render1_build(P3(gs_cie_render ** ppcrd, gs_memory_t * mem,
			    client_name_t cname));

/*
 * Initialize a CRD given all of the relevant parameters.
 * Any of the pointers except WhitePoint may be zero, meaning
 * use the default values.
 *
 * The actual point, matrix, range, and procedure values are copied into the
 * CRD, but only the pointer to the color lookup table
 * (RenderTable.lookup.table) is copied, not the table itself.
 *
 * If pfrom_crd is not NULL, then if the EncodeLMN, EncodeABC, or
 * RenderTable.T procedures indicate that the values exist only in the
 * cache, the corresponding values will be copied from pfrom_crd.
 * Note that NULL values for the individual pointers still represent
 * default values.
 */
int
    gs_cie_render1_init_from(P15(gs_cie_render * pcrd, void *client_data,
				 const gs_cie_render * pfrom_crd,
				 const gs_vector3 * WhitePoint,
				 const gs_vector3 * BlackPoint,
				 const gs_matrix3 * MatrixPQR,
				 const gs_range3 * RangePQR,
				 const gs_cie_transform_proc3 * TransformPQR,
				 const gs_matrix3 * MatrixLMN,
				 const gs_cie_render_proc3 * EncodeLMN,
				 const gs_range3 * RangeLMN,
				 const gs_matrix3 * MatrixABC,
				 const gs_cie_render_proc3 * EncodeABC,
				 const gs_range3 * RangeABC,
				 const gs_cie_render_table_t * RenderTable));
/*
 * Initialize a CRD without the option of copying cached values.
 */
int
    gs_cie_render1_initialize(P14(gs_cie_render * pcrd, void *client_data,
				  const gs_vector3 * WhitePoint,
				  const gs_vector3 * BlackPoint,
				  const gs_matrix3 * MatrixPQR,
				  const gs_range3 * RangePQR,
				  const gs_cie_transform_proc3 * TransformPQR,
				  const gs_matrix3 * MatrixLMN,
				  const gs_cie_render_proc3 * EncodeLMN,
				  const gs_range3 * RangeLMN,
				  const gs_matrix3 * MatrixABC,
				  const gs_cie_render_proc3 * EncodeABC,
				  const gs_range3 * RangeABC,
				  const gs_cie_render_table_t * RenderTable));

/*
 * Set or access the client_data pointer in a CRD.
 * The macro is an L-value.
 */
#define gs_cie_render_client_data(pcrd) ((pcrd)->client_data)

#endif /* gscrd_INCLUDED */

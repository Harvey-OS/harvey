/* Copyright (C) 2001 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpdfc.h,v 1.4 2002/05/07 19:56:46 lpd Exp $ */
/* Internal color space writing interfaces for pdfwrite driver. */

#ifndef gdevpdfc_INCLUDED
#  define gdevpdfc_INCLUDED

/* ------ Exported by gdevpdfc.c for gdevpdfk.c ------ */

/* Define the special cases for CIEBased spaces. */
typedef enum {
    ONE_STEP_NOT,		/* not one-step */
    ONE_STEP_LMN,		/* DecodeLMN (scalar cache) + matrix */
    ONE_STEP_ABC		/* DecodeABC (vector cache) + matrix */
} cie_cache_one_step_t;

/*
 * Finish creating a CIE-based color space (Calxxx or Lab.)
 */
int pdf_finish_cie_space(cos_array_t *pca, cos_dict_t *pcd,
			 const gs_cie_common *pciec);

/* ------ Exported by gdevpdfk.c for gdevpdfc.c ------ */

/*
 * Create an ICCBased color space.  This is a single-use procedure,
 * broken out only for readability.
 */
int pdf_iccbased_color_space(gx_device_pdf *pdev, cos_value_t *pvalue,
			     const gs_color_space *pcs, cos_array_t *pca);

/*
 * Convert a CIEBased space to Lab or ICCBased.
 */
int pdf_convert_cie_space(gx_device_pdf *pdev, cos_array_t *pca,
			  const gs_color_space *pcs, const char *dcsname,
			  const gs_cie_common *pciec, const gs_range *prange,
			  cie_cache_one_step_t one_step,
			  const gs_matrix3 *pmat, const gs_range_t **pprange);

/*
 * Create a Lab color space object.
 */
int pdf_put_lab_color_space(cos_array_t *pca, cos_dict_t *pcd,
			    const gs_range ranges[3] /* only [1] and [2] used */);

#endif /* gdevpdfc_INCLUDED */

/* Copyright (C) 1999, 2000, 2001 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpdfc.c,v 1.54 2005/10/18 09:05:58 leonardo Exp $ */
/* Color space management and writing for pdfwrite driver */
#include "math_.h"
#include "memory_.h"
#include "gx.h"
#include "gscspace.h"		/* for gscie.h */
#include "gscdevn.h"
#include "gscie.h"
#include "gscindex.h"
#include "gscsepr.h"
#include "stream.h"
#include "gsicc.h"
#include "gserrors.h"
#include "gdevpdfx.h"
#include "gdevpdfg.h"
#include "gdevpdfc.h"
#include "gdevpdfo.h"
#include "strimpl.h"
#include "sstring.h"
#include "gxcspace.h"
#include <assert.h>

/*
 * PDF doesn't have general CIEBased color spaces.  However, it provides
 * two methods for handling general CIE spaces:
 *
 *	- For PDF 1.2 and above, we note that the transformation from L*a*b*
 *	space to XYZ space is invertible, so we can handle any PostScript
 *	CIEBased space by transforming color values in that space to XYZ,
 *	then inverse-transforming them to L*a*b* and using a PDF Lab space
 *	with the same WhitePoint and BlackPoint and appropriate ranges for
 *	a and b.  This approach has the drawback that Y values outside the
 *	range [0..1] can't be represented: we just clamp them.
 *
 *	- For PDF 1.3 and above, we can create an ICCBased space.  This is
 *	actually necessary, not just an option, because for shadings (also
 *	introduced in PDF 1.3), we want color interpolation to occur in the
 *	original space.
 *
 * The Lab approach is not currently implemented, because it requires
 * transforming all the sample values of images.  The ICCBased approach is
 * implemented for color spaces whose ranges lie within [0..1], which are
 * the only ranges supported by the ICC standard: we think that removing
 * this limitation would also require transforming image sample values.
 */

/* GC descriptors */
public_st_pdf_color_space();

/* ------ CIE space testing ------ */

/* Test whether a cached CIE procedure is the identity function. */
#define CIE_CACHE_IS_IDENTITY(pc)\
  ((pc)->floats.params.is_identity)
#define CIE_CACHE3_IS_IDENTITY(pca)\
  (CIE_CACHE_IS_IDENTITY(&(pca)[0]) &&\
   CIE_CACHE_IS_IDENTITY(&(pca)[1]) &&\
   CIE_CACHE_IS_IDENTITY(&(pca)[2]))

/*
 * Test whether a cached CIE procedure is an exponential.  A cached
 * procedure is exponential iff f(x) = k*(x^p).  We make a very cursory
 * check for this: we require that f(0) = 0, set k = f(1), set p =
 * log[a](f(a)/k), and then require that f(b) = k*(b^p), where a and b are
 * two arbitrarily chosen values between 0 and 1.  Naturally all this is
 * done with some slop.
 */
#define CC_INDEX_A (gx_cie_cache_size / 3)
#define CC_INDEX_B (gx_cie_cache_size * 2 / 3)
#define CC_INDEX_1 (gx_cie_cache_size - 1)
#define CC_KEY(i) ((i) / (double)CC_INDEX_1)
#define CC_KEY_A CC_KEY(CC_INDEX_A)
#define CC_KEY_B CC_KEY(CC_INDEX_B)

private bool
cie_values_are_exponential(floatp v0, floatp va, floatp vb, floatp k,
			   float *pexpt)
{
    double p;

    if (fabs(v0) >= 0.001 || fabs(k) < 0.001)
	return false;
    if (va == 0 || (va > 0) != (k > 0))
	return false;
    p = log(va / k) / log(CC_KEY_A);
    if (fabs(vb - k * pow(CC_KEY_B, p)) >= 0.001)
	return false;
    *pexpt = p;
    return true;
}

private bool
cie_scalar_cache_is_exponential(const gx_cie_scalar_cache * pc, float *pexpt)
{
    return cie_values_are_exponential(pc->floats.values[0],
				      pc->floats.values[CC_INDEX_A],
				      pc->floats.values[CC_INDEX_B],
				      pc->floats.values[CC_INDEX_1],
				      pexpt);
}
#define CIE_SCALAR3_CACHE_IS_EXPONENTIAL(pca, expts)\
  (cie_scalar_cache_is_exponential(&(pca)[0], &(expts).u) &&\
   cie_scalar_cache_is_exponential(&(pca)[1], &(expts).v) &&\
   cie_scalar_cache_is_exponential(&(pca)[2], &(expts).w))

private bool
cie_vector_cache_is_exponential(const gx_cie_vector_cache * pc, float *pexpt)
{
    return cie_values_are_exponential(pc->vecs.values[0].u,
				      pc->vecs.values[CC_INDEX_A].u,
				      pc->vecs.values[CC_INDEX_B].u,
				      pc->vecs.values[CC_INDEX_1].u,
				      pexpt);
}
#define CIE_VECTOR3_CACHE_IS_EXPONENTIAL(pca, expts)\
  (cie_vector_cache_is_exponential(&(pca)[0], &(expts).u) &&\
   cie_vector_cache_is_exponential(&(pca)[1], &(expts).v) &&\
   cie_vector_cache_is_exponential(&(pca)[2], &(expts).w))

#undef CC_INDEX_A
#undef CC_INDEX_B
#undef CC_KEY_A
#undef CC_KEY_B

/*
 * Test whether a cached CIEBasedABC space consists only of a single
 * Decode step followed by a single Matrix step.
 */
private cie_cache_one_step_t
cie_cached_abc_is_one_step(const gs_cie_abc *pcie, const gs_matrix3 **ppmat)
{
    /* The order of steps is, DecodeABC, MatrixABC, DecodeLMN, MatrixLMN. */
    
    if (CIE_CACHE3_IS_IDENTITY(pcie->common.caches.DecodeLMN)) {
	if (pcie->MatrixABC.is_identity) {
	    *ppmat = &pcie->common.MatrixLMN;
	    return ONE_STEP_ABC;
	}
	if (pcie->common.MatrixLMN.is_identity) {
	    *ppmat = &pcie->MatrixABC;
	    return ONE_STEP_ABC;
	}
    }
    if (CIE_CACHE3_IS_IDENTITY(pcie->caches.DecodeABC.caches)) {
	if (pcie->MatrixABC.is_identity) {
	    *ppmat = &pcie->common.MatrixLMN;
	    return ONE_STEP_LMN;
	}
    }
    return ONE_STEP_NOT;
}

/*
 * Test whether a cached CIEBasedABC space is a L*a*b* space.
 */
private bool
cie_scalar_cache_is_lab_lmn(const gs_cie_abc *pcie, int i)
{
    double k = CC_KEY(i);
    double g = (k >= 6.0 / 29 ? k * k * k :
		(k - 4.0 / 29) * (108.0 / 841));

#define CC_V(j,i) (pcie->common.caches.DecodeLMN[j].floats.values[i])
#define CC_WP(uvw) (pcie->common.points.WhitePoint.uvw)
  
    return (fabs(CC_V(0, i) - g * CC_WP(u)) < 0.001 &&
	    fabs(CC_V(1, i) - g * CC_WP(v)) < 0.001 &&
	    fabs(CC_V(2, i) - g * CC_WP(w)) < 0.001
	    );

#undef CC_V
#undef CC_WP
}
private bool
cie_vector_cache_is_lab_abc(const gx_cie_vector_cache3_t *pvc, int i)
{
    const gx_cie_vector_cache *const pc3 = pvc->caches;
    double k = CC_KEY(i);
    double l0 = pc3[0].vecs.params.base,
	l = l0 + k * (pc3[0].vecs.params.limit - l0);
    double a0 = pc3[1].vecs.params.base,
	a = a0 + k * (pc3[1].vecs.params.limit - a0);
    double b0 = pc3[2].vecs.params.base,
	b = b0 + k * (pc3[2].vecs.params.limit - b0);

    return (fabs(cie_cached2float(pc3[0].vecs.values[i].u) -
		 (l + 16) / 116) < 0.001 &&
	    fabs(cie_cached2float(pc3[1].vecs.values[i].u) -
		 a / 500) < 0.001 &&
	    fabs(cie_cached2float(pc3[2].vecs.values[i].w) -
		 b / -200) < 0.001
	    );
}

private bool
cie_is_lab(const gs_cie_abc *pcie)
{
    int i;

    /* Check MatrixABC and MatrixLMN. */
    if (!(pcie->MatrixABC.cu.u == 1 && pcie->MatrixABC.cu.v == 1 &&
	  pcie->MatrixABC.cu.w == 1 &&
	  pcie->MatrixABC.cv.u == 1 && pcie->MatrixABC.cv.v == 0 &&
	  pcie->MatrixABC.cv.w == 0 &&
	  pcie->MatrixABC.cw.u == 0 && pcie->MatrixABC.cw.v == 0 &&
	  pcie->MatrixABC.cw.w == -1 &&
	  pcie->common.MatrixLMN.is_identity
	  ))
	return false;

    /* Check DecodeABC and DecodeLMN. */
    for (i = 0; i <= CC_INDEX_1; ++i)
	if (!(cie_vector_cache_is_lab_abc(&pcie->caches.DecodeABC, i) &&
	      cie_scalar_cache_is_lab_lmn(pcie, i)
	      ))
	    return false;

    return true;
}

#undef CC_INDEX_1
#undef CC_KEY

/* Test whether one or more CIE-based ranges are [0..1]. */
private bool
cie_ranges_are_0_1(const gs_range *prange, int n)
{
    int i;

    for (i = 0; i < n; ++i)
	if (prange[i].rmin != 0 || prange[i].rmax != 1)
	    return false;
    return true;
}

/* ------ Utilities ------ */

/* Add a 3-element vector to a Cos array or dictionary. */
private int
cos_array_add_vector3(cos_array_t *pca, const gs_vector3 *pvec)
{
    int code = cos_array_add_real(pca, pvec->u);

    if (code >= 0)
	code = cos_array_add_real(pca, pvec->v);
    if (code >= 0)
	code = cos_array_add_real(pca, pvec->w);
    return code;
}
private int
cos_dict_put_c_key_vector3(cos_dict_t *pcd, const char *key,
			   const gs_vector3 *pvec)
{
    cos_array_t *pca = cos_array_alloc(pcd->pdev, "cos_array_from_vector3");
    int code;

    if (pca == 0)
	return_error(gs_error_VMerror);
    code = cos_array_add_vector3(pca, pvec);
    if (code < 0) {
	COS_FREE(pca, "cos_array_from_vector3");
	return code;
    }
    return cos_dict_put_c_key_object(pcd, key, COS_OBJECT(pca));
}

/*
 * Finish creating a CIE-based color space (Calxxx or Lab.)
 * This procedure is exported for gdevpdfk.c.
 */
int
pdf_finish_cie_space(cos_array_t *pca, cos_dict_t *pcd,
		     const gs_cie_common *pciec)
{
    int code = cos_dict_put_c_key_vector3(pcd, "/WhitePoint",
					  &pciec->points.WhitePoint);

    if (code < 0)
	return code;
    if (pciec->points.BlackPoint.u != 0 ||
	pciec->points.BlackPoint.v != 0 ||
	pciec->points.BlackPoint.w != 0
	) {
	code = cos_dict_put_c_key_vector3(pcd, "/BlackPoint",
					  &pciec->points.BlackPoint);
	if (code < 0)
	    return code;
    }
    return cos_array_add_object(pca, COS_OBJECT(pcd));
}

/* ------ Color space writing ------ */

/* Define standard and short color space names. */
const pdf_color_space_names_t pdf_color_space_names = {
    PDF_COLOR_SPACE_NAMES
};
const pdf_color_space_names_t pdf_color_space_names_short = {
    PDF_COLOR_SPACE_NAMES_SHORT
};

/*
 * Create a local Device{Gray,RGB,CMYK} color space corresponding to the
 * given number of components.
 */
int
pdf_cspace_init_Device(const gs_memory_t *mem, gs_color_space *pcs, int num_components)
{
    switch (num_components) {
    case 1: gs_cspace_init_DeviceGray(mem, pcs); break;
    case 3: gs_cspace_init_DeviceRGB(mem, pcs); break;
    case 4: gs_cspace_init_DeviceCMYK(mem, pcs); break;
    default: return_error(gs_error_rangecheck);
    }
    return 0;
}

/* Create a Separation or DeviceN color space (internal). */
private int
pdf_separation_color_space(gx_device_pdf *pdev,
			   cos_array_t *pca, const char *csname,
			   const cos_value_t *snames,
			   const gs_color_space *alt_space,
			   const gs_function_t *pfn,
			   const pdf_color_space_names_t *pcsn)
{
    cos_value_t v;
    const gs_range_t *ranges;
    int code;

    if ((code = cos_array_add(pca, cos_c_string_value(&v, csname))) < 0 ||
	(code = cos_array_add_no_copy(pca, snames)) < 0 ||
	(code = pdf_color_space(pdev, &v, &ranges, alt_space, pcsn, false)) < 0 ||
	(code = cos_array_add(pca, &v)) < 0 ||
	(code = pdf_function_scaled(pdev, pfn, ranges, &v)) < 0 ||
	(code = cos_array_add(pca, &v)) < 0
	)
	return code;
    return 0;
}

/*
 * Create an Indexed color space.  This is a single-use procedure,
 * broken out only for readability.
 */
private int
pdf_indexed_color_space(gx_device_pdf *pdev, cos_value_t *pvalue,
			const gs_color_space *pcs, cos_array_t *pca)
{
    const gs_indexed_params *pip = &pcs->params.indexed;
    const gs_color_space *base_space =
	(const gs_color_space *)&pip->base_space;
    int num_entries = pip->hival + 1;
    int num_components = gs_color_space_num_components(base_space);
    uint table_size = num_entries * num_components;
    /* Guess at the extra space needed for PS string encoding. */
    uint string_size = 2 + table_size * 4;
    uint string_used;
    byte buf[100];		/* arbitrary */
    stream_AXE_state st;
    stream s, es;
    gs_memory_t *mem = pdev->pdf_memory;
    byte *table;
    byte *palette;
    gs_color_space cs_gray;
    cos_value_t v;
    int code;

    /* PDF doesn't support Indexed color spaces with more than 256 entries. */
    if (num_entries > 256)
	return_error(gs_error_rangecheck);
    if (pdev->CompatibilityLevel < 1.3) {
	switch (gs_color_space_get_index(pcs)) {
	    case gs_color_space_index_Pattern:
	    case gs_color_space_index_Separation:
	    case gs_color_space_index_Indexed:
	    case gs_color_space_index_DeviceN:
		return_error(gs_error_rangecheck);
	    default: DO_NOTHING; 
	}

    }
    table = gs_alloc_string(mem, string_size, "pdf_color_space(table)");
    palette = gs_alloc_string(mem, table_size, "pdf_color_space(palette)");
    if (table == 0 || palette == 0) {
	gs_free_string(mem, palette, table_size,
		       "pdf_color_space(palette)");
	gs_free_string(mem, table, string_size,
		       "pdf_color_space(table)");
	return_error(gs_error_VMerror);
    }
    swrite_string(&s, table, string_size);
    s_init(&es, mem);
    s_init_state((stream_state *)&st, &s_PSSE_template, NULL);
    s_init_filter(&es, (stream_state *)&st, buf, sizeof(buf), &s);
    sputc(&s, '(');
    if (pcs->params.indexed.use_proc) {
	gs_client_color cmin, cmax;
	byte *pnext = palette;
	int i, j;

	/* Find the legal range for the color components. */
	for (j = 0; j < num_components; ++j)
	    cmin.paint.values[j] = (float)min_long,
		cmax.paint.values[j] = (float)max_long;
	gs_color_space_restrict_color(&cmin, base_space);
	gs_color_space_restrict_color(&cmax, base_space);
	/*
	 * Compute the palette values, with the legal range for each
	 * one mapped to [0 .. 255].
	 */
	for (i = 0; i < num_entries; ++i) {
	    gs_client_color cc;

	    gs_cspace_indexed_lookup(&pcs->params.indexed, i, &cc);
	    for (j = 0; j < num_components; ++j) {
		float v = (cc.paint.values[j] - cmin.paint.values[j])
		    * 255 / (cmax.paint.values[j] - cmin.paint.values[j]);

		*pnext++ = (v <= 0 ? 0 : v >= 255 ? 255 : (byte)v);
	    }
	}
    } else
	memcpy(palette, pip->lookup.table.data, table_size);
    if (gs_color_space_get_index(base_space) ==
	gs_color_space_index_DeviceRGB
	) {
	/* Check for an all-gray palette3. */
	int i;

	for (i = table_size; (i -= 3) >= 0; )
	    if (palette[i] != palette[i + 1] ||
		palette[i] != palette[i + 2]
		)
		break;
	if (i < 0) {
	    /* Change the color space to DeviceGray. */
	    for (i = 0; i < num_entries; ++i)
		palette[i] = palette[i * 3];
	    table_size = num_entries;
	    gs_cspace_init_DeviceGray(mem, &cs_gray);
	    base_space = &cs_gray;
	}
    }
    stream_write(&es, palette, table_size);
    gs_free_string(mem, palette, table_size, "pdf_color_space(palette)");
    sclose(&es);
    sflush(&s);
    string_used = (uint)stell(&s);
    table = gs_resize_string(mem, table, string_size, string_used,
			     "pdf_color_space(table)");
    /*
     * Since the array is always referenced by name as a resource
     * rather than being written as a value, even for in-line images,
     * always use the full name for the color space.
     *
     * We don't have to worry about the range of the base space:
     * in PDF, unlike PostScript, the values from the lookup table are
     * scaled automatically.
     */
    if ((code = pdf_color_space(pdev, pvalue, NULL, base_space,
				&pdf_color_space_names, false)) < 0 ||
	(code = cos_array_add(pca,
			      cos_c_string_value(&v, 
						 pdf_color_space_names.Indexed
						 /*pcsn->Indexed*/))) < 0 ||
	(code = cos_array_add(pca, pvalue)) < 0 ||
	(code = cos_array_add_int(pca, pip->hival)) < 0 ||
	(code = cos_array_add_no_copy(pca,
				      cos_string_value(&v, table,
						       string_used))) < 0
	)
	return code;
    return 0;
}

/* 
 * Find a color space resource by seriialized data. 
 */
private pdf_resource_t *
pdf_find_cspace_resource(gx_device_pdf *pdev, const byte *serialized, uint serialized_size) 
{
    pdf_resource_t **pchain = pdev->resources[resourceColorSpace].chains;
    pdf_resource_t *pres;
    int i;
    
    for (i = 0; i < NUM_RESOURCE_CHAINS; i++) {
	for (pres = pchain[i]; pres != 0; pres = pres->next) {
	    const pdf_color_space_t *const ppcs =
		(const pdf_color_space_t *)pres;
	    if (ppcs->serialized_size != serialized_size)
		continue;
	    if (!memcmp(ppcs->serialized, serialized, ppcs->serialized_size))
		return pres;
	}
    }
    return NULL;
}


/*
 * Create a PDF color space corresponding to a PostScript color space.
 * For parameterless color spaces, set *pvalue to a (literal) string with
 * the color space name; for other color spaces, create a cos_array_t if
 * necessary and set *pvalue to refer to it.  In the latter case, if
 * by_name is true, return a string /Rxxxx rather than a reference to
 * the actual object.
 *
 * If ppranges is not NULL, then if  the domain of the color space had
 * to be scaled (to convert a CIEBased space to ICCBased), store a pointer
 * to the ranges in *ppranges, otherwise set *ppranges to 0.
 */
int
pdf_color_space_named(gx_device_pdf *pdev, cos_value_t *pvalue,
		const gs_range_t **ppranges,
		const gs_color_space *pcs,
		const pdf_color_space_names_t *pcsn,
		bool by_name, const byte *res_name, int name_length)
{
    gs_color_space_index csi = gs_color_space_get_index(pcs);
    cos_array_t *pca;
    cos_dict_t *pcd;
    cos_value_t v;
    const gs_cie_common *pciec;
    gs_function_t *pfn;
    const gs_range_t *ranges = 0;
    uint serialized_size;
    byte *serialized = NULL, serialized0[100];
    pdf_resource_t *pres = NULL;
    int code;

    if (ppranges)
	*ppranges = 0;		/* default */
    switch (csi) {
    case gs_color_space_index_DeviceGray:
	cos_c_string_value(pvalue, pcsn->DeviceGray);
	return 0;
    case gs_color_space_index_DeviceRGB:
	cos_c_string_value(pvalue, pcsn->DeviceRGB);
	return 0;
    case gs_color_space_index_DeviceCMYK:
	cos_c_string_value(pvalue, pcsn->DeviceCMYK);
	return 0;
    case gs_color_space_index_Pattern:
	if (!pcs->params.pattern.has_base_space) {
	    cos_c_string_value(pvalue, "/Pattern");
	    return 0;
	}
	break;
    case gs_color_space_index_CIEICC:
        /*
	 * Take a special early exit for unrecognized ICCBased color spaces,
	 * or for PDF 1.2 output (ICCBased color spaces date from PDF 1.3).
	 */
        if (pcs->params.icc.picc_info->picc == 0 ||
	    pdev->CompatibilityLevel < 1.3
	    ) {
	    if (res_name != NULL)
		return 0; /* Ignore .includecolorspace */
            return pdf_color_space( pdev, pvalue, ppranges,
                                    (const gs_color_space *)
                                        &pcs->params.icc.alt_space,
                                    pcsn, by_name);
	}
        break;
    default:
	break;
    }

    /* Check whether we already have a PDF object for this color space. */
    if (pcs->id != gs_no_id)
	pres = pdf_find_resource_by_gs_id(pdev, resourceColorSpace, pcs->id);
    if (pres == NULL) {
	stream s;

	s_init(&s, pdev->memory);
	swrite_position_only(&s);
	code = cs_serialize(pcs, &s);
	if (code < 0)
	    return_error(gs_error_unregistered); /* Must not happen. */
	serialized_size = stell(&s);
	sclose(&s);
	if (serialized_size <= sizeof(serialized0))
	    serialized = serialized0;
	else {
	    serialized = gs_alloc_bytes(pdev->pdf_memory, serialized_size, "pdf_color_space");
	    if (serialized == NULL)
		return_error(gs_error_VMerror);
	}
	swrite_string(&s, serialized, serialized_size);
	code = cs_serialize(pcs, &s);
	if (code < 0)
	    return_error(gs_error_unregistered); /* Must not happen. */
	if (stell(&s) != serialized_size) 
	    return_error(gs_error_unregistered); /* Must not happen. */
	sclose(&s);
	pres = pdf_find_cspace_resource(pdev, serialized, serialized_size);
	if (pres != NULL) {
	    if (serialized != serialized0)
		gs_free_object(pdev->pdf_memory, serialized, "pdf_color_space");
	    serialized = NULL;
	}
    }
    if (pres) {
	const pdf_color_space_t *const ppcs =
	    (const pdf_color_space_t *)pres;

	if (ppranges != 0 && ppcs->ranges != 0)
	    *ppranges = ppcs->ranges;
	pca = (cos_array_t *)pres->object;
	goto ret;
    }

    /* Space has parameters -- create an array. */
    pca = cos_array_alloc(pdev, "pdf_color_space");
    if (pca == 0)
	return_error(gs_error_VMerror);

    switch (csi) {

    case gs_color_space_index_CIEICC:
	code = pdf_iccbased_color_space(pdev, pvalue, pcs, pca);
        break;

    case gs_color_space_index_CIEA: {
	/* Check that we can represent this as a CalGray space. */
	const gs_cie_a *pcie = pcs->params.a;
	bool unitary = cie_ranges_are_0_1(&pcie->RangeA, 1);
	bool identityA = (pcie->MatrixA.u == 1 && pcie->MatrixA.v == 1 && 
	                  pcie->MatrixA.w == 1);
	gs_vector3 expts;

	pciec = (const gs_cie_common *)pcie;
	if (!pcie->common.MatrixLMN.is_identity) {
	    code = pdf_convert_cie_space(pdev, pca, pcs, "GRAY", pciec,
					 &pcie->RangeA, ONE_STEP_NOT, NULL,
					 &ranges);
	    break;
	}
	if (unitary && identityA &&
	    CIE_CACHE_IS_IDENTITY(&pcie->caches.DecodeA) &&
	    CIE_SCALAR3_CACHE_IS_EXPONENTIAL(pcie->common.caches.DecodeLMN, expts) &&
	    expts.v == expts.u && expts.w == expts.u
	    ) {
	    DO_NOTHING;
	} else if (unitary && identityA &&
		   CIE_CACHE3_IS_IDENTITY(pcie->common.caches.DecodeLMN) &&
		   cie_vector_cache_is_exponential(&pcie->caches.DecodeA, &expts.u)
		   ) {
	    DO_NOTHING;
	} else {
	    code = pdf_convert_cie_space(pdev, pca, pcs, "GRAY", pciec,
					 &pcie->RangeA, ONE_STEP_NOT, NULL,
					 &ranges);
	    break;
	}
	code = cos_array_add(pca, cos_c_string_value(&v, "/CalGray"));
	if (code < 0)
	    return code;
	pcd = cos_dict_alloc(pdev, "pdf_color_space(dict)");
	if (pcd == 0)
	    return_error(gs_error_VMerror);
	if (expts.u != 1) {
	    code = cos_dict_put_c_key_real(pcd, "/Gamma", expts.u);
	    if (code < 0)
		return code;
	}
    }
    cal:
    /* Finish handling a CIE-based color space (Calxxx or Lab). */
    if (code < 0)
	return code;
    code = pdf_finish_cie_space(pca, pcd, pciec);
    break;

    case gs_color_space_index_CIEABC: {
	/* Check that we can represent this as a CalRGB space. */
	const gs_cie_abc *pcie = pcs->params.abc;
	bool unitary = cie_ranges_are_0_1(pcie->RangeABC.ranges, 3);
	gs_vector3 expts;
	const gs_matrix3 *pmat = NULL;
	cie_cache_one_step_t one_step =
	    cie_cached_abc_is_one_step(pcie, &pmat);

	pciec = (const gs_cie_common *)pcie;
	if (unitary) {
	    switch (one_step) {
	    case ONE_STEP_ABC:
		if (CIE_VECTOR3_CACHE_IS_EXPONENTIAL(pcie->caches.DecodeABC.caches, expts))
		    goto calrgb;
		break;
	    case ONE_STEP_LMN:
		if (CIE_SCALAR3_CACHE_IS_EXPONENTIAL(pcie->common.caches.DecodeLMN, expts))
		    goto calrgb;
	    default:
		break;
	    }
	}
	if (cie_is_lab(pcie)) {
	    /* Represent this as a Lab space. */
	    pcd = cos_dict_alloc(pdev, "pdf_color_space(dict)");
	    if (pcd == 0)
		return_error(gs_error_VMerror);
	    code = pdf_put_lab_color_space(pca, pcd, pcie->RangeABC.ranges);
	    goto cal;
	} else {
	    code = pdf_convert_cie_space(pdev, pca, pcs, "RGB ", pciec,
					 pcie->RangeABC.ranges,
					 one_step, pmat, &ranges);
	    break;
	}
    calrgb:
	code = cos_array_add(pca, cos_c_string_value(&v, "/CalRGB"));
	if (code < 0)
	    return code;
	pcd = cos_dict_alloc(pdev, "pdf_color_space(dict)");
	if (pcd == 0)
	    return_error(gs_error_VMerror);
	if (expts.u != 1 || expts.v != 1 || expts.w != 1) {
	    code = cos_dict_put_c_key_vector3(pcd, "/Gamma", &expts);
	    if (code < 0)
		return code;
	}
	if (!pmat->is_identity) {
	    cos_array_t *pcma =
		cos_array_alloc(pdev, "pdf_color_space(Matrix)");

	    if (pcma == 0)
		return_error(gs_error_VMerror);
	    if ((code = cos_array_add_vector3(pcma, &pmat->cu)) < 0 ||
		(code = cos_array_add_vector3(pcma, &pmat->cv)) < 0 ||
		(code = cos_array_add_vector3(pcma, &pmat->cw)) < 0 ||
		(code = cos_dict_put(pcd, (const byte *)"/Matrix", 7,
				     COS_OBJECT_VALUE(&v, pcma))) < 0
		)
		return code;
	}
    }
    goto cal;

    case gs_color_space_index_CIEDEF:
	code = pdf_convert_cie_space(pdev, pca, pcs, "RGB ",
				     (const gs_cie_common *)pcs->params.def,
				     pcs->params.def->RangeDEF.ranges,
				     ONE_STEP_NOT, NULL, &ranges);
	break;

    case gs_color_space_index_CIEDEFG:
	code = pdf_convert_cie_space(pdev, pca, pcs, "CMYK",
				     (const gs_cie_common *)pcs->params.defg,
				     pcs->params.defg->RangeDEFG.ranges,
				     ONE_STEP_NOT, NULL, &ranges);
	break;

    case gs_color_space_index_Indexed:
	code = pdf_indexed_color_space(pdev, pvalue, pcs, pca);
	break;

    case gs_color_space_index_DeviceN:
        if (pdev->CompatibilityLevel < 1.3)
	    return_error(gs_error_rangecheck);
	pfn = gs_cspace_get_devn_function(pcs);
	/****** CURRENTLY WE ONLY HANDLE Functions ******/
	if (pfn == 0)
	    return_error(gs_error_rangecheck);
	{
	    cos_array_t *psna = 
		cos_array_alloc(pdev, "pdf_color_space(DeviceN)");
	    int i;
	    byte *name_string;
	    uint name_string_length;

	    if (psna == 0)
		return_error(gs_error_VMerror);
	    for (i = 0; i < pcs->params.device_n.num_components; ++i) {
	 	if ((code = pcs->params.device_n.get_colorname_string(
				  pdev->memory,
				  pcs->params.device_n.names[i], &name_string, 
				  &name_string_length)) < 0 ||
		    (code = pdf_string_to_cos_name(pdev, name_string, 
				  name_string_length, &v)) < 0 ||
		    (code = cos_array_add_no_copy(psna, &v)) < 0)
		    return code;
	    }
	    COS_OBJECT_VALUE(&v, psna);
	    if ((code = pdf_separation_color_space(pdev, pca, "/DeviceN", &v,
						   (const gs_color_space *)
					&pcs->params.device_n.alt_space,
					pfn, &pdf_color_space_names)) < 0)
		return code;
	}
	break;

    case gs_color_space_index_Separation:
	pfn = gs_cspace_get_sepr_function(pcs);
	/****** CURRENTLY WE ONLY HANDLE Functions ******/
	if (pfn == 0)
	    return_error(gs_error_rangecheck);
	{
	    byte *name_string;
	    uint name_string_length;
	    if ((code = pcs->params.separation.get_colorname_string(
				  pdev->memory, 
				  pcs->params.separation.sep_name, &name_string, 
				  &name_string_length)) < 0 ||
		(code = pdf_string_to_cos_name(pdev, name_string, 
				      name_string_length, &v)) < 0 ||
		(code = pdf_separation_color_space(pdev, pca, "/Separation", &v,
						   (const gs_color_space *)
					    &pcs->params.separation.alt_space,
					    pfn, &pdf_color_space_names)) < 0)
		return code;
	}
	break;

    case gs_color_space_index_Pattern:
	if ((code = pdf_color_space(pdev, pvalue, ppranges,
				    (const gs_color_space *)
				    &pcs->params.pattern.base_space,
				    &pdf_color_space_names, false)) < 0 ||
	    (code = cos_array_add(pca,
				  cos_c_string_value(&v, "/Pattern"))) < 0 ||
	    (code = cos_array_add(pca, pvalue)) < 0
	    )
	    return code;
	break;

    default:
	return_error(gs_error_rangecheck);
    }
    /*
     * Register the color space as a resource, since it must be referenced
     * by name rather than directly.
     */
    {
	pdf_color_space_t *ppcs;

	if (code < 0 ||
	    (code = pdf_alloc_resource(pdev, resourceColorSpace, pcs->id,
				       &pres, -1)) < 0
	    ) {
	    COS_FREE(pca, "pdf_color_space");
	    return code;
	}
	pdf_reserve_object_id(pdev, pres, 0);
	if (res_name != NULL) {
	    int l = min(name_length, sizeof(pres->rname) - 1);
	    
	    memcpy(pres->rname, res_name, l);
	    pres->rname[l] = 0;
	}
	ppcs = (pdf_color_space_t *)pres;
	if (serialized == serialized0) {
	    serialized = gs_alloc_bytes(pdev->pdf_memory, serialized_size, "pdf_color_space");
	    if (serialized == NULL)
		return_error(gs_error_VMerror);
	    memcpy(serialized, serialized0, serialized_size);
	}
	ppcs->serialized = serialized;
	ppcs->serialized_size = serialized_size;
	if (ranges) {
	    int num_comp = gs_color_space_num_components(pcs);
	    gs_range_t *copy_ranges = (gs_range_t *)
		gs_alloc_byte_array(pdev->pdf_memory, num_comp,
				    sizeof(gs_range_t), "pdf_color_space");

	    if (copy_ranges == 0) {
		COS_FREE(pca, "pdf_color_space");
		return_error(gs_error_VMerror);
	    }
	    memcpy(copy_ranges, ranges, num_comp * sizeof(gs_range_t));
	    ppcs->ranges = copy_ranges;
	    if (ppranges)
		*ppranges = copy_ranges;
	} else
	    ppcs->ranges = 0;
	pca->id = pres->object->id;
	COS_FREE(pres->object, "pdf_color_space");
	pres->object = (cos_object_t *)pca;
	cos_write_object(COS_OBJECT(pca), pdev);
    }
 ret:
    if (by_name) {
	/* Return a resource name rather than an object reference. */
	discard(COS_RESOURCE_VALUE(pvalue, pca));
    } else
	discard(COS_OBJECT_VALUE(pvalue, pca));
    if (pres != NULL) {
	pres->where_used |= pdev->used_mask;
	code = pdf_add_resource(pdev, pdev->substream_Resources, "/ColorSpace", pres);
	if (code < 0)
	    return code;
    }
    return 0;
}

int
pdf_color_space(gx_device_pdf *pdev, cos_value_t *pvalue,
		const gs_range_t **ppranges,
		const gs_color_space *pcs,
		const pdf_color_space_names_t *pcsn,
		bool by_name)
{
    return pdf_color_space_named(pdev, pvalue, ppranges, pcs, pcsn, by_name, NULL, 0);
}

/* ---------------- Miscellaneous ---------------- */

/* Create colored and uncolored Pattern color spaces. */
private int
pdf_pattern_space(gx_device_pdf *pdev, cos_value_t *pvalue,
		  pdf_resource_t **ppres, const char *cs_name)
{
    int code;

    if (!*ppres) {
	int code = pdf_begin_resource_body(pdev, resourceColorSpace, gs_no_id,
					   ppres);

	if (code < 0)
	    return code;
	pprints1(pdev->strm, "%s\n", cs_name);
	pdf_end_resource(pdev);
	(*ppres)->object->written = true; /* don't write at end */
	((pdf_color_space_t *)*ppres)->ranges = 0;
	((pdf_color_space_t *)*ppres)->serialized = 0;
    }
    code = pdf_add_resource(pdev, pdev->substream_Resources, "/ColorSpace", *ppres);
    if (code < 0)
	return code;
    cos_resource_value(pvalue, (*ppres)->object);
    return 0;
}
int
pdf_cs_Pattern_colored(gx_device_pdf *pdev, cos_value_t *pvalue)
{
    return pdf_pattern_space(pdev, pvalue, &pdev->cs_Patterns[0],
			     "[/Pattern]");
}
int
pdf_cs_Pattern_uncolored(gx_device_pdf *pdev, cos_value_t *pvalue)
{
    /* Only for process colors. */
    int ncomp = pdev->color_info.num_components;
    static const char *const pcs_names[5] = {
	0, "[/Pattern /DeviceGray]", 0, "[/Pattern /DeviceRGB]",
	"[/Pattern /DeviceCMYK]"
    };

    return pdf_pattern_space(pdev, pvalue, &pdev->cs_Patterns[ncomp],
			     pcs_names[ncomp]);
}
int
pdf_cs_Pattern_uncolored_hl(gx_device_pdf *pdev, 
		const gs_color_space *pcs, cos_value_t *pvalue)
{
    /* Only for high level colors. */
    return pdf_color_space(pdev, pvalue, NULL, pcs, &pdf_color_space_names, true);
}

/* Set the ProcSets bits corresponding to an image color space. */
void
pdf_color_space_procsets(gx_device_pdf *pdev, const gs_color_space *pcs)
{
    const gs_color_space *pbcs = pcs;

 csw:
    switch (gs_color_space_get_index(pbcs)) {
    case gs_color_space_index_DeviceGray:
    case gs_color_space_index_CIEA:
	/* We only handle CIEBasedA spaces that map to CalGray. */
	pdev->procsets |= ImageB;
	break;
    case gs_color_space_index_Indexed:
	pdev->procsets |= ImageI;
	pbcs = (const gs_color_space *)&pcs->params.indexed.base_space;
	goto csw;
    default:
	pdev->procsets |= ImageC;
	break;
    }
}

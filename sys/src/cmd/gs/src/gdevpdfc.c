/* Copyright (C) 1999, 2000, 2001 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gdevpdfc.c,v 1.17 2001/08/03 06:43:52 lpd Exp $ */
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
#include "gdevpdfo.h"
#include "strimpl.h"
#include "sstring.h"

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
#define CC_VALUE(i) ((i) / (double)(gx_cie_cache_size - 1))
#define CCX_VALUE_A CC_VALUE(CC_INDEX_A)
#define CCX_VALUE_B CC_VALUE(CC_INDEX_B)

private bool
cie_values_are_exponential(floatp va, floatp vb, floatp k, float *pexpt)
{
    double p;

    if (fabs(k) < 0.001)
	return false;
    if (va == 0 || (va > 0) != (k > 0))
	return false;
    p = log(va / k) / log(CCX_VALUE_A);
    if (fabs(vb - k * pow(CCX_VALUE_B, p)) >= 0.001)
	return false;
    *pexpt = p;
    return true;
}

private bool
cie_scalar_cache_is_exponential(const gx_cie_scalar_cache * pc, float *pexpt)
{
    if (fabs(pc->floats.values[0]) >= 0.001)
	return false;
    return cie_values_are_exponential(pc->floats.values[CC_INDEX_A],
				      pc->floats.values[CC_INDEX_B],
				      pc->floats.values[gx_cie_cache_size - 1],
				      pexpt);
}
#define CIE_SCALAR3_CACHE_IS_EXPONENTIAL(pca, expts)\
  (cie_scalar_cache_is_exponential(&(pca)[0], &(expts).u) &&\
   cie_scalar_cache_is_exponential(&(pca)[1], &(expts).v) &&\
   cie_scalar_cache_is_exponential(&(pca)[2], &(expts).w))

private bool
cie_vector_cache_is_exponential(const gx_cie_vector_cache * pc, float *pexpt)
{
    if (fabs(pc->vecs.values[0].u) >= 0.001)
	return false;
    return cie_values_are_exponential(pc->vecs.values[CC_INDEX_A].u,
				      pc->vecs.values[CC_INDEX_B].u,
				      pc->vecs.values[gx_cie_cache_size - 1].u,
				      pexpt);
}
#define CIE_VECTOR3_CACHE_IS_EXPONENTIAL(pca, expts)\
  (cie_vector_cache_is_exponential(&(pca)[0], &(expts).u) &&\
   cie_vector_cache_is_exponential(&(pca)[1], &(expts).v) &&\
   cie_vector_cache_is_exponential(&(pca)[2], &(expts).w))

#undef CC_INDEX_A
#undef CC_INDEX_B
#undef CC_VALUE
#undef CCX_VALUE_A
#undef CCX_VALUE_B

/* ------ Lab space synthesis ------ */

/*
 * PDF doesn't have general CIEBased color spaces.  However, since the
 * transformation from L*a*b* space to XYZ space is invertible, we can
 * handle any PostScript CIEBased space by transforming color values in
 * that space to XYZ, then inverse-transforming them to L*a*b* and using
 * a L*a*b* space with the same WhitePoint and BlackPoint and appropriate
 * ranges for a* and b*.  This approach has two drawbacks:
 *
 *	- Y values outside the range [0..1] can't be represented: we clamp
 *	them.
 *
 *	- For shadings, color interpolation will occur in the Lab space
 *	rather in the original CIEBased space.  We aren't going to worry
 *	about this.
 */

/* Transform a CIEBased color to XYZ. */
private int
cie_to_xyz(const double *in, double out[3], const gs_color_space *pcs)
{
    /****** NOT IMPLEMENTED YET ******/
    out[0] = in[0], out[1] = in[1], out[2] = in[2];	/****** BOGUS ******/
    return 0;
}

/* Transform XYZ values to Lab. */
private double
lab_g_inverse(double v)
{
    if (v >= (6.0 * 6.0 * 6.0) / (29 * 29 * 29))
	return pow(v, 1.0 / 3);	/* use cbrt if available? */
    else
	return (v * (841.0 / 108) + 4.0 / 29);
}
private void
xyz_to_lab(const double xyz[3], double lab[3], const gs_cie_common *pciec)
{
    const gs_vector3 *const pWhitePoint = &pciec->points.WhitePoint;
    double L, lunit;

    /* Calculate L* first. */
    L = lab_g_inverse(xyz[1] / pWhitePoint->v) * 116 - 16;
    /* Clamp L* to the PDF range [0..100]. */
    if (L < 0)
	L = 0;
    else if (L > 100)
	L = 100;
    lab[1] = L;
    lunit = (L + 16) / 116;

    /* Calculate a* and b*. */
    lab[0] = (lab_g_inverse(xyz[0] / pWhitePoint->u) - lunit) * 500;
    lab[2] = (lab_g_inverse(xyz[2] / pWhitePoint->w) - lunit) * -200;
}

/* Create a PDF Lab color space corresponding to a CIEBased color space. */
private int
lab_range(double lab_min[3], double lab_max[3],
	  const gs_color_space *pcs, const gs_cie_common *pciec)
{
    /*
     * Determine the range of a* and b* by evaluating the color space
     * mapping at all of its extrema.
     */
    int ncomp = gs_color_space_num_components(pcs);
    int i, j;
    const gs_range *ranges;

    switch (gs_color_space_get_index(pcs)) {
    case gs_color_space_index_CIEDEFG:
	ranges = pcs->params.defg->RangeDEFG.ranges;
	break;
    case gs_color_space_index_CIEDEF:
	ranges = pcs->params.def->RangeDEF.ranges;
	break;
    case gs_color_space_index_CIEABC:
	ranges = pcs->params.abc->RangeABC.ranges;
	break;
    case gs_color_space_index_CIEA:
	ranges = &pcs->params.a->RangeA;
	break;
    default:
	return_error(gs_error_rangecheck);
    }

    for (j = 1; j < 3; ++j)
	lab_min[j] = 1000.0, lab_max[j] = -1000.0;
    for (i = 0; i < 1 << ncomp; ++i) {
	double in[4], xyz[3];

	for (j = 0; j < ncomp; ++j)
	    in[j] = (i & (1 << j) ? ranges[j].rmax : ranges[j].rmin);
	if (cie_to_xyz(in, xyz, pcs) >= 0) {
	    double lab[3];

	    xyz_to_lab(xyz, lab, pciec);
	    for (j = 1; j < 3; ++j) {
		lab_min[j] = min(lab_min[j], lab[j]);
		lab_max[j] = max(lab_max[j], lab[j]);
	    }
	}
    }
    return 0;
}
private int
pdf_lab_color_space(cos_array_t *pca, cos_dict_t *pcd,
		    const gs_color_space *pcs, const gs_cie_common *pciec)
{
    double lab_min[3], lab_max[3]; /* only 1 and 2 used */
    cos_array_t *pcar = cos_array_alloc(pca->pdev, "pdf_lab_color_space");
    cos_value_t v;
    int code;

    if (pcar == 0)
	return_error(gs_error_VMerror);
    if ((code = lab_range(lab_min, lab_max, pcs, pciec)) < 0 ||
	(code = cos_array_add(pca, cos_c_string_value(&v, "/Lab"))) < 0 ||
	(code = cos_array_add_real(pcar, lab_min[1])) < 0 ||
	(code = cos_array_add_real(pcar, lab_max[1])) < 0 ||
	(code = cos_array_add_real(pcar, lab_min[2])) < 0 ||
	(code = cos_array_add_real(pcar, lab_max[2])) < 0 ||
	(code = cos_dict_put_c_key_object(pcd, "/Range", COS_OBJECT(pcar))) < 0
	)
	return code;
    return 0;
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
pdf_cspace_init_Device(gs_color_space *pcs, int num_components)
{
    switch (num_components) {
    case 1: gs_cspace_init_DeviceGray(pcs); break;
    case 3: gs_cspace_init_DeviceRGB(pcs); break;
    case 4: gs_cspace_init_DeviceCMYK(pcs); break;
    default: return_error(gs_error_rangecheck);
    }
    return 0;
}

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
    int code;

    if ((code = cos_array_add(pca, cos_c_string_value(&v, csname))) < 0 ||
	(code = cos_array_add_no_copy(pca, snames)) < 0 ||
	(code = pdf_color_space(pdev, &v, alt_space, pcsn, false)) < 0 ||
	(code = cos_array_add(pca, &v)) < 0 ||
	(code = pdf_function(pdev, pfn, &v)) < 0 ||
	(code = cos_array_add(pca, &v)) < 0
	)
	return code;
    return 0;
}

/*
 * Create a PDF color space corresponding to a PostScript color space.
 * For parameterless color spaces, set *pvalue to a (literal) string with
 * the color space name; for other color spaces, create a cos_dict_t if
 * necessary and set *pvalue to refer to it.
 */
int
pdf_color_space(gx_device_pdf *pdev, cos_value_t *pvalue,
		const gs_color_space *pcs,
		const pdf_color_space_names_t *pcsn,
		bool by_name)
{
    gs_memory_t *mem = pdev->pdf_memory;
    gs_color_space_index csi = gs_color_space_get_index(pcs);
    cos_array_t *pca;
    cos_dict_t *pcd;
    cos_value_t v;
    const gs_cie_common *pciec;
    gs_function_t *pfn;
    int code;

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
	    )
            return pdf_color_space( pdev, pvalue,
                                    (const gs_color_space *)
                                        &pcs->params.icc.alt_space,
                                    pcsn, by_name);
        break;
    default:
	break;
    }

    /* Space has parameters -- create an array. */
    pca = cos_array_alloc(pdev, "pdf_color_space");
    if (pca == 0)
	return_error(gs_error_VMerror);
    switch (csi) {
    case gs_color_space_index_CIEICC: {
        /* this would arise only in a pdf ==> pdf translation, but we
         * should allow for it anyway */
        const gs_icc_params * picc_params = &pcs->params.icc;
        const gs_cie_icc * picc_info = picc_params->picc_info;
        int ncomps = picc_info->num_components;
        cos_stream_t * pcstrm;
        int i;
        gs_color_space_index alt_csi;

        /* ICCBased color spaces are essentially copied to the output */
        if ((code = cos_array_add(pca, cos_c_string_value(&v, "/ICCBased"))) < 0)
            return code;

        /* create a stream for the output */
        if ((pcstrm = cos_stream_alloc( pdev, "pdf_color_space(stream)")) == 0)
            return_error(gs_error_VMerror);

        /* indicate the number of components */
        code = cos_dict_put_c_key_int(cos_stream_dict(pcstrm), "/N", ncomps);

        /* indicate the range, if appropriate */
        for (i = 0; i < ncomps && picc_info->Range.ranges[i].rmin == 0.0 &&
             picc_info->Range.ranges[i].rmax == 1.0; i++)
            ;
        if (code >= 0 && i != ncomps) {
            cos_array_t * prngca = cos_array_alloc(pdev,
                                                   "pdf_color_space(Range)");

            if (prngca == 0)
                return_error(gs_error_VMerror);
            for (i = 0; code >= 0 && i < ncomps; i++) {
                code = cos_array_add_int(prngca,
                                         picc_info->Range.ranges[i].rmin);
                if (code >= 0)
                    code = cos_array_add_int(prngca,
                                             picc_info->Range.ranges[i].rmax);
            }
            if (code < 0 || 
                (code = cos_dict_put_c_key_object(cos_stream_dict(pcstrm),
                                                  "/Range",
                                                  COS_OBJECT(prngca))) < 0)
                COS_FREE(prngca, "pcf_colos_space(Range)");
        }

        /* output the alternate color space, if necessary */
        alt_csi = gs_color_space_get_index(pcs);
        if (code >= 0 && alt_csi != gs_color_space_index_DeviceGray &&
            alt_csi != gs_color_space_index_DeviceRGB &&
            alt_csi != gs_color_space_index_DeviceCMYK) {
            code = pdf_color_space(pdev, pvalue, 
                               (const gs_color_space *)&picc_params->alt_space,
                               &pdf_color_space_names, false);
            if (code >= 0)
                code = cos_dict_put_c_key(cos_stream_dict(pcstrm), 
                                          "/Alternate", pvalue);
        }

        /* transfer the profile stream */
        while (code >= 0) {
            byte    sbuff[256];
            uint    cnt;

            code = sgets(picc_info->instrp, sbuff, sizeof(sbuff), &cnt);
            if (cnt == 0)
                break;
            if (code >= 0)
                code = cos_stream_add_bytes(pcstrm, sbuff, cnt);
        }
        if (code >= 0)
            code = cos_array_add(pca, COS_OBJECT_VALUE(&v, pcstrm));
        else
            COS_FREE(pcstrm, "pcf_color_space(ICCBased dictionary)");
        }
        break;

    case gs_color_space_index_CIEA: {
	/* Check that we can represent this as a CalGray space. */
	const gs_cie_a *pcie = pcs->params.a;
	gs_vector3 expts;

	pciec = (const gs_cie_common *)pcie;
	if (!(pcie->MatrixA.u == 1 && pcie->MatrixA.v == 1 &&
	      pcie->MatrixA.w == 1 &&
	      pcie->common.MatrixLMN.is_identity))
	    return_error(gs_error_rangecheck);
	if (CIE_CACHE_IS_IDENTITY(&pcie->caches.DecodeA) &&
	    CIE_SCALAR3_CACHE_IS_EXPONENTIAL(pcie->common.caches.DecodeLMN, expts) &&
	    expts.v == expts.u && expts.w == expts.u
	    ) {
	    DO_NOTHING;
	} else if (CIE_CACHE3_IS_IDENTITY(pcie->common.caches.DecodeLMN) &&
		   cie_vector_cache_is_exponential(&pcie->caches.DecodeA, &expts.u)
		   ) {
	    DO_NOTHING;
	} else
	    goto lab;
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
    code = cos_dict_put_c_key_vector3(pcd, "/WhitePoint",
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
    code = cos_array_add(pca, COS_OBJECT_VALUE(&v, pcd));
    break;
    case gs_color_space_index_CIEABC: {
	/* Check that we can represent this as a CalRGB space. */
	const gs_cie_abc *pcie = pcs->params.abc;
	gs_vector3 expts;
	const gs_matrix3 *pmat;

	pciec = (const gs_cie_common *)pcie;
	if (pcie->common.MatrixLMN.is_identity &&
	    CIE_CACHE3_IS_IDENTITY(pcie->common.caches.DecodeLMN) &&
	    CIE_VECTOR3_CACHE_IS_EXPONENTIAL(pcie->caches.DecodeABC, expts)
	    )
	    pmat = &pcie->MatrixABC;
	else if (pcie->MatrixABC.is_identity &&
		 CIE_CACHE3_IS_IDENTITY(pcie->caches.DecodeABC) &&
		 CIE_SCALAR3_CACHE_IS_EXPONENTIAL(pcie->common.caches.DecodeLMN, expts)
		 )
	    pmat = &pcie->common.MatrixLMN;
	else
	    goto lab;
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
	pciec = (const gs_cie_common *)pcs->params.def;
	goto lab;
    case gs_color_space_index_CIEDEFG:
	pciec = (const gs_cie_common *)pcs->params.defg;
    lab:
	/* Convert all other CIEBased spaces to a Lab space. */
	/****** NOT IMPLEMENTED YET, REQUIRES TRANSFORMING VALUES ******/
	if (1) return_error(gs_error_rangecheck);
	pcd = cos_dict_alloc(pdev, "pdf_color_space(dict)");
	if (pcd == 0)
	    return_error(gs_error_VMerror);
	code = pdf_lab_color_space(pca, pcd, pcs, pciec);
	if (code < 0)
	    return code;
	goto cal;
    case gs_color_space_index_Indexed: {
	const gs_indexed_params *pip = &pcs->params.indexed;
	const gs_color_space *base_space =
	    (const gs_color_space *)&pip->base_space;
	int num_entries = pip->hival + 1;
	int num_components = gs_color_space_num_components(base_space);
	uint table_size = num_entries * num_components;
	/* Guess at the extra space needed for ASCII85 encoding. */
	uint string_size = 1 + table_size * 2 + table_size / 30 + 2;
	uint string_used;
	byte buf[100];		/* arbitrary */
	stream_AXE_state st;
	stream s, es;
	byte *table =
	    gs_alloc_string(mem, string_size, "pdf_color_space(table)");
	byte *palette =
	    gs_alloc_string(mem, table_size, "pdf_color_space(palette)");
	gs_color_space cs_gray;

	if (table == 0 || palette == 0) {
	    gs_free_string(mem, palette, table_size,
			   "pdf_color_space(palette)");
	    gs_free_string(mem, table, string_size,
			   "pdf_color_space(table)");
	    return_error(gs_error_VMerror);
	}
	swrite_string(&s, table, string_size);
	s_init(&es, NULL);
	s_init_state((stream_state *)&st, &s_AXE_template, NULL);
	s_init_filter(&es, (stream_state *)&st, buf, sizeof(buf), &s);
	sputc(&s, '<');
	if (pcs->params.indexed.use_proc) {
	    gs_client_color cmin, cmax;
	    byte *pnext = palette;

	    int i, j;

	    /* Find the legal range for the color components. */
	    for (j = 0; j < num_components; ++j)
		cmin.paint.values[j] = min_long,
		    cmax.paint.values[j] = max_long;
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
	    /* Check for an all-gray palette. */
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
		gs_cspace_init_DeviceGray(&cs_gray);
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
	 */
	if ((code = pdf_color_space(pdev, pvalue, base_space,
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
    }
    break;
    case gs_color_space_index_DeviceN:
	pfn = gs_cspace_get_devn_function(pcs);
	/****** CURRENTLY WE ONLY HANDLE Functions ******/
	if (pfn == 0)
	    return_error(gs_error_rangecheck);
	{
	    cos_array_t *psna = 
		cos_array_alloc(pdev, "pdf_color_space(DeviceN)");
	    int i;

	    if (psna == 0)
		return_error(gs_error_VMerror);
	    for (i = 0; i < pcs->params.device_n.num_components; ++i) {
		code = pdf_separation_name(pdev, &v,
					   pcs->params.device_n.names[i]);
		if (code < 0 ||
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
	if ((code = pdf_separation_name(pdev, &v,
					pcs->params.separation.sname)) < 0 ||
	    (code = pdf_separation_color_space(pdev, pca, "/Separation", &v,
					       (const gs_color_space *)
					&pcs->params.separation.alt_space,
					pfn, &pdf_color_space_names)) < 0)
	    return code;
	break;
    case gs_color_space_index_Pattern:
	if ((code = pdf_color_space(pdev, pvalue,
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
	pdf_resource_t *pres;

	code =
	    pdf_alloc_resource(pdev, resourceColorSpace, gs_no_id, &pres, 0L);
	if (code < 0) {
	    COS_FREE(pca, "pdf_color_space");
	    return code;
	}
	pca->id = pres->object->id;
	COS_FREE(pres->object, "pdf_color_space");
	pres->object = (cos_object_t *)pca;
	cos_write_object(COS_OBJECT(pca), pdev);
    }
    if (by_name) {
	/* Return a resource name rather than an object reference. */
	discard(COS_RESOURCE_VALUE(pvalue, pca));
    } else
	discard(COS_OBJECT_VALUE(pvalue, pca));
    return 0;
}

/* Create colored and uncolored Pattern color spaces. */
private int
pdf_pattern_space(gx_device_pdf *pdev, cos_value_t *pvalue,
		  pdf_resource_t **ppres, const char *cs_name)
{
    if (!*ppres) {
	int code = pdf_begin_resource_body(pdev, resourceColorSpace, gs_no_id,
					   ppres);

	if (code < 0)
	    return code;
	pprints1(pdev->strm, "%s\n", cs_name);
	pdf_end_resource(pdev);
	(*ppres)->object->written = true; /* don't write at end */
    }
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
    int ncomp = pdev->color_info.num_components;
    static const char *const pcs_names[5] = {
	0, "[/Pattern /DeviceGray]", 0, "[/Pattern /DeviceRGB]",
	"[/Pattern /DeviceCMYK]"
    };

    return pdf_pattern_space(pdev, pvalue, &pdev->cs_Patterns[ncomp],
			     pcs_names[ncomp]);
}

/* ---------------- Miscellaneous ---------------- */

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

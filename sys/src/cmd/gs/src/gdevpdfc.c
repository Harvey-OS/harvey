/* Copyright (C) 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gdevpdfc.c,v 1.12 2000/09/19 19:00:17 lpd Exp $ */
/* Color space management and writing for pdfwrite driver */
#include "math_.h"
#include "memory_.h"
#include "gx.h"
#include "gscspace.h"		/* for gscie.h */
#include "gscdevn.h"
#include "gscie.h"
#include "gscindex.h"
#include "gscsepr.h"
#include "gserrors.h"
#include "gdevpdfx.h"
#include "gdevpdfg.h"
#include "gdevpdfo.h"
#include "stream.h"
#include "strimpl.h"
#include "sstring.h"

/* ------ CIE space testing ------ */

/* Test whether a cached CIE procedure is the identity function. */
#define cie_cache_is_identity(pc)\
  ((pc)->floats.params.is_identity)
#define cie_cache3_is_identity(pca)\
  (cie_cache_is_identity(&(pca)[0]) &&\
   cie_cache_is_identity(&(pca)[1]) &&\
   cie_cache_is_identity(&(pca)[2]))

/*
 * Test whether a cached CIE procedure is an exponential.  A cached
 * procedure is exponential iff f(x) = k*(x^p).  We make a very cursory
 * check for this: we require that f(0) = 0, set k = f(1), set p =
 * log[a](f(a)/k), and then require that f(b) = k*(b^p), where a and b are
 * two arbitrarily chosen values between 0 and 1.  Naturally all this is
 * done with some slop.
 */
#define ia (gx_cie_cache_size / 3)
#define ib (gx_cie_cache_size * 2 / 3)
#define iv(i) ((i) / (double)(gx_cie_cache_size - 1))
#define a iv(ia)
#define b iv(ib)

private bool
cie_values_are_exponential(floatp va, floatp vb, floatp k,
			   float *pexpt)
{
    double p;

    if (fabs(k) < 0.001)
	return false;
    if (va == 0 || (va > 0) != (k > 0))
	return false;
    p = log(va / k) / log(a);
    if (fabs(vb - k * pow(b, p)) >= 0.001)
	return false;
    *pexpt = p;
    return true;
}

private bool
cie_scalar_cache_is_exponential(const gx_cie_scalar_cache * pc, float *pexpt)
{
    double k, va, vb;

    if (fabs(pc->floats.values[0]) >= 0.001)
	return false;
    k = pc->floats.values[gx_cie_cache_size - 1];
    va = pc->floats.values[ia];
    vb = pc->floats.values[ib];
    return cie_values_are_exponential(va, vb, k, pexpt);
}
#define cie_scalar3_cache_is_exponential(pca, expts)\
  (cie_scalar_cache_is_exponential(&(pca)[0], &(expts).u) &&\
   cie_scalar_cache_is_exponential(&(pca)[1], &(expts).v) &&\
   cie_scalar_cache_is_exponential(&(pca)[2], &(expts).w))

private bool
cie_vector_cache_is_exponential(const gx_cie_vector_cache * pc, float *pexpt)
{
    double k, va, vb;

    if (fabs(pc->vecs.values[0].u) >= 0.001)
	return false;
    k = pc->vecs.values[gx_cie_cache_size - 1].u;
    va = pc->vecs.values[ia].u;
    vb = pc->vecs.values[ib].u;
    return cie_values_are_exponential(va, vb, k, pexpt);
}
#define cie_vector3_cache_is_exponential(pca, expts)\
  (cie_vector_cache_is_exponential(&(pca)[0], &(expts).u) &&\
   cie_vector_cache_is_exponential(&(pca)[1], &(expts).v) &&\
   cie_vector_cache_is_exponential(&(pca)[2], &(expts).w))

#undef ia
#undef ib
#undef iv
#undef a
#undef b

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
    default:
	break;
    }
    /* Space has parameters -- create an array. */
    pca = cos_array_alloc(pdev, "pdf_color_space");
    if (pca == 0)
	return_error(gs_error_VMerror);
    switch (csi) {
    case gs_color_space_index_CIEA: {
	/* Check that we can represent this as a CalGray space. */
	const gs_cie_a *pcie = pcs->params.a;
	gs_vector3 expts;

	if (!(pcie->MatrixA.u == 1 && pcie->MatrixA.v == 1 &&
	      pcie->MatrixA.w == 1 &&
	      pcie->common.MatrixLMN.is_identity))
	    return_error(gs_error_rangecheck);
	if (cie_cache_is_identity(&pcie->caches.DecodeA) &&
	    cie_scalar3_cache_is_exponential(pcie->common.caches.DecodeLMN, expts) &&
	    expts.v == expts.u && expts.w == expts.u
	    ) {
	    DO_NOTHING;
	} else if (cie_cache3_is_identity(pcie->common.caches.DecodeLMN) &&
		   cie_vector_cache_is_exponential(&pcie->caches.DecodeA, &expts.u)
		   ) {
	    DO_NOTHING;
	} else
	    return_error(gs_error_rangecheck);
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
	pciec = (const gs_cie_common *)pcie;
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

	if (pcie->common.MatrixLMN.is_identity &&
	    cie_cache3_is_identity(pcie->common.caches.DecodeLMN) &&
	    cie_vector3_cache_is_exponential(pcie->caches.DecodeABC, expts)
	    )
	    pmat = &pcie->MatrixABC;
	else if (pcie->MatrixABC.is_identity &&
		 cie_cache3_is_identity(pcie->caches.DecodeABC) &&
		 cie_scalar3_cache_is_exponential(pcie->common.caches.DecodeLMN, expts)
		 )
	    pmat = &pcie->common.MatrixLMN;
	else
	    return_error(gs_error_rangecheck);
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
	pciec = (const gs_cie_common *)pcie;
    }
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
	pwrite(&es, palette, table_size);
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

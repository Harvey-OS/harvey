/* Copyright (C) 1998, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gscscie.c,v 1.12 2004/08/04 19:36:12 stefan Exp $ */
/* CIE color space management */
#include "math_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gsmatrix.h"		/* for gscolor2.h */
#include "gxcspace.h"
#include "gscolor2.h"		/* for gs_set/currentcolorrendering */
#include "gxcie.h"
#include "gxarith.h"
#include "gxdevice.h"		/* for gxcmap.h */
#include "gxcmap.h"
#include "gzstate.h"
#include "stream.h"

/* ---------------- Color space definition ---------------- */

/* GC descriptors */
public_st_cie_common();
public_st_cie_common_elements();
private_st_cie_a();
private_st_cie_abc();
private_st_cie_def();
private_st_cie_defg();

/* Define the CIE color space types. */

/* CIEBasedDEFG */
gs_private_st_ptrs1(st_color_space_CIEDEFG, gs_base_color_space,
     "gs_color_space(CIEDEFG)", cs_CIEDEFG_enum_ptrs, cs_CIEDEFG_reloc_ptrs,
		    params.defg);
private cs_proc_adjust_cspace_count(gx_adjust_cspace_CIEDEFG);
private cs_proc_serialize(gx_serialize_CIEDEFG);
const gs_color_space_type gs_color_space_type_CIEDEFG = {
    gs_color_space_index_CIEDEFG, true, true,
    &st_color_space_CIEDEFG, gx_num_components_4,
    gx_no_base_space,
    gx_init_CIE, gx_restrict_CIEDEFG,
    gx_concrete_space_CIE,
    gx_concretize_CIEDEFG, NULL,
    gx_default_remap_color, gx_install_CIE,
    gx_spot_colors_set_overprint,
    gx_adjust_cspace_CIEDEFG, gx_no_adjust_color_count,
    gx_serialize_CIEDEFG,
    gx_cspace_is_linear_default
};

/* CIEBasedDEF */
gs_private_st_ptrs1(st_color_space_CIEDEF, gs_base_color_space,
	"gs_color_space(CIEDEF)", cs_CIEDEF_enum_ptrs, cs_CIEDEF_reloc_ptrs,
		    params.def);
private cs_proc_adjust_cspace_count(gx_adjust_cspace_CIEDEF);
private cs_proc_serialize(gx_serialize_CIEDEF);
const gs_color_space_type gs_color_space_type_CIEDEF = {
    gs_color_space_index_CIEDEF, true, true,
    &st_color_space_CIEDEF, gx_num_components_3,
    gx_no_base_space,
    gx_init_CIE, gx_restrict_CIEDEF,
    gx_concrete_space_CIE,
    gx_concretize_CIEDEF, NULL,
    gx_default_remap_color, gx_install_CIE,
    gx_spot_colors_set_overprint,
    gx_adjust_cspace_CIEDEF, gx_no_adjust_color_count,
    gx_serialize_CIEDEF,
    gx_cspace_is_linear_default
};

/* CIEBasedABC */
gs_private_st_ptrs1(st_color_space_CIEABC, gs_base_color_space,
	"gs_color_space(CIEABC)", cs_CIEABC_enum_ptrs, cs_CIEABC_reloc_ptrs,
		    params.abc);
private cs_proc_adjust_cspace_count(gx_adjust_cspace_CIEABC);
private cs_proc_serialize(gx_serialize_CIEABC);
const gs_color_space_type gs_color_space_type_CIEABC = {
    gs_color_space_index_CIEABC, true, true,
    &st_color_space_CIEABC, gx_num_components_3,
    gx_no_base_space,
    gx_init_CIE, gx_restrict_CIEABC,
    gx_concrete_space_CIE,
    gx_concretize_CIEABC, NULL,
    gx_remap_CIEABC, gx_install_CIE,
    gx_spot_colors_set_overprint,
    gx_adjust_cspace_CIEABC, gx_no_adjust_color_count,
    gx_serialize_CIEABC,
    gx_cspace_is_linear_default
};

/* CIEBasedA */
gs_private_st_ptrs1(st_color_space_CIEA, gs_base_color_space,
	      "gs_color_space(CIEA)", cs_CIEA_enum_ptrs, cs_CIEA_reloc_ptrs,
		    params.a);
private cs_proc_adjust_cspace_count(gx_adjust_cspace_CIEA);
private cs_proc_serialize(gx_serialize_CIEA);
const gs_color_space_type gs_color_space_type_CIEA = {
    gs_color_space_index_CIEA, true, true,
    &st_color_space_CIEA, gx_num_components_1,
    gx_no_base_space,
    gx_init_CIE, gx_restrict_CIEA,
    gx_concrete_space_CIE,
    gx_concretize_CIEA, NULL,
    gx_default_remap_color, gx_install_CIE,
    gx_spot_colors_set_overprint,
    gx_adjust_cspace_CIEA, gx_no_adjust_color_count,
    gx_serialize_CIEA,
    gx_cspace_is_linear_default
};

private gs_color_space rgb_cs, cmyk_cs;

/* Determine the concrete space underlying a CIEBased space. */
const gs_color_space *
gx_concrete_space_CIE(const gs_color_space * pcs, const gs_imager_state * pis)
{
    const gs_cie_render *pcie = pis->cie_render;

    if (pcie == 0 || pcie->RenderTable.lookup.table == 0 ||
	pcie->RenderTable.lookup.m == 3
	) {
	gs_cspace_init_DeviceRGB(pis->memory, &rgb_cs);  /* idempotent initialization */
        return &rgb_cs;
    } else {			/* pcie->RenderTable.lookup.m == 4 */
	gs_cspace_init_DeviceCMYK(pis->memory, &cmyk_cs); /* idempotent initialization */
	return &cmyk_cs;
    }
}

/* Install a CIE space in the graphics state. */
/* We go through an extra level of procedure so that */
/* interpreters can substitute their own installer. */
/* This procedure is exported for the benefit of gsicc.c */
int
gx_install_CIE(const gs_color_space * pcs, gs_state * pgs)
{
    return (*pcs->params.a->common.install_cspace) (pcs, pgs);
}

/* Adjust reference counts for a CIE color space */
private void
gx_adjust_cspace_CIEDEFG(const gs_color_space * pcs, int delta)
{
    rc_adjust_const(pcs->params.defg, delta, "gx_adjust_cspace_CIEDEFG");
}

private void
gx_adjust_cspace_CIEDEF(const gs_color_space * pcs, int delta)
{
    rc_adjust_const(pcs->params.def, delta, "gx_adjust_cspace_CIEDEF");
}

private void
gx_adjust_cspace_CIEABC(const gs_color_space * pcs, int delta)
{
    rc_adjust_const(pcs->params.abc, delta, "gx_adjust_cspace_CIEABC");
}

private void
gx_adjust_cspace_CIEA(const gs_color_space * pcs, int delta)
{
    rc_adjust_const(pcs->params.a, delta, "gx_adjust_cspace_CIEA");
}

/* ---------------- Procedures ---------------- */

/* ------ Internal initializers ------ */

/*
 * Set up the default values for the CIE parameters that are common to
 * all CIE color spaces.
 *
 * There is no default for the white point, so it is set equal to the
 * black point. If anyone actually uses the color space in that form,
 * the results are likely to be unsatisfactory.
 *
 * This procedure is exported for the benefit of gsicc.c.
 */
void
gx_set_common_cie_defaults(gs_cie_common * pcommon, void *client_data)
{
    pcommon->RangeLMN = Range3_default;
    pcommon->DecodeLMN = DecodeLMN_default;
    pcommon->MatrixLMN = Matrix3_default;
    pcommon->points.WhitePoint = BlackPoint_default;
    pcommon->points.BlackPoint = BlackPoint_default;
    pcommon->client_data = client_data;
}

/*
 * Set defaults for a CIEBasedABC color space.  This is also used for
 * CIEBasedDEF and CIEBasedDEFG color spaces.
 */
private void
set_cie_abc_defaults(gs_cie_abc * pabc, void *client_data)
{
    gx_set_common_cie_defaults(&pabc->common, client_data);
    pabc->RangeABC = Range3_default;
    pabc->DecodeABC = DecodeABC_default;
    pabc->MatrixABC = Matrix3_default;
}

/*
 * Set up a default color lookup table for a CIEBasedDEF[G] space. There is
 * no specified default for this structure, so the values used here (aside
 * from the input and output component numbers) are intended only to make
 * the system fail in a predictable manner.
 */
private void
set_ctbl_defaults(gx_color_lookup_table * plktblp, int num_comps)
{
    int i;

    plktblp->n = num_comps;
    plktblp->m = 3;		/* always output CIE ABC */
    for (i = 0; i < countof(plktblp->dims); i++)
	plktblp->dims[i] = 0;
    plktblp->table = 0;
}

/*
 * Allocate a color space and its parameter structure.
 * Return 0 if VMerror, otherwise the parameter structure.
 *
 * This is exported for the benefit of gsicc.c
 */
void *
gx_build_cie_space(gs_color_space ** ppcspace,
		const gs_color_space_type * pcstype,
		gs_memory_type_ptr_t stype, gs_memory_t * pmem)
{
    gs_color_space *pcspace;
    int code = gs_cspace_alloc(&pcspace, pcstype, pmem);
    gs_cie_common_elements_t *pdata;

    if (code < 0)
	return 0;
    rc_alloc_struct_1(pdata, gs_cie_common_elements_t, stype, pmem,
		      {
		      gs_free_object(pmem, pcspace, "gx_build_cie_space");
		      return 0;
		      }
		      ,
		      "gx_build_cie_space(data)");
    *ppcspace = pcspace;
    return (void *)pdata;
}

/* ------ Constructors ------ */

int
gs_cspace_build_CIEA(gs_color_space ** ppcspace, void *client_data,
		     gs_memory_t * pmem)
{
    gs_cie_a *pciea =
    gx_build_cie_space(ppcspace, &gs_color_space_type_CIEA, &st_cie_a, pmem);

    if (pciea == 0)
	return_error(gs_error_VMerror);

    gx_set_common_cie_defaults(&pciea->common, client_data);
    pciea->common.install_cspace = gx_install_CIEA;
    pciea->RangeA = RangeA_default;
    pciea->DecodeA = DecodeA_default;
    pciea->MatrixA = MatrixA_default;

    (*ppcspace)->params.a = pciea;
    return 0;
}

int
gs_cspace_build_CIEABC(gs_color_space ** ppcspace, void *client_data,
		       gs_memory_t * pmem)
{
    gs_cie_abc *pabc =
    gx_build_cie_space(ppcspace, &gs_color_space_type_CIEABC, &st_cie_abc,
		       pmem);

    if (pabc == 0)
	return_error(gs_error_VMerror);

    set_cie_abc_defaults(pabc, client_data);
    pabc->common.install_cspace = gx_install_CIEABC;

    (*ppcspace)->params.abc = pabc;
    return 0;
}

int
gs_cspace_build_CIEDEF(gs_color_space ** ppcspace, void *client_data,
		       gs_memory_t * pmem)
{
    gs_cie_def *pdef =
    gx_build_cie_space(ppcspace, &gs_color_space_type_CIEDEF, &st_cie_def,
		       pmem);

    if (pdef == 0)
	return_error(gs_error_VMerror);

    set_cie_abc_defaults((gs_cie_abc *) pdef, client_data);
    pdef->common.install_cspace = gx_install_CIEDEF;
    pdef->RangeDEF = Range3_default;
    pdef->DecodeDEF = DecodeDEF_default;
    pdef->RangeHIJ = Range3_default;
    set_ctbl_defaults(&pdef->Table, 3);

    (*ppcspace)->params.def = pdef;
    return 0;
}

int
gs_cspace_build_CIEDEFG(gs_color_space ** ppcspace, void *client_data,
			gs_memory_t * pmem)
{
    gs_cie_defg *pdefg =
    gx_build_cie_space(ppcspace, &gs_color_space_type_CIEDEFG, &st_cie_defg,
		       pmem);

    if (pdefg == 0)
	return_error(gs_error_VMerror);

    set_cie_abc_defaults((gs_cie_abc *) pdefg, client_data);
    pdefg->common.install_cspace = gx_install_CIEDEFG;
    pdefg->RangeDEFG = Range4_default;
    pdefg->DecodeDEFG = DecodeDEFG_default;
    pdefg->RangeHIJK = Range4_default;
    set_ctbl_defaults(&pdefg->Table, 4);

    (*ppcspace)->params.defg = pdefg;
    return 0;
}

/* ------ Accessors ------ */

int
gs_cie_defx_set_lookup_table(gs_color_space * pcspace, int *pdims,
			     const gs_const_string * ptable)
{
    gx_color_lookup_table *plktblp;

    switch (gs_color_space_get_index(pcspace)) {
	case gs_color_space_index_CIEDEF:
	    plktblp = &pcspace->params.def->Table;
	    break;
	case gs_color_space_index_CIEDEFG:
	    plktblp = &pcspace->params.defg->Table;
	    plktblp->dims[3] = pdims[3];
	    break;
	default:
	    return_error(gs_error_rangecheck);
    }

    plktblp->dims[0] = pdims[0];
    plktblp->dims[1] = pdims[1];
    plktblp->dims[2] = pdims[2];
    plktblp->table = ptable;
    return 0;
}

/* ---------------- Serialization. -------------------------------- */

private int
gx_serialize_cie_cache(const cie_cache_floats *c, stream * s)
{ /* p->DecodeA : */
    const uint cache_size = count_of(c->values);
    uint n;
    int code;

    code = sputs(s, (const byte *)&c->params.is_identity, sizeof(c->params.is_identity), &n);
    if (c->params.is_identity)
	return 0;
    code = sputs(s, (const byte *)&cache_size, sizeof(cache_size), &n);
    if (code < 0)
	return code;
    return sputs(s, (const byte *)c->values, sizeof(c->values), &n);
}

int 
gx_serialize_cie_common_elements(const gs_color_space * pcs, stream * s)
{
    const gs_cie_a * p = pcs->params.a;
    uint n, k;
    int code = gx_serialize_cspace_type(pcs, s);

    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&p->common.RangeLMN, 
			sizeof(p->common.RangeLMN), &n);
    if (code < 0)
	return code;
    for (k = 0; k < 3 && code >= 0; k++)
	code = gx_serialize_cie_cache(&p->common.caches.DecodeLMN[k].floats, s);
    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&p->common.MatrixLMN, 
			sizeof(p->common.MatrixLMN), &n);
    if (code < 0)
	return code;
    return sputs(s, (const byte *)&p->common.points, 
			sizeof(p->common.points), &n);
}

private int 
gx_serialize_CIEA(const gs_color_space * pcs, stream * s)
{
    const gs_cie_a * p = pcs->params.a;
    uint n;
    int code = gx_serialize_cie_common_elements(pcs, s);

    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&p->RangeA, sizeof(p->RangeA), &n);
    if (code < 0)
	return code;
    code = gx_serialize_cie_cache(&p->caches.DecodeA.floats, s);
    if (code < 0)
	return code;
    return sputs(s, (const byte *)&p->MatrixA, sizeof(p->MatrixA), &n);
}

private int 
gx_serialize_CIEABC(const gs_color_space * pcs, stream * s)
{
    const gs_cie_abc * p = pcs->params.abc;
    uint n, k;
    int code = gx_serialize_cie_common_elements(pcs, s);

    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&p->RangeABC, sizeof(p->RangeABC), &n);
    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&p->caches.skipABC, sizeof(p->caches.skipABC), &n);
    if (code < 0)
	return code;
    if (p->caches.skipABC)
	return 0;
    for (k = 0; k < 3 && code >= 0; k++)
	code = gx_serialize_cie_cache(&p->caches.DecodeABC.caches[k].floats, s);
    if (code < 0)
	return code;
    return sputs(s, (const byte *)&p->MatrixABC, sizeof(p->MatrixABC), &n);
}

private int 
gx_serialize_lookup_table(const gx_color_lookup_table * t, stream * s)
{
    uint n;
    int code = sputs(s, (const byte *)&t->n, sizeof(t->n), &n);

    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&t->dims[0], sizeof(t->dims[0]) * t->n, &n);
    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&t->m, sizeof(t->m), &n);
    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&t->table->size, sizeof(t->table->size), &n);
    if (code < 0)
	return code;
    return sputs(s, (const byte *)&t->table->data, t->table->size, &n);
}

private int 
gx_serialize_CIEDEF(const gs_color_space * pcs, stream * s)
{
    const gs_cie_def * p = pcs->params.def;
    uint n, k;
    int code = gx_serialize_cie_common_elements(pcs, s);

    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&p->RangeDEF, sizeof(p->RangeDEF), &n);
    if (code < 0)
	return code;
    for (k = 0; k < 3 && code >= 0; k++)
	code = gx_serialize_cie_cache(&p->caches_def.DecodeDEF[k].floats, s);
    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&p->RangeHIJ, sizeof(p->RangeHIJ), &n);
    if (code < 0)
	return code;
    return gx_serialize_lookup_table(&p->Table, s);
}

private int 
gx_serialize_CIEDEFG(const gs_color_space * pcs, stream * s)
{
    const gs_cie_defg * p = pcs->params.defg;
    uint n, k;
    int code = gx_serialize_cie_common_elements(pcs, s);

    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&p->RangeDEFG, sizeof(p->RangeDEFG), &n);
    if (code < 0)
	return code;
    for (k = 0; k < 3 && code >= 0; k++)
	code = gx_serialize_cie_cache(&p->caches_defg.DecodeDEFG[k].floats, s);
    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&p->RangeHIJK, sizeof(p->RangeHIJK), &n);
    if (code < 0)
	return code;
    return gx_serialize_lookup_table(&p->Table, s);
}

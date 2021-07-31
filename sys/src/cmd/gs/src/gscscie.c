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

/*$Id: gscscie.c,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
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

/* ---------------- Color space definition ---------------- */

/* GC descriptors */
private_st_cie_common();
private_st_cie_common_elements();
private_st_cie_a();
private_st_cie_abc();
private_st_cie_def();
private_st_cie_defg();

/* Define the CIE color space types. */
private cs_proc_concrete_space(gx_concrete_space_CIE);
private cs_proc_install_cspace(gx_install_CIE);

/* CIEBasedDEFG */
gs_private_st_ptrs1(st_color_space_CIEDEFG, gs_base_color_space,
     "gs_color_space(CIEDEFG)", cs_CIEDEFG_enum_ptrs, cs_CIEDEFG_reloc_ptrs,
		    params.defg);
private cs_proc_adjust_cspace_count(gx_adjust_cspace_CIEDEFG);
const gs_color_space_type gs_color_space_type_CIEDEFG = {
    gs_color_space_index_CIEDEFG, true, true,
    &st_color_space_CIEDEFG, gx_num_components_4,
    gx_no_base_space,
    gx_init_CIE, gx_restrict_CIEDEFG,
    gx_concrete_space_CIE,
    gx_concretize_CIEDEFG, NULL,
    gx_default_remap_color, gx_install_CIE,
    gx_adjust_cspace_CIEDEFG, gx_no_adjust_color_count
};

/* CIEBasedDEF */
gs_private_st_ptrs1(st_color_space_CIEDEF, gs_base_color_space,
	"gs_color_space(CIEDEF)", cs_CIEDEF_enum_ptrs, cs_CIEDEF_reloc_ptrs,
		    params.def);
private cs_proc_adjust_cspace_count(gx_adjust_cspace_CIEDEF);
const gs_color_space_type gs_color_space_type_CIEDEF = {
    gs_color_space_index_CIEDEF, true, true,
    &st_color_space_CIEDEF, gx_num_components_3,
    gx_no_base_space,
    gx_init_CIE, gx_restrict_CIEDEF,
    gx_concrete_space_CIE,
    gx_concretize_CIEDEF, NULL,
    gx_default_remap_color, gx_install_CIE,
    gx_adjust_cspace_CIEDEF, gx_no_adjust_color_count
};

/* CIEBasedABC */
gs_private_st_ptrs1(st_color_space_CIEABC, gs_base_color_space,
	"gs_color_space(CIEABC)", cs_CIEABC_enum_ptrs, cs_CIEABC_reloc_ptrs,
		    params.abc);
private cs_proc_adjust_cspace_count(gx_adjust_cspace_CIEABC);
const gs_color_space_type gs_color_space_type_CIEABC = {
    gs_color_space_index_CIEABC, true, true,
    &st_color_space_CIEABC, gx_num_components_3,
    gx_no_base_space,
    gx_init_CIE, gx_restrict_CIEABC,
    gx_concrete_space_CIE,
    gx_concretize_CIEABC, NULL,
    gx_remap_CIEABC, gx_install_CIE,
    gx_adjust_cspace_CIEABC, gx_no_adjust_color_count
};

/* CIEBasedA */
gs_private_st_ptrs1(st_color_space_CIEA, gs_base_color_space,
	      "gs_color_space(CIEA)", cs_CIEA_enum_ptrs, cs_CIEA_reloc_ptrs,
		    params.a);
private cs_proc_adjust_cspace_count(gx_adjust_cspace_CIEA);
const gs_color_space_type gs_color_space_type_CIEA = {
    gs_color_space_index_CIEA, true, true,
    &st_color_space_CIEA, gx_num_components_1,
    gx_no_base_space,
    gx_init_CIE, gx_restrict_CIEA,
    gx_concrete_space_CIE,
    gx_concretize_CIEA, NULL,
    gx_default_remap_color, gx_install_CIE,
    gx_adjust_cspace_CIEA, gx_no_adjust_color_count
};

/* Determine the concrete space underlying a CIEBased space. */
private const gs_color_space *
gx_concrete_space_CIE(const gs_color_space * pcs, const gs_imager_state * pis)
{
    const gs_cie_render *pcie = pis->cie_render;

    if (pcie == 0 || pcie->RenderTable.lookup.table == 0 ||
	pcie->RenderTable.lookup.m == 3
	)
	return gs_cspace_DeviceRGB(pis);
    else			/* pcie->RenderTable.lookup.m == 4 */
	return gs_cspace_DeviceCMYK(pis);
}

/* Install a CIE space in the graphics state. */
/* We go through an extra level of procedure so that */
/* interpreters can substitute their own installer. */
private int
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
 */
private void
set_common_cie_defaults(gs_cie_common * pcommon, void *client_data)
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
    set_common_cie_defaults(&pabc->common, client_data);
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
 */
private void *
build_cie_space(gs_color_space ** ppcspace,
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
		      gs_free_object(pmem, pcspace, "build_cie_space");
		      return 0;
		      }
		      ,
		      "build_cie_space(data)");
    *ppcspace = pcspace;
    return (void *)pdata;
}

/* ------ Constructors ------ */

int
gs_cspace_build_CIEA(gs_color_space ** ppcspace, void *client_data,
		     gs_memory_t * pmem)
{
    gs_cie_a *pciea =
    build_cie_space(ppcspace, &gs_color_space_type_CIEA, &st_cie_a, pmem);

    if (pciea == 0)
	return_error(gs_error_VMerror);

    set_common_cie_defaults(&pciea->common, client_data);
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
    build_cie_space(ppcspace, &gs_color_space_type_CIEABC, &st_cie_abc,
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
    build_cie_space(ppcspace, &gs_color_space_type_CIEDEF, &st_cie_def,
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
    build_cie_space(ppcspace, &gs_color_space_type_CIEDEFG, &st_cie_defg,
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

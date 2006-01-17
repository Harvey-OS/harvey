/* Copyright (C) 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zcrd.c,v 1.8 2004/08/04 19:36:13 stefan Exp $ */
/* CIE color rendering operators */
#include "math_.h"
#include "ghost.h"
#include "oper.h"
#include "gsstruct.h"
#include "gscspace.h"
#include "gscolor2.h"
#include "gscrd.h"
#include "gscrdp.h"
#include "estack.h"
#include "ialloc.h"
#include "idict.h"
#include "idparam.h"
#include "igstate.h"
#include "icie.h"
#include "iparam.h"
#include "ivmspace.h"
#include "store.h"		/* for make_null */

/* Forward references */
private int zcrd1_proc_params(const gs_memory_t *mem, os_ptr op, ref_cie_render_procs * pcprocs);
private int zcrd1_params(os_ptr op, gs_cie_render * pcrd,
			 ref_cie_render_procs * pcprocs, gs_memory_t * mem);
private int cache_colorrendering1(i_ctx_t *i_ctx_p, gs_cie_render * pcrd,
				  const ref_cie_render_procs * pcprocs,
				  gs_ref_memory_t * imem);

/* - currentcolorrendering <dict> */
private int
zcurrentcolorrendering(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    push(1);
    *op = istate->colorrendering.dict;
    return 0;
}

/* <dict> .buildcolorrendering1 <crd> */
private int
zbuildcolorrendering1(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_memory_t *mem = gs_state_memory(igs);
    int code;
    es_ptr ep = esp;
    gs_cie_render *pcrd;
    ref_cie_render_procs procs;

    check_read_type(*op, t_dictionary);
    check_dict_read(*op);
    code = gs_cie_render1_build(&pcrd, mem, ".buildcolorrendering1");
    if (code < 0)
	return code;
    code = zcrd1_params(op, pcrd, &procs, mem);
    if (code < 0 ||
    (code = cache_colorrendering1(i_ctx_p, pcrd, &procs,
				  (gs_ref_memory_t *) mem)) < 0
	) {
	rc_free_struct(pcrd, ".buildcolorrendering1");
	esp = ep;
	return code;
    }
    /****** FIX refct ******/
    /*rc_decrement(pcrd, ".buildcolorrendering1"); *//* build sets rc = 1 */
    istate->colorrendering.dict = *op;
    make_istruct_new(op, a_readonly, pcrd);
    return (esp == ep ? 0 : o_push_estack);
}

/* <dict> .builddevicecolorrendering1 <crd> */
private int
zbuilddevicecolorrendering1(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_memory_t *mem = gs_state_memory(igs);
    dict_param_list list;
    gs_cie_render *pcrd = 0;
    int code;

    check_type(*op, t_dictionary);
    code = dict_param_list_read(&list, op, NULL, false, iimemory);
    if (code < 0)
	return code;
    code = gs_cie_render1_build(&pcrd, mem, ".builddevicecolorrendering1");
    if (code >= 0) {
	code = param_get_cie_render1(pcrd, (gs_param_list *) & list,
				     gs_currentdevice(igs));
	if (code >= 0) {
	    /****** FIX refct ******/
	    /*rc_decrement(pcrd, ".builddevicecolorrendering1"); *//* build sets rc = 1 */
	}
    }
    iparam_list_release(&list);
    if (code < 0) {
	rc_free_struct(pcrd, ".builddevicecolorrendering1");
	return code;
    }
    istate->colorrendering.dict = *op;
    make_istruct_new(op, a_readonly, pcrd);
    return 0;
}

/* <dict> <crd> .setcolorrendering1 - */
private int
zsetcolorrendering1(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    es_ptr ep = esp;
    ref_cie_render_procs procs;
    int code;

    check_type(op[-1], t_dictionary);
    check_stype(*op, st_cie_render1);
    code = zcrd1_proc_params(imemory, op - 1, &procs);
    if (code < 0)
	return code;
    code = gs_setcolorrendering(igs, r_ptr(op, gs_cie_render));
    if (code < 0)
	return code;
    if (gs_cie_cs_common(igs) != 0 &&
	(code = cie_cache_joint(i_ctx_p, &procs, gs_cie_cs_common(igs), igs)) < 0
	)
	return code;
    istate->colorrendering.dict = op[-1];
    istate->colorrendering.procs = procs;
    pop(2);
    return (esp == ep ? 0 : o_push_estack);
}

/* <dict> <crd> .setdevicecolorrendering1 - */
private int
zsetdevicecolorrendering1(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code;
    ref_cie_render_procs procs;

    check_type(op[-1], t_dictionary);
    check_stype(*op, st_cie_render1);
    code = gs_setcolorrendering(igs, r_ptr(op, gs_cie_render));
    if (code < 0)
	return code;
    refset_null((ref *)&procs, sizeof(procs) / sizeof(ref));
    if (gs_cie_cs_common(igs) != 0 &&
	(code = cie_cache_joint(i_ctx_p, &procs, gs_cie_cs_common(igs), igs)) < 0
	)
	return code;
    istate->colorrendering.dict = op[-1];
    refset_null((ref *)&istate->colorrendering.procs,
		sizeof(istate->colorrendering.procs) / sizeof(ref));
    pop(2);
    return 0;
}

/* Get ColorRenderingType 1 procedures from the PostScript dictionary. */
private int
zcrd1_proc_params(const gs_memory_t *mem, 
		  os_ptr op, ref_cie_render_procs * pcprocs)
{
    int code;
    ref *pRT;

    if ((code = dict_proc3_param(mem, op, "EncodeLMN", &pcprocs->EncodeLMN)) < 0 ||
      (code = dict_proc3_param(mem, op, "EncodeABC", &pcprocs->EncodeABC)) < 0 ||
    (code = dict_proc3_param(mem, op, "TransformPQR", &pcprocs->TransformPQR)) < 0
	)
	return (code < 0 ? code : gs_note_error(e_rangecheck));
    if (dict_find_string(op, "RenderTable", &pRT) > 0) {
	const ref *prte;
	int size;
	int i;

	check_read_type(*pRT, t_array);
	size = r_size(pRT);
	if (size < 5)
	    return_error(e_rangecheck);
	prte = pRT->value.const_refs;
	for (i = 5; i < size; i++)
	    check_proc_only(prte[i]);
	make_const_array(&pcprocs->RenderTableT, a_readonly | r_space(pRT),
			 size - 5, prte + 5);
    } else
	make_null(&pcprocs->RenderTableT);
    return 0;
}

/* Get ColorRenderingType 1 parameters from the PostScript dictionary. */
private int
zcrd1_params(os_ptr op, gs_cie_render * pcrd,
	     ref_cie_render_procs * pcprocs, gs_memory_t * mem)
{
    int code;
    int ignore;
    gx_color_lookup_table *const prtl = &pcrd->RenderTable.lookup;
    ref *pRT;

    if ((code = dict_int_param(op, "ColorRenderingType", 1, 1, 0, &ignore)) < 0 ||
	(code = zcrd1_proc_params(mem, op, pcprocs)) < 0 ||
	(code = dict_matrix3_param(mem, op, "MatrixLMN", &pcrd->MatrixLMN)) < 0 ||
	(code = dict_range3_param(mem, op, "RangeLMN", &pcrd->RangeLMN)) < 0 ||
	(code = dict_matrix3_param(mem, op, "MatrixABC", &pcrd->MatrixABC)) < 0 ||
	(code = dict_range3_param(mem, op, "RangeABC", &pcrd->RangeABC)) < 0 ||
	(code = cie_points_param(mem, op, &pcrd->points)) < 0 ||
	(code = dict_matrix3_param(mem, op, "MatrixPQR", &pcrd->MatrixPQR)) < 0 ||
	(code = dict_range3_param(mem,op, "RangePQR", &pcrd->RangePQR)) < 0
	)
	return code;
    if (dict_find_string(op, "RenderTable", &pRT) > 0) {
	const ref *prte = pRT->value.const_refs;

	/* Finish unpacking and checking the RenderTable parameter. */
	check_type_only(prte[4], t_integer);
	if (!(prte[4].value.intval == 3 || prte[4].value.intval == 4))
	    return_error(e_rangecheck);
	prtl->n = 3;
	prtl->m = prte[4].value.intval;
	if (r_size(pRT) != prtl->m + 5)
	    return_error(e_rangecheck);
	code = cie_table_param(pRT, prtl, mem);
	if (code < 0)
	    return code;
    } else {
	prtl->table = 0;
    }
    pcrd->EncodeLMN = Encode_default;
    pcrd->EncodeABC = Encode_default;
    pcrd->TransformPQR = TransformPQR_default;
    pcrd->RenderTable.T = RenderTableT_default;
    return 0;
}

/* Cache the results of the color rendering procedures. */
private int cie_cache_render_finish(i_ctx_t *);
private int
cache_colorrendering1(i_ctx_t *i_ctx_p, gs_cie_render * pcrd,
		      const ref_cie_render_procs * pcrprocs,
		      gs_ref_memory_t * imem)
{
    es_ptr ep = esp;
    int code = gs_cie_render_init(pcrd);	/* sets Domain values */
    int i;

    if (code < 0 ||
	(code = cie_cache_push_finish(i_ctx_p, cie_cache_render_finish, imem, pcrd)) < 0 ||
	(code = cie_prepare_cache3(i_ctx_p, &pcrd->DomainLMN, pcrprocs->EncodeLMN.value.const_refs, pcrd->caches.EncodeLMN.caches, pcrd, imem, "Encode.LMN")) < 0 ||
	(code = cie_prepare_cache3(i_ctx_p, &pcrd->DomainABC, pcrprocs->EncodeABC.value.const_refs, &pcrd->caches.EncodeABC[0], pcrd, imem, "Encode.ABC")) < 0
	) {
	esp = ep;
	return code;
    }
    if (pcrd->RenderTable.lookup.table != 0) {
	bool is_identity = true;

	for (i = 0; i < pcrd->RenderTable.lookup.m; i++)
	    if (r_size(pcrprocs->RenderTableT.value.const_refs + i) != 0) {
		is_identity = false;
		break;
	    }
	pcrd->caches.RenderTableT_is_identity = is_identity;
	if (!is_identity)
	    for (i = 0; i < pcrd->RenderTable.lookup.m; i++)
		if ((code =
		     cie_prepare_cache(i_ctx_p, Range4_default.ranges,
				pcrprocs->RenderTableT.value.const_refs + i,
				       &pcrd->caches.RenderTableT[i].floats,
				       pcrd, imem, "RenderTable.T")) < 0
		    ) {
		    esp = ep;
		    return code;
		}
    }
    return o_push_estack;
}

/* Finish up after loading the rendering caches. */
private int
cie_cache_render_finish(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_cie_render *pcrd = r_ptr(op, gs_cie_render);
    int code;

    if (pcrd->RenderTable.lookup.table != 0 &&
	!pcrd->caches.RenderTableT_is_identity
	) {
	/* Convert the RenderTableT cache from floats to fracs. */
	int j;

	for (j = 0; j < pcrd->RenderTable.lookup.m; j++)
	    gs_cie_cache_to_fracs(&pcrd->caches.RenderTableT[j].floats,
				  &pcrd->caches.RenderTableT[j].fracs);
    }
    pcrd->status = CIE_RENDER_STATUS_SAMPLED;
    pcrd->EncodeLMN = EncodeLMN_from_cache;
    pcrd->EncodeABC = EncodeABC_from_cache;
    pcrd->RenderTable.T = RenderTableT_from_cache;
    code = gs_cie_render_complete(pcrd);
    if (code < 0)
	return code;
    pop(1);
    return 0;
}

/* ------ Internal procedures ------ */

/* Load the joint caches. */
private int
    cie_exec_tpqr(i_ctx_t *),
    cie_post_exec_tpqr(i_ctx_t *),
    cie_tpqr_finish(i_ctx_t *);
int
cie_cache_joint(i_ctx_t *i_ctx_p, const ref_cie_render_procs * pcrprocs,
		const gs_cie_common *pcie, gs_state * pgs)
{
    const gs_cie_render *pcrd = gs_currentcolorrendering(pgs);
    gx_cie_joint_caches *pjc = gx_currentciecaches(pgs);
    gs_ref_memory_t *imem = (gs_ref_memory_t *) gs_state_memory(pgs);
    ref pqr_procs;
    uint space;
    int code;
    int i;

    if (pcrd == 0)		/* cache is not set up yet */
	return 0;
    if (pjc == 0)		/* must already be allocated */
	return_error(e_VMerror);
    if (r_has_type(&pcrprocs->TransformPQR, t_null)) {
	/*
	 * This CRD came from a driver, not from a PostScript dictionary.
	 * Resample TransformPQR in C code.
	 */
	return gs_cie_cs_complete(pgs, true);
    }
    gs_cie_compute_points_sd(pjc, pcie, pcrd);
    code = ialloc_ref_array(&pqr_procs, a_readonly, 3 * (1 + 4 + 4 * 6),
			    "cie_cache_common");
    if (code < 0)
	return code;
    /* When we're done, deallocate the procs and complete the caches. */
    check_estack(3);
    cie_cache_push_finish(i_ctx_p, cie_tpqr_finish, imem, pgs);
    *++esp = pqr_procs;
    space = r_space(&pqr_procs);
    for (i = 0; i < 3; i++) {
	ref *p = pqr_procs.value.refs + 3 + (4 + 4 * 6) * i;
	const float *ppt = (float *)&pjc->points_sd;
	int j;

	make_array(pqr_procs.value.refs + i, a_readonly | a_executable | space,
		   4, p);
	make_array(p, a_readonly | space, 4 * 6, p + 4);
	p[1] = pcrprocs->TransformPQR.value.refs[i];
	make_oper(p + 2, 0, cie_exec_tpqr);
	make_oper(p + 3, 0, cie_post_exec_tpqr);
	for (j = 0, p += 4; j < 4 * 6; j++, p++, ppt++)
	    make_real(p, *ppt);
    }
    return cie_prepare_cache3(i_ctx_p, &pcrd->RangePQR,
			      pqr_procs.value.const_refs,
			      pjc->TransformPQR.caches,
			      pjc, imem, "Transform.PQR");
}

/* Private operator to shuffle arguments for the TransformPQR procedure: */
/* v [ws wd bs bd] proc -> -mark- ws wd bs bd v proc + exec */
private int
cie_exec_tpqr(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    const ref *ppt = op[-1].value.const_refs;
    uint space = r_space(op - 1);
    int i;

    check_op(3);
    push(4);
    *op = op[-4];		/* proc */
    op[-1] = op[-6];		/* v */
    for (i = 0; i < 4; i++)
	make_const_array(op - 5 + i, a_readonly | space,
			 6, ppt + i * 6);
    make_mark(op - 6);
    return zexec(i_ctx_p);
}

/* Remove extraneous values from the stack after executing */
/* the TransformPQR procedure.  -mark- ... v -> v */
private int
cie_post_exec_tpqr(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    uint count = ref_stack_counttomark(&o_stack);
    ref vref;

    if (count < 2)
	return_error(e_unmatchedmark);
    vref = *op;
    ref_stack_pop(&o_stack, count - 1);
    *osp = vref;
    return 0;
}

/* Free the procs array and complete the joint caches. */
private int
cie_tpqr_finish(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_state *pgs = r_ptr(op, gs_state);
    gs_cie_render *pcrd =
	(gs_cie_render *)gs_currentcolorrendering(pgs);  /* break const */
    int code;

    ifree_ref_array(op - 1, "cie_tpqr_finish");
    pcrd->TransformPQR = TransformPQR_from_cache;
    code = gs_cie_cs_complete(pgs, false);
    pop(2);
    return code;
}

/* Ws Bs Wd Bd Ps .transformPQR_scale_wb[012] Pd

   The default TransformPQR procedure is implemented in C, rather than
   PostScript, as a speed optimization.

   This TransformPQR implements a relative colorimetric intent by scaling
   the XYZ values relative to the white and black points.
*/
private int
ztpqr_scale_wb_common(i_ctx_t *i_ctx_p, int idx)
{
    os_ptr op = osp;
    double a[4], Ps; /* a[0] = ws, a[1] = bs, a[2] = wd, a[3] = bd */
    double result;
    int code;
    int i;

    code = real_param(op, &Ps);
    if (code < 0) return code;

    for (i = 0; i < 4; i++) {
	ref tmp;

	code = array_get(imemory, op - 4 + i, idx, &tmp);
	if (code >= 0)
	    code = real_param(&tmp, &a[i]);
	if (code < 0) return code;
    }

    if (a[0] == a[1])
	return_error(e_undefinedresult);
    result = a[3] + (a[2] - a[3]) * (Ps - a[1]) / (a[0] - a[1]);
    make_real(op - 4, result);
    pop(4);
    return 0;
}

/* Ws Bs Wd Bd Ps .TransformPQR_scale_wb0 Pd */
private int
ztpqr_scale_wb0(i_ctx_t *i_ctx_p)
{
    return ztpqr_scale_wb_common(i_ctx_p, 3);
}

/* Ws Bs Wd Bd Ps .TransformPQR_scale_wb2 Pd */
private int
ztpqr_scale_wb1(i_ctx_t *i_ctx_p)
{
    return ztpqr_scale_wb_common(i_ctx_p, 4);
}

/* Ws Bs Wd Bd Ps .TransformPQR_scale_wb2 Pd */
private int
ztpqr_scale_wb2(i_ctx_t *i_ctx_p)
{
    return ztpqr_scale_wb_common(i_ctx_p, 5);
}

/* ------ Initialization procedure ------ */

const op_def zcrd_l2_op_defs[] =
{
    op_def_begin_level2(),
    {"0currentcolorrendering", zcurrentcolorrendering},
    {"2.setcolorrendering1", zsetcolorrendering1},
    {"2.setdevicecolorrendering1", zsetdevicecolorrendering1},
    {"1.buildcolorrendering1", zbuildcolorrendering1},
    {"1.builddevicecolorrendering1", zbuilddevicecolorrendering1},
		/* Internal "operators" */
    {"1%cie_render_finish", cie_cache_render_finish},
    {"3%cie_exec_tpqr", cie_exec_tpqr},
    {"2%cie_post_exec_tpqr", cie_post_exec_tpqr},
    {"1%cie_tpqr_finish", cie_tpqr_finish},
    {"5.TransformPQR_scale_WB0", ztpqr_scale_wb0},
    {"5.TransformPQR_scale_WB1", ztpqr_scale_wb1},
    {"5.TransformPQR_scale_WB2", ztpqr_scale_wb2},
    op_def_end(0)
};

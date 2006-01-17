/* Copyright (C) 1992, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zcie.c,v 1.13 2005/07/28 15:24:29 alexcher Exp $ */
/* CIE color operators */
#include "math_.h"
#include "memory_.h"
#include "ghost.h"
#include "oper.h"
#include "gsstruct.h"
#include "gxcspace.h"		/* gscolor2.h requires gscspace.h */
#include "gscolor2.h"
#include "gscie.h"
#include "estack.h"
#include "ialloc.h"
#include "idict.h"
#include "idparam.h"
#include "igstate.h"
#include "icie.h"
#include "isave.h"
#include "ivmspace.h"
#include "store.h"		/* for make_null */

/* Empty procedures */
static const ref empty_procs[4] =
{
    empty_ref_data(t_array, a_readonly | a_executable),
    empty_ref_data(t_array, a_readonly | a_executable),
    empty_ref_data(t_array, a_readonly | a_executable),
    empty_ref_data(t_array, a_readonly | a_executable)
};

/* ------ Parameter extraction utilities ------ */

/* Get a range array parameter from a dictionary. */
/* We know that count <= 4. */
int
dict_ranges_param(const gs_memory_t *mem,
		  const ref * pdref, const char *kstr, int count,
		  gs_range * prange)
{
    int code = dict_floats_param(mem, pdref, kstr, count * 2,
				 (float *)prange, NULL);

    if (code < 0)
	return code;
    else if (code == 0)
	memcpy(prange, Range4_default.ranges, count * sizeof(gs_range));
    return 0;
}

/* Get an array of procedures from a dictionary. */
/* We know count <= countof(empty_procs). */
int
dict_proc_array_param(const gs_memory_t *mem,
		      const ref *pdict, const char *kstr,
		      uint count, ref *pparray)
{
    ref *pvalue;

    if (dict_find_string(pdict, kstr, &pvalue) > 0) {
	uint i;

	check_array_only(*pvalue);
	if (r_size(pvalue) != count)
	    return_error(e_rangecheck);
	for (i = 0; i < count; i++) {
	    ref proc;

	    array_get(mem, pvalue, (long)i, &proc);
	    check_proc_only(proc);
	}
	*pparray = *pvalue;
    } else
	make_const_array(pparray, a_readonly | avm_foreign,
			 count, &empty_procs[0]);
    return 0;
}

/* Get 3 ranges from a dictionary. */
int
dict_range3_param(const gs_memory_t *mem,
		  const ref *pdref, const char *kstr, 
		  gs_range3 *prange3)
{
    return dict_ranges_param(mem, pdref, kstr, 3, prange3->ranges);
}

/* Get a 3x3 matrix from a dictionary. */
int
dict_matrix3_param(const gs_memory_t *mem,
		   const ref *pdref, const char *kstr, gs_matrix3 *pmat3)
{
    /*
     * We can't simply call dict_float_array_param with the matrix
     * cast to a 9-element float array, because compilers may insert
     * padding elements after each of the vectors.  However, we can be
     * confident that there is no padding within a single vector.
     */
    float values[9], defaults[9];
    int code;

    memcpy(&defaults[0], &Matrix3_default.cu, 3 * sizeof(float));
    memcpy(&defaults[3], &Matrix3_default.cv, 3 * sizeof(float));
    memcpy(&defaults[6], &Matrix3_default.cw, 3 * sizeof(float));
    code = dict_floats_param(mem, pdref, kstr, 9, values, defaults);
    if (code < 0)
	return code;
    memcpy(&pmat3->cu, &values[0], 3 * sizeof(float));
    memcpy(&pmat3->cv, &values[3], 3 * sizeof(float));
    memcpy(&pmat3->cw, &values[6], 3 * sizeof(float));
    return 0;
}

/* Get 3 procedures from a dictionary. */
int
dict_proc3_param(const gs_memory_t *mem, const ref *pdref, const char *kstr, ref proc3[3])
{
    return dict_proc_array_param(mem, pdref, kstr, 3, proc3);
}

/* Get WhitePoint and BlackPoint values. */
int
cie_points_param(const gs_memory_t *mem, 
		 const ref * pdref, gs_cie_wb * pwb)
{
    int code;

    if ((code = dict_floats_param(mem, pdref, "WhitePoint", 3, (float *)&pwb->WhitePoint, NULL)) < 0 ||
	(code = dict_floats_param(mem, pdref, "BlackPoint", 3, (float *)&pwb->BlackPoint, (const float *)&BlackPoint_default)) < 0
	)
	return code;
    if (pwb->WhitePoint.u <= 0 ||
	pwb->WhitePoint.v != 1 ||
	pwb->WhitePoint.w <= 0 ||
	pwb->BlackPoint.u < 0 ||
	pwb->BlackPoint.v < 0 ||
	pwb->BlackPoint.w < 0
	)
	return_error(e_rangecheck);
    return 0;
}

/* Process a 3- or 4-dimensional lookup table from a dictionary. */
/* The caller has set pclt->n and pclt->m. */
/* ptref is known to be a readable array of size at least n+1. */
private int cie_3d_table_param(const ref * ptable, uint count, uint nbytes,
			       gs_const_string * strings);
int
cie_table_param(const ref * ptref, gx_color_lookup_table * pclt,
		gs_memory_t * mem)
{
    int n = pclt->n, m = pclt->m;
    const ref *pta = ptref->value.const_refs;
    int i;
    uint nbytes;
    int code;
    gs_const_string *table;

    for (i = 0; i < n; ++i) {
	check_type_only(pta[i], t_integer);
	if (pta[i].value.intval <= 1 || pta[i].value.intval > max_ushort)
	    return_error(e_rangecheck);
	pclt->dims[i] = (int)pta[i].value.intval;
    }
    nbytes = m * pclt->dims[n - 2] * pclt->dims[n - 1];
    if (n == 3) {
	table =
	    gs_alloc_struct_array(mem, pclt->dims[0], gs_const_string,
				  &st_const_string_element, "cie_table_param");
	if (table == 0)
	    return_error(e_VMerror);
	code = cie_3d_table_param(pta + 3, pclt->dims[0], nbytes, table);
    } else {			/* n == 4 */
	int d0 = pclt->dims[0], d1 = pclt->dims[1];
	uint ntables = d0 * d1;
	const ref *psuba;

	check_read_type(pta[4], t_array);
	if (r_size(pta + 4) != d0)
	    return_error(e_rangecheck);
	table =
	    gs_alloc_struct_array(mem, ntables, gs_const_string,
				  &st_const_string_element, "cie_table_param");
	if (table == 0)
	    return_error(e_VMerror);
	psuba = pta[4].value.const_refs;
	/*
	 * We know that d0 > 0, so code will always be set in the loop:
	 * we initialize code to 0 here solely to pacify stupid compilers.
	 */
	for (code = 0, i = 0; i < d0; ++i) {
	    code = cie_3d_table_param(psuba + i, d1, nbytes, table + d1 * i);
	    if (code < 0)
		break;
	}
    }
    if (code < 0) {
	gs_free_object(mem, table, "cie_table_param");
	return code;
    }
    pclt->table = table;
    return 0;
}
private int
cie_3d_table_param(const ref * ptable, uint count, uint nbytes,
		   gs_const_string * strings)
{
    const ref *rstrings;
    uint i;

    check_read_type(*ptable, t_array);
    if (r_size(ptable) != count)
	return_error(e_rangecheck);
    rstrings = ptable->value.const_refs;
    for (i = 0; i < count; ++i) {
	const ref *const prt2 = rstrings + i;

	check_read_type(*prt2, t_string);
	if (r_size(prt2) != nbytes)
	    return_error(e_rangecheck);
	strings[i].data = prt2->value.const_bytes;
	strings[i].size = nbytes;
    }
    return 0;
}

/* ------ CIE setcolorspace ------ */

/* Common code for the CIEBased* cases of setcolorspace. */
private int
cie_lmnp_param(const gs_memory_t *mem, const ref * pdref, gs_cie_common * pcie, ref_cie_procs * pcprocs)
{
    int code;

    if ((code = dict_range3_param(mem, pdref, "RangeLMN", &pcie->RangeLMN)) < 0 ||
	(code = dict_proc3_param(mem, pdref, "DecodeLMN", &pcprocs->DecodeLMN)) < 0 ||
	(code = dict_matrix3_param(mem, pdref, "MatrixLMN", &pcie->MatrixLMN)) < 0 ||
	(code = cie_points_param(mem, pdref, &pcie->points)) < 0
	)
	return code;
    pcie->DecodeLMN = DecodeLMN_default;
    return 0;
}

/* Common code for the CIEBasedABC/DEF[G] cases of setcolorspace. */
private int
cie_abc_param(const gs_memory_t *mem, const ref * pdref, gs_cie_abc * pcie, ref_cie_procs * pcprocs)
{
    int code;

    if ((code = dict_range3_param(mem, pdref, "RangeABC", &pcie->RangeABC)) < 0 ||
	(code = dict_proc3_param(mem, pdref, "DecodeABC", &pcprocs->Decode.ABC)) < 0 ||
	(code = dict_matrix3_param(mem, pdref, "MatrixABC", &pcie->MatrixABC)) < 0 ||
	(code = cie_lmnp_param(mem, pdref, &pcie->common, pcprocs)) < 0
	)
	return code;
    pcie->DecodeABC = DecodeABC_default;
    return 0;
}

/* Finish setting a CIE space (successful or not). */
int
cie_set_finish(i_ctx_t *i_ctx_p, gs_color_space * pcs,
	       const ref_cie_procs * pcprocs, int edepth, int code)
{
    if (code >= 0)
	code = gs_setcolorspace(igs, pcs);
    /* Delete the extra reference to the parameter tables. */
    gs_cspace_release(pcs);
    /* Free the top-level object, which was copied by gs_setcolorspace. */
    gs_free_object(gs_state_memory(igs), pcs, "cie_set_finish");
    if (code < 0) {
	ref_stack_pop_to(&e_stack, edepth);
	return code;
    }
    istate->colorspace.procs.cie = *pcprocs;
    pop(1);
    return (ref_stack_count(&e_stack) == edepth ? 0 : o_push_estack);
}

/* Forward references */
private int cache_common(i_ctx_t *, gs_cie_common *, const ref_cie_procs *,
			 void *, gs_ref_memory_t *);
private int cache_abc_common(i_ctx_t *, gs_cie_abc *, const ref_cie_procs *,
			     void *, gs_ref_memory_t *);

/* <dict> .setciedefgspace - */
private int cie_defg_finish(i_ctx_t *);
private int
zsetciedefgspace(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int edepth = ref_stack_count(&e_stack);
    gs_memory_t *mem = gs_state_memory(igs);
    gs_ref_memory_t *imem = (gs_ref_memory_t *)mem;
    gs_color_space *pcs;
    ref_cie_procs procs;
    gs_cie_defg *pcie;
    int code;
    ref *ptref;

    check_type(*op, t_dictionary);
    check_dict_read(*op);
    if ((code = dict_find_string(op, "Table", &ptref)) <= 0)
	return (code < 0 ? code : gs_note_error(e_rangecheck));
    check_read_type(*ptref, t_array);
    if (r_size(ptref) != 5)
	return_error(e_rangecheck);
    procs = istate->colorspace.procs.cie;
    code = gs_cspace_build_CIEDEFG(&pcs, NULL, mem);
    if (code < 0)
	return code;
    pcie = pcs->params.defg;
    pcie->Table.n = 4;
    pcie->Table.m = 3;
    if ((code = dict_ranges_param(mem, op, "RangeDEFG", 4, pcie->RangeDEFG.ranges)) < 0 ||
	(code = dict_proc_array_param(mem, op, "DecodeDEFG", 4, &procs.PreDecode.DEFG)) < 0 ||
	(code = dict_ranges_param(mem, op, "RangeHIJK", 4, pcie->RangeHIJK.ranges)) < 0 ||
	(code = cie_table_param(ptref, &pcie->Table, mem)) < 0 ||
	(code = cie_abc_param(imemory, op, (gs_cie_abc *) pcie, &procs)) < 0 ||
	(code = cie_cache_joint(i_ctx_p, &istate->colorrendering.procs, (gs_cie_common *)pcie, igs)) < 0 ||	/* do this last */
	(code = cie_cache_push_finish(i_ctx_p, cie_defg_finish, imem, pcie)) < 0 ||
	(code = cie_prepare_cache4(i_ctx_p, &pcie->RangeDEFG,
				   procs.PreDecode.DEFG.value.const_refs,
				   &pcie->caches_defg.DecodeDEFG[0],
				   pcie, imem, "Decode.DEFG")) < 0 ||
	(code = cache_abc_common(i_ctx_p, (gs_cie_abc *)pcie, &procs, pcie, imem)) < 0
	)
	DO_NOTHING;
    return cie_set_finish(i_ctx_p, pcs, &procs, edepth, code);
}
private int
cie_defg_finish(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_cie_defg *pcie = r_ptr(op, gs_cie_defg);

    pcie->DecodeDEFG = DecodeDEFG_from_cache;
    pcie->DecodeABC = DecodeABC_from_cache;
    pcie->common.DecodeLMN = DecodeLMN_from_cache;
    gs_cie_defg_complete(pcie);
    pop(1);
    return 0;
}

/* <dict> .setciedefspace - */
private int cie_def_finish(i_ctx_t *);
private int
zsetciedefspace(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int edepth = ref_stack_count(&e_stack);
    gs_memory_t *mem = gs_state_memory(igs);
    gs_ref_memory_t *imem = (gs_ref_memory_t *)mem;
    gs_color_space *pcs;
    ref_cie_procs procs;
    gs_cie_def *pcie;
    int code;
    ref *ptref;

    check_type(*op, t_dictionary);
    check_dict_read(*op);
    if ((code = dict_find_string(op, "Table", &ptref)) <= 0)
	return (code < 0 ? code : gs_note_error(e_rangecheck));
    check_read_type(*ptref, t_array);
    if (r_size(ptref) != 4)
	return_error(e_rangecheck);
    procs = istate->colorspace.procs.cie;
    code = gs_cspace_build_CIEDEF(&pcs, NULL, mem);
    if (code < 0)
	return code;
    pcie = pcs->params.def;
    pcie->Table.n = 3;
    pcie->Table.m = 3;
    if ((code = dict_range3_param(mem, op, "RangeDEF", &pcie->RangeDEF)) < 0 ||
	(code = dict_proc3_param(mem, op, "DecodeDEF", &procs.PreDecode.DEF)) < 0 ||
	(code = dict_range3_param(mem, op, "RangeHIJ", &pcie->RangeHIJ)) < 0 ||
	(code = cie_table_param(ptref, &pcie->Table, mem)) < 0 ||
	(code = cie_abc_param(imemory, op, (gs_cie_abc *) pcie, &procs)) < 0 ||
	(code = cie_cache_joint(i_ctx_p, &istate->colorrendering.procs, (gs_cie_common *)pcie, igs)) < 0 ||	/* do this last */
	(code = cie_cache_push_finish(i_ctx_p, cie_def_finish, imem, pcie)) < 0 ||
	(code = cie_prepare_cache3(i_ctx_p, &pcie->RangeDEF,
				   procs.PreDecode.DEF.value.const_refs,
				   &pcie->caches_def.DecodeDEF[0],
				   pcie, imem, "Decode.DEF")) < 0 ||
	(code = cache_abc_common(i_ctx_p, (gs_cie_abc *)pcie, &procs, pcie, imem)) < 0
	)
	DO_NOTHING;
    return cie_set_finish(i_ctx_p, pcs, &procs, edepth, code);
}
private int
cie_def_finish(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_cie_def *pcie = r_ptr(op, gs_cie_def);

    pcie->DecodeDEF = DecodeDEF_from_cache;
    pcie->DecodeABC = DecodeABC_from_cache;
    pcie->common.DecodeLMN = DecodeLMN_from_cache;
    gs_cie_def_complete(pcie);
    pop(1);
    return 0;
}

/* <dict> .setcieabcspace - */
private int cie_abc_finish(i_ctx_t *);
private int
zsetcieabcspace(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int edepth = ref_stack_count(&e_stack);
    gs_memory_t *mem = gs_state_memory(igs);
    gs_ref_memory_t *imem = (gs_ref_memory_t *)mem;
    gs_color_space *pcs;
    ref_cie_procs procs;
    gs_cie_abc *pcie;
    int code;

    check_type(*op, t_dictionary);
    check_dict_read(*op);
    procs = istate->colorspace.procs.cie;
    code = gs_cspace_build_CIEABC(&pcs, NULL, mem);
    if (code < 0)
	return code;
    pcie = pcs->params.abc;
    code = cie_abc_param(imemory, op, pcie, &procs);
    if (code < 0 ||
	(code = cie_cache_joint(i_ctx_p, &istate->colorrendering.procs, (gs_cie_common *)pcie, igs)) < 0 ||	/* do this last */
	(code = cie_cache_push_finish(i_ctx_p, cie_abc_finish, imem, pcie)) < 0 ||
	(code = cache_abc_common(i_ctx_p, pcie, &procs, pcie, imem)) < 0
	)
	DO_NOTHING;
    return cie_set_finish(i_ctx_p, pcs, &procs, edepth, code);
}
private int
cie_abc_finish(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_cie_abc *pcie = r_ptr(op, gs_cie_abc);

    pcie->DecodeABC = DecodeABC_from_cache;
    pcie->common.DecodeLMN = DecodeLMN_from_cache;
    gs_cie_abc_complete(pcie);
    pop(1);
    return 0;
}

/* <dict> .setcieaspace - */
private int cie_a_finish(i_ctx_t *);
private int
zsetcieaspace(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int edepth = ref_stack_count(&e_stack);
    gs_memory_t *mem = gs_state_memory(igs);
    gs_ref_memory_t *imem = (gs_ref_memory_t *)mem;
    gs_color_space *pcs;
    ref_cie_procs procs;
    gs_cie_a *pcie;
    int code;

    check_type(*op, t_dictionary);
    check_dict_read(*op);
    procs = istate->colorspace.procs.cie;
    if ((code = dict_proc_param(op, "DecodeA", &procs.Decode.A, true)) < 0)
	return code;
    code = gs_cspace_build_CIEA(&pcs, NULL, mem);
    if (code < 0)
	return code;
    pcie = pcs->params.a;
    if ((code = dict_floats_param(imemory, op, "RangeA", 2, (float *)&pcie->RangeA, (const float *)&RangeA_default)) < 0 ||
	(code = dict_floats_param(imemory, op, "MatrixA", 3, (float *)&pcie->MatrixA, (const float *)&MatrixA_default)) < 0 ||
	(code = cie_lmnp_param(imemory, op, &pcie->common, &procs)) < 0 ||
	(code = cie_cache_joint(i_ctx_p, &istate->colorrendering.procs, (gs_cie_common *)pcie, igs)) < 0 ||	/* do this last */
	(code = cie_cache_push_finish(i_ctx_p, cie_a_finish, imem, pcie)) < 0 ||
	(code = cie_prepare_cache(i_ctx_p, &pcie->RangeA, &procs.Decode.A, &pcie->caches.DecodeA.floats, pcie, imem, "Decode.A")) < 0 ||
	(code = cache_common(i_ctx_p, &pcie->common, &procs, pcie, imem)) < 0
	)
	DO_NOTHING;
    pcie->DecodeA = DecodeA_default;
    return cie_set_finish(i_ctx_p, pcs, &procs, edepth, code);
}
private int
cie_a_finish(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_cie_a *pcie = r_ptr(op, gs_cie_a);

    pcie->DecodeA = DecodeA_from_cache;
    pcie->common.DecodeLMN = DecodeLMN_from_cache;
    gs_cie_a_complete(pcie);
    pop(1);
    return 0;
}

/* Common cache code */

private int
cache_abc_common(i_ctx_t *i_ctx_p, gs_cie_abc * pcie,
		 const ref_cie_procs * pcprocs,
		 void *container, gs_ref_memory_t * imem)
{
    int code =
	cie_prepare_cache3(i_ctx_p, &pcie->RangeABC,
			   pcprocs->Decode.ABC.value.const_refs,
			   pcie->caches.DecodeABC.caches, pcie, imem,
			   "Decode.ABC");

    return (code < 0 ? code :
	    cache_common(i_ctx_p, &pcie->common, pcprocs, pcie, imem));
}

private int
cache_common(i_ctx_t *i_ctx_p, gs_cie_common * pcie,
	     const ref_cie_procs * pcprocs,
	     void *container, gs_ref_memory_t * imem)
{
    return cie_prepare_cache3(i_ctx_p, &pcie->RangeLMN,
			      pcprocs->DecodeLMN.value.const_refs,
			      &pcie->caches.DecodeLMN[0], container, imem,
			      "Decode.LMN");
}

/* ------ Internal routines ------ */

/* Prepare to cache the values for one or more procedures. */
private int cie_cache_finish1(i_ctx_t *);
private int cie_cache_finish(i_ctx_t *);
int
cie_prepare_cache(i_ctx_t *i_ctx_p, const gs_range * domain, const ref * proc,
		  cie_cache_floats * pcache, void *container,
		  gs_ref_memory_t * imem, client_name_t cname)
{
    int space = imemory_space(imem);
    gs_sample_loop_params_t lp;
    es_ptr ep;

    gs_cie_cache_init(&pcache->params, &lp, domain, cname);
    pcache->params.is_identity = r_size(proc) == 0;
    check_estack(9);
    ep = esp;
    make_real(ep + 9, lp.A);
    make_int(ep + 8, lp.N);
    make_real(ep + 7, lp.B);
    ep[6] = *proc;
    r_clear_attrs(ep + 6, a_executable);
    make_op_estack(ep + 5, zcvx);
    make_op_estack(ep + 4, zfor_samples);
    make_op_estack(ep + 3, cie_cache_finish);
    esp += 9;
    /*
     * The caches are embedded in the middle of other
     * structures, so we represent the pointer to the cache
     * as a pointer to the container plus an offset.
     */
    make_int(ep + 2, (char *)pcache - (char *)container);
    make_struct(ep + 1, space, container);
    return o_push_estack;
}
/* Note that pc3 may be 0, indicating that there are only 3 caches to load. */
int
cie_prepare_caches_4(i_ctx_t *i_ctx_p, const gs_range * domains,
		     const ref * procs,
		     cie_cache_floats * pc0, cie_cache_floats * pc1,
		     cie_cache_floats * pc2, cie_cache_floats * pc3,
		     void *container,
		     gs_ref_memory_t * imem, client_name_t cname)
{
    cie_cache_floats *pcn[4];
    int i, n, code = 0;

    pcn[0] = pc0, pcn[1] = pc1, pcn[2] = pc2;
    if (pc3 == 0)
	n = 3;
    else
	pcn[3] = pc3, n = 4;
    for (i = 0; i < n && code >= 0; ++i)
	code = cie_prepare_cache(i_ctx_p, domains + i, procs + i, pcn[i],
				 container, imem, cname);
    return code;
}

/* Store the result of caching one procedure. */
private int
cie_cache_finish_store(i_ctx_t *i_ctx_p, bool replicate)
{
    os_ptr op = osp;
    cie_cache_floats *pcache;
    int code;

    check_esp(2);
    /* See above for the container + offset representation of */
    /* the pointer to the cache. */
    pcache = (cie_cache_floats *) (r_ptr(esp - 1, char) + esp->value.intval);

    pcache->params.is_identity = false;	/* cache_set_linear computes this */
    if_debug3('c', "[c]cache 0x%lx base=%g, factor=%g:\n",
	      (ulong) pcache, pcache->params.base, pcache->params.factor);
    if (replicate ||
	(code = float_params(op, gx_cie_cache_size, &pcache->values[0])) < 0
	) {
	/* We might have underflowed the current stack block. */
	/* Handle the parameters one-by-one. */
	uint i;

	for (i = 0; i < gx_cie_cache_size; i++) {
	    code = float_param(ref_stack_index(&o_stack,
			       (replicate ? 0 : gx_cie_cache_size - 1 - i)),
			       &pcache->values[i]);
	    if (code < 0)
		return code;
	}
    }
#ifdef DEBUG
    if (gs_debug_c('c')) {
	int i;

	for (i = 0; i < gx_cie_cache_size; i += 4)
	    dlprintf5("[c]  cache[%3d]=%g, %g, %g, %g\n", i,
		      pcache->values[i], pcache->values[i + 1],
		      pcache->values[i + 2], pcache->values[i + 3]);
    }
#endif
    ref_stack_pop(&o_stack, (replicate ? 1 : gx_cie_cache_size));
    esp -= 2;			/* pop pointer to cache */
    return o_pop_estack;
}
private int
cie_cache_finish(i_ctx_t *i_ctx_p)
{
    return cie_cache_finish_store(i_ctx_p, false);
}
private int
cie_cache_finish1(i_ctx_t *i_ctx_p)
{
    return cie_cache_finish_store(i_ctx_p, true);
}

/* Push a finishing procedure on the e-stack. */
/* ptr will be the top element of the o-stack. */
int
cie_cache_push_finish(i_ctx_t *i_ctx_p, op_proc_t finish_proc,
		      gs_ref_memory_t * imem, void *data)
{
    check_estack(2);
    push_op_estack(finish_proc);
    ++esp;
    make_struct(esp, imemory_space(imem), data);
    return o_push_estack;
}

/* ------ Initialization procedure ------ */

const op_def zcie_l2_op_defs[] =
{
    op_def_begin_level2(),
    {"1.setcieaspace", zsetcieaspace},
    {"1.setcieabcspace", zsetcieabcspace},
    {"1.setciedefspace", zsetciedefspace},
    {"1.setciedefgspace", zsetciedefgspace},
		/* Internal operators */
    {"1%cie_defg_finish", cie_defg_finish},
    {"1%cie_def_finish", cie_def_finish},
    {"1%cie_abc_finish", cie_abc_finish},
    {"1%cie_a_finish", cie_a_finish},
    {"0%cie_cache_finish", cie_cache_finish},
    {"1%cie_cache_finish1", cie_cache_finish1},
    op_def_end(0)
};

/* Copyright (C) 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* zcie.c */
/* CIE color operators */
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "gsstruct.h"
#include "gscspace.h"
#include "gscolor2.h"
#include "gscie.h"
#include "estack.h"
#include "ialloc.h"
#include "idict.h"
#include "idparam.h"
#include "igstate.h"
#include "isave.h"
#include "ivmspace.h"
#include "store.h"		/* for make_null */

/* There are actually only two CIE-specific operators, */
/* but CIE color dictionaries are so complex that */
/* we handle the CIE case of setcolorspace here as well. */

/* Forward references */
private int cache_colorrendering(P3(gs_cie_render *,
  const ref_cie_render_procs *, gs_state *));
private int cache_common(P4(gs_cie_common *, const ref_cie_procs *,
  const ref_cie_render_procs *, gs_state *));

/* Allocator structure types for CIE structures */
private_st_cie_abc();
private_st_cie_a();
private_st_cie_render();

/* Empty procedures */
static ref empty_procs[3];

/* Original CIE color space types */
extern const gs_color_space_type
	gs_color_space_type_CIEBasedABC,
	gs_color_space_type_CIEBasedA;
/* Redefined CIE color space types (that load the cache when installed) */
gs_color_space_type
	cs_type_zCIEBasedABC,
	cs_type_zCIEBasedA;
private cs_proc_install_cspace(cs_install_zCIEBasedABC);
private cs_proc_install_cspace(cs_install_zCIEBasedA);

/* Initialization */
private void
zcie_init(void)
{
	/* Make the null (default) transformation procedures. */
	make_empty_const_array(&empty_procs[0], a_readonly + a_executable);
	make_empty_const_array(&empty_procs[1], a_readonly + a_executable);
	make_empty_const_array(&empty_procs[2], a_readonly + a_executable);

	/* Create the modified color space types. */
	cs_type_zCIEBasedABC = gs_color_space_type_CIEBasedABC;
	cs_type_zCIEBasedABC.install_cspace = cs_install_zCIEBasedABC;
	cs_type_zCIEBasedA = gs_color_space_type_CIEBasedA;
	cs_type_zCIEBasedA.install_cspace = cs_install_zCIEBasedA;

}

/* ------ CIE setcolorspace ------ */

/* Get a 3-element range parameter from a dictionary. */
#define dict_range3_param(op, kstr, prange)\
	dict_float_array_param(op, kstr, 6, (float *)prange, (float *)&Range3_default)
#define range3_ok 6

/* Get a 3x3 matrix parameter from a dictionary. */
#define dict_matrix3_param(op, kstr, pmat)\
	dict_float_array_param(op, kstr, 9, (float *)pmat, (float *)&Matrix3_default)
#define matrix3_ok 9

/* Get an array of procedures from a dictionary. */
/* We know count <= 3. */
private int
dict_proc_array_param(const ref *pdict, const char _ds *kstr,
  uint count, ref *pparray)
{	ref *pvalue;
	if ( dict_find_string(pdict, kstr, &pvalue) > 0 )
	{	uint i;
		check_array_only(*pvalue);
		if ( r_size(pvalue) != count )
			return_error(e_rangecheck);
		for ( i = 0; i < count; i++ )
		{	ref proc;
			array_get(pvalue, (long)i, &proc);
			check_proc_only(proc);
		}
		*pparray = *pvalue;
	}
	else
		make_const_array(pparray, a_readonly | avm_foreign,
				 count, &empty_procs[0]);
	return 0;
}

/* Get 3 procedures from a dictionary. */
#define dict_proc3_param(op, kstr, pparray)\
	dict_proc_array_param(op, kstr, 3, pparray)
		
/* Shared code for getting WhitePoint and BlackPoint values. */
private int
cie_points_param(const ref *pdref, gs_cie_wb *pwb)
{	int code;
	if ( (code = dict_float_array_param(pdref, "WhitePoint", 3, (float *)&pwb->WhitePoint, NULL)) != 3 ||
	     (code = dict_float_array_param(pdref, "BlackPoint", 3, (float *)&pwb->BlackPoint, (float *)&BlackPoint_default)) != 3
	   )
		return (code < 0 ? code : e_rangecheck);
	if ( pwb->WhitePoint.u <= 0 ||
	     pwb->WhitePoint.v != 1 ||
	     pwb->WhitePoint.w <= 0 ||
	     pwb->BlackPoint.u < 0 ||
	     pwb->BlackPoint.v < 0 ||
	     pwb->BlackPoint.w < 0
	   )
		return_error(e_rangecheck);
	return 0;
}

/* Common code for the CIEBasedA[BC] cases of setcolorspace. */
private int
cie_lmnp_param(const ref *pdref, gs_cie_common *pcie, ref_cie_procs *pcprocs)
{	int code;
	if ( (code = dict_range3_param(pdref, "RangeLMN", &pcie->RangeLMN)) != range3_ok ||
	     (code = dict_proc3_param(pdref, "DecodeLMN", &pcprocs->DecodeLMN)) < 0 ||
	     (code = dict_matrix3_param(pdref, "MatrixLMN", &pcie->MatrixLMN)) != matrix3_ok ||
	     (code = cie_points_param(pdref, &pcie->points)) < 0
	   )
		return (code < 0 ? code : e_rangecheck);
	pcie->DecodeLMN = DecodeLMN_default;
	return 0;
}

/* <dict> .setcieabcspace - */
int
zsetcieabcspace(register os_ptr op)
{	gs_memory_t *mem = gs_state_memory(igs);
	gs_color_space cs;
	ref_color_procs procs;
	ref_colorspace cspace_old;
	uint edepth = ref_stack_count(&e_stack);
	gs_cie_abc *pcie;
	int code;

	check_type(*op, t_dictionary);
	check_dict_read(*op);
	procs = istate->colorspace.procs;
	rc_alloc_struct_0(pcie, gs_cie_abc, &st_cie_abc, mem,
			  return_error(e_VMerror),
			  "setcolorspace(CIEBasedABC)");
	if ( (code = dict_range3_param(op, "RangeABC", &pcie->RangeABC)) != range3_ok ||
	     (code = dict_proc3_param(op, "DecodeABC", &procs.cie.Decode.ABC)) < 0 ||
	     (code = dict_matrix3_param(op, "MatrixABC", &pcie->MatrixABC)) != matrix3_ok ||
	     (code = cie_lmnp_param(op, &pcie->common, &procs.cie)) < 0
	   )
	{	rc_free_struct(pcie, mem, "setcolorspace(CIEBasedABC)");
		return (code < 0 ? code : e_rangecheck);
	}
	pcie->DecodeABC = DecodeABC_default;
	cs.params.abc = pcie;
	cs.type = &cs_type_zCIEBasedABC;
	/* The color space installation procedure may refer to */
	/* istate->colorspace.procs. */
	cspace_old = istate->colorspace;
	istate->colorspace.procs = procs;
	code = gs_setcolorspace(igs, &cs);
	if ( code < 0 )
	{	istate->colorspace = cspace_old;
		ref_stack_pop_to(&e_stack, edepth);
		return code;
	}
	pop(1);
	return (ref_stack_count(&e_stack) == edepth ? 0 : o_push_estack);  /* installation will load the caches */
}

/* <dict> .setcieaspace - */
int
zsetcieaspace(register os_ptr op)
{	gs_memory_t *mem = gs_state_memory(igs);
	gs_color_space cs;
	ref_color_procs procs;
	ref_colorspace cspace_old;
	uint edepth = ref_stack_count(&e_stack);
	gs_cie_a *pcie;
	int code;

	check_type(*op, t_dictionary);
	check_dict_read(*op);
	procs = istate->colorspace.procs;
	if ( (code = dict_proc_param(op, "DecodeA", &procs.cie.Decode.A, true)) < 0 )
		return code;
	rc_alloc_struct_0(pcie, gs_cie_a, &st_cie_a, mem,
			  return_error(e_VMerror),
			  "setcolorspace(CIEBasedA)");
	if ( (code = dict_float_array_param(op, "RangeA", 2, (float *)&pcie->RangeA, (float *)&RangeA_default)) != 2 ||
	     (code = dict_float_array_param(op, "MatrixA", 3, (float *)&pcie->MatrixA, (float *)&MatrixA_default)) != 3 ||
	     (code = cie_lmnp_param(op, &pcie->common, &procs.cie)) < 0
	   )
	{	rc_free_struct(pcie, mem, "setcolorspace(CIEBasedA)");
		return (code < 0 ? code : e_rangecheck);
	}
	pcie->DecodeA = DecodeA_default;
	cs.params.a = pcie;
	cs.type = &cs_type_zCIEBasedA;
	/* The color space installation procedure may refer to */
	/* istate->colorspace.procs. */
	cspace_old = istate->colorspace;
	istate->colorspace.procs = procs;
	code = gs_setcolorspace(igs, &cs);
	if ( code < 0 )
	{	istate->colorspace = cspace_old;
		ref_stack_pop_to(&e_stack, edepth);
		return code;
	}
	pop(1);
	return (ref_stack_count(&e_stack) == edepth ? 0 : o_push_estack);  /* installation will load the caches */
}

/* ------ CIE rendering dictionary ------ */

/* - currentcolorrendering <dict> */
private int
zcurrentcolorrendering(register os_ptr op)
{	push(1);
	*op = istate->colorrendering.dict;
	return 0;
}

/* <dict> setcolorrendering - */
private int zsetcolorrendering_internal(P4(os_ptr, gs_cie_render *, ref_cie_render_procs *, gs_memory_t *));
private int
zsetcolorrendering(register os_ptr op)
{	gs_memory_t *mem = gs_state_memory(igs);
	int code;
	es_ptr ep = esp;
	gs_cie_render *pcie;
	ref_cie_render_procs procs_old;
	check_read_type(*op, t_dictionary);
	check_dict_read(*op);
	rc_alloc_struct_0(pcie, gs_cie_render, &st_cie_render, mem,
			  return_error(e_VMerror),
			  "setcolorrendering");
	/* gs_setcolorrendering may refer to istate->colorrendering.procs. */
	procs_old = istate->colorrendering.procs;
	code = zsetcolorrendering_internal(op, pcie, &istate->colorrendering.procs, mem);
	if ( code < 0 )
	{	rc_free_struct(pcie, mem, "setcolorrendering");
		istate->colorrendering.procs = procs_old;
		esp = ep;
		return code;
	}
	istate->colorrendering.dict = *op;
	pop(1);
	return (esp == ep ? 0 : o_push_estack);
}
private int
zsetcolorrendering_internal(os_ptr op, gs_cie_render *pcie,
  ref_cie_render_procs *pcprocs, gs_memory_t *mem)
{	int code;
	int ignore;
	ref *pRT;
	if ( (code = dict_int_param(op, "ColorRenderingType", 1, 1, 0, &ignore)) < 0 ||
	     (code = dict_matrix3_param(op, "MatrixLMN", &pcie->MatrixLMN)) != matrix3_ok ||
	     (code = dict_proc3_param(op, "EncodeLMN", &pcprocs->EncodeLMN)) < 0 ||
	     (code = dict_range3_param(op, "RangeLMN", &pcie->RangeLMN)) != range3_ok ||
	     (code = dict_matrix3_param(op, "MatrixABC", &pcie->MatrixABC)) != matrix3_ok ||
	     (code = dict_proc3_param(op, "EncodeABC", &pcprocs->EncodeABC)) < 0 ||
	     (code = dict_range3_param(op, "RangeABC", &pcie->RangeABC)) != range3_ok ||
	     (code = cie_points_param(op, &pcie->points)) < 0 ||
	     (code = dict_matrix3_param(op, "MatrixPQR", &pcie->MatrixPQR)) != matrix3_ok ||
	     (code = dict_range3_param(op, "RangePQR", &pcie->RangePQR)) != range3_ok ||
	     (code = dict_proc3_param(op, "TransformPQR", &pcprocs->TransformPQR)) < 0
	   )
		return (code < 0 ? code : e_rangecheck);
#define rRT pcie->RenderTable
	if ( dict_find_string(op, "RenderTable", &pRT) > 0 )
	{	const ref *prte;
		int i;
		uint n2;
		const ref *strings;
		check_read_type(*pRT, t_array);
		prte = pRT->value.const_refs;
		check_type_only(prte[0], t_integer);
		check_type_only(prte[1], t_integer);
		check_type_only(prte[2], t_integer);
		check_read_type(prte[3], t_array);
		check_type_only(prte[4], t_integer);
		if ( prte[0].value.intval <= 1 ||
		     prte[1].value.intval <= 1 ||
		     prte[2].value.intval <= 1 ||
		     !(prte[4].value.intval == 3 || prte[4].value.intval == 4)
		   )
		  return_error(e_rangecheck);
		rRT.NA = prte[0].value.intval;
		rRT.NB = prte[1].value.intval;
		rRT.NC = prte[2].value.intval;
		rRT.m = prte[4].value.intval;
		n2 = rRT.m * rRT.NB * rRT.NC;
		if ( r_size(pRT) != rRT.m + 5 || r_size(&prte[3]) != rRT.NA )
		  return_error(e_rangecheck);
		strings = prte[3].value.const_refs;
		for ( i = 0; i < rRT.NA; i++ )
		{	const ref *prt2 = strings + i;
			check_read_type(*prt2, t_string);
			if ( r_size(prt2) != n2 )
			  return_error(e_rangecheck);
		}
		prte += 5;
		for ( i = 0; i < rRT.m; i++ )
		{	const ref *prt2 = prte + i;
			check_proc_only(*prt2);
		}
		/* gs_alloc_byte_array is ****** WRONG ****** */
		rRT.table = (gs_const_string *)gs_alloc_byte_array(mem, rRT.NA,
						sizeof(gs_const_string),
						"setcolorrendering(table)");
		if ( rRT.table == 0 )
		  return_error(e_VMerror);
		for ( i = 0; i < rRT.NA; i++ )
		  {	rRT.table[i].data = strings[i].value.bytes;
			rRT.table[i].size = n2;
		  }
		make_const_array(&pcprocs->RenderTableT,
				 a_readonly | r_space(pRT),
				 rRT.m, prte);
	}
	else
	{	rRT.table = 0;
		make_null(&pcprocs->RenderTableT);
	}
#undef rRT
	pcie->EncodeLMN = Encode_default;
	pcie->EncodeABC = Encode_default;
	pcie->TransformPQR = TransformPQR_default;
	pcie->RenderTable.T = RenderTableT_default;
	code = cache_colorrendering(pcie, pcprocs, igs);
	if ( code < 0 )
	  return code;
	return gs_setcolorrendering(igs, pcie);
}

/* ------ Internal routines ------ */

/* Import operators. */
extern int
  zfor(P1(os_ptr)),
  zcvx(P1(os_ptr));
extern int zexec(P1(os_ptr));
/* Import accessors. */
extern gx_cie_joint_caches *gx_currentciecaches(P1(gs_state *));

/* Forward declarations */
private int
  cie_cache_finish(P1(os_ptr)),
  cie_exec_tpqr(P1(os_ptr)),
  cie_tpqr_finish(P1(os_ptr));

/* Transform a set of ranges. */
private void
cie_transform_range(const gs_range3 *in, const gs_vector3 *col,
  gs_range *out)
{	float umin = col->u * in->u.rmin, umax = col->u * in->u.rmax;
	float vmin = col->v * in->v.rmin, vmax = col->v * in->v.rmax;
	float wmin = col->w * in->w.rmin, wmax = col->w * in->w.rmax;
	float temp;
#define swap(x, y) temp = x, x = y, y = temp
	if ( umin > umax ) swap(umin, umax);
	if ( vmin > vmax ) swap(vmin, vmax);
	if ( wmin > wmax ) swap(wmin, wmax);
	out->rmin = umin + vmin + wmin;
	out->rmax = umax + vmax + wmax;
#undef swap
}
private void
cie_transform_range3(const gs_range3 *in, const gs_matrix3 *mat,
  gs_range3 *out)
{	cie_transform_range(in, &mat->cu, &out->u);
	cie_transform_range(in, &mat->cv, &out->v);
	cie_transform_range(in, &mat->cw, &out->w);
}

/* Prepare to cache the values for one or more procedures. */
private int
cie_prepare_caches(const gs_range *domain, const ref *proc,
  gx_cie_cache *pcache, int n)
{	check_estack(n * 8);
	for ( ; --n >= 0; domain++, proc++, pcache++, esp += 8 )
	{	float diff = domain->rmax - domain->rmin;
		float delta = diff / (gx_cie_cache_size - 1);
		register es_ptr ep = esp;
		/* Zero out the cache, since the gs level will try to */
		/* access it before it has been filled. */
		{	register float *pcv = &pcache->values[0];
			register int i;
			for ( i = 0; i < gx_cie_cache_size; i++, pcv++ )
				*pcv = 0.0;
		}
		pcache->base = domain->rmin - delta / 2;	/* so lookup will round */
		pcache->factor = (delta == 0 ? 0 : 1 / delta);
		pcache->is_identity = r_size(proc) == 0;
		if_debug3('c', "[c]cache 0x%lx base=%g, factor=%g\n",
			  (ulong)pcache, pcache->base, pcache->factor);
		make_real(ep + 8, domain->rmin);
		make_real(ep + 7, delta);
		make_real(ep + 6, domain->rmax + delta / 2);
		ep[5] = *proc;
		r_clear_attrs(ep + 5, a_executable);
		make_op_estack(ep + 4, zcvx);
		make_op_estack(ep + 3, zfor);
		make_op_estack(ep + 2, cie_cache_finish);
		make_string(ep + 1, 0, sizeof(*pcache), (byte *)pcache);
	}
	return o_push_estack;
}
/* Prepare to cache the values for 3 procedures. */
#define cie_prepare_cache3(d3,p3,c3)\
  cie_prepare_caches((const gs_range *)(d3), p3, c3, 3)

/* Store the result of caching one procedure. */
private int
cie_cache_finish(os_ptr op)
{	gx_cie_cache *pcache;
	int code;
	check_esp(1);
	/* The following should be
		pcache = r_ptr(esp, gx_cie_cache);
	 * but we can't do this right now, because the caches are
	 * embedded in the middle of another structure.
	 */
	pcache = (gx_cie_cache *)esp->value.bytes;
	code = num_params(op, gx_cie_cache_size, &pcache->values[0]);
	if_debug3('c', "[c]cache 0x%lx base=%g, factor=%g:\n",
		  (ulong)pcache, pcache->base, pcache->factor);
	if ( code < 0 )
	  {	/* We might have underflowed the current stack block. */
		/* Handle the parameters one-by-one. */
		uint i;
		for ( i = 0; i < gx_cie_cache_size; i++ )
		  {	code = real_param(ref_stack_index(&o_stack,
						gx_cie_cache_size - 1 - i),
					  &pcache->values[i]);
			if ( code < 0 )
			  return code;
		  }
	  }
#ifdef DEBUG
	if ( gs_debug_c('c') )
	{	int i;
		for ( i = 0; i < gx_cie_cache_size; i++ )
		  dprintf2("[c]cache[%3d]=%g\n", i, pcache->values[i]);
	}
#endif
	ref_stack_pop(&o_stack, gx_cie_cache_size);
	esp--;				/* pop pointer to cache */
	return o_pop_estack;
}

/* Install a CIE-based color space. */

private int
cs_install_zCIEBasedABC(gs_color_space *pcs, gs_state *pgs)
{	es_ptr ep = esp;
	gs_cie_abc *pcie = pcs->params.abc;
	const int_gstate *pigs = gs_int_gstate(pgs);
	const ref_cie_procs *pcprocs = &pigs->colorspace.procs.cie;
	int code =
	  (*gs_color_space_type_CIEBasedABC.install_cspace)(pcs, pgs);	/* former routine */
	if ( code < 0 ) return code;
	code = cie_prepare_cache3(&pcie->RangeABC, pcprocs->Decode.ABC.value.const_refs, &pcie->caches.DecodeABC[0]);
	if ( code < 0 ||
	     (code = cache_common(&pcie->common, pcprocs, &pigs->colorrendering.procs, pgs)) < 0
	   )
	{	esp = ep;
		return code;
	}
	return o_push_estack;
}

private int
cs_install_zCIEBasedA(gs_color_space *pcs, gs_state *pgs)
{	es_ptr ep = esp;
	gs_cie_a *pcie = pcs->params.a;
	const int_gstate *pigs = gs_int_gstate(pgs);
	const ref_cie_procs *pcprocs = &pigs->colorspace.procs.cie;
	int code =
	  (*gs_color_space_type_CIEBasedA.install_cspace)(pcs, pgs);	/* former routine */
	if ( code < 0 ) return code;
	code = cie_prepare_caches(&pcie->RangeA, &pcprocs->Decode.A, &pcie->caches.DecodeA, 1);
	if ( code < 0 ||
	     (code = cache_common(&pcie->common, pcprocs, &pigs->colorrendering.procs, pgs)) < 0
	   )
	{	esp = ep;
		return code;
	}
	return o_push_estack;
}

/* Cache the results of the color rendering procedures. */
private int
cache_colorrendering(gs_cie_render *pcie,
  const ref_cie_render_procs *pcrprocs, gs_state *pgs)
{	gs_range3 DomainLMN;
	gs_range3 DomainABC;
	es_ptr ep = esp;
	static const gs_range ranges_01[4] =
		{ {0,1}, {0,1}, {0,1}, {0,1} };
	int code = gs_cie_render_init(pcie);	/* sets PQR'*LMN */
	if ( code < 0 ) return code;
	cie_transform_range3(&pcie->RangePQR, &pcie->MatrixPQR_inverse_LMN, &DomainLMN);
	cie_transform_range3(&pcie->RangeLMN, &pcie->MatrixABC, &DomainABC);
	if ( (code = cie_prepare_cache3(&DomainLMN, pcrprocs->EncodeLMN.value.const_refs, &pcie->caches.EncodeLMN[0])) < 0 ||
	     (code = cie_prepare_cache3(&DomainABC, pcrprocs->EncodeABC.value.const_refs, &pcie->caches.EncodeABC[0])) < 0 ||
	     (pcie->RenderTable.table != 0 &&
	      (code = cie_prepare_caches(ranges_01, pcrprocs->RenderTableT.value.const_refs, &pcie->caches.RenderTableT[0], pcie->RenderTable.m)) < 0)
	   )
	{	esp = ep;
		return code;
	}
	/* gs_setcolorrendering reinstalls the color space, */
	/* which reloads the joint caches if needed. */
	return o_push_estack;
}

/* Common cache code */
private int
cache_common(gs_cie_common *pcie, const ref_cie_procs *pcprocs,
  const ref_cie_render_procs *pcrprocs, gs_state *pgs)
{	int code = cie_prepare_cache3(&pcie->RangeLMN,
				      pcprocs->DecodeLMN.value.const_refs,
				      &pcie->caches.DecodeLMN[0]);
	const gs_cie_render *pcier = gs_currentcolorrendering(pgs);
	/* The former installation procedures have allocated */
	/* the joint caches and filled in points_sd. */
	gx_cie_joint_caches *pjc = gx_currentciecaches(pgs);
	ref pqr_procs;
#define pqr_refs pqr_procs.value.refs
	uint space;
	int i;
	if ( code < 0 ) return code;
	if ( pcier == 0 ) return 0;	/* cache is not used */
	check_estack(2);
	code = ialloc_ref_array(&pqr_procs, a_readonly, 3*(1+3+4*6), "cie_cache_common");
	if ( code < 0 ) return code;
	/* Make sure we deallocate the procs when we're done. */
	push_op_estack(cie_tpqr_finish);
	*++esp = pqr_procs;
	space = r_space(&pqr_procs);
	for ( i = 0; i < 3; i++ )
	{	ref *p = pqr_refs + 3 + (3+4*6) * i;
		const float *ppt = (float *)&pjc->points_sd;
		int j;
		make_array(pqr_refs + i, a_readonly | a_executable | space, 3, p);
		make_array(p, a_readonly | space, 4*6, p + 3);
		p[1] = pcrprocs->TransformPQR.value.refs[i];
		make_oper(p + 2, 0, cie_exec_tpqr);
		for ( j = 0, p += 3; j < 4*6; j++, p++, ppt++ )
			make_real(p, *ppt);
	}
	return cie_prepare_cache3(&pcier->RangePQR,
				  pqr_procs.value.const_refs,
				  &pjc->TransformPQR[0]);
}

/* Private operator to shuffle arguments for the TransformPQR procedure: */
/* v [ws wd bs bd] proc -> ws wd bs bd v proc + exec */
private int
cie_exec_tpqr(register os_ptr op)
{	const ref *ppt = op[-1].value.const_refs;
	uint space = r_space(op - 1);
	int i;
	check_op(3);
	push(3);
	*op = op[-3];		/* proc */
	op[-1] = op[-5];	/* v */
	for ( i = 0; i < 4; i++ )
		make_const_array(op - 5 + i, a_readonly | space,
				 6, ppt + i * 6);
	return zexec(op);
}

/* Private operator to free procs array. */
private int
cie_tpqr_finish(register os_ptr op)
{	ifree_ref_array(op, "cie_tpqr_finish");
	pop(1);
	return 0;
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zcie_l2_op_defs) {
		op_def_begin_level2(),
	{"1.setcieaspace", zsetcieaspace},
	{"1.setcieabcspace", zsetcieabcspace},
	{"0currentcolorrendering", zcurrentcolorrendering},
	{"1setcolorrendering", zsetcolorrendering},
		/* Internal operators */
	{"0%cie_cache_finish", cie_cache_finish},
	{"3%cie_exec_tpqr", cie_exec_tpqr},
	{"1%cie_tpqr_finish", cie_tpqr_finish},
END_OP_DEFS(zcie_init) }

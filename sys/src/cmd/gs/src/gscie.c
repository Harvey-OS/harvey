/* Copyright (C) 1992, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gscie.c,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* CIE color rendering cache management */
#include "math_.h"
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gsmatrix.h"		/* for gscolor2.h */
#include "gxcspace.h"		/* for gxcie.c */
#include "gscolor2.h"		/* for gs_set/currentcolorrendering */
#include "gxarith.h"
#include "gxcie.h"
#include "gxdevice.h"		/* for gxcmap.h */
#include "gxcmap.h"
#include "gzstate.h"

/* Forward references */
private int cie_joint_caches_init(P3(gx_cie_joint_caches *,
				     const gs_cie_common *,
				     gs_cie_render *));
private void cie_joint_caches_complete(P4(gx_cie_joint_caches *,
					  const gs_cie_common *,
					  const gs_cie_abc *,
					  const gs_cie_render *));
private void cie_cache_restrict(P2(cie_cache_floats *, const gs_range *));
private void cie_mult3(P3(const gs_vector3 *, const gs_matrix3 *,
			  gs_vector3 *));
private void cie_matrix_mult3(P3(const gs_matrix3 *, const gs_matrix3 *,
				 gs_matrix3 *));
private void cie_invert3(P2(const gs_matrix3 *, gs_matrix3 *));
private void cie_matrix_init(P1(gs_matrix3 *));

/* Allocator structure types */
private_st_joint_caches();

#define RESTRICTED_INDEX(v, n, itemp)\
  ((uint)(itemp = (int)(v)) >= (n) ?\
   (itemp < 0 ? 0 : (n) - 1) : itemp)

/* Define the template for loading a cache. */
/* If we had parameterized types, or a more flexible type system, */
/* this could be done with a single procedure. */
#define CIE_LOAD_CACHE_BODY(pcache, domains, rprocs, dprocs, pcie, cname)\
  BEGIN\
	int j;\
\
	for (j = 0; j < countof(pcache); j++) {\
	    cie_cache_floats *pcf = &(pcache)[j].floats;\
	    int i;\
	    gs_for_loop_params lp;\
\
	    gs_cie_cache_init(&pcf->params, &lp, &(domains)[j], cname);\
	    for (i = 0; i < gx_cie_cache_size; lp.init += lp.step, i++) {\
		pcf->values[i] = (*(rprocs)->procs[j])(lp.init, pcie);\
		if_debug5('C', "[C]%s[%d,%d] = %g => %g\n",\
			  cname, j, i, lp.init, pcf->values[i]);\
	    }\
	    pcf->params.is_identity =\
		(rprocs)->procs[j] == (dprocs).procs[j];\
	}\
  END

/*
 * Determine whether a function is a linear transformation of the form
 * f(x) = scale * x + origin.
 */
private bool
cache_is_linear(cie_linear_params_t *params, const cie_cache_floats *pcf)
{
    double origin = pcf->values[0];
    double diff = pcf->values[countof(pcf->values) - 1] - origin;
    double scale = diff / (countof(pcf->values) - 1);
    int i;
    double test = origin + scale;

    for (i = 1; i < countof(pcf->values) - 1; ++i, test += scale)
	if (fabs(pcf->values[i] - test) >= 0.5 / countof(pcf->values))
	    return (params->is_linear = false);
    params->origin = origin - pcf->params.base;
    params->scale = diff * pcf->params.factor / (countof(pcf->values) - 1);
    return (params->is_linear = true);
}

private void
cache_set_linear(cie_cache_floats *pcf)
{
	if (pcf->params.is_identity) {
	    if_debug1('c', "[c]linear(0x%lx) = true (is_identity)\n",
		      (ulong)pcf);
	    pcf->params.linear.is_linear = true;
	    pcf->params.linear.origin = 0;
	    pcf->params.linear.scale = 1;
	} else if (cache_is_linear(&pcf->params.linear, pcf)) {
	    if (pcf->params.linear.origin == 0 &&
		fabs(pcf->params.linear.scale - 1) < 0.00001)
		pcf->params.is_identity = true;
	    if_debug4('c',
		      "[c]linear(0x%lx) = true, origin = %g, scale = %g%s\n",
		      (ulong)pcf, pcf->params.linear.origin,
		      pcf->params.linear.scale,
		      (pcf->params.is_identity ? " (=> is_identity)" : ""));
	}
#ifdef DEBUG
	else
	    if_debug1('c', "[c]linear(0x%lx) = false\n", (ulong)pcf);
#endif
}
private void
cache3_set_linear(gx_cie_vector_cache *caches /*[3]*/)
{
    cache_set_linear(&caches[0].floats);
    cache_set_linear(&caches[1].floats);
    cache_set_linear(&caches[2].floats);
}

#ifdef DEBUG
private void
if_debug_vector3(const char *str, const gs_vector3 *vec)
{
    if_debug4('c', "%s[%g %g %g]\n", str, vec->u, vec->v, vec->w);
}
private void
if_debug_matrix3(const char *str, const gs_matrix3 *mat)
{
    if_debug10('c', "%s [%g %g %g] [%g %g %g] [%g %g %g]\n", str,
	       mat->cu.u, mat->cu.v, mat->cu.w,
	       mat->cv.u, mat->cv.v, mat->cv.w,
	       mat->cw.u, mat->cw.v, mat->cw.w);
}
#else
#  define if_debug_vector3(str, vec) DO_NOTHING
#  define if_debug_matrix3(str, mat) DO_NOTHING
#endif

/* ------ Default values for CIE dictionary elements ------ */

/* Default transformation procedures. */

private float
a_identity(floatp in, const gs_cie_a * pcie)
{
    return in;
}
private float
a_from_cache(floatp in, const gs_cie_a * pcie)
{
    return gs_cie_cached_value(in, &pcie->caches.DecodeA.floats);
}

private float
abc_identity(floatp in, const gs_cie_abc * pcie)
{
    return in;
}
private float
abc_from_cache_0(floatp in, const gs_cie_abc * pcie)
{
    return gs_cie_cached_value(in, &pcie->caches.DecodeABC[0].floats);
}
private float
abc_from_cache_1(floatp in, const gs_cie_abc * pcie)
{
    return gs_cie_cached_value(in, &pcie->caches.DecodeABC[1].floats);
}
private float
abc_from_cache_2(floatp in, const gs_cie_abc * pcie)
{
    return gs_cie_cached_value(in, &pcie->caches.DecodeABC[2].floats);
}

private float
def_identity(floatp in, const gs_cie_def * pcie)
{
    return in;
}
private float
def_from_cache_0(floatp in, const gs_cie_def * pcie)
{
    return gs_cie_cached_value(in, &pcie->caches_def.DecodeDEF[0].floats);
}
private float
def_from_cache_1(floatp in, const gs_cie_def * pcie)
{
    return gs_cie_cached_value(in, &pcie->caches_def.DecodeDEF[1].floats);
}
private float
def_from_cache_2(floatp in, const gs_cie_def * pcie)
{
    return gs_cie_cached_value(in, &pcie->caches_def.DecodeDEF[2].floats);
}

private float
defg_identity(floatp in, const gs_cie_defg * pcie)
{
    return in;
}
private float
defg_from_cache_0(floatp in, const gs_cie_defg * pcie)
{
    return gs_cie_cached_value(in, &pcie->caches_defg.DecodeDEFG[0].floats);
}
private float
defg_from_cache_1(floatp in, const gs_cie_defg * pcie)
{
    return gs_cie_cached_value(in, &pcie->caches_defg.DecodeDEFG[1].floats);
}
private float
defg_from_cache_2(floatp in, const gs_cie_defg * pcie)
{
    return gs_cie_cached_value(in, &pcie->caches_defg.DecodeDEFG[2].floats);
}
private float
defg_from_cache_3(floatp in, const gs_cie_defg * pcie)
{
    return gs_cie_cached_value(in, &pcie->caches_defg.DecodeDEFG[3].floats);
}

private float
common_identity(floatp in, const gs_cie_common * pcie)
{
    return in;
}
private float
lmn_from_cache_0(floatp in, const gs_cie_common * pcie)
{
    return gs_cie_cached_value(in, &pcie->caches.DecodeLMN[0].floats);
}
private float
lmn_from_cache_1(floatp in, const gs_cie_common * pcie)
{
    return gs_cie_cached_value(in, &pcie->caches.DecodeLMN[1].floats);
}
private float
lmn_from_cache_2(floatp in, const gs_cie_common * pcie)
{
    return gs_cie_cached_value(in, &pcie->caches.DecodeLMN[2].floats);
}

/* Transformation procedures for accessing an already-loaded cache. */

float
gs_cie_cached_value(floatp in, const cie_cache_floats *pcache)
{
    /*
     * We need to get the same results when we sample an already-loaded
     * cache, so we need to round the index just a tiny bit.
     */
    int index =
	(int)((in - pcache->params.base) * pcache->params.factor + 0.0001);

    CIE_CLAMP_INDEX(index);
    return pcache->values[index];
}

/* Default vectors and matrices. */

const gs_range3 Range3_default = {
    { {0, 1}, {0, 1}, {0, 1} }
};
const gs_range4 Range4_default = {
    { {0, 1}, {0, 1}, {0, 1}, {0, 1} }
};
const gs_cie_defg_proc4 DecodeDEFG_default = {
    {defg_identity, defg_identity, defg_identity, defg_identity}
};
const gs_cie_defg_proc4 DecodeDEFG_from_cache = {
    {defg_from_cache_0, defg_from_cache_1, defg_from_cache_2, defg_from_cache_3}
};
const gs_cie_def_proc3 DecodeDEF_default = {
    {def_identity, def_identity, def_identity}
};
const gs_cie_def_proc3 DecodeDEF_from_cache = {
    {def_from_cache_0, def_from_cache_1, def_from_cache_2}
};
const gs_cie_abc_proc3 DecodeABC_default = {
    {abc_identity, abc_identity, abc_identity}
};
const gs_cie_abc_proc3 DecodeABC_from_cache = {
    {abc_from_cache_0, abc_from_cache_1, abc_from_cache_2}
};
const gs_cie_common_proc3 DecodeLMN_default = {
    {common_identity, common_identity, common_identity}
};
const gs_cie_common_proc3 DecodeLMN_from_cache = {
    {lmn_from_cache_0, lmn_from_cache_1, lmn_from_cache_2}
};
const gs_matrix3 Matrix3_default = {
    {1, 0, 0},
    {0, 1, 0},
    {0, 0, 1},
    1 /*true */
};
const gs_range RangeA_default = {0, 1};
const gs_cie_a_proc DecodeA_default = a_identity;
const gs_cie_a_proc DecodeA_from_cache = a_from_cache;
const gs_vector3 MatrixA_default = {1, 1, 1};
const gs_vector3 BlackPoint_default = {0, 0, 0};

/* Initialize a CIE color. */
/* This only happens on setcolorspace. */
void
gx_init_CIE(gs_client_color * pcc, const gs_color_space * pcs)
{
    gx_init_paint_4(pcc, pcs);
    /* (0...) may not be within the range of allowable values. */
    (*pcs->type->restrict_color)(pcc, pcs);
}

/* Restrict CIE colors. */

inline private void
cie_restrict(float *pv, const gs_range *range)
{
    if (*pv <= range->rmin)
	*pv = range->rmin;
    else if (*pv >= range->rmax)
	*pv = range->rmax;
}

void
gx_restrict_CIEDEFG(gs_client_color * pcc, const gs_color_space * pcs)
{
    const gs_cie_defg *pcie = pcs->params.defg;

    cie_restrict(&pcc->paint.values[0], &pcie->RangeDEFG.ranges[0]);
    cie_restrict(&pcc->paint.values[1], &pcie->RangeDEFG.ranges[1]);
    cie_restrict(&pcc->paint.values[2], &pcie->RangeDEFG.ranges[2]);
    cie_restrict(&pcc->paint.values[3], &pcie->RangeDEFG.ranges[3]);
}
void
gx_restrict_CIEDEF(gs_client_color * pcc, const gs_color_space * pcs)
{
    const gs_cie_def *pcie = pcs->params.def;

    cie_restrict(&pcc->paint.values[0], &pcie->RangeDEF.ranges[0]);
    cie_restrict(&pcc->paint.values[1], &pcie->RangeDEF.ranges[1]);
    cie_restrict(&pcc->paint.values[2], &pcie->RangeDEF.ranges[2]);
}
void
gx_restrict_CIEABC(gs_client_color * pcc, const gs_color_space * pcs)
{
    const gs_cie_abc *pcie = pcs->params.abc;

    cie_restrict(&pcc->paint.values[0], &pcie->RangeABC.ranges[0]);
    cie_restrict(&pcc->paint.values[1], &pcie->RangeABC.ranges[1]);
    cie_restrict(&pcc->paint.values[2], &pcie->RangeABC.ranges[2]);
}
void
gx_restrict_CIEA(gs_client_color * pcc, const gs_color_space * pcs)
{
    const gs_cie_a *pcie = pcs->params.a;

    cie_restrict(&pcc->paint.values[0], &pcie->RangeA);
}

/* ================ Table setup ================ */

/* ------ Install a CIE color space ------ */

private void cie_load_common_cache(P2(gs_cie_common *, gs_state *));
private void cie_cache_mult(P3(gx_cie_vector_cache *, const gs_vector3 *,
			       const cie_cache_floats *));
private bool cie_cache_mult3(P2(gx_cie_vector_cache *,
				const gs_matrix3 *));

private int
gx_install_cie_abc(gs_cie_abc *pcie, gs_state * pgs)
{
    if_debug_matrix3("[c]CIE MatrixABC =", &pcie->MatrixABC);
    cie_matrix_init(&pcie->MatrixABC);
    CIE_LOAD_CACHE_BODY(pcie->caches.DecodeABC, pcie->RangeABC.ranges,
			&pcie->DecodeABC, DecodeABC_default, pcie,
			"DecodeABC");
    cie_load_common_cache(&pcie->common, pgs);
    gs_cie_abc_complete(pcie);
    return gs_cie_cs_complete(pgs, true);
}

int
gx_install_CIEDEFG(const gs_color_space * pcs, gs_state * pgs)
{
    gs_cie_defg *pcie = pcs->params.defg;

    CIE_LOAD_CACHE_BODY(pcie->caches_defg.DecodeDEFG, pcie->RangeDEFG.ranges,
			&pcie->DecodeDEFG, DecodeDEFG_default, pcie,
			"DecodeDEFG");
    return gx_install_cie_abc((gs_cie_abc *)pcie, pgs);
}

int
gx_install_CIEDEF(const gs_color_space * pcs, gs_state * pgs)
{
    gs_cie_def *pcie = pcs->params.def;

    CIE_LOAD_CACHE_BODY(pcie->caches_def.DecodeDEF, pcie->RangeDEF.ranges,
			&pcie->DecodeDEF, DecodeDEF_default, pcie,
			"DecodeDEF");
    return gx_install_cie_abc((gs_cie_abc *)pcie, pgs);
}

int
gx_install_CIEABC(const gs_color_space * pcs, gs_state * pgs)
{
    return gx_install_cie_abc(pcs->params.abc, pgs);
}

int
gx_install_CIEA(const gs_color_space * pcs, gs_state * pgs)
{
    gs_cie_a *pcie = pcs->params.a;
    int i;
    gs_for_loop_params lp;
    float in;

    gs_cie_cache_init(&pcie->caches.DecodeA.floats.params, &lp,
		      &pcie->RangeA, "DecodeA");
    for (i = 0, in = lp.init; i < gx_cie_cache_size; in += lp.step, i++) {
	pcie->caches.DecodeA.floats.values[i] = (*pcie->DecodeA)(in, pcie);
	if_debug3('C', "[C]DecodeA[%d] = %g => %g\n",
		  i, in, pcie->caches.DecodeA.floats.values[i]);
    }
    cie_load_common_cache(&pcie->common, pgs);
    gs_cie_a_complete(pcie);
    return gs_cie_cs_complete(pgs, true);
}

/* Load the common caches when installing the color space. */
private void
cie_load_common_cache(gs_cie_common * pcie, gs_state * pgs)
{
    if_debug_matrix3("[c]CIE MatrixLMN =", &pcie->MatrixLMN);
    cie_matrix_init(&pcie->MatrixLMN);
    CIE_LOAD_CACHE_BODY(pcie->caches.DecodeLMN, pcie->RangeLMN.ranges,
			&pcie->DecodeLMN, DecodeLMN_default, pcie,
			"DecodeLMN");
}

/* Complete loading the common caches. */
private void
cie_common_complete(gs_cie_common *pcie)
{
    int i;

    for (i = 0; i < 3; ++i)
	cache_set_linear(&pcie->caches.DecodeLMN[i].floats);
}

/* Restrict and scale the DecodeDEF[G] cache according to RangeHIJ[K]. */
private void
gs_cie_defx_scale(float *values, const gs_range *range)
{
    double scale = 255.0 / (range->rmax - range->rmin);
    int i;

    for (i = 0; i < gx_cie_cache_size; ++i) {
	float value = values[i];

	values[i] =
	    (value <= range->rmin ? 0 :
	     value >= range->rmax ? 255 :
	     (value - range->rmin) * scale);
    }
}

/* Complete loading a CIEBasedDEFG color space. */
/* This routine is NOT idempotent. */
void
gs_cie_defg_complete(gs_cie_defg * pcie)
{
    int j;

    for (j = 0; j < 4; ++j)
	gs_cie_defx_scale(pcie->caches_defg.DecodeDEFG[j].floats.values,
			  &pcie->RangeHIJK.ranges[j]);
    gs_cie_abc_complete((gs_cie_abc *)pcie);
}

/* Complete loading a CIEBasedDEF color space. */
/* This routine is NOT idempotent. */
void
gs_cie_def_complete(gs_cie_def * pcie)
{
    int j;

    for (j = 0; j < 3; ++j)
	gs_cie_defx_scale(pcie->caches_def.DecodeDEF[j].floats.values,
			  &pcie->RangeHIJ.ranges[j]);
    gs_cie_abc_complete((gs_cie_abc *)pcie);
}

/* Complete loading a CIEBasedABC color space. */
/* This routine is idempotent. */
void
gs_cie_abc_complete(gs_cie_abc * pcie)
{
    cache3_set_linear(pcie->caches.DecodeABC);
    pcie->caches.skipABC =
	cie_cache_mult3(pcie->caches.DecodeABC, &pcie->MatrixABC);
    cie_common_complete((gs_cie_common *)pcie);
}

/* Complete loading a CIEBasedA color space. */
/* This routine is idempotent. */
void
gs_cie_a_complete(gs_cie_a * pcie)
{
    cie_cache_mult(&pcie->caches.DecodeA, &pcie->MatrixA,
		   &pcie->caches.DecodeA.floats);
    cache_set_linear(&pcie->caches.DecodeA.floats);
    cie_common_complete((gs_cie_common *)pcie);
}

/* Convert a scalar cache to a vector cache by multiplying */
/* the scalar values by a vector. */
/* This procedure is idempotent. */
private void
cie_cache_mult(gx_cie_vector_cache * pcache, const gs_vector3 * pvec,
	       const cie_cache_floats * pcf)
{
    int i;

    pcache->vecs.params.base = float2cie_cached(pcf->params.base);
    pcache->vecs.params.factor = float2cie_cached(pcf->params.factor);
    pcache->vecs.params.limit =
	float2cie_cached((gx_cie_cache_size - 1) / pcf->params.factor +
			 pcf->params.base);
    for (i = 0; i < gx_cie_cache_size; ++i) {
	float f = pcf->values[i];

	pcache->vecs.values[i].u = float2cie_cached(f * pvec->u);
	pcache->vecs.values[i].v = float2cie_cached(f * pvec->v);
	pcache->vecs.values[i].w = float2cie_cached(f * pvec->w);
    }
}

/* Convert 3 scalar caches to vector caches by multiplying by a matrix. */
/* Return true iff the resulting cache is an identity transformation. */
private bool
cie_cache_mult3(gx_cie_vector_cache * pc /*[3] */ , const gs_matrix3 * pmat)
{
    cie_cache_mult(pc, &pmat->cu, &pc->floats);
    cie_cache_mult(pc + 1, &pmat->cv, &pc[1].floats);
    cie_cache_mult(pc + 2, &pmat->cw, &pc[2].floats);
    return pmat->is_identity & pc[0].floats.params.is_identity &
	pc[1].floats.params.is_identity & pc[2].floats.params.is_identity;
}

/* ------ Install a rendering dictionary ------ */

/* setcolorrendering */
int
gs_setcolorrendering(gs_state * pgs, gs_cie_render * pcrd)
{
    int code = gs_cie_render_complete(pcrd);
    const gs_cie_render *pcrd_old = pgs->cie_render;
    bool joint_ok;

    if (code < 0)
	return code;
    if (pcrd_old != 0 && pcrd->id == pcrd_old->id)
	return 0;		/* detect needless reselecting */
    joint_ok =
	pcrd_old != 0 &&
#define CRD_SAME(elt) !memcmp(&pcrd->elt, &pcrd_old->elt, sizeof(pcrd->elt))
	CRD_SAME(points.WhitePoint) && CRD_SAME(points.BlackPoint) &&
	CRD_SAME(MatrixPQR) && CRD_SAME(RangePQR) &&
	CRD_SAME(TransformPQR);
#undef CRD_SAME
    rc_assign(pgs->cie_render, pcrd, "gs_setcolorrendering");
    /* Initialize the joint caches if needed. */
    if (!joint_ok)
	code = gs_cie_cs_complete(pgs, true);
    gx_unset_dev_color(pgs);
    return code;
}

/* currentcolorrendering */
const gs_cie_render *
gs_currentcolorrendering(const gs_state * pgs)
{
    return pgs->cie_render;
}

/* Unshare (allocating if necessary) the joint caches. */
gx_cie_joint_caches *
gx_currentciecaches(gs_state * pgs)
{
    gx_cie_joint_caches *pjc = pgs->cie_joint_caches;

    rc_unshare_struct(pgs->cie_joint_caches, gx_cie_joint_caches,
		      &st_joint_caches, pgs->memory,
		      return 0, "gx_currentciecaches");
    if (pgs->cie_joint_caches != pjc) {
	pjc = pgs->cie_joint_caches;
	pjc->cspace_id = pjc->render_id = gs_no_id;
	pjc->id_status = pjc->status = CIE_JC_STATUS_BUILT;
    }
    return pjc;
}

/* Compute the parameters for loading a cache, setting base and factor. */
/* This procedure is idempotent. */
void
gs_cie_cache_init(cie_cache_params * pcache, gs_for_loop_params * pflp,
		  const gs_range * domain, client_name_t cname)
{
	/* We need to map the values in the range
	 * [domain->rmin..domain->rmax].  However, if rmin < 0 < rmax and
	 * the function is non-linear, this can lead to anomalies at zero,
	 * which is the default value for CIE colors.  The "correct" way to
	 * approach this is to run the mapping functions on demand, but we
	 * don't want to deal with the complexities of the callbacks this
	 * would involve (especially in the middle of rendering images);
	 * instead, we adjust the range so that zero maps precisely to a
	 * cache slot.  Define:
	 *      a = domain->rmin;
	 *      b = domain->rmax;
	 *      R = b - a;
	 *      N = gx_cie_cache_size - 1;
	 *      f(v) = N(v-a)/R;
	 *      x = f(0).
	 * If x is not an integer, we can either increase b or
	 * decrease a to make it one.  In the former case, compute:
	 *      Kb = floor(x); R'b = N(0-a)/Kb; b' = a + R'b.
	 * In the latter case, compute:
	 *      Ka = ceiling(x-N); R'a = N(0-b)/Ka; a' = b - R'a.
	 * We choose whichever method stretches the range the least,
	 * i.e., the one whose R' value (R'a or R'b) is smaller.
	 */
    double a = domain->rmin, b = domain->rmax;
    double R = b - a;
    double delta;
#define N (gx_cie_cache_size - 1)

    /* Adjust the range if necessary. */
    if (a < 0 && b >= 0) {
	double x = -N * a / R;	/* must be > 0 */
	double Kb = floor(x);	/* must be >= 0 */
	double Ka = ceil(x) - N;	/* must be <= 0 */

	if (Kb == 0 || (Ka != 0 && -b / Ka < -a / Kb))	/* use R'a */
	    R = -N * b / Ka, a = b - R;
	else			/* use R'b */
	    R = -N * a / Kb, b = a + R;
    }
    delta = R / N;
#ifdef CIE_CACHE_INTERPOLATE
    pcache->base = a;		/* no rounding */
#else
    pcache->base = a - delta / 2;	/* so lookup will round */
#endif
    pcache->factor = (delta == 0 ? 0 : N / R);
    if_debug4('c', "[c]cache %s 0x%lx base=%g, factor=%g\n",
	      (const char *)cname, (ulong) pcache,
	      pcache->base, pcache->factor);
    pflp->init = a;
    pflp->step = delta;
    pflp->limit = b + delta / 2;
#undef N
}

/* ------ Complete a rendering structure ------ */

/*
 * Compute the derived values in a CRD that don't involve the cached
 * procedure values.  This procedure is idempotent.
 */
private void cie_transform_range3(P3(const gs_range3 *, const gs_matrix3 *,
				     gs_range3 *));
int
gs_cie_render_init(gs_cie_render * pcrd)
{
    gs_matrix3 PQR_inverse;

    if (pcrd->status >= CIE_RENDER_STATUS_INITED)
	return 0;		/* init already done */
    if_debug_matrix3("[c]CRD MatrixLMN =", &pcrd->MatrixLMN);
    cie_matrix_init(&pcrd->MatrixLMN);
    if_debug_matrix3("[c]CRD MatrixABC =", &pcrd->MatrixABC);
    cie_matrix_init(&pcrd->MatrixABC);
    if_debug_matrix3("[c]CRD MatrixPQR =", &pcrd->MatrixPQR);
    cie_matrix_init(&pcrd->MatrixPQR);
    cie_invert3(&pcrd->MatrixPQR, &PQR_inverse);
    cie_matrix_mult3(&pcrd->MatrixLMN, &PQR_inverse,
		     &pcrd->MatrixPQR_inverse_LMN);
    cie_transform_range3(&pcrd->RangePQR, &pcrd->MatrixPQR_inverse_LMN,
			 &pcrd->DomainLMN);
    cie_transform_range3(&pcrd->RangeLMN, &pcrd->MatrixABC,
			 &pcrd->DomainABC);
    cie_mult3(&pcrd->points.WhitePoint, &pcrd->MatrixPQR, &pcrd->wdpqr);
    cie_mult3(&pcrd->points.BlackPoint, &pcrd->MatrixPQR, &pcrd->bdpqr);
    pcrd->status = CIE_RENDER_STATUS_INITED;
    return 0;
}

/*
 * Sample the EncodeLMN, EncodeABC, and RenderTableT CRD procedures, and
 * load the caches.  This procedure is idempotent.
 */
int
gs_cie_render_sample(gs_cie_render * pcrd)
{
    int code;

    if (pcrd->status >= CIE_RENDER_STATUS_SAMPLED)
	return 0;		/* sampling already done */
    code = gs_cie_render_init(pcrd);
    if (code < 0)
	return code;
    CIE_LOAD_CACHE_BODY(pcrd->caches.EncodeLMN, pcrd->DomainLMN.ranges,
			&pcrd->EncodeLMN, Encode_default, pcrd, "EncodeLMN");
    cache3_set_linear(pcrd->caches.EncodeLMN);
    CIE_LOAD_CACHE_BODY(pcrd->caches.EncodeABC, pcrd->DomainABC.ranges,
			&pcrd->EncodeABC, Encode_default, pcrd, "EncodeABC");
    if (pcrd->RenderTable.lookup.table != 0) {
	int i, j, m = pcrd->RenderTable.lookup.m;
	gs_for_loop_params flp;
	bool is_identity = true;

	for (j = 0; j < m; j++) {
	    gs_cie_cache_init(&pcrd->caches.RenderTableT[j].fracs.params,
			      &flp, &Range3_default.ranges[0],
			      "RenderTableT");
	    is_identity &= pcrd->RenderTable.T.procs[j] ==
		RenderTableT_default.procs[j];
	}
	pcrd->caches.RenderTableT_is_identity = is_identity;
	/*
	 * Unfortunately, we defined the first argument of the RenderTable
	 * T procedures as being a byte, limiting the number of distinct
	 * cache entries to 256 rather than gx_cie_cache_size.
	 * We confine this decision to this loop, rather than propagating
	 * it to the procedures that use the cached data, so that we can
	 * change it more easily at some future time.
	 */
	for (i = 0; i < gx_cie_cache_size; i++) {
#if gx_cie_log2_cache_size >= 8
	    byte value = i >> (gx_cie_log2_cache_size - 8);
#else
	    byte value = (i << (8 - gx_cie_log2_cache_size)) +
		(i >> (gx_cie_log2_cache_size * 2 - 8));
#endif
	    for (j = 0; j < m; j++) {
		pcrd->caches.RenderTableT[j].fracs.values[i] =
		    (*pcrd->RenderTable.T.procs[j])(value, pcrd);
		if_debug3('C', "[C]RenderTableT[%d,%d] = %g\n",
			  i, j,
			  frac2float(pcrd->caches.RenderTableT[j].fracs.values[i]));
	    }
	}
    }
    pcrd->status = CIE_RENDER_STATUS_SAMPLED;
    return 0;
}

/* Transform a set of ranges. */
private void
cie_transform_range(const gs_range3 * in, floatp mu, floatp mv, floatp mw,
		    gs_range * out)
{
    float umin = mu * in->ranges[0].rmin, umax = mu * in->ranges[0].rmax;
    float vmin = mv * in->ranges[1].rmin, vmax = mv * in->ranges[1].rmax;
    float wmin = mw * in->ranges[2].rmin, wmax = mw * in->ranges[2].rmax;
    float temp;

    if (umin > umax)
	temp = umin, umin = umax, umax = temp;
    if (vmin > vmax)
	temp = vmin, vmin = vmax, vmax = temp;
    if (wmin > wmax)
	temp = wmin, wmin = wmax, wmax = temp;
    out->rmin = umin + vmin + wmin;
    out->rmax = umax + vmax + wmax;
}
private void
cie_transform_range3(const gs_range3 * in, const gs_matrix3 * mat,
		     gs_range3 * out)
{
    cie_transform_range(in, mat->cu.u, mat->cv.u, mat->cw.u,
			&out->ranges[0]);
    cie_transform_range(in, mat->cu.v, mat->cv.v, mat->cw.v,
			&out->ranges[1]);
    cie_transform_range(in, mat->cu.w, mat->cv.w, mat->cw.w,
			&out->ranges[2]);
}

/*
 * Finish preparing a CRD for installation, by restricting and/or
 * transforming the cached procedure values.
 * This procedure is idempotent.
 */
int
gs_cie_render_complete(gs_cie_render * pcrd)
{
    int code;

    if (pcrd->status >= CIE_RENDER_STATUS_COMPLETED)
	return 0;		/* completion already done */
    code = gs_cie_render_sample(pcrd);
    if (code < 0)
	return code;
    /*
     * Since range restriction happens immediately after
     * the cache lookup, we can save a step by restricting
     * the values in the cache entries.
     *
     * If there is no lookup table, we want the final ABC values
     * to be fracs; if there is a table, we want them to be
     * appropriately scaled ints.
     */
    pcrd->MatrixABCEncode = pcrd->MatrixABC;
    {
	int c;
	double f;

	for (c = 0; c < 3; c++) {
	    gx_cie_float_fixed_cache *pcache = &pcrd->caches.EncodeABC[c];

	    cie_cache_restrict(&pcrd->caches.EncodeLMN[c].floats,
			       &pcrd->RangeLMN.ranges[c]);
	    cie_cache_restrict(&pcrd->caches.EncodeABC[c].floats,
			       &pcrd->RangeABC.ranges[c]);
	    if (pcrd->RenderTable.lookup.table == 0) {
		cie_cache_restrict(&pcache->floats,
				   &Range3_default.ranges[0]);
		gs_cie_cache_to_fracs(&pcache->floats, &pcache->fixeds.fracs);
		pcache->fixeds.fracs.params.is_identity = false;
	    } else {
		int i;
		int n = pcrd->RenderTable.lookup.dims[c];

#ifdef CIE_RENDER_TABLE_INTERPOLATE
#  define SCALED_INDEX(f, n, itemp)\
     RESTRICTED_INDEX(f * (1 << _cie_interpolate_bits),\
		      (n) << _cie_interpolate_bits, itemp)
#else
		int m = pcrd->RenderTable.lookup.m;
		int k =
		    (c == 0 ? 1 : c == 1 ?
		     m * pcrd->RenderTable.lookup.dims[2] : m);
#  define SCALED_INDEX(f, n, itemp)\
     (RESTRICTED_INDEX(f, n, itemp) * k)
#endif
		const gs_range *prange = pcrd->RangeABC.ranges + c;
		double scale = (n - 1) / (prange->rmax - prange->rmin);

		for (i = 0; i < gx_cie_cache_size; ++i) {
		    float v =
			(pcache->floats.values[i] - prange->rmin) * scale
#ifndef CIE_RENDER_TABLE_INTERPOLATE
			+ 0.5
#endif
			;
		    int itemp;

		    if_debug5('c',
			      "[c]cache[%d][%d] = %g => %g => %d\n",
			      c, i, pcache->floats.values[i], v,
			      SCALED_INDEX(v, n, itemp));
		    pcache->fixeds.ints.values[i] =
			SCALED_INDEX(v, n, itemp);
		}
		pcache->fixeds.ints.params = pcache->floats.params;
		pcache->fixeds.ints.params.is_identity = false;
#undef SCALED_INDEX
	    }
	}
	/* Fold the scaling of the EncodeABC cache index */
	/* into MatrixABC. */
#define MABC(i, t)\
  f = pcrd->caches.EncodeABC[i].floats.params.factor;\
  pcrd->MatrixABCEncode.cu.t *= f;\
  pcrd->MatrixABCEncode.cv.t *= f;\
  pcrd->MatrixABCEncode.cw.t *= f;\
  pcrd->EncodeABC_base[i] =\
    float2cie_cached(pcrd->caches.EncodeABC[i].floats.params.base * f)
	MABC(0, u);
	MABC(1, v);
	MABC(2, w);
#undef MABC
	pcrd->MatrixABCEncode.is_identity = 0;
    }
    cie_cache_mult3(pcrd->caches.EncodeLMN, &pcrd->MatrixABCEncode);
    pcrd->status = CIE_RENDER_STATUS_COMPLETED;
    return 0;
}

/* Apply a range restriction to a cache. */
private void
cie_cache_restrict(cie_cache_floats * pcache, const gs_range * prange)
{
    int i;

    for (i = 0; i < gx_cie_cache_size; i++) {
	float v = pcache->values[i];

	if (v < prange->rmin)
	    pcache->values[i] = prange->rmin;
	else if (v > prange->rmax)
	    pcache->values[i] = prange->rmax;
    }
}

/* Convert a cache from floats to fracs. */
/* Note that the two may be aliased. */
void
gs_cie_cache_to_fracs(const cie_cache_floats *pfloats, cie_cache_fracs *pfracs)
{
    int i;

    /* Loop from bottom to top so that we don't */
    /* overwrite elements before they're used. */
    for (i = 0; i < gx_cie_cache_size; ++i)
	pfracs->values[i] = float2frac(pfloats->values[i]);
    pfracs->params = pfloats->params;
}

/* ------ Fill in the joint cache ------ */

/* If the current color space is a CIE space, or has a CIE base space, */
/* return a pointer to the common part of the space; otherwise return 0. */
private const gs_cie_common *
cie_cs_common_abc(const gs_color_space *pcs_orig, const gs_cie_abc **ppabc)
{
    const gs_color_space *pcs = pcs_orig;

    *ppabc = 0;
    do {
        switch (pcs->type->index) {
	case gs_color_space_index_CIEDEF:
	    *ppabc = (const gs_cie_abc *)pcs->params.def;
	    return &pcs->params.def->common;
	case gs_color_space_index_CIEDEFG:
	    *ppabc = (const gs_cie_abc *)pcs->params.defg;
	    return &pcs->params.defg->common;
	case gs_color_space_index_CIEABC:
	    *ppabc = pcs->params.abc;
	    return &pcs->params.abc->common;
	case gs_color_space_index_CIEA:
	    return &pcs->params.a->common;
	default:
            pcs = gs_cspace_base_space(pcs);
            break;
        }
    } while (pcs != 0);

    return 0;
}
const gs_cie_common *
gs_cie_cs_common(const gs_state * pgs)
{
    const gs_cie_abc *ignore_pabc;

    return cie_cs_common_abc(pgs->color_space, &ignore_pabc);
}

/*
 * Mark the joint caches as needing completion.  This is done lazily,
 * when a color is being mapped.  However, make sure the joint caches
 * exist now.
 */
int
gs_cie_cs_complete(gs_state * pgs, bool init)
{
    gx_cie_joint_caches *pjc = gx_currentciecaches(pgs);

    if (pjc == 0)
	return_error(gs_error_VMerror);
    pjc->status = (init ? CIE_JC_STATUS_BUILT : CIE_JC_STATUS_INITED);
    return 0;
}
/* Actually complete the joint caches. */
int
gs_cie_jc_complete(const gs_imager_state *pis, const gs_color_space *pcs)
{
    const gs_cie_abc *pabc;
    const gs_cie_common *common = cie_cs_common_abc(pcs, &pabc);
    gs_cie_render *pcrd = pis->cie_render;
    gx_cie_joint_caches *pjc = pis->cie_joint_caches;

    if (pjc->cspace_id == pcs->id &&
	pjc->render_id == pcrd->id)
	pjc->status = pjc->id_status;
    switch (pjc->status) {
    case CIE_JC_STATUS_BUILT: {
	int code = cie_joint_caches_init(pjc, common, pcrd);

	if (code < 0)
	    return code;
    }
	/* falls through */
    case CIE_JC_STATUS_INITED:
	cie_joint_caches_complete(pjc, common, pabc, pcrd);
	pjc->cspace_id = pcs->id;
	pjc->render_id = pcrd->id;
	pjc->id_status = pjc->status = CIE_JC_STATUS_COMPLETED;
	/* falls through */
    case CIE_JC_STATUS_COMPLETED:
	break;
    }
    return 0;
}

/*
 * Compute the source and destination WhitePoint and BlackPoint for
 * the TransformPQR procedure.
 */
int 
gs_cie_compute_points_sd(gx_cie_joint_caches *pjc,
			 const gs_cie_common * pcie,
			 const gs_cie_render * pcrd)
{
    gs_cie_wbsd *pwbsd = &pjc->points_sd;

    pwbsd->ws.xyz = pcie->points.WhitePoint;
    cie_mult3(&pwbsd->ws.xyz, &pcrd->MatrixPQR, &pwbsd->ws.pqr);
    pwbsd->bs.xyz = pcie->points.BlackPoint;
    cie_mult3(&pwbsd->bs.xyz, &pcrd->MatrixPQR, &pwbsd->bs.pqr);
    pwbsd->wd.xyz = pcrd->points.WhitePoint;
    pwbsd->wd.pqr = pcrd->wdpqr;
    pwbsd->bd.xyz = pcrd->points.BlackPoint;
    pwbsd->bd.pqr = pcrd->bdpqr;
    return 0;
}

/*
 * Sample the TransformPQR procedure for the joint caches.
 * This routine is idempotent.
 */
private int
cie_joint_caches_init(gx_cie_joint_caches * pjc,
		      const gs_cie_common * pcie,
		      gs_cie_render * pcrd)
{
    bool is_identity;
    int j;

    gs_cie_compute_points_sd(pjc, pcie, pcrd);
    /*
     * If a client pre-loaded the cache, we can't adjust the range.
     * ****** WRONG ******
     */
    if (pcrd->TransformPQR.proc == TransformPQR_from_cache.proc)
	return 0;
    is_identity = pcrd->TransformPQR.proc == TransformPQR_default.proc;
    for (j = 0; j < 3; j++) {
	int i;
	gs_for_loop_params lp;

	gs_cie_cache_init(&pjc->TransformPQR[j].floats.params, &lp,
			  &pcrd->RangePQR.ranges[j], "TransformPQR");
	for (i = 0; i < gx_cie_cache_size; lp.init += lp.step, i++) {
	    float out;
	    int code =
		(*pcrd->TransformPQR.proc)(j, lp.init, &pjc->points_sd,
					   pcrd, &out);

	    if (code < 0)
		return code;
	    pjc->TransformPQR[j].floats.values[i] = out;
	    if_debug4('C', "[C]TransformPQR[%d,%d] = %g => %g\n",
		      j, i, lp.init, out);
	}
	pjc->TransformPQR[j].floats.params.is_identity = is_identity;
    }
    return 0;
}

/*
 * Complete the loading of the joint caches.
 * This routine is idempotent.
 */
private void
cie_joint_caches_complete(gx_cie_joint_caches * pjc,
			  const gs_cie_common * pcie,
			  const gs_cie_abc * pabc /* NULL if CIEA */,
			  const gs_cie_render * pcrd)
{
    gs_matrix3 mat3, mat2, mat1;
    gs_matrix3 MatrixLMN_PQR;
    int j;

    /*
     * We number the pipeline steps as follows:
     *   1 - DecodeABC/MatrixABC
     *   2 - DecodeLMN/MatrixLMN/MatrixPQR
     *   3 - TransformPQR/MatrixPQR'/MatrixLMN
     *   4 - EncodeLMN/MatrixABC
     *   5 - EncodeABC, RenderTable (we don't do anything with this here)
     * We work from back to front, combining steps where possible.
     * Currently we only combine steps if a procedure is the identity
     * transform, but we could do it whenever the procedure is linear.
     * A project for another day....
     */
    /* Step 4 */
    if (pcrd->caches.EncodeLMN[0].floats.params.is_identity &&
	pcrd->caches.EncodeLMN[1].floats.params.is_identity &&
	pcrd->caches.EncodeLMN[2].floats.params.is_identity) {
	/* Fold step 4 into step 3. */
	cie_matrix_mult3(&pcrd->MatrixABCEncode, &pcrd->MatrixPQR_inverse_LMN,
			 &mat3);
	pjc->skipEncodeLMN = true;
    } else {
	mat3 = pcrd->MatrixPQR_inverse_LMN;
	pjc->skipEncodeLMN = false;
    }
    /* Step 3 */
    cache3_set_linear(pjc->TransformPQR);
    cie_matrix_mult3(&pcrd->MatrixPQR, &pcie->MatrixLMN,
		     &MatrixLMN_PQR);
    if (pjc->TransformPQR[0].floats.params.is_identity &
	pjc->TransformPQR[1].floats.params.is_identity &
	pjc->TransformPQR[2].floats.params.is_identity) {
	/* Fold step 3 into step 2. */
	cie_matrix_mult3(&mat3, &MatrixLMN_PQR, &mat2);
	pjc->skipPQR = true;
    } else {
	mat2 = MatrixLMN_PQR;
	for (j = 0; j < 3; j++) {
	    cie_cache_restrict(&pjc->TransformPQR[j].floats,
			       &pcrd->RangePQR.ranges[j]);
	}
	cie_cache_mult3(pjc->TransformPQR, &mat3);
	pjc->skipPQR = false;
    }
    /* Steps 2 & 1 */
    if (pcie->caches.DecodeLMN[0].floats.params.is_identity &
	pcie->caches.DecodeLMN[1].floats.params.is_identity &
	pcie->caches.DecodeLMN[2].floats.params.is_identity) {
	if (!pabc) {
	    pjc->skipDecodeLMN = mat2.is_identity;
	    pjc->skipDecodeABC = false;
	    if (!pjc->skipDecodeLMN)
		for (j = 0; j < 3; j++) {
		    cie_cache_mult(&pjc->DecodeLMN[j], &mat2.cu + j,
				   &pcie->caches.DecodeLMN[j].floats);
		}
	} else {
	    /*
	     * Fold step 2 into step 1.  This is a little different because
	     * the data for step 1 are in the color space structure.
	     */
	    cie_matrix_mult3(&mat2, &pabc->MatrixABC, &mat1);
	    for (j = 0; j < 3; j++) {
		cie_cache_mult(&pjc->DecodeLMN[j], &mat1.cu + j,
			       &pabc->caches.DecodeABC[j].floats);
	    }
	    pjc->skipDecodeLMN = false;
	    pjc->skipDecodeABC = true;
	}
    } else {
	for (j = 0; j < 3; j++) {
	    cie_cache_mult(&pjc->DecodeLMN[j], &mat2.cu + j,
			   &pcie->caches.DecodeLMN[j].floats);
	}
	pjc->skipDecodeLMN = false;
	pjc->skipDecodeABC = pabc != 0 && pabc->caches.skipABC;
    }
}

/* ================ Utilities ================ */

/* Multiply a vector by a matrix. */
/* Note that we are computing M * V where v is a column vector. */
private void
cie_mult3(const gs_vector3 * in, register const gs_matrix3 * mat,
	  gs_vector3 * out)
{
    if_debug_vector3("[c]mult", in);
    if_debug_matrix3("	*", mat);
    {
	float u = in->u, v = in->v, w = in->w;

	out->u = (u * mat->cu.u) + (v * mat->cv.u) + (w * mat->cw.u);
	out->v = (u * mat->cu.v) + (v * mat->cv.v) + (w * mat->cw.v);
	out->w = (u * mat->cu.w) + (v * mat->cv.w) + (w * mat->cw.w);
    }
    if_debug_vector3("	=", out);
}

/*
 * Multiply two matrices.  Note that the composition of the transformations
 * M1 followed by M2 is M2 * M1, not M1 * M2.  (See gscie.h for details.)
 */
private void
cie_matrix_mult3(const gs_matrix3 *ma, const gs_matrix3 *mb, gs_matrix3 *mc)
{
    gs_matrix3 mprod;
    gs_matrix3 *mp = (mc == ma || mc == mb ? &mprod : mc);

    if_debug_matrix3("[c]matrix_mult", ma);
    if_debug_matrix3("             *", mb);
    cie_mult3(&mb->cu, ma, &mp->cu);
    cie_mult3(&mb->cv, ma, &mp->cv);
    cie_mult3(&mb->cw, ma, &mp->cw);
    cie_matrix_init(mp);
    if_debug_matrix3("             =", mp);
    if (mp != mc)
	*mc = *mp;
}

/* Invert a matrix. */
/* The output must not be an alias for the input. */
private void
cie_invert3(const gs_matrix3 *in, gs_matrix3 *out)
{	/* This is a brute force algorithm; maybe there are better. */
    /* We label the array elements */
    /*   [ A B C ]   */
    /*   [ D E F ]   */
    /*   [ G H I ]   */
#define A cu.u
#define B cv.u
#define C cw.u
#define D cu.v
#define E cv.v
#define F cw.v
#define G cu.w
#define H cv.w
#define I cw.w
    double coA = in->E * in->I - in->F * in->H;
    double coB = in->F * in->G - in->D * in->I;
    double coC = in->D * in->H - in->E * in->G;
    double det = in->A * coA + in->B * coB + in->C * coC;

    if_debug_matrix3("[c]invert", in);
    out->A = coA / det;
    out->D = coB / det;
    out->G = coC / det;
    out->B = (in->C * in->H - in->B * in->I) / det;
    out->E = (in->A * in->I - in->C * in->G) / det;
    out->H = (in->B * in->G - in->A * in->H) / det;
    out->C = (in->B * in->F - in->C * in->E) / det;
    out->F = (in->C * in->D - in->A * in->F) / det;
    out->I = (in->A * in->E - in->B * in->D) / det;
    if_debug_matrix3("        =", out);
#undef A
#undef B
#undef C
#undef D
#undef E
#undef F
#undef G
#undef H
#undef I
    out->is_identity = in->is_identity;
}

/* Set the is_identity flag that accelerates multiplication. */
private void
cie_matrix_init(register gs_matrix3 * mat)
{
    mat->is_identity =
	mat->cu.u == 1.0 && is_fzero2(mat->cu.v, mat->cu.w) &&
	mat->cv.v == 1.0 && is_fzero2(mat->cv.u, mat->cv.w) &&
	mat->cw.w == 1.0 && is_fzero2(mat->cw.u, mat->cw.v);
}

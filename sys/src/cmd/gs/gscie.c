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

/* gscie.c */
/* CIE color rendering for Ghostscript */
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gscspace.h"
#include "gsmatrix.h"			/* for gscolor2.h */
#include "gscolor2.h"			/* for gs_set/currentcolorrendering */
#include "gscie.h"
#include "gxarith.h"
#include "gzstate.h"

/* Define the CIE color space types. */
/* We use CIEA[BC] rather than CIEBasedA[BC] in some places because */
/* gcc under VMS only retains 23 characters of procedure names. */
private cs_proc_concrete_space(gx_concrete_space_CIEBased);
cs_declare_procs(private, gx_concretize_CIEABC, gx_install_CIEABC,
  gx_adjust_cspace_CIEABC,
  gx_enum_ptrs_CIEABC, gx_reloc_ptrs_CIEABC);
cs_declare_procs(private, gx_concretize_CIEA, gx_install_CIEA,
  gx_adjust_cspace_CIEA,
  gx_enum_ptrs_CIEA, gx_reloc_ptrs_CIEA);
const gs_color_space_type
	gs_color_space_type_CIEBasedABC =
	 { gs_color_space_index_CIEBasedABC, 3, true,
	   gx_init_paint_3, gx_concrete_space_CIEBased,
	   gx_concretize_CIEABC, NULL,
	   gx_default_remap_color, gx_install_CIEABC,
	   gx_adjust_cspace_CIEABC, gx_no_adjust_color_count,
	   gx_enum_ptrs_CIEABC, gx_reloc_ptrs_CIEABC
	 },
	gs_color_space_type_CIEBasedA =
	 { gs_color_space_index_CIEBasedA, 1, true,
	   gx_init_paint_1, gx_concrete_space_CIEBased,
	   gx_concretize_CIEA, NULL,
	   gx_default_remap_color, gx_install_CIEA,
	   gx_adjust_cspace_CIEA, gx_no_adjust_color_count,
	   gx_enum_ptrs_CIEA, gx_reloc_ptrs_CIEA
	 };

/* Forward references */
private void near cie_mult3(P3(const gs_vector3 *, const gs_matrix3 *, gs_vector3 *));
private void near cie_matrix_mult3(P3(const gs_matrix3 *, const gs_matrix3 *, gs_matrix3 *));
private void near cie_invert3(P2(const gs_matrix3 *, gs_matrix3 *));
private void near cie_restrict3(P3(const gs_vector3 *, const gs_range3 *, gs_vector3 *));
private void near cie_lookup3(P3(const gs_vector3 *, const gx_cie_cache *, gs_vector3 *));
private void near cie_matrix_init(P1(gs_matrix3 *));

#define lookup(vin, pcache, vout)\
  if ( (pcache)->is_identity ) vout = (vin);\
  else vout = (pcache)->values[(int)(((vin) - (pcache)->base) * (pcache)->factor)]

#define restrict(vin, range, vout)\
  if ( (vin) < (range).rmin ) vout = (range).rmin;\
  else if ( (vin) > (range).rmax ) vout = (range).rmax;\
  else vout = (vin)

/* Allocator structure types */
private_st_joint_caches();
private_st_const_string();
public_st_const_string_element();
#define sptr ((gs_const_string *)vptr)
private ENUM_PTRS_BEGIN(const_string_enum_ptrs) return 0;
	case 0: *pep = (void *)sptr; return ptr_const_string_type;
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(const_string_reloc_ptrs) {
	gs_reloc_const_string(sptr, gcst);
} RELOC_PTRS_END
#undef sptr

/* ------ Default values for CIE dictionary elements ------ */

/* Default transformation procedures. */

private int
a_identity(const float *in, const gs_cie_a *pcie, float *out)
{	*out = *in;
	return 0;
}
private int
abc_identity(const gs_vector3 *in, const gs_cie_abc *pcie, gs_vector3 *out)
{	*out = *in;
	return 0;
}
private int
common_identity(const gs_vector3 *in, const gs_cie_common *pcie, gs_vector3 *out)
{	*out = *in;
	return 0;
}
private int
render_identity(const gs_vector3 *in, const gs_cie_render *pcie, gs_vector3 *out)
{	*out = *in;
	return 0;
}
private int
tpqr_identity(const gs_vector3 *in, const gs_cie_wbsd *pwbsd, const gs_cie_render *pcie, gs_vector3 *out)
{	*out = *in;
	return 0;
}
private int
render_table_identity(const byte *in, int m, const gs_cie_render *pcie, float *out)
{	int j;
	for ( j = 0; j < m; j++ ) out[j] = in[j] / 255.0;
	return 0;
}

/* Default vectors and matrices. */

const gs_range3 Range3_default = { {0,1}, {0,1}, {0,1} };
const gs_cie_abc_proc3 DecodeABC_default = abc_identity;
const gs_cie_common_proc3 DecodeLMN_default = common_identity;
const gs_matrix3 Matrix3_default = { {1,0,0}, {0,1,0}, {0,0,1}, 1 };
const gs_range RangeA_default = {0,1};
const gs_cie_a_proc DecodeA_default = a_identity;
const gs_vector3 MatrixA_default = { 1, 1, 1 };
const gs_vector3 BlackPoint_default = { 0, 0, 0 };
const gs_cie_render_proc3 Encode_default = render_identity;
const gs_cie_transform_proc3 TransformPQR_default = tpqr_identity;
const gs_cie_render_table_proc RenderTableT_default = render_table_identity;

/* setcolorrendering */
int
gs_setcolorrendering(gs_state *pgs, gs_cie_render *pcie)
{	int code = gs_cie_render_init(pcie);
	if ( code < 0 )
	  return code;
	rc_assign(pgs->cie_render, pcie, pgs->memory,
		  "gs_setcolorrendering");
	/* Initialize the joint caches, if needed, */
	/* by re-installing the color space. */
	(*pgs->color_space->type->install_cspace)(pgs->color_space, pgs);
	gx_unset_dev_color(pgs);
	return 0;
}

/* currentcolorrendering */
const gs_cie_render *
gs_currentcolorrendering(const gs_state *pgs)
{	return pgs->cie_render;
}

/* Get the joint caches, to avoid having to import gzstate.h */
gx_cie_joint_caches *
gx_currentciecaches(gs_state *pgs)
{	return pgs->cie_joint_caches;
}

/* ------ Complete a rendering structure ------ */

int
gs_cie_render_init(gs_cie_render *pcie)
{	gs_matrix3 PQR_inverse;
	cie_matrix_init(&pcie->MatrixLMN);
	cie_matrix_init(&pcie->MatrixABC);
	cie_matrix_init(&pcie->MatrixPQR);
	cie_invert3(&pcie->MatrixPQR, &PQR_inverse);
	cie_matrix_mult3(&PQR_inverse, &pcie->MatrixLMN, &pcie->MatrixPQR_inverse_LMN);
	cie_mult3(&pcie->points.WhitePoint, &pcie->MatrixPQR, &pcie->wdpqr);
	cie_mult3(&pcie->points.BlackPoint, &pcie->MatrixPQR, &pcie->bdpqr);
	/****** FINISH ******/
	return 0;
}

/* ------ Fill in the joint cache ------ */

int
gx_cie_joint_caches_init(gx_cie_joint_caches *pjc,
  const gs_cie_common *pcie, const gs_cie_render *pcier)
{	pjc->points_sd.ws.xyz = pcie->points.WhitePoint;
	cie_mult3(&pjc->points_sd.ws.xyz, &pcier->MatrixPQR, &pjc->points_sd.ws.pqr);
	pjc->points_sd.bs.xyz = pcie->points.BlackPoint;
	cie_mult3(&pjc->points_sd.bs.xyz, &pcier->MatrixPQR, &pjc->points_sd.bs.pqr);
	pjc->points_sd.wd.xyz = pcier->points.WhitePoint;
	pjc->points_sd.wd.pqr = pcier->wdpqr;
	pjc->points_sd.bd.xyz = pcier->points.BlackPoint;
	pjc->points_sd.bd.pqr = pcier->bdpqr;
	/****** FINISH ******/
	return 0;
}

/* ------ Render a CIE color to a concrete color (using the caches). ------ */

private int near cie_remap_finish(P4(const gs_vector3 *,
  const gs_cie_common *, frac *, const gs_state *));

/* Determine the concrete space underlying a CIEBased space. */
private const gs_color_space cie_rgb_space =
	{ &gs_color_space_type_DeviceRGB };
private const gs_color_space cie_cmyk_space =
	{ &gs_color_space_type_DeviceCMYK };
private const gs_color_space *
gx_concrete_space_CIEBased(const gs_color_space *pcs, const gs_state *pgs)
{	const gs_cie_render *pcie = pgs->cie_render;
	if ( pcie == 0 )
		return &cie_rgb_space;
	if ( pcie->RenderTable.table == 0 || pcie->RenderTable.m == 3 )
		return &cie_rgb_space;
	else				/* pcie->RenderTable.m == 4 */
		return &cie_cmyk_space;
}

/* Render a CIEBasedABC color. */
private int
gx_concretize_CIEABC(const gs_client_color *pc, const gs_color_space *pcs,
  frac *pconc, const gs_state *pgs)
{	const gs_cie_abc *pcie = pcs->params.abc;
	gs_vector3 abc, lmn;
	cie_restrict3((const gs_vector3 *)&pc->paint.values[0], &pcie->RangeABC, &abc);
		/* (*pcie->DecodeABC)(&abc, pcie, &abc); */
	cie_lookup3(&abc, &pcie->caches.DecodeABC[0], &abc);
	cie_mult3(&abc, &pcie->MatrixABC, &lmn);
	return cie_remap_finish(&lmn, &pcie->common, pconc, pgs);
}

/* Render a CIEBasedA color. */
private int
gx_concretize_CIEA(const gs_client_color *pc, const gs_color_space *pcs,
  frac *pconc, const gs_state *pgs)
{	const gs_cie_a *pcie = pcs->params.a;
	const gx_cie_cache *pcache = &pcie->caches.DecodeA;
	float a;
	gs_vector3 lmn;
	restrict(pc->paint.values[0], pcie->RangeA, a);
		/* (*pcie->DecodeA)(&a, pcie, &a); */
	lookup(a, pcache, a);
	lmn.u = a * pcie->MatrixA.u;
	lmn.v = a * pcie->MatrixA.v;
	lmn.w = a * pcie->MatrixA.w;
	return cie_remap_finish(&lmn, &pcie->common, pconc, pgs);
}

/* Common rendering code. */
private int near
cie_remap_finish(const gs_vector3 *plmn, const gs_cie_common *pcommon,
  frac *pconc, const gs_state *pgs)
{	const gs_cie_render *pcie = pgs->cie_render;
	gs_const_string *table;
	gs_vector3 abc, lmn, xyz, pqr;
	gs_client_color cc;
	gs_color_space cs;

		/* Finish decoding. */

	cie_restrict3(plmn, &pcommon->RangeLMN, &lmn);
		/* (*pcommon->DecodeLMN)(&lmn, pcommon, &lmn); */
	cie_lookup3(&lmn, &pcommon->caches.DecodeLMN[0], &lmn);
	cie_mult3(&lmn, &pcommon->MatrixLMN, &xyz);

		/* Render. */

	if ( pcie == 0 )		/* default rendering */
	{	abc = xyz;
		table = 0;
	}
	else
	{	const gx_cie_joint_caches *pjc = pgs->cie_joint_caches;
		cie_mult3(&xyz, &pcie->MatrixPQR, &pqr);
		cie_restrict3(&pqr, &pcie->RangePQR, &pqr);
			/* (*pcie->TransformPQR)(&pqr, &pjc->points_sd, pcie, &pqr); */
		cie_lookup3(&pqr, &pjc->TransformPQR[0], &pqr);
		cie_mult3(&pqr, &pcie->MatrixPQR_inverse_LMN, &lmn);
			/* (*pcie->EncodeLMN)(&lmn, pcie, &lmn); */
		cie_lookup3(&lmn, &pcie->caches.EncodeLMN[0], &lmn);
		cie_restrict3(&lmn, &pcie->RangeLMN, &lmn);
		cie_mult3(&lmn, &pcie->MatrixABC, &abc);
			/* (*pcie->EncodeABC)(&abc, pcie, &abc); */
		cie_lookup3(&abc, &pcie->caches.EncodeABC[0], &abc);
		cie_restrict3(&abc, &pcie->RangeABC, &abc);
		table = pcie->RenderTable.table;
	}
	if ( table == 0 )
	{	/* No further transformation */
		cc.paint.values[0] = abc.u;
		cc.paint.values[1] = abc.v;
		cc.paint.values[2] = abc.w;
		cs.type = &gs_color_space_type_DeviceRGB;
	}
	else
	{	/* Use the RenderTable. */
		int m = pcie->RenderTable.m;
#define ri(s,n)\
  (int)((abc.s - pcie->RangeABC.s.rmin) * (pcie->RenderTable.n - 1) /\
	(pcie->RangeABC.s.rmax - pcie->RangeABC.s.rmin) + 0.5)
		int ia = ri(u, NA);
		int ib = ri(v, NB);
		int ic = ri(w, NC);
		const byte *prtc =
		  table[ia].data + m * (ib * pcie->RenderTable.NC + ic);
			/* (*pcie->RenderTable.T)(prtc, m, pcie, &cc.paint.values[0]); */
#define shift_in(b) gx_cie_byte_to_cache_index(b)
#define rtc(i) (pcie->caches.RenderTableT[i])
		cc.paint.values[0] = rtc(0).values[shift_in(prtc[0])];
		cc.paint.values[1] = rtc(1).values[shift_in(prtc[1])];
		cc.paint.values[2] = rtc(2).values[shift_in(prtc[2])];
		if ( m == 3 )
		{	cs.type = &gs_color_space_type_DeviceRGB;
		}
		else
		{	cc.paint.values[3] = rtc(3).values[shift_in(prtc[3])];
			cs.type = &gs_color_space_type_DeviceCMYK;
		}
#undef rtc
#undef shift_in
	}
	return (*cs.type->concretize_color)(&cc, &cs, pconc, pgs);
}

/* ------ Adjust reference counts for a CIE color space ------ */

private void
gx_adjust_cspace_CIEABC(const gs_color_space *pcs, gs_state *pgs, int delta)
{	rc_adjust_const(pcs->params.abc, delta, pgs->memory,
			"gx_adjust_cspace_CIEABC");
}

private void
gx_adjust_cspace_CIEA(const gs_color_space *pcs, gs_state *pgs, int delta)
{	rc_adjust_const(pcs->params.a, delta, pgs->memory,
			"gx_adjust_cspace_CIEA");
}

/* ------ Install a CIE color space ------ */
/* These routines should load the cache, but they don't. */

private int
gx_install_CIEABC(gs_color_space *pcs, gs_state *pgs)
{	gs_cie_abc *pcie = pcs->params.abc;
	cie_matrix_init(&pcie->common.MatrixLMN);
	cie_matrix_init(&pcie->MatrixABC);
	if ( pgs->cie_render == 0 )
		return 0;
	rc_unshare_struct(pgs->cie_joint_caches, gx_cie_joint_caches,
			  &st_joint_caches, pgs->memory,
			  return_error(gs_error_VMerror),
			  "gx_install_CIEABC");
	return gx_cie_joint_caches_init(pgs->cie_joint_caches,
		&pcie->common, pgs->cie_render);
}

private int
gx_install_CIEA(gs_color_space *pcs, gs_state *pgs)
{	gs_cie_a *pcie = pcs->params.a;
	cie_matrix_init(&pcie->common.MatrixLMN);
	if ( pgs->cie_render == 0 )
		return 0;
	rc_unshare_struct(pgs->cie_joint_caches, gx_cie_joint_caches,
			  &st_joint_caches, pgs->memory,
			  return_error(gs_error_VMerror),
			  "gx_install_CIEA");
	return gx_cie_joint_caches_init(pgs->cie_joint_caches,
		&pcie->common, pgs->cie_render);
}

/* GC procedures for CIE color spaces */

#define pcs ((gs_color_space *)vptr)

private ENUM_PTRS_BEGIN(gx_enum_ptrs_CIEABC) return 0;
	ENUM_PTR(0, gs_color_space, params.abc);
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(gx_reloc_ptrs_CIEABC) {
	RELOC_PTR(gs_color_space, params.abc);
} RELOC_PTRS_END

private ENUM_PTRS_BEGIN(gx_enum_ptrs_CIEA) return 0;
	ENUM_PTR(0, gs_color_space, params.a);
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(gx_reloc_ptrs_CIEA) {
	RELOC_PTR(gs_color_space, params.a);
} RELOC_PTRS_END

#undef pcs

/* ------ Utilities ------ */

#define if_debug_vector3(str, vec)\
  if_debug4('c', "%s[%g %g %g]\n", str, vec->u, vec->v, vec->w)
#define if_debug_matrix3(str, mat)\
  if_debug10('c', "%s[%g %g %g / %g %g %g / %g %g %g]\n", str,\
    mat->cu.u, mat->cu.v, mat->cu.w, mat->cv.u, mat->cv.v, mat->cv.w,\
    mat->cw.u, mat->cw.v, mat->cw.w)

/* Multiply a vector by a matrix. */
/* Optimizing this routine is the justification for is_identity! */
private void near
cie_mult3(const gs_vector3 *in, register const gs_matrix3 *mat, gs_vector3 *out)
{	if_debug_vector3("[c]mult", in);
	if_debug_matrix3("	*", mat);
	if ( mat->is_identity )
		*out = *in;
	else
	{	float u = in->u, v = in->v, w = in->w;
		out->u = (u * mat->cu.u) + (v * mat->cv.u) + (w * mat->cw.u);
		out->v = (u * mat->cu.v) + (v * mat->cv.v) + (w * mat->cw.v);
		out->w = (u * mat->cu.w) + (v * mat->cv.w) + (w * mat->cw.w);
	}
	if_debug_vector3("	=", out);
}

/* Multiply two matrices.  We assume the result is not an alias for */
/* either of the operands. */
private void near
cie_matrix_mult3(const gs_matrix3 *ma, const gs_matrix3 *mb, gs_matrix3 *mc)
{	gs_vector3 row_in, row_out;
	if_debug_matrix3("[c]matrix_mult", ma);
	if_debug_matrix3("             *", mb);
#define mult_row(e)\
  row_in.u = ma->cu.e, row_in.v = ma->cv.e, row_in.w = ma->cw.e;\
  cie_mult3(&row_in, mb, &row_out);\
  mc->cu.e = row_out.u, mc->cv.e = row_out.v, mc->cw.e = row_out.w
	mult_row(u);
	mult_row(v);
	mult_row(w);
#undef mult_row
	cie_matrix_init(mc);
	if_debug_matrix3("             =", mc);
}

/* Invert a matrix. */
/* The output must not be an alias for the input. */
private void near
cie_invert3(register const gs_matrix3 *in, register gs_matrix3 *out)
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

/* Force values within bounds. */
private void near
cie_restrict3(const gs_vector3 *in, const gs_range3 *range, gs_vector3 *out)
{	float temp;
	temp = in->u; restrict(temp, range->u, out->u);
	temp = in->v; restrict(temp, range->v, out->v);
	temp = in->w; restrict(temp, range->w, out->w);
}

private void near
cie_lookup3(const gs_vector3 *in, const gx_cie_cache *pc /*[3]*/, gs_vector3 *out)
{	if_debug4('c', "[c]lookup 0x%lx [%g %g %g]\n", (ulong)pc,
		  in->u, in->v, in->w);
	lookup(in->u, pc, out->u); pc++;
	lookup(in->v, pc, out->v); pc++;
	lookup(in->w, pc, out->w);
	if_debug_vector3("        =", out);
}

/* Set the is_identity flag that accelerates multiplication. */
private void near
cie_matrix_init(register gs_matrix3 *mat)
{	mat->is_identity =
		mat->cu.u == 1.0 && is_fzero2(mat->cu.v, mat->cu.w) &&
		mat->cv.v == 1.0 && is_fzero2(mat->cv.u, mat->cv.w) &&
		mat->cw.w == 1.0 && is_fzero2(mat->cw.u, mat->cw.v);
}

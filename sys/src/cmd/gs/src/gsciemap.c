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

/*$Id: gsciemap.c,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* CIE color rendering */
#include "math_.h"
#include "gx.h"
#include "gserrors.h"
#include "gxcspace.h"		/* for gxcie.c */
#include "gxarith.h"
#include "gxcie.h"
#include "gxdevice.h"		/* for gxcmap.h */
#include "gxcmap.h"
#include "gxistate.h"

/* Compute a cache index as (vin - base) * factor. */
/* vin, base, factor, and the result are cie_cached_values. */
/* We know that the result doesn't exceed (gx_cie_cache_size - 1) << fbits. */
#define LOOKUP_INDEX(vin, pcache, fbits)\
  ((vin) <= (pcache)->vecs.params.base ? 0 :\
   (vin) >= (pcache)->vecs.params.limit ? (gx_cie_cache_size - 1) << (fbits) :\
   cie_cached_product2int( ((vin) - (pcache)->vecs.params.base),\
			   (pcache)->vecs.params.factor, fbits ))
#define LOOKUP_VALUE(vin, pcache)\
  ((pcache)->vecs.values[LOOKUP_INDEX(vin, pcache, 0)])

/*
 * Test whether a CIE rendering has been defined; ensure that the joint
 * caches are loaded.  Note that the procedure may return if no rendering
 * has been defined, and returns if an error occurs.
 */
#define CIE_CHECK_RENDERING(pcs, pconc, pis, do_exit)\
  BEGIN\
    if (pis->cie_render == 0) {\
	/* No rendering has been defined yet: return black. */\
	pconc[0] = pconc[1] = pconc[2] = frac_0;\
	do_exit;\
    }\
    if (pis->cie_joint_caches->status != CIE_JC_STATUS_COMPLETED) {\
	int code = gs_cie_jc_complete(pis, pcs);\
\
	if (code < 0)\
	    return code;\
    }\
  END

/* Forward references */
private int cie_remap_finish(P4(cie_cached_vector3,
				frac *, const gs_imager_state *,
				const gs_color_space *));
private void cie_lookup_mult3(P2(cie_cached_vector3 *,
				 const gx_cie_vector_cache *));

#ifdef DEBUG
private void
cie_lookup_map3(cie_cached_vector3 * pvec,
		const gx_cie_vector_cache * pc /*[3] */ , const char *cname)
{
    if_debug5('c', "[c]lookup %s 0x%lx [%g %g %g]\n",
	      (const char *)cname, (ulong) pc,
	      cie_cached2float(pvec->u), cie_cached2float(pvec->v),
	      cie_cached2float(pvec->w));
    cie_lookup_mult3(pvec, pc);
    if_debug3('c', "        =[%g %g %g]\n",
	      cie_cached2float(pvec->u), cie_cached2float(pvec->v),
	      cie_cached2float(pvec->w));
}
#else
#  define cie_lookup_map3(pvec, pc, cname) cie_lookup_mult3(pvec, pc)
#endif

/* Render a CIEBasedDEFG color. */
int
gx_concretize_CIEDEFG(const gs_client_color * pc, const gs_color_space * pcs,
		      frac * pconc, const gs_imager_state * pis)
{
    const gs_cie_defg *pcie = pcs->params.defg;
    int i;
    fixed hijk[4];
    frac abc[3];
    cie_cached_vector3 vec3;

    if_debug4('c', "[c]concretize DEFG [%g %g %g %g]\n",
	      pc->paint.values[0], pc->paint.values[1],
	      pc->paint.values[2], pc->paint.values[3]);
    CIE_CHECK_RENDERING(pcs, pconc, pis, return 0);

    /* Apply DecodeDEFG (including restriction to RangeHIJK). */
    for (i = 0; i < 4; ++i) {
	int tmax = pcie->Table.dims[i] - 1;
	float value = (pc->paint.values[i] - pcie->RangeDEFG.ranges[i].rmin) *
	    tmax /
	    (pcie->RangeDEFG.ranges[i].rmax - pcie->RangeDEFG.ranges[i].rmin);
	int vi = (int)value;
	float vf = value - vi;
	float v = pcie->caches_defg.DecodeDEFG[i].floats.values[vi];

	if (vf != 0 && vi < tmax)
	    v += vf *
		(pcie->caches_defg.DecodeDEFG[i].floats.values[vi + 1] - v);
	hijk[i] = float2fixed(v);
    }
    /* Apply Table. */
    gx_color_interpolate_linear(hijk, &pcie->Table, abc);
    vec3.u = float2cie_cached(frac2float(abc[0]));
    vec3.v = float2cie_cached(frac2float(abc[1]));
    vec3.w = float2cie_cached(frac2float(abc[2]));
    /* Apply DecodeABC and MatrixABC. */
    if (!pis->cie_joint_caches->skipDecodeABC)
	cie_lookup_map3(&vec3 /* ABC => LMN */, &pcie->caches.DecodeABC[0],
			"Decode/MatrixABC");
    cie_remap_finish(vec3, pconc, pis, pcs);
    return 0;
}

/* Render a CIEBasedDEF color. */
int
gx_concretize_CIEDEF(const gs_client_color * pc, const gs_color_space * pcs,
		     frac * pconc, const gs_imager_state * pis)
{
    const gs_cie_def *pcie = pcs->params.def;
    int i;
    fixed hij[3];
    frac abc[3];
    cie_cached_vector3 vec3;

    if_debug3('c', "[c]concretize DEF [%g %g %g]\n",
	      pc->paint.values[0], pc->paint.values[1],
	      pc->paint.values[2]);
    CIE_CHECK_RENDERING(pcs, pconc, pis, return 0);

    /* Apply DecodeDEF (including restriction to RangeHIJ). */
    for (i = 0; i < 3; ++i) {
	int tmax = pcie->Table.dims[i] - 1;
	float value = (pc->paint.values[i] - pcie->RangeDEF.ranges[i].rmin) *
	    tmax /
	    (pcie->RangeDEF.ranges[i].rmax - pcie->RangeDEF.ranges[i].rmin);
	int vi = (int)value;
	float vf = value - vi;
	float v = pcie->caches_def.DecodeDEF[i].floats.values[vi];

	if (vf != 0 && vi < tmax)
	    v += vf *
		(pcie->caches_def.DecodeDEF[i].floats.values[vi + 1] - v);
	hij[i] = float2fixed(v);
    }
    /* Apply Table. */
    gx_color_interpolate_linear(hij, &pcie->Table, abc);
    vec3.u = float2cie_cached(frac2float(abc[0]));
    vec3.v = float2cie_cached(frac2float(abc[1]));
    vec3.w = float2cie_cached(frac2float(abc[2]));
    /* Apply DecodeABC and MatrixABC. */
    if (!pis->cie_joint_caches->skipDecodeABC)
	cie_lookup_map3(&vec3 /* ABC => LMN */, &pcie->caches.DecodeABC[0],
			"Decode/MatrixABC");
    cie_remap_finish(vec3, pconc, pis, pcs);
    return 0;
}

/* Render a CIEBasedABC color. */
/* We provide both remap and concretize, but only the former */
/* needs to be efficient. */
int
gx_remap_CIEABC(const gs_client_color * pc, const gs_color_space * pcs,
	gx_device_color * pdc, const gs_imager_state * pis, gx_device * dev,
		gs_color_select_t select)
{
    frac conc[4];
    cie_cached_vector3 vec3;

    if_debug3('c', "[c]remap CIEABC [%g %g %g]\n",
	      pc->paint.values[0], pc->paint.values[1],
	      pc->paint.values[2]);
    CIE_CHECK_RENDERING(pcs, conc, pis, goto map3);
    vec3.u = float2cie_cached(pc->paint.values[0]);
    vec3.v = float2cie_cached(pc->paint.values[1]);
    vec3.w = float2cie_cached(pc->paint.values[2]);

    /* Apply DecodeABC and MatrixABC. */
    if (!pis->cie_joint_caches->skipDecodeABC) {
	const gs_cie_abc *pcie = pcs->params.abc;

	cie_lookup_map3(&vec3 /* ABC => LMN */, &pcie->caches.DecodeABC[0],
			"Decode/MatrixABC");
    }
    switch (cie_remap_finish(vec3 /* LMN */, conc, pis, pcs)) {
	case 4:
	    if_debug4('c', "[c]=CMYK [%g %g %g %g]\n",
		      frac2float(conc[0]), frac2float(conc[1]),
		      frac2float(conc[2]), frac2float(conc[3]));
	    gx_remap_concrete_cmyk(conc[0], conc[1], conc[2], conc[3],
				   pdc, pis, dev, select);
	    return 0;
	default:	/* Can't happen. */
	    return_error(gs_error_unknownerror);
	case 3:
	    ;
    }
map3:
    if_debug3('c', "[c]=RGB [%g %g %g]\n",
	      frac2float(conc[0]), frac2float(conc[1]),
	      frac2float(conc[2]));
    gx_remap_concrete_rgb(conc[0], conc[1], conc[2], pdc, pis,
			  dev, select);
    return 0;
}
int
gx_concretize_CIEABC(const gs_client_color * pc, const gs_color_space * pcs,
		     frac * pconc, const gs_imager_state * pis)
{
    const gs_cie_abc *pcie = pcs->params.abc;
    cie_cached_vector3 vec3;

    if_debug3('c', "[c]concretize CIEABC [%g %g %g]\n",
	      pc->paint.values[0], pc->paint.values[1],
	      pc->paint.values[2]);
    CIE_CHECK_RENDERING(pcs, pconc, pis, return 0);

    vec3.u = float2cie_cached(pc->paint.values[0]);
    vec3.v = float2cie_cached(pc->paint.values[1]);
    vec3.w = float2cie_cached(pc->paint.values[2]);
    if (!pis->cie_joint_caches->skipDecodeABC)
	cie_lookup_map3(&vec3 /* ABC => LMN */, &pcie->caches.DecodeABC[0],
			"Decode/MatrixABC");
    cie_remap_finish(vec3, pconc, pis, pcs);
    return 0;
}

/* Render a CIEBasedA color. */
int
gx_concretize_CIEA(const gs_client_color * pc, const gs_color_space * pcs,
		   frac * pconc, const gs_imager_state * pis)
{
    const gs_cie_a *pcie = pcs->params.a;
    cie_cached_value a = float2cie_cached(pc->paint.values[0]);
    cie_cached_vector3 vlmn;

    if_debug1('c', "[c]concretize CIEA %g\n", pc->paint.values[0]);
    CIE_CHECK_RENDERING(pcs, pconc, pis, return 0);

    /* Apply DecodeA and MatrixA. */
    if (!pis->cie_joint_caches->skipDecodeABC)
	vlmn = LOOKUP_VALUE(a, &pcie->caches.DecodeA);
    else
	vlmn.u = vlmn.v = vlmn.w = a;
    cie_remap_finish(vlmn, pconc, pis, pcs);
    return 0;
}

/* Common rendering code. */
/* Return 3 if RGB, 4 if CMYK. */
private int
cie_remap_finish(cie_cached_vector3 vec3, frac * pconc,
		 const gs_imager_state * pis, const gs_color_space *pcs)
{
    const gs_cie_render *pcrd = pis->cie_render;
    const gx_cie_joint_caches *pjc = pis->cie_joint_caches;
    const gs_const_string *table = pcrd->RenderTable.lookup.table;
    int tabc[3];		/* indices for final EncodeABC lookup */

    /* Apply DecodeLMN, MatrixLMN(decode), and MatrixPQR. */
    if (!pjc->skipDecodeLMN)
	cie_lookup_map3(&vec3 /* LMN => PQR */, &pjc->DecodeLMN[0],
			"Decode/MatrixLMN+MatrixPQR");

    /* Apply TransformPQR, MatrixPQR', and MatrixLMN(encode). */
    if (!pjc->skipPQR)
	cie_lookup_map3(&vec3 /* PQR => LMN */, &pjc->TransformPQR[0],
			"Transform/Matrix'PQR+MatrixLMN");

    /* Apply EncodeLMN and MatrixABC(encode). */
    if (!pjc->skipEncodeLMN)
	cie_lookup_map3(&vec3 /* LMN => ABC */, &pcrd->caches.EncodeLMN[0],
			"EncodeLMN+MatrixABC");

    /* MatrixABCEncode includes the scaling of the EncodeABC */
    /* cache index. */
#define SET_TABC(i, t)\
  BEGIN\
    tabc[i] = cie_cached2int(vec3 /*ABC*/.t - pcrd->EncodeABC_base[i],\
			     _cie_interpolate_bits);\
    if ((uint)tabc[i] > (gx_cie_cache_size - 1) << _cie_interpolate_bits)\
	tabc[i] = (tabc[i] < 0 ? 0 :\
		   (gx_cie_cache_size - 1) << _cie_interpolate_bits);\
  END
    SET_TABC(0, u);
    SET_TABC(1, v);
    SET_TABC(2, w);
#undef SET_TABC
    if (table == 0) {
	/*
	 * No further transformation.
	 * The final mapping step includes both restriction to
	 * the range [0..1] and conversion to fracs.
	 */
#define EABC(i)\
  cie_interpolate_fracs(pcrd->caches.EncodeABC[i].fixeds.fracs.values, tabc[i])
	pconc[0] = EABC(0);
	pconc[1] = EABC(1);
	pconc[2] = EABC(2);
#undef EABC
	return 3;
    } else {
	/*
	 * Use the RenderTable.
	 */
	int m = pcrd->RenderTable.lookup.m;

#define RT_LOOKUP(j, i) pcrd->caches.RenderTableT[j].fracs.values[i]
#ifdef CIE_RENDER_TABLE_INTERPOLATE

	/*
	 * The final mapping step includes restriction to the
	 * ranges [0..dims[c]] as ints with interpolation bits.
	 */
	fixed rfix[3];

#define EABC(i)\
  cie_interpolate_fracs(pcrd->caches.EncodeABC[i].fixeds.ints.values, tabc[i])
#define FABC(i)\
  (EABC(i) << (_fixed_shift - _cie_interpolate_bits))
	rfix[0] = FABC(0);
	rfix[1] = FABC(1);
	rfix[2] = FABC(2);
#undef FABC
#undef EABC
	if_debug6('c', "[c]ABC=%g,%g,%g => iabc=%g,%g,%g\n",
		  cie_cached2float(vec3.u), cie_cached2float(vec3.v),
		  cie_cached2float(vec3.w), fixed2float(rfix[0]),
		  fixed2float(rfix[1]), fixed2float(rfix[2]));
	gx_color_interpolate_linear(rfix, &pcrd->RenderTable.lookup,
				    pconc);
	if_debug3('c', "[c]  interpolated => %g,%g,%g\n",
		  frac2float(pconc[0]), frac2float(pconc[1]),
		  frac2float(pconc[2]));
	if (!pcrd->caches.RenderTableT_is_identity) {
	    /* Map the interpolated values. */
#define frac2cache_index(v) frac2bits(v, gx_cie_log2_cache_size)
	    pconc[0] = RT_LOOKUP(0, frac2cache_index(pconc[0]));
	    pconc[1] = RT_LOOKUP(1, frac2cache_index(pconc[1]));
	    pconc[2] = RT_LOOKUP(2, frac2cache_index(pconc[2]));
	    if (m > 3)
		pconc[3] = RT_LOOKUP(3, frac2cache_index(pconc[3]));
#undef frac2cache_index
	}

#else /* !CIE_RENDER_TABLE_INTERPOLATE */

	/*
	 * The final mapping step includes restriction to the ranges
	 * [0..dims[c]], plus scaling of the indices in the strings.
	 */
#define RI(i)\
  pcrd->caches.EncodeABC[i].ints.values[tabc[i] >> _cie_interpolate_bits]
	int ia = RI(0);
	int ib = RI(1);		/* pre-multiplied by m * NC */
	int ic = RI(2);		/* pre-multiplied by m */
	const byte *prtc = table[ia].data + ib + ic;

	/* (*pcrd->RenderTable.T)(prtc, m, pcrd, pconc); */

	if_debug6('c', "[c]ABC=%g,%g,%g => iabc=%d,%d,%d\n",
		  cie_cached2float(vec3.u), cie_cached2float(vec3.v),
		  cie_cached2float(vec3.w), ia, ib, ic);
	if (pcrd->caches.RenderTableT_is_identity) {
	    pconc[0] = byte2frac(prtc[0]);
	    pconc[1] = byte2frac(prtc[1]);
	    pconc[2] = byte2frac(prtc[2]);
	    if (m > 3)
		pconc[3] = byte2frac(prtc[3]);
	} else {
#if gx_cie_log2_cache_size == 8
#  define byte2cache_index(b) (b)
#else
# if gx_cie_log2_cache_size > 8
#  define byte2cache_index(b)\
    ( ((b) << (gx_cie_log2_cache_size - 8)) +\
      ((b) >> (16 - gx_cie_log2_cache_size)) )
# else				/* < 8 */
#  define byte2cache_index(b) ((b) >> (8 - gx_cie_log2_cache_size))
# endif
#endif
	    pconc[0] = RT_LOOKUP(0, byte2cache_index(prtc[0]));
	    pconc[1] = RT_LOOKUP(1, byte2cache_index(prtc[1]));
	    pconc[2] = RT_LOOKUP(2, byte2cache_index(prtc[2]));
	    if (m > 3)
		pconc[3] = RT_LOOKUP(3, byte2cache_index(prtc[3]));
#undef byte2cache_index
	}

#endif /* !CIE_RENDER_TABLE_INTERPOLATE */
#undef RI
#undef RT_LOOKUP
	return m;
    }
}

/* Look up 3 values in a cache, with cached post-multiplication. */
private void
cie_lookup_mult3(cie_cached_vector3 * pvec,
		 const gx_cie_vector_cache * pc /*[3] */ )
{
/****** Interpolating at intermediate stages doesn't seem to ******/
/****** make things better, and slows things down, so....    ******/
#ifdef CIE_INTERPOLATE_INTERMEDIATE
    /* Interpolate between adjacent cache entries. */
    /* This is expensive! */
#ifdef CIE_CACHE_USE_FIXED
#  define lookup_interpolate_between(v0, v1, i, ftemp)\
     cie_interpolate_between(v0, v1, i)
#else
    float ftu, ftv, ftw;

#  define lookup_interpolate_between(v0, v1, i, ftemp)\
     ((v0) + ((v1) - (v0)) *\
      ((ftemp = float_rshift(i, _cie_interpolate_bits)), ftemp - (int)ftemp))
#endif

    cie_cached_value iu =
	 LOOKUP_INDEX(pvec->u, pc, _cie_interpolate_bits);
    const cie_cached_vector3 *pu =
	&pc[0].vecs.values[(int)cie_cached_rshift(iu,
						  _cie_interpolate_bits)];
    const cie_cached_vector3 *pu1 =
	(iu >= (gx_cie_cache_size - 1) << _cie_interpolate_bits ?
	 pu : pu + 1);

    cie_cached_value iv =
	LOOKUP_INDEX(pvec->v, pc + 1, _cie_interpolate_bits);
    const cie_cached_vector3 *pv =
	&pc[1].vecs.values[(int)cie_cached_rshift(iv,
						  _cie_interpolate_bits)];
    const cie_cached_vector3 *pv1 =
	(iv >= (gx_cie_cache_size - 1) << _cie_interpolate_bits ?
	 pv : pv + 1);

    cie_cached_value iw =
	LOOKUP_INDEX(pvec->w, pc + 2, _cie_interpolate_bits);
    const cie_cached_vector3 *pw =
	&pc[2].vecs.values[(int)cie_cached_rshift(iw,
						  _cie_interpolate_bits)];
    const cie_cached_vector3 *pw1 =
	(iw >= (gx_cie_cache_size - 1) << _cie_interpolate_bits ?
	 pw : pw + 1);

    pvec->u = lookup_interpolate_between(pu->u, pu1->u, iu, ftu) +
	lookup_interpolate_between(pv->u, pv1->u, iv, ftv) +
	lookup_interpolate_between(pw->u, pw1->u, iw, ftw);
    pvec->v = lookup_interpolate_between(pu->v, pu1->v, iu, ftu) +
	lookup_interpolate_between(pv->v, pv1->v, iv, ftv) +
	lookup_interpolate_between(pw->v, pw1->v, iw, ftw);
    pvec->w = lookup_interpolate_between(pu->w, pu1->w, iu, ftu) +
	lookup_interpolate_between(pv->w, pv1->w, iv, ftv) +
	lookup_interpolate_between(pw->w, pw1->w, iw, ftw);
#else
    const cie_cached_vector3 *pu = &LOOKUP_VALUE(pvec->u, pc);
    const cie_cached_vector3 *pv = &LOOKUP_VALUE(pvec->v, pc + 1);
    const cie_cached_vector3 *pw = &LOOKUP_VALUE(pvec->w, pc + 2);

    pvec->u = pu->u + pv->u + pw->u;
    pvec->v = pu->v + pv->v + pw->v;
    pvec->w = pu->w + pv->w + pw->w;
#endif
}

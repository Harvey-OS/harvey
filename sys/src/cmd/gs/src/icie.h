/* Copyright (C) 1995, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: icie.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Internal definitions for interpreter CIE color handling */

#ifndef icie_INCLUDED
#  define icie_INCLUDED

/*
 * All of the routines below are exported by zcie.c for zcrd.c,
 * except for cie_cache_joint which is exported by zcrd.c for zcie.c.
 */

/* ------ Parameter acquisition ------ */

/* Get a range array parameter from a dictionary. */
/* We know that count <= 4. */
int dict_ranges_param(P4(const ref * pdref, const char *kstr, int count,
			 gs_range * prange));

/* Get 3 ranges from a dictionary. */
int dict_range3_param(P3(const ref *pdref, const char *kstr,
			 gs_range3 *prange3));

/* Get a 3x3 matrix parameter from a dictionary. */
int dict_matrix3_param(P3(const ref *pdref, const char *kstr,
			  gs_matrix3 *pmat3));

/* Get an array of procedures from a dictionary. */
/* We know count <= countof(empty_procs). */
int dict_proc_array_param(P4(const ref * pdict, const char *kstr,
			     uint count, ref * pparray));

/* Get 3 procedures from a dictionary. */
int dict_proc3_param(P3(const ref *pdref, const char *kstr, ref proc3[3]));

/* Get WhitePoint and BlackPoint values. */
int cie_points_param(P2(const ref * pdref, gs_cie_wb * pwb));

/* Process a 3- or 4-dimensional lookup table from a dictionary. */
/* The caller has set pclt->n and pclt->m. */
/* ptref is known to be a readable array of size at least n+1. */
int cie_table_param(P3(const ref * ptable, gx_color_lookup_table * pclt,
		       gs_memory_t * mem));

/* ------ Internal routines ------ */

int cie_cache_push_finish(P4(i_ctx_t *i_ctx_p, op_proc_t finish_proc,
			     gs_ref_memory_t * imem, void *data));
int cie_prepare_cache(P7(i_ctx_t *i_ctx_p, const gs_range * domain,
			 const ref * proc, cie_cache_floats * pcache,
			 void *container, gs_ref_memory_t * imem,
			 client_name_t cname));
int cie_prepare_caches_4(P10(i_ctx_t *i_ctx_p, const gs_range * domains,
			     const ref * procs,
			     cie_cache_floats * pc0,
			     cie_cache_floats * pc1,
			     cie_cache_floats * pc2,
			     cie_cache_floats * pc3 /* may be 0 */,
			     void *container,
			     gs_ref_memory_t * imem, client_name_t cname));
#define cie_prepare_cache3(p,d3,p3,c3,pcie,imem,cname)\
  cie_prepare_caches_4(p, (d3)->ranges, p3,\
		       &(c3)->floats, &(c3)[1].floats, &(c3)[2].floats,\
		       NULL, pcie, imem, cname)
#define cie_prepare_cache4(p,d4,p4,c4,pcie,imem,cname)\
  cie_prepare_caches_4(p, (d4)->ranges, p4,\
		       &(c4)->floats, &(c4)[1].floats, &(c4)[2].floats,\
		       &(c4)[3].floats, pcie, imem, cname)

int cie_cache_joint(P4(i_ctx_t *, const ref_cie_render_procs *,
		       const gs_cie_common *, gs_state *));

#endif /* icie_INCLUDED */

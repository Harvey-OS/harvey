/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gscsepr.c */
/* Separation color space type definition */
#include "gx.h"
#include "gserrors.h"
#include "gsrefct.h"
#include "gscspace.h"
#include "gsmatrix.h"			/* for gscolor2.h */
#include "gxfixed.h"			/* for gxcolor2.h */
#include "gxcolor2.h"			/* for gs_indexed_map */

extern gs_memory_t *gs_state_memory(P1(const gs_state *));

/* Define the Separation color space type. */
cs_declare_procs(private, gx_concretize_Separation, gx_install_Separation,
  gx_adjust_cspace_Separation,
  gx_enum_ptrs_Separation, gx_reloc_ptrs_Separation);
private cs_proc_concrete_space(gx_concrete_space_Separation);
private cs_proc_remap_concrete_color(gx_remap_concrete_Separation);
private cs_proc_init_color(gx_init_Separation);
const gs_color_space_type
	gs_color_space_type_Separation =
	 { gs_color_space_index_Separation, 1, false,
	   gx_init_Separation, gx_concrete_space_Separation,
	   gx_concretize_Separation, gx_remap_concrete_Separation,
	   gx_default_remap_color, gx_install_Separation,
	   gx_adjust_cspace_Separation, gx_no_adjust_color_count,
	   gx_enum_ptrs_Separation, gx_reloc_ptrs_Separation
	 };

/* Initialize a Separation color. */

private void
gx_init_Separation(gs_client_color *pcc, const gs_color_space *pcs)
{	pcc->paint.values[0] = 1.0;
}

/* Remap a Separation color. */

private const gs_color_space *
gx_concrete_space_Separation(const gs_color_space *pcs, const gs_state *pgs)
{	/* We don't support concrete Separation spaces yet. */
	const gs_color_space *pacs =
	  (const gs_color_space *)&pcs->params.separation.alt_space;
	return cs_concrete_space(pacs, pgs);
}

private int
gx_concretize_Separation(const gs_client_color *pc, const gs_color_space *pcs,
  frac *pconc, const gs_state *pgs)
{	float tint = pc->paint.values[0];
	int code;
	gs_client_color cc;
	const gs_color_space *pacs =
	  (const gs_color_space *)&pcs->params.separation.alt_space;
	if ( tint < 0 ) tint = 0;
	else if ( tint > 1 ) tint = 1;
	/* We always map into the alternate color space. */
	code = (*pcs->params.separation.map->proc.tint_transform)(&pcs->params.separation, tint, &cc.paint.values[0]);
	if ( code < 0 ) return code;
	return (*pacs->type->concretize_color)(&cc, pacs, pconc, pgs);
}

private int
gx_remap_concrete_Separation(const frac *pconc,
  gx_device_color *pdc, const gs_state *pgs)
{	/* We don't support concrete Separation colors yet. */
	return_error(gs_error_rangecheck);
}

/* Install a Separation color space. */

private int
gx_install_Separation(gs_color_space *pcs, gs_state *pgs)
{	return (*pcs->params.separation.alt_space.type->install_cspace)
		((gs_color_space *)&pcs->params.separation.alt_space, pgs);
}

/* Adjust the reference count of a Separation color space. */

private void
gx_adjust_cspace_Separation(const gs_color_space *pcs, gs_state *pgs, int delta)
{	rc_adjust_const(pcs->params.separation.map, delta,
			gs_state_memory(pgs), "gx_adjust_Separation");
	(*pcs->params.separation.alt_space.type->adjust_cspace_count)
	 ((const gs_color_space *)&pcs->params.separation.alt_space, pgs, delta);
}

/* GC procedures */

#define pcs ((gs_color_space *)vptr)

private ENUM_PTRS_BEGIN(gx_enum_ptrs_Separation) {
	return (*pcs->params.separation.alt_space.type->enum_ptrs)
		 (&pcs->params.separation.alt_space,
		  sizeof(pcs->params.separation.alt_space), index-1, pep);
	}
	ENUM_PTR(0, gs_color_space, params.separation.map);
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(gx_reloc_ptrs_Separation) {
	RELOC_PTR(gs_color_space, params.separation.map);
	(*pcs->params.separation.alt_space.type->reloc_ptrs)
	  (&pcs->params.separation.alt_space, sizeof(gs_base_color_space), gcst);
} RELOC_PTRS_END

#undef pcs

/* ---------------- Notes on real Separation colors ---------------- */

typedef ulong gs_separation;			/* BOGUS */
#define gs_no_separation ((gs_separation)(-1L))

#define dev_proc_lookup_separation(proc)\
  gs_separation proc(P4(gx_device *dev, const byte *sname, uint len,\
    gx_color_value *num_levels))

#define dev_proc_map_tint_color(proc)\
  gx_color_index proc(P4(gx_device *dev, gs_separation sepr, bool overprint,\
    gx_color_value tint))

/*
 * In principle, setting a Separation color space, or setting the device
 * when the current color space is a Separation space, calls the
 * lookup_separation device procedure to obtain the separation ID and
 * the number of achievable levels.  Currently, the only hooks for doing
 * this are unsuitable: gx_set_cmap_procs isn't called when the color
 * space changes, and doing it in gx_remap_Separation is inefficient.
 * Probably the best approach is to call gx_set_cmap_procs whenever the
 * color space changes.  In fact, if we do this, we can probably short-cut
 * two levels of procedure call in color remapping (gx_remap_color, by
 * turning it into a macro, and gx_remap_DeviceXXX, by calling the
 * cmap_proc procedure directly).  Some care will be required for the
 * implicit temporary resetting of the color space in [color]image.
 *
 * For actual remapping of Separation colors, we need cmap_separation_direct
 * and cmap_separation_halftoned, just as for the other device color spaces.
 * So we need to break apart gx_render_gray in gxdither.c so it can also
 * do the job for separations.
 */

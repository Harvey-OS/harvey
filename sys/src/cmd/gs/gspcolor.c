/* Copyright (C) 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gspcolor.c */
/* Pattern color operators and procedures for Ghostscript library */
#include "math_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gsutil.h"			/* for gs_next_ids */
#include "gxarith.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gxcoord.h"			/* for gs_concat, gx_tr'_to_fixed */
#include "gscspace.h"			/* for gscolor2.h */
#include "gxcolor2.h"
#include "gxdevice.h"
#include "gxdevmem.h"
#include "gxclip2.h"
#include "gxpath.h"
#include "gxpcolor.h"
#include "gzstate.h"

private_st_client_pattern();
public_st_pattern_instance();

/* Define the Pattern color space. */
extern cs_proc_remap_color(gx_remap_Pattern);
private cs_proc_install_cspace(gx_install_Pattern);
private cs_proc_adjust_cspace_count(gx_adjust_cspace_Pattern);
private cs_proc_init_color(gx_init_Pattern);
private cs_proc_adjust_color_count(gx_adjust_color_Pattern);
private struct_proc_enum_ptrs(gx_enum_ptrs_Pattern);
private struct_proc_reloc_ptrs(gx_reloc_ptrs_Pattern);
const gs_color_space_type
	gs_color_space_type_Pattern =
	 { gs_color_space_index_Pattern, -1, false,
	   gx_init_Pattern, gx_no_concrete_space,
	   gx_no_concretize_color, NULL,
	   gx_remap_Pattern, gx_install_Pattern,
	   gx_adjust_cspace_Pattern, gx_adjust_color_Pattern,
	   gx_enum_ptrs_Pattern, gx_reloc_ptrs_Pattern
	 };

/* makepattern */
int
gs_makepattern(gs_client_color *pcc, const gs_client_pattern *pcp,
  const gs_matrix *pmat, gs_state *pgs)
{	gs_pattern_instance inst;
	gs_pattern_instance *pinst;
	gs_state *saved;
	gs_rect bbox;
	gs_fixed_rect cbox;
	int code;
	rc_alloc_struct_1(pinst, gs_pattern_instance, &st_pattern_instance,
			  pgs->memory,
			  return_error(gs_error_VMerror), "gs_makepattern");
	inst.rc = pinst->rc;
	saved = gs_gstate(pgs);
	if ( saved == 0 )
	  return_error(gs_error_VMerror);
	gs_concat(saved, pmat);
	gs_newpath(saved);
	inst.template = *pcp;
	inst.saved = saved;
	if ( (code = gs_distance_transform2fixed(&saved->ctm, inst.template.XStep, 0.0, &inst.xstep)) < 0 ||
	     (code = gs_distance_transform2fixed(&saved->ctm, 0.0, inst.template.YStep, &inst.ystep)) < 0 ||
	     (code = gs_point_transform2fixed(&saved->ctm, 0.0, 0.0, &inst.origin)) < 0
	   )
	  {	gs_state_free(saved);
		return code;
	  }
#define dx inst.xstep
#define dy inst.ystep
	if_debug4('t', "[t]original XStep=(%g,%g) YStep=(%g,%g)\n",
		  fixed2float(dx.x), fixed2float(dx.y),
		  fixed2float(dy.x), fixed2float(dy.y));
	/* Check for non-zero step values and non-singular transformation. */
	if ( !((dx.x != 0 && dy.y != 0) ||
	       (dx.y != 0 && dy.x != 0))
	   )
	{	gs_state_free(saved);
		return_error(gs_error_undefinedresult);
	}
	/* Normalize dx & dy so that dx.x > 0, dy.y > dx.y >= 0, dy.x = 0. */
	/* Note that all of the steps in doing this are exact. */
	if ( dx.x < 0 )
	  dx.x = -dx.x, dx.y = -dx.y;
	if ( dy.x < 0 )
	  dy.x = -dy.x, dy.y = -dy.y;
	/* Now dx.x >= 0, dy.x >= 0. */
	/* Use a GCD-like algorithm to reduce one of dx.x or dy.x to 0. */
	for ( ; ; )
	  if ( dx.x == 0 )
	  {	gs_fixed_point temp;
		temp = dx;  dx = dy;  dy = temp;
		break;
	  }
	  else if ( dy.x == 0 )
		break;
	  else if ( dx.x < dy.x )
		dy.x -= dx.x, dy.y -= dx.y;
	  else
		dx.x -= dy.x, dx.y -= dy.y;
	/* Now dx.x > 0, dy.x = 0.  We may have dx.y < 0 or dy.y < 0. */
	if ( dy.y < 0 )
	  dy.y = -dy.y;
	/* Now also dy.y > 0.  We only need to ensure dy.y > dx.y >= 0. */
	if ( dx.y < 0 )
	  dx.y += dy.y * ((-dx.y + dy.y - 1) / dy.y);
	else if ( dx.y > dy.y )
	  dx.y %= dy.y;
	if_debug4('t', "[t]normalized XStep=(%g,%g) YStep=(%g,%g)\n",
		  fixed2float(dx.x), fixed2float(dx.y),
		  fixed2float(dy.x), fixed2float(dy.y));
	/* All done with the step computation. */
	gs_bbox_transform(&inst.template.BBox, &ctm_only(saved), &bbox);
	{	fixed bbw = float2fixed(bbox.q.x - bbox.p.x);
		fixed bbh = float2fixed(bbox.q.y - bbox.p.y);
		/* If the step and the size agree to within 1/2 pixel, */
		/* make them the same. */
		inst.size.x = fixed2int_var_ceiling(bbw);
		inst.size.y = fixed2int_var_ceiling(bbh);
		if ( dx.y == 0 &&
		     any_abs(dx.x - bbw) < fixed_half &&
		     any_abs(dy.y - bbh) < fixed_half
		   )
		  {	dx.x = int2fixed(inst.size.x);
			dy.y = int2fixed(inst.size.y);
			if_debug2('t',
				"[t]adjusted XStep & YStep to size=(%d,%d)\n",
				inst.size.x, inst.size.y);
		  }
	}
#undef dx
#undef dy
	gx_translate_to_fixed(saved, inst.origin.x - float2fixed(bbox.p.x),
			      inst.origin.y - float2fixed(bbox.p.y));
	cbox.p.x = fixed_0;
	cbox.p.y = fixed_0;
	cbox.q.x = int2fixed(inst.size.x);
	cbox.q.y = int2fixed(inst.size.y);
	code = gx_clip_to_rectangle(saved, &cbox);
	if ( code < 0 )
	  { gs_state_free(saved);
	    return code;
	  }
	inst.id = gs_next_ids(1);
	*pinst = inst;
	pcc->pattern = pinst;
	return 0;
}

/* setpattern */
int
gs_setpattern(gs_state *pgs, const gs_client_color *pcc)
{	int code = gs_setpatternspace(pgs);
	if ( code < 0 )
		return code;
	return gs_setcolor(pgs, pcc);
}

/* setpatternspace */
/* This does all the work of setpattern except for the final setcolor. */
int
gs_setpatternspace(gs_state *pgs)
{	int code = 0;
	if ( pgs->color_space->type->index != gs_color_space_index_Pattern )
	{	gs_color_space cs;
		cs.params.pattern.base_space =
			*(gs_paint_color_space *)pgs->color_space;
		cs.params.pattern.has_base_space = true;
		cs.type = &gs_color_space_type_Pattern;
		code = gs_setcolorspace(pgs, &cs);
	}
	return code;
}

/* getpattern */
/* This is only intended for the benefit of pattern PaintProcs. */
const gs_client_pattern *
gs_getpattern(const gs_client_color *pcc)
{	return &pcc->pattern->template;
}

/* ------ Color space implementation ------ */

/* Pattern device color types. */
/* We need a masked analogue of each of the non-pattern types, */
/* to handle uncolored patterns. */
/* We use 'masked_fill_rect' instead of 'masked_fill_rectangle' */
/* in order to limit identifier lengths to 32 characters. */
private dev_color_proc_load(gx_dc_pattern_load);
private dev_color_proc_fill_rectangle(gx_dc_pattern_fill_rectangle);
private struct_proc_enum_ptrs(dc_pattern_enum_ptrs);
private struct_proc_reloc_ptrs(dc_pattern_reloc_ptrs);
private dev_color_proc_load(gx_dc_pure_masked_load);
private dev_color_proc_fill_rectangle(gx_dc_pure_masked_fill_rect);
private struct_proc_enum_ptrs(dc_masked_enum_ptrs);
private struct_proc_reloc_ptrs(dc_masked_reloc_ptrs);
private dev_color_proc_load(gx_dc_binary_masked_load);
private dev_color_proc_fill_rectangle(gx_dc_binary_masked_fill_rect);
private struct_proc_enum_ptrs(dc_binary_masked_enum_ptrs);
private struct_proc_reloc_ptrs(dc_binary_masked_reloc_ptrs);
private dev_color_proc_load(gx_dc_colored_masked_load);
private dev_color_proc_fill_rectangle(gx_dc_colored_masked_fill_rect);
/* The device color types are exported for gxpcmap.c. */
const gx_device_color_procs
  gx_dc_pattern =
    { gx_dc_pattern_load, gx_dc_pattern_fill_rectangle,
      dc_pattern_enum_ptrs, dc_pattern_reloc_ptrs
    },
  gx_dc_pure_masked =
    { gx_dc_pure_masked_load, gx_dc_pure_masked_fill_rect,
      dc_masked_enum_ptrs, dc_masked_reloc_ptrs
    },
  gx_dc_binary_masked =
    { gx_dc_binary_masked_load, gx_dc_binary_masked_fill_rect,
      dc_binary_masked_enum_ptrs, dc_binary_masked_reloc_ptrs
    },
  gx_dc_colored_masked =
    { gx_dc_colored_masked_load, gx_dc_colored_masked_fill_rect,
      dc_masked_enum_ptrs, dc_masked_reloc_ptrs
    };
/* GC procedures */
#define cptr ((gx_device_color *)vptr)
private ENUM_PTRS_BEGIN(dc_pattern_enum_ptrs) {
	return dc_masked_enum_ptrs(vptr, size, index - 1, pep);
	}
	case 0:
	{	gx_color_tile *tile = cptr->colors.pattern.p_tile;
		ENUM_RETURN(tile - tile->index);
	}
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(dc_pattern_reloc_ptrs) {
	uint index = cptr->colors.pattern.p_tile->index;
	dc_masked_reloc_ptrs(vptr, size, gcst);
	RELOC_TYPED_OFFSET_PTR(gx_device_color, colors.pattern.p_tile, index);
} RELOC_PTRS_END
private ENUM_PTRS_BEGIN(dc_masked_enum_ptrs) return 0;
	case 0:
	{	gx_color_tile *mask = cptr->mask;
		ENUM_RETURN((mask == 0 ? mask : mask - mask->index));
	}
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(dc_masked_reloc_ptrs) {
	gx_color_tile *mask = cptr->mask;
	if ( mask != 0 )
	  {	uint index = mask->index;
		RELOC_TYPED_OFFSET_PTR(gx_device_color, mask, index);
	  }
} RELOC_PTRS_END
private ENUM_PTRS_BEGIN(dc_binary_masked_enum_ptrs) {
	return (*gx_dc_ht_binary.enum_ptrs)(vptr, size, index - 1, pep);
	}
	case 0:
	  return dc_masked_enum_ptrs(vptr, size, index, pep);
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(dc_binary_masked_reloc_ptrs) {
	dc_masked_reloc_ptrs(vptr, size, gcst);
	(*gx_dc_ht_binary.reloc_ptrs)(vptr, size, gcst);
} RELOC_PTRS_END
#undef cptr

/* Macros for pattern loading */
private int near pattern_load(P2(gx_device_color *, const gs_state *));
#define FINISH_PATTERN_LOAD\
	while ( !gx_pattern_cache_lookup(pdevc, pgs) )\
	 { code = pattern_load(pdevc, pgs);\
	   if ( code < 0 ) break;\
	 }\
	return code;

/* Ensure that a colored Pattern is loaded in the cache. */
private int
gx_dc_pattern_load(gx_device_color *pdevc, const gs_state *pgs)
{	int code = 0;
	FINISH_PATTERN_LOAD
}
/* Ensure that an uncolored Pattern is loaded in the cache. */
private int
gx_dc_pure_masked_load(gx_device_color *pdevc, const gs_state *pgs)
{	int code = gx_dc_pure_load(pdevc, pgs);
	if ( code < 0 )
	  return code;
	FINISH_PATTERN_LOAD
}
private int
gx_dc_binary_masked_load(gx_device_color *pdevc, const gs_state *pgs)
{	int code = gx_dc_ht_binary_load(pdevc, pgs);
	if ( code < 0 )
	  return code;
	FINISH_PATTERN_LOAD
}
private int
gx_dc_colored_masked_load(gx_device_color *pdevc, const gs_state *pgs)
{	int code = gx_dc_ht_colored_load(pdevc, pgs);
	if ( code < 0 )
	  return code;
	FINISH_PATTERN_LOAD
}

/* Look up a pattern color in the cache. */
bool
gx_pattern_cache_lookup(gx_device_color *pdevc, const gs_state *pgs)
{	gx_pattern_cache *pcache = pgs->pattern_cache;
	if ( pcache != 0 )
	  { gx_bitmap_id id = pdevc->id;
	    gx_color_tile *ctile = &pcache->tiles[id % pcache->num_tiles];
	    if ( ctile->id == id &&
		(pdevc->type != &gx_dc_pattern ||
		 ctile->depth == gs_currentdevice_inline(pgs)->color_info.depth)
	       )
	      { if ( pdevc->type == &gx_dc_pattern ) /* colored */
		  pdevc->colors.pattern.p_tile = ctile;
		pdevc->mask =
		  (ctile->mask.data == 0 ? (gx_color_tile *)0 :
		   ctile);
		return true;
	      }
	  }
	return false;
}
/* Remap the current (Pattern) color before trying the cache again. */
private int near
pattern_load(gx_device_color *pdevc, const gs_state *pgs)
{	const gs_color_space *pcs = pgs->color_space;
	/****** pgs->ccolor IS WRONG ******/
	return (*pcs->type->remap_color)(pgs->ccolor, pcs, pdevc, pgs);
}

#undef FINISH_PATTERN_LOAD

/* Macros for filling with a possibly masked pattern. */
/****** PHASE IS WRONG HERE ******/
#define BEGIN_PATTERN_FILL\
	  {	gx_device_tile_clip cdev;\
		gx_device *pcdev;\
		int code;\
		if ( pdevc->mask == 0 )	/* no clipping */\
		  {	code = 0;\
			pcdev = dev;\
		  }\
		else\
		  {	code = tile_clip_initialize(&cdev,\
					  &pdevc->mask->mask, dev,\
					  0, 0);\
			if ( code < 0 )\
			  return code;\
			pcdev = (gx_device *)&cdev;\
		  }
#define CLIPPING_FILL (pcdev == (gx_device *)&cdev)
#define END_PATTERN_FILL\
		return code;\
	  }

/* Macros for filling with non-standard X and Y stepping. */
/* Free variables: x, y, w, h, ptile. */
#define pdx (ptile->xstep)
#define pdy (ptile->ystep)
/* bits_or_mask is whichever of bits and mask is supplying the tile size. */
#define BEGIN_STEPS(bits_or_mask)\
	  {	int x0 = x, x1 = x + w;\
		int y0 = y, y1 = y + h;\
		int tw = ptile->bits_or_mask.size.x;\
		int th = ptile->bits_or_mask.size.y;\
		int xoff, yoff;\
		fixed\
		  xo = int2fixed(x0), xe = int2fixed(x1),\
		  yo = int2fixed(y0), ye = int2fixed(y1);\
		fixed xf, yf, cy;\
		  {	int tnx = (xo + pdx.x - int2fixed(tw)) / pdx.x;\
			fixed tpy = (pdx.y * tnx) % pdy.y;\
			int tny =\
			  (yo + pdy.y - int2fixed(th) - tpy) / pdy.y;\
			xf = pdx.x * tnx;\
			cy = pdy.y * tny + tpy;\
		  }\
		for ( ; xf < xe; xf += pdx.x )\
		  {	x = fixed2int_var(xf);\
			if ( x < x0 )\
			  xoff = x0 - x, w = tw - xoff, x = x0;\
			else xoff = 0, w = tw;\
			if ( x + w > x1 ) w = x1 - x;\
			for ( yf = cy; yf < ye; yf += pdy.y )\
			  {	y = fixed2int_var(yf);\
				if ( y < y0 )\
				  yoff = y0 - y, h = th - yoff, y = y0;\
				else yoff = 0, h = th;\
				if ( y + h > y1 ) h = y1 - y;\
				if ( CLIPPING_FILL )\
				  tile_clip_set_phase(&cdev, xoff-x, yoff-y);
#define END_STEPS\
			  }\
			if ( (cy += pdx.y) > yo )\
			  cy -= pdy.y;\
		  }\
	  }

/* Fill a rectangle with a colored Pattern. */
private int
gx_dc_pattern_fill_rectangle(const gx_device_color *pdevc, int x, int y,
  int w, int h, gx_device *dev, const gs_state *pgs)
{	gx_color_tile *ptile = pdevc->colors.pattern.p_tile;
	gx_tile_bitmap *bits;
	if ( ptile == 0 )	/* null pattern */
	  return 0;
	bits = &ptile->bits;
	/****** PHASE IS WRONG HERE ******/
	BEGIN_PATTERN_FILL
	if ( ptile->is_simple )
	  {	code = (*dev_proc(pcdev, tile_rectangle))(pcdev, bits,
			x, y, w, h,
			gx_no_color_index, gx_no_color_index,
			pgs->ht_phase.x, pgs->ht_phase.y);
	  }
	else
	  {	int w0 = w, h0 = h;
		BEGIN_STEPS(bits)
		code = (*dev_proc(pcdev, copy_color))(pcdev,
			bits->data + bits->raster * yoff,
			xoff, bits->raster,
			(w == w0 && h == h0 ? bits->id : gx_no_bitmap_id),
			x, y, w, h);
		if ( code < 0 )
		  return code;
		END_STEPS
	  }
	END_PATTERN_FILL
}
/* Fill a rectangle with an uncolored Pattern. */
private int
gx_dc_pure_masked_fill_rect(const gx_device_color *pdevc, int x, int y,
  int w, int h, gx_device *dev, const gs_state *pgs)
{	gx_color_tile *ptile = pdevc->mask;
	BEGIN_PATTERN_FILL
	if ( ptile == 0 || ptile->is_simple )
		code = gx_dc_pure_fill_rectangle(pdevc, x, y, w, h,
						 pcdev, pgs);
	else
	  {	BEGIN_STEPS(mask)
		code = gx_dc_pure_fill_rectangle(pdevc, x, y, w, h,
						 pcdev, pgs);
		if ( code < 0 )
		  return code;
		END_STEPS
	  }
	END_PATTERN_FILL
}
private int
gx_dc_binary_masked_fill_rect(const gx_device_color *pdevc, int x, int y,
  int w, int h, gx_device *dev, const gs_state *pgs)
{	gx_color_tile *ptile = pdevc->mask;
	BEGIN_PATTERN_FILL
	if ( ptile == 0 || ptile->is_simple )
		code = gx_dc_ht_binary_fill_rectangle(pdevc, x, y, w, h,
						      pcdev, pgs);
	else
	  {	BEGIN_STEPS(mask)
		code = gx_dc_ht_binary_fill_rectangle(pdevc, x, y, w, h,
						      pcdev, pgs);
		if ( code < 0 )
		  return code;
		END_STEPS
	  }
	END_PATTERN_FILL
}
private int
gx_dc_colored_masked_fill_rect(const gx_device_color *pdevc, int x, int y,
  int w, int h, gx_device *dev, const gs_state *pgs)
{	gx_color_tile *ptile = pdevc->mask;
	BEGIN_PATTERN_FILL
	if ( ptile == 0 || ptile->is_simple )
		code = gx_dc_ht_colored_fill_rectangle(pdevc, x, y, w, h,
						       pcdev, pgs);
	else
	  {	BEGIN_STEPS(mask)
		code = gx_dc_ht_colored_fill_rectangle(pdevc, x, y, w, h,
						       pcdev, pgs);
		if ( code < 0 )
		  return code;
		END_STEPS
	  }
	END_PATTERN_FILL
}

#undef BEGIN_PATTERN_FILL
#undef END_PATTERN_FILL

/* Initialize a Pattern color. */
private void
gx_init_Pattern(gs_client_color *pcc, const gs_color_space *pcs)
{	if ( pcs->params.pattern.has_base_space )
	{	gs_color_space *pbcs =
			(gs_color_space *)&pcs->params.pattern.base_space;
		cs_init_color(pcc, pbcs);
	}
	/*pcc->pattern = 0;*/		/* cs_full_init_color handles this */
}

/* Install a Pattern color space. */
private int
gx_install_Pattern(gs_color_space *pcs, gs_state *pgs)
{	if ( !pcs->params.pattern.has_base_space )
		return 0;
	return (*pcs->params.pattern.base_space.type->install_cspace)
		((gs_color_space *)&pcs->params.pattern.base_space, pgs);
}

/* Adjust the reference counts for Pattern color spaces or colors. */
private void
gx_adjust_cspace_Pattern(const gs_color_space *pcs, gs_state *pgs, int delta)
{	if ( pcs->params.pattern.has_base_space )
		(*pcs->params.pattern.base_space.type->adjust_cspace_count)
		 ((const gs_color_space *)&pcs->params.pattern.base_space, pgs, delta);
}

private void
gx_adjust_color_Pattern(const gs_client_color *pcc, const gs_color_space *pcs,
  gs_state *pgs, int delta)
{	gs_pattern_instance *pinst = pcc->pattern;
	if ( pinst != 0 && (pinst->rc.ref_count += delta) == 0 )
	{	/* Release all the storage associated with the instance. */
		gs_state *saved = pinst->saved;
		gs_state_free(saved);
		gs_free_object(saved->memory, pinst,
			       "gx_adjust_color_Pattern");
	}
	if ( pcs->params.pattern.has_base_space )
		(*pcs->params.pattern.base_space.type->adjust_color_count)
		 (pcc, (const gs_color_space *)&pcs->params.pattern.base_space,
		  pgs, delta);
}

/* GC procedures */

#define pcs ((gs_color_space *)vptr)

private ENUM_PTRS_BEGIN_PROC(gx_enum_ptrs_Pattern) {
	if ( !pcs->params.pattern.has_base_space )
	  return 0;
	return (*pcs->params.pattern.base_space.type->enum_ptrs)
		 (&pcs->params.pattern.base_space,
		  sizeof(pcs->params.pattern.base_space), index, pep);
} ENUM_PTRS_END_PROC
private RELOC_PTRS_BEGIN(gx_reloc_ptrs_Pattern) {
	if ( !pcs->params.pattern.has_base_space )
	  return;
	(*pcs->params.pattern.base_space.type->reloc_ptrs)
	  (&pcs->params.pattern.base_space, sizeof(gs_paint_color_space), gcst);
} RELOC_PTRS_END

#undef pcs

/* Copyright (C) 1989, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gspath2.c */
/* Non-constructor path routines for Ghostscript library */
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gxfixed.h"
#include "gxarith.h"
#include "gxmatrix.h"
#include "gzstate.h"
#include "gzpath.h"
#include "gzcpath.h"
#include "gscoord.h"            /* gs_itransform prototype */

/* Forward references */
private int common_clip(P2(gs_state *, int));
private int set_clip_path(P3(gs_state *, gx_clip_path *, int));

/* Path enumeration structure */
struct gs_path_enum_s {
	const segment *pseg;
	const gs_state *pgs;
	gx_path *copied_path;	/* a copy of the path, release when done */
	bool moveto_done;	/* have we reported a final moveto yet? */
};
gs_private_st_ptrs3(st_gs_path_enum, gs_path_enum, "gs_path_enum",
  path_enum_enum_ptrs, path_enum_reloc_ptrs, pseg, pgs, copied_path);

/* ------ Path transformers ------ */

int
gs_flattenpath(gs_state *pgs)
{	gx_path fpath;
	int code;
	if ( !pgs->path->curve_count ) return 0;	/* no curves */
	code = gx_path_flatten(pgs->path, &fpath, pgs->flatness, 0);
	if ( code < 0 ) return code;
	gx_path_release(pgs->path);
	*pgs->path = fpath;
	return 0;
}

int
gs_reversepath(gs_state *pgs)
{	gx_path rpath;
	int code = gx_path_copy_reversed(pgs->path, &rpath, 1);
	if ( code < 0 ) return code;
	gx_path_release(pgs->path);
	*pgs->path = rpath;
	return 0;
}

/* ------ Accessors ------ */

int
gs_pathbbox(gs_state *pgs, gs_rect *pbox)
{	gs_fixed_rect fbox;		/* box in device coordinates */
	gs_rect dbox;
	int code = gx_path_bbox(pgs->path, &fbox);
	if ( code < 0 ) return code;
	/* Transform the result back to user coordinates. */
	dbox.p.x = fixed2float(fbox.p.x);
	dbox.p.y = fixed2float(fbox.p.y);
	dbox.q.x = fixed2float(fbox.q.x);
	dbox.q.y = fixed2float(fbox.q.y);
	return gs_bbox_transform_inverse(&dbox, &ctm_only(pgs), pbox);
}

/* ------ Enumerators ------ */

/* Allocate a path enumerator. */
gs_path_enum *
gs_path_enum_alloc(gs_memory_t *mem, client_name_t cname)
{	return gs_alloc_struct(mem, gs_path_enum, &st_gs_path_enum, cname);
}

/* Start enumerating a path */
int
gs_path_enum_init(gs_path_enum *penum, const gs_state *pgs)
{	gx_path *ppath = pgs->path;
	int code;
	penum->copied_path = gs_alloc_struct(pgs->memory, gx_path, &st_path,
					     "gs_path_enum_init");
	if ( penum->copied_path == 0 )
		return_error(gs_error_VMerror);
	code = gx_path_copy(ppath, penum->copied_path, 1);
	if ( code < 0 )
	{	gs_free_object(pgs->memory, penum->copied_path,
			       "gs_path_enum_init");
		return code;
	}
	penum->pgs = pgs;
	penum->pseg = (const segment *)penum->copied_path->first_subpath;
	penum->moveto_done = false;
	return 0;
}

/* Enumerate the next element of a path. */
/* If the path is finished, return 0; */
/* otherwise, return the element type. */
int
gs_path_enum_next(gs_path_enum *penum, gs_point ppts[3])
{	const segment *pseg = penum->pseg;
	const gs_state *pgs = penum->pgs;
	gs_point pt;
	int code;
	if ( pseg == 0 )
	{	/* We've enumerated all the segments, but there might be */
		/* a trailing moveto. */
		const gx_path *ppath = pgs->path;
		if ( ppath->subpath_open < 0 && !penum->moveto_done )
		{	/* Handle a trailing moveto */
			penum->moveto_done = true;
			if ( (code = gs_itransform((gs_state *)pgs,
					fixed2float(ppath->position.x),
					fixed2float(ppath->position.y),
					&ppts[0])) < 0
			   )
				return code;
			return gs_pe_moveto;
		}
		return 0;
	}
	penum->pseg = pseg->next;
	if ( pseg->type == s_line_close )
	  return gs_pe_closepath;
	if ( (code = gs_itransform((gs_state *)pgs, fixed2float(pseg->pt.x),
				   fixed2float(pseg->pt.y), &pt)) < 0 )
	  return code;
	switch ( pseg->type )
	   {
	case s_start:
	     ppts[0] = pt;
	     return gs_pe_moveto;
	case s_line:
	     ppts[0] = pt;
	     return gs_pe_lineto;
	case s_curve:
#define pcurve ((const curve_segment *)pseg)
	     if ( (code =
		   gs_itransform((gs_state *)pgs, fixed2float(pcurve->p1.x),
				 fixed2float(pcurve->p1.y), &ppts[0])) < 0 ||
		  (code =
		   gs_itransform((gs_state *)pgs, fixed2float(pcurve->p2.x),
				 fixed2float(pcurve->p2.y), &ppts[1])) < 0 )
	       return 0;
	     ppts[2] = pt;
	     return gs_pe_curveto;
#undef pcurve
	default:
	     lprintf1("bad type %x in gs_path_enum_next!\n", pseg->type);
	     return_error(gs_error_Fatal);
	   }
}

/* Clean up after a pathforall. */
void
gs_path_enum_cleanup(gs_path_enum *penum)
{	if ( penum->copied_path != 0 )		/* don't do it twice ... */
						/* shouldn't be needed! */
	{	gx_path_release(penum->copied_path);
		gs_free_object(penum->pgs->memory, penum->copied_path,
			       "gs_path_enum_cleanup");
		penum->copied_path = 0;
	}
}

/* ------ Clipping ------ */

int
gs_clippath(gs_state *pgs)
{	gx_path path;
	int code = gx_cpath_path(pgs->clip_path, &path);
	if ( code < 0 ) return code;
	return gx_path_copy(&path, pgs->path, 1);
}

int
gs_initclip(gs_state *pgs)
{	register gx_device *dev = gs_currentdevice(pgs);
	gs_rect bbox;
	gs_fixed_rect box;
	gs_matrix imat;
	gs_defaultmatrix(pgs, &imat);
	if ( dev->ImagingBBox_set )
	{	/* Use the ImagingBBox. */
		bbox.p.x = dev->ImagingBBox[0];
		bbox.p.y = dev->ImagingBBox[1];
		bbox.q.x = dev->ImagingBBox[2];
		bbox.q.y = dev->ImagingBBox[3];
	}
	else
	{	/* Use the PageSize indented by the HWMargins. */
		bbox.p.x = dev->HWMargins[0];
		bbox.p.y = dev->HWMargins[1];
		bbox.q.x = dev->PageSize[0] - dev->HWMargins[2];
		bbox.q.y = dev->PageSize[1] - dev->HWMargins[3];
	}
	gs_bbox_transform(&bbox, &imat, &bbox);
	/* Round the clipping box so that it doesn't get ceilinged. */
	box.p.x = fixed_rounded(float2fixed(bbox.p.x));
	box.p.y = fixed_rounded(float2fixed(bbox.p.y));
	box.q.x = fixed_rounded(float2fixed(bbox.q.x));
	box.q.y = fixed_rounded(float2fixed(bbox.q.y));
	return gx_clip_to_rectangle(pgs, &box);
}

int
gs_clip(gs_state *pgs)
{	return common_clip(pgs, gx_rule_winding_number);
}

int
gs_eoclip(gs_state *pgs)
{	return common_clip(pgs, gx_rule_even_odd);
}

private int
common_clip(gs_state *pgs, int rule)
{	gx_path fpath;
	int code = gx_path_flatten(pgs->path, &fpath, pgs->flatness, 0);
	if ( code < 0 ) return code;
	code = gx_cpath_intersect(pgs, pgs->clip_path, &fpath, rule);
	if ( code != 1 ) gx_path_release(&fpath);
	if ( code < 0 ) return code;
	return set_clip_path(pgs, pgs->clip_path, rule);
}

/* Establish a rectangle as the clipping path. */
/* Used by initclip and by the character and Pattern cache logic. */
int
gx_clip_to_rectangle(gs_state *pgs, gs_fixed_rect *pbox)
{	gx_clip_path cpath;
	int code = gx_cpath_from_rectangle(&cpath, pbox, pgs->memory);
	if ( code < 0 ) return code;
	gx_cpath_release(pgs->clip_path);
	return set_clip_path(pgs, &cpath, gx_rule_winding_number);
}

/* Set the clipping path to the current path, without intersecting. */
/* Currently only used by the insideness testing operators, */
/* but might be used by viewclip eventually. */
/* The algorithm is very inefficient; we'll improve it later if needed. */
int
gx_clip_to_path(gs_state *pgs)
{	gs_fixed_rect bbox;
	int code;
	if ( (code = gx_path_bbox(pgs->path, &bbox)) < 0 ||
	     (code = gx_clip_to_rectangle(pgs, &bbox)) < 0
	   )
	  return code;
	return gs_clip(pgs);
}

/* Set the clipping path (internal). */
private int
set_clip_path(gs_state *pgs, gx_clip_path *pcpath, int rule)
{	*pgs->clip_path = *pcpath;
	pgs->clip_rule = rule;
#ifdef DEBUG
if ( gs_debug_c('p') )
  {	extern void gx_cpath_print(P1(const gx_clip_path *));
	dprintf("[p]Clipping path:\n"),
	gx_cpath_print(pcpath);
  }
#endif
	return 0;
}

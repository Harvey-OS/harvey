/* Copyright (C) 1989, 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* gxpath2.c */
/* Path tracing procedures for Ghostscript library */
#include "math_.h"
#include "gx.h"
#include "gserrors.h"
#include "gxfixed.h"
#include "gxarith.h"
#include "gzpath.h"

/* Read the current point of a path. */
int
gx_path_current_point(const gx_path *ppath, gs_fixed_point *ppt)
{	if ( !ppath->position_valid )
	  return_error(gs_error_nocurrentpoint);
	/* Copying the coordinates individually */
	/* is much faster on a PC, and almost as fast on other machines.... */
	ppt->x = ppath->position.x, ppt->y = ppath->position.y;
	return 0;
}

/* Read the bounding box of a path. */
int
gx_path_bbox(gx_path *ppath, gs_fixed_rect *pbox)
{	if ( ppath->bbox_set )
	{	/* The bounding box was set by setbbox. */
		*pbox = ppath->bbox;
		return 0;
	}
	if ( ppath->first_subpath == 0 )
	   {	/* The path is empty, use the current point if any. */
		gx_path_current_point(ppath, &pbox->p);
		return gx_path_current_point(ppath, &pbox->q);
	   }
	/* The stored bounding box may not be up to date. */
	/* Correct it now if necessary. */
	if ( ppath->box_last == ppath->current_subpath->last )
	   {	/* Box is up to date */
		*pbox = ppath->bbox;
	   }
	else
	   {	gs_fixed_rect box;
		register segment *pseg = ppath->box_last;
		if ( pseg == 0 )	/* box is uninitialized */
		   {	pseg = (segment *)ppath->first_subpath;
			box.p.x = box.q.x = pseg->pt.x;
			box.p.y = box.q.y = pseg->pt.y;
		   }
		else
		   {	box = ppath->bbox;
			pseg = pseg->next;
		   }
/* Macro for adjusting the bounding box when adding a point */
#define adjust_bbox(pt)\
  if ( (pt).x < box.p.x ) box.p.x = (pt).x;\
  else if ( (pt).x > box.q.x ) box.q.x = (pt).x;\
  if ( (pt).y < box.p.y ) box.p.y = (pt).y;\
  else if ( (pt).y > box.q.y ) box.q.y = (pt).y
		while ( pseg )
		   {	switch ( pseg->type )
			   {
			case s_curve:
#define pcurve ((curve_segment *)pseg)
				adjust_bbox(pcurve->p1);
				adjust_bbox(pcurve->p2);
#undef pcurve
				/* falls through */
			default:
				adjust_bbox(pseg->pt);
			   }
			pseg = pseg->next;
		   }
#undef adjust_bbox
		ppath->bbox = box;
		ppath->box_last = ppath->current_subpath->last;
		*pbox = box;
	   }
	return 0;
}

/* Test if a path has any curves. */
int
gx_path_has_curves(const gx_path *ppath)
{	return ppath->curve_count != 0;
}

/* Test if a path has any segments. */
int
gx_path_is_void(const gx_path *ppath)
{	return ppath->first_subpath == 0;
}

/* Test if a path is a rectangle. */
/* If so, return its bounding box. */
/* Note that this must recognize open rectangles (i.e., with one side */
/* not drawn) as well as closed rectangles, and also rectangles which */
/* are closed with lineto rather than closepath. */
int
gx_path_is_rectangle(const gx_path *ppath, gs_fixed_rect *pbox)
{	const subpath *pseg0;
	const segment *pseg1, *pseg2, *pseg3, *pseg4;
	if (	ppath->subpath_count == 1 &&
		ppath->curve_count == 0 &&
		(pseg1 = (pseg0 = ppath->first_subpath)->next) != 0 &&
		(pseg2 = pseg1->next) != 0 &&
		(pseg3 = pseg2->next) != 0 &&
		((pseg4 = pseg3->next) == 0 || pseg4->type == s_line_close ||
		 (pseg4->next == 0 && pseg4->pt.x == pseg0->pt.x &&
		  pseg4->pt.y == pseg0->pt.y))
	   )
	   {	fixed x0 = pseg0->pt.x, y0 = pseg0->pt.y;
		fixed x2 = pseg2->pt.x, y2 = pseg2->pt.y;
		if (	(x0 == pseg1->pt.x && pseg1->pt.y == y2 &&
			 x2 == pseg3->pt.x && pseg3->pt.y == y0) ||
			(x0 == pseg3->pt.x && pseg3->pt.y == y2 &&
			 x2 == pseg1->pt.x && pseg1->pt.y == y0)
		   )
		   {	/* Path is a rectangle.  Return bounding box. */
			if ( x0 < x2 )
				pbox->p.x = x0, pbox->q.x = x2;
			else
				pbox->p.x = x2, pbox->q.x = x0;
			if ( y0 < y2 )
				pbox->p.y = y0, pbox->q.y = y2;
			else
				pbox->p.y = y2, pbox->q.y = y0;
			return 1;
		   }
	   }
	return 0;
}

/* Translate an already-constructed path (in device space). */
/* Don't bother to update the cbox. */
int
gx_path_translate(gx_path *ppath, fixed dx, fixed dy)
{	segment *pseg;
#define update_xy(pt)\
  pt.x += dx, pt.y += dy
	update_xy(ppath->bbox.p);
	update_xy(ppath->bbox.q);
	update_xy(ppath->position);
	for ( pseg = (segment *)(ppath->first_subpath); pseg != 0;
	      pseg = pseg->next
	    )
	  switch ( pseg->type )
	    {
	    case s_curve:
#define pcseg ((curve_segment *)pseg)
		update_xy(pcseg->p1);
		update_xy(pcseg->p2);
#undef pcseg
	    default:
		update_xy(pseg->pt);
	    }
#undef update_xy
	return 0;
}

/* Scale an existing path by a power of 2 (positive or negative). */
void
gx_point_scale_exp2(gs_fixed_point *pt, int sx, int sy)
{	if ( sx >= 0 ) pt->x <<= sx; else pt->x >>= -sx;
	if ( sy >= 0 ) pt->y <<= sy; else pt->y >>= -sy;
}
void
gx_rect_scale_exp2(gs_fixed_rect *pr, int sx, int sy)
{	gx_point_scale_exp2(&pr->p, sx, sy);
	gx_point_scale_exp2(&pr->q, sx, sy);
}
int
gx_path_scale_exp2(gx_path *ppath, int log2_scale_x, int log2_scale_y)
{	segment *pseg;
	gx_rect_scale_exp2(&ppath->bbox, log2_scale_x, log2_scale_y);
#define update_xy(pt) gx_point_scale_exp2(&pt, log2_scale_x, log2_scale_y)
	update_xy(ppath->position);
	for ( pseg = (segment *)(ppath->first_subpath); pseg != 0;
	      pseg = pseg->next
	    )
	  switch ( pseg->type )
	    {
	    case s_curve:
#define pcseg ((curve_segment *)pseg)
		update_xy(pcseg->p1);
		update_xy(pcseg->p2);
#undef pcseg
	    default:
		update_xy(pseg->pt);
	    }
#undef update_xy
	return 0;
}

/* Reverse a path. */
/* We know ppath != ppath_old. */
int
gx_path_copy_reversed(const gx_path *ppath_old, gx_path *ppath, bool init)
{	const subpath *psub = ppath_old->first_subpath;
	int code;
#ifdef DEBUG
if ( gs_debug_c('p') )
	gx_dump_path(ppath_old, "before reversepath");
#endif
	if ( init )
		gx_path_init(ppath, ppath_old->memory);
nsp:	while ( psub )
	{	const segment *pseg = psub->last;
		const segment *prev;
		code = gx_path_add_point(ppath, pseg->pt.x, pseg->pt.y);
		if ( code < 0 )
			goto fx;
		for ( ; ; pseg = prev )
		{	prev = pseg->prev;
			switch ( pseg->type )
			{
			case s_start:
endsp:				/* Finished subpath */
				if ( psub->is_closed )
				{	code = gx_path_close_subpath(ppath);
					if ( code < 0 )
						goto fx;
				}
				psub = (const subpath *)psub->last->next;
				goto nsp;
			case s_curve:
			{	const curve_segment *pc = (const curve_segment *)pseg;
				code = gx_path_add_curve(ppath,
					pc->p2.x, pc->p2.y,
					pc->p1.x, pc->p1.y,
					prev->pt.x, prev->pt.y);
				break;
			}
			case s_line:
			case s_line_close:
				if ( prev->type == s_start && psub->is_closed )
				{	pseg = prev;
					goto endsp;
				}
				code = gx_path_add_line(ppath, prev->pt.x, prev->pt.y);
				break;
			}
			if ( code < 0 )
				goto fx;
		}
		/* not reached */
	}
	if ( ppath_old->subpath_open < 0 )	/* final moveto */
	{	code = gx_path_add_point(ppath, ppath_old->position.x,
					 ppath_old->position.y);
		if ( code < 0 )
			goto fx;
	}
#ifdef DEBUG
if ( gs_debug_c('p') )
	gx_dump_path(ppath, "after reversepath");
#endif
	return 0;
fx:	gx_path_release(ppath);
	return code;
}

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

/* gxhint3.c */
/* Apply hints for Type 1 fonts. */
#include "gx.h"
#include "gserrors.h"
#include "gxarith.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gzstate.h"
#include "gxdevmem.h"			/* ditto */
#include "gxchar.h"
#include "gxfont.h"
#include "gxfont1.h"
#include "gxtype1.h"
#include "gxop1.h"
#include "gzpath.h"

/* ------ Path hints ------ */

/*
 * Apply hints along a newly added tail of a subpath.
 * Path segments require hints as follows:
 *	Nearly vertical line: vstem hints at both ends.
 *	Nearly horizontal line: hstem hints at both ends.
 *	Curve with nearly vertical/horizontal start/end:
 *	  vstem/hstem hints at start/end.
 * We also must take care to handle the implicit closing line for
 * subpaths that aren't explicitly closed.
 */
/* The ...upper flag must equal the ...lower flag x 2. */
#define hint_vert_lower 1
#define hint_vert_upper 2
#define hint_horz_lower 4
#define hint_horz_upper 8

/* Determine which hints, if any, are applicable to a given line segment. */
private int near
line_hints(const gs_type1_state *pis, const gs_fixed_point *p0,
  const gs_fixed_point *p1)
{	fixed dx = p1->x - p0->x, adx;
	fixed dy = p1->y - p0->y, ady;
	int hints;

	/* Map the deltas back into character space. */
	if ( pis->fh.axes_swapped )
	  {	fixed t = dx; dx = dy; dy = t;
	  }
	if ( pis->fh.x_inverted )
	  dx = -dx;
	if ( pis->fh.y_inverted )
	  dy = -dy;
	adx = any_abs(dx);
	ady = any_abs(dy);
	if ( dy != 0 && adx <= ady >> 2 )
	  hints = (dy > 0 ? hint_vert_upper : hint_vert_lower);
	else if ( dx != 0 && ady <= adx >> 2 )
	  hints = (dx < 0 ? hint_horz_upper : hint_horz_lower);
	else
	  hints = 0;
	if_debug7('y', "[y]hint from 0x%lx(%g,%g) to 0x%lx(%g,%g) = %d\n",
		  (ulong)p0, fixed2float(p0->x), fixed2float(p0->y),
		  (ulong)p1, fixed2float(p1->x), fixed2float(p1->y),
		  hints);
	return hints;
}

/* Apply hints at a point.  If requested, return the amount of adjustment. */
private void near
apply_hints_at(gs_type1_state *pis, int hints, gs_fixed_point *ppt,
  gs_fixed_point *pdiff)
{	fixed ptx = ppt->x, pty = ppt->y;
	if_debug4('y', "[y]applying hints %d to 0x%lx(%g,%g) ...\n",
		  hints, (ulong)ppt, fixed2float(ptx), fixed2float(pty));
	if ( (hints & (hint_vert_upper | hint_vert_lower)) != 0 &&
	     (pis->vstem_hints.count & pis->dotsection_flag) != 0
	   )
	  {	/* "Upper" vertical edges move in +Y. */
		  apply_vstem_hints(pis, (hints & hint_vert_upper) -
				    ((hints & hint_vert_lower) << 1), ppt);
	  }
	if ( (hints & (hint_horz_upper | hint_horz_lower)) != 0 &&
	     (pis->hstem_hints.count & pis->dotsection_flag) != 0
	   )
	  {	/* "Upper" horizontal edges move in -X. */
		  apply_hstem_hints(pis, (hints & hint_horz_lower) -
				    ((hints & hint_horz_upper) >> 1), ppt);
	  }
	pdiff->x = ppt->x - ptx,
	pdiff->y = ppt->y - pty;
	/* Here is where we would round to the nearest quarter-pixel */
	/* if we wanted to. */
	if_debug2('y', "[y] ... => (%g,%g)\n",
		  fixed2float(ppt->x), fixed2float(ppt->y));
}

#ifdef DEBUG
private void near
add_hint_diff_proc(gs_fixed_point *ppt, const gs_fixed_point *pdiff)
{	if_debug7('y', "[y]adding diff (%g,%g) to 0x%lx(%g,%g) => (%g,%g)\n",
		  fixed2float(pdiff->x), fixed2float(pdiff->y), (ulong)ppt,
		  fixed2float(ppt->x), fixed2float(ppt->y),
		  fixed2float(ppt->x + pdiff->x),
		  fixed2float(ppt->y + pdiff->y));
	ppt->x += pdiff->x;
	ppt->y += pdiff->y;
}
#define add_hint_diff(pt, diff)\
  add_hint_diff_proc(&(pt), &(diff))
#else
#define add_hint_diff(pt, diff)\
  (pt).x += (diff).x, (pt).y += (diff).y
#endif

/* Apply hints along a subpath.  If closing is true, consider the subpath */
/* closed; if not, we may add more to the subpath later. */
void
apply_path_hints(gs_type1_state *pis, bool closing)
{	gx_path *ppath = pis->pgs->path;
	segment *pseg = pis->hint_next;
#define pseg_curve ((curve_segment *)pseg)
	segment *pnext;
#define pnext_curve ((curve_segment *)pnext)
	subpath *psub = ppath->current_subpath;
	int hints = pis->hints_pending;

	if ( pseg == 0 )
	  {	/* Start at the beginning of the subpath. */
		if ( psub == 0 )
		  return;
		pseg = psub->next;
		if ( pseg == 0 )
		  return;
		if ( pseg->type == s_curve )
		{	hints = line_hints(pis, &pseg_curve->p2, &pseg->pt);
			pis->hints_initial =
			  line_hints(pis, &psub->pt, &pseg_curve->p1);
		}
		else
		{	hints = line_hints(pis, &psub->pt, &pseg->pt);
			pis->hints_initial = hints;
		}
	  }
	for ( ; (pnext = pseg->next) != 0; pseg = pnext )
	  {	int hints_next;
		gs_fixed_point diff;
		/* Apply hints to the end of the previous segment */
		/* and the beginning of this one. */
		switch ( pnext->type )
		  {
		  case s_curve:
			hints |= line_hints(pis, &pseg->pt, &pnext_curve->p1);
			apply_hints_at(pis, hints, &pseg->pt, &diff);
			add_hint_diff(pnext_curve->p1, diff);
			hints_next = line_hints(pis, &pnext_curve->p2,
						&pnext->pt);
			break;
		  default:		/* s_line, s_line_close */
			hints |= hints_next =
			  line_hints(pis, &pseg->pt, &pnext->pt);
			apply_hints_at(pis, hints, &pseg->pt, &diff);
		  }
		if ( pseg->type == s_curve )
		  add_hint_diff(pseg_curve->p2, diff);
		hints = hints_next;
	  }
	if ( closing )
	  {	/* Handle the end of the subpath wrapping around to the start. */
		/* This is ugly, messy code that we can surely improve. */
		fixed ctemp;
		/* Some fonts don't use closepath when they should.... */
		bool closed =
		  (pseg->type == s_line_close ||
		   (ctemp = pseg->pt.x - psub->pt.x,
		    any_abs(ctemp) < float2fixed(0.1)) ||
		   (ctemp = pseg->pt.y - psub->pt.y,
		    any_abs(ctemp) < float2fixed(0.1)));
		segment *pfirst = psub->next;
#define pfirst_curve ((curve_segment *)pfirst)
		gs_fixed_point diff;
		int hints_first = pis->hints_initial;
		if ( closed )
		  {	/* Apply the union of the hints at both */
			/* the end and the start of the subpath. */
			hints |= hints_first;
			apply_hints_at(pis, hints, &pseg->pt, &diff);
			if ( pseg->type == s_curve )
			  add_hint_diff(pseg_curve->p2, diff);
			add_hint_diff(psub->pt, diff);
		  }
		else
		  {	int hints_close = line_hints(pis, &pseg->pt,
						     &psub->pt);
			apply_hints_at(pis, hints | hints_close, &pseg->pt,
				       &diff);
			if ( pseg->type == s_curve )
			  add_hint_diff(pseg_curve->p2, diff);
			apply_hints_at(pis, hints_first | hints_close,
				       &psub->pt, &diff);
		  }
		if ( pfirst->type == s_curve )
		  add_hint_diff(pfirst_curve->p1, diff);
		pis->hint_next = 0;
		pis->hints_pending = 0;
#undef pfirst_curve
	  }
	else
	  {	pis->hint_next = pseg;
		pis->hints_pending = hints;
	  }
#undef pseg_curve
#undef pnext_curve
}

/* ------ Individual hints ------ */

private stem_hint *near search_hints(P2(stem_hint_table *, fixed));

/*
 * Adjust a point according to the relevant hints.
 * x and y are the current point in device space after moving;
 * dx or dy is the delta component in character space.
 * The caller is responsible for checking use_hstem_hints or use_vstem_hints
 * and not calling the find_xxx_hints routine if this is false.
 * Note that if use_x/y_hints is false, no entries ever get made
 * in the stem hint tables, so these routines will not get called.
 */

/*
 * To figure out which side of the stem we are on, we assume that
 * the inside of the filled area is always to the left of the edge, i.e.,
 * edges moving in -X or +Y in character space are on the "upper" side of
 * the stem, while edges moving by +X or -Y are on the "lower" side.
 * (See section 3.5 of the Adobe Type 1 Font Format book.)
 * However, since dv0 and dv1 correspond to the lesser and greater
 * values in *device* space, we have to take the inversion of the other axis
 * into account when selecting between them.
 */

void
apply_vstem_hints(gs_type1_state *pis, fixed dy, gs_fixed_point *ppt)
{	fixed *pv = (pis->fh.axes_swapped ? &ppt->y : &ppt->x);
	stem_hint *ph = search_hints(&pis->vstem_hints, *pv);
	if ( ph != 0 )
	{
#define vstem_upper(pis, dy)\
  (pis->fh.x_inverted ? dy < 0 : dy > 0)
		if_debug3('Y', "[Y]use vstem %d: %g (%s)",
			  (int)(ph - &pis->vstem_hints.data[0]),
			  fixed2float(*pv),
			  (dy == 0 ? "middle" :
			   vstem_upper(pis, dy) ? "upper" : "lower"));
		*pv += (dy == 0 ? arith_rshift_1(ph->dv0 + ph->dv1) :
			vstem_upper(pis, dy) ? ph->dv1 : ph->dv0);
		if_debug1('Y', " -> %g\n", fixed2float(*pv));
#undef vstem_upper
	}
}

void
apply_hstem_hints(gs_type1_state *pis, fixed dx, gs_fixed_point *ppt)
{	fixed *pv = (pis->fh.axes_swapped ? &ppt->x : &ppt->y);
	stem_hint *ph = search_hints(&pis->hstem_hints, *pv);
	if ( ph != 0 )
	{
#define hstem_upper(pis, dx)\
  (pis->fh.y_inverted ? dx > 0 : dx < 0)
		if_debug3('Y', "[Y]use hstem %d: %g (%s)",
			  (int)(ph - &pis->hstem_hints.data[0]),
			  fixed2float(*pv),
			  (dx == 0 ? "middle" :
			   hstem_upper(pis, dx) ? "upper" : "lower"));
		*pv += (dx == 0 ? arith_rshift_1(ph->dv0 + ph->dv1) :
			hstem_upper(pis, dx) ? ph->dv1 : ph->dv0);
		if_debug1('Y', " -> %g\n", fixed2float(*pv));
#undef hstem_upper
	}
}

/* Search one hint table for an adjustment. */
private stem_hint *near
search_hints(stem_hint_table *psht, fixed v)
{	stem_hint *table = &psht->data[0];
	stem_hint *ph = table + psht->current;
	if ( v >= ph->v0 && v <= ph->v1 )
	  return ph;
	/* We don't bother with binary or even up/down search, */
	/* because there won't be very many hints. */
	for ( ph = &table[psht->count]; --ph >= table; )
	  if ( v >= ph->v0 && v <= ph->v1 )
	    {	psht->current = ph - table;
		return ph;
	    }
	return 0;
}


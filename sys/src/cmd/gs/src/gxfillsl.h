/* Copyright (C) 2002 artofcode LLC. All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: gxfillsl.h,v 1.8 2005/02/15 14:47:37 igor Exp $ */
/* Configurable algorithm for filling a path by scanlines. */

/*
 * Since we need several statically defined variants of this agorithm,
 * we store it in .h file and include it several times into gxfill.c .
 * Configuration macros (template arguments) are :
 * 
 *  FILL_DIRECT - See LOOP_FILL_RECTANGLE_DIRECT.
 *  TEMPLATE_spot_into_scanlines - the name of the procedure to generate.
*/

private int
TEMPLATE_spot_into_scanlines (line_list *ll, fixed band_mask)
{
    const fill_options fo = *ll->fo;
    active_line *yll = ll->y_list;
    fixed y_limit = fo.ymax;
    /*
     * The meaning of adjust_below (B) and adjust_above (A) is that the
     * pixels that would normally be painted at coordinate Y get "smeared"
     * to coordinates Y-B through Y+A-epsilon, inclusive.  This is
     * equivalent to saying that the pixels actually painted at coordinate Y
     * are those contributed by scan lines Y-A+epsilon through Y+B,
     * inclusive.  (A = B = 0 is a special case, equivalent to B = 0, A =
     * epsilon.)
     */
    fixed y_frac_min =
	(fo.adjust_above == fixed_0 ? fixed_half :
	 fixed_half + fixed_epsilon - fo.adjust_above);
    fixed y_frac_max =
	fixed_half + fo.adjust_below;
    int y0 = fixed2int(min_fixed);
    fixed y_bot = min_fixed;	/* normally int2fixed(y0) + y_frac_min */
    fixed y_top = min_fixed;	/* normally int2fixed(y0) + y_frac_max */
    fixed y = min_fixed;
    coord_range_list_t rlist;
    coord_range_t rlocal[MAX_LOCAL_ACTIVE];
    int code = 0;

    if (yll == 0)		/* empty list */
	return 0;
    range_list_init(&rlist, rlocal, countof(rlocal), ll->memory);
    ll->x_list = 0;
    ll->x_head.x_current = min_fixed;	/* stop backward scan */
    while (code >= 0) {
	active_line *alp, *nlp;
	fixed x;
	bool new_band;

	INCR(iter);

	move_al_by_y(ll, y); /* Skip horizontal pieces. */
	/*
	 * Find the next sampling point, either the bottom of a sampling
	 * band or a line start.
	 */

	if (ll->x_list == 0)
	    y = (yll == 0 ? ll->y_break : yll->start.y);
	else {
	    y = y_bot + fixed_1;
	    if (yll != 0)
		y = min(y, yll->start.y);
	    for (alp = ll->x_list; alp != 0; alp = alp->next) {
		fixed yy = max(alp->fi.y3, alp->fi.y0);
		
		yy = max(yy, alp->end.y); /* Non-monotonic curves may have an inner extreme. */
		y = min(y, yy);
	    }
	}

	/* Move newly active lines from y to x list. */

	while (yll != 0 && yll->start.y == y) {
	    active_line *ynext = yll->next;	/* insert smashes next/prev links */

	    if (yll->direction == DIR_HORIZONTAL) {
		/* Ignore for now. */
	    } else
		insert_x_new(yll, ll);
	    yll = ynext;
	}

	/* Update active lines to y. */

	x = min_fixed;
	for (alp = ll->x_list; alp != 0; alp = nlp) {
	    fixed nx;

	    nlp = alp->next;
	  e:if (alp->end.y <= y || alp->start.y == alp->end.y) {
		if (end_x_line(alp, ll, true))
		    continue;
		if (alp->more_flattened)
		    if (alp->end.y <= y || alp->start.y == alp->end.y)
			step_al(alp, true);
		goto e;
	    }
	    nx = alp->x_current = (alp->start.y >= y ? alp->start.x : AL_X_AT_Y(alp, y));
	    if (nx < x) {
		/* Move this line backward in the list. */
		active_line *ilp = alp;

		while (nx < (ilp = ilp->prev)->x_current)
		    DO_NOTHING;
		/* Now ilp->x_current <= nx < ilp->next->x_cur. */
		alp->prev->next = alp->next;
		if (alp->next)
		    alp->next->prev = alp->prev;
		if (ilp->next)
		    ilp->next->prev = alp;
		alp->next = ilp->next;
		ilp->next = alp;
		alp->prev = ilp;
		continue;
	    }
	    x = nx;
	}

	if (y > y_top || y >= y_limit) {
	    /* We're beyond the end of the previous sampling band. */
	    const coord_range_t *pcr;

	    /* Fill the ranges for y0. */

	    for (pcr = rlist.first.next; pcr != &rlist.last;
		 pcr = pcr->next
		 ) {
		int x0 = pcr->rmin, x1 = pcr->rmax;

		if_debug4('Q', "[Qr]draw 0x%lx: [%d,%d),%d\n", (ulong)pcr,
			  x0, x1, y0);
		VD_RECT(x0, y0, x1 - x0, 1, VD_TRAP_COLOR);
		code = LOOP_FILL_RECTANGLE_DIRECT(&fo, x0, y0, x1 - x0, 1);
		if_debug3('F', "[F]drawing [%d:%d),%d\n", x0, x1, y0);
		if (code < 0)
		    goto done;
	    }
	    range_list_reset(&rlist);

	    /* Check whether we've reached the maximum y. */

	    if (y >= y_limit)
		break;

	    /* Reset the sampling band. */

	    y0 = fixed2int(y);
	    if (fixed_fraction(y) < y_frac_min)
		--y0;
	    y_bot = int2fixed(y0) + y_frac_min;
	    y_top = int2fixed(y0) + y_frac_max;
	    new_band = true;
	} else
	    new_band = false;

	if (y <= y_top) {
	    /*
	     * We're within the same Y pixel.  Merge regions for segments
	     * starting here (at y), up to y_top or the end of the segment.
	     * If this is the first sampling within the band, run the
	     * fill/eofill algorithm.
	     */
	    fixed y_min;

	    if (new_band) {
		int inside = 0;

		INCR(band);
		for (alp = ll->x_list; alp != 0; alp = alp->next) {
		    int x0 = fixed2int_pixround(alp->x_current - fo.adjust_left);

		    for (;;) {
			/* We're inside a filled region. */
			print_al("step", alp);
			INCR(band_step);
			inside += alp->direction;
			if (!INSIDE_PATH_P(inside, fo.rule))
			    break;
			/*
			 * Since we're dealing with closed paths, the test
			 * for alp == 0 shouldn't be needed, but we may have
			 * omitted lines that are to the right of the
			 * clipping region.
			 */
			if ((alp = alp->next) == 0)
			    goto out;
		    }
		    /* We just went from inside to outside, so fill the region. */
		    code = range_list_add(&rlist, x0,
					  fixed2int_rounded(alp->x_current +
							    fo.adjust_right));
		    if (code < 0)
			goto done;
		}
	    out:
		y_min = min_fixed;
	    } else
		y_min = y;
	    code = merge_ranges(&rlist, ll, y_min, y_top);
	} /* else y < y_bot + 1, do nothing */
    }
 done:
    range_list_free(&rlist);
    return code;
}


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

/* gxfill.c */
/* Lower-level path filling procedures */
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gxdevice.h"
#include "gzpath.h"
#include "gzstate.h"
#include "gzcpath.h"
#include "gxdraw.h"		/* gx_fill_trapezoid_fixed prototype */

/* Import the fixed * fixed / fixed routine from gxdraw.c. */
/* The second argument must be less than or equal to the third; */
/* all must be non-negative, and the last must be non-zero. */
extern fixed fixed_mult_quo(P3(fixed, fixed, fixed));

/* Define the structure for keeping track of active lines. */
typedef struct active_line_s active_line;
struct active_line_s {
	gs_fixed_point start;		/* x,y where line starts */
	gs_fixed_point end;		/* x,y where line ends */
	fixed dx;			/* end.x - start.x */
#define al_dx(alp) ((alp)->dx)
#define al_dy(alp) ((alp)->end.y - (alp)->start.y)
	fixed y_fast_max;		/* can do x_at_y in fixed point */
					/* if y <= y_fast_max */
#define set_al_points(alp, startp, endp)\
  (alp)->dx = (endp).x - (startp).x,\
  (alp)->y_fast_max = max_fixed /\
    (((alp)->dx >= 0 ? (alp)->dx : -(alp)->dx) | 1) + (startp).y,\
  (alp)->start = startp, (alp)->end = endp
#define al_x_at_y(alp, yv)\
  ((yv) == (alp)->end.y ? (alp)->end.x :\
   ((yv) <= (alp)->y_fast_max ?\
    ((yv) - (alp)->start.y) * al_dx(alp) / al_dy(alp) :\
    (stat(n_slow_x),\
     fixed_mult_quo(al_dx(alp), (yv) - (alp)->start.y, al_dy(alp)))) +\
   (alp)->start.x)
	fixed x_current;		/* current x position */
	fixed x_next;			/* x position at end of band */
	const segment *pseg;		/* endpoint of this line */
	int direction;			/* direction of line segment */
#define dir_up 1
#define dir_horizontal 0		/* (these are handled specially) */
#define dir_down (-1)
/* "Pending" lines (not reached in the Y ordering yet) use next and prev */
/* to order lines by increasing starting Y.  "Active" lines (being scanned) */
/* use next and prev to order lines by increasing current X, or if the */
/* current Xs are equal, by increasing final X. */
	active_line *prev, *next;
/* Link together active_lines allocated individually */
	active_line *alloc_next;
};
/*
 * The active_line structure isn't really simple, but since its instances
 * only exist temporarily during a fill operation, we don't have to
 * worry about a garbage collection occurring.
 */
gs_private_st_simple(st_active_line, active_line, "active_line");

/* Define the ordering criterion for active lines. */
/* The xc argument is a copy of lp2->x_current. */
#define x_precedes(lp1, lp2, xc)\
  (lp1->x_current < xc || (lp1->x_current == xc &&\
   (lp1->start.x > lp2->start.x || lp1->end.x < lp2->end.x)))

#ifdef DEBUG
/* Internal procedures for printing active lines */
private void
print_active_line(const char *label, const active_line *alp)
{	dprintf5("[f]%s 0x%lx(%d): x_current=%f x_next=%f\n",
	         label, (ulong)alp, alp->direction,
	         fixed2float(alp->x_current), fixed2float(alp->x_next));
	dprintf5("    start=(%f,%f) pt_end=0x%lx(%f,%f)\n",
	         fixed2float(alp->start.x), fixed2float(alp->start.y),
	         (ulong)alp->pseg,
	         fixed2float(alp->end.x), fixed2float(alp->end.y));
	dprintf2("    prev=0x%lx next=0x%lx\n",
		 (ulong)alp->prev, (ulong)alp->next);
}
private void
print_line_list(const active_line *flp)
{	const active_line *lp;
	for ( lp = flp; lp != 0; lp = lp->next )
	   {	fixed xc = lp->x_current, xn = lp->x_next;
		dprintf3("[f]0x%lx(%d): x_current/next=%g",
		         (ulong)lp, lp->direction,
		         fixed2float(xc));
		if ( xn != xc ) dprintf1("/%g", fixed2float(xn));
		dputc('\n');
	   }
}
#define print_al(label,alp)\
  if ( gs_debug_c('F') ) print_active_line(label, alp)
#else
#define print_al(label,alp) DO_NOTHING
#endif

/* Line list structure */
struct line_list_s {
	gs_memory_t *memory;
	active_line *active_area;	/* allocated active_line list */
	gs_fixed_rect box;		/* intersection of bounding boxes, */
					/* disregard lines outside this */
	active_line *next_active;	/* next allocation slot */
	active_line *limit;		/* limit of local allocation */
	int close_count;		/* # of added closing lines */
	active_line *y_list;		/* Y-sorted list of pending lines */
	active_line *y_line;		/* most recently inserted line */
	active_line x_head;		/* X-sorted list of active lines */
#define x_list x_head.next
		/* Put the arrays last so the scalars will have */
		/* small displacements. */
		/* Allocate a few active_lines locally */
		/* to avoid round trips through the allocator. */
#define max_local_active 20
	active_line local_active[max_local_active];
};
typedef struct line_list_s line_list;
typedef line_list _ss *ll_ptr;

/* Forward declarations */
private int alloc_line_list(P2(ll_ptr, gs_memory_t *));
private void unclose_path(P2(gx_path *, int));
private void free_line_list(P1(ll_ptr));
private int add_y_list(P3(gx_path *, ll_ptr, fixed));
private int add_y_line(P4(const segment *, const segment *, int, ll_ptr));
private int fill_loop(P7(const gx_device_color *, int, ll_ptr,
  gs_state *, fixed, fixed, fixed));
private int near fill_slant_adjust(P9(fixed, fixed, fixed, fixed, fixed,
  fixed, fixed, const gx_device_color *, const gs_state *));
private void insert_x_new(P2(active_line *, ll_ptr));
private int update_x_list(P2(active_line *, fixed));

/* Statistics */
#ifdef DEBUG
#  define stat(x) (x++)
#  define statn(x,n) (x += (n))
private long n_fill;
private long n_fill_alloc;
private long n_y_up;
private long n_y_down;
private long n_horiz;
private long n_x_step;
private long n_slow_x;
private long n_iter;
private long n_find_y;
private long n_band;
private long n_band_step;
private long n_band_fill;
private long n_afill;
private long n_slant;
private long n_slant_shallow;
private long n_sfill;
#else
#  define stat(x) 0
#  define statn(x,n) 0
#endif

/* General area filling algorithm. */
/* The adjust parameter is a hack for keeping regions */
/* from coming out too faint: it specifies an amount by which to expand */
/* all sides of every filled region. */
/* The algorithms use the center-of-pixel rule for filling; */
/* setting adjust = fixed_half produces the effect of Adobe's */
/* any-part-of-pixel rule. */
int
gx_fill_path_banded(gx_path *ppath, gx_device_color *pdevc, gs_state *pgs,
		    int rule, fixed adjust, fixed band_mask)
{	fixed adjust_x = adjust, adjust_y = adjust;
	const gx_clip_path *pcpath = pgs->clip_path;
	gs_fixed_rect cbox;
	gx_path *pfpath;
	gx_path ffpath;
	int code;
	line_list lst;
	gx_device_clip cdev;
	bool do_clip;

	/*
	 * Compute the bounding box before we flatten the path.
	 * This can save a lot of time if the path has curves.
	 * If the path is neither fully within nor fully outside
	 * the quick-check boxes, we could recompute the bounding box
	 * and make the checks again after flattening the path,
	 * but right now we don't bother.
	 */
#define ibox lst.box
	gx_path_bbox(ppath, &ibox);
	/*
	 * Now flatten the path.  We should do this on-the-fly....
	 */
	if ( !ppath->curve_count )	/* don't need to flatten */
		pfpath = ppath;
	else
	   {	if ( (code = gx_path_flatten(ppath, &ffpath, pgs->flatness,
					     pgs->in_cachedevice > 1)) < 0
		   )
		  return code;
		pfpath = &ffpath;
	   }
	/* Check the bounding boxes. */
	gx_cpath_inner_box(pcpath, &cbox);
	if_debug9('f', "[f]adjust=%g bbox=(%g,%g),(%g,%g) inner_box=(%g,%g),(%g,%g)\n",
		  fixed2float(adjust),
		  fixed2float(ibox.p.x), fixed2float(ibox.p.y),
		  fixed2float(ibox.q.x), fixed2float(ibox.q.y),
		  fixed2float(cbox.p.x), fixed2float(cbox.p.y),
		  fixed2float(cbox.q.x), fixed2float(cbox.q.y));
	/* Check for a (nearly) empty rectangle.  This is a hack */
	/* to work around a bug in the Microsoft Windows PostScript driver, */
	/* which draws thin lines by filling zero-width rectangles, */
	/* and in some other drivers that try to fill epsilon-width */
	/* rectangles. */
	if ( ibox.q.x - ibox.p.x < fixed_half &&
	     ibox.q.y - ibox.p.y >= int2fixed(2)
	   )
	{	adjust_x = arith_rshift_1(fixed_1 + ibox.p.x - ibox.q.x + 1);
		if_debug1('f', "[f]thin adjust_x=%g\n",
			  fixed2float(adjust_x));
	}
	else
	if ( ibox.q.y - ibox.p.y < fixed_half &&
	     ibox.q.x - ibox.p.x >= int2fixed(2)
	   )
	{	adjust_y = arith_rshift_1(fixed_1 + ibox.p.y - ibox.q.y + 1);
		if_debug1('f', "[f]thin adjust_y=%g\n",
			  fixed2float(adjust_y));
	}
#define rect_within(ibox, cbox)\
  (ibox.q.y <= cbox.q.y && ibox.q.x <= cbox.q.x &&\
   ibox.p.y >= cbox.p.y && ibox.p.x >= cbox.p.x)
	if ( rect_within(ibox, cbox) )
	   {	/* Path lies entirely within clipping rectangle */
		do_clip = false;
	   }
	else
	   {	/* Intersect the path box and the clip bounding box. */
		/* If the intersection is empty, this fill is a no-op. */
		gs_fixed_rect bbox;
		gx_cpath_outer_box(pcpath, &bbox);
		if_debug4('f', "   outer_box=(%g,%g),(%g,%g)\n",
			  fixed2float(bbox.p.x), fixed2float(bbox.p.y),
			  fixed2float(bbox.q.x), fixed2float(bbox.q.y));
		if ( bbox.p.x > ibox.p.x )
		  ibox.p.x = bbox.p.x;
		if ( bbox.q.x < ibox.q.x )
		  ibox.q.x = bbox.q.x;
		if ( bbox.p.y > ibox.p.y )
		  ibox.p.y = bbox.p.y;
		if ( bbox.q.y < ibox.q.y )
		  ibox.q.y = bbox.q.y;
		if ( ibox.p.x - adjust_x >= ibox.q.x + adjust_x ||
		     ibox.p.y - adjust_y >= ibox.q.y + adjust_y
		   )
		{	/* Intersection of boxes is empty! */
			code = 0;
			goto skip;
		}
		/*
		 * The path is neither entirely inside the inner clip box
		 * nor entirely outside the outer clip box.
		 * If we had to flatten the path, this is where we would
		 * recompute its bbox and make the tests again,
		 * but we don't bother right now.
		 */
		do_clip = true;
	   }
#undef ibox
	if ( !(code = alloc_line_list(&lst, pgs->memory)) )
	   {	gx_device *save_dev;
		if ( (code = add_y_list(pfpath, &lst, adjust_y)) < 0 )
			goto nope;
		if ( do_clip )
		   {	/* Set up a clipping device. */
			gx_device *dev = (gx_device *)&cdev;
			save_dev = gs_currentdevice(pgs);
			gx_make_clip_device(&cdev, &cdev, &pcpath->list);
			cdev.target = save_dev;
			gx_set_device_only(pgs, dev);
			(*dev_proc(dev, open_device))(dev);
		   }
		code = fill_loop(pdevc, rule, &lst, pgs, adjust_x, adjust_y,
				 band_mask);
		if ( do_clip )
			gx_set_device_only(pgs, save_dev);
nope:		if ( lst.close_count != 0 )
			unclose_path(pfpath, lst.close_count);
		free_line_list(&lst);
	   }
skip:	if ( pfpath != ppath )	/* had to flatten */
		gx_path_release(pfpath);
#ifdef DEBUG
if ( gs_debug_c('f') )
	   {	dputs("[f]  # alloc    up  down  horiz step slowx  iter  find  band bstep bfill\n");
		dprintf5(" %5ld %5ld %5ld %5ld %5ld",
			 n_fill, n_fill_alloc, n_y_up, n_y_down, n_horiz);
		dprintf4(" %5ld %5ld %5ld %5ld",
			 n_x_step, n_slow_x, n_iter, n_find_y);
		dprintf3(" %5ld %5ld %5ld\n",
			 n_band, n_band_step, n_band_fill);
		dputs("[f]    afill slant shall sfill\n");
		dprintf4("       %5ld %5ld %5ld %5ld\n",
			 n_afill, n_slant, n_slant_shallow, n_sfill);
	   }
#endif
	return code;
}

/* Initialize the line list for a (flattened) path. */
private int
alloc_line_list(ll_ptr ll, gs_memory_t *mem)
{	ll->memory = mem;
	ll->active_area = 0;
	ll->next_active = ll->local_active;
	ll->limit = ll->next_active + max_local_active;
	ll->close_count = 0;
	ll->y_list = 0;
	ll->y_line = 0;
	stat(n_fill);
	return 0;
}

/* Unlink any line_close segments added temporarily. */
private void
unclose_path(gx_path *ppath, int count)
{	subpath *psub;
	for ( psub = ppath->first_subpath; count != 0;
	      psub = (subpath *)psub->last->next
	    )
	  if ( psub->last == (segment *)&psub->closer )
	  {	segment *prev = psub->closer.prev, *next = psub->closer.next;
		prev->next = next;
		if ( next ) next->prev = prev;
		psub->last = prev;
		count--;
	  }
}

/* Free the line list. */
private void
free_line_list(ll_ptr ll)
{	gs_memory_t *mem = ll->memory;
	active_line *alp;
	/* Free any individually allocated active_lines. */
	while ( (alp = ll->active_area) != 0 )
	{	active_line *next = alp->alloc_next;
		gs_free_object(mem, alp, "active line");
		ll->active_area = next;
	}
}

/* Construct a Y-sorted list of lines for a (flattened) path. */
/* We assume the path is non-empty.  Only include non-horizontal */
/* lines where one endpoint is locally Y-minimal, and horizontal lines */
/* that might color some additional pixels. */
private int
add_y_list(gx_path *ppath, ll_ptr ll, fixed adjust_y)
{	register segment *pseg = (segment *)ppath->first_subpath;
	subpath *psub;
	segment *plast;
	int first_dir, prev_dir, dir;
	segment *prev;
	int close_count = 0;
	/* fixed xmin = ll->box.p.x; */	/* not currently used */
	fixed ymin = ll->box.p.y;
	/* fixed xmax = ll->box.q.x; */	/* not currently used */
	fixed ymax = ll->box.q.y;
	int code;

	while ( pseg )
	   {	switch ( pseg->type )
		   {	/* No curves left */
		case s_start:
			psub = (subpath *)pseg;
			plast = psub->last;
			dir = 2;	/* hack to skip first line */
			if ( plast->type != s_line_close )
			   {	/* Create a fake s_line_close */
				line_close_segment *lp = &psub->closer;
				segment *next = plast->next;
				lp->next = next;
				lp->prev = plast;
				plast->next = (segment *)lp;
				if ( next ) next->prev = (segment *)lp;
				lp->type = s_line_close;
				lp->pt = psub->pt;
				lp->sub = psub;
				psub->last = plast = (segment *)lp;
				ll->close_count++;
			   }
			break;
		default:		/* s_line, _close */
		   {	fixed iy = pseg->pt.y;
			fixed py = prev->pt.y;
			/* Lines falling entirely outside the ibox in Y */
			/* are treated as though they were horizontal, */
			/* i.e., they are never put on the list. */
#define compute_dir(xo, xe, yo, ye)\
  (ye > yo ? (ye <= ymin || yo >= ymax ? 0 : dir_up) :\
   ye < yo ? (yo <= ymin || ye >= ymax ? 0 : dir_down) :\
   2)
#define add_dir_lines(prev2, prev, this, pdir, dir)\
  if ( pdir )\
   { if ( (code = add_y_line(prev2, prev, pdir, ll)) < 0 ) return code; }\
  if ( dir )\
   { if ( (code = add_y_line(prev, this, dir, ll)) < 0 ) return code; }
			dir = compute_dir(prev->pt.x, pseg->pt.x, py, iy);
			if ( dir == 2 )
			  {	/* Put horizontal lines on the list */
				/* if they would color any pixels. */
				if ( fixed2int_rounded(iy - adjust_y) <
				     fixed2int_rounded(iy + adjust_y)
				   )
				  {	stat(n_horiz);
					if ( (code = add_y_line(prev, pseg, dir_horizontal, ll)) < 0 )
					  return code;
				  }
				dir = 0;
			  }
			if ( dir > prev_dir )
			   {	add_dir_lines(prev->prev, prev, pseg, prev_dir, dir);
			   }
			else if ( prev_dir == 2 )	/* first line */
				first_dir = dir;
			if ( pseg == plast )
			   {	/* We skipped the first segment of the */
				/* subpath, so the last segment must */
				/* receive special consideration. */
				/* Note that we have `closed' all subpaths. */
				if ( first_dir > dir )
				   {	add_dir_lines(prev, pseg, psub->next, dir, first_dir);
				   }
			   }
		   }
#undef compute_dir
#undef add_dir_lines
		   }
		prev = pseg;
		prev_dir = dir;
		pseg = pseg->next;
	   }
	return close_count;
}
/* Internal routine to test a line segment and add it to the */
/* pending list if appropriate. */
private int
add_y_line(const segment *prev_lp, const segment *lp, int dir, ll_ptr ll)
{	gs_fixed_point this, prev;
	register active_line *alp = ll->next_active;
	fixed y_start;
	if ( alp == ll->limit )
	   {	/* Allocate separately */
		alp = gs_alloc_struct(ll->memory, active_line,
				      &st_active_line, "active line");
		if ( alp == 0 ) return_error(gs_error_VMerror);
		alp->alloc_next = ll->active_area;
		ll->active_area = alp;
		stat(n_fill_alloc);
	   }
	else
		ll->next_active++;
	this.x = lp->pt.x;
	this.y = lp->pt.y;
	prev.x = prev_lp->pt.x;
	prev.y = prev_lp->pt.y;
	switch ( (alp->direction = dir) )
	  {
	  case dir_up:
		y_start = prev.y;
		set_al_points(alp, prev, this);
		alp->pseg = lp;
		break;
	  case dir_down:
		y_start = this.y;
		set_al_points(alp, this, prev);
		alp->pseg = prev_lp;
		break;
	  case dir_horizontal:
		y_start = this.y;		/* = prev.y */
		alp->start = prev;
		alp->end = this;
		/* Don't need to set dx or y_fast_max */
		alp->pseg = prev_lp;		/* may not need this either */
		break;
	   }
	/* Insert the new line in the Y ordering */
	   {	register active_line *yp = ll->y_line;
		register active_line *nyp;
		if ( yp == 0 )
		   {	alp->next = alp->prev = 0;
			ll->y_list = alp;
		   }
		else if ( y_start >= yp->start.y )
		   {	/* Insert the new line after y_line */
			while ( stat(n_y_up),
			        ((nyp = yp->next) != NULL &&
				 y_start > nyp->start.y)
			      )
			  yp = nyp;
			alp->next = nyp;
			alp->prev = yp;
			yp->next = alp;
			if ( nyp ) nyp->prev = alp;
		   }
		else
		   {	/* Insert the new line before y_line */
			while ( stat(n_y_down),
			        ((nyp = yp->prev) != NULL &&
				 y_start < nyp->start.y)
			      )
			  yp = nyp;
			alp->prev = nyp;
			alp->next = yp;
			yp->prev = alp;
			if ( nyp ) nyp->next = alp;
			else ll->y_list = alp;
		   }
	   }
	ll->y_line = alp;
	print_al("add ", alp);
	return 0;
}

/* Main filling loop.  Takes lines off of y_list and adds them to */
/* x_list as needed.  band_mask limits the size of each band, */
/* by requiring that ((y1 - 1) & band_mask) == (y0 & band_mask). */
private int
fill_loop(const gx_device_color *pdevc, int rule, ll_ptr ll,
  gs_state *pgs, fixed adjust_x, fixed adjust_y, fixed band_mask)
{	active_line *yll = ll->y_list;
	gs_fixed_point pmax;
	fixed y;
	int code;
/*
 * Define a faster test for
 *	fixed2int_rounded(y - adj) != fixed2int_rounded(y + adj)
 * This is equivalent to
 *	fixed2int_rounded(fixed_fraction(y) - adj) !=
 *	  fixed2int_rounded(fixed_fraction(y) + adj)
 * or to
 *	fixed2int(fixed_fraction(y) + fixed_half - adj) !=
 *	  fixed2int(fixed_fraction(y) + fixed_half + adj)
 * Since we know that adj <= fixed_half, this is true precisely when
 *	fixed_fraction(y) >= fixed_half - adj &&
 *	  fixed_fraction(y) < fixed_half + adj
 * or equivalently when
 *	fixed_fraction(y - (fixed_half - adj)) < adj * 2
 */
	fixed half_minus_adjust_y = fixed_half - adjust_y;
	fixed adjust_y2 = adjust_y << 1;
#define adjusted_y_spans_pixel(y)\
  fixed_fraction((y) - half_minus_adjust_y) < adjust_y2

	if ( yll == 0 ) return 0;		/* empty list */
	pmax = ll->box.q;
	y = yll->start.y;			/* first Y value */
	ll->x_list = 0;
	ll->x_head.x_current = min_fixed;	/* stop backward scan */
	while ( 1 )
	   {	fixed y1;
		active_line *endp, *alp, *stopx;
		fixed x;
		int draw;
		stat(n_iter);
		/* Move newly active lines from y to x list. */
		while ( yll != 0 && yll->start.y == y )
		   {	active_line *ynext = yll->next;	/* insert smashes next/prev links */
			if ( yll->direction == dir_horizontal )
			  {	/* This is a hack to make sure that */
				/* isolated horizontal lines get stroked. */
				int yi = fixed2int_rounded(y - adjust_y);
				int xi, wi;
				if ( yll->start.x <= yll->end.x )
				  xi = fixed2int_rounded(yll->start.x - adjust_x),
				  wi = fixed2int_rounded(yll->end.x + adjust_x) - xi;
				else
				  xi = fixed2int_rounded(yll->end.x - adjust_x),
				  wi = fixed2int_rounded(yll->start.x + adjust_x) - xi;
				code = gx_fill_rectangle(xi, yi, wi, 1,
							 pdevc, pgs);
				if ( code < 0 )
				  return code;
			  }
			else
			  insert_x_new(yll, ll);
			yll = ynext;
		   }
		/* Check whether we've reached the maximum y. */
		if ( y >= pmax.y ) break;
		if ( ll->x_list == 0 )
		   {	/* No active lines, skip to next start */
			if ( yll == 0 ) break;	/* no lines left */
			y = yll->start.y;
			continue;
		   }
		/* Find the next evaluation point. */
		/* Start by finding the smallest y value */
		/* at which any currently active line ends */
		/* (or the next to-be-active line begins). */
		y1 = (yll != 0 ? yll->start.y : pmax.y);
		/* Make sure we don't exceed the maximum band height. */
		{ fixed y_band = y | ~band_mask;
		  if ( y1 > y_band ) y1 = y_band + 1;
		}
		for ( alp = ll->x_list; alp != 0; alp = alp->next )
		  if ( alp->end.y < y1 ) y1 = alp->end.y;
#ifdef DEBUG
if ( gs_debug_c('F') )
   {		dprintf2("[F]before loop: y=%f y1=%f:\n",
		         fixed2float(y), fixed2float(y1));
		print_line_list(ll->x_list);
   }
#endif
		/* Now look for line intersections before y1. */
		x = min_fixed;
#define have_pixels()\
  (fixed_rounded(y - adjust_y) < fixed_rounded(y1 + adjust_y))
		draw = (have_pixels() ? 1 : -1);
		/*
		 * Loop invariants:
		 *	alp = endp->next;
		 *	for all lines lp from stopx up to alp,
		 *	  lp->x_next = al_x_at_y(lp, y1).
		 */
		for ( alp = stopx = ll->x_list; stat(n_find_y), alp != 0;
		     endp = alp, alp = alp->next
		    )
		   {	fixed nx = al_x_at_y(alp, y1);
			fixed dx_old, dx_den;
			/* Check for intersecting lines. */
			if ( nx >= x )
				x = nx;
			else if
			   ( draw >= 0 && /* don't bother if no pixels */
			     (dx_old = alp->x_current - endp->x_current) >= 0 &&
			     (dx_den = dx_old + endp->x_next - nx) > dx_old
			   )
			   {	/* Make a good guess at the intersection */
				/* Y value using only local information. */
				fixed dy = y1 - y, y_new;
				if_debug3('f', "[f]cross: dy=%g, dx_old=%g, dx_new=%g\n",
				  fixed2float(dy), fixed2float(dx_old),
				  fixed2float(dx_den - dx_old));
				/* Do the computation in single precision */
				/* if the values are small enough. */
				y_new =
				  ((dy | dx_old) < 1L << (size_of(fixed)*4-1) ?
				   dy * dx_old / dx_den :
				   fixed_mult_quo(dy, dx_old, dx_den))
				  + y;
				/* The crossing value doesn't have to be */
				/* very accurate, but it does have to be */
				/* greater than y and less than y1. */
				if_debug3('f', "[f]cross y=%g, y_new=%g, y1=%g\n",
				  fixed2float(y), fixed2float(y_new),
				  fixed2float(y1));
				stopx = alp;
				if ( y_new <= y ) y_new = y + 1;
				if ( y_new < y1 )
				   {	y1 = y_new;
					nx = al_x_at_y(alp, y1);
					draw = 0;
				   }
				if ( nx > x ) x = nx;
			   }
			alp->x_next = nx;
		   }
		/* Recompute next_x for lines before the intersection. */
		for ( alp = ll->x_list; alp != stopx; alp = alp->next )
			alp->x_next = al_x_at_y(alp, y1);
#ifdef DEBUG
if ( gs_debug_c('F') )
   {		dprintf1("[F]after loop: y1=%f\n", fixed2float(y1));
		print_line_list(ll->x_list);
   }
#endif
		/* Fill a multi-trapezoid band for the active lines. */
		/* Don't bother if no pixel centers lie within the band. */
		if ( draw > 0 || (draw == 0 && have_pixels()) )
		{

/*******************************************************************/
/* For readability, we start indenting from the left margin again. */
/*******************************************************************/

	fixed xlbot, xltop;  /* as of last "outside" line */
	int inside = 0;
	stat(n_band);
	x = min_fixed;
	/* rule = -1 for winding number rule, i.e. */
	/* we are inside if the winding number is non-zero; */
	/* rule = 1 for even-odd rule, i.e. */
	/* we are inside if the winding number is odd. */
#define inside_path_p() ((inside & rule) != 0)
	for ( alp = ll->x_list; alp != 0; alp = alp->next )
	   {	fixed xbot = alp->x_current;
		fixed xtop = alp->x_next;
		fixed wtop;
		fixed height = y1 - y;
		int code;
		print_al("step", alp);
		stat(n_band_step);
		if ( !inside_path_p() )		/* i.e., outside */
		   {	inside += alp->direction;
			if ( inside_path_p() )	/* about to go in */
			  xlbot = xbot, xltop = xtop;
			continue;
		   }
		/* We're inside a region being filled. */
		inside += alp->direction;
		if ( inside_path_p() )		/* not about to go out */
		  continue;
		/* We just went from inside to outside, so fill the region. */
		wtop = xtop - xltop;
		stat(n_band_fill);
		/* If lines are temporarily out of */
		/* order, wtop might be negative. */
		/* Patch this up now. */
		if ( wtop < 0 )
		  {	if_debug2('f', "[f]patch %g,%g\n",
				  fixed2float(xltop), fixed2float(xtop));
			xtop = xltop += arith_rshift(wtop, 1);
			wtop = 0;
		  }
		if ( adjust_x != 0 )
		{	xlbot -= adjust_x;  xbot += adjust_x;
			xltop -= adjust_x;  xtop += adjust_x;
			wtop = xtop - xltop;
		}
		if ( adjust_y != 0 )
		{	/*
			 * We want to get the effect of filling an area whose
			 * outline is formed by dragging a square of side adj2
			 * along the border of the trapezoid.  This is *not*
			 * equivalent to simply expanding the corners by
			 * adjust: There are 3 cases needing different
			 * algorithms, plus rectangles as a fast special case.
			 */
			int xli, xi, yi;
			fixed wbot = xbot - xlbot;
			if ( (xli = fixed2int_var_rounded(xltop)) ==
			      fixed2int_var_rounded(xlbot) &&
			     (xi = fixed2int_var_rounded(xtop)) ==
			      fixed2int_var_rounded(xbot)
			   )
			   {	/* Rectangle. */
				yi = fixed2int_rounded(y - adjust_y);
				code = gx_fill_rectangle(xli, yi,
					xi - xli,
					fixed2int_rounded(y1 + adjust_y) - yi,
					pdevc, pgs);
			  }
			else if ( xltop <= xlbot )
			 { if ( xtop >= xbot )
			     {	/* Top wider than bottom. */
				code = gx_fill_trapezoid_fixed(
					xlbot, wbot, y - adjust_y,
					xltop, wtop, height,
					false, pdevc, pgs);
				if ( adjusted_y_spans_pixel(y1) )
				   {	if ( code < 0 ) return code;
					stat(n_afill);
					code = gx_fill_rectangle(
						xli, fixed2int_rounded(y1 - adjust_y),
						fixed2int_var_rounded(xtop) - xli, 1,
						pdevc, pgs);
				   }
			     }
			   else
			    {	/* Slanted trapezoid. */
				code = fill_slant_adjust(xlbot, xbot, y,
					xltop, xtop, height, adjust_y,
					pdevc, pgs);
			    }
			 }
			else
			 { if ( xtop <= xbot )
			    {	/* Bottom wider than top. */
				if ( adjusted_y_spans_pixel(y) )
				   {	stat(n_afill);
					xli = fixed2int_var_rounded(xlbot);
					code = gx_fill_rectangle(
						xli, fixed2int_rounded(y - adjust_y),
						fixed2int_var_rounded(xbot) - xli, 1,
						pdevc, pgs);
					if ( code < 0 ) return code;
				   }
				code = gx_fill_trapezoid_fixed(
					xlbot, wbot, y + adjust_y,
					xltop, wtop, height,
					false, pdevc, pgs);
			    }
			   else
			    {	/* Slanted trapezoid. */
				code = fill_slant_adjust(xlbot, xbot, y,
					xltop, xtop, height, adjust_y,
					pdevc, pgs);
			    }
			 }
		}
		else
			code = gx_fill_trapezoid_fixed(
				xlbot, xbot - xlbot, y, xltop, wtop, height,
				false, pdevc, pgs);
		if ( code < 0 ) return code;
	   }

/**************************************************************/
/* End of section requiring less indentation for readability. */
/**************************************************************/

		}
		code = update_x_list(ll->x_list, y1);
		if ( code < 0 ) return code;
		y = y1;
	   }
	return 0;
}

/* Handle the case of a slanted trapezoid with adjustment. */
/* To do this exactly right requires filling a central trapezoid */
/* plus two narrow vertical triangles or two horizontal almost-rectangles. */
/* Yuck! */
private int near
fill_slant_adjust(fixed xlbot, fixed xbot, fixed y,
  fixed xltop, fixed xtop, fixed height, fixed adjust_y,
  const gx_device_color *pdevc, const gs_state *pgs)
{	fixed adjust_y2 = adjust_y << 1;
	fixed y1 = y + height;
	int code;
	stat(n_slant);
	if ( height < adjust_y2 )
	  {	/*
		 * The upper and lower adjustment bands overlap.
		 * Use a different algorithm.
		 * Since the entire entity is less than 2 pixels high
		 * in this case, we could handle it very efficiently
		 * with no more than 2 rectangle fills, but for right now
		 * we won't attempt to do this.
		 */
		fixed xl, wx;
		int yi, hi;
		stat(n_slant_shallow);
		if ( xltop >= xlbot )		/* && xtop >= xbot */
		  wx = xtop - (xl = xlbot);
		else
		  wx = xbot - (xl = xltop);
		code = gx_fill_trapezoid_fixed(xlbot, xbot - xlbot,
					       y - adjust_y, xl, wx, height,
					       false, pdevc, pgs);
		if ( code < 0 )
		  return code;
		yi = fixed2int_rounded(y1 - adjust_y);
		hi = fixed2int_rounded(y + adjust_y) - yi;
		if ( hi > 0 )
		  {	int xi = fixed2int_var_rounded(xl);
			int wi = fixed2int_rounded(xl + wx) - xi;
			code = gx_fill_rectangle(xi, yi, wi, hi,
						 pdevc, pgs);
			if ( code < 0 )
			  return code;
		  }
		code = gx_fill_trapezoid_fixed(xl, wx, y + adjust_y,
					       xltop, xtop - xltop, height,
					       false, pdevc, pgs);
	  }
	else
	  {	fixed dx_left = xltop - xlbot, dx_right = xtop - xbot;
		fixed xlb, xrb, xlt, xrt;
		int xli, xri;
		fixed half_minus_adjust_y = fixed_half - adjust_y;
		if ( dx_left <= 0 )		/* && dx_right <= 0 */
		  {	xlb = xlbot -
			  fixed_mult_quo(-dx_left, adjust_y2, height);
			xrb = xbot;
			xlt = xltop;
			xrt = xtop +
			  fixed_mult_quo(-dx_right, adjust_y2, height);
		  }
		else		/* dx_left >= 0, dx_right >= 0 */
		  {	xlb = xlbot;
			xrb = xbot +
			  fixed_mult_quo(dx_right, adjust_y2, height);
			xlt = xltop -
			  fixed_mult_quo(dx_left, adjust_y2, height);
			xrt = xtop;
		  }
		/* Do the bottom adjustment band, if any. */
		if ( adjusted_y_spans_pixel(y) )
		  {	/* We can always do this with a rectangle, */
			/* but the computation may be too much trouble. */
			stat(n_sfill);
			if ( (xli = fixed2int_var_rounded(xlbot)) ==
			      fixed2int_var_rounded(xlb) &&
			     (xri = fixed2int_var_rounded(xbot)) ==
			      fixed2int_var_rounded(xrb)
			   )
			  code =
			    gx_fill_rectangle(xli, fixed2int_rounded(y - adjust_y),
					      xri - xli, 1, pdevc, pgs);
			else
			  code =
			    gx_fill_trapezoid_fixed(xlbot, xbot - xlbot,
						    y - adjust_y,
						    xlb, xrb - xlb, adjust_y2,
						    false, pdevc, pgs);
			if ( code < 0 )
			  return code;
		  }
		/* Do the central trapezoid. */
		code = gx_fill_trapezoid_fixed(xlb, xrb - xlb, y + adjust_y,
					       xlt, xrt - xlt,
					       height - adjust_y2,
					       false, pdevc, pgs);
		/* Do the top adjustment band. */
		if ( adjusted_y_spans_pixel(y1) )
		  {	if ( code < 0 )
			  return code;
			stat(n_sfill);
			if ( (xli = fixed2int_var_rounded(xltop)) ==
			      fixed2int_var_rounded(xlt) &&
			     (xri = fixed2int_var_rounded(xtop)) ==
			      fixed2int_var_rounded(xrt)
			   )
			  code =
			    gx_fill_rectangle(xli, fixed2int_rounded(y1 - adjust_y),
					      xri - xli, 1, pdevc, pgs);
			else
			  code =
			    gx_fill_trapezoid_fixed(xlt, xrt - xlt,
						    y1 - adjust_y,
						    xltop, xtop - xltop,
						    adjust_y2,
						    false, pdevc, pgs);
		  }
	  }
	return code;
}

/* Insert a newly active line in the X ordering. */
private void
insert_x_new(active_line *alp, ll_ptr ll)
{	register active_line *next;
	register active_line *prev = &ll->x_head;
	register fixed x = alp->start.x;
	alp->x_current = x;
	while ( stat(n_x_step),
		(next = prev->next) != 0 && x_precedes(next, alp, x)
	       )
		prev = next;
	alp->next = next;
	alp->prev = prev;
	if ( next != 0 ) next->prev = alp;
	prev->next = alp;
}

/* Clean up after a pass through the main loop. */
/* If any lines are out of order, re-sort them now. */
/* Also drop any ended lines. */
private int
update_x_list(active_line *x_first, fixed y1)
{	fixed x;
	register active_line *alp;
	active_line *nlp;
	for ( x = min_fixed, alp = x_first; alp != 0; alp = nlp )
	   {	fixed nx = alp->x_current = alp->x_next;
		nlp = alp->next;
		if_debug4('F', "[F]check 0x%lx,x=%g 0x%lx,x=%g\n",
			  (ulong)alp->prev, fixed2float(x),
			  (ulong)alp, fixed2float(nx));
		if ( alp->end.y == y1 )
		   {	/* Handle a line segment that just ended. */
			const segment *pseg = alp->pseg;
			const segment *next;
			gs_fixed_point npt;
			/*
			 * The computation of next relies on the fact that
			 * all subpaths have been closed.  When we cycle
			 * around to the other end of a subpath, we must be
			 * sure not to process the start/end point twice.
			 */
			next =
			  (alp->direction == dir_up ?
			   (/* Upward line, go forward along path. */
			    pseg->type == s_line_close ? /* end of subpath */
			     ((line_close_segment *)pseg)->sub->next :
			     pseg->next) :
			   (/* Downward line, go backward along path. */
			    pseg->type == s_start ? /* start of subpath */
			     ((subpath *)pseg)->last->prev :
			     pseg->prev)
			  );
			npt.y = next->pt.y;
			if_debug5('F', "[F]ended 0x%lx: pseg=0x%lx y=%f next=0x%lx npt.y=%f\n",
				 (ulong)alp, (ulong)pseg, fixed2float(pseg->pt.y),
				 (ulong)next, fixed2float(npt.y));
			if ( npt.y <= pseg->pt.y )
			   {	/* End of a line sequence */
				alp->prev->next = nlp;
				if ( nlp ) nlp->prev = alp->prev;
				if_debug1('F', "[F]drop 0x%lx\n", (ulong)alp);
				continue;
			   }
			else
			   {	alp->pseg = next;
				npt.x = next->pt.x;
				set_al_points(alp, alp->end, npt);
				print_al("repl", alp);
			   }
		   }
		if ( nx <= x )
		   {	/* Move alp backward in the list. */
			active_line *prev = alp->prev;
			active_line *next = nlp;
			prev->next = next;
			if ( next ) next->prev = prev;
			while ( !x_precedes(prev, alp, nx) )
			   {	if_debug2('f', "[f]swap 0x%lx,0x%lx\n",
				         (ulong)alp, (ulong)prev);
				next = prev, prev = prev->prev;
			   }
			alp->next = next;
			alp->prev = prev;
			/* next might be null, if alp was in */
			/* the correct spot already. */
			if ( next ) next->prev = alp;
			prev->next = alp;
		   }
		else
			x = nx;
	   }
#ifdef DEBUG
if ( gs_debug_c('f') )
	for ( alp = x_first->prev->next; alp != 0; alp = alp->next )
	 if ( alp->next != 0 && alp->next->x_current < alp->x_current )
	   {	lprintf("[f]Lines out of order!\n");
		print_active_line("   1:", alp);
		print_active_line("   2:", alp->next);
		return_error(gs_error_Fatal);
	   }
#endif
	return 0;
}

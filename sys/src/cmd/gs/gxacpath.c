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

/* gxacpath.c */
/* Accumulator for clipping paths */
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gxdevice.h"
#include "gxfixed.h"
#include "gzpath.h"
#include "gxpaint.h"
#include "gzcpath.h"

/* Imported procedures */
extern gx_device *gs_currentdevice(P1(const gs_state *));
extern bool clip_list_validate(P1(gx_clip_list *));

/* Device for accumulating a rectangle list. */
typedef struct gx_device_accum_s {
	gx_device_common;
	gs_memory_t *list_memory;		/* set by client */
	gs_int_rect bbox;
	gx_clip_list list;
} gx_device_accum;

/* Device procedures */
private dev_proc_open_device(accum_open);
private dev_proc_close_device(accum_close);
private dev_proc_fill_rectangle(accum_fill_rectangle);

/* The device descriptor */
/* Many of these procedures won't be called; they are set to NULL. */
private const gx_device_accum gs_accum_device =
{	std_device_std_body(gx_device_accum, 0, "clip list accumulator",
	  0, 0, 1, 1),
	{	accum_open,
		NULL,			/* get_initial_matrix */
		NULL,			/* sync_output */
		NULL,			/* output_page */
		accum_close,
		NULL,			/* map_rgb_color */
		NULL,			/* map_color_rgb */
		accum_fill_rectangle
	}
};

/* Intersect two clipping paths using an accumulator. */
int
gx_cpath_intersect_slow(gs_state *pgs, gx_clip_path *pcpath, gx_path *ppath,
  int rule)
{	gx_device_accum adev;
	gx_device_color devc;
	gx_device *save_dev = gs_currentdevice(pgs);
	int code;
	adev = gs_accum_device;
	adev.list_memory = pcpath->path.memory;
	(*dev_proc(&adev, open_device))((gx_device *)&adev);
	color_set_pure(&devc, 0);	/* arbitrary, but not */
					/* transparent */
	gx_set_device_only(pgs, (gx_device *)&adev);
	code = gx_fill_path(ppath, &devc, pgs, rule, fixed_half);
	gx_set_device_only(pgs, save_dev);
	if ( code < 0 ||
	     (code = (*dev_proc(&adev, close_device))((gx_device *)&adev)) < 0
	   )
	{	gx_clip_list_free(&adev.list, adev.list_memory);
		return code;
	}
	pcpath->list = adev.list;
	gx_path_init(&pcpath->path, pcpath->path.memory);
	pcpath->path.bbox.p.x = int2fixed(adev.bbox.p.x);
	pcpath->path.bbox.p.y = int2fixed(adev.bbox.p.y);
	pcpath->path.bbox.q.x = int2fixed(adev.bbox.q.x);
	pcpath->path.bbox.q.y = int2fixed(adev.bbox.q.y);
	/* Using the setbbox flag here is slightly bogus, */
	/* but it's as good a way as any to indicate that */
	/* the bbox is accurate. */
	pcpath->path.bbox_set = 1;
	/* Note that the result of the intersection might be */
	/* a single rectangle.  This will cause clip_path_is_rect.. */
	/* to return true.  This, in turn, requires that */
	/* we set pcpath->inner_box correctly. */
	if ( clip_list_is_rectangle(&adev.list) )
		pcpath->inner_box = pcpath->path.bbox;
	else
	{	/* The quick check must fail. */
		pcpath->inner_box.p.x = pcpath->inner_box.p.y = 0;
		pcpath->inner_box.q.x = pcpath->inner_box.q.y = 0;
	}
	gx_cpath_set_outer_box(pcpath);
	pcpath->segments_valid = 0;
	pcpath->shares_list = 0;
	return 0;
}

/* ------ Device implementation ------ */

#define adev ((gx_device_accum *)dev)

/* Initialize the accumulation device. */
private int
accum_open(register gx_device *dev)
{	gx_clip_list_init(&adev->list);
	adev->bbox.p.x = adev->bbox.p.y = max_int;
	adev->bbox.q.x = adev->bbox.q.y = min_int;
	return 0;
}

/* Close the accumulation device. */
private int
accum_close(gx_device *dev)
{
#ifdef DEBUG
if ( gs_debug_c('q') )
   {	gx_clip_rect *rp =
	  (adev->list.count <= 1 ? &adev->list.single : adev->list.head);
	dprintf4("[q]list at 0x%lx, count=%d, head=0x%lx, tail=0x%lx:\n",
		 (ulong)&adev->list, adev->list.count,
		 (ulong)adev->list.head, (ulong)adev->list.tail);
	while ( rp != 0 )
	   {	clip_rect_print('q', "   ", rp);
		rp = rp->next;
	   }
   }
	if ( !clip_list_validate(&adev->list) )
	  {	lprintf1("[q]Bad clip list 0x%lx!\n", (ulong)&adev->list);
		return_error(gs_error_Fatal);
	  }
#endif
	return 0;
}

/* Accumulate one rectangle. */
#undef adev
/* Allocate a rectangle to be added to the list. */
static const gx_clip_rect clip_head_rect =
  { 0, 0, min_int, min_int, min_int, min_int };
static const gx_clip_rect clip_tail_rect =
  { 0, 0, max_int, max_int, max_int, max_int };
private gx_clip_rect *
accum_alloc_rect(gx_device_accum *adev)
{	gs_memory_t *mem = adev->list_memory;
	gx_clip_rect *ar = gs_alloc_struct(mem, gx_clip_rect, &st_clip_rect,
					   "accum_alloc_rect");
	if ( ar == 0 )
	  return 0;
	if ( adev->list.count == 2 )
	  {	/* We're switching from a single rectangle to a list. */
		/* Allocate the head and tail entries. */
		gx_clip_rect *head = ar;
		gx_clip_rect *tail =
		  gs_alloc_struct(mem, gx_clip_rect, &st_clip_rect,
				  "accum_alloc_rect(tail)");
		gx_clip_rect *single =
		  gs_alloc_struct(mem, gx_clip_rect, &st_clip_rect,
				  "accum_alloc_rect(single)");
		ar = gs_alloc_struct(mem, gx_clip_rect, &st_clip_rect,
				     "accum_alloc_rect(head)");
		if ( tail == 0 || single == 0 || ar == 0 )
		  {	gs_free_object(mem, ar, "accum_alloc_rect");
			gs_free_object(mem, single, "accum_alloc_rect(single)");
			gs_free_object(mem, tail, "accum_alloc_rect(tail)");
			gs_free_object(mem, head, "accum_alloc_rect(head)");
			return 0;
		  }
		*head = clip_head_rect;
		head->next = single;
		*single = adev->list.single;
		single->prev = head;
		single->next = tail;
		*tail = clip_tail_rect;
		tail->prev = single;
		adev->list.head = head;
		adev->list.tail = tail;
	  }
	return ar;
}
#define accum_alloc(s, ar, px, py, qx, qy)\
	if ( ++(adev->list.count) == 1 )\
	  ar = &adev->list.single;\
	else if ( (ar = accum_alloc_rect(adev)) == 0 )\
	  return_error(gs_error_VMerror);\
	ar->xmin = px, ar->ymin = py, ar->xmax = qx, ar->ymax = qy;\
	clip_rect_print('Q', s, ar)
/* Link or unlink a rectangle in the list. */
#define accum_add_last(ar)\
	accum_add_before(ar, adev->list.tail)
#define accum_add_after(ar, rprev)\
	ar->prev = (rprev), (ar->next = (rprev)->next)->prev = ar,\
	  (rprev)->next = ar
#define accum_add_before(ar, rnext)\
	(ar->prev = (rnext)->prev)->next = ar, ar->next = (rnext),\
	  (rnext)->prev = ar
#define accum_remove(ar)\
	ar->next->prev = ar->prev, ar->prev->next = ar->next
/* Free a rectangle that was removed from the list. */
#define accum_free(s, ar)\
	if ( --(adev->list.count) )\
	  { clip_rect_print('Q', s, ar);\
	    gs_free_object(adev->list_memory, ar, "accum_rect");\
	  }
/*
 * Add a rectangle to the list.  It would be wonderful if rectangles
 * were always disjoint and always presented in the correct order,
 * but they aren't: the fill loop works by trapezoids, not by scan lines,
 * and may produce slightly overlapping rectangles because of "fattening".
 * All we can count on is that they are approximately disjoint and
 * approximately in order.
 */
private int
accum_add_rect(gx_device_accum *adev, int x, int y, int xe, int ye)
{	gx_clip_rect *nr;
	gx_clip_rect *ar;
	register gx_clip_rect *rptr;
	int ymin, ymax;
top:	accum_alloc("accum", nr, x, y, xe, ye);
	if ( adev->list.count == 1 )
	  return 0;		/* single rectangle */
	rptr = adev->list.tail->prev;
	if ( y >= rptr->ymax ||
	     (y == rptr->ymin && ye == rptr->ymax && x >= rptr->xmax)
	   )
	{	accum_add_last(nr);
		return 0;
	}
	/* Work backwards till we find the insertion point. */
	while ( ye <= rptr->ymin ) rptr = rptr->prev;
	ymin = rptr->ymin;
	ymax = rptr->ymax;
	if ( ye > ymax )
	{	if ( y >= ymax )
		{	/* Insert between two bands. */
			accum_add_after(nr, rptr);
			return 0;
		}
		/* Split off the top part of the new rectangle. */
		accum_alloc("a.top", ar, x, ymax, xe, ye);
		accum_add_after(ar, rptr);
		ye = nr->ymax = ymax;
		clip_rect_print('Q', " ymax", nr);
	}
	/* Here we know ymin < ye <= ymax; */
	/* rptr points to the last node with this value of ymin/ymax. */
	/* If necessary, split off the part of the existing band */
	/* that is above the new band. */
	if ( ye < ymax )
	{	gx_clip_rect *rsplit = rptr;
		while ( rsplit->ymax == ymax )
		{	accum_alloc("s.top", ar, rsplit->xmin, ye, rsplit->xmax, ymax);
			accum_add_after(ar, rptr);
			rsplit->ymax = ye;
			rsplit = rsplit->prev;
		}
		ymax = ye;
	}
	/* Now ye = ymax.  If necessary, split off the part of the */
	/* existing band that is below the new band. */
	if ( y > ymin )
	{	gx_clip_rect *rbot = rptr, *rsplit;
		while ( rbot->prev->ymin == ymin )
			rbot = rbot->prev;
		for ( rsplit = rbot; ; )
		{	accum_alloc("s.bot", ar, rsplit->xmin, ymin, rsplit->xmax, y);
			accum_add_before(ar, rbot);
			rsplit->ymin = y;
			if ( rsplit == rptr ) break;
			rsplit = rsplit->next;
		}
		ymin = y;
	}
	/* Now y <= ymin as well.  (y < ymin is possible.) */
	nr->ymin = ymin;
	/* Search for the X insertion point. */
	for ( ; rptr->ymin == ymin; rptr = rptr->prev )
	{	if ( xe < rptr->xmin ) continue;  /* still too far to right */
		if ( x > rptr->xmax ) break;	/* disjoint */
		/* The new rectangle overlaps an existing one.  Merge them. */
		if ( xe > rptr->xmax )
		  {	rptr->xmax = nr->xmax;	/* might be > xe if */
					/* we already did a merge */
			clip_rect_print('Q', "widen", rptr);
		  }
		accum_free("free", nr);
		if ( x >= rptr->xmin )
		  goto out;
		/* Might overlap other rectangles to the left. */
		rptr->xmin = x;
		nr = rptr;
		accum_remove(rptr);
		clip_rect_print('Q', "merge", nr);
	}
	accum_add_after(nr, rptr);
out:	/* Check whether there are only 0 or 1 rectangles left. */
	if ( adev->list.count <= 1 )
	  {	/* We're switching from a list to at most 1 rectangle. */
		/* Free the head and tail entries. */
		gs_memory_t *mem = adev->list_memory;
		gx_clip_rect *single = adev->list.head->next;
		if ( single != adev->list.tail )
		  {	adev->list.single = *single;
			gs_free_object(mem, single, "accum_free_rect(single)");
			adev->list.single.next = adev->list.single.prev = 0;
		  }
		gs_free_object(mem, adev->list.tail, "accum_free_rect(tail)");
		gs_free_object(mem, adev->list.head, "accum_free_rect(head)");
		adev->list.head = 0;
		adev->list.tail = 0;
	  }
	/* Check whether there is still more of the new band to process. */
	if ( y < ymin )
	{	/* Continue with the bottom part of the new rectangle. */
		clip_rect_print('Q', " ymin", nr);
		ye = ymin;
		goto top;
	}
	return 0;
}
#define adev ((gx_device_accum *)dev)
private int
accum_fill_rectangle(gx_device *dev, int x, int y, int w, int h,
  gx_color_index color)
{	int xe, ye;
	if ( w <= 0 || h <= 0 ) return 0;
	xe = x + w, ye = y + h;
	/* Update the bounding box. */
	if ( x < adev->bbox.p.x ) adev->bbox.p.x = x;
	if ( y < adev->bbox.p.y ) adev->bbox.p.y = y;
	if ( xe > adev->bbox.q.x ) adev->bbox.q.x = xe;
	if ( ye > adev->bbox.q.y ) adev->bbox.q.y = ye;
	return accum_add_rect(adev, x, y, xe, ye);
}

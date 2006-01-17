/* Copyright (C) 1993, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gxacpath.c,v 1.10 2004/08/04 19:36:12 stefan Exp $ */
/* Accumulator for clipping paths */
#include "gx.h"
#include "gserrors.h"
#include "gsrop.h"
#include "gsstruct.h"
#include "gsutil.h"
#include "gsdcolor.h"
#include "gxdevice.h"
#include "gxfixed.h"
#include "gxistate.h"
#include "gzpath.h"
#include "gxpaint.h"
#include "gzcpath.h"
#include "gzacpath.h"

/* Device procedures */
private dev_proc_open_device(accum_open);
private dev_proc_close_device(accum_close);
private dev_proc_fill_rectangle(accum_fill_rectangle);

/* The device descriptor */
/* Many of these procedures won't be called; they are set to NULL. */
private const gx_device_cpath_accum gs_cpath_accum_device =
{std_device_std_body(gx_device_cpath_accum, 0, "clip list accumulator",
		     0, 0, 1, 1),
 {accum_open,
  NULL,
  NULL,
  NULL,
  accum_close,
  NULL,
  NULL,
  accum_fill_rectangle,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  gx_default_fill_path,
  gx_default_stroke_path,
  NULL,
  gx_default_fill_trapezoid,
  gx_default_fill_parallelogram,
  gx_default_fill_triangle,
  gx_default_draw_thin_line,
  gx_default_begin_image,
  gx_default_image_data,
  gx_default_end_image,
  NULL,
  NULL,
  gx_get_largest_clipping_box,
  gx_default_begin_typed_image,
  NULL,
  NULL,
  NULL,
  NULL,
  gx_default_text_begin,
  gx_default_finish_copydevice,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
 }
};

/* Start accumulating a clipping path. */
void
gx_cpath_accum_begin(gx_device_cpath_accum * padev, gs_memory_t * mem)
{
    gx_device_init((gx_device *) padev,
		   (const gx_device *) & gs_cpath_accum_device,
		   NULL /* allocated on stack */ , true);
    padev->list_memory = mem;
    (*dev_proc(padev, open_device)) ((gx_device *) padev);
}

void
gx_cpath_accum_set_cbox(gx_device_cpath_accum * padev,
			const gs_fixed_rect * pbox)
{
    padev->clip_box.p.x = fixed2int_var(pbox->p.x);
    padev->clip_box.p.y = fixed2int_var(pbox->p.y);
    padev->clip_box.q.x = fixed2int_var_ceiling(pbox->q.x);
    padev->clip_box.q.y = fixed2int_var_ceiling(pbox->q.y);
}

/* Finish accumulating a clipping path. */
int
gx_cpath_accum_end(const gx_device_cpath_accum * padev, gx_clip_path * pcpath)
{
    int code = (*dev_proc(padev, close_device)) ((gx_device *) padev);
    /* Make an entire clipping path so we can use cpath_assign. */
    gx_clip_path apath;

    if (code < 0)
	return code;
    gx_cpath_init_local(&apath, padev->list_memory);
    apath.rect_list->list = padev->list;
    if (padev->list.count == 0)
	apath.path.bbox.p.x = apath.path.bbox.p.y =
	apath.path.bbox.q.x = apath.path.bbox.q.y = 0;
    else {
	apath.path.bbox.p.x = int2fixed(padev->bbox.p.x);
	apath.path.bbox.p.y = int2fixed(padev->bbox.p.y);
	apath.path.bbox.q.x = int2fixed(padev->bbox.q.x);
	apath.path.bbox.q.y = int2fixed(padev->bbox.q.y);
    }
    /* indicate that the bbox is accurate */
    apath.path.bbox_accurate = 1;
    /* Note that the result of the intersection might be */
    /* a single rectangle.  This will cause clip_path_is_rect.. */
    /* to return true.  This, in turn, requires that */
    /* we set apath.inner_box correctly. */
    if (clip_list_is_rectangle(&padev->list))
	apath.inner_box = apath.path.bbox;
    else {
	/* The quick check must fail. */
	apath.inner_box.p.x = apath.inner_box.p.y = 0;
	apath.inner_box.q.x = apath.inner_box.q.y = 0;
    }
    gx_cpath_set_outer_box(&apath);
    apath.path_valid = false;
    apath.id = gs_next_ids(padev->list_memory, 1);	/* path changed => change id */
    gx_cpath_assign_free(pcpath, &apath);
    return 0;
}

/* Discard an accumulator in case of error. */
void
gx_cpath_accum_discard(gx_device_cpath_accum * padev)
{
    gx_clip_list_free(&padev->list, padev->list_memory);
}

/* Intersect two clipping paths using an accumulator. */
int
gx_cpath_intersect_path_slow(gx_clip_path * pcpath, gx_path * ppath,
			     int rule, gs_imager_state *pis)
{
    gs_logical_operation_t save_lop = gs_current_logical_op_inline(pis);
    gx_device_cpath_accum adev;
    gx_device_color devc;
    gx_fill_params params;
    int code;

    gx_cpath_accum_begin(&adev, pcpath->path.memory);
    set_nonclient_dev_color(&devc, 0);	/* arbitrary, but not transparent */
    gs_set_logical_op_inline(pis, lop_default);
    params.rule = rule;
    params.adjust.x = params.adjust.y = fixed_half;
    params.flatness = gs_currentflat_inline(pis);
    params.fill_zero_width = true;
    code = gx_fill_path_only(ppath, (gx_device *)&adev, pis,
			     &params, &devc, pcpath);
    if (code < 0 || (code = gx_cpath_accum_end(&adev, pcpath)) < 0)
	gx_cpath_accum_discard(&adev);
    gs_set_logical_op_inline(pis, save_lop);
    return code;
}

/* ------ Device implementation ------ */

#ifdef DEBUG
/* Validate a clipping path after accumulation. */
private bool
clip_list_validate(const gx_clip_list * clp)
{
    if (clp->count <= 1)
	return (clp->head == 0 && clp->tail == 0 &&
		clp->single.next == 0 && clp->single.prev == 0);
    else {
	const gx_clip_rect *prev = clp->head;
	const gx_clip_rect *ptr;
	bool ok = true;

	while ((ptr = prev->next) != 0) {
	    if (ptr->ymin > ptr->ymax || ptr->xmin > ptr->xmax ||
		!(ptr->ymin >= prev->ymax ||
		  (ptr->ymin == prev->ymin &&
		   ptr->ymax == prev->ymax &&
		   ptr->xmin >= prev->xmax)) ||
		ptr->prev != prev
		) {
		clip_rect_print('q', "WRONG:", ptr);
		ok = false;
	    }
	    prev = ptr;
	}
	return ok && prev == clp->tail;
    }
}
#endif /* DEBUG */

/* Initialize the accumulation device. */
private int
accum_open(register gx_device * dev)
{
    gx_device_cpath_accum * const adev = (gx_device_cpath_accum *)dev;

    gx_clip_list_init(&adev->list);
    adev->bbox.p.x = adev->bbox.p.y = max_int;
    adev->bbox.q.x = adev->bbox.q.y = min_int;
    adev->clip_box.p.x = adev->clip_box.p.y = min_int;
    adev->clip_box.q.x = adev->clip_box.q.y = max_int;
    return 0;
}

/* Close the accumulation device. */
private int
accum_close(gx_device * dev)
{
    gx_device_cpath_accum * const adev = (gx_device_cpath_accum *)dev;

    adev->list.xmin = adev->bbox.p.x;
    adev->list.xmax = adev->bbox.q.x;
#ifdef DEBUG
    if (gs_debug_c('q')) {
	gx_clip_rect *rp =
	    (adev->list.count <= 1 ? &adev->list.single : adev->list.head);

	dlprintf6("[q]list at 0x%lx, count=%d, head=0x%lx, tail=0x%lx, xrange=(%d,%d):\n",
		  (ulong) & adev->list, adev->list.count,
		  (ulong) adev->list.head, (ulong) adev->list.tail,
		  adev->list.xmin, adev->list.xmax);
	while (rp != 0) {
	    clip_rect_print('q', "   ", rp);
	    rp = rp->next;
	}
    }
    if (!clip_list_validate(&adev->list)) {
	lprintf1("[q]Bad clip list 0x%lx!\n", (ulong) & adev->list);
	return_error(gs_error_Fatal);
    }
#endif
    return 0;
}

/* Accumulate one rectangle. */
/* Allocate a rectangle to be added to the list. */
static const gx_clip_rect clip_head_rect = {
    0, 0, min_int, min_int, min_int, min_int
};
static const gx_clip_rect clip_tail_rect = {
    0, 0, max_int, max_int, max_int, max_int
};
private gx_clip_rect *
accum_alloc_rect(gx_device_cpath_accum * adev)
{
    gs_memory_t *mem = adev->list_memory;
    gx_clip_rect *ar = gs_alloc_struct(mem, gx_clip_rect, &st_clip_rect,
				       "accum_alloc_rect");

    if (ar == 0)
	return 0;
    if (adev->list.count == 2) {
	/* We're switching from a single rectangle to a list. */
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
	if (tail == 0 || single == 0 || ar == 0) {
	    gs_free_object(mem, ar, "accum_alloc_rect");
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
#define ACCUM_ALLOC(s, ar, px, py, qx, qy)\
	if (++(adev->list.count) == 1)\
	  ar = &adev->list.single;\
	else if ((ar = accum_alloc_rect(adev)) == 0)\
	  return_error(gs_error_VMerror);\
	ACCUM_SET(s, ar, px, py, qx, qy)
#define ACCUM_SET(s, ar, px, py, qx, qy)\
	(ar)->xmin = px, (ar)->ymin = py, (ar)->xmax = qx, (ar)->ymax = qy;\
	clip_rect_print('Q', s, ar)
/* Link or unlink a rectangle in the list. */
#define ACCUM_ADD_LAST(ar)\
	ACCUM_ADD_BEFORE(ar, adev->list.tail)
#define ACCUM_ADD_AFTER(ar, rprev)\
	ar->prev = (rprev), (ar->next = (rprev)->next)->prev = ar,\
	  (rprev)->next = ar
#define ACCUM_ADD_BEFORE(ar, rnext)\
	(ar->prev = (rnext)->prev)->next = ar, ar->next = (rnext),\
	  (rnext)->prev = ar
#define ACCUM_REMOVE(ar)\
	ar->next->prev = ar->prev, ar->prev->next = ar->next
/* Free a rectangle that was removed from the list. */
#define ACCUM_FREE(s, ar)\
	if (--(adev->list.count)) {\
	  clip_rect_print('Q', s, ar);\
	  gs_free_object(adev->list_memory, ar, "accum_rect");\
	}
/*
 * Add a rectangle to the list.  It would be wonderful if rectangles
 * were always disjoint and always presented in the correct order,
 * but they aren't: the fill loop works by trapezoids, not by scan lines,
 * and may produce slightly overlapping rectangles because of "fattening".
 * All we can count on is that they are approximately disjoint and
 * approximately in order.
 *
 * Because of the way the fill loop handles a path that is just a single
 * rectangle, we take special care to merge Y-adjacent rectangles when
 * this is possible.
 */
private int
accum_fill_rectangle(gx_device * dev, int x, int y, int w, int h,
		     gx_color_index color)
{
    gx_device_cpath_accum * const adev = (gx_device_cpath_accum *)dev;
    int xe = x + w, ye = y + h;
    gx_clip_rect *nr;
    gx_clip_rect *ar;
    register gx_clip_rect *rptr;
    int ymin, ymax;

    /* Clip the rectangle being added. */
    if (y < adev->clip_box.p.y)
	y = adev->clip_box.p.y;
    if (ye > adev->clip_box.q.y)
	ye = adev->clip_box.q.y;
    if (y >= ye)
	return 0;
    if (x < adev->clip_box.p.x)
	x = adev->clip_box.p.x;
    if (xe > adev->clip_box.q.x)
	xe = adev->clip_box.q.x;
    if (x >= xe)
	return 0;

    /* Update the bounding box. */
    if (x < adev->bbox.p.x)
	adev->bbox.p.x = x;
    if (y < adev->bbox.p.y)
	adev->bbox.p.y = y;
    if (xe > adev->bbox.q.x)
	adev->bbox.q.x = xe;
    if (ye > adev->bbox.q.y)
	adev->bbox.q.y = ye;

top:
    if (adev->list.count == 0) {	/* very first rectangle */
	adev->list.count = 1;
	ACCUM_SET("single", &adev->list.single, x, y, xe, ye);
	return 0;
    }
    if (adev->list.count == 1) {	/* check for Y merging */
	rptr = &adev->list.single;
	if (x == rptr->xmin && xe == rptr->xmax &&
	    y <= rptr->ymax && ye >= rptr->ymin
	    ) {
	    if (y < rptr->ymin)
		rptr->ymin = y;
	    if (ye > rptr->ymax)
		rptr->ymax = ye;
	    return 0;
	}
    }
    else
	rptr = adev->list.tail->prev;
    if (y >= rptr->ymax) {
	if (y == rptr->ymax && x == rptr->xmin && xe == rptr->xmax &&
	    (rptr->prev == 0 || y != rptr->prev->ymax)
	    ) {
	    rptr->ymax = ye;
	    return 0;
	}
	ACCUM_ALLOC("app.y", nr, x, y, xe, ye);
	ACCUM_ADD_LAST(nr);
	return 0;
    } else if (y == rptr->ymin && ye == rptr->ymax && x >= rptr->xmin) {
	if (x <= rptr->xmax) {
	    if (xe > rptr->xmax)
		rptr->xmax = xe;
	    return 0;
	}
	ACCUM_ALLOC("app.x", nr, x, y, xe, ye);
	ACCUM_ADD_LAST(nr);
	return 0;
    }
    ACCUM_ALLOC("accum", nr, x, y, xe, ye);
    rptr = adev->list.tail->prev;
    /* Work backwards till we find the insertion point. */
    while (ye <= rptr->ymin)
	rptr = rptr->prev;
    ymin = rptr->ymin;
    ymax = rptr->ymax;
    if (ye > ymax) {
	if (y >= ymax) {	/* Insert between two bands. */
	    ACCUM_ADD_AFTER(nr, rptr);
	    return 0;
	}
	/* Split off the top part of the new rectangle. */
	ACCUM_ALLOC("a.top", ar, x, ymax, xe, ye);
	ACCUM_ADD_AFTER(ar, rptr);
	ye = nr->ymax = ymax;
	clip_rect_print('Q', " ymax", nr);
    }
    /* Here we know ymin < ye <= ymax; */
    /* rptr points to the last node with this value of ymin/ymax. */
    /* If necessary, split off the part of the existing band */
    /* that is above the new band. */
    if (ye < ymax) {
	gx_clip_rect *rsplit = rptr;

	while (rsplit->ymax == ymax) {
	    ACCUM_ALLOC("s.top", ar, rsplit->xmin, ye, rsplit->xmax, ymax);
	    ACCUM_ADD_AFTER(ar, rptr);
	    rsplit->ymax = ye;
	    rsplit = rsplit->prev;
	}
	ymax = ye;
    }
    /* Now ye = ymax.  If necessary, split off the part of the */
    /* existing band that is below the new band. */
    if (y > ymin) {
	gx_clip_rect *rbot = rptr, *rsplit;

	while (rbot->prev->ymin == ymin)
	    rbot = rbot->prev;
	for (rsplit = rbot;;) {
	    ACCUM_ALLOC("s.bot", ar, rsplit->xmin, ymin, rsplit->xmax, y);
	    ACCUM_ADD_BEFORE(ar, rbot);
	    rsplit->ymin = y;
	    if (rsplit == rptr)
		break;
	    rsplit = rsplit->next;
	}
	ymin = y;
    }
    /* Now y <= ymin as well.  (y < ymin is possible.) */
    nr->ymin = ymin;
    /* Search for the X insertion point. */
    for (; rptr->ymin == ymin; rptr = rptr->prev) {
	if (xe < rptr->xmin)
	    continue;		/* still too far to right */
	if (x > rptr->xmax)
	    break;		/* disjoint */
	/* The new rectangle overlaps an existing one.  Merge them. */
	if (xe > rptr->xmax) {
	    rptr->xmax = nr->xmax;	/* might be > xe if */
	    /* we already did a merge */
	    clip_rect_print('Q', "widen", rptr);
	}
	ACCUM_FREE("free", nr);
	if (x >= rptr->xmin)
	    goto out;
	/* Might overlap other rectangles to the left. */
	rptr->xmin = x;
	nr = rptr;
	ACCUM_REMOVE(rptr);
	clip_rect_print('Q', "merge", nr);
    }
    ACCUM_ADD_AFTER(nr, rptr);
out:
    /* Check whether there are only 0 or 1 rectangles left. */
    if (adev->list.count <= 1) {
	/* We're switching from a list to at most 1 rectangle. */
	/* Free the head and tail entries. */
	gs_memory_t *mem = adev->list_memory;
	gx_clip_rect *single = adev->list.head->next;

	if (single != adev->list.tail) {
	    adev->list.single = *single;
	    gs_free_object(mem, single, "accum_free_rect(single)");
	    adev->list.single.next = adev->list.single.prev = 0;
	}
	gs_free_object(mem, adev->list.tail, "accum_free_rect(tail)");
	gs_free_object(mem, adev->list.head, "accum_free_rect(head)");
	adev->list.head = 0;
	adev->list.tail = 0;
    }
    /* Check whether there is still more of the new band to process. */
    if (y < ymin) {
	/* Continue with the bottom part of the new rectangle. */
	clip_rect_print('Q', " ymin", nr);
	ye = ymin;
	goto top;
    }
    return 0;
}

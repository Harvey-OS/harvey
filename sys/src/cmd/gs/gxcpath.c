/* Copyright (C) 1991, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gxcpath.c */
/* Implementation of clipping paths */
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gxdevice.h"
#include "gxfixed.h"
#include "gzpath.h"
#include "gzcpath.h"

/* Imported from gxacpath.c */
extern int gx_cpath_intersect_slow(P4(gs_state *, gx_clip_path *,
  gx_path *, int));

/* Forward references */
private void gx_clip_list_from_rectangle(P2(gx_clip_list *, gs_fixed_rect *));
private int gx_clip_list_add_to_path(P2(gx_clip_list *, gx_path *));

/* Structure types */
public_st_clip_rect();
private_st_clip_list();
public_st_clip_path();
public_st_device_clip();

/* GC procedures for gx_clip_path */
#define cptr ((gx_clip_path *)vptr)
private ENUM_PTRS_BEGIN(clip_path_enum_ptrs) ;
	if ( index < st_clip_list_max_ptrs )
	  { gs_ptr_type_t ret = clip_list_enum_ptrs(&cptr->list, sizeof(cptr->list), index, pep);
	    if ( ret == 0 )	/* don't stop early */
	      ret = ptr_struct_type, *pep = 0;
	    return ret;
	  }
	return (*st_path.enum_ptrs)(&cptr->path, sizeof(cptr->path), index - st_clip_list_max_ptrs, pep);
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(clip_path_reloc_ptrs) {
	clip_list_reloc_ptrs(&cptr->list, sizeof(gx_clip_list), gcst);
	(*st_path.reloc_ptrs)(&cptr->path, sizeof(gx_path), gcst);
} RELOC_PTRS_END
#undef cptr

/* GC procedures for gx_device_clip */
#define cptr ((gx_device_clip *)vptr)
private ENUM_PTRS_BEGIN(device_clip_enum_ptrs) {
	if ( index < st_clip_list_max_ptrs + 1 )
	  return clip_list_enum_ptrs(&cptr->list, sizeof(gx_clip_list),
				     index - 1, pep);
	return (*st_device_forward.enum_ptrs)(vptr, sizeof(gx_device_forward),
					      index - (st_clip_list_max_ptrs + 1), pep);
	}
	case 0:
		*pep = (cptr->current == &cptr->list.single ? NULL :
			(void *)cptr->current);
		break;
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(device_clip_reloc_ptrs) {
	if ( cptr->current == &cptr->list.single )
	  cptr->current =
	    &((gx_device_clip *)gs_reloc_struct_ptr(vptr, gcst))->list.single;
	else
	  RELOC_PTR(gx_device_clip, current);
	clip_list_reloc_ptrs(&cptr->list, sizeof(gx_clip_list), gcst);
	(*st_device_forward.reloc_ptrs)(vptr, sizeof(gx_device_forward), gcst);
} RELOC_PTRS_END
#undef cptr

/* Define an empty clip list. */
private const gx_clip_list clip_list_empty =
{  { 0, 0, min_int, max_int, 0, 0 },
   0, 0, 0
};

/* Debugging */

#ifdef DEBUG
/* Validate a clipping path. */
bool		/* only exported for gxacpath.c */
clip_list_validate(gx_clip_list *clp)
{	if ( clp->count <= 1 )
	  return (clp->head == 0 && clp->tail == 0 &&
		  clp->single.next == 0 && clp->single.prev == 0);
	else
	  {	gx_clip_rect *prev = clp->head;
		gx_clip_rect *ptr;
		bool ok = true;
		while ( (ptr = prev->next) != 0 )
		  { if ( ptr->ymin > ptr->ymax || ptr->xmin > ptr->xmax ||
			 !(ptr->ymin >= prev->ymax ||
			   (ptr->ymin == prev->ymin &&
			    ptr->ymax == prev->ymax &&
			    ptr->xmin >= prev->xmax)) ||
			 ptr->prev != prev
		       )
		      { clip_rect_print('q', "WRONG:", ptr);
			ok = false;
		      }
		    prev = ptr;
		  }
		return ok && prev == clp->tail;
	  }
}
#endif

/* ------ Clipping path accessing ------ */

/* Initialize a clipping path. */
int
gx_cpath_init(gx_clip_path *pcpath, gs_memory_t *mem)
{	static /*const*/ gs_fixed_rect null_rect = { { 0, 0 }, { 0, 0 } };
	return gx_cpath_from_rectangle(pcpath, &null_rect, mem); /* does a gx_path_init */
}

/* Return the path of a clipping path. */
int
gx_cpath_path(gx_clip_path *pcpath, gx_path *ppath)
{	if ( !pcpath->segments_valid )
	{	int code = gx_clip_list_add_to_path(&pcpath->list, &pcpath->path);
		if ( code < 0 ) return code;
		pcpath->segments_valid = 1;
	}
	*ppath = pcpath->path;
	return 0;
}

/* Return the inner and outer check rectangles for a clipping path. */
int
gx_cpath_inner_box(const gx_clip_path *pcpath, gs_fixed_rect *pbox)
{	*pbox = pcpath->inner_box;
	return 0;
}
int
gx_cpath_outer_box(const gx_clip_path *pcpath, gs_fixed_rect *pbox)
{	*pbox = pcpath->outer_box;
	return 0;
}

/* Test if a clipping path includes a rectangle. */
/* The rectangle need not be oriented correctly, i.e. x0 > x1 is OK. */
int
gx_cpath_includes_rectangle(register const gx_clip_path *pcpath,
  fixed x0, fixed y0, fixed x1, fixed y1)
{	return
	  (x0 <= x1 ?
	    (pcpath->inner_box.p.x <= x0 && x1 <= pcpath->inner_box.q.x) :
	    (pcpath->inner_box.p.x <= x1 && x0 <= pcpath->inner_box.q.x)) &&
	  (y0 <= y1 ?
	    (pcpath->inner_box.p.y <= y0 && y1 <= pcpath->inner_box.q.y) :
	    (pcpath->inner_box.p.y <= y1 && y0 <= pcpath->inner_box.q.y));
}

/* Release a clipping path. */
void
gx_cpath_release(gx_clip_path *pcpath)
{	if ( !pcpath->shares_list )
		gx_clip_list_free(&pcpath->list, pcpath->path.memory);
	gx_path_release(&pcpath->path);
}

/* Share a clipping path. */
void
gx_cpath_share(gx_clip_path *pcpath)
{	gx_path_share(&pcpath->path);
	pcpath->shares_list = 1;
}

/* Set the outer clipping box to the path bounding box, */
/* expanded to pixel boundaries. */
void
gx_cpath_set_outer_box(gx_clip_path *pcpath)
{	pcpath->outer_box.p.x = fixed_floor(pcpath->path.bbox.p.x);
	pcpath->outer_box.p.y = fixed_floor(pcpath->path.bbox.p.y);
	pcpath->outer_box.q.x = fixed_ceiling(pcpath->path.bbox.q.x);
	pcpath->outer_box.q.y = fixed_ceiling(pcpath->path.bbox.q.y);
}

/* Create a rectangular clipping path. */
/* The supplied rectangle may not be oriented correctly, */
/* but it will be oriented correctly upon return. */
int
gx_cpath_from_rectangle(gx_clip_path *pcpath, gs_fixed_rect *pbox,
  gs_memory_t *mem)
{	gx_clip_list_from_rectangle(&pcpath->list, pbox);
	pcpath->inner_box = *pbox;
	pcpath->segments_valid = 0;
	pcpath->shares_list = 0;
	gx_path_init(&pcpath->path, mem);
	pcpath->path.bbox = *pbox;
	gx_cpath_set_outer_box(pcpath);
	return 0;
}

/* Intersect a new clipping path with an old one. */
/* Note that it may overwrite its path argument; return 1 in this case, */
/* otherwise 0 for success, <0 for failure as usual. */
int
gx_cpath_intersect(gs_state *pgs, gx_clip_path *pcpath, gx_path *ppath,
  int rule)
{	gs_fixed_rect old_box, new_box;
	int code;
	if ( gx_cpath_is_rectangle(pcpath, &old_box) &&
	    gx_path_is_rectangle(ppath, &new_box)
	   )
	   {	int changed = 0;
		/* Intersect the two rectangles if necessary. */
		if ( old_box.p.x > new_box.p.x )
			new_box.p.x = old_box.p.x, changed = 1;
		if ( old_box.p.y > new_box.p.y )
			new_box.p.y = old_box.p.y, changed = 1;
		if ( old_box.q.x < new_box.q.x )
			new_box.q.x = old_box.q.x, changed = 1;
		if ( old_box.q.y < new_box.q.y )
			new_box.q.y = old_box.q.y, changed = 1;
		if ( changed )
		   {	/* Store the new rectangle back into the new path. */
			register segment *pseg =
				(segment *)ppath->first_subpath;
#define set_pt(pqx,pqy)\
  pseg->pt.x = new_box.pqx.x, pseg->pt.y = new_box.pqy.y
			set_pt(p, p); pseg = pseg->next;
			set_pt(q, p); pseg = pseg->next;
			set_pt(q, q); pseg = pseg->next;
			set_pt(p, q); pseg = pseg->next;
			if ( pseg != 0 ) /* might be an open rectangle */
			  set_pt(p, p);
#undef set_pt
		   }
		ppath->bbox = new_box;
		gx_clip_list_from_rectangle(&pcpath->list, &new_box);
		pcpath->inner_box = new_box;
		pcpath->path = *ppath;
		gx_cpath_set_outer_box(pcpath);
		pcpath->segments_valid = 1;
		code = 1;
	   }
	else
	   {	/* Not a rectangle.  Intersect the slow way. */
		code = gx_cpath_intersect_slow(pgs, pcpath, ppath, rule);
	   }
	return code;
}

/* Scale a clipping path by a power of 2. */
int
gx_cpath_scale_exp2(gx_clip_path *pcpath, int log2_scale_x, int log2_scale_y)
{	int code =
	  gx_path_scale_exp2(&pcpath->path, log2_scale_x, log2_scale_y);
	gx_clip_rect *pr;
	if ( code < 0 )
	  return code;
	/* Scale the fixed entries. */
	gx_rect_scale_exp2(&pcpath->inner_box, log2_scale_x, log2_scale_y);
	gx_rect_scale_exp2(&pcpath->outer_box, log2_scale_x, log2_scale_y);
	/* Scale the clipping list. */
	pr = pcpath->list.head;
	if ( pr == 0 )
	  pr = &pcpath->list.single;
	for ( ; pr != 0; pr = pr->next )
	  if ( pr != pcpath->list.head && pr != pcpath->list.tail )
	    {
#define scale_v(v, s)\
  if ( pr->v != min_int && pr->v != max_int )\
    pr->v = (s >= 0 ? pr->v << s : pr->v >> -s)
		scale_v(xmin, log2_scale_x);
		scale_v(xmax, log2_scale_x);
		scale_v(ymin, log2_scale_y);
		scale_v(ymax, log2_scale_y);
#undef scale_v
	    }
	return 0;
}

/* ------ Clipping list routines ------ */

/* Initialize a clip list. */
void
gx_clip_list_init(gx_clip_list *clp)
{	*clp = clip_list_empty;
}

/* Initialize a clip list to a rectangle. */
/* The supplied rectangle may not be oriented correctly, */
/* but it will be oriented correctly upon return. */
private void
gx_clip_list_from_rectangle(register gx_clip_list *clp,
  register gs_fixed_rect *rp)
{	gx_clip_list_init(clp);
	if ( rp->p.x > rp->q.x )
	  { fixed t = rp->p.x; rp->p.x = rp->q.x; rp->q.x = t; }
	if ( rp->p.y > rp->q.y )
	  { fixed t = rp->p.y; rp->p.y = rp->q.y; rp->q.y = t; }
	clp->single.xmin = fixed2int_var(rp->p.x);
	clp->single.ymin = fixed2int_var(rp->p.y);
	clp->single.xmax = fixed2int_var_ceiling(rp->q.x);
	clp->single.ymax = fixed2int_var_ceiling(rp->q.y);
	clp->count = 1;
}

/* Add a clip list to a path. */
/* The current implementation is very inefficient. */
private int
gx_clip_list_add_to_path(gx_clip_list *clp, gx_path *ppath)
{	const gx_clip_rect *rp;
	int code = -1;
	for ( rp = (clp->count <= 1 ? &clp->single : clp->head); rp != 0;
	      rp = rp->next
	    )
	{	if ( rp->xmin < rp->xmax && rp->ymin < rp->ymax )
		{	code = gx_path_add_rectangle(ppath,
					int2fixed(rp->xmin),
					int2fixed(rp->ymin),
					int2fixed(rp->xmax),
					int2fixed(rp->ymax));
			if ( code < 0 )
				return code;
		}
	}
	if ( code < 0 )
	{	/* We didn't have any rectangles. */
		code = gx_path_add_point(ppath, fixed_0, fixed_0);
	}
	return code;
}

/* Free a clip list. */
void
gx_clip_list_free(gx_clip_list *clp, gs_memory_t *mem)
{	gx_clip_rect *rp = clp->tail;
	while ( rp != 0 )
	{	gx_clip_rect *prev = rp->prev;
		gs_free_object(mem, rp, "gx_clip_list_free");
		rp = prev;
	}
	gx_clip_list_init(clp);
}

/* ------ Rectangle list clipper ------ */

/* Device for clipping with a region. */
private dev_proc_open_device(clip_open);
private dev_proc_fill_rectangle(clip_fill_rectangle);
private dev_proc_tile_rectangle(clip_tile_rectangle);
private dev_proc_copy_mono(clip_copy_mono);
private dev_proc_copy_color(clip_copy_color);
private dev_proc_get_bits(clip_get_bits);
private dev_proc_copy_alpha(clip_copy_alpha);

/* The device descriptor. */
private const gx_device_clip gs_clip_device =
{	std_device_std_body(gx_device_clip, 0, "clipper",
	  0, 0, 1, 1),
	{	clip_open,
		gx_forward_get_initial_matrix,
		gx_default_sync_output,
		gx_default_output_page,
		gx_default_close_device,
		gx_forward_map_rgb_color,
		gx_forward_map_color_rgb,
		clip_fill_rectangle,
		clip_tile_rectangle,
		clip_copy_mono,
		clip_copy_color,
		gx_default_draw_line,
		clip_get_bits,
		gx_forward_get_params,
		gx_forward_put_params,
		gx_forward_map_cmyk_color,
		gx_forward_get_xfont_procs,
		gx_forward_get_xfont_device,
		gx_forward_map_rgb_alpha_color,
		gx_forward_get_page_device,
		gx_forward_get_alpha_bits,
		clip_copy_alpha
	}
};
#define rdev ((gx_device_clip *)dev)

/* Make a clipping device. */
void
gx_make_clip_device(gx_device_clip *dev, void *container,
  const gx_clip_list *list)
{	*dev = gs_clip_device;
	dev->list = *list;
}

/* Declare and initialize the cursor variables. */
#ifdef DEBUG
private ulong clip_loops, clip_in, clip_down, clip_up, clip_x, clip_no_x;
private uint clip_interval = 100;
# define inc(v) v++
# define print_clip()\
    if ( clip_loops % clip_interval == 0 )\
      if_debug10('q', "[q]rect=(%d,%d),(%d,%d)\n     loops=%ld in=%ld down=%ld up=%ld x=%ld no_x=%ld\n",\
		 x, y, x + w, y + h,\
		 clip_loops, clip_in, clip_down, clip_up, clip_x, clip_no_x)
#else
# define inc(v) 0
# define print_clip() DO_NOTHING
#endif
#define DECLARE_CLIP\
  register gx_clip_rect *rptr = rdev->current;\
  gx_device *tdev = rdev->target;
/* Check whether the rectangle x,y,w,h falls within the current entry. */
#define xywh_is_in_ryptr()\
  (y >= rptr->ymin && y + h <= rptr->ymax &&\
   x >= rptr->xmin && x + w <= rptr->xmax)
#ifdef DEBUG
#  define xywh_in_ryptr() (xywh_is_in_ryptr() ? (inc(clip_in), 1) : 0)
#else
#  define xywh_in_ryptr() xywh_is_in_ryptr()
#endif
/*
 * Warp the cursor forward or backward to the first rectangle row that
 * could include a given y value.  Assumes rptr is set, and updates it.
 * Specifically, after warp_cursor, y < rptr->ymax and either
 * rptr->prev == 0 or y >= rptr->prev->ymax.  Note that y <= rptr->ymin
 * is possible.
 */
#define warp_cursor(y)\
  if ( (y) >= rptr->ymax )\
   { if ( (rptr = rptr->next) == 0 ) return 0;\
     while ( inc(clip_up), (y) >= rptr->ymax ) rptr = rptr->next;\
   }\
  else while ( rptr->prev != 0 && (y) < rptr->prev->ymax )\
   { inc(clip_down); rptr = rptr->prev; }
/*
 * Enumerate the rectangles of the x,w,y,h argument that fall within
 * the clipping region.  Usage:
 *	BEGIN_CLIP
 *		(adjust for yc > y if necessary)
 *	FOR_CLIP
 *		... xc, yc, xec, yec ... [must be an expression statement]
 *	NEXT_CLIP
 *		(about to set yc to yec)
 *	END_CLIP
 */
#define BEGIN_CLIP\
	if ( w <= 0 || h <= 0 ) return 0;\
	inc(clip_loops);\
   {	int yc;\
	const int xe = x + w, ye = y + h;\
	warp_cursor(y);\
	rdev->current = rptr;\
	yc = rptr->ymin;\
	if ( yc < y ) yc = y;\
	else if ( yc >= ye ) return 0;
#define FOR_CLIP\
	for ( ; ; )\
	   {	const int ymax = rptr->ymax;\
		int yec = ymax;\
		if ( yec > ye ) yec = ye;\
		if_debug2('Q', "[Q]yc=%d yec=%d\n", yc, yec);\
		do \
		   {	int xc = rptr->xmin;\
			int xec = rptr->xmax;\
			if ( xc < x ) xc = x;\
			if ( xec > xe ) xec = xe;\
			if ( xec > xc )\
			   {	int code;\
				clip_rect_print('Q', "match", rptr);\
				if_debug2('Q', "[Q]xc=%d xec=%d\n", xc, xec);\
				inc(clip_x);\
				code =
#define NEXT_CLIP\
				if ( code < 0 ) return code;\
			   }\
			else inc(clip_no_x);\
		   }\
		while ( (rptr = rptr->next) != 0 && rptr->ymax == ymax );\
		if ( rptr == 0 || (yec = rptr->ymin) >= ye ) break;
#define END_CLIP\
		yc = yec;\
	   }\
	print_clip();\
   }

/* Open a clipping device */
private int
clip_open(register gx_device *dev)
{	gx_device *tdev = rdev->target;
	/* Initialize the cursor. */
	rdev->current =
	  (rdev->list.head == 0 ? &rdev->list.single : rdev->list.head);
	rdev->color_info = tdev->color_info;
	dev->cached = tdev->cached;	/* copy cached colors */
	return 0;
}

/* Fill a rectangle */
private int
clip_fill_rectangle(gx_device *dev, int x, int y, int w, int h,
  gx_color_index color)
{	DECLARE_CLIP
	dev_proc_fill_rectangle((*fill)) = dev_proc(tdev, fill_rectangle);
	if ( xywh_in_ryptr() )
		return (*fill)(tdev, x, y, w, h, color);
	BEGIN_CLIP
	FOR_CLIP
		(*fill)(tdev, xc, yc, xec - xc, yec - yc, color);
	NEXT_CLIP
	END_CLIP
	return 0;
}

/* Tile a rectangle */
private int
clip_tile_rectangle(gx_device *dev, const gx_tile_bitmap *tile,
  int x, int y, int w, int h,
  gx_color_index color0, gx_color_index color1, int phase_x, int phase_y)
{	DECLARE_CLIP
	dev_proc_tile_rectangle((*fill)) = dev_proc(tdev, tile_rectangle);
	if ( xywh_in_ryptr() )
		return (*fill)(tdev, tile, x, y, w, h, color0, color1, phase_x, phase_y);
	BEGIN_CLIP
	FOR_CLIP
		(*fill)(tdev, tile, xc, yc, xec - xc, yec - yc, color0, color1, phase_x, phase_y);
	NEXT_CLIP
	END_CLIP
	return 0;
}

/* Copy a monochrome rectangle */
private int
clip_copy_mono(gx_device *dev,
  const byte *data, int sourcex, int raster, gx_bitmap_id id,
  int x, int y, int w, int h,
  gx_color_index color0, gx_color_index color1)
{	DECLARE_CLIP
	dev_proc_copy_mono((*copy)) = dev_proc(tdev, copy_mono);
	if ( xywh_in_ryptr() )
		return (*copy)(tdev, data, sourcex, raster, id, x, y, w, h, color0, color1);
	BEGIN_CLIP
		if ( yc > y ) data += (yc - y) * raster;
	FOR_CLIP
		(*copy)(tdev, data, sourcex + xc - x, raster, gx_no_bitmap_id, xc, yc, xec - xc, yec - yc, color0, color1);
	NEXT_CLIP
		data += (yec - yc) * raster;
	END_CLIP
	return 0;
}

/* Copy a color rectangle */
private int
clip_copy_color(gx_device *dev,
  const byte *data, int sourcex, int raster, gx_bitmap_id id,
  int x, int y, int w, int h)
{	DECLARE_CLIP
	dev_proc_copy_color((*copy)) = dev_proc(tdev, copy_color);
	if ( xywh_in_ryptr() )
		return (*copy)(tdev, data, sourcex, raster, id, x, y, w, h);
	BEGIN_CLIP
		if ( yc > y ) data += (yc - y) * raster;
	FOR_CLIP
		(*copy)(tdev, data, sourcex + xc - x, raster, gx_no_bitmap_id, xc, yc, xec - xc, yec - yc);
	NEXT_CLIP
		data += (yec - yc) * raster;
	END_CLIP
	return 0;
}

/* Copy a rectangle with alpha */
private int
clip_copy_alpha(gx_device *dev,
  const byte *data, int sourcex, int raster, gx_bitmap_id id,
  int x, int y, int w, int h,
  gx_color_index color, int depth)
{	DECLARE_CLIP
	dev_proc_copy_alpha((*copy)) = dev_proc(tdev, copy_alpha);
	if ( xywh_in_ryptr() )
		return (*copy)(tdev, data, sourcex, raster, id, x, y, w, h, color, depth);
	BEGIN_CLIP
		if ( yc > y ) data += (yc - y) * raster;
	FOR_CLIP
		(*copy)(tdev, data, sourcex + xc - x, raster, gx_no_bitmap_id, xc, yc, xec - xc, yec - yc, color, depth);
	NEXT_CLIP
		data += (yec - yc) * raster;
	END_CLIP
	return 0;
}

/* Get bits back from the device. */
private int
clip_get_bits(gx_device *dev, int y, byte *data, byte **actual_data)
{	gx_device *tdev = rdev->target;
	return (*dev_proc(tdev, get_bits))(tdev, y, data, actual_data);
}

/* ------ Debugging printout ------ */

#ifdef DEBUG

/* Print a clipping path */
void
gx_cpath_print(const gx_clip_path *pcpath)
{	const gx_clip_rect *pr;
	if ( pcpath->segments_valid )
		gx_path_print(&pcpath->path);
	else
		dputs("   (segments not valid)\n");
	dprintf4("   inner_box=(%g,%g),(%g,%g)\n",
		 fixed2float(pcpath->inner_box.p.x),
		 fixed2float(pcpath->inner_box.p.y),
		 fixed2float(pcpath->inner_box.q.x),
		 fixed2float(pcpath->inner_box.q.y));
	dprintf5("     outer_box=(%g,%g),(%g,%g) count=%d\n",
		 fixed2float(pcpath->outer_box.p.x),
		 fixed2float(pcpath->outer_box.p.y),
		 fixed2float(pcpath->outer_box.q.x),
		 fixed2float(pcpath->outer_box.q.y),
		 pcpath->list.count);
	switch ( pcpath->list.count )
	{
	case 0: pr = 0; break;
	case 1: pr = &pcpath->list.single; break;
	default: pr = pcpath->list.head;
	}
	for ( ; pr != 0; pr = pr->next )
		dprintf4("   rect: (%d,%d),(%d,%d)\n",
			 pr->xmin, pr->ymin, pr->xmax, pr->ymax);
}

#endif					/* DEBUG */

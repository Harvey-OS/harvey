/* Copyright (C) 1991 Aladdin Enterprises.  All rights reserved.
  
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

/* gxcpath.h */
/* Interface to clipping devices */
/* requires gzpath.h */

/* We expose the implementation of clipping lists so that clients */
/* can allocate clipping lists or devices on the stack. */

/*
 * For clipping, a path is represented as a list of rectangles.
 * Normally, a path is created as a list of segments;
 * installing it as a clipping path creates the rectangle list.
 * However, when the clipping path originates in some other way
 * (e.g., from initclip, or for clipping a cached character),
 * or if it is a non-trivial intersection of two paths,
 * the resulting clipping path exists only as a rectangle list;
 * clippath constructs the segment representation if needed.
 * Note that even if the path only exists as a rectangle list,
 * its bounding box (path.bbox) is still correct.
 */

/*
 * Rectangle list structure.
 * Consecutive gx_clip_rect entries either have the same Y values,
 * or ymin of this entry >= ymax of the previous entry.
 */
typedef struct gx_clip_rect_s gx_clip_rect;
struct gx_clip_rect_s {
	gx_clip_rect *next, *prev;
	int ymin, ymax;			/* ymax > ymin */
	int xmin, xmax;			/* xmax > xmin */
};
/* The descriptor is public only for gxacpath.c. */
extern_st(st_clip_rect);
#define public_st_clip_rect()	/* in gxcpath.c */\
  gs_public_st_ptrs2(st_clip_rect, gx_clip_rect, "clip_rect",\
    clip_rect_enum_ptrs, clip_rect_reloc_ptrs, next, prev)
#define st_clip_rect_max_ptrs 2

/*
 * A clip list may consist either of a single rectangle,
 * with null head and tail, or a list of rectangles.  In the latter case,
 * there is a dummy head entry with p.x = q.x to cover Y values
 * starting at min_int, and a dummy tail entry to cover Y values
 * ending at max_int.  This eliminates the need for end tests.
 */
struct gx_clip_list_s {
	gx_clip_rect single;		/* (has next = prev = 0) */
	gx_clip_rect *head;
	gx_clip_rect *tail;
	int count;			/* # of rectangles not counting */
					/* head or tail */
};
#define private_st_clip_list()	/* in gxcpath.c */\
  gs_private_st_ptrs2(st_clip_list, gx_clip_list, "clip_list",\
    clip_list_enum_ptrs, clip_list_reloc_ptrs, head, tail)
#define st_clip_list_max_ptrs 2	/* head, tail */
#define clip_list_is_rectangle(clp) ((clp)->count <= 1)

/* Device for clipping with a rectangle list. */
typedef struct gx_device_clip_s {
	gx_device_forward_common;	/* target is set by client */
	gx_clip_list list;		/* set by client */
	gx_clip_rect *current;		/* cursor in list */
} gx_device_clip;
extern_st(st_device_clip);
#define public_st_device_clip()	/* in gxcpath.c */\
  gs_public_st_composite(st_device_clip, gx_device_clip,\
    "gx_device_clip", device_clip_enum_ptrs, device_clip_reloc_ptrs)
void gx_make_clip_device(P3(gx_device_clip *dev, void *container,
  const gx_clip_list *list));

#define clip_rect_print(ch, str, ar)\
  if_debug7(ch, "[%c]%s 0x%lx: (%d,%d),(%d,%d)\n", ch, str, (ulong)ar,\
	    (ar)->xmin, (ar)->ymin, (ar)->xmax, (ar)->ymax)

/* Routines exported from gxcpath.c for gxacpath.c */

/* Initialize a clip list. */
void gx_clip_list_init(P1(gx_clip_list *));

/* Free a clip list. */
void gx_clip_list_free(P2(gx_clip_list *, gs_memory_t *));

/* Set the outer box for a clipping path from its bounding box. */
void gx_cpath_set_outer_box(P1(gx_clip_path *));

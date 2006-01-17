/* Copyright (C) 1991, 1995, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxcpath.h,v 1.5 2002/06/16 08:45:43 lpd Exp $ */
/* Definitions for clipping lists and devices */
/* Requires gxdevice.h */

#ifndef gxcpath_INCLUDED
#  define gxcpath_INCLUDED

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
    int ymin, ymax;		/* ymax > ymin */
    int xmin, xmax;		/* xmax > xmin */
    byte to_visit;		/* bookkeeping for gs_clippath */
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
#ifndef gx_clip_list_DEFINED
#  define gx_clip_list_DEFINED
typedef struct gx_clip_list_s gx_clip_list;
#endif
struct gx_clip_list_s {
    gx_clip_rect single;	/* (has next = prev = 0) */
    gx_clip_rect *head;
    gx_clip_rect *tail;
    int xmin, xmax;		/* min and max X over all but head/tail */
    int count;			/* # of rectangles not counting */
				/* head or tail */
};

#define private_st_clip_list()	/* in gxcpath.c */\
  gs_private_st_ptrs2(st_clip_list, gx_clip_list, "clip_list",\
    clip_list_enum_ptrs, clip_list_reloc_ptrs, head, tail)
#define st_clip_list_max_ptrs 2	/* head, tail */
#define clip_list_is_rectangle(clp) ((clp)->count <= 1)

/*
 * Clipping devices provide for translation before clipping.
 * This ability, a late addition, currently is used only in a few
 * situations that require breaking up a transfer into pieces,
 * but we suspect it could be used more widely.
 *
 * Note that clipping devices cache their clipping box, so the target's
 * clipping box and the clip list must be const after the clipping device
 * is opened.
 */
#ifndef gx_device_clip_DEFINED
#  define gx_device_clip_DEFINED
typedef struct gx_device_clip_s gx_device_clip;
#endif
struct gx_device_clip_s {
    gx_device_forward_common;	/* target is set by client */
    gx_clip_list list;		/* set by client */
    gx_clip_rect *current;	/* cursor in list */
    gs_int_point translation;
    gs_fixed_rect clipping_box;
    bool clipping_box_set;
};

extern_st(st_device_clip);
#define public_st_device_clip()	/* in gxcpath.c */\
  gs_public_st_composite_use_final(st_device_clip, gx_device_clip,\
    "gx_device_clip", device_clip_enum_ptrs, device_clip_reloc_ptrs,\
    gx_device_finalize)
void gx_make_clip_translate_device(gx_device_clip * dev,
				   const gx_clip_list * list,
				   int tx, int ty, gs_memory_t *mem);

#define gx_make_clip_device(dev, list)\
  gx_make_clip_translate_device(dev, list, 0, 0, NULL)
void gx_make_clip_path_device(gx_device_clip *, const gx_clip_path *);

#define clip_rect_print(ch, str, ar)\
  if_debug7(ch, "[%c]%s 0x%lx: (%d,%d),(%d,%d)\n", ch, str, (ulong)ar,\
	    (ar)->xmin, (ar)->ymin, (ar)->xmax, (ar)->ymax)

/* Exported by gxcpath.c for gxacpath.c */

/* Initialize a clip list. */
void gx_clip_list_init(gx_clip_list *);

/* Free a clip list. */
void gx_clip_list_free(gx_clip_list *, gs_memory_t *);

/* Set the outer box for a clipping path from its bounding box. */
void gx_cpath_set_outer_box(gx_clip_path *);

/* Exported by gxcpath.c for gxclip.c */

/* Return the rectangle list of a clipping path (for local use only). */
const gx_clip_list *gx_cpath_list(const gx_clip_path *pcpath);

#endif /* gxcpath_INCLUDED */

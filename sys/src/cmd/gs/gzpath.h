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

/* gzpath.h */
/* Private representation of paths for Ghostscript library */
/* Requires gxfixed.h */
#include "gxpath.h"
#include "gsstruct.h"			/* for extern_st */

/* Paths are represented as a linked list of line or curve segments, */
/* similar to what pathforall reports. */

/* Definition of a path segment: a segment start, a line, */
/* or a Bezier curve. */
typedef enum {
	s_start,
	s_line,
	s_line_close,
	s_curve
} segment_type;
#define segment_common\
	segment *prev;\
	segment *next;\
	segment_type type;\
	gs_fixed_point pt;		/* initial point for starts, */\
					/* final point for others */
/* Forward declarations for structure types */
#ifndef segment_DEFINED
#  define segment_DEFINED
typedef struct segment_s segment;
#endif
typedef struct subpath_s subpath;

/* A generic segment.  This is never instantiated, */
/* but we define a descriptor anyway for the benefit of subclasses. */
struct segment_s {
	segment_common
};
#define private_st_segment()	/* in gxpath.c */\
  gs_private_st_ptrs2(st_segment, struct segment_s, "segment",\
    segment_enum_ptrs, segment_reloc_ptrs, prev, next)

/* Line segments have no special data. */
typedef struct {
	segment_common
} line_segment;
#define private_st_line()	/* in gxpath.c */\
  gs_private_st_suffix_add0(st_line, line_segment, "line",\
    segment_enum_ptrs, segment_reloc_ptrs)

/* Line_close segments are for the lines appended by closepath. */
/* They point back to the subpath being closed. */
typedef struct {
	segment_common
	subpath *sub;
} line_close_segment;
#define private_st_line_close()	/* in gxpath.c */\
  gs_private_st_suffix_add1(st_line_close, line_close_segment, "close",\
    close_enum_ptrs, close_reloc_ptrs, st_segment, sub)

/* Curve segments store the control points, not the coefficients. */
/* We may want to change this someday. */
typedef struct {
	segment_common
	gs_fixed_point p1, p2;
} curve_segment;
#define private_st_curve()	/* in gxpath.c */\
  gs_private_st_composite_only(st_curve, curve_segment, "curve",\
    segment_enum_ptrs, segment_reloc_ptrs)

/* A start segment.  This serves as the head of a subpath. */
/* The closer is only used temporarily when filling, */
/* to close an open subpath. */
struct subpath_s {
	segment_common
	segment *last;			/* last segment of subpath, */
					/* points back to here if empty */
	int curve_count;		/* # of curves */
	line_close_segment closer;
	char/*bool*/ is_closed;		/* true if subpath is closed */
};
#define private_st_subpath()	/* in gxpath.c */\
  gs_private_st_suffix_add1(st_subpath, subpath, "subpath",\
    subpath_enum_ptrs, subpath_reloc_ptrs, st_segment, last)

/* Define a structure that can hold any kind of segment. */
/* (We never allocate these in heap memory.) */
typedef union union_segment_s {
	segment common;
	subpath start;
	line_segment line;
	line_close_segment line_close;
	curve_segment curve;
} union_segment;

/*
 * Here is the actual structure of a path.
 * The path state reflects the most recent operation on the path as follows:
 *	Operation	position_valid	subpath_open
 *	newpath		0		0
 *	moveto		1		-1
 *	lineto/curveto	1		1
 *	closepath	1		0
 */
struct gx_path_s {
	gs_memory_t *memory;
	gs_fixed_rect bbox;		/* bounding box (in device space) */
	segment *box_last;		/* bbox incorporates segments */
					/* up to & including this one */
	subpath *first_subpath;
	subpath *current_subpath;
	int subpath_count;
	int curve_count;
	gs_fixed_point position;	/* current position */
	int subpath_open;		/* 0 = newpath or closepath, */
					/* -1 = moveto, 1 = lineto/curveto */
	char/*bool*/ position_valid;
	char/*bool*/ bbox_set;		/* true if setbbox is in effect */
	char/*bool*/ shares_segments;	/* if true, this path shares its */
					/* segment storage with the one in */
					/* the previous saved graphics state */
};
extern_st(st_path);
#define public_st_path()	/* in gxpath.c */\
  gs_public_st_ptrs3(st_path, gx_path, "path",\
    path_enum_ptrs, path_reloc_ptrs, box_last, first_subpath, current_subpath)
#define st_path_max_ptrs 3

/* Macros equivalent to a few heavily used procedures. */
/* Be aware that these macros may evaluate arguments more than once. */
#define gx_path_current_point_inline(ppath,ppt)\
 ( !ppath->position_valid ? gs_note_error(gs_error_nocurrentpoint) :\
   ((ppt)->x = ppath->position.x, (ppt)->y = ppath->position.y, 0) )
/* ...rel_point rather than ...relative_point is because */
/* some compilers dislike identifiers of >31 characters. */
#define gx_path_add_rel_point_inline(ppath,dx,dy)\
 ( !ppath->position_valid || ppath->bbox_set ?\
   gx_path_add_relative_point(ppath, dx, dy) :\
   (ppath->position.x += dx, ppath->position.y += dy,\
    ppath->subpath_open = 0) )

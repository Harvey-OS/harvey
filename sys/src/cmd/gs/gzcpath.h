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

/* gzcpath.h */
/* Private representation of clipping paths for Ghostscript library */
/* Requires gzpath.h. */
#include "gxcpath.h"

/* gx_clip_path is a 'subclass' of gx_path. */
struct gx_clip_path_s {
	gx_path path;
		/* Anything within the inner_box is guaranteed to fall */
		/* entirely within the clipping path. */
	gs_fixed_rect inner_box;
		/* Anything outside the outer_box is guaranteed to fall */
		/* entirely outside the clipping path.  This is the same */
		/* as the path bounding box, widened to pixel boundaries. */
	gs_fixed_rect outer_box;
	gx_clip_list list;
	char segments_valid;		/* segment representation is valid */
	char shares_list;		/* if true, this path shares its */
					/* clip list storage with the one in */
					/* the previous saved graphics state */
};
extern_st(st_clip_path);
#define public_st_clip_path()	/* in gxcpath.c */\
  gs_public_st_composite(st_clip_path, gx_clip_path, "clip_path",\
    clip_path_enum_ptrs, clip_path_reloc_ptrs)
#define st_clip_path_max_ptrs (st_path_max_ptrs + st_clip_list_max_ptrs)
#define gx_cpath_is_rectangle(pcpath, pbox)\
  (clip_list_is_rectangle(&(pcpath)->list) ?\
   (*(pbox) = (pcpath)->inner_box, 1) : 0)

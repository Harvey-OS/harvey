/* Copyright (C) 1989, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: gzstate.h,v 1.5 2001/03/13 00:41:10 raph Exp $ */
/* Private graphics state definition for Ghostscript library */

#ifndef gzstate_INCLUDED
#  define gzstate_INCLUDED

#include "gscpm.h"
#include "gscspace.h"
#include "gsrefct.h"
#include "gxdcolor.h"
#include "gxistate.h"
#include "gsstate.h"
#include "gxstate.h"

/* Opaque types referenced by the graphics state. */
#ifndef gx_path_DEFINED
#  define gx_path_DEFINED
typedef struct gx_path_s gx_path;
#endif
#ifndef gx_clip_path_DEFINED
#  define gx_clip_path_DEFINED
typedef struct gx_clip_path_s gx_clip_path;
#endif
#ifndef gx_clip_stack_DEFINED
#  define gx_clip_stack_DEFINED
typedef struct gx_clip_stack_s gx_clip_stack_t;
#endif
#ifndef gs_color_space_DEFINED
#  define gs_color_space_DEFINED
typedef struct gs_color_space_s gs_color_space;
#endif
#ifndef gs_client_color_DEFINED
#  define gs_client_color_DEFINED
typedef struct gs_client_color_s gs_client_color;
#endif
#ifndef gs_font_DEFINED
#  define gs_font_DEFINED
typedef struct gs_font_s gs_font;
#endif
#ifndef gs_transparency_group_DEFINED
#  define gs_transparency_group_DEFINED
typedef struct gs_transparency_group_s gs_transparency_group_t;
#endif
#ifndef gs_device_filter_stack_DEFINED
#  define gs_device_filter_stack_DEFINED
typedef struct gs_device_filter_stack_s gs_device_filter_stack_t;
#endif

/* Graphics state structure. */

struct gs_state_s {
    gs_imager_state_common;	/* imager state, must be first */
    gs_state *saved;		/* previous state from gsave */

    /* Transformation: */

    gs_matrix ctm_inverse;
    bool ctm_inverse_valid;	/* true if ctm_inverse = ctm^-1 */
    gs_matrix ctm_default;
    bool ctm_default_set;	/* if true, use ctm_default; */
				/* if false, ask device */

    /* Paths: */

    gx_path *path;
    gx_clip_path *clip_path;
    gx_clip_stack_t *clip_stack;  /* (LanguageLevel 3 only) */
    gx_clip_path *view_clip;	/* (may be 0, or have rule = 0) */
    bool clamp_coordinates;	/* if true, clamp out-of-range */
				/* coordinates; if false, */
				/* report a limitcheck */
    /* Effective clip path cache */
    gs_id effective_clip_id;	/* (key) clip path id */
    gs_id effective_view_clip_id;	/* (key) view clip path id */
    gx_clip_path *effective_clip_path;	/* (value) effective clip path, */
				/* possibly = clip_path or view_clip */
    bool effective_clip_shared;	/* true iff e.c.p. = c.p. or v.c. */

    /* Color (device-independent): */

    gs_color_space_index	/* before substitution */
        orig_cspace_index, orig_base_cspace_index;
    gs_color_space *color_space; /* after substitution */
    gx_device_color_spaces_t device_color_spaces; /* substituted spaces */
    gs_client_color *ccolor;

    /* Color caches: */

    gx_device_color *dev_color;

    /* Font: */

    gs_font *font;
    gs_font *root_font;
    gs_matrix_fixed char_tm;	/* font matrix * ctm */
#define char_tm_only(pgs) *(gs_matrix *)&(pgs)->char_tm
    bool char_tm_valid;		/* true if char_tm is valid */
    gs_in_cache_device_t in_cachedevice;    /* (see gscpm.h) */
    gs_char_path_mode in_charpath;	/* (see gscpm.h) */
    gs_state *show_gstate;	/* gstate when show was invoked */
				/* (so charpath can append to path) */

    /* Other stuff: */

    int level;			/* incremented by 1 per gsave */
    gx_device *device;
#undef gs_currentdevice_inline
#define gs_currentdevice_inline(pgs) ((pgs)->device)
    gs_device_filter_stack_t *dfilter_stack;

    gs_transparency_group_t *transparency_group_stack; /* (PDF 1.4 only) */

    /* Client data: */

    /*void *client_data;*/	/* in imager state */
#define gs_state_client_data(pgs) ((pgs)->client_data)
    gs_state_client_procs client_procs;
};

#define public_st_gs_state()	/* in gsstate.c */\
  gs_public_st_composite(st_gs_state, gs_state, "gs_state",\
    gs_state_enum_ptrs, gs_state_reloc_ptrs)

/*
 * Enumerate the pointers in a graphics state, other than the ones in the
 * imager state, and device, which must be handled specially.
 */
#define gs_state_do_ptrs(m)\
  m(0,saved) m(1,path) m(2,clip_path) m(3,clip_stack)\
  m(4,view_clip) m(5,effective_clip_path)\
  m(6,color_space) m(7,ccolor) m(8,dev_color)\
  m(9,font) m(10,root_font) m(11,show_gstate) /*m(---,device)*/\
  m(12,transparency_group_stack)\
  m(13,device_color_spaces.named.Gray)\
  m(14,device_color_spaces.named.RGB)\
  m(15,device_color_spaces.named.CMYK)
#define gs_state_num_ptrs 16

#endif /* gzstate_INCLUDED */

/* Copyright (C) 1998 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxclipsr.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Internals of clipsave/cliprestore */

#ifndef gxclipsr_INCLUDED
#  define gxclipsr_INCLUDED

#include "gsrefct.h"

/*
 * Unlike the graphics state stack, which is threaded through the actual
 * gstate objects, the clipping path stack is implemented with separate,
 * small objects.  These are reference-counted, because they may be
 * shared by off-stack graphics states.
 */

#ifndef gx_clip_path_DEFINED
#  define gx_clip_path_DEFINED
typedef struct gx_clip_path_s gx_clip_path;
#endif
#ifndef gx_clip_stack_DEFINED
#  define gx_clip_stack_DEFINED
typedef struct gx_clip_stack_s gx_clip_stack_t;
#endif

struct gx_clip_stack_s {
    rc_header rc;
    gx_clip_path *clip_path;
    gx_clip_stack_t *next;
};

#define private_st_clip_stack()	/* in gsclipsr.c */\
  gs_private_st_ptrs2(st_clip_stack, gx_clip_stack_t,\
    "gx_clip_stack_t", clip_stack_enum_ptrs, clip_stack_reloc_ptrs,\
    clip_path, next)

#endif /* gxclipsr_INCLUDED */

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

/*$Id: gxropc.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Internals for RasterOp compositing */

#ifndef gxropc_INCLUDED
#  define gxropc_INCLUDED

#include "gsropc.h"
#include "gxcomp.h"

/* Define RasterOp-compositing objects. */
typedef struct gs_composite_rop_s {
    gs_composite_common;
    gs_composite_rop_params_t params;
} gs_composite_rop_t;

#define private_st_composite_rop() /* in gsropc.c */\
  gs_private_st_ptrs1(st_composite_rop, gs_composite_rop_t,\
    "gs_composite_rop_t", composite_rop_enum_ptrs, composite_rop_reloc_ptrs,\
    params.texture)

/*
 * Initialize a RasterOp compositing function from parameters.
 * We make this visible so that clients can allocate gs_composite_rop_t
 * objects on the stack, to reduce memory manager overhead.
 */
void gx_init_composite_rop(P2(gs_composite_rop_t * pcte,
			      const gs_composite_rop_params_t * params));

#endif /* gxropc_INCLUDED */

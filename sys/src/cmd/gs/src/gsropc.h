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

/*$Id: gsropc.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* RasterOp-compositing interface */

#ifndef gsropc_INCLUDED
#  define gsropc_INCLUDED

#include "gscompt.h"
#include "gsropt.h"

/*
 * Define parameters for RasterOp-compositing.
 * There are two kinds of RasterOp compositing operations.
 * If texture == 0, the input data are the texture, and the source is
 * implicitly all 0 (black).  If texture != 0, it defines the texture,
 * and the input data are the source.  Note that in the latter case,
 * the client (the caller of gs_create_composite_rop) promises that
 * *texture will not change.
 */

#ifndef gx_device_color_DEFINED
#  define gx_device_color_DEFINED
typedef struct gx_device_color_s gx_device_color;

#endif

typedef struct gs_composite_rop_params_s {
    gs_logical_operation_t log_op;
    const gx_device_color *texture;
} gs_composite_rop_params_t;

/* Create a RasterOp-compositing object. */
int gs_create_composite_rop(P3(gs_composite_t ** ppcte,
			       const gs_composite_rop_params_t * params,
			       gs_memory_t * mem));

#endif /* gsropc_INCLUDED */

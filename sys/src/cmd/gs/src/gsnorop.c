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

/*$Id: gsnorop.c,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Stubs for unimplemented RasterOp */
#include "gx.h"
#include "gserrors.h"
#include "gsrop.h"
#include "gxdevcli.h"
#include "gxdevice.h"		/* for gx_default_*copy_rop prototypes */
#include "gxdevmem.h"		/* for gdevmem.h */
#include "gdevmem.h"		/* for mem_*_strip_copy_rop prototypes */
#include "gdevmrop.h"

/* Stub accessors to logical operation in graphics state. */

gs_logical_operation_t
gs_current_logical_op(const gs_state * pgs)
{
    return lop_default;
}

int
gs_set_logical_op(gs_state * pgs, gs_logical_operation_t lop)
{
    return (lop == lop_default ? 0 : gs_note_error(gs_error_rangecheck));
}

/* Stub RasterOp implementations for memory devices. */

int
mem_mono_strip_copy_rop(gx_device * dev,
	     const byte * sdata, int sourcex, uint sraster, gx_bitmap_id id,
			const gx_color_index * scolors,
	   const gx_strip_bitmap * textures, const gx_color_index * tcolors,
			int x, int y, int width, int height,
			int phase_x, int phase_y, gs_logical_operation_t lop)
{
    return_error(gs_error_rangecheck);
}

int
mem_gray_strip_copy_rop(gx_device * dev,
	     const byte * sdata, int sourcex, uint sraster, gx_bitmap_id id,
			const gx_color_index * scolors,
	   const gx_strip_bitmap * textures, const gx_color_index * tcolors,
			int x, int y, int width, int height,
			int phase_x, int phase_y, gs_logical_operation_t lop)
{
    return_error(gs_error_rangecheck);
}

int
mem_gray8_rgb24_strip_copy_rop(gx_device * dev,
	     const byte * sdata, int sourcex, uint sraster, gx_bitmap_id id,
			       const gx_color_index * scolors,
	   const gx_strip_bitmap * textures, const gx_color_index * tcolors,
			       int x, int y, int width, int height,
		       int phase_x, int phase_y, gs_logical_operation_t lop)
{
    return_error(gs_error_rangecheck);
}

/* Stub default implementations of device procedures. */

int
gx_default_copy_rop(gx_device * dev,
	     const byte * sdata, int sourcex, uint sraster, gx_bitmap_id id,
		    const gx_color_index * scolors,
	     const gx_tile_bitmap * texture, const gx_color_index * tcolors,
		    int x, int y, int width, int height,
		    int phase_x, int phase_y, gs_logical_operation_t lop)
{
    return_error(gs_error_unknownerror);	/* not implemented */
}

int
gx_default_strip_copy_rop(gx_device * dev,
	     const byte * sdata, int sourcex, uint sraster, gx_bitmap_id id,
			  const gx_color_index * scolors,
	   const gx_strip_bitmap * textures, const gx_color_index * tcolors,
			  int x, int y, int width, int height,
		       int phase_x, int phase_y, gs_logical_operation_t lop)
{
    return_error(gs_error_unknownerror);	/* not implemented */
}

int
mem_default_strip_copy_rop(gx_device * dev,
	     const byte * sdata, int sourcex, uint sraster, gx_bitmap_id id,
			  const gx_color_index * scolors,
	   const gx_strip_bitmap * textures, const gx_color_index * tcolors,
			  int x, int y, int width, int height,
		       int phase_x, int phase_y, gs_logical_operation_t lop)
{
    return_error(gs_error_unknownerror);	/* not implemented */
}

/* Stub RasterOp source devices. */

int
gx_alloc_rop_texture_device(gx_device_rop_texture ** prsdev, gs_memory_t * mem,
			    client_name_t cname)
{
    return_error(gs_error_rangecheck);
}

void
gx_make_rop_texture_device(gx_device_rop_texture * dev, gx_device * target,
	     gs_logical_operation_t log_op, const gx_device_color * texture)
{				/* Never called. */
}

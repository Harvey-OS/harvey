/* Copyright (C) 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* gxclip2.h */
/* Mask clipping device and interface */
/* Requires gxdevice.h, gxdevmem.h */

/*
 * Patterns that don't completely fill their bounding boxes require
 * the ability to clip against a tiled mask.  For now, we only support
 * tiling parallel to the axes.
 */

#define tile_clip_buffer_request 128
#define tile_clip_buffer_size\
  ((tile_clip_buffer_request / arch_sizeof_long) * arch_sizeof_long)
typedef struct gx_device_tile_clip_s {
	gx_device_forward_common;	/* target is set by client */
	gx_tile_bitmap tile;
	gx_device_memory mdev;		/* for tile buffer for copy_mono */
	gs_int_point phase;		/* device space origin relative */
				/* to tile (backwards from gstate phase) */
	/* Ensure that the buffer is long-aligned. */
	union _b {
		byte bytes[tile_clip_buffer_size];
		ulong longs[tile_clip_buffer_size / arch_sizeof_long];
	} buffer;
} gx_device_tile_clip;
#define private_st_device_tile_clip() /* in gxclip2.c */\
  gs_private_st_simple(st_device_tile_clip, gx_device_tile_clip,\
    "gx_device_tile_clip")

/* Initialize a tile clipping device from a mask. */
/* We supply an explicit phase. */
int tile_clip_initialize(P5(gx_device_tile_clip *, gx_tile_bitmap *,
  gx_device *, int, int));

/* Set the phase of the tile -- used in the tiling loop when */
/* the tile doesn't simply fill the plane. */
void tile_clip_set_phase(P3(gx_device_tile_clip *, int, int));

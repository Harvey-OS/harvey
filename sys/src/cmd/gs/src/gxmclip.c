/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxmclip.c,v 1.4 2002/02/21 22:24:53 giles Exp $ */
/* Mask clipping support */
#include "gx.h"
#include "gxdevice.h"
#include "gxdevmem.h"
#include "gxmclip.h"

/* Structure descriptor */
public_st_device_mask_clip();

/* GC procedures */
private ENUM_PTRS_WITH(device_mask_clip_enum_ptrs, gx_device_mask_clip *mcdev)
{
    if (index < st_gx_strip_bitmap_max_ptrs) {
	return ENUM_USING(st_gx_strip_bitmap, &mcdev->tiles,
			  sizeof(mcdev->tiles), index);
    }
    index -= st_gx_strip_bitmap_max_ptrs;
    if (index < st_device_memory_max_ptrs) {
	return ENUM_USING(st_device_memory, &mcdev->mdev,
			  sizeof(mcdev->mdev), index);
    }
    ENUM_PREFIX(st_device_forward, st_device_memory_max_ptrs);
}
ENUM_PTRS_END
private RELOC_PTRS_WITH(device_mask_clip_reloc_ptrs, gx_device_mask_clip *mcdev)
{
    RELOC_PREFIX(st_device_forward);
    RELOC_USING(st_gx_strip_bitmap, &mcdev->tiles, sizeof(mcdev->tiles));
    RELOC_USING(st_device_memory, &mcdev->mdev, sizeof(mcdev->mdev));
    if (mcdev->mdev.base != 0) {
	/*
	 * Update the line pointers specially, since they point into the
	 * buffer that is part of the mask clipping device itself.
	 */
	long diff = (char *)RELOC_OBJ(mcdev) - (char *)mcdev;
	int i;

	for (i = 0; i < mcdev->mdev.height; ++i)
	    mcdev->mdev.line_ptrs[i] += diff;
	mcdev->mdev.base = mcdev->mdev.line_ptrs[0];
	mcdev->mdev.line_ptrs =
	    (void *)((char *)(mcdev->mdev.line_ptrs) + diff);
    }
}
RELOC_PTRS_END

/* Initialize a mask clipping device. */
int
gx_mask_clip_initialize(gx_device_mask_clip * cdev,
			const gx_device_mask_clip * proto,
			const gx_bitmap * bits, gx_device * tdev,
			int tx, int ty, gs_memory_t *mem)
{
    int buffer_width = bits->size.x;
    int buffer_height =
	tile_clip_buffer_size / (bits->raster + sizeof(byte *));

    gx_device_init((gx_device *)cdev, (const gx_device *)proto,
		   mem, true);
    cdev->width = tdev->width;
    cdev->height = tdev->height;
    cdev->color_info = tdev->color_info;
    gx_device_set_target((gx_device_forward *)cdev, tdev);
    cdev->phase.x = -tx;
    cdev->phase.y = -ty;
    if (buffer_height > bits->size.y)
	buffer_height = bits->size.y;
    gs_make_mem_mono_device(&cdev->mdev, 0, 0);
    for (;;) {
	if (buffer_height <= 0) {
	    /*
	     * The tile is too wide to buffer even one scan line.
	     * We could do copy_mono in chunks, but for now, we punt.
	     */
	    cdev->mdev.base = 0;
	    return 0;
	}
	cdev->mdev.width = buffer_width;
	cdev->mdev.height = buffer_height;
	if (gdev_mem_bitmap_size(&cdev->mdev) <= tile_clip_buffer_size)
	    break;
	buffer_height--;
    }
    cdev->mdev.base = cdev->buffer.bytes;
    return (*dev_proc(&cdev->mdev, open_device))((gx_device *)&cdev->mdev);
}

/* Copyright (C) 1989, 1992, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gdevmem.c */
/* Generic "memory" (stored bitmap) device */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gxdevice.h"
#include "gxdevmem.h"			/* semi-public definitions */
#include "gdevmem.h"			/* private definitions */

/* Structure descriptor */
public_st_device_memory();

/* GC procedures */
#define mptr ((gx_device_memory *)vptr)
private ENUM_PTRS_BEGIN(device_memory_enum_ptrs) {
	return (*st_device_forward.enum_ptrs)(vptr, sizeof(gx_device_forward), index-2, pep);
	}
	case 0:
	  *pep = (mptr->foreign_bits ? NULL : (void *)mptr->base);
	  break;
	ENUM_STRING_PTR(1, gx_device_memory, palette);
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(device_memory_reloc_ptrs) {
	if ( !mptr->foreign_bits )
	{	byte *base_old = mptr->base;
		uint reloc;
		int y;
		RELOC_PTR(gx_device_memory, base);
		reloc = base_old - mptr->base;
		for ( y = 0; y < mptr->height; y++ )
		  mptr->line_ptrs[y] -= reloc;
	}
	RELOC_STRING_PTR(gx_device_memory, palette);
	(*st_device_forward.reloc_ptrs)(vptr, sizeof(gx_device_forward), gcst);
} RELOC_PTRS_END
#undef mptr

/* ------ Generic code ------ */

/* Return the appropriate memory device for a given */
/* number of bits per pixel (0 if none suitable). */
const gx_device_memory *
gdev_mem_device_for_bits(int bits_per_pixel)
{	switch ( bits_per_pixel )
	   {
	case 1: return &mem_mono_device;
	case 2: return &mem_mapped2_color_device;
	case 4: return &mem_mapped4_color_device;
	case 8: return &mem_mapped8_color_device;
	case 16: return &mem_true16_color_device;
	case 24: return &mem_true24_color_device;
	case 32: return &mem_true32_color_device;
	default: return 0;
	   }
}

/* Make a memory device. */
void
gs_make_mem_device(gx_device_memory *dev, const gx_device_memory *mdproto,
  gs_memory_t *mem, int page_device, gx_device *target)
{	*dev = *mdproto;
	dev->memory = mem;
	switch ( page_device )
	  {
	  case -1:
	    dev->std_procs.get_page_device = gx_default_get_page_device;
	    break;
	  case 1:
	    dev->std_procs.get_page_device = gx_page_device_get_page_device;
	    break;
	  }
	mdev->target = target;
	if ( target != 0 )
	  {	/* Forward the color mapping operations to the target. */
		gx_device_forward_color_procs((gx_device_forward *)dev);
	  }
}
/* Make a monobit memory device.  This is never a page device. */
void
gs_make_mem_mono_device(gx_device_memory *dev, gs_memory_t *mem,
  gx_device *target)
{	gs_make_mem_device(dev, &mem_mono_device, mem, -1, target);
}

/* Compute the size of the bitmap storage, */
/* including the space for the scan line pointer table. */
/* Note that scan lines are padded to a multiple of align_bitmap_mod bytes, */
/* and additional padding may be needed if the pointer table */
/* must be aligned to an even larger modulus. */
private ulong
mem_bitmap_bits_size(const gx_device_memory *dev)
{	return round_up((ulong)dev->height * gdev_mem_raster(dev),
			max(align_bitmap_mod, arch_align_ptr_mod));
}
ulong
gdev_mem_bitmap_size(const gx_device_memory *dev)
{        return mem_bitmap_bits_size(dev) +
		(ulong)dev->height * sizeof(byte *);
}

/* Open a memory device, allocating the data area if appropriate, */
/* and create the scan line table. */
private void mem_set_line_ptrs(P3(gx_device_memory *, byte **, byte *));
int
mem_open(gx_device *dev)
{	if ( mdev->bitmap_memory != 0 )
	{	/* Allocate the data now. */
		ulong size = gdev_mem_bitmap_size(mdev);
		if ( (uint)size != size )
			return_error(gs_error_limitcheck);
		mdev->base = gs_alloc_bytes(mdev->bitmap_memory, (uint)size,
					    "mem_open");
		if ( mdev->base == 0 )
			return_error(gs_error_VMerror);
		mdev->foreign_bits = false;
	}
/*
 * Macro for adding an offset to a pointer when setting up the
 * scan line table.  This isn't just pointer arithmetic, because of
 * the segmenting considerations discussed in gdevmem.h.
 */
#define huge_ptr_add(base, offset)\
   ((void *)((byte huge *)(base) + (offset)))
	mem_set_line_ptrs(mdev,
			  huge_ptr_add(mdev->base,
				       mem_bitmap_bits_size(mdev)),
			  mdev->base);
	return 0;
}
/* Set up the scan line pointers of a memory device. */
/* Sets line_ptrs, base, raster; uses width, height, color_info.depth. */
private void
mem_set_line_ptrs(gx_device_memory *devm, byte **line_ptrs, byte *base)
{	byte **pptr = devm->line_ptrs = line_ptrs;
	byte **pend = pptr + devm->height;
	byte *scan_line = devm->base = base;
	uint raster = devm->raster = gdev_mem_raster(devm);

	while ( pptr < pend )
	   {	*pptr++ = scan_line;
		scan_line = huge_ptr_add(scan_line, raster);
	   }
}

/* Return the initial transformation matrix */
void
mem_get_initial_matrix(gx_device *dev, gs_matrix *pmat)
{	pmat->xx = mdev->initial_matrix.xx;
	pmat->xy = mdev->initial_matrix.xy;
	pmat->yx = mdev->initial_matrix.yx;
	pmat->yy = mdev->initial_matrix.yy;
	pmat->tx = mdev->initial_matrix.tx;
	pmat->ty = mdev->initial_matrix.ty;
}

/* Test whether a device is a memory device */
bool
gs_device_is_memory(const gx_device *dev)
{	/* We can't just compare the procs, or even an individual proc, */
	/* because we might be tracing.  Compare the device name, */
	/* and hope for the best. */
	const char *name = dev->dname;
	int i;
	for ( i = 0; i < 6; i++ )
	  if ( name[i] != "image("[i] ) return false;
	return true;
}

/* Close a memory device, freeing the data area if appropriate. */
int
mem_close(gx_device *dev)
{	if ( mdev->bitmap_memory != 0 )
	  gs_free_object(mdev->bitmap_memory, mdev->base, "mem_close");
	return 0;
}

/* Copy a scan line to a client. */
#undef chunk
#define chunk byte
int
mem_get_bits(gx_device *dev, int y, byte *str, byte **actual_data)
{	byte *src;
	if ( y < 0 || y >= dev->height )
		return_error(gs_error_rangecheck);
	src = scan_line_base(mdev, y);
	if ( actual_data == 0 )
		memcpy(str, src, gx_device_raster(dev, 0));
	else
		*actual_data = src;
	return 0;
}

/* Map a r-g-b color to a color index for a mapped color memory device */
/* (2, 4, or 8 bits per pixel.) */
/* This requires searching the palette. */
gx_color_index
mem_mapped_map_rgb_color(gx_device *dev, gx_color_value r, gx_color_value g,
  gx_color_value b)
{	gx_device *target = mdev->target;
	if ( target != 0 )
	  return (*dev_proc(target, map_rgb_color))(target, r, g, b);
  {	byte br = gx_color_value_to_byte(r);
	byte bg = gx_color_value_to_byte(g);
	byte bb = gx_color_value_to_byte(b);
	register const byte *pptr = mdev->palette.data;
	int cnt = mdev->palette.size;
	const byte *which = 0;		/* initialized only to pacify gcc */
	int best = 256*3;

	while ( (cnt -= 3) >= 0 )
	   {	register int diff = *pptr - br;
		if ( diff < 0 ) diff = -diff;
		if ( diff < best )	/* quick rejection */
		   {	int dg = pptr[1] - bg;
			if ( dg < 0 ) dg = -dg;
			if ( (diff += dg) < best )	/* quick rejection */
			   {	int db = pptr[2] - bb;
				if ( db < 0 ) db = -db;
				if ( (diff += db) < best )
					which = pptr, best = diff;
			   }
		   }
		pptr += 3;
	   }
	return (gx_color_index)((which - mdev->palette.data) / 3);
  }
}

/* Map a color index to a r-g-b color for a mapped color memory device. */
int
mem_mapped_map_color_rgb(gx_device *dev, gx_color_index color,
  gx_color_value prgb[3])
{	gx_device *target = mdev->target;
	if ( target != 0 )
	  return (*dev_proc(target, map_color_rgb))(target, color, prgb);
  {	const byte *pptr = mdev->palette.data + (int)color * 3;
	prgb[0] = gx_color_value_from_byte(pptr[0]);
	prgb[1] = gx_color_value_from_byte(pptr[1]);
	prgb[2] = gx_color_value_from_byte(pptr[2]);
	return 0;
  }
}

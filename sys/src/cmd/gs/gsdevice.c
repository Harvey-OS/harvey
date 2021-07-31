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

/* gsdevice.c */
/* Device operators for Ghostscript library */
#include "math_.h"			/* for fabs */
#include "memory_.h"			/* for memcpy */
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gsparam.h"
#include "gspath.h"			/* gs_initclip prototype */
#include "gspaint.h"			/* gs_erasepage prototype */
#include "gsmatrix.h"			/* for gscoord.h */
#include "gscoord.h"			/* for gs_initmatrix */
#include "gxarith.h"
#include "gzstate.h"
#include "gxcmap.h"
#include "gxdevmem.h"

/* Import the device list from gconfig.c */
extern gx_device *gx_device_list[];
extern uint gx_device_list_count;

/* Structure descriptors */
public_st_device();
public_st_device_forward();
public_st_device_null();

/* GC utilities */
/* Enumerate or relocate a device pointer for a client. */
gx_device *
gx_device_enum_ptr(gx_device *dev)
{	if ( dev == 0 || dev->memory == 0 )
	  return 0;
	return dev;
}
gx_device *
gx_device_reloc_ptr(gx_device *dev, gc_state_t *gcst)
{	if ( dev == 0 || dev->memory == 0 )
	  return dev;
	return gs_reloc_struct_ptr(dev, gcst);
}
/* GC procedures */
#define fdev ((gx_device_forward *)vptr)
private ENUM_PTRS_BEGIN(device_forward_enum_ptrs) return 0;
	case 0:
	  *pep = gx_device_enum_ptr(fdev->target);
	  break;
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(device_forward_reloc_ptrs) {
	fdev->target = gx_device_reloc_ptr(fdev->target, gcst);
} RELOC_PTRS_END
#undef fdev

/* The null device */
private dev_proc_fill_rectangle(null_fill_rectangle);
private dev_proc_copy_mono(null_copy_mono);
private dev_proc_put_params(null_put_params);
private dev_proc_copy_alpha(null_copy_alpha);

private gx_device_null gs_null_device = {
	std_device_std_body_open(gx_device, 0, "null",
	  0, 0, 72, 72),
	{	gx_default_open_device,
		gx_forward_get_initial_matrix,
		gx_default_sync_output,
		gx_default_output_page,
		gx_default_close_device,
		gx_forward_map_rgb_color,
		gx_forward_map_color_rgb,
		null_fill_rectangle,
		gx_default_tile_rectangle,
		null_copy_mono,
		gx_default_copy_color,
		gx_default_draw_line,
		gx_default_get_bits,
		gx_forward_get_params,
		null_put_params,
		gx_forward_map_cmyk_color,
		gx_forward_get_xfont_procs,
		gx_forward_get_xfont_device,
		gx_forward_map_rgb_alpha_color,
		gx_default_get_page_device,	/* not a page device */
		gx_forward_get_alpha_bits,
		null_copy_alpha
	},
	0				/* target */
};

/* Set up the device procedures in the device structure. */
/* Also copy old fields to new ones. */
void
gx_device_set_procs(register gx_device *dev)
{	if ( dev->static_procs != 0 )		/* 0 if already populated */
	{	dev->std_procs = *dev->static_procs;
		dev->static_procs = 0;
	}
}

/* Initialize a device just after allocation. */
int
gdev_initialize(gx_device *dev)
{	*dev = *(gx_device *)&gs_null_device;
	return 0;
}

/* Fill in NULL procedures in a device procedure record. */
void
gx_device_fill_in_procs(register gx_device *dev)
{	gx_device_set_procs(dev);
	fill_dev_proc(dev, open_device, gx_default_open_device);
	fill_dev_proc(dev, get_initial_matrix, gx_default_get_initial_matrix);
	fill_dev_proc(dev, sync_output, gx_default_sync_output);
	fill_dev_proc(dev, output_page, gx_default_output_page);
	fill_dev_proc(dev, close_device, gx_default_close_device);
	fill_dev_proc(dev, map_rgb_color, gx_default_map_rgb_color);
	fill_dev_proc(dev, map_color_rgb, gx_default_map_color_rgb);
	/* NOT fill_rectangle */
	fill_dev_proc(dev, tile_rectangle, gx_default_tile_rectangle);
	/* NOT copy_mono */
	fill_dev_proc(dev, copy_color, gx_default_copy_color);	/* Bogus? */
	fill_dev_proc(dev, draw_line, gx_default_draw_line);
	fill_dev_proc(dev, get_bits, gx_default_get_bits);
	fill_dev_proc(dev, get_params, gx_default_get_params);
	fill_dev_proc(dev, put_params, gx_default_put_params);
	fill_dev_proc(dev, map_cmyk_color, gx_default_map_cmyk_color);
	fill_dev_proc(dev, get_xfont_procs, gx_default_get_xfont_procs);
	fill_dev_proc(dev, get_xfont_device, gx_default_get_xfont_device);
	fill_dev_proc(dev, map_rgb_alpha_color, gx_default_map_rgb_alpha_color);
	fill_dev_proc(dev, get_page_device, gx_default_get_page_device);
	fill_dev_proc(dev, get_alpha_bits, gx_default_get_alpha_bits);
	/* NOT copy_alpha */
}
/* Fill in NULL procedures in a forwarding device procedure record. */
void
gx_device_forward_fill_in_procs(register gx_device_forward *dev)
{	gx_device_set_procs((gx_device *)dev);
	fill_dev_proc(dev, get_initial_matrix, gx_forward_get_initial_matrix);
	fill_dev_proc(dev, map_rgb_color, gx_forward_map_rgb_color);
	fill_dev_proc(dev, map_color_rgb, gx_forward_map_color_rgb);
	fill_dev_proc(dev, get_params, gx_forward_get_params);
	fill_dev_proc(dev, put_params, gx_forward_put_params);
	fill_dev_proc(dev, map_cmyk_color, gx_forward_map_cmyk_color);
	fill_dev_proc(dev, get_xfont_procs, gx_forward_get_xfont_procs);
	fill_dev_proc(dev, get_xfont_device, gx_forward_get_xfont_device);
	fill_dev_proc(dev, map_rgb_alpha_color, gx_forward_map_rgb_alpha_color);
	fill_dev_proc(dev, get_page_device, gx_forward_get_page_device);
	fill_dev_proc(dev, get_alpha_bits, gx_forward_get_alpha_bits);
	gx_device_fill_in_procs((gx_device *)dev);
}

/* Forward the color mapping procedures from a device to its target. */
void
gx_device_forward_color_procs(gx_device_forward *dev)
{	set_dev_proc(dev, map_rgb_color, gx_forward_map_rgb_color);
	set_dev_proc(dev, map_color_rgb, gx_forward_map_color_rgb);
	set_dev_proc(dev, map_cmyk_color, gx_forward_map_cmyk_color);
	set_dev_proc(dev, map_rgb_alpha_color, gx_forward_map_rgb_alpha_color);
}

/* Flush buffered output to the device */
int
gs_flushpage(gs_state *pgs)
{	gx_device *dev = gs_currentdevice(pgs);
	return (*dev_proc(dev, sync_output))(dev);
}

/* Make the device output the accumulated page description */
int
gs_copypage(gs_state *pgs)
{	return gs_output_page(pgs, 1, 0);
}
int
gs_output_page(gs_state *pgs, int num_copies, int flush)
{	gx_device *dev = gs_currentdevice(pgs);
	int code = (*dev_proc(dev, output_page))(dev, num_copies, flush);
	if ( code >= 0 )
	{	dev->PageCount++;
		if ( flush )
			dev->ShowpageCount++;
	}
	return code;
}

/* Copy scan lines from an image device */
int
gs_copyscanlines(gx_device *dev, int start_y, byte *data, uint size,
  int *plines_copied, uint *pbytes_copied)
{	uint line_size = gx_device_raster(dev, 0);
	uint count = size / line_size;
	uint i;
	byte *dest = data;
	for ( i = 0; i < count; i++, dest += line_size )
	{	int code = (*dev_proc(dev, get_bits))(dev, start_y + i, dest, NULL);
		if ( code < 0 )
		{	/* Might just be an overrun. */
			if ( start_y + i == dev->height ) break;
			return_error(code);
		}
	}
	if ( plines_copied != NULL )
	  *plines_copied = i;
	if ( pbytes_copied != NULL )
	  *pbytes_copied = i * line_size;
	return 0;
}

/* Get the current device from the graphics state */
gx_device *
gs_currentdevice(const gs_state *pgs)
{	return pgs->device;
}

/* Get the name of a device */
const char *
gs_devicename(const gx_device *dev)
{	return dev->dname;
}

/* Get the initial matrix of a device. */
void
gs_deviceinitialmatrix(gx_device *dev, gs_matrix *pmat)
{	fill_dev_proc(dev, get_initial_matrix, gx_default_get_initial_matrix);
	(*dev_proc(dev, get_initial_matrix))(dev, pmat);
}

/* Get the N'th device from the known device list */
gx_device *
gs_getdevice(int index)
{	if ( index < 0 || index >= gx_device_list_count )
		return 0;		/* index out of range */
	return gx_device_list[index];
}

/* Clone an existing device. */
int
gs_copydevice(gx_device **pnew_dev, const gx_device *dev, gs_memory_t *mem)
{	register gx_device *new_dev;
	new_dev = (gx_device *)gs_alloc_bytes(mem, dev->params_size, "gs_copydevice");
	if ( new_dev == 0 )
		return_error(gs_error_VMerror);
	memcpy(new_dev, dev, dev->params_size);
	new_dev->memory = mem;
	new_dev->is_open = false;
	*pnew_dev = new_dev;
	return 0;
}

/* Make a memory (image) device. */
/* If colors_size = -16, -24, or -32, this is a true-color device; */
/* otherwise, colors_size is the size of the palette in bytes */
/* (2^N for gray scale, 3*2^N for RGB color). */
int
gs_makeimagedevice(gx_device **pnew_dev, const gs_matrix *pmat,
  uint width, uint height, const byte *colors, int colors_size,
  gs_memory_t *mem)
{	const gx_device_memory *old_dev;
	register gx_device_memory *new_dev;
	int palette_count = colors_size;
	int num_components = 1;
	int pcount;
	int bits_per_pixel;
	float x_pixels_per_unit, y_pixels_per_unit;
	byte palette[256 * 3];
	byte *dev_palette;
	int has_color;
	switch ( colors_size )
	   {
	case 3*2:
		palette_count = 2; num_components = 3;
	case 2:
		bits_per_pixel = 1; break;
	case 3*4:
		palette_count = 4; num_components = 3;
	case 4:
		bits_per_pixel = 2; break;
	case 3*16:
		palette_count = 16; num_components = 3;
	case 16:
		bits_per_pixel = 4; break;
	case 3*256:
		palette_count = 256; num_components = 3;
	case 256:
		bits_per_pixel = 8; break;
	case -16:
		bits_per_pixel = 16; palette_count = 0; break;
	case -24:
		bits_per_pixel = 24; palette_count = 0; break;
	case -32:
		bits_per_pixel = 32; palette_count = 0; break;
	default:
		return_error(gs_error_rangecheck);
	   }
	old_dev = gdev_mem_device_for_bits(bits_per_pixel);
	if ( old_dev == 0 )		/* no suitable device */
		return_error(gs_error_rangecheck);
	pcount = palette_count * 3;
	/* Check to make sure the palette contains white and black, */
	/* and, if it has any colors, the six primaries. */
	if ( bits_per_pixel <= 8 )
	   {	const byte *p;
		byte *q;
		int primary_mask = 0;
		int i;
		has_color = 0;
		for ( i = 0, p = colors, q = palette;
		      i < palette_count; i++, q += 3
		    )
		   {	int mask = 1;
			switch ( num_components )
			   {
			case 1:			/* gray */
				q[0] = q[1] = q[2] = *p++;
				break;
			default /* case 3 */:	/* RGB */
				q[0] = p[0], q[1] = p[1], q[2] = p[2];
				p += 3;
			   }
#define shift_mask(b,n)\
  switch ( b ) { case 0xff: mask <<= n; case 0: break; default: mask = 0; }
			shift_mask(q[0], 4);
			shift_mask(q[1], 2);
			shift_mask(q[2], 1);
#undef shift_mask
			primary_mask |= mask;
			if ( q[0] != q[1] || q[0] != q[2] )
				has_color = 1;
		   }
		switch ( primary_mask )
		   {
		case 129:		/* just black and white */
			if ( has_color )	/* color but no primaries */
				return_error(gs_error_rangecheck);
		case 255:		/* full color */
			break;
		default:
			return_error(gs_error_rangecheck);
		   }
	   }
	else
		has_color = 1;
	/*
	 * The initial transformation matrix must map 1 user unit to
	 * 1/72".  Let W and H be the width and height in pixels, and
	 * assume the initial matrix is of the form [A 0 0 B X Y].
	 * Then the size of the image in user units is (W/|A|,H/|B|),
	 * hence the size in inches is ((W/|A|)/72,(H/|B|)/72), so
	 * the number of pixels per inch is
	 * (W/((W/|A|)/72),H/((H/|B|)/72)), or (|A|*72,|B|*72).
	 * Similarly, if the initial matrix is [0 A B 0 X Y] for a 90
	 * or 270 degree rotation, the size of the image in user
	 * units is (W/|B|,H/|A|), so the pixels per inch are
	 * (|B|*72,|A|*72).  We forbid non-orthogonal transformation
	 * matrices.
	 */
	if ( is_fzero2(pmat->xy, pmat->yx) )
		x_pixels_per_unit = pmat->xx, y_pixels_per_unit = pmat->yy;
	else if ( is_fzero2(pmat->xx, pmat->yy) )
		x_pixels_per_unit = pmat->yx, y_pixels_per_unit = pmat->xy;
	else
		return_error(gs_error_undefinedresult);
	/* All checks done, allocate the device. */
	new_dev = gs_alloc_struct(mem, gx_device_memory, &st_device_memory,
				  "gs_makeimagedevice(device)");
	dev_palette = gs_alloc_string(mem, pcount, "gs_makeimagedevice(palette)");
	if ( new_dev == 0 || dev_palette == 0 )
	  {	gs_free_object(mem, new_dev, "gs_makeimagedevice(device)");
		gs_free_object(mem, dev_palette, "gs_makeimagedevice(palette)");
		return_error(gs_error_VMerror);
	  }
	gs_make_mem_device(new_dev, old_dev, mem, 1, 0);
	new_dev->initial_matrix = *pmat;
	new_dev->x_pixels_per_inch = fabs(x_pixels_per_unit) * 72;
	new_dev->y_pixels_per_inch = fabs(y_pixels_per_unit) * 72;
	gx_device_set_width_height((gx_device *)new_dev, width, height);
	if ( !has_color )
	  {	new_dev->color_info.num_components = 1;
		new_dev->color_info.max_color = 0;
		new_dev->color_info.dither_colors = 0;
	  }
	new_dev->invert = (palette[0] | palette[1] | palette[2] ? -1 : 0);	/* bogus */
	new_dev->palette.size = pcount;
	new_dev->palette.data = dev_palette;
	memcpy(dev_palette, palette, pcount);
	/* The bitmap will be allocated when the device is opened. */
	new_dev->memory = mem;
	new_dev->is_open = false;
	new_dev->bitmap_memory = mem;
	*pnew_dev = (gx_device *)new_dev;
	return 0;
}

/* Set the device in the graphics state */
int
gs_setdevice(gs_state *pgs, gx_device *dev)
{	int code = gs_setdevice_no_erase(pgs, dev);
	if ( code == 1 )
		code = gs_erasepage(pgs);
	return code;
}
int
gs_setdevice_no_erase(gs_state *pgs, gx_device *dev)
{	bool was_open = dev->is_open;
	int code;
	/* Initialize the device */
	if ( !was_open )
	{	gx_device_fill_in_procs(dev);
		if ( gs_device_is_memory(dev) )
		{	/* Set the target to the current device. */
			gx_device *odev = gs_currentdevice_inline(pgs);
			while ( odev != 0 && gs_device_is_memory(odev) )
				odev = ((gx_device_memory *)odev)->target;
			((gx_device_memory *)dev)->target = odev;
		}
		code = (*dev_proc(dev, open_device))(dev);
		if ( code < 0 ) return_error(code);
		dev->is_open = true;
	}
	/* Compute device white and black codes */
	dev->cached.black = gx_map_cmyk_color(dev, 0, 0, 0, gx_max_color_value);
	dev->cached.white = gx_map_cmyk_color(dev, 0, 0, 0, 0);
	pgs->device = dev;
	gx_set_cmap_procs(pgs);
	if (	(code = gs_initmatrix(pgs)) < 0 ||
		(code = gs_initclip(pgs)) < 0
	   )
		return code;
	gx_unset_dev_color(pgs);
	/* If we were in a charpath or a setcachedevice, */
	/* we aren't any longer. */
	pgs->in_cachedevice = 0;
	pgs->in_charpath = 0;
	return (was_open ? 0 : 1);
}

/* Make a null device. */
void
gs_make_null_device(gx_device_null *dev, gs_memory_t *mem)
{	*dev = gs_null_device;
	dev->memory = mem;
}

/* Select the null device.  This is just a convenience. */
void
gs_nulldevice(gs_state *pgs)
{	gs_setdevice(pgs, (gx_device *)&gs_null_device);
}

/* Close a device.  The client is responsible for ensuring that */
/* this device is not current in any graphics state. */
int
gs_closedevice(gx_device *dev)
{	int code = 0;
	if ( dev->is_open )
	   {	code = (*dev_proc(dev, close_device))(dev);
		if ( code < 0 ) return_error(code);
		dev->is_open = false;
	   }
	return code;
}

/* Install enough of a null device to suppress graphics output */
/* during the execution of stringwidth. */
void
gx_device_no_output(gs_state *pgs)
{	pgs->device = (gx_device *)&gs_null_device;
}

/* Just set the device without reinitializing. */
/* (For internal use only.) */
void
gx_set_device_only(gs_state *pgs, gx_device *dev)
{	pgs->device = dev;
}

/* Compute the size of one scan line for a device, */
/* with or without padding to a word boundary. */
uint
gx_device_raster(const gx_device *dev, int pad)
{	ulong bits = (ulong)dev->width * dev->color_info.depth;
	return (pad ? bitmap_raster(bits) : (uint)((bits + 7) >> 3));
}

/* Adjust the resolution for devices that only have a fixed set of */
/* geometries, so that the apparent size in inches remains constant. */
/* If fit=1, the resolution is adjusted so that the entire image fits; */
/* if fit=0, one dimension fits, but the other one is clipped. */
int
gx_device_adjust_resolution(gx_device *dev,
  int actual_width, int actual_height, int fit)
{	double width_ratio = (double)actual_width / dev->width ;
	double height_ratio = (double)actual_height / dev->height ;
	double ratio =
		(fit ? min(width_ratio, height_ratio) :
		 max(width_ratio, height_ratio));
	dev->x_pixels_per_inch *= ratio;
	dev->y_pixels_per_inch *= ratio;
	gx_device_set_width_height(dev, actual_width, actual_height);
	return 0;
}

/* Set the HWMargins to values defined in inches. */
/* If move_origin is true, also reset the Margins. */
void
gx_device_set_margins(gx_device *dev, const float *margins /*[4]*/,
  bool move_origin)
{	int i;
	for ( i = 0; i < 4; ++i )
		dev->HWMargins[i] = margins[i] * 72.0;
	if ( move_origin )
	{	dev->Margins[0] = -margins[0] * dev->Margins_HWResolution[0];
		dev->Margins[1] = -margins[3] * dev->Margins_HWResolution[1];
	}
}

/* Set the width and height, updating PageSize to remain consistent. */
void
gx_device_set_width_height(gx_device *dev, int width, int height)
{	dev->width = width;
	dev->height = height;
	dev->PageSize[0] = width * 72.0 / dev->x_pixels_per_inch;
	dev->PageSize[1] = height * 72.0 / dev->y_pixels_per_inch;
}

/* ------ Default device procedures ------ */

int
gx_default_open_device(gx_device *dev)
{	return 0;
}

/* Get the initial matrix for a device with inverted Y. */
/* This includes essentially all printers and displays. */
void
gx_default_get_initial_matrix(gx_device *dev, register gs_matrix *pmat)
{	int orientation = (dev->Orientation_set ? dev->Orientation : 0);
	pmat->xx = dev->HWResolution[0] / 72.0;  /* x_pixels_per_inch */
	pmat->xy = 0;
	pmat->yx = 0;
	pmat->yy = dev->HWResolution[1] / -72.0;  /* y_pixels_per_inch */
		/****** tx/y is WRONG for devices with ******/
		/****** arbitrary initial matrix ******/
	pmat->tx = 0;
	pmat->ty = dev->height;
	if ( orientation != 0 )
	  {	gs_matrix_rotate(pmat, 90.0 * orientation, pmat);
		/* We must also adjust the translation so that */
		/* the image falls on the page. */
		switch ( orientation )
		  {
		  case 2: pmat->ty = 0;
		  case 1: pmat->tx = dev->width; break;
		  case 3: pmat->ty = 0;
		  }
	  }
}
/* Get the initial matrix for a device with upright Y. */
/* This includes just a few printers and window systems. */
void
gx_upright_get_initial_matrix(gx_device *dev, register gs_matrix *pmat)
{	int orientation = (dev->Orientation_set ? dev->Orientation : 0);
	pmat->xx = dev->HWResolution[0] / 72.0;  /* x_pixels_per_inch */
	pmat->xy = 0;
	pmat->yx = 0;
	pmat->yy = dev->HWResolution[1] / 72.0;  /* y_pixels_per_inch */
		/****** tx/y is WRONG for devices with ******/
		/****** arbitrary initial matrix ******/
	pmat->tx = 0;
	pmat->ty = 0;
	if ( orientation != 0 )
	  {	gs_matrix_rotate(pmat, 90.0 * orientation, pmat);
		/* We must also adjust the translation so that */
		/* the image falls on the page. */
		switch ( orientation )
		  {
		  case 2: pmat->ty = dev->height;
		  case 1: pmat->tx = dev->width; break;
		  case 3: pmat->ty = dev->height;
		  }
	  }
}

int
gx_default_sync_output(gx_device *dev)
{	return 0;
}

int
gx_default_output_page(gx_device *dev, int num_copies, int flush)
{	return (*dev_proc(dev, sync_output))(dev);
}

int
gx_default_close_device(gx_device *dev)
{	return 0;
}

int
gx_default_copy_color(gx_device *dev, const byte *data,
  int data_x, int raster, gx_bitmap_id id,
  int x, int y, int width, int height)
{	return (*dev_proc(dev, copy_mono))(dev, data, data_x, raster, id,
		x, y, width, height, (gx_color_index)0, (gx_color_index)1);
}

int
gx_default_get_bits(gx_device *dev, int y, byte *data, byte **actual_data)
{	return -1;
}

gx_xfont_procs *
gx_default_get_xfont_procs(gx_device *dev)
{	return NULL;
}

gx_device *
gx_default_get_xfont_device(gx_device *dev)
{	return dev;
}

gx_device *
gx_default_get_page_device(gx_device *dev)
{	return NULL;
}
gx_device *
gx_page_device_get_page_device(gx_device *dev)
{	return dev;
}

int
gx_default_get_alpha_bits(gx_device *dev, graphics_object_type type)
{	return 1;
}

int
gx_default_copy_alpha(gx_device *dev, const byte *data, int data_x,
  int raster, gx_bitmap_id id, int x, int y, int width, int height,
  gx_color_index color, int depth)
{	return -1;		/* should never be called */
}

/* ------ Default per-instance procedures ------ */

int
gx_default_install(gx_device *dev, gs_state *pgs)
{	return 0;
}

int
gx_default_begin_page(gx_device *dev, gs_state *pgs)
{	return 0;
}

int
gx_default_end_page(gx_device *dev, int reason, gs_state *pgs)
{	return (reason != 2 ? 1 : 0);
}

/* ------ Default forwarding procedures ------ */

#define fdev ((gx_device_forward *)dev)

void
gx_forward_get_initial_matrix(gx_device *dev, gs_matrix *pmat)
{ gx_device *tdev = fdev->target;
	if ( tdev == 0 )
		gx_default_get_initial_matrix(dev, pmat);
	else
		(*dev_proc(tdev, get_initial_matrix))(tdev, pmat);
}

gx_color_index
gx_forward_map_rgb_color(gx_device *dev, gx_color_value r, gx_color_value g,
  gx_color_value b)
{	gx_device *tdev = fdev->target;
	return (tdev == 0 ? gx_default_map_rgb_color(dev, r, g, b) :
		(*dev_proc(tdev, map_rgb_color))(tdev, r, g, b));
}

int
gx_forward_map_color_rgb(gx_device *dev, gx_color_index color,
  gx_color_value prgb[3])
{	gx_device *tdev = fdev->target;
	return (tdev == 0 ? gx_default_map_color_rgb(dev, color, prgb) :
		(*dev_proc(tdev, map_color_rgb))(tdev, color, prgb));
}

int
gx_forward_get_params(gx_device *dev, gs_param_list *plist)
{	gx_device *tdev = fdev->target;
	return (tdev == 0 ? gx_default_get_params(dev, plist) :
		(*dev_proc(tdev, get_params))(tdev, plist));
}

int
gx_forward_put_params(gx_device *dev, gs_param_list *plist)
{	gx_device *tdev = fdev->target;
	return (tdev == 0 ? gx_default_put_params(dev, plist) :
		(*dev_proc(tdev, put_params))(tdev, plist));
}

gx_color_index
gx_forward_map_cmyk_color(gx_device *dev, gx_color_value c, gx_color_value m,
  gx_color_value y, gx_color_value k)
{	gx_device *tdev = fdev->target;
	return (tdev == 0 ? gx_default_map_cmyk_color(dev, c, m, y, k) :
		(*dev_proc(tdev, map_cmyk_color))(tdev, c, m, y, k));
}

gx_xfont_procs *
gx_forward_get_xfont_procs(gx_device *dev)
{	gx_device *tdev = fdev->target;
	return (tdev == 0 ? gx_default_get_xfont_procs(dev) :
		(*dev_proc(tdev, get_xfont_procs))(tdev));
}

gx_device *
gx_forward_get_xfont_device(gx_device *dev)
{	gx_device *tdev = fdev->target;
	return (tdev == 0 ? gx_default_get_xfont_device(dev) :
		(*dev_proc(tdev, get_xfont_device))(tdev));
}

gx_color_index
gx_forward_map_rgb_alpha_color(gx_device *dev, gx_color_value r,
  gx_color_value g, gx_color_value b, gx_color_value alpha)
{	gx_device *tdev = fdev->target;
	return (tdev == 0 ?
		gx_default_map_rgb_alpha_color(dev, r, g, b, alpha) :
		(*dev_proc(tdev, map_rgb_alpha_color))(tdev, r, g, b, alpha));
}

gx_device *
gx_forward_get_page_device(gx_device *dev)
{	gx_device *tdev = fdev->target;
	return (tdev == 0 ? gx_default_get_page_device(dev) :
		(*dev_proc(tdev, get_page_device))(tdev));
}

int
gx_forward_get_alpha_bits(gx_device *dev, graphics_object_type type)
{	gx_device *tdev = fdev->target;
	return (tdev == 0 ?
		gx_default_get_alpha_bits(dev, type) :
		(*dev_proc(tdev, get_alpha_bits))(tdev, type));
}

/* ------ The null device ------ */

private int
null_fill_rectangle(gx_device *dev, int x, int y, int w, int h,
  gx_color_index color)
{	return 0;
}
private int
null_copy_mono(gx_device *dev, const byte *data,
  int dx, int raster, gx_bitmap_id id, int x, int y, int w, int h,
  gx_color_index zero, gx_color_index one)
{	return 0;
}
private int
null_put_params(gx_device *dev, gs_param_list *plist)
{	/* We must defeat attempts to reset the size; */
	/* otherwise this is equivalent to gx_forward_put_params. */
	gx_device *tdev = fdev->target;
	int code;

	if ( tdev != 0 )
	  return (*dev_proc(tdev, put_params))(tdev, plist);
	code = gx_default_put_params(dev, plist);
	if ( code < 0 )
	  return code;
	dev->width = dev->height = 0;
	return code;
}
private int
null_copy_alpha(gx_device *dev, const byte *data, int data_x,
    int raster, gx_bitmap_id id, int x, int y, int width, int height,
    gx_color_index color, int depth)
{	return 0;
}

/* Copyright (C) 1989, 2000 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gsdevice.c,v 1.2 2000/03/18 01:45:16 lpd Exp $ */
/* Device operators for Ghostscript library */
#include "ctype_.h"
#include "memory_.h"		/* for memcpy */
#include "string_.h"
#include "gx.h"
#include "gp.h"
#include "gscdefs.h"		/* for gs_lib_device_list */
#include "gserrors.h"
#include "gsfname.h"
#include "gsstruct.h"
#include "gspath.h"		/* gs_initclip prototype */
#include "gspaint.h"		/* gs_erasepage prototype */
#include "gsmatrix.h"		/* for gscoord.h */
#include "gscoord.h"		/* for gs_initmatrix */
#include "gzstate.h"
#include "gxcmap.h"
#include "gxdevice.h"
#include "gxdevmem.h"
#include "gxiodev.h"

/* Include the extern for the device list. */
extern_gs_lib_device_list();

/*
 * Finalization for devices: do any special finalization first, then
 * close the device if it is open, and finally free the structure
 * descriptor if it is dynamic.
 */
void
gx_device_finalize(void *vptr)
{
    gx_device * const dev = (gx_device *)vptr;

    if (dev->finalize)
	dev->finalize(dev);
    discard(gs_closedevice(dev));
    if (dev->stype_is_dynamic)
	gs_free_const_object(&gs_memory_default, dev->stype,
			     "gx_device_finalize");
}

/* "Free" a device locally allocated on the stack, by finalizing it. */
void
gx_device_free_local(gx_device *dev)
{
    gx_device_finalize(dev);
}

/* GC procedures */
private 
ENUM_PTRS_WITH(device_forward_enum_ptrs, gx_device_forward *fdev) return 0;
case 0: ENUM_RETURN(gx_device_enum_ptr(fdev->target));
ENUM_PTRS_END
private RELOC_PTRS_WITH(device_forward_reloc_ptrs, gx_device_forward *fdev)
{
    fdev->target = gx_device_reloc_ptr(fdev->target, gcst);
}
RELOC_PTRS_END

/*
 * Structure descriptors.  These must follow the procedures, because
 * we can't conveniently forward-declare the procedures.
 * (See gxdevice.h for details.)
 */
public_st_device();
public_st_device_forward();
public_st_device_null();

/* GC utilities */
/* Enumerate or relocate a device pointer for a client. */
gx_device *
gx_device_enum_ptr(gx_device * dev)
{
    if (dev == 0 || dev->memory == 0)
	return 0;
    return dev;
}
gx_device *
gx_device_reloc_ptr(gx_device * dev, gc_state_t * gcst)
{
    if (dev == 0 || dev->memory == 0)
	return dev;
    return RELOC_OBJ(dev);	/* gcst implicit */
}

/* Set up the device procedures in the device structure. */
/* Also copy old fields to new ones. */
void
gx_device_set_procs(gx_device * dev)
{
    if (dev->static_procs != 0) {	/* 0 if already populated */
	dev->procs = *dev->static_procs;
	dev->static_procs = 0;
    }
}

/* Flush buffered output to the device */
int
gs_flushpage(gs_state * pgs)
{
    gx_device *dev = gs_currentdevice(pgs);

    return (*dev_proc(dev, sync_output)) (dev);
}

/* Make the device output the accumulated page description */
int
gs_copypage(gs_state * pgs)
{
    return gs_output_page(pgs, 1, 0);
}
int
gs_output_page(gs_state * pgs, int num_copies, int flush)
{
    gx_device *dev = gs_currentdevice(pgs);

    if (dev->IgnoreNumCopies)
	num_copies = 1;
    return (*dev_proc(dev, output_page)) (dev, num_copies, flush);
}

/*
 * Do generic work for output_page.  All output_page procedures must call
 * this as the last thing they do, unless an error has occurred earlier.
 */
int
gx_finish_output_page(gx_device *dev, int num_copies, int flush)
{
    dev->PageCount += num_copies;
    return 0;
}

/* Copy scan lines from an image device */
int
gs_copyscanlines(gx_device * dev, int start_y, byte * data, uint size,
		 int *plines_copied, uint * pbytes_copied)
{
    uint line_size = gx_device_raster(dev, 0);
    uint count = size / line_size;
    uint i;
    byte *dest = data;

    for (i = 0; i < count; i++, dest += line_size) {
	int code = (*dev_proc(dev, get_bits)) (dev, start_y + i, dest, NULL);

	if (code < 0) {
	    /* Might just be an overrun. */
	    if (start_y + i == dev->height)
		break;
	    return_error(code);
	}
    }
    if (plines_copied != NULL)
	*plines_copied = i;
    if (pbytes_copied != NULL)
	*pbytes_copied = i * line_size;
    return 0;
}

/* Get the current device from the graphics state. */
gx_device *
gs_currentdevice(const gs_state * pgs)
{
    return pgs->device;
}

/* Get the name of a device. */
const char *
gs_devicename(const gx_device * dev)
{
    return dev->dname;
}

/* Get the initial matrix of a device. */
void
gs_deviceinitialmatrix(gx_device * dev, gs_matrix * pmat)
{
    fill_dev_proc(dev, get_initial_matrix, gx_default_get_initial_matrix);
    (*dev_proc(dev, get_initial_matrix)) (dev, pmat);
}

/* Get the N'th device from the known device list */
const gx_device *
gs_getdevice(int index)
{
    const gx_device *const *list;
    int count = gs_lib_device_list(&list, NULL);

    if (index < 0 || index >= count)
	return 0;		/* index out of range */
    return list[index];
}

/* Fill in the GC structure descriptor for a device. */
private void
gx_device_make_struct_type(gs_memory_struct_type_t *st,
			   const gx_device *dev)
{
    const gx_device_procs *procs = dev->static_procs;

    /*
     * Try to figure out whether this is a forwarding device.  For printer
     * devices, we rely on the prototype referencing the correct structure
     * descriptor; for other devices, we look for a likely forwarding
     * procedure in the vector.  The algorithm isn't foolproof, but it's the
     * best we can come up with.
     */
    if (procs == 0)
	procs = &dev->procs;
    if (dev->stype)
	*st = *dev->stype;
    else if (procs->get_xfont_procs == gx_forward_get_xfont_procs)
	*st = st_device_forward;
    else
	*st = st_device;
    st->ssize = dev->params_size;
}

/* Clone an existing device. */
int
gs_copydevice(gx_device ** pnew_dev, const gx_device * dev, gs_memory_t * mem)
{
    gx_device *new_dev;
    const gs_memory_struct_type_t *std = dev->stype;
    const gs_memory_struct_type_t *new_std;

    if (dev->stype_is_dynamic) {
	/*
	 * We allocated the stype for this device previously.
	 * Just allocate a new stype and copy the old one into it.
	 */
	gs_memory_struct_type_t *a_std = (gs_memory_struct_type_t *)
	    gs_alloc_bytes_immovable(&gs_memory_default, sizeof(*std),
				     "gs_copydevice(stype)");

	if (!a_std)
	    return_error(gs_error_VMerror);
	*a_std = *std;
	new_std = a_std;
    } else if (std != 0 && std->ssize == dev->params_size) {
	/* Use the static stype. */
	new_std = std;
    } else {
	/* We need to figure out or adjust the stype. */
	gs_memory_struct_type_t *a_std = (gs_memory_struct_type_t *)
	    gs_alloc_bytes_immovable(&gs_memory_default, sizeof(*std),
				     "gs_copydevice(stype)");

	if (!a_std)
	    return_error(gs_error_VMerror);
	gx_device_make_struct_type(a_std, dev);
	new_std = a_std;
    }
    /*
     * Because command list devices have complicated internal pointer
     * structures, we allocate all device instances as immovable.
     */
    new_dev = gs_alloc_struct_immovable(mem, gx_device, new_std,
					"gs_copydevice(device)");
    if (new_dev == 0)
	return_error(gs_error_VMerror);
    gx_device_init(new_dev, dev, mem, false);
    new_dev->stype = new_std;
    new_dev->stype_is_dynamic = new_std != std;
    new_dev->is_open = false;
    *pnew_dev = new_dev;
    return 0;
}

/* Open a device if not open already.  Return 0 if the device was open, */
/* 1 if it was closed. */
int
gs_opendevice(gx_device *dev)
{
    if (dev->is_open)
	return 0;
    gx_device_fill_in_procs(dev);
    {
	int code = (*dev_proc(dev, open_device))(dev);

	if (code < 0)
	    return_error(code);
	dev->is_open = true;
	return 1;
    }
}

/* Set device parameters, updating a graphics state or imager state. */
int
gs_imager_putdeviceparams(gs_imager_state *pis, gx_device *dev,
			  gs_param_list *plist)
{
    int code = gs_putdeviceparams(dev, plist);

    if (code >= 0)
	gx_set_cmap_procs(pis, dev);
    return code;
}
private void
gs_state_update_device(gs_state *pgs)
{
    gx_set_cmap_procs((gs_imager_state *)pgs, pgs->device);
    gx_unset_dev_color(pgs);
}
int
gs_state_putdeviceparams(gs_state *pgs, gs_param_list *plist)
{
    int code = gs_putdeviceparams(pgs->device, plist);

    if (code >= 0)
	gs_state_update_device(pgs);
    return code;
}

/* Set the device in the graphics state */
int
gs_setdevice(gs_state * pgs, gx_device * dev)
{
    int code = gs_setdevice_no_erase(pgs, dev);

    if (code == 1)
	code = gs_erasepage(pgs);
    return code;
}
int
gs_setdevice_no_erase(gs_state * pgs, gx_device * dev)
{
    int open_code = 0, code;

    /* Initialize the device */
    if (!dev->is_open) {
	gx_device_fill_in_procs(dev);
	if (gs_device_is_memory(dev)) {
	    /* Set the target to the current device. */
	    gx_device *odev = gs_currentdevice_inline(pgs);

	    while (odev != 0 && gs_device_is_memory(odev))
		odev = ((gx_device_memory *)odev)->target;
	    gx_device_set_target(((gx_device_forward *)dev), odev);
	}
	code = open_code = gs_opendevice(dev);
	if (code < 0)
	    return code;
    }
    gs_setdevice_no_init(pgs, dev);
    pgs->ctm_default_set = false;
    if ((code = gs_initmatrix(pgs)) < 0 ||
	(code = gs_initclip(pgs)) < 0
	)
	return code;
    /* If we were in a charpath or a setcachedevice, */
    /* we aren't any longer. */
    pgs->in_cachedevice = 0;
    pgs->in_charpath = (gs_char_path_mode) 0;
    return open_code;
}
int
gs_setdevice_no_init(gs_state * pgs, gx_device * dev)
{
    /*
     * Just set the device, possibly changing color space but no other
     * device parameters.
     */
    rc_assign(pgs->device, dev, "gs_setdevice_no_init");
    gs_state_update_device(pgs);
    return 0;
}

/* Initialize a just-allocated device. */
void
gx_device_init(gx_device * dev, const gx_device * proto, gs_memory_t * mem,
	       bool internal)
{
    memcpy(dev, proto, proto->params_size);
    dev->memory = mem;
    dev->retained = !internal;
    rc_init(dev, mem, (internal ? 0 : 1));
}

/* Make a null device. */
void
gs_make_null_device(gx_device_null *dev_null, gx_device *dev,
		    gs_memory_t * mem)
{
    gx_device_init((gx_device *)dev_null, (const gx_device *)&gs_null_device,
		   mem, true);
    gx_device_set_target((gx_device_forward *)dev_null, dev);
    if (dev)
	gx_device_copy_color_params((gx_device *)dev_null, dev);
}

/* Mark a device as retained or not retained. */
void
gx_device_retain(gx_device *dev, bool retained)
{
    int delta = (int)retained - (int)dev->retained;

    if (delta) {
	dev->retained = retained; /* do first in case dev is freed */
	rc_adjust_only(dev, delta, "gx_device_retain");
    }
}

/* Select a null device. */
int
gs_nulldevice(gs_state * pgs)
{
    if (pgs->device == 0 || !gx_device_is_null(pgs->device)) {
	gx_device *ndev;
	int code = gs_copydevice(&ndev, (const gx_device *)&gs_null_device,
				 pgs->memory);

	if (code < 0)
	    return code;
	/*
	 * Internal devices have a reference count of 0, not 1,
	 * aside from references from graphics states.
	 */
	rc_init(ndev, pgs->memory, 0);
	return gs_setdevice_no_erase(pgs, ndev);
    }
    return 0;
}

/* Close a device.  The client is responsible for ensuring that */
/* this device is not current in any graphics state. */
int
gs_closedevice(gx_device * dev)
{
    int code = 0;

    if (dev->is_open) {
	code = (*dev_proc(dev, close_device))(dev);
	if (code < 0)
	    return_error(code);
	dev->is_open = false;
    }
    return code;
}

/*
 * Just set the device without any reinitializing.
 * (For internal use only.)
 */
void
gx_set_device_only(gs_state * pgs, gx_device * dev)
{
    rc_assign(pgs->device, dev, "gx_set_device_only");
}

/* Compute the size of one scan line for a device, */
/* with or without padding to a word boundary. */
uint
gx_device_raster(const gx_device * dev, bool pad)
{
    ulong bits = (ulong) dev->width * dev->color_info.depth;

    return (pad ? bitmap_raster(bits) : (uint) ((bits + 7) >> 3));
}

/* Adjust the resolution for devices that only have a fixed set of */
/* geometries, so that the apparent size in inches remains constant. */
/* If fit=1, the resolution is adjusted so that the entire image fits; */
/* if fit=0, one dimension fits, but the other one is clipped. */
int
gx_device_adjust_resolution(gx_device * dev,
			    int actual_width, int actual_height, int fit)
{
    double width_ratio = (double)actual_width / dev->width;
    double height_ratio = (double)actual_height / dev->height;
    double ratio =
    (fit ? min(width_ratio, height_ratio) :
     max(width_ratio, height_ratio));

    dev->HWResolution[0] *= ratio;
    dev->HWResolution[1] *= ratio;
    gx_device_set_width_height(dev, actual_width, actual_height);
    return 0;
}

/* Set the HWMargins to values defined in inches. */
/* If move_origin is true, also reset the Margins. */
/* Note that this assumes a printer-type device (Y axis inverted). */
void
gx_device_set_margins(gx_device * dev, const float *margins /*[4] */ ,
		      bool move_origin)
{
    int i;

    for (i = 0; i < 4; ++i)
	dev->HWMargins[i] = margins[i] * 72.0;
    if (move_origin) {
	dev->Margins[0] = -margins[0] * dev->MarginsHWResolution[0];
	dev->Margins[1] = -margins[3] * dev->MarginsHWResolution[1];
    }
}

/* Set the width and height, updating MediaSize to remain consistent. */
void
gx_device_set_width_height(gx_device * dev, int width, int height)
{
    dev->width = width;
    dev->height = height;
    dev->MediaSize[0] = width * 72.0 / dev->HWResolution[0];
    dev->MediaSize[1] = height * 72.0 / dev->HWResolution[1];
}

/* Set the resolution, updating width and height to remain consistent. */
void
gx_device_set_resolution(gx_device * dev, floatp x_dpi, floatp y_dpi)
{
    dev->HWResolution[0] = x_dpi;
    dev->HWResolution[1] = y_dpi;
    dev->width = dev->MediaSize[0] * x_dpi / 72.0 + 0.5;
    dev->height = dev->MediaSize[1] * y_dpi / 72.0 + 0.5;
}

/* Set the MediaSize, updating width and height to remain consistent. */
void
gx_device_set_media_size(gx_device * dev, floatp media_width, floatp media_height)
{
    dev->MediaSize[0] = media_width;
    dev->MediaSize[1] = media_height;
    dev->width = media_width * dev->HWResolution[0] / 72.0 + 0.499;
    dev->height = media_height * dev->HWResolution[1] / 72.0 + 0.499;
}

/*
 * Copy the color mapping procedures from the target if they are
 * standard ones (saving a level of procedure call at mapping time).
 */
void
gx_device_copy_color_procs(gx_device *dev, const gx_device *target)
{
    dev_proc_map_cmyk_color((*from_cmyk)) =
	dev_proc(dev, map_cmyk_color);
    dev_proc_map_rgb_color((*from_rgb)) =
	dev_proc(dev, map_rgb_color);
    dev_proc_map_color_rgb((*to_rgb)) =
	dev_proc(dev, map_color_rgb);

    if (from_cmyk == gx_forward_map_cmyk_color ||
	from_cmyk == cmyk_1bit_map_cmyk_color ||
	from_cmyk == cmyk_8bit_map_cmyk_color) {
	from_cmyk = dev_proc(target, map_cmyk_color);
	set_dev_proc(dev, map_cmyk_color,
		     (from_cmyk == cmyk_1bit_map_cmyk_color ||
		      from_cmyk == cmyk_8bit_map_cmyk_color ?
		      from_cmyk : gx_forward_map_cmyk_color));
    }
    if (from_rgb == gx_forward_map_rgb_color ||
	from_rgb == gx_default_rgb_map_rgb_color) {
	from_rgb = dev_proc(target, map_rgb_color);
	set_dev_proc(dev, map_rgb_color,
		     (from_rgb == gx_default_rgb_map_rgb_color ?
		      from_rgb : gx_forward_map_rgb_color));
    }
    if (to_rgb == gx_forward_map_color_rgb ||
	to_rgb == cmyk_1bit_map_color_rgb ||
	to_rgb == cmyk_8bit_map_color_rgb) {
	to_rgb = dev_proc(target, map_color_rgb);
	set_dev_proc(dev, map_color_rgb,
		     (to_rgb == cmyk_1bit_map_color_rgb ||
		      to_rgb == cmyk_8bit_map_color_rgb ?
		      to_rgb : gx_forward_map_color_rgb));
    }
}

#define COPY_PARAM(p) dev->p = target->p

/*
 * Copy the color-related device parameters back from the target:
 * color_info and color mapping procedures.
 */
void
gx_device_copy_color_params(gx_device *dev, const gx_device *target)
{
	COPY_PARAM(color_info);
	COPY_PARAM(cached_colors);
	gx_device_copy_color_procs(dev, target);
}

/*
 * Copy device parameters back from a target.  This copies all standard
 * parameters related to page size and resolution, plus color_info
 * and (if appropriate) color mapping procedures.
 */
void
gx_device_copy_params(gx_device *dev, const gx_device *target)
{
#define COPY_ARRAY_PARAM(p) memcpy(dev->p, target->p, sizeof(dev->p))
	COPY_PARAM(width);
	COPY_PARAM(height);
	COPY_ARRAY_PARAM(MediaSize);
	COPY_ARRAY_PARAM(ImagingBBox);
	COPY_PARAM(ImagingBBox_set);
	COPY_ARRAY_PARAM(HWResolution);
	COPY_ARRAY_PARAM(MarginsHWResolution);
	COPY_ARRAY_PARAM(Margins);
	COPY_ARRAY_PARAM(HWMargins);
	COPY_PARAM(PageCount);
#undef COPY_ARRAY_PARAM
	gx_device_copy_color_params(dev, target);
}

#undef COPY_PARAM

/*
 * Parse the output file name for a device, recognizing "-" and "|command",
 * and also detecting and validating any %nnd format for inserting the
 * page count.  If a format is present, store a pointer to its last
 * character in *pfmt, otherwise store 0 there.  Note that an empty name
 * is currently allowed.
 */
int
gx_parse_output_file_name(gs_parsed_file_name_t *pfn, const char **pfmt,
			  const char *fname, uint fnlen)
{
    int code = gs_parse_file_name(pfn, fname, fnlen);
    bool have_format = false, field = 0;
    int width[2], int_width = sizeof(int) * 3, w = 0;
    uint i;

    *pfmt = 0;
    if (fnlen == 0) {		/* allow null name */
	pfn->memory = 0;
	pfn->iodev = NULL;
	pfn->fname = 0;		/* irrelevant since length = 0 */
	pfn->len = 0;
	return 0;
    }
    if (code < 0)
	return code;
    if (!pfn->iodev) {
	if (!strcmp(pfn->fname, "-")) {
	    pfn->iodev = gs_findiodevice((const byte *)"%stdout", 7);
	    pfn->fname = NULL;
	} else if (pfn->fname[0] == '|') {
	    pfn->iodev = gs_findiodevice((const byte *)"%pipe", 5);
	    pfn->fname++, pfn->len--;
	} else
	    pfn->iodev = iodev_default;
	if (!pfn->iodev)
	    return_error(gs_error_undefinedfilename);
    }
    if (!pfn->fname)
	return 0;
    /* Scan the file name for a format string, and validate it if present. */
    width[0] = width[1] = 0;
    for (i = 0; i < pfn->len; ++i)
	if (pfn->fname[i] == '%') {
	    if (i + 1 < pfn->len && pfn->fname[i + 1] == '%')
		continue;
	    if (have_format)	/* more than one % */
		return_error(gs_error_rangecheck);
	    have_format = true;
	sw:
	    if (++i == pfn->len)
		return_error(gs_error_rangecheck);
	    switch (pfn->fname[i]) {
		case 'l':
		    int_width = sizeof(long) * 3;
		case ' ': case '#': case '+': case '-':
		    goto sw;
		case '.':
		    if (field)
			return_error(gs_error_rangecheck);
		    field = 1;
		    continue;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		    width[field] = width[field] * 10 + pfn->fname[i] - '0';
		    goto sw;
		case 'd': case 'i': case 'u': case 'o': case 'x': case 'X':
		    *pfmt = &pfn->fname[i];
		    continue;
		default:
		    return_error(gs_error_rangecheck);
	    }
	}
    if (have_format) {
	/* Calculate a conservative maximum width. */
	w = max(width[0], width[1]);
	w = max(w, int_width) + 5;
    }
    if (strlen(pfn->iodev->dname) + pfn->len + w >= gp_file_name_sizeof)
	return_error(gs_error_rangecheck);
    return 0;
}

/* Open the output file for a device. */
int
gx_device_open_output_file(const gx_device * dev, char *fname,
			   bool binary, bool positionable, FILE ** pfile)
{
    gs_parsed_file_name_t parsed;
    const char *fmt;
    char pfname[gp_file_name_sizeof];
    int code = gx_parse_output_file_name(&parsed, &fmt, fname, strlen(fname));

    if (code < 0)
	return code;
    if (parsed.iodev && !strcmp(parsed.iodev->dname, "%stdout%")) {
	if (parsed.fname)
	    return_error(gs_error_undefinedfilename);
	*pfile = stdout;
	/* Force stdout to binary. */
	return gp_setmode_binary(*pfile, true);
    }
    if (fmt) {
	long count1 = dev->PageCount + 1;

	while (*fmt != 'l' && *fmt != '%')
	    --fmt;
	if (*fmt == 'l')
	    sprintf(pfname, parsed.fname, count1);
	else
	    sprintf(pfname, parsed.fname, (int)count1);
	parsed.fname = pfname;
	parsed.len = strlen(parsed.fname);
    }
    if (positionable || (parsed.iodev && parsed.iodev != iodev_default)) {
	char fmode[4];

	if (!parsed.fname)
	    return_error(gs_error_undefinedfilename);
	strcpy(fmode, gp_fmode_wb);
	if (positionable)
	    strcat(fmode, "+");
	code = parsed.iodev->procs.fopen(parsed.iodev, parsed.fname, fmode,
					 pfile, NULL, 0);
	if (*pfile)
	    return 0;
    }
    *pfile = gp_open_printer((fmt ? pfname : fname), binary);
    if (*pfile)
	return 0;
    return_error(gs_error_invalidfileaccess);
}

/* Close the output file for a device. */
int
gx_device_close_output_file(const gx_device * dev, const char *fname,
			    FILE *file)
{
    gs_parsed_file_name_t parsed;
    const char *fmt;
    int code = gx_parse_output_file_name(&parsed, &fmt, fname, strlen(fname));

    if (code < 0)
	return code;
    if (parsed.iodev) {
	if (!strcmp(parsed.iodev->dname, "%stdout%"))
	    return 0;
	/* NOTE: fname is unsubstituted if the name has any %nnd formats. */
	if (parsed.iodev != iodev_default)
	    return parsed.iodev->procs.fclose(parsed.iodev, file);
    }
    gp_close_printer(file, (parsed.fname ? parsed.fname : fname));
    return 0;
}

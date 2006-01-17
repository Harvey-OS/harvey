/* Copyright (C) 1990, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevprn.c,v 1.20 2005/05/27 05:43:24 dan Exp $ */
/* Generic printer driver support */
#include "ctype_.h"
#include "gdevprn.h"
#include "gp.h"
#include "gsdevice.h"		/* for gs_deviceinitialmatrix */
#include "gsfname.h"
#include "gsparam.h"
#include "gxclio.h"
#include "gxgetbit.h"
#include "gdevplnx.h"
#include "gstrans.h"

/*#define DEBUGGING_HACKS*/

/* GC information */
#define PRINTER_IS_CLIST(pdev) ((pdev)->buffer_space != 0)
private
ENUM_PTRS_WITH(device_printer_enum_ptrs, gx_device_printer *pdev)
    if (PRINTER_IS_CLIST(pdev))
	ENUM_PREFIX(st_device_clist, 0);
    else
	ENUM_PREFIX(st_device_forward, 0);
ENUM_PTRS_END
private
RELOC_PTRS_WITH(device_printer_reloc_ptrs, gx_device_printer *pdev)
{
    if (PRINTER_IS_CLIST(pdev))
	RELOC_PREFIX(st_device_clist);
    else
	RELOC_PREFIX(st_device_forward);
} RELOC_PTRS_END
public_st_device_printer();

/* ---------------- Standard device procedures ---------------- */

/* Define the standard printer procedure vector. */
const gx_device_procs prn_std_procs =
    prn_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close);

/* Forward references */
int gdev_prn_maybe_realloc_memory(gx_device_printer *pdev, 
				  gdev_prn_space_params *old_space,
			          int old_width, int old_height,
			          bool old_page_uses_transparency);

/* ------ Open/close ------ */

/* Open a generic printer device. */
/* Specific devices may wish to extend this. */
int
gdev_prn_open(gx_device * pdev)
{
    gx_device_printer * const ppdev = (gx_device_printer *)pdev;
    int code;

    ppdev->file = NULL;
    code = gdev_prn_allocate_memory(pdev, NULL, 0, 0);
    if (code < 0)
	return code;
    if (ppdev->OpenOutputFile)
	code = gdev_prn_open_printer(pdev, 1);
    return code;
}

/* Generic closing for the printer device. */
/* Specific devices may wish to extend this. */
int
gdev_prn_close(gx_device * pdev)
{
    gx_device_printer * const ppdev = (gx_device_printer *)pdev;
    int code = 0;

    gdev_prn_free_memory(pdev);
    if (ppdev->file != NULL) {
	code = gx_device_close_output_file(pdev, ppdev->fname, ppdev->file);
	ppdev->file = NULL;
    }
    return code;
}

private int		/* returns 0 ok, else -ve error cde */
gdev_prn_setup_as_command_list(gx_device *pdev, gs_memory_t *buffer_memory,
			       byte **the_memory,
			       const gdev_prn_space_params *space_params,
			       bool bufferSpace_is_exact)
{
    gx_device_printer * const ppdev = (gx_device_printer *)pdev;
    uint space;
    int code;
    gx_device_clist *const pclist_dev = (gx_device_clist *)pdev;
    gx_device_clist_common * const pcldev = &pclist_dev->common;
    bool reallocate = *the_memory != 0;
    byte *base;

    /* Try to allocate based simply on param-requested buffer size */
#ifdef DEBUGGING_HACKS
#define BACKTRACE(first_arg)\
  BEGIN\
    ulong *fp_ = (ulong *)&first_arg - 2;\
    for (; fp_ && (fp_[1] & 0xff000000) == 0x08000000; fp_ = (ulong *)*fp_)\
	dprintf2("  fp=0x%lx ip=0x%lx\n", (ulong)fp_, fp_[1]);\
  END
dputs("alloc buffer:\n");
BACKTRACE(pdev);
#endif /*DEBUGGING_HACKS*/
    for ( space = space_params->BufferSpace; ; ) {
	base = (reallocate ?
		(byte *)gs_resize_object(buffer_memory, *the_memory, space,
					 "cmd list buffer") :
		gs_alloc_bytes(buffer_memory, space,
			       "cmd list buffer"));
	if (base != 0)
	    break;
	if (bufferSpace_is_exact || (space >>= 1) < PRN_MIN_BUFFER_SPACE)
	    break;
    }
    if (base == 0)
	return_error(gs_error_VMerror);
    *the_memory = base;

    /* Try opening the command list, to see if we allocated */
    /* enough buffer space. */
open_c:
    ppdev->buf = base;
    ppdev->buffer_space = space;
    clist_init_params(pclist_dev, base, space, pdev,
		      ppdev->printer_procs.buf_procs,
		      space_params->band, ppdev->is_async_renderer,
		      (ppdev->bandlist_memory == 0 ? pdev->memory->non_gc_memory:
		       ppdev->bandlist_memory),
		      ppdev->free_up_bandlist_memory,
		      ppdev->clist_disable_mask,
		      ppdev->page_uses_transparency);
    code = (*gs_clist_device_procs.open_device)( (gx_device *)pcldev );
    if (code < 0) {
	/* If there wasn't enough room, and we haven't */
	/* already shrunk the buffer, try enlarging it. */
	if ( code == gs_error_limitcheck &&
	     space >= space_params->BufferSpace &&
	     !bufferSpace_is_exact
	     ) {
	    space <<= 1;
	    if (reallocate) {
		base = gs_resize_object(buffer_memory, 
					*the_memory, space,
					"cmd list buf(retry open)");
		if (base != 0)
		    *the_memory = base;
	    } else {
		gs_free_object(buffer_memory, base,
			       "cmd list buf(retry open)");
		*the_memory = base =
		    gs_alloc_bytes(buffer_memory, space,
				   "cmd list buf(retry open)");
	    }
	    ppdev->buf = *the_memory;
	    if (base != 0)
		goto open_c;
	}
	/* Failure. */
	if (!reallocate) {
	    gs_free_object(buffer_memory, base, "cmd list buf");
	    ppdev->buffer_space = 0;
	    *the_memory = 0;
	}
    }
    return code;
}

private bool	/* ret true if device was cmd list, else false */
gdev_prn_tear_down(gx_device *pdev, byte **the_memory)
{
    gx_device_printer * const ppdev = (gx_device_printer *)pdev;
    gx_device_memory * const pmemdev = (gx_device_memory *)pdev;
    gx_device_clist *const pclist_dev = (gx_device_clist *)pdev;
    gx_device_clist_common * const pcldev = &pclist_dev->common;
    bool is_command_list;

    if (ppdev->buffer_space != 0) {
	/* Close cmd list device & point to the storage */
	(*gs_clist_device_procs.close_device)( (gx_device *)pcldev );
	*the_memory = ppdev->buf;
	ppdev->buf = 0;
	ppdev->buffer_space = 0;
	is_command_list = true;
    } else {
	/* point at the device bitmap, no need to close mem dev */
	*the_memory = pmemdev->base;
	pmemdev->base = 0;
	is_command_list = false;
    }

    /* Reset device proc vector to default */
    if (ppdev->orig_procs.open_device != 0)
	pdev->procs = ppdev->orig_procs;
    ppdev->orig_procs.open_device = 0;	/* prevent uninit'd restore of procs */

    return is_command_list;
}

private int
gdev_prn_allocate(gx_device *pdev, gdev_prn_space_params *new_space_params,
		  int new_width, int new_height, bool reallocate)
{
    gx_device_printer * const ppdev = (gx_device_printer *)pdev;
    gx_device_memory * const pmemdev = (gx_device_memory *)pdev;
    byte *the_memory = 0;
    gdev_prn_space_params save_params;
    int save_width = 0x0badf00d; /* Quiet compiler */
    int save_height = 0x0badf00d; /* Quiet compiler */
    bool is_command_list = false; /* Quiet compiler */
    bool save_is_command_list = false; /* Quiet compiler */
    int ecode = 0;
    int pass;
    gs_memory_t *buffer_memory =
	(ppdev->buffer_memory == 0 ? pdev->memory->non_gc_memory :
	 ppdev->buffer_memory);

    /* If reallocate, find allocated memory & tear down buffer device */
    if (reallocate)
	save_is_command_list = gdev_prn_tear_down(pdev, &the_memory);

    /* Re/allocate memory */
    ppdev->orig_procs = pdev->procs;
    for ( pass = 1; pass <= (reallocate ? 2 : 1); ++pass ) {
	ulong mem_space;
	ulong pdf14_trans_buffer_size = 0;
	byte *base = 0;
	bool bufferSpace_is_default = false;
	gdev_prn_space_params space_params;
	gx_device_buf_space_t buf_space;

	if (reallocate)
	    switch (pass)
	        {
		case 1:
		    /* Setup device to get reallocated */
		    save_params = ppdev->space_params;
		    ppdev->space_params = *new_space_params;
		    save_width = ppdev->width;
		    ppdev->width = new_width;
		    save_height = ppdev->height;
		    ppdev->height = new_height;
		    break;
		case 2:	/* only comes here if reallocate */
		    /* Restore device to previous contents */
		    ppdev->space_params = save_params;
		    ppdev->width = save_width;
		    ppdev->height = save_height;
		    break;
	        }

	/* Init clist/mem device-specific fields */
	memset(ppdev->skip, 0, sizeof(ppdev->skip));
	ppdev->printer_procs.buf_procs.size_buf_device
	    (&buf_space, pdev, NULL, pdev->height, false);
	if (ppdev->page_uses_transparency)
	    pdf14_trans_buffer_size = new_height
		* (ESTIMATED_PDF14_ROW_SPACE(new_width) >> 3);
	mem_space = buf_space.bits + buf_space.line_ptrs
		    + pdf14_trans_buffer_size;

	/* Compute desired space params: never use the space_params as-is. */
	/* Rather, give the dev-specific driver a chance to adjust them. */
	space_params = ppdev->space_params;
	space_params.BufferSpace = 0;
	(*ppdev->printer_procs.get_space_params)(ppdev, &space_params);
	if (ppdev->is_async_renderer && space_params.band.BandBufferSpace != 0)
	    space_params.BufferSpace = space_params.band.BandBufferSpace;
	else if (space_params.BufferSpace == 0) {
	    if (space_params.band.BandBufferSpace > 0)
	        space_params.BufferSpace = space_params.band.BandBufferSpace;
	    else {
		space_params.BufferSpace = ppdev->space_params.BufferSpace;
		bufferSpace_is_default = true;
	    }
	}

	/* Determine if we can use a full bitmap buffer, or have to use banding */
	if (pass > 1)
	    is_command_list = save_is_command_list;
	else {
	    is_command_list = space_params.banding_type == BandingAlways ||
		mem_space >= space_params.MaxBitmap ||
		mem_space != (uint)mem_space;	    /* too big to allocate */
	}
	if (!is_command_list) {
	    /* Try to allocate memory for full memory buffer */
	    base =
		(reallocate ?
		 (byte *)gs_resize_object(buffer_memory, the_memory,
					  (uint)mem_space, "printer buffer") :
		 gs_alloc_bytes(buffer_memory, (uint)mem_space,
				"printer_buffer"));
	    if (base == 0)
		is_command_list = true;
	    else
		the_memory = base;
	}
	if (!is_command_list && pass == 1 && PRN_MIN_MEMORY_LEFT != 0
	    && buffer_memory == pdev->memory->non_gc_memory) {
	    /* before using full memory buffer, ensure enough working mem left */
	    byte * left = gs_alloc_bytes( buffer_memory,
					  PRN_MIN_MEMORY_LEFT, "printer mem left");
	    if (left == 0)
		is_command_list = true;
	    else
		gs_free_object(buffer_memory, left, "printer mem left");
	}

	if (is_command_list) {
	    /* Buffer the image in a command list. */
	    /* Release the buffer if we allocated it. */
	    int code;
	    if (!reallocate) {
		gs_free_object(buffer_memory, the_memory,
			       "printer buffer(open)");
		the_memory = 0;
	    }
	    if (space_params.banding_type == BandingNever) {
		ecode = gs_note_error(gs_error_VMerror);
		continue;
	    }
	    code = gdev_prn_setup_as_command_list(pdev, buffer_memory,
						  &the_memory, &space_params,
						  !bufferSpace_is_default);
	    if (ecode == 0)
		ecode = code;

	    if ( code >= 0 || (reallocate && pass > 1) )
		ppdev->procs = gs_clist_device_procs;
	} else {
	    /* Render entirely in memory. */
	    gx_device *bdev = (gx_device *)pmemdev;
	    int code;

	    ppdev->buffer_space = 0;
	    if ((code = gdev_create_buf_device
		 (ppdev->printer_procs.buf_procs.create_buf_device,
		  &bdev, pdev, NULL, NULL, false)) < 0 ||
		(code = ppdev->printer_procs.buf_procs.setup_buf_device
		 (bdev, base, buf_space.raster,
		  (byte **)(base + buf_space.bits), 0, pdev->height,
		  pdev->height)) < 0
		) {
		/* Catastrophic. Shouldn't ever happen */
		gs_free_object(buffer_memory, base, "printer buffer");
		pdev->procs = ppdev->orig_procs;
		ppdev->orig_procs.open_device = 0;	/* prevent uninit'd restore of procs */
		return_error(code);
	    }
	    pmemdev->base = base;
	}
	if (ecode == 0)
	    break;
    }

    if (ecode >= 0 || reallocate) {	/* even if realloc failed */
	/* Synthesize the procedure vector. */
	/* Rendering operations come from the memory or clist device, */
	/* non-rendering come from the printer device. */
#define COPY_PROC(p) set_dev_proc(ppdev, p, ppdev->orig_procs.p)
	COPY_PROC(get_initial_matrix);
	COPY_PROC(output_page);
	COPY_PROC(close_device);
	COPY_PROC(map_rgb_color);
	COPY_PROC(map_color_rgb);
	COPY_PROC(get_params);
	COPY_PROC(put_params);
	COPY_PROC(map_cmyk_color);
	COPY_PROC(get_xfont_procs);
	COPY_PROC(get_xfont_device);
	COPY_PROC(map_rgb_alpha_color);
	/* All printers are page devices, even if they didn't use the */
	/* standard macros for generating their procedure vectors. */
	set_dev_proc(ppdev, get_page_device, gx_page_device_get_page_device);
	COPY_PROC(get_clipping_box);
	COPY_PROC(map_color_rgb_alpha);
	COPY_PROC(get_hardware_params);
	COPY_PROC(get_color_mapping_procs);
	COPY_PROC(get_color_comp_index);
	COPY_PROC(encode_color);
	COPY_PROC(decode_color);
	COPY_PROC(begin_image);
	COPY_PROC(text_begin);
	COPY_PROC(fill_path);
	COPY_PROC(stroke_path);
	COPY_PROC(fill_rectangle_hl_color);
	COPY_PROC(update_spot_equivalent_colors);
#undef COPY_PROC
	/* If using a command list, already opened the device. */
	if (is_command_list)
	    return ecode;
	else
	    return (*dev_proc(pdev, open_device))(pdev);
    } else {
	pdev->procs = ppdev->orig_procs;
	ppdev->orig_procs.open_device = 0;	/* prevent uninit'd restore of procs */
	return ecode;
    }
}

int
gdev_prn_allocate_memory(gx_device *pdev,
			 gdev_prn_space_params *new_space_params,
			 int new_width, int new_height)
{
    return gdev_prn_allocate(pdev, new_space_params,
			     new_width, new_height, false);
}

int
gdev_prn_reallocate_memory(gx_device *pdev,
			 gdev_prn_space_params *new_space_params,
			 int new_width, int new_height)
{
    return gdev_prn_allocate(pdev, new_space_params,
			     new_width, new_height, true);
}

int
gdev_prn_free_memory(gx_device *pdev)
{
    gx_device_printer * const ppdev = (gx_device_printer *)pdev;
    byte *the_memory = 0;
    gs_memory_t *buffer_memory =
	(ppdev->buffer_memory == 0 ? pdev->memory->non_gc_memory :
	 ppdev->buffer_memory);

    gdev_prn_tear_down(pdev, &the_memory);
    gs_free_object(buffer_memory, the_memory, "gdev_prn_free_memory");
    return 0;
}

/* ------------- Stubs related only to async rendering ------- */

int	/* rets 0 ok, -ve error if couldn't start thread */
gx_default_start_render_thread(gdev_prn_start_render_params *params)
{
    return gs_error_unknownerror;
}

/* Open the renderer's copy of a device. */
/* This is overriden in gdevprna.c */
int
gx_default_open_render_device(gx_device_printer *pdev)
{
    return gs_error_unknownerror;
}

/* Close the renderer's copy of a device. */
int
gx_default_close_render_device(gx_device_printer *pdev)
{
    return gdev_prn_close( (gx_device *)pdev );
}

/* ------ Get/put parameters ------ */

/* Get parameters.  Printer devices add several more parameters */
/* to the default set. */
int
gdev_prn_get_params(gx_device * pdev, gs_param_list * plist)
{
    gx_device_printer * const ppdev = (gx_device_printer *)pdev;
    int code = gx_default_get_params(pdev, plist);
    gs_param_string ofns;

    if (code < 0 ||
	(code = param_write_long(plist, "MaxBitmap", &ppdev->space_params.MaxBitmap)) < 0 ||
	(code = param_write_long(plist, "BufferSpace", &ppdev->space_params.BufferSpace)) < 0 ||
	(code = param_write_int(plist, "BandWidth", &ppdev->space_params.band.BandWidth)) < 0 ||
	(code = param_write_int(plist, "BandHeight", &ppdev->space_params.band.BandHeight)) < 0 ||
	(code = param_write_long(plist, "BandBufferSpace", &ppdev->space_params.band.BandBufferSpace)) < 0 ||
	(code = param_write_bool(plist, "OpenOutputFile", &ppdev->OpenOutputFile)) < 0 ||
	(code = param_write_bool(plist, "ReopenPerPage", &ppdev->ReopenPerPage)) < 0 ||
	(code = param_write_bool(plist, "PageUsesTransparency",
			 	&ppdev->page_uses_transparency)) < 0 ||
	(ppdev->Duplex_set >= 0 &&
	 (code = (ppdev->Duplex_set ?
		  param_write_bool(plist, "Duplex", &ppdev->Duplex) :
		  param_write_null(plist, "Duplex"))) < 0)
	)
	return code;

    ofns.data = (const byte *)ppdev->fname,
	ofns.size = strlen(ppdev->fname),
	ofns.persistent = false;
    return param_write_string(plist, "OutputFile", &ofns);
}

/* Validate an OutputFile name by checking any %-formats. */
private int
validate_output_file(const gs_param_string * ofs)
{
    gs_parsed_file_name_t parsed;
    const char *fmt;

    return gx_parse_output_file_name(&parsed, &fmt, (const char *)ofs->data,
				     ofs->size) >= 0;
}

/* Put parameters. */
int
gdev_prn_put_params(gx_device * pdev, gs_param_list * plist)
{
    gx_device_printer * const ppdev = (gx_device_printer *)pdev;
    int ecode = 0;
    int code;
    const char *param_name;
    bool is_open = pdev->is_open;
    bool oof = ppdev->OpenOutputFile;
    bool rpp = ppdev->ReopenPerPage;
    bool page_uses_transparency = ppdev->page_uses_transparency;
    bool old_page_uses_transparency = ppdev->page_uses_transparency;
    bool duplex;
    int duplex_set = -1;
    int width = pdev->width;
    int height = pdev->height;
    gdev_prn_space_params sp, save_sp;
    gs_param_string ofs;
    gs_param_dict mdict;

    sp = ppdev->space_params;
    save_sp = sp;

    switch (code = param_read_bool(plist, (param_name = "OpenOutputFile"), &oof)) {
	default:
	    ecode = code;
	    param_signal_error(plist, param_name, ecode);
	case 0:
	case 1:
	    break;
    }

    switch (code = param_read_bool(plist, (param_name = "ReopenPerPage"), &rpp)) {
	default:
	    ecode = code;
	    param_signal_error(plist, param_name, ecode);
	case 0:
	case 1:
	    break;
    }

    switch (code = param_read_bool(plist, (param_name = "PageUsesTransparency"),
			    				&page_uses_transparency)) {
	default:
	    ecode = code;
	    param_signal_error(plist, param_name, ecode);
	case 0:
	case 1:
	    break;
    }

    if (ppdev->Duplex_set >= 0)	/* i.e., Duplex is supported */
	switch (code = param_read_bool(plist, (param_name = "Duplex"),
				       &duplex)) {
	    case 0:
		duplex_set = 1;
		break;
	    default:
		if ((code = param_read_null(plist, param_name)) == 0) {
		    duplex_set = 0;
		    break;
		}
		ecode = code;
		param_signal_error(plist, param_name, ecode);
	    case 1:
		;
	}
#define CHECK_PARAM_CASES(member, bad, label)\
    case 0:\
	if ((sp.params_are_read_only ? sp.member != save_sp.member : bad))\
	    ecode = gs_error_rangecheck;\
	else\
	    break;\
	goto label;\
    default:\
	ecode = code;\
label:\
	param_signal_error(plist, param_name, ecode);\
    case 1:\
	break

    switch (code = param_read_long(plist, (param_name = "MaxBitmap"), &sp.MaxBitmap)) {
	CHECK_PARAM_CASES(MaxBitmap, sp.MaxBitmap < 10000, mbe);
    }

    switch (code = param_read_long(plist, (param_name = "BufferSpace"), &sp.BufferSpace)) {
	CHECK_PARAM_CASES(BufferSpace, sp.BufferSpace < 10000, bse);
    }

    switch (code = param_read_int(plist, (param_name = "BandWidth"), &sp.band.BandWidth)) {
	CHECK_PARAM_CASES(band.BandWidth, sp.band.BandWidth < 0, bwe);
    }

    switch (code = param_read_int(plist, (param_name = "BandHeight"), &sp.band.BandHeight)) {
	CHECK_PARAM_CASES(band.BandHeight, sp.band.BandHeight < 0, bhe);
    }

    switch (code = param_read_long(plist, (param_name = "BandBufferSpace"), &sp.band.BandBufferSpace)) {
	CHECK_PARAM_CASES(band.BandBufferSpace, sp.band.BandBufferSpace < 0, bbse);
    }

    switch (code = param_read_string(plist, (param_name = "OutputFile"), &ofs)) {
	case 0:
	    if (pdev->LockSafetyParams &&
		    bytes_compare(ofs.data, ofs.size,
			(const byte *)ppdev->fname, strlen(ppdev->fname))) {
	        code = gs_error_invalidaccess;
	    }
	    else
		code = validate_output_file(&ofs);
	    if (code >= 0)
		break;
	    /* falls through */
	default:
	    ecode = code;
	    param_signal_error(plist, param_name, ecode);
	case 1:
	    ofs.data = 0;
	    break;
    }

    /* Read InputAttributes and OutputAttributes just for the type */
    /* check and to indicate that they aren't undefined. */
#define read_media(pname)\
	switch ( code = param_begin_read_dict(plist, (param_name = pname), &mdict, true) )\
	  {\
	  case 0:\
		param_end_read_dict(plist, pname, &mdict);\
		break;\
	  default:\
		ecode = code;\
		param_signal_error(plist, param_name, ecode);\
	  case 1:\
		;\
	  }

    read_media("InputAttributes");
    read_media("OutputAttributes");

    if (ecode < 0)
	return ecode;
    /* Prevent gx_default_put_params from closing the printer. */
    pdev->is_open = false;
    code = gx_default_put_params(pdev, plist);
    pdev->is_open = is_open;
    if (code < 0)
	return code;

    ppdev->OpenOutputFile = oof;
    ppdev->ReopenPerPage = rpp;
    ppdev->page_uses_transparency = page_uses_transparency;
    if (duplex_set >= 0) {
	ppdev->Duplex = duplex;
	ppdev->Duplex_set = duplex_set;
    }
    ppdev->space_params = sp;

    /* If necessary, free and reallocate the printer memory. */
    /* Formerly, would not reallocate if device is not open: */
    /* we had to patch this out (see News for 5.50). */
    code = gdev_prn_maybe_realloc_memory(ppdev, &save_sp, width, height,
		    				old_page_uses_transparency);
    if (code < 0)
	return code;

    /* If filename changed, close file. */
    if (ofs.data != 0 &&
	bytes_compare(ofs.data, ofs.size,
		      (const byte *)ppdev->fname, strlen(ppdev->fname))
	) {
	/* Close the file if it's open. */
	if (ppdev->file != NULL)
	    gx_device_close_output_file(pdev, ppdev->fname, ppdev->file);
	ppdev->file = NULL;
	memcpy(ppdev->fname, ofs.data, ofs.size);
	ppdev->fname[ofs.size] = 0;
    }
    /* If the device is open and OpenOutputFile is true, */
    /* open the OutputFile now.  (If the device isn't open, */
    /* this will happen when it is opened.) */
    if (pdev->is_open && oof) {
	code = gdev_prn_open_printer(pdev, 1);
	if (code < 0)
	    return code;
    }
    return 0;
}

/* ------ Others ------ */

/* Default routine to (not) override current space_params. */
void
gx_default_get_space_params(const gx_device_printer *printer_dev,
			    gdev_prn_space_params *space_params)
{
    return;
}

/* Generic routine to send the page to the printer. */
int	/* 0 ok, -ve error, or 1 if successfully upgraded to buffer_page */
gdev_prn_output_page(gx_device * pdev, int num_copies, int flush)
{
    gx_device_printer * const ppdev = (gx_device_printer *)pdev;
    int outcode = 0, closecode = 0, errcode = 0, endcode;
    bool upgraded_copypage = false;

    if (num_copies > 0 || !flush) {
	int code = gdev_prn_open_printer(pdev, 1);

	if (code < 0)
	    return code;

	/* If copypage request, try to do it using buffer_page */
	if ( !flush &&
	     (*ppdev->printer_procs.buffer_page)
	     (ppdev, ppdev->file, num_copies) >= 0
	     ) {
	    upgraded_copypage = true;
	    flush = true;
	}
	else if (num_copies > 0)
	    /* Print the accumulated page description. */
	    outcode =
		(*ppdev->printer_procs.print_page_copies)(ppdev, ppdev->file,
							  num_copies);
	fflush(ppdev->file);
	errcode =
	    (ferror(ppdev->file) ? gs_note_error(gs_error_ioerror) : 0);
	if (!upgraded_copypage)
	    closecode = gdev_prn_close_printer(pdev);
    }
    endcode = (ppdev->buffer_space && !ppdev->is_async_renderer ?
	       clist_finish_page(pdev, flush) : 0);

    if (outcode < 0)
	return outcode;
    if (errcode < 0)
	return errcode;
    if (closecode < 0)
	return closecode;
    if (endcode < 0)
	return endcode;
    endcode = gx_finish_output_page(pdev, num_copies, flush);
    return (endcode < 0 ? endcode : upgraded_copypage ? 1 : 0);
}

/* Print a single copy of a page by calling print_page_copies. */
int
gx_print_page_single_copy(gx_device_printer * pdev, FILE * prn_stream)
{
    return pdev->printer_procs.print_page_copies(pdev, prn_stream, 1);
}

/* Print multiple copies of a page by calling print_page multiple times. */
int
gx_default_print_page_copies(gx_device_printer * pdev, FILE * prn_stream,
			     int num_copies)
{
    int i = 1;
    int code = 0;

    for (; i < num_copies; ++i) {
	int errcode, closecode;

	code = (*pdev->printer_procs.print_page) (pdev, prn_stream);
	if (code < 0)
	    return code;
	/*
	 * Close and re-open the printer, to reset is_new and do the
	 * right thing if we're producing multiple output files.
	 * Code is mostly copied from gdev_prn_output_page.
	 */
	fflush(pdev->file);
	errcode =
	    (ferror(pdev->file) ? gs_note_error(gs_error_ioerror) : 0);
	closecode = gdev_prn_close_printer((gx_device *)pdev);
	pdev->PageCount++;
	code = (errcode < 0 ? errcode : closecode < 0 ? closecode :
		gdev_prn_open_printer((gx_device *)pdev, true));
	if (code < 0) {
	    pdev->PageCount -= i;
	    return code;
	}
	prn_stream = pdev->file;
    }
    /* Print the last (or only) page. */
    pdev->PageCount -= num_copies - 1;
    return (*pdev->printer_procs.print_page) (pdev, prn_stream);
}

/*
 * Buffer a (partial) rasterized page & optionally print result multiple times.
 * The default implementation returns error, since the driver needs to override
 * this (in procedure vector) in configurations where this call may occur.
 */
int
gx_default_buffer_page(gx_device_printer *pdev, FILE *prn_stream,
		       int num_copies)
{
    return gs_error_unknownerror;
}

/* ---------------- Driver services ---------------- */

/* Initialize a rendering plane specification. */
int
gx_render_plane_init(gx_render_plane_t *render_plane, const gx_device *dev,
		     int index)
{
    /*
     * Eventually the computation of shift and depth from dev and index
     * will be made device-dependent.
     */
    int num_planes = dev->color_info.num_components;
    int plane_depth = dev->color_info.depth / num_planes;

    if (index < 0 || index >= num_planes)
	return_error(gs_error_rangecheck);
    render_plane->index = index;
    render_plane->depth = plane_depth;
    render_plane->shift = plane_depth * (num_planes - 1 - index);
    return 0;
}

/* Clear trailing bits in the last byte of (a) scan line(s). */
void
gdev_prn_clear_trailing_bits(byte *data, uint raster, int height,
			     const gx_device *dev)
{
    int first_bit = dev->width * dev->color_info.depth;

    if (first_bit & 7)
	bits_fill_rectangle(data, first_bit, raster, mono_fill_make_pattern(0),
			    -first_bit & 7, height);
}

/* Return the number of scan lines that should actually be passed */
/* to the device. */
int
gdev_prn_print_scan_lines(gx_device * pdev)
{
    int height = pdev->height;
    gs_matrix imat;
    float yscale;
    int top, bottom, offset, end;

    (*dev_proc(pdev, get_initial_matrix)) (pdev, &imat);
    yscale = imat.yy * 72.0;	/* Y dpi, may be negative */
    top = (int)(dev_t_margin(pdev) * yscale);
    bottom = (int)(dev_b_margin(pdev) * yscale);
    offset = (int)(dev_y_offset(pdev) * yscale);
    if (yscale < 0) {		/* Y=0 is top of page */
	end = -offset + height + bottom;
    } else {			/* Y=0 is bottom of page */
	end = offset + height - top;
    }
    return min(height, end);
}

/* Open the current page for printing. */
int
gdev_prn_open_printer_seekable(gx_device *pdev, bool binary_mode,
			       bool seekable)
{
    gx_device_printer * const ppdev = (gx_device_printer *)pdev;

    if (ppdev->file != 0) {
	ppdev->file_is_new = false;
	return 0;
    }
    {
	int code = gx_device_open_output_file(pdev, ppdev->fname,
					      binary_mode, seekable,
					      &ppdev->file);
	if (code < 0)
	    return code;
    }
    ppdev->file_is_new = true;
    return 0;
}
int
gdev_prn_open_printer(gx_device *pdev, bool binary_mode)
{
    return gdev_prn_open_printer_seekable(pdev, binary_mode, false);
}

/*
 * Test whether the printer's output file was just opened, i.e., whether
 * this is the first page being written to this file.  This is only valid
 * at the entry to a driver's print_page procedure.
 */
bool
gdev_prn_file_is_new(const gx_device_printer *pdev)
{
    return pdev->file_is_new;
}

/* Determine the colors used in a range of lines. */
int
gx_page_info_colors_used(const gx_device *dev,
			 const gx_band_page_info_t *page_info,
			 int y, int height,
			 gx_colors_used_t *colors_used, int *range_start)
{
    int start, end, i;
    int num_lines = page_info->scan_lines_per_colors_used;
    gx_color_index or = 0;
    bool slow_rop = false;

    if (y < 0 || height < 0 || height > dev->height - y)
	return -1;
    start = y / num_lines;
    end = (y + height + num_lines - 1) / num_lines;
    for (i = start; i < end; ++i) {
	or |= page_info->band_colors_used[i].or;
	slow_rop |= page_info->band_colors_used[i].slow_rop;
    }
    colors_used->or = or;
    colors_used->slow_rop = slow_rop;
    *range_start = start * num_lines;
    return min(end * num_lines, dev->height) - *range_start;
}
int
gdev_prn_colors_used(gx_device *dev, int y, int height,
		     gx_colors_used_t *colors_used, int *range_start)
{
    gx_device_clist_writer *cldev;

    /* If this isn't a banded device, return default values. */
    if (dev_proc(dev, get_bits_rectangle) !=
	  gs_clist_device_procs.get_bits_rectangle
	) {
	*range_start = 0;
	colors_used->or = ((gx_color_index)1 << dev->color_info.depth) - 1;
	return dev->height;
    }
    cldev = (gx_device_clist_writer *)dev;
    if (cldev->page_info.scan_lines_per_colors_used == 0) /* not set yet */
	clist_compute_colors_used(cldev);
    return
	gx_page_info_colors_used(dev, &cldev->page_info,
				 y, height, colors_used, range_start);
}

/*
 * Create the buffer device for a printer device.  Clients should always
 * call this, and never call the device procedure directly.
 */
int
gdev_create_buf_device(create_buf_device_proc_t cbd_proc, gx_device **pbdev,
		       gx_device *target,
		       const gx_render_plane_t *render_plane,
		       gs_memory_t *mem, bool for_band)
{
    int code = cbd_proc(pbdev, target, render_plane, mem, for_band);

    if (code < 0)
	return code;
    /* Retain this device -- it will be freed explicitly. */
    gx_device_retain(*pbdev, true);
    return code;
}

/*
 * Create an ordinary memory device for page or band buffering,
 * possibly preceded by a plane extraction device.
 */
int
gx_default_create_buf_device(gx_device **pbdev, gx_device *target,
    const gx_render_plane_t *render_plane, gs_memory_t *mem, bool for_band)
{
    int plane_index = (render_plane ? render_plane->index : -1);
    int depth;
    const gx_device_memory *mdproto;
    gx_device_memory *mdev;
    gx_device *bdev;

    if (plane_index >= 0)
	depth = render_plane->depth;
    else
	depth = target->color_info.depth;
    mdproto = gdev_mem_device_for_bits(depth);
    if (mdproto == 0)
	return_error(gs_error_rangecheck);
    if (mem) {
	mdev = gs_alloc_struct(mem, gx_device_memory, &st_device_memory,
			       "create_buf_device");
	if (mdev == 0)
	    return_error(gs_error_VMerror);
    } else {
	mdev = (gx_device_memory *)*pbdev;
    }
    if (target == (gx_device *)mdev) {
	/* The following is a special hack for setting up printer devices. */
	assign_dev_procs(mdev, mdproto);
        check_device_separable((gx_device *)mdev);
	gx_device_fill_in_procs((gx_device *)mdev);
    } else
	gs_make_mem_device(mdev, mdproto, mem, (for_band ? 1 : 0),
			   (target == (gx_device *)mdev ? NULL : target));
    mdev->width = target->width;
    /*
     * The matrix in the memory device is irrelevant,
     * because all we do with the device is call the device-level
     * output procedures, but we may as well set it to
     * something halfway reasonable.
     */
    gs_deviceinitialmatrix(target, &mdev->initial_matrix);
    if (plane_index >= 0) {
	gx_device_plane_extract *edev =
	    gs_alloc_struct(mem, gx_device_plane_extract,
			    &st_device_plane_extract, "create_buf_device");

	if (edev == 0) {
	    gx_default_destroy_buf_device((gx_device *)mdev);
	    return_error(gs_error_VMerror);
	}
	edev->memory = mem;
	plane_device_init(edev, target, (gx_device *)mdev, render_plane,
			  false);
	bdev = (gx_device *)edev;
    } else
	bdev = (gx_device *)mdev;
    /****** QUESTIONABLE, BUT BETTER THAN OMITTING ******/
    bdev->color_info = target->color_info;
    *pbdev = bdev;
    return 0;
}

/* Determine the space needed by the buffer device. */
int
gx_default_size_buf_device(gx_device_buf_space_t *space, gx_device *target,
			   const gx_render_plane_t *render_plane,
			   int height, bool for_band)
{
    gx_device_memory mdev;

    mdev.color_info.depth =
	(render_plane && render_plane->index >= 0 ? render_plane->depth :
	 target->color_info.depth);
    mdev.width = target->width;
    mdev.num_planes = 0;
    space->bits = gdev_mem_bits_size(&mdev, target->width, height);
    space->line_ptrs = gdev_mem_line_ptrs_size(&mdev, target->width, height);
    space->raster = gdev_mem_raster(&mdev);
    return 0;
}

/* Set up the buffer device. */
int
gx_default_setup_buf_device(gx_device *bdev, byte *buffer, int bytes_per_line,
			    byte **line_ptrs, int y, int setup_height,
			    int full_height)
{
    gx_device_memory *mdev =
	(gs_device_is_memory(bdev) ? (gx_device_memory *)bdev :
	 (gx_device_memory *)(((gx_device_plane_extract *)bdev)->plane_dev));
    byte **ptrs = line_ptrs;
    int raster = bytes_per_line;
    int code;

    /****** HACK ******/
    if ((gx_device *)mdev == bdev && mdev->num_planes)
	raster = bitmap_raster(mdev->planes[0].depth * mdev->width);
    if (ptrs == 0) {
	/*
	 * Allocate line pointers now; free them when we close the device.
	 * Note that for multi-planar devices, we have to allocate using
	 * full_height rather than setup_height.
	 */
	ptrs = (byte **)
	    gs_alloc_byte_array(mdev->memory,
				(mdev->num_planes ?
				 full_height * mdev->num_planes :
				 setup_height),
				sizeof(byte *), "setup_buf_device");
	if (ptrs == 0)
	    return_error(gs_error_VMerror);
	mdev->line_pointer_memory = mdev->memory;
	mdev->foreign_line_pointers = false;
    }
    mdev->height = full_height;
    code = gdev_mem_set_line_ptrs(mdev, buffer + raster * y, bytes_per_line,
				  ptrs, setup_height);
    mdev->height = setup_height;
    bdev->height = setup_height; /* do here in case mdev == bdev */
    return code;
}

/* Destroy the buffer device. */
void
gx_default_destroy_buf_device(gx_device *bdev)
{
    gx_device *mdev = bdev;

    if (!gs_device_is_memory(bdev)) {
	/* bdev must be a plane extraction device. */
	mdev = ((gx_device_plane_extract *)bdev)->plane_dev;
	gs_free_object(bdev->memory, bdev, "destroy_buf_device");
    }
    dev_proc(mdev, close_device)(mdev);
    gs_free_object(mdev->memory, mdev, "destroy_buf_device");
}

/*
 * Copy one or more rasterized scan lines to a buffer, or return a pointer
 * to them.  See gdevprn.h for detailed specifications.
 */
int
gdev_prn_get_lines(gx_device_printer *pdev, int y, int height,
		   byte *buffer, uint bytes_per_line,
		   byte **actual_buffer, uint *actual_bytes_per_line,
		   const gx_render_plane_t *render_plane)
{
    int code;
    gs_int_rect rect;
    gs_get_bits_params_t params;
    int plane;

    if (y < 0 || height < 0 || y + height > pdev->height)
	return_error(gs_error_rangecheck);
    rect.p.x = 0, rect.p.y = y;
    rect.q.x = pdev->width, rect.q.y = y + height;
    params.options =
	GB_RETURN_POINTER | GB_ALIGN_STANDARD | GB_OFFSET_0 |
	GB_RASTER_ANY |
	/* No depth specified, we always use native colors. */
	GB_COLORS_NATIVE | GB_ALPHA_NONE;
    if (render_plane) {
	params.options |= GB_PACKING_PLANAR | GB_SELECT_PLANES;
	memset(params.data, 0,
	       sizeof(params.data[0]) * pdev->color_info.num_components);
	plane = render_plane->index;
	params.data[plane] = buffer;
    } else {
	params.options |= GB_PACKING_CHUNKY;
	params.data[0] = buffer;
	plane = 0;
    }
    params.x_offset = 0;
    params.raster = bytes_per_line;
    code = dev_proc(pdev, get_bits_rectangle)
	((gx_device *)pdev, &rect, &params, NULL);
    if (code < 0 && actual_buffer) {
	/*
	 * RETURN_POINTER might not be implemented for this
	 * combination of parameters: try RETURN_COPY.
	 */
	params.options &= ~(GB_RETURN_POINTER | GB_RASTER_ALL);
	params.options |= GB_RETURN_COPY | GB_RASTER_SPECIFIED;
	code = dev_proc(pdev, get_bits_rectangle)
	    ((gx_device *)pdev, &rect, &params, NULL);
    }
    if (code < 0)
	return code;
    if (actual_buffer)
	*actual_buffer = params.data[plane];
    if (actual_bytes_per_line)
	*actual_bytes_per_line = params.raster;
    return code;
}


/* Copy a scan line from the buffer to the printer. */
int
gdev_prn_get_bits(gx_device_printer * pdev, int y, byte * str, byte ** actual_data)
{
    int code = (*dev_proc(pdev, get_bits)) ((gx_device *) pdev, y, str, actual_data);
    uint line_size = gdev_prn_raster(pdev);
    int last_bits = -(pdev->width * pdev->color_info.depth) & 7;

    if (code < 0)
	return code;
    if (last_bits != 0) {
	byte *dest = (actual_data != 0 ? *actual_data : str);

	dest[line_size - 1] &= 0xff << last_bits;
    }
    return 0;
}
/* Copy scan lines to a buffer.  Return the number of scan lines, */
/* or <0 if error.  This procedure is DEPRECATED. */
int
gdev_prn_copy_scan_lines(gx_device_printer * pdev, int y, byte * str, uint size)
{
    uint line_size = gdev_prn_raster(pdev);
    int count = size / line_size;
    int i;
    byte *dest = str;

    count = min(count, pdev->height - y);
    for (i = 0; i < count; i++, dest += line_size) {
	int code = gdev_prn_get_bits(pdev, y + i, dest, NULL);

	if (code < 0)
	    return code;
    }
    return count;
}

/* Close the current page. */
int
gdev_prn_close_printer(gx_device * pdev)
{
    gx_device_printer * const ppdev = (gx_device_printer *)pdev;
    gs_parsed_file_name_t parsed;
    const char *fmt;
    int code = gx_parse_output_file_name(&parsed, &fmt, ppdev->fname,
					 strlen(ppdev->fname));

    if ((code >= 0 && fmt) /* file per page */ ||
	ppdev->ReopenPerPage	/* close and reopen for each page */
	) {
	gx_device_close_output_file(pdev, ppdev->fname, ppdev->file);
	ppdev->file = NULL;
    }
    return 0;
}

/* If necessary, free and reallocate the printer memory after changing params */
int
gdev_prn_maybe_realloc_memory(gx_device_printer *prdev, 
			      gdev_prn_space_params *old_sp,
			      int old_width, int old_height,
			      bool old_page_uses_transparency)
{
    int code = 0;
    gx_device *const pdev = (gx_device *)prdev;
    /*gx_device_memory * const mdev = (gx_device_memory *)prdev;*/
	
    /*
     * The first test was changed to mdev->base != 0 in 5.50 (per Artifex).
     * Not only was this test wrong logically, it was incorrect in that
     * casting pdev to a (gx_device_memory *) is only meaningful if banding
     * is not being used.  The test was changed back to prdev->is_open in
     * 5.67 (also per Artifex).  For more information, see the News items
     * for these filesets.
     */
    if (prdev->is_open &&
	(memcmp(&prdev->space_params, old_sp, sizeof(*old_sp)) != 0 ||
	 prdev->width != old_width || prdev->height != old_height ||
	 prdev->page_uses_transparency != old_page_uses_transparency)
	) {
	int new_width = prdev->width;
	int new_height = prdev->height;
	gdev_prn_space_params new_sp;

#ifdef DEBUGGING_HACKS
debug_dump_bytes((const byte *)old_sp, (const byte *)(old_sp + 1), "old");
debug_dump_bytes((const byte *)&prdev->space_params,
		 (const byte *)(&prdev->space_params + 1), "new");
dprintf4("w=%d/%d, h=%d/%d\n", old_width, new_width, old_height, new_height);
#endif /*DEBUGGING_HACKS*/
	new_sp = prdev->space_params;
	prdev->width = old_width;
	prdev->height = old_height;
	prdev->space_params = *old_sp;
	code = gdev_prn_reallocate_memory(pdev, &new_sp,
					  new_width, new_height);
	/* If this fails, device should be usable w/old params, but */
	/* band files may not be open. */
    }
    return code;
}

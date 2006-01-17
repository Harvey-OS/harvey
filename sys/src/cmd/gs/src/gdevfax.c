/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevfax.c,v 1.4 2002/02/21 22:24:51 giles Exp $ */
/* Fax devices */
#include "gdevprn.h"
#include "strimpl.h"
#include "scfx.h"
#include "gdevfax.h"

/* The device descriptors */
private dev_proc_print_page(faxg3_print_page);
private dev_proc_print_page(faxg32d_print_page);
private dev_proc_print_page(faxg4_print_page);

/* Define procedures that adjust the paper size. */
const gx_device_procs gdev_fax_std_procs =
    prn_params_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
		     gdev_fax_get_params, gdev_fax_put_params);

#define FAX_DEVICE(dname, print_page)\
{\
    FAX_DEVICE_BODY(gx_device_fax, gdev_fax_std_procs, dname, print_page)\
}


const gx_device_fax gs_faxg3_device =
    FAX_DEVICE("faxg3", faxg3_print_page);

const gx_device_fax gs_faxg32d_device =
    FAX_DEVICE("faxg32d", faxg32d_print_page);

const gx_device_fax gs_faxg4_device =
    FAX_DEVICE("faxg4", faxg4_print_page);

/* Open the device. */
/* This is no longer needed: we retain it for client backward compatibility. */
int
gdev_fax_open(gx_device * dev)
{
    return gdev_prn_open(dev);
}

/* Get/put the AdjustWidth parameter. */
int
gdev_fax_get_params(gx_device * dev, gs_param_list * plist)
{
    gx_device_fax *const fdev = (gx_device_fax *)dev;
    int code = gdev_prn_get_params(dev, plist);
    int ecode = code;

    if ((code = param_write_int(plist, "AdjustWidth", &fdev->AdjustWidth)) < 0)
        ecode = code;
    return ecode;
}
int
gdev_fax_put_params(gx_device * dev, gs_param_list * plist)
{
    gx_device_fax *const fdev = (gx_device_fax *)dev;
    int ecode = 0;
    int code;
    int aw = fdev->AdjustWidth;
    const char *param_name;

    switch (code = param_read_int(plist, (param_name = "AdjustWidth"), &aw)) {
        case 0:
	    if (aw >= 0 && aw <= 1)
		break;
	    code = gs_error_rangecheck;
	default:
	    ecode = code;
	    param_signal_error(plist, param_name, ecode);
	case 1:
	    break;
    }

    if (ecode < 0)
	return ecode;
    code = gdev_prn_put_params(dev, plist);
    if (code < 0)
	return code;

    fdev->AdjustWidth = aw;
    return code;
}

/* Initialize the stream state with a set of default parameters. */
/* These select the same defaults as the CCITTFaxEncode filter, */
/* except we set BlackIs1 = true. */
private void
gdev_fax_init_state_adjust(stream_CFE_state *ss,
			   const gx_device_fax *fdev,
			   int adjust_width)
{
    s_CFE_template.set_defaults((stream_state *)ss);
    ss->Columns = fdev->width;
    ss->Rows = fdev->height;
    ss->BlackIs1 = true;
    if (adjust_width > 0) {
	/* Adjust the page width to a legal value for fax systems. */
	if (ss->Columns >= 1680 && ss->Columns <= 1736) {
	    /* Adjust width for A4 paper. */
	    ss->Columns = 1728;
	} else if (ss->Columns >= 2000 && ss->Columns <= 2056) {
	    /* Adjust width for B4 paper. */
	    ss->Columns = 2048;
	}
    }
}
void
gdev_fax_init_state(stream_CFE_state *ss, const gx_device_fax *fdev)
{
    gdev_fax_init_state_adjust(ss, fdev, 1);
}
void
gdev_fax_init_fax_state(stream_CFE_state *ss, const gx_device_fax *fdev)
{
    gdev_fax_init_state_adjust(ss, fdev, fdev->AdjustWidth);
}

/*
 * Print one strip with fax compression.  Fax devices call this once per
 * page; TIFF devices call this once per strip.
 */
int
gdev_fax_print_strip(gx_device_printer * pdev, FILE * prn_stream,
		     const stream_template * temp, stream_state * ss,
		     int width, int row_first, int row_end /* last + 1 */)
{
    gs_memory_t *mem = pdev->memory;
    int code;
    stream_cursor_read r;
    stream_cursor_write w;
    int in_size = gdev_mem_bytes_per_scan_line((gx_device *) pdev);
    /*
     * Because of the width adjustment for fax systems, width may
     * be different from (either greater than or less than) pdev->width.
     * Allocate a large enough buffer to account for this.
     */
    int col_size = (width * pdev->color_info.depth + 7) >> 3;
    int max_size = max(in_size, col_size);
    int lnum;
    byte *in;
    byte *out;
    /* If the file is 'nul', don't even do the writes. */
    bool nul = !strcmp(pdev->fname, "nul");

    /* Initialize the common part of the encoder state. */
    ss->template = temp;
    ss->memory = mem;
    /* Now initialize the encoder. */
    code = temp->init(ss);
    if (code < 0)
	return_error(gs_error_limitcheck);	/* bogus, but as good as any */

    /* Allocate the buffers. */
    in = gs_alloc_bytes(mem, temp->min_in_size + max_size + 1,
			"gdev_stream_print_page(in)");
#define OUT_SIZE 1000
    out = gs_alloc_bytes(mem, OUT_SIZE, "gdev_stream_print_page(out)");
    if (in == 0 || out == 0) {
	code = gs_note_error(gs_error_VMerror);
	goto done;
    }
    /* Set up the processing loop. */
    lnum = 0;
    r.ptr = r.limit = in - 1;
    w.ptr = out - 1;
    w.limit = w.ptr + OUT_SIZE;
#undef OUT_SIZE

    /* Process the image. */
    for (lnum = row_first; ;) {
	int status;

	if_debug7('w', "[w]lnum=%d r=0x%lx,0x%lx,0x%lx w=0x%lx,0x%lx,0x%lx\n", lnum,
		  (ulong)in, (ulong)r.ptr, (ulong)r.limit,
		  (ulong)out, (ulong)w.ptr, (ulong)w.limit);
	status = temp->process(ss, &r, &w, lnum == row_end);
	if_debug7('w', "...%d, r=0x%lx,0x%lx,0x%lx w=0x%lx,0x%lx,0x%lx\n", status,
		  (ulong)in, (ulong)r.ptr, (ulong)r.limit,
		  (ulong)out, (ulong)w.ptr, (ulong)w.limit);
	switch (status) {
	    case 0:		/* need more input data */
		if (lnum == row_end)
		    goto ok;
		{
		    uint left = r.limit - r.ptr;

		    memcpy(in, r.ptr + 1, left);
		    gdev_prn_copy_scan_lines(pdev, lnum++, in + left, in_size);
		    /* Note: we use col_size here, not in_size. */
		    if (col_size > in_size) {
			memset(in + left + in_size, 0, col_size - in_size);
		    }
		    r.limit = in + left + col_size - 1;
		    r.ptr = in - 1;
		}
		break;
	    case 1:		/* need to write output */
		if (!nul)
		    fwrite(out, 1, w.ptr + 1 - out, prn_stream);
		w.ptr = out - 1;
		break;
	}
    }

  ok:
    /* Write out any remaining output. */
    if (!nul)
	fwrite(out, 1, w.ptr + 1 - out, prn_stream);

  done:
    gs_free_object(mem, out, "gdev_stream_print_page(out)");
    gs_free_object(mem, in, "gdev_stream_print_page(in)");
    if (temp->release)
	temp->release(ss);
    return code;
}

/* Print a fax page.  Other fax drivers use this. */
int
gdev_fax_print_page(gx_device_printer * pdev, FILE * prn_stream,
		    stream_CFE_state * ss)
{
    return gdev_fax_print_strip(pdev, prn_stream, &s_CFE_template,
				(stream_state *)ss, ss->Columns,
				0, pdev->height);
}

/* Print a 1-D Group 3 page. */
private int
faxg3_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
    stream_CFE_state state;

    gdev_fax_init_fax_state(&state, (gx_device_fax *)pdev);
    state.EndOfLine = true;
    state.EndOfBlock = false;
    return gdev_fax_print_page(pdev, prn_stream, &state);
}

/* Print a 2-D Group 3 page. */
private int
faxg32d_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
    stream_CFE_state state;

    gdev_fax_init_fax_state(&state, (gx_device_fax *)pdev);
    state.K = (pdev->y_pixels_per_inch < 100 ? 2 : 4);
    state.EndOfLine = true;
    state.EndOfBlock = false;
    return gdev_fax_print_page(pdev, prn_stream, &state);
}

/* Print a Group 4 page. */
private int
faxg4_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
    stream_CFE_state state;

    gdev_fax_init_fax_state(&state, (gx_device_fax *)pdev);
    state.K = -1;
    state.EndOfBlock = false;
    return gdev_fax_print_page(pdev, prn_stream, &state);
}

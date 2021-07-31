/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevjpeg.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* JPEG output driver */
#include "stdio_.h"		/* for jpeglib.h */
#include "jpeglib_.h"
#include "gdevprn.h"
#include "stream.h"
#include "strimpl.h"
#include "sdct.h"
#include "sjpeg.h"

/* Structure for the JPEG-writing device. */
typedef struct gx_device_jpeg_s {
    gx_device_common;
    gx_prn_device_common;
    /* Additional parameters */
    int JPEGQ;			/* quality on IJG scale */
    float QFactor;		/* quality per DCTEncode conventions */
    /* JPEGQ overrides QFactor if both are specified. */
} gx_device_jpeg;

/* The device descriptor */
private dev_proc_get_params(jpeg_get_params);
private dev_proc_put_params(jpeg_put_params);
private dev_proc_print_page(jpeg_print_page);

/* ------ The device descriptors ------ */

/* Default X and Y resolution. */
#ifndef X_DPI
#  define X_DPI 72
#endif
#ifndef Y_DPI
#  define Y_DPI 72
#endif

/* 24-bit color */

private const gx_device_procs jpeg_procs =
prn_color_params_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
		       gx_default_rgb_map_rgb_color,
		       gx_default_rgb_map_color_rgb,
		       jpeg_get_params, jpeg_put_params);

const gx_device_jpeg gs_jpeg_device =
{prn_device_std_body(gx_device_jpeg, jpeg_procs, "jpeg",
		     DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
		     X_DPI, Y_DPI, 0, 0, 0, 0, 24, jpeg_print_page),
 0,				/* JPEGQ: 0 indicates not specified */
 0.0				/* QFactor: 0 indicates not specified */
};

/* 8-bit gray */

private const gx_device_procs jpeggray_procs =
prn_color_params_procs(gdev_prn_open, gdev_prn_output_page, gdev_prn_close,
		       gx_default_gray_map_rgb_color,
		       gx_default_gray_map_color_rgb,
		       jpeg_get_params, jpeg_put_params);

const gx_device_jpeg gs_jpeggray_device =
{prn_device_body(gx_device_jpeg, jpeggray_procs, "jpeggray",
		 DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
		 X_DPI, Y_DPI, 0, 0, 0, 0,
		 1, 8, 255, 0, 256, 0,
		 jpeg_print_page),
 0,				/* JPEGQ: 0 indicates not specified */
 0.0				/* QFactor: 0 indicates not specified */
};

/* Get parameters. */
private int
jpeg_get_params(gx_device * dev, gs_param_list * plist)
{
    gx_device_jpeg *jdev = (gx_device_jpeg *) dev;
    int code = gdev_prn_get_params(dev, plist);
    int ecode;

    if (code < 0)
	return code;

    if ((ecode = param_write_int(plist, "JPEGQ", &jdev->JPEGQ)) < 0)
	code = ecode;
    if ((ecode = param_write_float(plist, "QFactor", &jdev->QFactor)) < 0)
	code = ecode;

    return code;
}

/* Put parameters. */
private int
jpeg_put_params(gx_device * dev, gs_param_list * plist)
{
    gx_device_jpeg *jdev = (gx_device_jpeg *) dev;
    int ecode = 0;
    int code;
    gs_param_name param_name;
    int jq = jdev->JPEGQ;
    float qf = jdev->QFactor;

    switch (code = param_read_int(plist, (param_name = "JPEGQ"), &jq)) {
	case 0:
	    if (jq < 0 || jq > 100)
		ecode = gs_error_limitcheck;
	    else
		break;
	    goto jqe;
	default:
	    ecode = code;
	  jqe:param_signal_error(plist, param_name, ecode);
	case 1:
	    break;
    }

    switch (code = param_read_float(plist, (param_name = "QFactor"), &qf)) {
	case 0:
	    if (qf < 0.0 || qf > 1.0e6)
		ecode = gs_error_limitcheck;
	    else
		break;
	    goto qfe;
	default:
	    ecode = code;
	  qfe:param_signal_error(plist, param_name, ecode);
	case 1:
	    break;
    }

    if (ecode < 0)
	return ecode;
    code = gdev_prn_put_params(dev, plist);
    if (code < 0)
	return code;

    jdev->JPEGQ = jq;
    jdev->QFactor = qf;
    return 0;
}

/* Send the page to the file. */
private int
jpeg_print_page(gx_device_printer * pdev, FILE * prn_stream)
{
    gx_device_jpeg *jdev = (gx_device_jpeg *) pdev;
    gs_memory_t *mem = pdev->memory;
    int line_size = gdev_mem_bytes_per_scan_line((gx_device *) pdev);
    byte *in = gs_alloc_bytes(mem, line_size, "jpeg_print_page(in)");
    jpeg_compress_data *jcdp = (jpeg_compress_data *)
    gs_alloc_bytes(mem, sizeof(*jcdp),
		   "jpeg_print_page(jpeg_compress_data)");
    byte *fbuf = 0;
    uint fbuf_size;
    byte *jbuf = 0;
    uint jbuf_size;
    int lnum;
    int code;
    stream_DCT_state state;
    stream fstrm, jstrm;

    if (jcdp == 0 || in == 0) {
	code = gs_note_error(gs_error_VMerror);
	goto fail;
    }
    /* Create the DCT decoder state. */
    jcdp->template = s_DCTE_template;
    state.template = &jcdp->template;
    state.memory = 0;
    if (state.template->set_defaults)
	(*state.template->set_defaults) ((stream_state *) & state);
    state.QFactor = 1.0;	/* disable quality adjustment in zfdcte.c */
    state.ColorTransform = 1;	/* default for RGB */
    /* We insert no markers, allowing the IJG library to emit */
    /* the format it thinks best. */
    state.NoMarker = true;	/* do not insert our own Adobe marker */
    state.Markers.data = 0;
    state.Markers.size = 0;
    state.data.compress = jcdp;
    jcdp->memory = state.jpeg_memory = mem;
    if ((code = gs_jpeg_create_compress(&state)) < 0)
	goto fail;
    jcdp->cinfo.image_width = pdev->width;
    jcdp->cinfo.image_height = pdev->height;
    switch (pdev->color_info.depth) {
	case 24:
	    jcdp->cinfo.input_components = 3;
	    jcdp->cinfo.in_color_space = JCS_RGB;
	    break;
	case 8:
	    jcdp->cinfo.input_components = 1;
	    jcdp->cinfo.in_color_space = JCS_GRAYSCALE;
	    break;
    }
    /* Set compression parameters. */
    if ((code = gs_jpeg_set_defaults(&state)) < 0)
	goto done;
    if (jdev->JPEGQ > 0) {
	code = gs_jpeg_set_quality(&state, jdev->JPEGQ, TRUE);
	if (code < 0)
	    goto done;
    } else if (jdev->QFactor > 0.0) {
	code = gs_jpeg_set_linear_quality(&state,
					  (int)(min(jdev->QFactor, 100.0)
						* 100.0 + 0.5),
					  TRUE);
	if (code < 0)
	    goto done;
    }
    jcdp->cinfo.restart_interval = 0;
    jcdp->cinfo.density_unit = 1;	/* dots/inch (no #define or enum) */
    jcdp->cinfo.X_density = pdev->HWResolution[0];
    jcdp->cinfo.Y_density = pdev->HWResolution[1];
    /* Create the filter. */
    /* Make sure we get at least a full scan line of input. */
    state.scan_line_size = jcdp->cinfo.input_components *
	jcdp->cinfo.image_width;
    jcdp->template.min_in_size =
	max(s_DCTE_template.min_in_size, state.scan_line_size);
    /* Make sure we can write the user markers in a single go. */
    jcdp->template.min_out_size =
	max(s_DCTE_template.min_out_size, state.Markers.size);

    /* Set up the streams. */
    fbuf_size = max(512 /* arbitrary */ , jcdp->template.min_out_size);
    jbuf_size = jcdp->template.min_in_size;
    if ((fbuf = gs_alloc_bytes(mem, fbuf_size, "jpeg_print_page(fbuf)")) == 0 ||
	(jbuf = gs_alloc_bytes(mem, jbuf_size, "jpeg_print_page(jbuf)")) == 0
	) {
	code = gs_note_error(gs_error_VMerror);
	goto done;
    }
    swrite_file(&fstrm, prn_stream, fbuf, fbuf_size);
    s_std_init(&jstrm, jbuf, jbuf_size, &s_filter_write_procs,
	       s_mode_write);
    jstrm.memory = mem;
    jstrm.state = (stream_state *) & state;
    jstrm.procs.process = state.template->process;
    jstrm.strm = &fstrm;
    if (state.template->init)
	(*state.template->init) (jstrm.state);

    /* Copy the data to the output. */
    for (lnum = 0; lnum < pdev->height; ++lnum) {
	byte *data;
	uint ignore_used;

	gdev_prn_get_bits(pdev, lnum, in, &data);
	sputs(&jstrm, data, state.scan_line_size, &ignore_used);
    }

    /* Wrap up. */
    sclose(&jstrm);
    sflush(&fstrm);
    jcdp = 0;
  done:
    gs_free_object(mem, jbuf, "jpeg_print_page(jbuf)");
    gs_free_object(mem, fbuf, "jpeg_print_page(fbuf)");
    if (jcdp)
	gs_jpeg_destroy(&state);	/* frees *jcdp */
    gs_free_object(mem, in, "jpeg_print_page(in)");
    return code;
  fail:
    if (jcdp)
	gs_free_object(mem, jcdp, "jpeg_print_page(jpeg_compress_data)");
    gs_free_object(mem, in, "jpeg_print_page(in)");
    return code;
#undef jcdp
}

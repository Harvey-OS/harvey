/* Copyright (C) 2001-2002 artofcode LLC.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: gdevijs.c,v 1.1.2.1 2002/02/01 03:30:13 raph Exp $ */
/*
 * IJS device for Ghostscript.
 * Intended to work with any IJS compliant inkjet driver, including
 * hpijs 1.0 and later, an IJS-enhanced gimp-print driver, and
 * the IJS Windows GDI server (ijsmswin.exe).
 * 
 * DRAFT
 *
 * WARNING: The ijs server can be selected on the gs command line
 * which is a security risk, since any program can be run.
 * You should use -dSAFER which sets .LockSafetyParams to true 
 * before opening this device.
 */

#include "unistd_.h"	/* for dup() */
#include <stdlib.h>
#include "gdevprn.h"
#include "gp.h"
#include "ijs.h"
#include "ijs_client.h"

/* This should go into gdevprn.h, or, better yet, gdevprn should
   acquire an API for changing resolution. */
int gdev_prn_maybe_realloc_memory(gx_device_printer *pdev,
				  gdev_prn_space_params *old_space,
				  int old_width, int old_height);

/* Device procedures */

/* See gxdevice.h for the definitions of the procedures. */
private dev_proc_open_device(gsijs_open);
private dev_proc_close_device(gsijs_close);
private dev_proc_output_page(gsijs_output_page);
private dev_proc_get_params(gsijs_get_params);
private dev_proc_put_params(gsijs_put_params);

private const gx_device_procs gsijs_procs =
prn_color_params_procs(gsijs_open, gsijs_output_page, gsijs_close,
		   gx_default_rgb_map_rgb_color, gx_default_rgb_map_color_rgb,
		   gsijs_get_params, gsijs_put_params);

typedef struct gx_device_ijs_s gx_device_ijs;

/* The device descriptor */
struct gx_device_ijs_s {
    gx_device_common;
    gx_prn_device_common;
    bool IjsUseOutputFD;
    char IjsServer[gp_file_name_sizeof]; /* name of executable ijs server */
    char *ColorSpace;
    int ColorSpace_size;
    int BitsPerSample;
    char *DeviceManufacturer;
    int DeviceManufacturer_size;
    char *DeviceModel;
    int DeviceModel_size;
    char *IjsParams;
    int IjsParams_size;

    /* Common setpagedevice parameters supported by ijs but not
       currently parsed by gx_prn_device. We prefix these with Ijs to
       avoid namespace collision if they do get added to gx_prn_device.
    */
    bool IjsTumble;
    bool IjsTumble_set;

    IjsClientCtx *ctx;
    int ijs_version;
};

#define DEFAULT_DPI 74   /* See gsijs_set_resolution() below. */

gx_device_ijs gs_ijs_device =
{
    prn_device_std_body(gx_device_ijs, gsijs_procs, "ijs",
			DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
			DEFAULT_DPI, DEFAULT_DPI,
			0, 0, 0, 0,
			24 /* depth */, NULL /* print page */),
    FALSE,	/* IjsUseOutputFD */
    "",		/* IjsServer */
    NULL,	/* ColorSpace */
    0,		/* ColorSpace_size */
    8,		/* BitsPerSample */
    NULL,	/* DeviceManufacturer */
    0,		/* DeviceManufacturer_size */
    NULL,	/* DeviceModel */
    0,		/* DeviceModel_size */
    NULL,	/* IjsParams */
    0,		/* IjsParams_size */

    FALSE,	/* Tumble */
    FALSE,	/* Tumble_set */

    NULL,	/* IjsClient *ctx */
    0		/* ijs_version */
};


private int gsijs_client_set_param(gx_device_ijs *ijsdev, const char *key,
    const char *value);
private int gsijs_set_color_format(gx_device_ijs *ijsdev);
private int gsijs_read_int(gs_param_list *plist, gs_param_name pname, 
   int *pval, int min_value, int max_value, bool only_when_closed);
private int gsijs_read_bool(gs_param_list *plist, gs_param_name pname, 
   bool *pval, bool only_when_closed);
private int gsijs_read_string(gs_param_list * plist, gs_param_name pname, 
   char * str, uint size, bool safety, bool only_when_closed);

/**************************************************************************/

/* ------ Private definitions ------ */

/* Versions 1.0 through 1.0.2 of hpijs report IJS version 0.29, and
   require some workarounds. When more up-to-date hpijs versions
   become ubiquitous, all these workarounds should be removed. */
#define HPIJS_1_0_VERSION 29

private int
gsijs_parse_wxh (const char *val, int size, double *pw, double *ph)
{
    char buf[256];
    char *tail;
    int i;

    for (i = 0; i < size; i++)
	if (val[i] == 'x')
	    break;

    if (i + 1 >= size)
	return IJS_ESYNTAX;

    if (i >= sizeof(buf))
	return IJS_EBUF;

    memcpy (buf, val, i);
    buf[i] = 0;
    *pw = strtod (buf, &tail);
    if (tail == buf)
	return IJS_ESYNTAX;

    if (size - i > sizeof(buf))
	return IJS_EBUF;

    memcpy (buf, val + i + 1, size - i - 1);
    buf[size - i - 1] = 0;
    *ph = strtod (buf, &tail);
    if (tail == buf)
	return IJS_ESYNTAX;

    return 0;
}

/**
 * gsijs_set_generic_params_hpijs: Set generic IJS parameters.
 *
 * This version is specialized for hpijs 1.0 through 1.0.2, and
 * accommodates a number of quirks.
 **/
private int
gsijs_set_generic_params_hpijs(gx_device_ijs *ijsdev)
{
    char buf[256];
    int code = 0;

    /* IjsParams, Duplex, and Tumble get set at this point because
       they may affect margins. */
    if (ijsdev->IjsParams) {
	code = gsijs_client_set_param(ijsdev, "IjsParams", ijsdev->IjsParams);
    }

    if (code == 0 && ijsdev->Duplex_set) {
	int duplex_val;
	
	duplex_val = ijsdev->Duplex ? (ijsdev->IjsTumble ? 1 : 2) : 0;
	sprintf (buf, "%d", duplex_val);
	code = gsijs_client_set_param(ijsdev, "Duplex", buf);
    }
    return code;
}

/**
 * gsijs_set_generic_params: Set generic IJS parameters.
 **/
private int
gsijs_set_generic_params(gx_device_ijs *ijsdev)
{
    char buf[256];
    int code = 0;
    int i, j;
    char *value;

    if (ijsdev->ijs_version == HPIJS_1_0_VERSION)
	return gsijs_set_generic_params_hpijs(ijsdev);

    /* Split IjsParams into separate parameters and send to ijs server */
    value = NULL;
    for (i=0, j=0; (j < ijsdev->IjsParams_size) && (i < sizeof(buf)-1); j++) {
	char ch = ijsdev->IjsParams[j];
	if (ch == '\\') {
	    j++;
	    buf[i++] = ijsdev->IjsParams[j];
	}
	else {
	    if (ch == '=') {
		buf[i++] = '\0';
		value = &buf[i];
	    }
	    else
		buf[i++] = ch;
	    if (ch == ',') {
		buf[i-1] = '\0';
		if (value)
		    gsijs_client_set_param(ijsdev, buf, value);
		i = 0;
		value = NULL;
	    }
	}
    }
    if (value)
	code = gsijs_client_set_param(ijsdev, buf, value);

    if (code == 0 && ijsdev->Duplex_set) {
	code = gsijs_client_set_param(ijsdev, "PS:Duplex",
				      ijsdev->Duplex ? "true" : "false");
    }
    if (code == 0 && ijsdev->IjsTumble_set) {
	code = gsijs_client_set_param(ijsdev, "PS:Tumble",
				      ijsdev->IjsTumble ? "true" :
				      "false");
    }
    return code;
}

/**
 * gsijs_set_margin_params_hpijs: Do margin negotiation with IJS server.
 *
 * This version is specialized for hpijs 1.0 through 1.0.2, and
 * accommodates a number of quirks.
 **/
private int
gsijs_set_margin_params_hpijs(gx_device_ijs *ijsdev)
{
    char buf[256];
    int code = 0;

    if (code == 0) {
	sprintf(buf, "%d", ijsdev->width);
	code = gsijs_client_set_param(ijsdev, "Width", buf);
    }
    if (code == 0) {
	sprintf(buf, "%d", ijsdev->height);
	code = gsijs_client_set_param(ijsdev, "Height", buf);
    }

    if (code == 0) {
	double printable_width, printable_height;
	double printable_left, printable_top;
	float m[4];

	code = ijs_client_get_param(ijsdev->ctx, 0, "PrintableArea",
				   buf, sizeof(buf));
	if (code == IJS_EUNKPARAM)
	    /* IJS server doesn't support margin negotiations.
	       That's ok. */
	    return 0;
	else if (code >= 0) {
	    code = gsijs_parse_wxh(buf, code,
				    &printable_width, &printable_height);
	}

	if (code == 0) {
	    code = ijs_client_get_param(ijsdev->ctx, 0, "PrintableTopLeft",
					buf, sizeof(buf));
	    if (code == IJS_EUNKPARAM)
		return 0;
	    else if (code >= 0) {
		code = gsijs_parse_wxh(buf, code,
					&printable_left, &printable_top);
	    }
	}

	if (code == 0) {
	    m[0] = printable_left;
	    m[1] = ijsdev->MediaSize[1] * (1.0 / 72) -
	      printable_top - printable_height;
	    m[2] = ijsdev->MediaSize[0] * (1.0 / 72) -
	      printable_left - printable_width;
	    m[3] = printable_top;
	    gx_device_set_margins((gx_device *)ijsdev, m, true);
	}
    }

    return code;
}

/**
 * gsijs_set_margin_params: Do margin negotiation with IJS server.
 **/
private int
gsijs_set_margin_params(gx_device_ijs *ijsdev)
{
    char buf[256];
    int code = 0;
    int i, j;
    char *value;

    if (ijsdev->ijs_version == HPIJS_1_0_VERSION)
	return gsijs_set_margin_params_hpijs(ijsdev);

    /* Split IjsParams into separate parameters and send to ijs server */
    value = NULL;
    for (i=0, j=0; (j < ijsdev->IjsParams_size) && (i < sizeof(buf)-1); j++) {
	char ch = ijsdev->IjsParams[j];
	if (ch == '\\') {
	    j++;
	    buf[i++] = ijsdev->IjsParams[j];
	}
	else {
	    if (ch == '=') {
		buf[i++] = '\0';
		value = &buf[i];
	    }
	    else
		buf[i++] = ch;
	    if (ch == ',') {
		buf[i-1] = '\0';
		if (value)
		    gsijs_client_set_param(ijsdev, buf, value);
		i = 0;
		value = NULL;
	    }
	}
    }
    if (value)
	code = gsijs_client_set_param(ijsdev, buf, value);

    if (code == 0 && ijsdev->Duplex_set) {
	code = gsijs_client_set_param(ijsdev, "Duplex",
				      ijsdev->Duplex ? "true" : "false");
    }
    if (code == 0 && ijsdev->IjsTumble_set) {
	code = gsijs_client_set_param(ijsdev, "Tumble",
				      ijsdev->IjsTumble ? "true" :
				      "false");
    }

    if (code == 0) {
	sprintf (buf, "%gx%g", ijsdev->MediaSize[0] * (1.0 / 72),
		 ijsdev->MediaSize[1] * (1.0 / 72));
	code = ijs_client_set_param(ijsdev->ctx, 0, "PaperSize",
				    buf, strlen(buf));
    }

    if (code == 0) {
	double printable_width, printable_height;
	double printable_left, printable_top;
	float m[4];

	code = ijs_client_get_param(ijsdev->ctx, 0, "PrintableArea",
				   buf, sizeof(buf));
	if (code == IJS_EUNKPARAM)
	    /* IJS server doesn't support margin negotiations.
	       That's ok. */
	    return 0;
	else if (code >= 0) {
	    code = gsijs_parse_wxh (buf, code,
				    &printable_width, &printable_height);
	}

	if (code == 0) {
	    code = ijs_client_get_param(ijsdev->ctx, 0, "PrintableTopLeft",
					buf, sizeof(buf));
	    if (code == IJS_EUNKPARAM)
		return 0;
	    else if (code >= 0) {
		code = gsijs_parse_wxh(buf, code,
					&printable_left, &printable_top);
	    }
	}

	if (code == 0) {
	    m[0] = printable_left;
	    m[1] = printable_top;
	    m[2] = ijsdev->MediaSize[0] * (1.0 / 72) -
		printable_left - printable_width;
	    m[3] = ijsdev->MediaSize[1] * (1.0 / 72) -
		printable_top - printable_height;
	    gx_device_set_margins((gx_device *)ijsdev, m, true);
	    sprintf (buf, "%gx%g", printable_left, printable_top);
	    code = ijs_client_set_param(ijsdev->ctx, 0, "TopLeft",
					buf, strlen(buf));
	}
    }

    return code;
}

/**
 * gsijs_set_resolution: Set resolution.
 *
 * The priority order, highest first, is: commandline -r switch,
 * if specified; IJS get_param of Dpi; 72 dpi default.
 *
 * Because Ghostscript doesn't have a really good way to detect
 * whether resolution was set on the command line, we set a
 * low-probability resolution (DEFAULT_DPI) in the static
 * initialization of the device, then detect whether it has been
 * changed from that.  This causes a minor infelicity: if DEFAULT_DPI
 * is set on the command line, it is changed to the default here.
 **/
private int
gsijs_set_resolution(gx_device_ijs *ijsdev)
{
    char buf[256];
    int code;
    floatp x_dpi, y_dpi;
    int width = ijsdev->width;
    int height = ijsdev->height;
    bool save_is_open = ijsdev->is_open;

    if (ijsdev->HWResolution[0] != DEFAULT_DPI ||
	ijsdev->HWResolution[1] != DEFAULT_DPI) {
	/* Resolution has been set on command line. */
	return 0;
    }
    code = ijs_client_get_param(ijsdev->ctx, 0, "Dpi",
				buf, sizeof(buf));
    if (code >= 0) {
	int i;

	for (i = 0; i < code; i++)
	    if (buf[i] == 'x')
		break;
	if (i == code) {
	    char *tail;

	    if (i == sizeof(buf))
		code = IJS_EBUF;
	    buf[i] = 0;
	    x_dpi = y_dpi = strtod (buf, &tail);
	    if (tail == buf)
		code = IJS_ESYNTAX;
	} else {
	    double x, y;

	    code = gsijs_parse_wxh(buf, code, &x, &y);
	    x_dpi = x;
	    y_dpi = y;
	}
    }

    if (code < 0) {
	x_dpi = 72.0;
	y_dpi = 72.0;
    }

    gx_device_set_resolution((gx_device *)ijsdev, x_dpi, y_dpi);

    ijsdev->is_open = true;
    code = gdev_prn_maybe_realloc_memory((gx_device_printer *)ijsdev,
					 &ijsdev->space_params,
					 width, height);
    ijsdev->is_open = save_is_open;
    return code;
}

/* Open the gsijs driver */
private int
gsijs_open(gx_device *dev)
{
    gx_device_ijs *ijsdev = (gx_device_ijs *)dev;
    int code;
    char buf[256];
    bool use_outputfd;
    int fd = -1;

    if (strlen(ijsdev->IjsServer) == 0) {
	eprintf("ijs server not specified\n");
	return gs_note_error(gs_error_ioerror);
    }

    /* Decide whether to use OutputFile or OutputFD. Note: how to
       determine this is a tricky question, so we just allow the
       user to set it.
    */
    use_outputfd = ijsdev->IjsUseOutputFD;

    /* If using OutputFilename, we don't want to open the output file.
       Leave that to the ijs server. */
    ijsdev->OpenOutputFile = use_outputfd;

    code = gdev_prn_open(dev);
    if (code < 0)
	return code;

    if (use_outputfd) {
	/* Note: dup() may not be portable to all interesting IJS
	   platforms. In that case, this branch should be #ifdef'ed out.
	*/
	fd = dup(fileno(ijsdev->file));
    }

    /* WARNING: Ghostscript should be run with -dSAFER to stop
     * someone changing the ijs server (e.g. running a shell).
     */
    ijsdev->ctx = ijs_invoke_server(ijsdev->IjsServer);
    if (ijsdev->ctx == (IjsClientCtx *)NULL) {
	eprintf1("Can't start ijs server \042%s\042\n", ijsdev->IjsServer);
	return gs_note_error(gs_error_ioerror);
    }

    ijsdev->ijs_version = ijs_client_get_version (ijsdev->ctx);

    if (ijs_client_open(ijsdev->ctx) < 0) {
	eprintf("Can't open ijs\n");
	return gs_note_error(gs_error_ioerror);
    }
    if (ijs_client_begin_job(ijsdev->ctx, 0) < 0) {
	eprintf("Can't begin ijs job 0\n");
	ijs_client_close(ijsdev->ctx);
	return gs_note_error(gs_error_ioerror);
    }

    if (use_outputfd) {
	/* Note: dup() may not be portable to all interesting IJS
	   platforms. In that case, this branch should be #ifdef'ed out.
	*/
	sprintf(buf, "%d", fd);
	ijs_client_set_param(ijsdev->ctx, 0, "OutputFD", buf, strlen(buf)); 
	close(fd);
    } else {
	ijs_client_set_param(ijsdev->ctx, 0, "OutputFile", 
			     ijsdev->fname, strlen(ijsdev->fname));
    }

    if (code >= 0 && ijsdev->DeviceManufacturer)
	code = ijs_client_set_param(ijsdev->ctx, 0, "DeviceManufacturer",
			     ijsdev->DeviceManufacturer,
			     strlen(ijsdev->DeviceManufacturer));

    if (code >= 0 && ijsdev->DeviceModel)
	code = ijs_client_set_param(ijsdev->ctx, 0, "DeviceModel",
			     ijsdev->DeviceModel,
			     strlen(ijsdev->DeviceModel));

    if (code >= 0)
	code = gsijs_set_generic_params(ijsdev);

    if (code >= 0)
	code = gsijs_set_resolution(ijsdev);

    if (code >= 0)
	code = gsijs_set_margin_params(ijsdev);

    return code;
};

/* Close the gsijs driver */
private int
gsijs_close(gx_device *dev)
{
    gx_device_ijs *ijsdev = (gx_device_ijs *)dev;
    int code;

    /* ignore ijs errors on close */
    ijs_client_end_job(ijsdev->ctx, 0);
    ijs_client_close(ijsdev->ctx);
    ijs_client_begin_cmd(ijsdev->ctx, IJS_CMD_EXIT);
    ijs_client_send_cmd_wait(ijsdev->ctx);

    code = gdev_prn_close(dev);
    if (ijsdev->IjsParams)
	gs_free(ijsdev->IjsParams, ijsdev->IjsParams_size, 1, 
	    "gsijs_read_string_malloc");
    if (ijsdev->ColorSpace)
	gs_free(ijsdev->ColorSpace,
		ijsdev->ColorSpace_size, 1, 
		"gsijs_read_string_malloc");
    if (ijsdev->DeviceManufacturer)
	gs_free(ijsdev->DeviceManufacturer,
		ijsdev->DeviceManufacturer_size, 1, 
		"gsijs_read_string_malloc");
    if (ijsdev->DeviceModel)
	gs_free(ijsdev->DeviceModel, ijsdev->DeviceModel_size, 1, 
		"gsijs_read_string_malloc");
    ijsdev->IjsParams = NULL;
    ijsdev->IjsParams_size = 0;
    ijsdev->DeviceManufacturer = NULL;
    ijsdev->DeviceManufacturer_size = 0;
    ijsdev->DeviceModel = NULL;
    ijsdev->DeviceModel_size = 0;
    return code;
}

/* This routine is entirely analagous to gdev_prn_print_scan_lines(),
   but computes width instead of height. It is not specific to IJS,
   and a strong case could be made for moving it into gdevprn.c. */
private int
gsijs_raster_width(gx_device *pdev)
{
    int width = pdev->width;
    gs_matrix imat;
    float xscale;
    int right, offset, end;

    (*dev_proc(pdev, get_initial_matrix)) (pdev, &imat);
    xscale = imat.xx * 72.0;
    right = (int)(dev_r_margin(pdev) * xscale);
    offset = (int)(dev_x_offset(pdev) * xscale);
    end = offset + width - right;
    return min(width, end);
}

private int ijs_all_white(unsigned char *data, int size)
{
   int clean = 1;
   int i;
   for (i = 0; i < size; i++)
   {
     if (data[i] != 0xFF)
     {
        clean = 0;
        break;
     }
   }
   return clean;
}

/* Print a page.  Don't use normal printer gdev_prn_output_page 
 * because it opens the output file.
 */
private int
gsijs_output_page(gx_device *dev, int num_copies, int flush)
{
    gx_device_ijs *ijsdev = (gx_device_ijs *)dev;
    gx_device_printer *pdev = (gx_device_printer *)dev;
    int raster = gdev_prn_raster(pdev);
    int ijs_width, ijs_height;
    int row_bytes;
    int n_chan = pdev->color_info.num_components;
    unsigned char *data;
    char buf[256];
    double xres = pdev->HWResolution[0];
    double yres = pdev->HWResolution[1];
    int code = 0;
    int endcode = 0;
    int status = 0;
    int i, y;

    if ((data = gs_alloc_bytes(pdev->memory, raster, "gsijs_output_page"))
	== (unsigned char *)NULL)
        return gs_note_error(gs_error_VMerror);

    /* Determine bitmap width and height */
    ijs_height = gdev_prn_print_scan_lines(dev);
    if (ijsdev->ijs_version == HPIJS_1_0_VERSION) {
	ijs_width = pdev->width;
    } else {
	ijs_width = gsijs_raster_width(dev);
    }
    row_bytes = (ijs_width * pdev->color_info.depth + 7) >> 3;

    /* Required page parameters */
    sprintf(buf, "%d", n_chan);
    gsijs_client_set_param(ijsdev, "NumChan", buf);
    sprintf(buf, "%d", ijsdev->BitsPerSample);
    gsijs_client_set_param(ijsdev, "BitsPerSample", buf);

    /* This needs to become more sophisticated for DeviceN. */
    strcpy(buf, (n_chan == 4) ? "DeviceCMYK" : 
	((n_chan == 3) ? "DeviceRGB" : "DeviceGray"));
    gsijs_client_set_param(ijsdev, "ColorSpace", buf);

    /* If hpijs 1.0, don't set width and height here, because it
       expects them to be the paper size. */
    if (ijsdev->ijs_version != HPIJS_1_0_VERSION) {
	sprintf(buf, "%d", ijs_width);
	gsijs_client_set_param(ijsdev, "Width", buf);
	sprintf(buf, "%d", ijs_height);
	gsijs_client_set_param(ijsdev, "Height", buf);
    }

    sprintf(buf, "%gx%g", xres, yres);
    gsijs_client_set_param(ijsdev, "Dpi", buf);

    for (i=0; i<num_copies; i++) {
 	unsigned char *actual_data;
	ijs_client_begin_cmd (ijsdev->ctx, IJS_CMD_BEGIN_PAGE);
	status = ijs_client_send_cmd_wait(ijsdev->ctx);

	for (y = 0; y < ijs_height; y++) {
	    code = gdev_prn_get_bits(pdev, y, data, &actual_data);
	    if (code < 0)
		break;

	    if (ijsdev->ijs_version == HPIJS_1_0_VERSION &&
		ijs_all_white(actual_data, row_bytes))
		status = ijs_client_send_data_wait(ijsdev->ctx, 0, NULL, 0);
	    else
		status = ijs_client_send_data_wait(ijsdev->ctx, 0,
		    (char *)actual_data, row_bytes);
	    if (status)
		break;
	}
	ijs_client_begin_cmd(ijsdev->ctx, IJS_CMD_END_PAGE);
	status = ijs_client_send_cmd_wait(ijsdev->ctx);
    }

    gs_free_object(pdev->memory, data, "gsijs_output_page");

    endcode = (pdev->buffer_space && !pdev->is_async_renderer ?
	       clist_finish_page(dev, flush) : 0);


    if (endcode < 0)
	return endcode;

    if (code < 0)
	return endcode;

    if (status < 0)
	return gs_note_error(gs_error_ioerror);

    code = gx_finish_output_page(dev, num_copies, flush);
    return code;
}


/**************************************************************************/

/* Get device parameters */
private int
gsijs_get_params(gx_device *dev, gs_param_list *plist)
{
    gx_device_ijs *ijsdev = (gx_device_ijs *)dev;
    gs_param_string gps;
    int code = gdev_prn_get_params(dev, plist);

    if (code >= 0) {
	param_string_from_transient_string(gps, ijsdev->IjsServer);
	code = param_write_string(plist, "IjsServer", &gps);
    }

    if (code >= 0) {
	if (ijsdev->DeviceManufacturer) {
	    param_string_from_transient_string(gps,
					       ijsdev->DeviceManufacturer);
	    code = param_write_string(plist, "DeviceManufacturer", &gps);
	} else {
	    code = param_write_null(plist, "DeviceManufacturer");
	}
    }

    if (code >= 0) {
	if (ijsdev->DeviceModel) {
	    param_string_from_transient_string(gps, ijsdev->DeviceModel);
	    code = param_write_string(plist, "DeviceModel", &gps);
	} else {
	    code = param_write_null(plist, "DeviceModel");
	}
    }

    if (code >= 0) {
	if (ijsdev->IjsParams) {
	    param_string_from_transient_string(gps, ijsdev->IjsParams);
	    code = param_write_string(plist, "IjsParams", &gps);
	} else {
	    code = param_write_null(plist, "IjsParams");
	}
    }

    if (code >= 0)
 	code = param_write_int(plist, "BitsPerSample", &ijsdev->BitsPerSample);

    if (code >= 0)
 	code = param_write_bool(plist, "IjsUseOutputFD",
				&ijsdev->IjsUseOutputFD);

    if (code >= 0) {
	if (ijsdev->IjsTumble_set) {
	    code = param_write_bool(plist, "Tumble", &ijsdev->IjsTumble);
	} else {
	    code = param_write_null(plist, "Tumble");
	}
    }

    return code;
}

private int
gsijs_read_int(gs_param_list *plist, gs_param_name pname, int *pval,
     int min_value, int max_value, bool only_when_closed)
{
    int code = 0;
    int new_value;

    switch (code = param_read_int(plist, pname, &new_value)) {
	case 0:
	    if (only_when_closed && (new_value != *pval)) {
		code = gs_error_rangecheck;
		goto e;
	    }
	    if ((new_value >= min_value) && (new_value <= max_value)) {
		*pval = new_value;
		break;
	    }
	    code = gs_note_error(gs_error_rangecheck);
	    goto e;
	default:
	    if (param_read_null(plist, pname) == 0)
		return 1;
	    e:param_signal_error(plist, pname, code);
	case 1:
	    ;
    }
    return code;
}

private int
gsijs_read_bool(gs_param_list *plist, gs_param_name pname, bool *pval,
		bool only_when_closed)
{
    int code = 0;
    bool new_value;

    switch (code = param_read_bool(plist, pname, &new_value)) {
	case 0:
	    if (only_when_closed && (new_value != *pval)) {
		code = gs_error_rangecheck;
		goto e;
	    }
	    *pval = new_value;
	    break;
	default:
	    if (param_read_null(plist, pname) == 0) {
		return 1;
	    }
	    e:param_signal_error(plist, pname, code);
	case 1:
	    ;
    }
    return code;
}

private int
gsijs_read_string(gs_param_list *plist, gs_param_name pname, char *str,
    uint size, bool safety, bool only_when_closed)
{
    int code;
    gs_param_string new_value;
    int differs;

    switch (code = param_read_string(plist, pname, &new_value)) {
        case 0:
	    differs = bytes_compare(new_value.data, new_value.size,
			(const byte *)str, strlen(str));
	    if (safety && differs) {
		code = gs_error_invalidaccess;
		goto e;
	    }
	    if (only_when_closed && differs) {
		code = gs_error_rangecheck;
		goto e;
	    }
            if (new_value.size < size) {
		strncpy(str, (const char *)new_value.data, new_value.size);
		str[new_value.size+1] = '\0';
                break;
	    }
            code = gs_note_error(gs_error_rangecheck);
            goto e;
        default:
            if (param_read_null(plist, pname) == 0)
                return 1;
          e:param_signal_error(plist, pname, code);
        case 1:
            ;
    }
    return code;
}

private int
gsijs_read_string_malloc(gs_param_list *plist, gs_param_name pname, char **str,
    int *size, bool only_when_closed)
{
    int code;
    gs_param_string new_value;
    int differs;

    switch (code = param_read_string(plist, pname, &new_value)) {
        case 0:
	    differs = bytes_compare(new_value.data, new_value.size,
			(const byte *)(*str ? *str : ""), 
		  	*str ? strlen(*str) : 0);
	    if (only_when_closed && differs) {
		code = gs_error_rangecheck;
		goto e;
	    }
	    if (new_value.size >= *size) {
	        if (*str)
		    gs_free(str, *size, 1, "gsijs_read_string_malloc");
		*str = NULL;
		*size = 0;
	    }
	    *str = gs_malloc(new_value.size + 1, 1, 
		"gsijs_read_string_malloc");
	    if (*str == NULL) {
                code = gs_note_error(gs_error_VMerror);
                goto e;
	    }
	    *size = new_value.size + 1;
	    strncpy(*str, (const char *)new_value.data, new_value.size);
	    (*str)[new_value.size] = '\0';
            break;
        default:
            if (param_read_null(plist, pname) == 0)
                return 1;
          e:param_signal_error(plist, pname, code);
        case 1:
            ;
    }
    return code;
}


private int
gsijs_put_params(gx_device *dev, gs_param_list *plist)
{
    gx_device_ijs *ijsdev = (gx_device_ijs *)dev;
    int code = 0;
    bool is_open = dev->is_open;

    /* We allow duplex to be set in all cases. At some point, it may
       be worthwhile to query the device to see if it supports
       duplex. Note also that this code will get called even before
       the device has been opened, which is when the -DDuplex
       command line is processed. */
    if (ijsdev->Duplex_set < 0) {
	ijsdev->Duplex = 1;
	ijsdev->Duplex_set = 0;
    }

    /* If a parameter must not be changed after the device is open,
     * the last parameter of gsijs_read_xxx() is is_open.
     * If a parameter may be changed at any time, it is false.
     */
    if (code >= 0)
	code = gsijs_read_string(plist, "IjsServer", 
	    ijsdev->IjsServer, sizeof(ijsdev->IjsServer), 
	    dev->LockSafetyParams, is_open);

    if (code >= 0)
	code = gsijs_read_string_malloc(plist, "DeviceManufacturer", 
	    &ijsdev->DeviceManufacturer, &ijsdev->DeviceManufacturer_size, 
	    is_open);

    if (code >= 0)
	code = gsijs_read_string_malloc(plist, "DeviceModel", 
	    &ijsdev->DeviceModel, &ijsdev->DeviceModel_size, 
	    is_open);

    if (code >= 0)
	code = gsijs_read_string_malloc(plist, "IjsParams", 
	    &(ijsdev->IjsParams), &(ijsdev->IjsParams_size), is_open);

    if (code >= 0)
	code = gsijs_read_int(plist, "BitsPerSample", &ijsdev->BitsPerSample, 
		1, 16, is_open);

    if (code >= 0)
 	code = gsijs_read_bool(plist, "IjsUseOutputFD",
			       &ijsdev->IjsUseOutputFD, is_open);

    if (code >= 0) {
	code = gsijs_read_string_malloc(plist, "ProcessColorModel",
	    &ijsdev->ColorSpace, &ijsdev->ColorSpace_size, is_open);
    }

    if (code >= 0) {
 	code = gsijs_read_bool(plist, "Tumble", &ijsdev->IjsTumble, false);
	if (code == 0)
	    ijsdev->IjsTumble_set = true;
    }

    if (code >= 0)
	code = gsijs_set_color_format(ijsdev);

    if (code >= 0)
	code = gdev_prn_put_params(dev, plist);

    if (code >= 0 && is_open) {
	code = gsijs_set_generic_params(ijsdev);
	if (code >= 0)
	  code = gsijs_set_margin_params(ijsdev);
	if (code < 0)
	    return gs_note_error(gs_error_ioerror);
    }
    
    return code;
}

private int
gsijs_client_set_param(gx_device_ijs *ijsdev, const char *key,
    const char *value)
{
    int code = ijs_client_set_param(ijsdev->ctx, 0 /* job id */, 
	key, value, strlen(value));
    if (code < 0)
	dprintf2("ijs: Can't set parameter %s=%s\n", key, value);
    return code;
}
 

private int
gsijs_set_color_format(gx_device_ijs *ijsdev)
{
    gx_device_color_info dci = ijsdev->color_info;
    int components;	/* 1=gray, 3=RGB, 4=CMYK */
    int bpc = ijsdev->BitsPerSample;		/* bits per component */
    int maxvalue;
    const char *ColorSpace = ijsdev->ColorSpace;

    if (ColorSpace == NULL)
	ColorSpace = "DeviceRGB";

    if (!strcmp (ColorSpace, "DeviceGray")) {
	components = 1;
	if (bpc == 1) {
	    ijsdev->procs.map_rgb_color = gx_default_w_b_map_rgb_color;
	    ijsdev->procs.map_color_rgb = gx_default_w_b_map_color_rgb;
	} else {
	    ijsdev->procs.map_rgb_color = gx_default_gray_map_rgb_color;
	    ijsdev->procs.map_color_rgb = gx_default_gray_map_color_rgb;
	}
    } else if (!strcmp (ColorSpace, "DeviceRGB")) {
	components = 3;
	ijsdev->procs.map_rgb_color = gx_default_rgb_map_rgb_color;
	ijsdev->procs.map_color_rgb = gx_default_rgb_map_color_rgb;
    } else if (!strcmp (ColorSpace, "DeviceCMYK")) {
	components = 4;
	ijsdev->procs.map_cmyk_color = cmyk_8bit_map_cmyk_color;
	ijsdev->procs.map_color_rgb = cmyk_8bit_map_color_rgb;
    } else {
	return -1;
    }

    maxvalue = (1 << bpc) - 1;
    dci.num_components = components;
    dci.depth = bpc * components;
    dci.max_gray = maxvalue;
    dci.max_color = components > 1 ? maxvalue : 0;
    dci.dither_grays = maxvalue+1;
    dci.dither_colors = components > 1 ? maxvalue+1 : 0;

    /* restore old anti_alias info */
    dci.anti_alias = ijsdev->color_info.anti_alias;

    ijsdev->color_info = dci;

    return 0;
}

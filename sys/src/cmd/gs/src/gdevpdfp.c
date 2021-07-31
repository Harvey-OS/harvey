/* Copyright (C) 1996, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gdevpdfp.c,v 1.10.2.2 2000/11/09 23:19:36 rayjj Exp $ */
/* Get/put parameters for PDF-writing driver */
#include "memory_.h"		/* should be first */
#include "gx.h"
#include "gserrors.h"
#include "gdevpdfx.h"
#include "gsparamx.h"

/*
 * The pdfwrite device supports the following "real" parameters:
 *      OutputFile <string>
 *      all the Distiller parameters (also see gdevpsdp.c)
 * Only some of the Distiller parameters actually have any effect.
 *
 * The device also supports the following write-only pseudo-parameters that
 * serve only to communicate other information from the PostScript file.
 * Their "value" is an array of strings, some of which may be the result
 * of converting arbitrary PostScript objects to string form.
 *      pdfmark - see gdevpdfm.c
 *	DSC - processed in this file
 */
private int pdf_dsc_process(P2(gx_device_pdf * pdev,
			       const gs_param_string_array * pma));

private const int CoreDistVersion = 4000;	/* Distiller 4.0 */
private const gs_param_item_t pdf_param_items[] = {
#define pi(key, type, memb) { key, type, offset_of(gx_device_pdf, memb) }

	/* Acrobat Distiller 4 parameters */

    /*
     * EndPage and StartPage are renamed because EndPage collides with
     * a page device parameter.
     */
    pi("PDFEndPage", gs_param_type_int, EndPage),
    pi("PDFStartPage", gs_param_type_int, StartPage),
    pi("Optimize", gs_param_type_bool, Optimize),
    pi("ParseDSCCommentsForDocInfo", gs_param_type_bool,
       ParseDSCCommentsForDocInfo),
    pi("ParseDSCComments", gs_param_type_bool, ParseDSCComments),
    pi("EmitDSCWarnings", gs_param_type_bool, EmitDSCWarnings),
    pi("CreateJobTicket", gs_param_type_bool, CreateJobTicket),
    pi("PreserveEPSInfo", gs_param_type_bool, PreserveEPSInfo),
    pi("AutoPositionEPSFiles", gs_param_type_bool, AutoPositionEPSFiles),
    pi("PreserveCopyPage", gs_param_type_bool, PreserveCopyPage),
    pi("UsePrologue", gs_param_type_bool, UsePrologue),

	/* Ghostscript-specific parameters */

    pi("ReAssignCharacters", gs_param_type_bool, ReAssignCharacters),
    pi("ReEncodeCharacters", gs_param_type_bool, ReEncodeCharacters),
    pi("FirstObjectNumber", gs_param_type_long, FirstObjectNumber),
#undef pi
    gs_param_item_end
};
  
/*
  Notes on implementing the remaining Distiller functionality
  ===========================================================

  Architectural issues
  --------------------

  Must disable all color conversions, so that driver gets original color
    and color space -- needs "protean" device color space
  Must optionally disable application of TR, BG, UCR similarly.  Affects:
    PreserveHalftoneInfo
    PreserveOverprintSettings
    TransferFunctionInfo
    UCRandBGInfo

  * = requires architectural change to complete

  Current limitations
  -------------------

  Non-primary elements in HalftoneType 5 are not written correctly
  Doesn't recognize Default TR/HT/BG/UCR
  Optimization is a separate program

  Optimizations
  -------------

  Create shared resources for Indexed (and other) color spaces
  Remember image XObject IDs for sharing
  Remember image and pattern MD5 fingerprints for sharing -- see
    CD-ROM from dhoff@margnat.com
  Merge font subsets?  (k/ricktest.ps, from rick@dgii.com re file output
    size ps2pdf vs. pstoedit)
  Minimize tables for embedded TT fonts (requires renumbering glyphs)
  Clip off image data outside bbox of clip path?

  Acrobat Distiller 3
  -------------------

  ---- Other functionality ----

  Embed CID fonts
  Compress TT, CFF, CID/TT, and CID/CFF if CompressPages
  Compress forms, Type 3 fonts, and Cos streams

  ---- Image parameters ----

  AntiAlias{Color,Gray,Mono}Images
  AutoFilter{Color,Gray}Images
    Needs to scan image
  Convert CIE images to Device if can't represent color space

  ---- Other parameters ----

  CompressPages
    Compress things other than page contents
  * PreserveHalftoneInfo
  PreserveOPIComments
    ? see OPI spec?
  * PreserveOverprintSettings
  * TransferFunctionInfo
  * UCRandBGInfo
  ColorConversionStrategy
    Select color space for drawing commands
  ConvertImagesToIndexed
    Postprocess image data *after* downsampling (requires an extra pass)

  Acrobat Distiller 4
  -------------------

  ---- Other functionality ----

  Document structure pdfmarks

  ---- Parameters ----

  xxxDownsampleType = /Bicubic
    Add new filter (or use siscale?) & to setup (gdevpsdi.c)
  Binding
    ? not sure where this goes (check with AD4)
  DetectBlends
    Idiom recognition?  PatternType 2 patterns / shfill?  (see AD4)
  DoThumbnails
    Also output to memory device -- resolution issue
  EndPage / StartPage
    Only affects AR? -- see what AD4 produces
  ###Profile
    Output in ICCBased color spaces
  ColorConversionStrategy
  * Requires suppressing CIE => Device color conversion
    Convert other CIE spaces to ICCBased
  CannotEmbedFontPolicy
    Check when trying to embed font -- how to produce warning?

  ---- Job-level control ----

  ParseDSCComments
  ParseDSCCommentsForDocInfo
  EmitDSCWarnings
    Require DSC parser / interceptor
  CreateJobTicket
    ?
  PreserveEPSInfo
  AutoPositionEPSFiles
    Require DSC parsing
  PreserveCopyPage
    Concatenate Contents streams
  UsePrologue
    Needs hack in top-level control?

*/

/* ---------------- Get parameters ---------------- */

/* Get parameters. */
int
gdev_pdf_get_params(gx_device * dev, gs_param_list * plist)
{
    gx_device_pdf *pdev = (gx_device_pdf *) dev;
    float cl = (float)pdev->CompatibilityLevel;
    int code = gdev_psdf_get_params(dev, plist);
    int cdv = CoreDistVersion;

    if (code < 0 ||
	(code = param_write_int(plist, "CoreDistVersion", &cdv)) < 0 ||
	(code = param_write_float(plist, "CompatibilityLevel", &cl)) < 0 ||
	/* Indicate that we can process pdfmark and DSC. */
	(param_requested(plist, "pdfmark") > 0 &&
	 (code = param_write_null(plist, "pdfmark")) < 0) ||
	(param_requested(plist, "DSC") > 0 &&
	 (code = param_write_null(plist, "DSC")) < 0) ||
	(code = gs_param_write_items(plist, pdev, NULL, pdf_param_items)) < 0
	);
    return code;
}

/* ---------------- Put parameters ---------------- */

/* Put parameters. */
int
gdev_pdf_put_params(gx_device * dev, gs_param_list * plist)
{
    gx_device_pdf *pdev = (gx_device_pdf *) dev;
    int ecode, code;
    gx_device_pdf save_dev;
    float cl = (float)pdev->CompatibilityLevel;
    bool locked = pdev->params.LockDistillerParams;
    gs_param_name param_name;

    /*
     * If this is a pseudo-parameter (pdfmark or DSC),
     * don't bother checking for any real ones.
     */

    {
	gs_param_string_array ppa;

	code = param_read_string_array(plist, (param_name = "pdfmark"), &ppa);
	switch (code) {
	    case 0:
		pdf_open_document(pdev);
		code = pdfmark_process(pdev, &ppa);
		if (code >= 0)
		    return code;
		/* falls through for errors */
	    default:
		param_signal_error(plist, param_name, code);
		return code;
	    case 1:
		break;
	}

	code = param_read_string_array(plist, (param_name = "DSC"), &ppa);
	switch (code) {
	    case 0:
		pdf_open_document(pdev);
		code = pdf_dsc_process(pdev, &ppa);
		if (code >= 0)
		    return code;
		/* falls through for errors */
	    default:
		param_signal_error(plist, param_name, code);
		return code;
	    case 1:
		break;
	}
    }
  
    /* Check for LockDistillerParams before doing anything else. */

    ecode = code = param_read_bool(plist, "LockDistillerParams", &locked);
    if (locked && pdev->params.LockDistillerParams)
	return ecode;

    /* General parameters. */

    {
	int cdv = CoreDistVersion;

	ecode = param_put_int(plist, (param_name = "CoreDistVersion"), &cdv, ecode);
	if (cdv != CoreDistVersion)
	    param_signal_error(plist, param_name, ecode = gs_error_rangecheck);
    }

    save_dev = *pdev;

    switch (code = param_read_float(plist, (param_name = "CompatibilityLevel"), &cl)) {
	default:
	    ecode = code;
	    param_signal_error(plist, param_name, ecode);
	case 0:
	case 1:
	    break;
    }

    code = gs_param_read_items(plist, pdev, pdf_param_items);
    if (code < 0)
	ecode = code;
    {
	/*
	 * Setting FirstObjectNumber is only legal if the file
	 * has just been opened and nothing has been written,
	 * or if we are setting it to the same value.
	 */
	long fon = pdev->FirstObjectNumber;

	if (fon != save_dev.FirstObjectNumber) {
	    if (fon <= 0 || fon > 0x7fff0000 ||
		(pdev->next_id != 0 &&
		 pdev->next_id !=
		 save_dev.FirstObjectNumber + pdf_num_initial_ids)
		) {
		ecode = gs_error_rangecheck;
		param_signal_error(plist, "FirstObjectNumber", ecode);
	    }
	}
    }
    {
	/*
	 * Set ProcessColorModel now, because gx_default_put_params checks
	 * it.
	 */
	static const char *const pcm_names[] = {
	    "DeviceGray", "DeviceRGB", "DeviceCMYK", 0
	};
	static const gx_device_color_info pcm_color_info[] = {
	    dci_values(1, 8, 255, 0, 256, 0),
	    dci_values(3, 24, 255, 255, 256, 256),
	    dci_values(4, 32, 255, 255, 256, 256)
	};
	int pcm = -1;

	ecode = param_put_enum(plist, "ProcessColorModel", &pcm,
			       pcm_names, ecode);
	if (pcm >= 0) {
	    pdev->color_info = pcm_color_info[pcm];
	    pdf_set_process_color_model(pdev);
	}
    }
    if (ecode < 0)
	goto fail;
    /*
     * We have to set version to the new value, because the set of
     * legal parameter values for psdf_put_params varies according to
     * the version.
     */
    pdev->version = (cl < 1.2 ? psdf_version_level2 : psdf_version_ll3);
    ecode = gdev_psdf_put_params(dev, plist);
    if (ecode < 0)
	goto fail;
    /*
     * Acrobat Reader 4.0 and earlier don't handle user-space coordinates
     * larger than 32K.  To compensate for this, reduce the resolution so that
     * the page size in device space (which we equate to user space) is
     * significantly less than 32K.  Note that this still does not protect
     * us against input files that use coordinates far outside the page
     * boundaries.
     */
#define MAX_EXTENT 28000
    /* Changing resolution or page size requires closing the device, */
    if (dev->height > MAX_EXTENT || dev->width > MAX_EXTENT) {
	double factor =
	    max(dev->height / (double)MAX_EXTENT,
		dev->width / (double)MAX_EXTENT);

	if (dev->is_open)
	    gs_closedevice(dev);
	gx_device_set_resolution(dev, dev->HWResolution[0] / factor,
				 dev->HWResolution[1] / factor);
    }
#undef MAX_EXTENT
    if (pdev->FirstObjectNumber != save_dev.FirstObjectNumber) {
	if (pdev->xref.file != 0) {
	    fseek(pdev->xref.file, 0L, SEEK_SET);
	    pdf_initialize_ids(pdev);
	}
    }
    /* Handle the float/double mismatch. */
    pdev->CompatibilityLevel = (int)(cl * 10 + 0.5) / 10.0;
    return 0;
 fail:
    /* Restore all the parameters to their original state. */
    pdev->version = save_dev.version;
    pdev->color_info = save_dev.color_info;
    pdf_set_process_color_model(pdev);
    {
	const gs_param_item_t *ppi = pdf_param_items;

	for (; ppi->key; ++ppi)
	    memcpy((char *)pdev + ppi->offset,
		   (char *)&save_dev + ppi->offset,
		   gs_param_type_sizes[ppi->type]);
    }
    return ecode;
}

/* ---------------- Process DSC comments ---------------- */

private int
pdf_dsc_process(gx_device_pdf * pdev, const gs_param_string_array * pma)
{
    /* This is just a place-holder. */
    return 0;
}

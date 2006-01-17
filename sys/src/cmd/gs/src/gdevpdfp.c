/* Copyright (C) 1996, 2000, 2001 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpdfp.c,v 1.53 2005/09/12 11:34:50 leonardo Exp $ */
/* Get/put parameters for PDF-writing driver */
#include "memory_.h"
#include "string_.h"
#include "gx.h"
#include "gserrors.h"
#include "gdevpdfx.h"
#include "gdevpdfo.h"
#include "gdevpdfg.h"
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
private int pdf_dsc_process(gx_device_pdf * pdev,
			    const gs_param_string_array * pma);

private const int CoreDistVersion = 5000;	/* Distiller 5.0 */
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

	/* Acrobat Distiller 5 parameters */

    pi("OffOptimizations", gs_param_type_int, OffOptimizations),

	/* Ghostscript-specific parameters */

    pi("ReAssignCharacters", gs_param_type_bool, ReAssignCharacters),
    pi("ReEncodeCharacters", gs_param_type_bool, ReEncodeCharacters),
    pi("FirstObjectNumber", gs_param_type_long, FirstObjectNumber),
    pi("CompressFonts", gs_param_type_bool, CompressFonts),
    pi("PrintStatistics", gs_param_type_bool, PrintStatistics),
    pi("MaxInlineImageSize", gs_param_type_long, MaxInlineImageSize),

	/* PDF Encryption */
    pi("OwnerPassword", gs_param_type_string, OwnerPassword),
    pi("UserPassword", gs_param_type_string, UserPassword),
    pi("KeyLength", gs_param_type_int, KeyLength),
    pi("Permissions", gs_param_type_int, Permissions),
    pi("EncryptionR", gs_param_type_int, EncryptionR),
    pi("NoEncrypt", gs_param_type_string, NoEncrypt),

	/* Target viewer capabilities (Ghostscript-specific)  */
    pi("ForOPDFRead", gs_param_type_bool, ForOPDFRead),
    pi("PatternImagemask", gs_param_type_bool, PatternImagemask),
    pi("MaxClipPathSize", gs_param_type_int, MaxClipPathSize),
    pi("MaxShadingBitmapSize", gs_param_type_int, MaxShadingBitmapSize),
    pi("MaxViewerMemorySize", gs_param_type_int, MaxViewerMemorySize),
    pi("HaveTrueTypes", gs_param_type_bool, HaveTrueTypes),
    pi("HaveCIDSystem", gs_param_type_bool, HaveCIDSystem),
    pi("HaveTransparency", gs_param_type_bool, HaveTransparency),
    pi("OPDFReadProcsetPath", gs_param_type_string, OPDFReadProcsetPath),
    pi("CompressEntireFile", gs_param_type_bool, CompressEntireFile),
    pi("PDFX", gs_param_type_bool, PDFX),
#undef pi
    gs_param_item_end
};
  
/*
  Notes on implementing the remaining Distiller functionality
  ===========================================================

  Architectural issues
  --------------------

  Must optionally disable application of TR, BG, UCR similarly.  Affects:
    PreserveHalftoneInfo
    PreserveOverprintSettings
    TransferFunctionInfo
    UCRandBGInfo

  Current limitations
  -------------------

  Non-primary elements in HalftoneType 5 are not written correctly

  Acrobat Distiller 3
  -------------------

  ---- Image parameters ----

  AntiAlias{Color,Gray,Mono}Images

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
  DetectBlends
    Idiom recognition?  PatternType 2 patterns / shfill?  (see AD4)
  DoThumbnails
    Also output to memory device -- resolution issue

  ---- Job-level control ----

  EmitDSCWarnings
    Require DSC parser / interceptor
  CreateJobTicket
    ?
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
    int EmbedFontObjects = 1;

    if (code < 0 ||
	(code = param_write_int(plist, ".EmbedFontObjects", &EmbedFontObjects)) < 0 ||
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
		code = pdf_open_document(pdev);
		if (code < 0)
		    return code;
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
		code = pdf_open_document(pdev);
		if (code < 0)
		    return code;
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
  
    /*
     * Check for LockDistillerParams before doing anything else.
     * If LockDistillerParams is true and is not being set to false,
     * ignore all resettings of PDF-specific parameters.  Note that
     * LockDistillerParams is read again, and reset if necessary, in
     * psdf_put_params.
     */
    ecode = code = param_read_bool(plist, "LockDistillerParams", &locked);
 
    if (!(locked && pdev->params.LockDistillerParams)) {
	/* General parameters. */

	{
	    int efo = 1;

	    ecode = param_put_int(plist, (param_name = ".EmbedFontObjects"), &efo, ecode);
	    if (efo != 1)
		param_signal_error(plist, param_name, ecode = gs_error_rangecheck);
	}
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
		/*
		 * Must be 1.2, 1.3, or 1.4.  Per Adobe documentation, substitute
		 * the nearest achievable value.
		 */
		if (cl < (float)1.15)
		    cl = (float)1.1;
		else if (cl < (float)1.25)
		    cl = (float)1.2;
		else if (cl >= (float)1.35)
		    cl = (float)1.4;
		else
		    cl = (float)1.3;
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
		"DeviceGray", "DeviceRGB", "DeviceCMYK", "DeviceN", 0
	    };
	    int pcm = -1;

	    ecode = param_put_enum(plist, "ProcessColorModel", &pcm,
				   pcm_names, ecode);
	    if (pcm >= 0) {
		pdf_set_process_color_model(pdev, pcm);
		pdf_set_initial_color(pdev, &pdev->saved_fill_color, &pdev->saved_stroke_color,
				&pdev->fill_used_process_color, &pdev->stroke_used_process_color);
	    }
	}
    }
    if (ecode < 0)
	goto fail;
    /*
     * We have to set version to the new value, because the set of
     * legal parameter values for psdf_put_params varies according to
     * the version.
     */
    if (pdev->PDFX)
	cl = (float)1.3; /* Instead pdev->CompatibilityLevel = 1.2; - see below. */
    pdev->version = (cl < 1.2 ? psdf_version_level2 : psdf_version_ll3);
    if (pdev->ForOPDFRead) {
	pdev->ResourcesBeforeUsage = true;
	pdev->HaveCFF = false;
	pdev->HavePDFWidths = false;
	pdev->HaveStrokeColor = false;
	cl = (float)1.2; /* Instead pdev->CompatibilityLevel = 1.2; - see below. */
	pdev->MaxInlineImageSize = max_long; /* Save printer's RAM from saving temporary image data.
					        Immediate images doen't need buffering. */
	pdev->version = psdf_version_level2;
    } else {
	pdev->ResourcesBeforeUsage = false;
	pdev->HaveCFF = true;
	pdev->HavePDFWidths = true;
	pdev->HaveStrokeColor = true;
    }
    ecode = gdev_psdf_put_params(dev, plist);
    if (ecode < 0)
	goto fail;
    if (pdev->HaveTrueTypes && pdev->version == psdf_version_level2) {
	pdev->version = psdf_version_level2_with_TT ;
    }
    /*
     * Acrobat Reader doesn't handle user-space coordinates larger than
     * MAX_USER_COORD.  To compensate for this, reduce the resolution so
     * that the page size in device space (which we equate to user space) is
     * significantly less than MAX_USER_COORD.  Note that this still does
     * not protect us against input files that use coordinates far outside
     * the page boundaries.
     */
#define MAX_EXTENT ((int)(MAX_USER_COORD * 0.9))
    /* Changing resolution or page size requires closing the device, */
    if (dev->height > MAX_EXTENT || dev->width > MAX_EXTENT) {
	double factor =
	    max(dev->height / (double)MAX_EXTENT,
		dev->width / (double)MAX_EXTENT);

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
    pdf_set_process_color_model(pdev, save_dev.pcm_color_info_index);
    pdev->saved_fill_color = save_dev.saved_fill_color;
    pdev->saved_stroke_color = save_dev.saved_fill_color;
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
    /*
     * The Adobe "Distiller Parameters" documentation says that Distiller
     * looks at DSC comments, but it doesn't say which ones.  We look at
     * the ones that we see how to map directly to obvious PDF constructs.
     */
    int code = 0;
    int i;

    /*
     * If ParseDSCComments is false, all DSC comments are ignored, even if
     * ParseDSCComentsForDocInfo or PreserveEPSInfo is true.
     */
    if (!pdev->ParseDSCComments)
	return 0;

    for (i = 0; i + 1 < pma->size && code >= 0; i += 2) {
	const gs_param_string *pkey = &pma->data[i];
	const gs_param_string *pvalue = &pma->data[i + 1];
	const char *key;
	int code;

	/*
	 * %%For, %%Creator, and %%Title are recognized only if either
	 * ParseDSCCommentsForDocInfo or PreserveEPSInfo is true.
	 * The other DSC comments are always recognized.
	 *
	 * Acrobat Distiller sets CreationDate and ModDate to the current
	 * time, not the value of %%CreationDate.  We think this is wrong,
	 * but we do the same -- we ignore %%CreationDate here.
	 */

	if (pdf_key_eq(pkey, "Creator"))
	    key = "/Creator";
	else if (pdf_key_eq(pkey, "Title"))
	    key = "/Title";
	else if (pdf_key_eq(pkey, "For"))
	    key = "/Author";
	else {
	    pdf_page_dsc_info_t *ppdi;

	    if ((ppdi = &pdev->doc_dsc_info,
		 pdf_key_eq(pkey, "Orientation")) ||
		(ppdi = &pdev->page_dsc_info,
		 pdf_key_eq(pkey, "PageOrientation"))
		) {
		if (pvalue->size == 1 && pvalue->data[0] >= '0' &&
		    pvalue->data[0] <= '3'
		    )
		    ppdi->orientation = pvalue->data[0] - '0';
		else
		    ppdi->orientation = -1;
	    } else if ((ppdi = &pdev->doc_dsc_info,
			pdf_key_eq(pkey, "ViewingOrientation")) ||
		       (ppdi = &pdev->page_dsc_info,
			pdf_key_eq(pkey, "PageViewingOrientation"))
		       ) {
		gs_matrix mat;
		int orient;

		if (sscanf((const char *)pvalue->data, "[%g %g %g %g]",
			   &mat.xx, &mat.xy, &mat.yx, &mat.yy) != 4
		    )
		    continue;	/* error */
		for (orient = 0; orient < 4; ++orient) {
		    if (mat.xx == 1 && mat.xy == 0 && mat.yx == 0 && mat.yy == 1)
			break;
		    gs_matrix_rotate(&mat, -90.0, &mat);
		}
		if (orient == 4) /* error */
		    orient = -1;
		ppdi->viewing_orientation = orient;
	    } else {
		gs_rect box;

		if (pdf_key_eq(pkey, "EPSF")) {
		    pdev->is_EPS = (pvalue->size >= 1 && pvalue->data[0] != '0');
		    continue;
		}
		/*
		 * We only parse the BoundingBox for the sake of
		 * AutoPositionEPSFiles.
		 */
		if (pdf_key_eq(pkey, "BoundingBox"))
		    ppdi = &pdev->doc_dsc_info;
		else if (pdf_key_eq(pkey, "PageBoundingBox"))
		    ppdi = &pdev->page_dsc_info;
		else
		    continue;
		if (sscanf((const char *)pvalue->data, "[%lg %lg %lg %lg]",
			   &box.p.x, &box.p.y, &box.q.x, &box.q.y) != 4
		    )
		    continue;	/* error */
		ppdi->bounding_box = box;
	    }
	    continue;
	}

	if (pdev->ParseDSCCommentsForDocInfo || pdev->PreserveEPSInfo)
	    code = cos_dict_put_c_key_string(pdev->Info, key,
					     pvalue->data, pvalue->size);
    }
    return code;
}

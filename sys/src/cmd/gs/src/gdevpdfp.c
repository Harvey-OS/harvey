/* Copyright (C) 1996, 2000 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevpdfp.c,v 1.2 2000/03/16 01:21:24 lpd Exp $ */
/* Get/put parameters for PDF-writing driver */
#include "gx.h"
#include "gserrors.h"
#include "gdevpdfx.h"
#include "gsparamx.h"
#ifdef POST60
#include "memory_.h"		/* should be first */
#endif

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
 */

#ifdef POST60
private const int CoreDistVersion = 4000;	/* Distiller 4.0 */
private const gs_param_item_t pdf_param_items[] = {
#define pi(key, type, memb) { key, type, offset_of(gx_device_pdf, memb) }
    /* Acrobat Distiller 4 parameters */
    pi("Optimize", gs_param_type_bool, Optimize),
    pi("ParseDSCCommentsForDocInfo", gs_param_type_bool,
       ParseDSCCommentsForDocInfo),
    pi("ParseDSCComments", gs_param_type_bool, ParseDSCComments),
    pi("EmitDSCWarnings", gs_param_type_bool, EmitDSCWarnings),
    pi("CreateJobTicket", gs_param_type_bool, CreateJobTicket),
    pi("PreserveEPSInfo", gs_param_type_bool, PreserveEPSInfo),
    pi("AutoPositionEPSFile", gs_param_type_bool, AutoPositionEPSFile),
    pi("PreserveCopyPage", gs_param_type_bool, PreserveCopyPage),
    pi("UsePrologue", gs_param_type_bool, UsePrologue),
    /* Ghostscript-specific parameters */
    pi("ReAssignCharacters", gs_param_type_bool, ReAssignCharacters),
    pi("ReEncodeCharacters", gs_param_type_bool, ReEncodeCharacters),
    pi("FirstObjectNumber", gs_param_type_long, FirstObjectNumber),
#undef pi
    gs_param_item_end
};
#else
private const int CoreDistVersion = 3000;	/* Distiller 3.0 */
#endif

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
#ifdef POST60
	(code = gs_param_write_items(plist, pdev, NULL, pdf_param_items)) < 0
#else
	(code = param_write_bool(plist, "ReAssignCharacters",
				 &pdev->ReAssignCharacters)) < 0 ||
	(code = param_write_bool(plist, "ReEncodeCharacters",
				 &pdev->ReEncodeCharacters)) < 0 ||
	(code = param_write_long(plist, "FirstObjectNumber",
				 &pdev->FirstObjectNumber)) < 0
#endif
	);
    return code;
}

/* ---------------- Put parameters ---------------- */

/* Put parameters. */
int
gdev_pdf_put_params(gx_device * dev, gs_param_list * plist)
{
    gx_device_pdf *pdev = (gx_device_pdf *) dev;
    int ecode = 0;
    int code;
    gx_device_pdf save_dev;
#ifndef POST60
    float cl = (float)pdev->CompatibilityLevel;
    bool rac = pdev->ReAssignCharacters;
    bool rec = pdev->ReEncodeCharacters;
    long fon = pdev->FirstObjectNumber;
    psdf_version save_version = pdev->version;
#endif
    gs_param_name param_name;

    /*
     * If this is a pseudo-parameter (show or pdfmark),
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
    }

    /* General parameters. */

    {
	int cdv = CoreDistVersion;

	ecode = param_put_int(plist, (param_name = "CoreDistVersion"), &cdv, ecode);
	if (cdv != CoreDistVersion)
	    param_signal_error(plist, param_name, ecode = gs_error_rangecheck);
    }

    save_dev = *pdev;
#ifdef POST60
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
    if (ecode < 0)
	goto fail;
    /*
     * We have to set version to the new value, because the set of
     * legal parameter values for psdf_put_params varies according to
     * the version.
     */
    pdev->version =
	(pdev->CompatibilityLevel < 1.2 ? psdf_version_level2 :
	 psdf_version_ll3);
    ecode = gdev_psdf_put_params(dev, plist);
    if (ecode < 0)
	goto fail;
    if (pdev->FirstObjectNumber != save_dev.FirstObjectNumber) {
	if (pdev->xref.file != 0) {
	    fseek(pdev->xref.file, 0L, SEEK_SET);
	    pdf_initialize_ids(pdev);
	}
    }
    return 0;
 fail:
    /* Restore all the parameters to their original state. */
    pdev->version = save_dev.version;
    {
	const gs_param_item_t *ppi = pdf_param_items;

	for (; ppi->key; ++ppi)
	    memcpy((char *)pdev + ppi->offset,
		   (char *)&save_dev + ppi->offset,
		   gs_param_type_sizes[ppi->type]);
    }
    return ecode;
#else
    switch (code = param_read_float(plist, (param_name = "CompatibilityLevel"), &cl)) {
	default:
	    ecode = code;
	    param_signal_error(plist, param_name, ecode);
	case 0:
	case 1:
	    break;
    }
    ecode = param_put_bool(plist, "ReAssignCharacters", &rac, ecode);
    ecode = param_put_bool(plist, "ReEncodeCharacters", &rec, ecode);
    switch (code = param_read_long(plist, (param_name = "FirstObjectNumber"), &fon)) {
	case 0:
	    /*
	     * Setting this parameter is only legal if the file
	     * has just been opened and nothing has been written,
	     * or if we are setting it to the same value.
	     */
	    if (fon == pdev->FirstObjectNumber)
		break;
	    if (fon <= 0 || fon > 0x7fff0000 ||
		(pdev->next_id != 0 &&
		 pdev->next_id !=
		 pdev->FirstObjectNumber + pdf_num_initial_ids)
		)
		ecode = gs_error_rangecheck;
	    else
		break;
	default:
	    ecode = code;
	    param_signal_error(plist, param_name, ecode);
	case 1:
	    break;
    }
    {
	/*
	 * Set ProcessColorModel now, because gx_default_put_params checks
	 * it.
	 */
	static const char *pcm_names[] = {
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
	return ecode;
    /*
     * We have to set version to the new value, because the set of
     * legal parameter values for psdf_put_params varies according to
     * the version.
     */
    pdev->version =
	(cl < 1.2 ? psdf_version_level2 : psdf_version_ll3);
    code = gdev_psdf_put_params(dev, plist);
    if (code < 0) {
	pdev->version = save_version;
	pdev->color_info = save_dev.color_info;
	pdf_set_process_color_model(pdev);
	return code;
    }

    /*
     * Acrobat Reader 4.0 and earlier don't handle user-space coordinates
     * larger than 32K.  To compensate for this, reduce the resolution until
     * the page size in device space (which we equate to user space)
     * is significantly less than 32K.  Note
     * that this still does not protect us against input files that use
     * coordinates far outside the page boundaries.
     */
    /* Changing resolution or page size requires closing the device, */
    while (dev->height > 16000 || dev->width > 16000) {
	if (dev->is_open)
	    gs_closedevice(dev);
	gx_device_set_resolution(dev, dev->HWResolution[0] / 2,
				 dev->HWResolution[1] / 2);
    }

    /* Handle the float/double mismatch. */
    pdev->CompatibilityLevel = (int)(cl * 10 + 0.5) / 10.0;
    pdev->ReAssignCharacters = rac;
    pdev->ReEncodeCharacters = rec;
    if (fon != pdev->FirstObjectNumber) {
	pdev->FirstObjectNumber = fon;
	if (pdev->xref.file != 0) {
	    fseek(pdev->xref.file, 0L, SEEK_SET);
	    pdf_initialize_ids(pdev);
	}
    }
    return 0;
#endif
}

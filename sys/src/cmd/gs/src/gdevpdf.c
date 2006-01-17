/* Copyright (C) 1996, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpdf.c,v 1.124 2005/09/12 11:34:50 leonardo Exp $ */
/* PDF-writing driver */
#include "fcntl_.h"
#include "memory_.h"
#include "string_.h"
#include "time_.h"
#include "unistd_.h"
#include "gx.h"
#include "gp.h"			/* for gp_get_realtime */
#include "gserrors.h"
#include "gxdevice.h"
#include "gdevpdfx.h"
#include "gdevpdfg.h"		/* only for pdf_reset_graphics */
#include "gdevpdfo.h"
#include "gdevpdt.h"
#include "smd5.h"
#include "sarc4.h"

/* Define the default language level and PDF compatibility level. */
/* Acrobat 4 (PDF 1.3) is the default. */
#define PSDF_VERSION_INITIAL psdf_version_ll3
#define PDF_COMPATIBILITY_LEVEL_INITIAL 1.4

/* Define the size of internal stream buffers. */
/* (This is not a limitation, it only affects performance.) */
#define sbuf_size 512

/* GC descriptors */
private_st_pdf_page();
gs_private_st_element(st_pdf_page_element, pdf_page_t, "pdf_page_t[]",
		      pdf_page_elt_enum_ptrs, pdf_page_elt_reloc_ptrs,
		      st_pdf_page);
private_st_device_pdfwrite();
private_st_pdf_substream_save();
private_st_pdf_substream_save_element();

/* GC procedures */
private 
ENUM_PTRS_WITH(device_pdfwrite_enum_ptrs, gx_device_pdf *pdev)
{
    index -= gx_device_pdf_num_ptrs + gx_device_pdf_num_param_strings;
    if (index < NUM_RESOURCE_TYPES * NUM_RESOURCE_CHAINS)
	ENUM_RETURN(pdev->resources[index / NUM_RESOURCE_CHAINS].chains[index % NUM_RESOURCE_CHAINS]);
    index -= NUM_RESOURCE_TYPES * NUM_RESOURCE_CHAINS;
    if (index <= pdev->outline_depth)
	ENUM_RETURN(pdev->outline_levels[index].first.action);
    index -= pdev->outline_depth + 1;
    if (index <= pdev->outline_depth)
	ENUM_RETURN(pdev->outline_levels[index].last.action);
    index -= pdev->outline_depth + 1;
    ENUM_PREFIX(st_device_psdf, 0);
}
#define e1(i,elt) ENUM_PTR(i, gx_device_pdf, elt);
gx_device_pdf_do_ptrs(e1)
#undef e1
#define e1(i,elt) ENUM_PARAM_STRING_PTR(i + gx_device_pdf_num_ptrs, gx_device_pdf, elt);
gx_device_pdf_do_param_strings(e1)
#undef e1
#define e1(i,elt) ENUM_STRING_PTR(i + gx_device_pdf_num_ptrs + gx_device_pdf_num_param_strings,\
				 gx_device_pdf, elt);
gx_device_pdf_do_const_strings(e1)
#undef e1
ENUM_PTRS_END
private RELOC_PTRS_WITH(device_pdfwrite_reloc_ptrs, gx_device_pdf *pdev)
{
    RELOC_PREFIX(st_device_psdf);
#define r1(i,elt) RELOC_PTR(gx_device_pdf,elt);
    gx_device_pdf_do_ptrs(r1)
#undef r1
#define r1(i,elt) RELOC_PARAM_STRING_PTR(gx_device_pdf,elt);
	gx_device_pdf_do_param_strings(r1)
#undef r1
#define r1(i,elt) RELOC_CONST_STRING_PTR(gx_device_pdf,elt);
	gx_device_pdf_do_const_strings(r1)
#undef r1
    {
	int i, j;

	for (i = 0; i < NUM_RESOURCE_TYPES; ++i)
	    for (j = 0; j < NUM_RESOURCE_CHAINS; ++j)
		RELOC_PTR(gx_device_pdf, resources[i].chains[j]);
	for (i = 0; i <= pdev->outline_depth; ++i) {
	    RELOC_PTR(gx_device_pdf, outline_levels[i].first.action);
	    RELOC_PTR(gx_device_pdf, outline_levels[i].last.action);
	}
    }
}
RELOC_PTRS_END
/* Even though device_pdfwrite_finalize is the same as gx_device_finalize, */
/* we need to implement it separately because st_composite_final */
/* declares all 3 procedures as private. */
private void
device_pdfwrite_finalize(void *vpdev)
{
    gx_device_finalize(vpdev);
}

/* Driver procedures */
private dev_proc_open_device(pdf_open);
private dev_proc_output_page(pdf_output_page);
private dev_proc_close_device(pdf_close);
/* Driver procedures defined in other files are declared in gdevpdfx.h. */

#ifndef X_DPI
#  define X_DPI 720
#endif
#ifndef Y_DPI
#  define Y_DPI 720
#endif

/* ---------------- Device prototype ---------------- */

#define PDF_DEVICE_NAME "pdfwrite"
#define PDF_DEVICE_IDENT gs_pdfwrite_device
#define PDF_DEVICE_MaxInlineImageSize 4000
#define PDF_FOR_OPDFREAD 0

#include "gdevpdfb.h"

#undef PDF_DEVICE_NAME
#undef PDF_DEVICE_IDENT
#undef PDF_DEVICE_MaxInlineImageSize
#undef PDF_FOR_OPDFREAD

#define PDF_DEVICE_NAME "ps2write"
#define PDF_DEVICE_IDENT gs_ps2write_device
#define PDF_DEVICE_MaxInlineImageSize max_long
#define PDF_FOR_OPDFREAD 1

#include "gdevpdfb.h"

#undef PDF_DEVICE_NAME
#undef PDF_DEVICE_IDENT
#undef PDF_DEVICE_MaxInlineImageSize
#undef PDF_FOR_OPDFREAD
/* ---------------- Device open/close ---------------- */

/* Close and remove temporary files. */
private int
pdf_close_temp_file(gx_device_pdf *pdev, pdf_temp_file_t *ptf, int code)
{
    int err = 0;
    FILE *file = ptf->file;

    /*
     * ptf->strm == 0 or ptf->file == 0 is only possible if this procedure
     * is called to clean up during initialization failure, but ptf->strm
     * might not be open if it was finalized before the device was closed.
     */
    if (ptf->strm) {
	if (s_is_valid(ptf->strm)) {
	    sflush(ptf->strm);
	    /* Prevent freeing the stream from closing the file. */
	    ptf->strm->file = 0;
	} else
	    ptf->file = file = 0;	/* file was closed by finalization */
	gs_free_object(pdev->pdf_memory, ptf->strm_buf,
		       "pdf_close_temp_file(strm_buf)");
	ptf->strm_buf = 0;
	gs_free_object(pdev->pdf_memory, ptf->strm,
		       "pdf_close_temp_file(strm)");
	ptf->strm = 0;
    }
    if (file) {
	err = ferror(file) | fclose(file);
	unlink(ptf->file_name);
	ptf->file = 0;
    }
    ptf->save_strm = 0;
    return
	(code < 0 ? code : err != 0 ? gs_note_error(gs_error_ioerror) : code);
}
private int
pdf_close_files(gx_device_pdf * pdev, int code)
{
    code = pdf_close_temp_file(pdev, &pdev->pictures, code);
    code = pdf_close_temp_file(pdev, &pdev->streams, code);
    code = pdf_close_temp_file(pdev, &pdev->asides, code);
    return pdf_close_temp_file(pdev, &pdev->xref, code);
}

/* Reset the state of the current page. */
private void
pdf_reset_page(gx_device_pdf * pdev)
{
    pdev->page_dsc_info = gs_pdfwrite_device.page_dsc_info;
    pdev->contents_id = 0;
    pdf_reset_graphics(pdev);
    pdev->procsets = NoMarks;
    memset(pdev->cs_Patterns, 0, sizeof(pdev->cs_Patterns));	/* simplest to create for each page */
    pdf_reset_text_page(pdev->text);
    pdf_remember_clip_path(pdev, 0);
    pdev->clip_path_id = pdev->no_clip_path_id;
}

/* Open a temporary file, with or without a stream. */
private int
pdf_open_temp_file(gx_device_pdf *pdev, pdf_temp_file_t *ptf)
{
    char fmode[4];

    strcpy(fmode, "w+");
    strcat(fmode, gp_fmode_binary_suffix);
    ptf->file =
	gp_open_scratch_file(gp_scratch_file_name_prefix,
			     ptf->file_name, fmode);
    if (ptf->file == 0)
	return_error(gs_error_invalidfileaccess);
    return 0;
}
private int
pdf_open_temp_stream(gx_device_pdf *pdev, pdf_temp_file_t *ptf)
{
    int code = pdf_open_temp_file(pdev, ptf);

    if (code < 0)
	return code;
    ptf->strm = s_alloc(pdev->pdf_memory, "pdf_open_temp_stream(strm)");
    if (ptf->strm == 0)
	return_error(gs_error_VMerror);
    ptf->strm_buf = gs_alloc_bytes(pdev->pdf_memory, sbuf_size,
				   "pdf_open_temp_stream(strm_buf)");
    if (ptf->strm_buf == 0) {
	gs_free_object(pdev->pdf_memory, ptf->strm,
		       "pdf_open_temp_stream(strm)");
	ptf->strm = 0;
	return_error(gs_error_VMerror);
    }
    swrite_file(ptf->strm, ptf->file, ptf->strm_buf, sbuf_size);
    return 0;
}

/* Initialize the IDs allocated at startup. */
void
pdf_initialize_ids(gx_device_pdf * pdev)
{
    gs_param_string nstr;

    pdev->next_id = pdev->FirstObjectNumber;

    /* Initialize the Catalog. */

    param_string_from_string(nstr, "{Catalog}");
    pdf_create_named_dict(pdev, &nstr, &pdev->Catalog, 0L);

    /* Initialize the Info dictionary. */

    param_string_from_string(nstr, "{DocInfo}");
    pdf_create_named_dict(pdev, &nstr, &pdev->Info, 0L);
    {
	char buf[PDF_MAX_PRODUCER];

	pdf_store_default_Producer(buf);
	cos_dict_put_c_key_string(pdev->Info, "/Producer", (byte *)buf,
				  strlen(buf));
    }
    /*
     * Acrobat Distiller sets CreationDate and ModDate to the current
     * date and time, rather than (for example) %%CreationDate from the
     * PostScript file.  We think this is wrong, but we do the same.
     */
    {
	struct tm tms;
	time_t t;
	char buf[1+2+4+2+2+2+2+2+2+1+1]; /* (D:yyyymmddhhmmss)\0 */

	time(&t);
	tms = *localtime(&t);
	sprintf(buf,
		"(D:%04d%02d%02d%02d%02d%02d)",
		tms.tm_year + 1900, tms.tm_mon + 1, tms.tm_mday,
		tms.tm_hour, tms.tm_min, tms.tm_sec);
	cos_dict_put_c_key_string(pdev->Info, "/CreationDate", (byte *)buf,
				  strlen(buf));
	cos_dict_put_c_key_string(pdev->Info, "/ModDate", (byte *)buf,
				  strlen(buf));
    }

    /* Allocate the root of the pages tree. */

    pdf_create_named_dict(pdev, NULL, &pdev->Pages, 0L);
}

private int
pdf_compute_fileID(gx_device_pdf * pdev)
{
    /* We compute a file identifier when beginning a document
       to allow its usage with PDF encryption. Due to that,
       in contradiction to the Adobe recommendation, our
       ID doesn't depend on the document size. 
    */
    gs_memory_t *mem = pdev->pdf_memory;
    stream *strm = pdev->strm;
    uint ignore;
    int code;
    stream *s = s_MD5E_make_stream(mem, pdev->fileID, sizeof(pdev->fileID));
    long secs_ns[2];
    uint KeyLength = pdev->KeyLength;

    if (s == NULL)
	return_error(gs_error_VMerror);
    pdev->KeyLength = 0; /* Disable encryption. Not so important though. */
    gp_get_usertime(secs_ns);
    sputs(s, (byte *)secs_ns, sizeof(secs_ns), &ignore);
    sputs(s, (const byte *)pdev->fname, strlen(pdev->fname), &ignore);
    pdev->strm = s;
    code = cos_dict_elements_write(pdev->Info, pdev);
    pdev->strm = strm;
    pdev->KeyLength = KeyLength;
    if (code < 0)
	return code;
    sclose(s);
    gs_free_object(mem, s, "pdf_compute_fileID");
#if 0
    memcpy(pdev->fileID, "xxxxxxxxxxxxxxxx", sizeof(pdev->fileID)); /* Debug */
#endif
    return 0;
}

private const byte pad[32] = { 0x28, 0xBF, 0x4E, 0x5E, 0x4E, 0x75, 0x8A, 0x41, 
			       0x64, 0x00, 0x4E, 0x56, 0xFF, 0xFA, 0x01, 0x08,
			       0x2E, 0x2E, 0x00, 0xB6, 0xD0, 0x68, 0x3E, 0x80,
			       0x2F, 0x0C, 0xA9, 0xFE, 0x64, 0x53, 0x69, 0x7A};

private inline void
copy_padded(byte buf[32], gs_param_string *str)
{
    memcpy(buf, str->data, min(str->size, 32));
    if (32 > str->size)
	memcpy(buf + str->size, pad, 32 - str->size);
}

private void
Adobe_magic_loop_50(byte digest[16], int key_length)
{
    md5_state_t md5;
    int i;

    for (i = 0; i < 50; i++) {
	md5_init(&md5);
	md5_append(&md5, digest, key_length);
	md5_finish(&md5, digest);
    }
}

private void
Adobe_magic_loop_19(byte *data, int data_size, const byte *key, int key_size)
{
    stream_arcfour_state sarc4;
    byte key_buf[16];
    int i, j;

    for (i = 1; i <= 19; i++) {
	for (j = 0; j < key_size; j++)
	    key_buf[j] = key[j] ^ (byte)i;
	s_arcfour_set_key(&sarc4, key_buf, key_size);
	s_arcfour_process_buffer(&sarc4, data, data_size);
    }
}

private int
pdf_compute_encryption_data(gx_device_pdf * pdev)
{
    md5_state_t md5;
    byte digest[16], buf[32], t;
    stream_arcfour_state sarc4;

    if (pdev->PDFX && pdev->KeyLength != 0) {
	eprintf("Encryption is not allowed in a PDF/X doucment.\n");
	return_error(gs_error_rangecheck);
    }
    if (pdev->KeyLength == 0)
	pdev->KeyLength = 40;
    if (pdev->EncryptionV == 0 && pdev->KeyLength == 40)
	pdev->EncryptionV = 1;	
    if (pdev->EncryptionV == 0 && pdev->KeyLength > 40)
	pdev->EncryptionV = 2;	
    if (pdev->EncryptionV > 1 && pdev->CompatibilityLevel < 1.4) {
	eprintf("PDF 1.3 only supports 40 bits keys.\n");
	return_error(gs_error_rangecheck);
    }
    if (pdev->EncryptionR == 0)
	pdev->EncryptionR = 2;
    if (pdev->EncryptionR < 2 || pdev->EncryptionR > 3) {
	eprintf("Encryption revisions 2 and 3 are only supported.\n");
	return_error(gs_error_rangecheck);
    }
    if (pdev->EncryptionR > 2 && pdev->CompatibilityLevel < 1.4) {
	eprintf("PDF 1.3 only supports the encryption revision 2.\n");
	return_error(gs_error_rangecheck);
    }
    if (pdev->KeyLength > 128) {
	eprintf("The maximal length of PDF encryption key is 128 bits.\n");
	return_error(gs_error_rangecheck);
    }
    if (pdev->KeyLength % 8) {
	eprintf("PDF encryption key length must be a multiple of 8.\n");
	return_error(gs_error_rangecheck);
    }
    if (pdev->EncryptionR == 2 &&
	((pdev->Permissions & (0xFFFFFFC3)) != 0xFFFFFFC0)) {
	eprintf("Some of Permissions are not allowed with R=2.\n");
	return_error(gs_error_rangecheck);
    }
    if (pdev->EncryptionV == 2 && pdev->EncryptionR == 2 && pdev->KeyLength > 40) {
	eprintf("Encryption version 2 revision 2 with KeyLength > 40 appears incompatible to some viewers. With long keys use revision 3.\n");
	return_error(gs_error_rangecheck);
    }
    /* Compute O : */
    md5_init(&md5);
    copy_padded(buf, &pdev->OwnerPassword);
    md5_append(&md5, buf, sizeof(buf));
    md5_finish(&md5, digest);
    if (pdev->EncryptionR == 3)
	Adobe_magic_loop_50(digest, pdev->KeyLength / 8);
    copy_padded(buf, &pdev->UserPassword);
    s_arcfour_set_key(&sarc4, digest, pdev->KeyLength / 8);
    s_arcfour_process_buffer(&sarc4, buf, sizeof(buf));
    if (pdev->EncryptionR == 3)
	Adobe_magic_loop_19(buf, sizeof(buf), digest, pdev->KeyLength / 8);
    memcpy(pdev->EncryptionO, buf, sizeof(pdev->EncryptionO));
    /* Compute Key : */
    md5_init(&md5);
    copy_padded(buf, &pdev->UserPassword);
    md5_append(&md5, buf, sizeof(buf));
    md5_append(&md5, pdev->EncryptionO, sizeof(pdev->EncryptionO));
    t = (byte)(pdev->Permissions >>  0);  md5_append(&md5, &t, 1);
    t = (byte)(pdev->Permissions >>  8);  md5_append(&md5, &t, 1);
    t = (byte)(pdev->Permissions >> 16);  md5_append(&md5, &t, 1);
    t = (byte)(pdev->Permissions >> 24);  md5_append(&md5, &t, 1);
    md5_append(&md5, pdev->fileID, sizeof(pdev->fileID));
    if (pdev->EncryptionR == 3)
	if (!pdev->EncryptMetadata) {
	    const byte v[4] = {0xFF, 0xFF, 0xFF, 0xFF};

	    md5_append(&md5, v, 4);
	}
    md5_finish(&md5, digest);
    if (pdev->EncryptionR == 3)
	Adobe_magic_loop_50(digest, pdev->KeyLength / 8);
    memcpy(pdev->EncryptionKey, digest, pdev->KeyLength / 8);
    /* Compute U : */
    if (pdev->EncryptionR == 3) {
	md5_init(&md5);
	md5_append(&md5, pad, sizeof(pad));
	md5_append(&md5, pdev->fileID, sizeof(pdev->fileID));
	md5_finish(&md5, digest);
	s_arcfour_set_key(&sarc4, pdev->EncryptionKey, pdev->KeyLength / 8);
	s_arcfour_process_buffer(&sarc4, digest, sizeof(digest));
	Adobe_magic_loop_19(digest, sizeof(digest), pdev->EncryptionKey, pdev->KeyLength / 8);
	memcpy(pdev->EncryptionU, digest, sizeof(digest));
	memcpy(pdev->EncryptionU + sizeof(digest), pad, 
		sizeof(pdev->EncryptionU) - sizeof(digest));
    } else {
	memcpy(pdev->EncryptionU, pad, sizeof(pdev->EncryptionU));
	s_arcfour_set_key(&sarc4, pdev->EncryptionKey, pdev->KeyLength / 8);
	s_arcfour_process_buffer(&sarc4, pdev->EncryptionU, sizeof(pdev->EncryptionU));
    }
    return 0;
}

#ifdef __DECC
/* The ansi alias rules are violated in this next routine.  Tell the compiler
   to ignore this.
 */
#pragma optimize save
#pragma optimize ansi_alias=off
#endif
/*
 * Update the color mapping procedures after setting ProcessColorModel.
 *
 * The 'index' value indicates the ProcessColorModel.
 *	0 = DeviceGray
 *	1 = DeviceRGB
 *	2 = DeviceCMYK
 *	3 = DeviceN (treat like CMYK except for color model name)
 */
void
pdf_set_process_color_model(gx_device_pdf * pdev, int index)
{

    const static gx_device_color_info pcm_color_info[] = {
	dci_values(1, 8, 255, 0, 256, 0),		/* Gray */
	dci_values(3, 24, 255, 255, 256, 256),		/* RGB */
	dci_values(4, 32, 255, 255, 256, 256),		/* CMYK */
	dci_values(4, 32, 255, 255, 256, 256)	/* Treat DeviceN like CMYK */
    };

    pdev->pcm_color_info_index = index;
    pdev->color_info = pcm_color_info[index];
    /* Set the separable and linear shift, masks, bits. */
    set_linear_color_bits_mask_shift((gx_device *)pdev);
    pdev->color_info.separable_and_linear = GX_CINFO_SEP_LIN;
    /*
     * The conversion from PS to PDF should be transparent as possible.
     * Particularly it should not change representation of colors.
     * Perhaps due to historical reasons the source color information
     * sometimes isn't accessible from device methods, and
     * therefore they perform a mapping of colors to 
     * an output color model. Here we handle some color models,
     * which were selected almost due to antique reasons.
     */
    switch (index) {
	case 0:		/* DeviceGray */
	    set_dev_proc(pdev, map_rgb_color, gx_default_gray_map_rgb_color);
	    set_dev_proc(pdev, map_color_rgb, gx_default_gray_map_color_rgb);
	    set_dev_proc(pdev, map_cmyk_color, NULL);
	    set_dev_proc(pdev, get_color_mapping_procs,
			gx_default_DevGray_get_color_mapping_procs);
	    set_dev_proc(pdev, get_color_comp_index,
			gx_default_DevGray_get_color_comp_index);
	    set_dev_proc(pdev, encode_color, gx_default_gray_encode);
	    set_dev_proc(pdev, decode_color, gx_default_decode_color);
	    break;
	case 1:		/* DeviceRGB */
	    set_dev_proc(pdev, map_rgb_color, gx_default_rgb_map_rgb_color);
	    set_dev_proc(pdev, map_color_rgb, gx_default_rgb_map_color_rgb);
	    set_dev_proc(pdev, map_cmyk_color, NULL);
	    set_dev_proc(pdev, get_color_mapping_procs,
			gx_default_DevRGB_get_color_mapping_procs);
	    set_dev_proc(pdev, get_color_comp_index,
			gx_default_DevRGB_get_color_comp_index);
	    set_dev_proc(pdev, encode_color, gx_default_rgb_map_rgb_color);
	    set_dev_proc(pdev, decode_color, gx_default_rgb_map_color_rgb);
	    break;
	case 3:		/* DeviceN - treat like DeviceCMYK except for cm_name */
	    pdev->color_info.cm_name = "DeviceN";
	case 2:		/* DeviceCMYK */
	    set_dev_proc(pdev, map_rgb_color, NULL);
	    set_dev_proc(pdev, map_color_rgb, cmyk_8bit_map_color_rgb);
	   /* possible problems with aliassing on next statement */
	    set_dev_proc(pdev, map_cmyk_color, cmyk_8bit_map_cmyk_color);
	    set_dev_proc(pdev, get_color_mapping_procs,
			gx_default_DevCMYK_get_color_mapping_procs);
	    set_dev_proc(pdev, get_color_comp_index,
			gx_default_DevCMYK_get_color_comp_index);
	    set_dev_proc(pdev, encode_color, cmyk_8bit_map_cmyk_color);
	    set_dev_proc(pdev, decode_color, cmyk_8bit_map_color_rgb);
	    break;
	default:	/* can't happen - see the call from gdev_pdf_put_params. */
	    DO_NOTHING;
    }
}
#ifdef __DECC
#pragma optimize restore
#endif

/*
 * Reset the text state parameters to initial values.
 */
void
pdf_reset_text(gx_device_pdf * pdev)
{
    pdf_reset_text_state(pdev->text);
}

/* Open the device. */
private int
pdf_open(gx_device * dev)
{
    gx_device_pdf *const pdev = (gx_device_pdf *) dev;
    gs_memory_t *mem = pdev->pdf_memory = gs_memory_stable(pdev->memory);
    int code;

    if ((code = pdf_open_temp_file(pdev, &pdev->xref)) < 0 ||
	(code = pdf_open_temp_stream(pdev, &pdev->asides)) < 0 ||
	(code = pdf_open_temp_stream(pdev, &pdev->streams)) < 0 ||
	(code = pdf_open_temp_stream(pdev, &pdev->pictures)) < 0
	)
	goto fail;
    code = gdev_vector_open_file((gx_device_vector *) pdev, sbuf_size);
    if (code < 0)
	goto fail;
    gdev_vector_init((gx_device_vector *) pdev);
    pdev->vec_procs = &pdf_vector_procs;
    pdev->fill_options = pdev->stroke_options = gx_path_type_optimize;
    /* Set in_page so the vector routines won't try to call */
    /* any vector implementation procedures. */
    pdev->in_page = true;
    /*
     * pdf_initialize_ids allocates some (global) named objects, so we must
     * initialize the named objects dictionary now.
     */
    pdev->local_named_objects =
	pdev->global_named_objects =
	cos_dict_alloc(pdev, "pdf_open(global_named_objects)");
    /* Initialize internal structures that don't have IDs. */
    pdev->NI_stack = cos_array_alloc(pdev, "pdf_open(NI stack)");
    pdev->Namespace_stack = cos_array_alloc(pdev, "pdf_open(Namespace stack)");
    pdf_initialize_ids(pdev);
    code = pdf_compute_fileID(pdev);
    if (code < 0)
	goto fail;
    if (pdev->OwnerPassword.size > 0) {
	code = pdf_compute_encryption_data(pdev);
	if (code < 0)
	    goto fail;
    } else if(pdev->UserPassword.size > 0) {
	eprintf("User password is specified. Need an Owner password or both.\n");
	return_error(gs_error_rangecheck);
    } else if (pdev->KeyLength) {
	eprintf("Can't accept encryption options without a password.\n");
	return_error(gs_error_rangecheck);
    }
    /* Now create a new dictionary for the local named objects. */
    pdev->local_named_objects =
	cos_dict_alloc(pdev, "pdf_open(local_named_objects)");
    pdev->outlines_id = 0;
    pdev->next_page = 0;
    pdev->text = pdf_text_data_alloc(mem);
    pdev->sbstack_size = count_of(pdev->vgstack); /* Overestimated a few. */
    pdev->sbstack = gs_alloc_struct_array(mem, pdev->sbstack_size, pdf_substream_save,
				 &st_pdf_substream_save_element, "pdf_open");
    pdev->pages =
	gs_alloc_struct_array(mem, initial_num_pages, pdf_page_t,
			      &st_pdf_page_element, "pdf_open(pages)");
    if (pdev->text == 0 || pdev->pages == 0 || pdev->sbstack == 0) {
	code = gs_error_VMerror;
	goto fail;
    }
    memset(pdev->sbstack, 0, pdev->sbstack_size * sizeof(pdf_substream_save));
    memset(pdev->pages, 0, initial_num_pages * sizeof(pdf_page_t));
    pdev->num_pages = initial_num_pages;
    {
	int i, j;

	for (i = 0; i < NUM_RESOURCE_TYPES; ++i)
	    for (j = 0; j < NUM_RESOURCE_CHAINS; ++j)
		pdev->resources[i].chains[j] = 0;
    }
    pdev->outline_levels[0].first.id = 0;
    pdev->outline_levels[0].left = max_int;
    pdev->outline_levels[0].first.action = 0;
    pdev->outline_levels[0].last.action = 0;
    pdev->outline_depth = 0;
    pdev->closed_outline_depth = 0;
    pdev->outlines_open = 0;
    pdev->articles = 0;
    pdev->Dests = 0;
    /* {global,local}_named_objects was initialized above */
    pdev->PageLabels = 0;
    pdev->PageLabels_current_page = 0;
    pdev->PageLabels_current_label = 0;
    pdev->pte = NULL;
    pdf_reset_page(pdev);
    return 0;
  fail:
    return pdf_close_files(pdev, code);
}

/* Detect I/O errors. */
private int
pdf_ferror(gx_device_pdf *pdev)
{
    fflush(pdev->file);
    fflush(pdev->xref.file);
    sflush(pdev->strm);
    sflush(pdev->asides.strm);
    sflush(pdev->streams.strm);
    sflush(pdev->pictures.strm);
    return ferror(pdev->file) || ferror(pdev->xref.file) ||
	ferror(pdev->asides.file) || ferror(pdev->streams.file) ||
	ferror(pdev->pictures.file);
}

/* Compute the dominant text orientation of a page. */
private int
pdf_dominant_rotation(const pdf_text_rotation_t *ptr)
{
    int i, imax = -1;
    long max_count = 0;
    static const int angles[] = { pdf_text_rotation_angle_values };

    for (i = 0; i < countof(ptr->counts); ++i) {
	long count = ptr->counts[i];

	if (count > max_count)
	    imax = i, max_count = count;
    }
    return (imax < 0 ? imax : angles[imax]);
}

/* Print a Rotate command, if requested and possible. */
private void
pdf_print_orientation(gx_device_pdf * pdev, pdf_page_t *page)
{
    stream *s = pdev->strm;
    int dsc_orientation = -1;
    const pdf_page_dsc_info_t *ppdi;

    if (pdev->params.AutoRotatePages == arp_None)
	return; /* Not requested. */

    ppdi = (page != NULL ? &page->dsc_info : &pdev->doc_dsc_info);

    /* Determine DSC orientation : */
    if (ppdi->viewing_orientation >= 0)
	dsc_orientation = ppdi->viewing_orientation;
    else if (ppdi->orientation >= 0)
	dsc_orientation = ppdi->orientation;
    if ((page == NULL && pdev->params.AutoRotatePages == arp_All) || /* document */
        (page != NULL && page->text_rotation.Rotate >= 0) || /* page */
	dsc_orientation >= 0 /* have DSC */) {
        const pdf_text_rotation_t *ptr = 
	    (page != NULL ? &page->text_rotation : &pdev->text_rotation);
	int angle = -1;

#define  Bug687800
#ifndef Bug687800 	/* Bug 687800 together with Bug687489.ps . */
	const gs_point *pbox = &(page != NULL ? page : &pdev->pages[0])->MediaBox;

	if (dsc_orientation >= 0 && pbox->x > pbox->y) {
	    /* The page is in landscape format. Adjust the rotation accordingly. */
	    dsc_orientation ^= 1;
	}
#endif

	/* Combine DSC rotation with text rotation : */
	if (dsc_orientation == 0) {
	    if (ptr->Rotate == 0 || ptr->Rotate == 180)
		angle = ptr->Rotate;
	} else if (dsc_orientation == 1) {
	    if (ptr->Rotate == 90 || ptr->Rotate == 270)
		angle = ptr->Rotate;
	    else 
		angle = 90;
	}

	/* If not combinable, prefer text rotation : */
	if (angle < 0) {
	    if (ptr->Rotate >= 0)
		angle = ptr->Rotate;
#ifdef Bug687800
	    else
		angle = dsc_orientation * 90;
#endif
	}

	/* If got some, write it out : */
	if (angle >= 0)
	    pprintd1(s, "/Rotate %d", angle);
    }
}


/* Close the current page. */
private int
pdf_close_page(gx_device_pdf * pdev)
{
    int page_num = ++(pdev->next_page);
    pdf_page_t *page;
    int code;

    /*
     * If the very first page is blank, we need to open the document
     * before doing anything else.
     */

    code = pdf_open_document(pdev);
    if (code < 0)
	return code;
    if (pdev->ForOPDFRead && pdev->context == PDF_IN_NONE) {
	/* Must create a context stream for empty pages. */
	code = pdf_open_contents(pdev, PDF_IN_STREAM);
	if (code < 0)
	    return code;
    }
    pdf_close_contents(pdev, true);

    /*
     * We can't write the page object or the annotations array yet, because
     * later pdfmarks might add elements to them.  Write the other objects
     * that the page references, and record what we'll need later.
     *
     * Start by making sure the pages array element exists.
     */

    pdf_page_id(pdev, page_num);
    page = &pdev->pages[page_num - 1];
    page->MediaBox.x = pdev->MediaSize[0];
    page->MediaBox.y = pdev->MediaSize[1];
    page->contents_id = pdev->contents_id;
    page->NumCopies_set = pdev->NumCopies_set;
    page->NumCopies = pdev->NumCopies;
    /* pdf_store_page_resources sets procsets, resource_ids[]. */
    code = pdf_store_page_resources(pdev, page);
    if (code < 0)
	return code;

    /* Write the Functions. */

    pdf_write_resource_objects(pdev, resourceFunction);

    /* Save viewer's memory with cleaning resources. */

    if (pdev->MaxViewerMemorySize < 10000000) {
	/* fixme: the condition above and the cleaning algorithm
	   may be improved with counting stored resource size
	   and creating multiple streams per page. */

	if (pdev->ForOPDFRead) {
	    pdf_resource_t *pres = pdf_find_resource_by_resource_id(pdev, resourcePage, pdev->contents_id);
	    
	    if (pres != NULL) {
		code = cos_dict_put_c_strings((cos_dict_t *)pres->object, "/.CleanResources", "/All");
		if (code < 0)
		    return code;
	    }
	}
	code = pdf_close_text_document(pdev);
	if (code < 0)
	    return code;
	code = pdf_write_and_free_all_resource_objects(pdev);
	if (code < 0)
	    return code;
    }

    /* Close use of text on the page. */

    pdf_close_text_page(pdev);

    /* Accumulate text rotation. */

    page->text_rotation.Rotate =
	(pdev->params.AutoRotatePages == arp_PageByPage ?
	 pdf_dominant_rotation(&page->text_rotation) : -1);
    {
	int i;

	for (i = 0; i < countof(page->text_rotation.counts); ++i)
	    pdev->text_rotation.counts[i] += page->text_rotation.counts[i];
    }

    /* Record information from DSC comments. */

    page->dsc_info = pdev->page_dsc_info;
    if (page->dsc_info.orientation < 0)
	page->dsc_info.orientation = pdev->doc_dsc_info.orientation;
    if (page->dsc_info.bounding_box.p.x >= page->dsc_info.bounding_box.q.x ||
	page->dsc_info.bounding_box.p.y >= page->dsc_info.bounding_box.q.y
	)
	page->dsc_info.bounding_box = pdev->doc_dsc_info.bounding_box;

    /* Finish up. */

    pdf_reset_page(pdev);
    return (pdf_ferror(pdev) ? gs_note_error(gs_error_ioerror) : 0);
}

/* Write the page object. */
private double
round_box_coord(floatp xy)
{
    return (int)(xy * 100 + 0.5) / 100.0;
}
private int
pdf_write_page(gx_device_pdf *pdev, int page_num)
{
    long page_id = pdf_page_id(pdev, page_num);
    pdf_page_t *page = &pdev->pages[page_num - 1];
    stream *s;

    pdf_open_obj(pdev, page_id);
    s = pdev->strm;
    pprintg2(s, "<</Type/Page/MediaBox [0 0 %g %g]\n",
	     round_box_coord(page->MediaBox.x),
	     round_box_coord(page->MediaBox.y));
    if (pdev->PDFX) {
	pprintg2(s, "/TrimBox [0 0 %g %g]\n",
		round_box_coord(page->MediaBox.x),
		round_box_coord(page->MediaBox.y));
    }
    pdf_print_orientation(pdev, page);
    pprintld1(s, "/Parent %ld 0 R\n", pdev->Pages->id);
    if (pdev->ForOPDFRead) {
	if (page->NumCopies_set)
	    pprintld1(s, "/NumCopies %ld\n", page->NumCopies);
    }
    if (page->group_id > 0) {
	pprintld1(s, "/Group %ld 0 R\n", page->group_id);
    }
    stream_puts(s, "/Resources<</ProcSet[/PDF");
    if (page->procsets & ImageB)
	stream_puts(s, " /ImageB");
    if (page->procsets & ImageC)
	stream_puts(s, " /ImageC");
    if (page->procsets & ImageI)
	stream_puts(s, " /ImageI");
    if (page->procsets & Text)
	stream_puts(s, " /Text");
    stream_puts(s, "]\n");
    {
	int i;

	for (i = 0; i < countof(page->resource_ids); ++i)
	    if (page->resource_ids[i] && pdf_resource_type_names[i]) {
		stream_puts(s, pdf_resource_type_names[i]);
		pprintld1(s, " %ld 0 R\n", page->resource_ids[i]);
	    }
    }
    stream_puts(s, ">>\n");

    /* Write the annotations array if any. */

    if (page->Annots) {
	stream_puts(s, "/Annots");
	COS_WRITE(page->Annots, pdev);
	COS_FREE(page->Annots, "pdf_write_page(Annots)");
	page->Annots = 0;
    }
    /*
     * The PDF documentation allows, and this code formerly emitted,
     * a Contents entry whose value was an empty array.  Acrobat Reader
     * 3 and 4 accept this, but Acrobat Reader 5.0 rejects it.
     * Fortunately, the Contents entry is optional.
     */
    if (page->contents_id != 0)
	pprintld1(s, "/Contents %ld 0 R\n", page->contents_id);

    /* Write any elements stored by pdfmarks. */

    cos_dict_elements_write(page->Page, pdev);

    stream_puts(s, ">>\n");
    pdf_end_obj(pdev);
    return 0;
}

/* Wrap up ("output") a page. */
private int
pdf_output_page(gx_device * dev, int num_copies, int flush)
{
    gx_device_pdf *const pdev = (gx_device_pdf *) dev;
    int code = pdf_close_page(pdev);

    return (code < 0 ? code :
	    pdf_ferror(pdev) ? gs_note_error(gs_error_ioerror) :
	    gx_finish_output_page(dev, num_copies, flush));
}

/* Close the device. */
private int
pdf_close(gx_device * dev)
{
    gx_device_pdf *const pdev = (gx_device_pdf *) dev;
    gs_memory_t *mem = pdev->pdf_memory;
    stream *s;
    FILE *tfile = pdev->xref.file;
    long xref;
    long resource_pos;
    long Catalog_id = pdev->Catalog->id, Info_id = pdev->Info->id,
	Pages_id = pdev->Pages->id, Encrypt_id = 0;
    long Threads_id = 0;
    bool partial_page = (pdev->contents_id != 0 && pdev->next_page != 0);
    int code = 0, code1;

    /*
     * If this is an EPS file, or if the file didn't end with a showpage for
     * some other reason, or if the file has produced no marks at all, we
     * need to tidy up a little so as not to produce illegal PDF.  However,
     * if there is at least one complete page, we discard any leftover
     * marks.
     */
    if (pdev->next_page == 0) {
	code = pdf_open_page(pdev, PDF_IN_STREAM);

	if (code < 0)
	    return code;
    }
    if (pdev->contents_id != 0)
	pdf_close_page(pdev);

    /* Write the page objects. */

    {
	int i;

	for (i = 1; i <= pdev->next_page; ++i)
	    pdf_write_page(pdev, i);
    }

    if (pdev->PrintStatistics)
	pdf_print_resource_statistics(pdev);

    /* Write the font resources and related resources. */
    code1 = pdf_write_resource_objects(pdev, resourceXObject);
    if (code >= 0)
	code = code1;
    code1 = pdf_free_resource_objects(pdev, resourceXObject);
    if (code >= 0)
	code = code1;
    code1 = pdf_write_resource_objects(pdev, resourceGroup);
    if (code >= 0)
	code = code1;
    code1 = pdf_free_resource_objects(pdev, resourceGroup);
    if (code >= 0)
	code = code1;
    code1 = pdf_write_resource_objects(pdev, resourceSoftMaskDict);
    if (code >= 0)
	code = code1;
    code1 = pdf_free_resource_objects(pdev, resourceSoftMaskDict);
    if (code >= 0)
	code = code1;
    code1 = pdf_close_text_document(pdev);
    if (code >= 0)
	code = code1;
    code1 = pdf_write_resource_objects(pdev, resourceCMap);
    if (code >= 0)
	code = code1;
    code1 = pdf_free_resource_objects(pdev, resourceCMap);
    if (code >= 0)
	code = code1;
    if (pdev->ResourcesBeforeUsage)
	pdf_reverse_resource_chain(pdev, resourcePage);
    code1 = pdf_write_resource_objects(pdev, resourcePage);
    if (code >= 0)
	code = code1;
    code1 = pdf_free_resource_objects(pdev, resourcePage);
    if (code >= 0)
	code = code1;

    code1 = pdf_free_resource_objects(pdev, resourceOther);
    if (code >= 0)
	code = code1;


    /* Create the Pages tree. */

    pdf_open_obj(pdev, Pages_id);
    s = pdev->strm;
    stream_puts(s, "<< /Type /Pages /Kids [\n");
    /* Omit the last page if it was incomplete. */
    if (partial_page)
	--(pdev->next_page);
    {
	int i;

	for (i = 0; i < pdev->next_page; ++i)
	    pprintld1(s, "%ld 0 R\n", pdev->pages[i].Page->id);
    }
    pprintd1(s, "] /Count %d\n", pdev->next_page);
    pdev->text_rotation.Rotate = pdf_dominant_rotation(&pdev->text_rotation);
    pdf_print_orientation(pdev, NULL);
    cos_dict_elements_write(pdev->Pages, pdev);
    stream_puts(s, ">>\n");
    pdf_end_obj(pdev);

    /* Close outlines and articles. */

    if (pdev->outlines_id != 0) {
	/* depth > 0 is only possible for an incomplete outline tree. */
	while (pdev->outline_depth > 0)
	    pdfmark_close_outline(pdev);
	pdfmark_close_outline(pdev);
	pdf_open_obj(pdev, pdev->outlines_id);
	pprintd1(s, "<< /Count %d", pdev->outlines_open);
	pprintld2(s, " /First %ld 0 R /Last %ld 0 R >>\n",
		  pdev->outline_levels[0].first.id,
		  pdev->outline_levels[0].last.id);
	pdf_end_obj(pdev);
    }
    if (pdev->articles != 0) {
	pdf_article_t *part;

	/* Write the remaining information for each article. */
	for (part = pdev->articles; part != 0; part = part->next)
	    pdfmark_write_article(pdev, part);
    }

    /* Write named destinations.  (We can't free them yet.) */

    if (pdev->Dests)
	COS_WRITE_OBJECT(pdev->Dests, pdev);

    /* Write the PageLabel array */
    pdfmark_end_pagelabels(pdev);
    if (pdev->PageLabels) {
	COS_WRITE_OBJECT(pdev->PageLabels, pdev);
    }

    /* Write the Catalog. */

    /*
     * The PDF specification requires Threads to be an indirect object.
     * Write the threads now, if any.
     */
    if (pdev->articles != 0) {
	pdf_article_t *part;

	Threads_id = pdf_begin_obj(pdev);
	s = pdev->strm;
	stream_puts(s, "[ ");
	while ((part = pdev->articles) != 0) {
	    pdev->articles = part->next;
	    pprintld1(s, "%ld 0 R\n", part->contents->id);
	    COS_FREE(part->contents, "pdf_close(article contents)");
	    gs_free_object(mem, part, "pdf_close(article)");
	}
	stream_puts(s, "]\n");
	pdf_end_obj(pdev);
    }
    pdf_open_obj(pdev, Catalog_id);
    s = pdev->strm;
    stream_puts(s, "<<");
    pprintld1(s, "/Type /Catalog /Pages %ld 0 R\n", Pages_id);
    if (pdev->outlines_id != 0)
	pprintld1(s, "/Outlines %ld 0 R\n", pdev->outlines_id);
    if (Threads_id)
	pprintld1(s, "/Threads %ld 0 R\n", Threads_id);
    if (pdev->Dests)
	pprintld1(s, "/Dests %ld 0 R\n", pdev->Dests->id);
    if (pdev->PageLabels)
	pprintld1(s, "/PageLabels << /Nums  %ld 0 R >>\n", 
                  pdev->PageLabels->id);
    cos_dict_elements_write(pdev->Catalog, pdev);
    stream_puts(s, ">>\n");
    pdf_end_obj(pdev);
    if (pdev->Dests) {
	COS_FREE(pdev->Dests, "pdf_close(Dests)");
	pdev->Dests = 0;
    }
    if (pdev->PageLabels) {
	COS_FREE(pdev->PageLabels, "pdf_close(PageLabels)");
	pdev->PageLabels = 0;
        pdev->PageLabels_current_label = 0;
    }

    /* Prevent writing special named objects twice. */

    pdev->Catalog->id = 0;
    /*pdev->Info->id = 0;*/	/* Info should get written */
    pdev->Pages->id = 0;
    {
	int i;

	for (i = 0; i < pdev->num_pages; ++i)
	    if (pdev->pages[i].Page)
		pdev->pages[i].Page->id = 0;
    }

    /*
     * Write the definitions of the named objects.
     * Note that this includes Form XObjects created by BP/EP, named PS
     * XObjects, and images named by NI.
     */

    do {
	cos_dict_objects_write(pdev->local_named_objects, pdev);
    } while (pdf_pop_namespace(pdev) >= 0);
    cos_dict_objects_write(pdev->global_named_objects, pdev);

    /* Copy the resources into the main file. */

    s = pdev->strm;
    resource_pos = stell(s);
    sflush(pdev->asides.strm);
    {
	FILE *rfile = pdev->asides.file;
	long res_end = ftell(rfile);

	fseek(rfile, 0L, SEEK_SET);
	pdf_copy_data(s, rfile, res_end, NULL);
    }

    /* Write Encrypt. */
    if (pdev->OwnerPassword.size > 0) {
	Encrypt_id = pdf_obj_ref(pdev);

	pdf_open_obj(pdev, Encrypt_id);
	s = pdev->strm;
	stream_puts(s, "<<");
	stream_puts(s, "/Filter /Standard ");
	pprintld1(s, "/V %ld ", pdev->EncryptionV);
	pprintld1(s, "/Length %ld ", pdev->KeyLength);
	pprintld1(s, "/R %ld ", pdev->EncryptionR);
	pprintld1(s, "/P %ld ", pdev->Permissions);
	stream_puts(s, "/O ");
	pdf_put_string(pdev, pdev->EncryptionO, sizeof(pdev->EncryptionO));
	stream_puts(s, "\n/U ");
	pdf_put_string(pdev, pdev->EncryptionU, sizeof(pdev->EncryptionU));
	stream_puts(s, ">>\n");
	pdf_end_obj(pdev);
	s = pdev->strm;
    }

    /* Write the cross-reference section. */

    xref = pdf_stell(pdev);
    if (pdev->FirstObjectNumber == 1)
	pprintld1(s, "xref\n0 %ld\n0000000000 65535 f \n",
		  pdev->next_id);
    else
	pprintld2(s, "xref\n0 1\n0000000000 65535 f \n%ld %ld\n",
		  pdev->FirstObjectNumber,
		  pdev->next_id - pdev->FirstObjectNumber);
    fseek(tfile, 0L, SEEK_SET);
    {
	long i;

	for (i = pdev->FirstObjectNumber; i < pdev->next_id; ++i) {
	    ulong pos;
	    char str[21];

	    fread(&pos, sizeof(pos), 1, tfile);
	    if (pos & ASIDES_BASE_POSITION)
		pos += resource_pos - ASIDES_BASE_POSITION;
	    sprintf(str, "%010ld 00000 n \n", pos);
	    stream_puts(s, str);
	}
    }

    /* Write the trailer. */

    stream_puts(s, "trailer\n");
    pprintld3(s, "<< /Size %ld /Root %ld 0 R /Info %ld 0 R\n",
	      pdev->next_id, Catalog_id, Info_id);
    stream_puts(s, "/ID [");
    psdf_write_string(pdev->strm, pdev->fileID, sizeof(pdev->fileID), 0);
    psdf_write_string(pdev->strm, pdev->fileID, sizeof(pdev->fileID), 0);
    stream_puts(s, "]\n");
    if (pdev->OwnerPassword.size > 0) {
	pprintld1(s, "/Encrypt %ld 0 R ", Encrypt_id);
    }
    stream_puts(s, ">>\n");
    pprintld1(s, "startxref\n%ld\n%%%%EOF\n", xref);

    /* Release the resource records. */

    {
	pdf_resource_t *pres;
	pdf_resource_t *prev;

	for (prev = pdev->last_resource; (pres = prev) != 0;) {
	    prev = pres->prev;
	    gs_free_object(mem, pres, "pdf_resource_t");
	}
	pdev->last_resource = 0;
    }

    /* Free named objects. */

    cos_dict_objects_delete(pdev->local_named_objects);
    COS_FREE(pdev->local_named_objects, "pdf_close(local_named_objects)");
    pdev->local_named_objects = 0;
    cos_dict_objects_delete(pdev->global_named_objects);
    COS_FREE(pdev->global_named_objects, "pdf_close(global_named_objects)");
    pdev->global_named_objects = 0;

    /* Wrap up. */

    gs_free_object(mem, pdev->pages, "pages");
    pdev->pages = 0;
    pdev->num_pages = 0;

    if (pdev->ForOPDFRead && pdev->OPDFReadProcsetPath.size) {
        /* pdf_open_dcument could set up filters for entire document.
           Removing them now. */
	int status;
    
	stream_putc(s, 0x04);
	while (s->strm) {
	    s = s->strm;
	}
	status = s_close_filters(&pdev->strm, s);
	if (status < 0 && code == 0)
	    code = gs_error_ioerror;
    }
    code1 = gdev_vector_close_file((gx_device_vector *) pdev);
    if (code >= 0)
	code = code1;
    if (pdev->max_referred_page >= pdev->next_page + 1) {
        /* Note : pdev->max_referred_page counts from 1, 
	   and pdev->next_page counts from 0. */
	eprintf2("ERROR: A pdfmark destination page %d points beyond the last page %d.\n",
		pdev->max_referred_page, pdev->next_page);
#if 0 /* Temporary disabled due to Bug 687686. */
	if (code >= 0)
	    code = gs_note_error(gs_error_rangecheck);
#endif
    }
    return pdf_close_files(pdev, code);
}

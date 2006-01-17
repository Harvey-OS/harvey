/* Copyright (C) 2001 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpdfb.h,v 1.14 2005/10/12 08:16:50 leonardo Exp $ */
/* pdfwrite, ps2write device body template. */

/* This file is allowed to #include several times into a single .c file.
   The following macros to be defined in advance :
   PDF_DEVICE_NAME - a string like "pdfwrite".
   PDF_DEVICE_IDENT - an identifier like gs_pdfwrite_device.
   PDF_DEVICE_MaxInlineImageSize - a value of PDF_DEVICE_MaxInlineImageSize.
   PDF_FOR_OPDFREAD - an integer 0 (false) or 1 (true).
 */


const gx_device_pdf PDF_DEVICE_IDENT =
{std_device_dci_type_body(gx_device_pdf, 0, PDF_DEVICE_NAME,
			  &st_device_pdfwrite,
			  DEFAULT_WIDTH_10THS * X_DPI / 10,
			  DEFAULT_HEIGHT_10THS * Y_DPI / 10,
			  X_DPI, Y_DPI,
			  3, 24, 255, 255, 256, 256),
 {pdf_open,
  gx_upright_get_initial_matrix,
  NULL,				/* sync_output */
  pdf_output_page,
  pdf_close,
  gx_default_rgb_map_rgb_color,
  gx_default_rgb_map_color_rgb,
  gdev_pdf_fill_rectangle,
  NULL,				/* tile_rectangle */
  gdev_pdf_copy_mono,
  gdev_pdf_copy_color,
  NULL,				/* draw_line */
  psdf_get_bits,		/* get_bits */
  gdev_pdf_get_params,
  gdev_pdf_put_params,
  NULL,				/* map_cmyk_color */
  NULL,				/* get_xfont_procs */
  NULL,				/* get_xfont_device */
  NULL,				/* map_rgb_alpha_color */
  gx_page_device_get_page_device,
  NULL,				/* get_alpha_bits */
  NULL,				/* copy_alpha */
  NULL,				/* get_band */
  NULL,				/* copy_rop */
  gdev_pdf_fill_path,
  gdev_pdf_stroke_path,
  gdev_pdf_fill_mask,
  NULL,				/* fill_trapezoid */
  NULL,				/* fill_parallelogram */
  NULL,				/* fill_triangle */
  NULL,				/* draw_thin_line */
  NULL,				/* begin_image */
  NULL,				/* image_data */
  NULL,				/* end_image */
  gdev_pdf_strip_tile_rectangle,
  NULL,				/* strip_copy_rop */
  NULL,				/* get_clipping_box */
  gdev_pdf_begin_typed_image,
  psdf_get_bits_rectangle,	/* get_bits_rectangle */
  NULL,				/* map_color_rgb_alpha */
  gdev_pdf_create_compositor,	/* create_compositor */
  NULL,				/* get_hardware_params */
  gdev_pdf_text_begin,
  NULL,				/* finish_copydevice */
  gdev_pdf_begin_transparency_group,	/* begin_transparency_group */
  gdev_pdf_end_transparency_group,	/* end_transparency_group */
  gdev_pdf_begin_transparency_mask,	/* begin_transparency_mask */
  gdev_pdf_end_transparency_mask,	/* end_transparency_mask */
  gdev_pdf_discard_transparency_layer,	/* discard_transparency_layer */
  NULL,				/* get_color_mapping_procs */
  NULL,				/* get_color_comp_index */
  NULL,				/* encode_color */
  NULL,				/* decode_color */
  gdev_pdf_pattern_manage, 	/* pattern_manage */
  gdev_pdf_fill_rectangle_hl_color, 	/* fill_rectangle_hl_color */
  gdev_pdf_include_color_space 	/* include_color_space */
 },
 psdf_initial_values(PSDF_VERSION_INITIAL, 0 /*false */ ),  /* (!ASCII85EncodePages) */
 PDF_COMPATIBILITY_LEVEL_INITIAL,  /* CompatibilityLevel */
 -1,				/* EndPage */
 1,				/* StartPage */
 1 /*true*/,			/* Optimize */
 0 /*false*/,			/* ParseDSCCommentsForDocInfo */
 1 /*true*/,			/* ParseDSCComments */
 0 /*false*/,			/* EmitDSCWarnings */
 0 /*false*/,			/* CreateJobTicket */
 0 /*false*/,			/* PreserveEPSInfo */
 1 /*true*/,			/* AutoPositionEPSFiles */
 1 /*true*/,			/* PreserveCopyPage */
 0 /*false*/,			/* UsePrologue */
 0,				/* OffOptimizations */
 1 /*true*/,			/* ReAssignCharacters */
 1 /*true*/,			/* ReEncodeCharacters */
 1,				/* FirstObjectNumber */
 1 /*true*/,			/* CompressFonts */
 0 /*false*/,			/* PrintStatistics */
 PDF_FOR_OPDFREAD,		/* ForOPDFRead */
 {0, 0, 0},			/* OPDFReadProcsetPath */
 false,				/* CompressEntireFile */
 0 /*false*/,			/* ResourcesBeforeUsage */
 1 /*true*/,			/* HavePDFWidths */
 0 /*false*/,			/* HaveStrokeColor */
 1 /*true*/,			/* HaveTransparency */
 0 /*false*/,			/* PatternImagemask */
 0 /*false*/,			/* PDFX */
 12000,				/* MaxClipPathSize */ /* HP LaserJet 1320 hangs with 14000. */
 max_long,                      /* MaxViewerMemorySize */
 256000,			/* MaxShadingBitmapSize */
 PDF_DEVICE_MaxInlineImageSize,	/* MaxInlineImageSize */
 {0, 0, 0},			/* OwnerPassword */
 {0, 0, 0},			/* UserPassword */
 0,				/* KeyLength */
 -4,				/* Permissions */
 0,				/* EncryptionR */
 {0},				/* EncryptionO */
 {0},				/* EncryptionU */
 {0},				/* EncryptionKey */
 0,				/* EncryptionV */
 true,				/* EncryptMetadata */
 {0, 0, 0},			/* NoEncrypt */
 0 /*false*/,			/* is_EPS */
 {-1, -1},			/* doc_dsc_info */
 {-1, -1},			/* page_dsc_info */
 0 /*false*/,			/* fill_overprint */
 0 /*false*/,			/* stroke_overprint */
 0,				/* overprint_mode */
 gs_no_id,			/* halftone_id */
 {gs_no_id, gs_no_id, gs_no_id, gs_no_id}, /* transfer_ids */
 0,				/* transfer_not_identity */
 gs_no_id,			/* black_generation_id */
 gs_no_id,			/* undercolor_removal_id */
 pdf_compress_none,		/* compression */
 pdf_compress_none,		/* compression_at_page_start */
 {{0}},				/* xref */
 {{0}},				/* asides */
 {{0}},				/* streams */
 {{0}},				/* pictures */
 0,				/* next_id */
 0,				/* Catalog */
 0,				/* Info */
 0,				/* Pages */
 0,				/* outlines_id */
 0,				/* next_page */
 -1,				/* max_referred_page */
 0,				/* contents_id */
 PDF_IN_NONE,			/* context */
 0,				/* contents_length_id */
 0,				/* contents_pos */
 NoMarks,			/* procsets */
 0,				/* text */
 {{0}},				/* text_rotation */
 0,				/* pages */
 0,				/* num_pages */
 1,				/* used_mask */
 {
     {
	 {0}}},			/* resources */
 {0},				/* cs_Patterns */
 {0},				/* Identity_ToUnicode_CMaps */
 0,				/* last_resource */
 {
     {
	 {0}}},			/* outline_levels */
 0,				/* outline_depth */
 0,				/* closed_outline_depth */
 0,				/* outlines_open */
 0,				/* articles */
 0,				/* Dests */
 {0},				/* fileID */
 0,				/* global_named_objects */
 0,				/* local_named_objects */
 0,				/* NI_stack */
 0,				/* Namespace_stack */
 0,				/* font_cache */
 {0, 0},			/* char_width */
 0,				/* clip_path */
 0,                             /* PageLabels */
 -1,                            /* PageLabels_current_page */
 0,                             /* PageLabels_current_label */
 0,				/* */
 {				/* vgstack[2] */
    {0}, {0}
 },
 0,				/* vgstack_depth */
 0,				/* vgstack_bottom */
 {0},				/* vg_initial */
 false,				/* vg_initial_set */
 0,				/* sbstack_size */
 0,				/* sbstack_depth */
 0,				/* sbstack */
 0,				/* substream_Resources */
 1,				/* pcm_color_info_index == DeviceRGB */
 false,				/* skip_colors */
 false,				/* AR4_save_bug */
 0,				/* font3 */
 0,				/* accumulating_substream_resource */
 {0,0,0,0,0,0,0,0,0},		/* charproc_ctm */
 0,				/* charproc_just_accumulated */
 0,				/* accumulating_a_global_object */
 0,				/* cgp */
 0,				/* substituted_pattern_count */
 0,				/* substituted_pattern_drop_page */
 0,				/* image_mask_id */
 0,				/* image_mask_is_SMask */
 0,				/* image_mask_skip */
 {0,0,0,0,0,0}, 		/* gs_matrix converting_image_matrix */
 0,				/* image_mask_scale */
 NULL,				/* pres_soft_mask_dict */
 {0, 0}				/* objname */
};


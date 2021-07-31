/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gdevpdfg.h,v 1.9 2000/09/19 19:00:17 lpd Exp $ */
/* Internal graphics interfaces for PDF-writing driver. */

#ifndef gdevpdfg_INCLUDED
#  define gdevpdfg_INCLUDED

#include "gscspace.h"		/* for gs_separation_name */

/* ---------------- Exported by gdevpdfc.c ---------------- */

/* Define standard and short color space names. */
/*
 * Based on Adobe's published PDF documentation, it appears that the
 * abbreviations for the calibrated color spaces were introduced in
 * PDF 1.1 (added to Table 7.3) and removed in PDF 1.2 (Table 8.1)!
 */
typedef struct pdf_color_space_names_s {
    const char *DeviceCMYK;
    const char *DeviceGray;
    const char *DeviceRGB;
    const char *Indexed;
} pdf_color_space_names_t;
#define PDF_COLOR_SPACE_NAMES\
  "/DeviceCMYK", "/DeviceGray", "/DeviceRGB", "/Indexed"
#define PDF_COLOR_SPACE_NAMES_SHORT\
  "/CMYK", "/G", "/RGB", "/I"
extern const pdf_color_space_names_t
    pdf_color_space_names,
    pdf_color_space_names_short;

/* Define an abstract color space type. */
#ifndef gs_color_space_DEFINED
#  define gs_color_space_DEFINED
typedef struct gs_color_space_s gs_color_space;
#endif

/*
 * Create a local Device{Gray,RGB,CMYK} color space corresponding to the
 * given number of components.
 */
int pdf_cspace_init_Device(P2(gs_color_space *pcs, int num_components));

/*
 * Create a PDF color space corresponding to a PostScript color space.
 * For parameterless color spaces, set *pvalue to a (literal) string with
 * the color space name; for other color spaces, create a cos_array_t if
 * necessary and set *pvalue to refer to it.  In the latter case, if
 * by_name is true, return a string /Rxxxx rather than a reference to
 * the actual object.
 */
int pdf_color_space(P5(gx_device_pdf *pdev, cos_value_t *pvalue,
		       const gs_color_space *pcs,
		       const pdf_color_space_names_t *pcsn,
		       bool by_name));

/* Create colored and uncolored Pattern color spaces. */
int pdf_cs_Pattern_colored(P2(gx_device_pdf *pdev, cos_value_t *pvalue));
int pdf_cs_Pattern_uncolored(P2(gx_device_pdf *pdev, cos_value_t *pvalue));

/* Set the ProcSets bits corresponding to an image color space. */
void pdf_color_space_procsets(P2(gx_device_pdf *pdev,
				 const gs_color_space *pcs));

/* ---------------- Exported by gdevpdfg.c ---------------- */

/* Reset the graphics state parameters to initial values. */
void pdf_reset_graphics(P1(gx_device_pdf *pdev));

/* Set the fill or stroke color. */
/* pdecolor is &pdev->fill_color or &pdev->stroke_color. */
int pdf_set_pure_color(P4(gx_device_pdf *pdev, gx_color_index color,
			  gx_drawing_color *pdcolor,
			  const psdf_set_color_commands_t *ppscc));
int pdf_set_drawing_color(P4(gx_device_pdf *pdev,
			     const gx_drawing_color *pdc,
			     gx_drawing_color *pdcolor,
			     const psdf_set_color_commands_t *ppscc));

/*
 * Bring the graphics state up to date for a drawing operation.
 * (Text uses either fill or stroke.)
 */
int pdf_prepare_fill(P2(gx_device_pdf *pdev, const gs_imager_state *pis));
int pdf_prepare_stroke(P2(gx_device_pdf *pdev, const gs_imager_state *pis));
int pdf_prepare_image(P2(gx_device_pdf *pdev, const gs_imager_state *pis));
int pdf_prepare_imagemask(P3(gx_device_pdf *pdev, const gs_imager_state *pis,
			     const gx_drawing_color *pdcolor));

/* Get the (string) name of a separation, */
/* returning a newly allocated string with a / prefixed. */
/****** BOGUS for all but standard separations ******/
int pdf_separation_name(P3(gx_device_pdf *pdev, cos_value_t *pvalue,
			   gs_separation_name sname));

/* ---------------- Exported by gdevpdfj.c ---------------- */

/* ------ Image stream dictionaries ------ */

/* Define the long and short versions of the keys in an image dictionary, */
/* and other strings for images. */
typedef struct pdf_image_names_s {
    pdf_color_space_names_t color_spaces;
    pdf_filter_names_t filter_names;
    const char *BitsPerComponent;
    const char *ColorSpace;
    const char *Decode;
    const char *Height;
    const char *ImageMask;
    const char *Interpolate;
    const char *Width;
} pdf_image_names_t;
#define PDF_IMAGE_PARAM_NAMES\
    "/BitsPerComponent", "/ColorSpace", "/Decode",\
    "/Height", "/ImageMask", "/Interpolate", "/Width"
#define PDF_IMAGE_PARAM_NAMES_SHORT\
    "/BPC", "/CS", "/D", "/H", "/IM", "/I", "/W"
extern const pdf_image_names_t pdf_image_names_full, pdf_image_names_short;

/* Store the values of image parameters other than filters. */
/* pdev is used only for updating procsets. */
/* pcsvalue is not used for masks. */
int pdf_put_image_values(P5(cos_dict_t *pcd, gx_device_pdf *pdev,
			    const gs_pixel_image_t *pic,
			    const pdf_image_names_t *pin,
			    const cos_value_t *pcsvalue));

/* Store filters for an image. */
/* Currently this only saves parameters for CCITTFaxDecode. */
int pdf_put_image_filters(P4(cos_dict_t *pcd, gx_device_pdf *pdev,
			     const psdf_binary_writer * pbw,
			     const pdf_image_names_t *pin));

/* ------ Image writing ------ */

/* Define the maximum size of an in-line image. */
#define MAX_INLINE_IMAGE_BYTES 4000

/*
 * Fill in the image parameters for a device space bitmap.
 * PDF images are always specified top-to-bottom.
 * data_h is the actual number of data rows, which may be less than h.
 */
void pdf_make_bitmap_matrix(P6(gs_matrix * pmat, int x, int y, int w, int h,
			       int h_actual));

/* Put out the gsave and matrix for an image. */
void pdf_put_image_matrix(P3(gx_device_pdf * pdev, const gs_matrix * pmat,
			     floatp y_scale));

/* Put out a reference to an image resource. */
int pdf_do_image(P4(gx_device_pdf * pdev, const pdf_resource_t * pres,
		    const gs_matrix * pimat, bool in_contents));

/* Define the structure for writing an image. */
typedef struct pdf_image_writer_s {
    psdf_binary_writer binary;
    const pdf_image_names_t *pin;
    pdf_resource_t *pres;	/* XObject resource iff not in-line */
    int height;			/* initially specified image height */
    cos_stream_t *data;
    const char *end_string;	/* string to write after EI if in-line */
} pdf_image_writer;
extern_st(st_pdf_image_writer);	/* public for gdevpdfi.c */
#define public_st_pdf_image_writer()\
  gs_public_st_suffix_add2(st_pdf_image_writer, pdf_image_writer,\
    "pdf_image_writer", pdf_image_writer_enum_ptrs,\
    pdf_image_writer_reloc_ptrs, st_psdf_binary_writer, pres, data)
#define pdf_image_writer_max_ptrs (psdf_binary_writer_max_ptrs + 2)

/*
 * Begin writing an image, creating the resource if not in-line and
 * pres == 0, and setting up the binary writer.
 */
int pdf_begin_write_image(P7(gx_device_pdf * pdev, pdf_image_writer * piw,
			     gx_bitmap_id id, int w, int h,
			     pdf_resource_t *pres, bool in_line));

/* Begin writing the image data, setting up the dictionary and filters. */
int pdf_begin_image_data(P4(gx_device_pdf * pdev, pdf_image_writer * piw,
			    const gs_pixel_image_t * pim,
			    const cos_value_t *pcsvalue));

/* Copy the data for a mask or monobit bitmap. */
int pdf_copy_mask_bits(P7(stream *s, const byte *base, int sourcex,
			  int raster, int w, int h, byte invert));

/* Copy the data for a colored image (device pixels). */
int pdf_copy_color_bits(P7(stream *s, const byte *base, int sourcex,
			   int raster, int w, int h, int bytes_per_pixel));

/* Finish writing the binary image data. */
int pdf_end_image_binary(P3(gx_device_pdf *pdev, pdf_image_writer *piw,
			    int data_h));

/*
 * Finish writing an image.  If in-line, write the BI/dict/ID/data/EI and
 * return 1; if a resource, write the resource definition and return 0.
 */
int pdf_end_write_image(P2(gx_device_pdf * pdev, pdf_image_writer * piw));

/* ---------------- Exported by gdevpdfv.c ---------------- */

/* Write a color value. */
int pdf_put_drawing_color(P3(gx_device_pdf *pdev, const gx_drawing_color *pdc,
			     const psdf_set_color_commands_t *ppscc));

#endif /* gdevpdfg_INCLUDED */

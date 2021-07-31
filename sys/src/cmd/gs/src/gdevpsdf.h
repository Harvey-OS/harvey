/* Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gdevpsdf.h,v 1.8.2.1 2000/11/09 23:24:18 rayjj Exp $ */
/* Common output syntax and parameters for PostScript and PDF writers */

#ifndef gdevpsdf_INCLUDED
#  define gdevpsdf_INCLUDED

/*
 * This file should really be named gdevpsd.h or gdevpsdx.h, but we're
 * keeping its current name for backward compatibility.
 */

#include "gdevvec.h"
#include "gsparam.h"
#include "strimpl.h"
#include "sa85x.h"
#include "scfx.h"
#include "spsdf.h"

extern const stream_template s_DCTE_template; /* don't want all of sdct.h */

/*
 * NOTE: the default filter for color and gray images should be DCTEncode.
 * We don't currently set this, because the management of the parameters
 * for this filter simply doesn't work.
 */

/* ---------------- Distiller parameters ---------------- */

/* Parameters for controlling distillation of images. */
typedef struct psdf_image_params_s {
    gs_c_param_list *ACSDict;	/* JPEG */
    bool AntiAlias;
    bool AutoFilter;
    int Depth;
    gs_c_param_list *Dict;	/* JPEG or CCITTFax */
    bool Downsample;
    float DownsampleThreshold;
    enum psdf_downsample_type {
	ds_Average,
	ds_Bicubic,
	ds_Subsample
    } DownsampleType;
#define psdf_ds_names\
	"Average", "Bicubic", "Subsample"
    bool Encode;
    const char *Filter;
    int Resolution;
    const stream_template *filter_template;
} psdf_image_params;

#define psdf_image_param_defaults(af, res, dst, f, ft)\
  NULL/*ACSDict*/, 0/*false*/, af, -1, NULL/*Dict*/, 1/*true*/,\
  dst, ds_Subsample, 1/*true*/, f, res, ft

/* Declare templates for default image compression filters. */
extern const stream_template s_CFE_template;

/* Complete distiller parameters. */
typedef struct psdf_distiller_params_s {

    /* General parameters */

    bool ASCII85EncodePages;
    enum psdf_auto_rotate_pages {
	arp_None,
	arp_All,
	arp_PageByPage
    } AutoRotatePages;
#define psdf_arp_names\
	"None", "All", "PageByPage"
    enum psdf_binding {
	binding_Left,
	binding_Right
    } Binding;
#define psdf_binding_names\
	"Left", "Right"
    bool CompressPages;
    enum psdf_default_rendering_intent {
	ri_Default,
	ri_Perceptual,
	ri_Saturation,
	ri_RelativeColorimetric,
	ri_AbsoluteColorimetric
    } DefaultRenderingIntent;
#define psdf_ri_names\
	"Default", "Perceptual", "Saturation", "RelativeColorimetric",\
	"AbsoluteColorimetric"
    bool DetectBlends;
    bool DoThumbnails;
    long ImageMemory;
    bool LockDistillerParams;
    bool LZWEncodePages;
    int OPM;
    bool PreserveOPIComments;
    bool UseFlateCompression;

    /* Color processing parameters */

    gs_const_string CalCMYKProfile;
    gs_const_string CalGrayProfile;
    gs_const_string CalRGBProfile;
    gs_const_string sRGBProfile;
    enum psdf_color_conversion_strategy {
	ccs_LeaveColorUnchanged,
	ccs_UseDeviceDependentColor, /* not in Acrobat Distiller 4.0 */
	ccs_UseDeviceIndependentColor,
	ccs_UseDeviceIndependentColorForImages,
	ccs_sRGB
    } ColorConversionStrategy;
#define psdf_ccs_names\
	"LeaveColorUnchanged", "UseDeviceDependentColor",\
	"UseDeviceIndependentColor", "UseDeviceIndependentColorForImages",\
	"sRGB"
    bool PreserveHalftoneInfo;
    bool PreserveOverprintSettings;
    enum psdf_transfer_function_info {
	tfi_Preserve,
	tfi_Apply,
	tfi_Remove
    } TransferFunctionInfo;
#define psdf_tfi_names\
	"Preserve", "Apply", "Remove"
    enum psdf_ucr_and_bg_info {
	ucrbg_Preserve,
	ucrbg_Remove
    } UCRandBGInfo;
#define psdf_ucrbg_names\
	"Preserve", "Remove"

#define psdf_general_param_defaults(ascii)\
  ascii, arp_None, binding_Left, 1/*true*/,\
  ri_Default, 1 /*true*/, 0 /*false*/,\
  500000, 0 /*false*/, 0/*false*/, 1,\
  0 /*false*/, 1 /*true*/,\
	/* Color processing parameters */\
  {0}, {0}, {0}, {0},\
  ccs_LeaveColorUnchanged, 0/*false*/, 0/*false*/, tfi_Preserve, ucrbg_Remove

    /* Color sampled image parameters */

    psdf_image_params ColorImage;
    bool ConvertCMYKImagesToRGB;
    bool ConvertImagesToIndexed;

#define psdf_color_image_param_defaults\
  { psdf_image_param_defaults(1/*true*/, 72, 1.5, 0/*"DCTEncode"*/, 0/*&s_DCTE_template*/) },\
  0/*false*/, 1/*true*/

    /* Grayscale sampled image parameters */

    psdf_image_params GrayImage;

#define psdf_gray_image_param_defaults\
  { psdf_image_param_defaults(1/*true*/, 72, 1.5, 0/*"DCTEncode"*/, 0/*&s_DCTE_template*/) }

    /* Monochrome sampled image parameters */

    psdf_image_params MonoImage;

#define psdf_mono_image_param_defaults\
  { psdf_image_param_defaults(0/*false*/, 300, 2.0, "CCITTFaxEncode", &s_CFE_template) }

    /* Font embedding parameters */

    gs_param_string_array AlwaysEmbed;
    gs_param_string_array NeverEmbed;
    enum psdf_cannot_embed_font_policy {
	cefp_OK,
	cefp_Warning,
	cefp_Error
    } CannotEmbedFontPolicy;
#define psdf_cefp_names\
	"OK", "Warning", "Error"
    bool EmbedAllFonts;
    int MaxSubsetPct;
    bool SubsetFonts;

#define psdf_font_param_defaults\
  {0}, {0}, cefp_Warning, 1/*true*/, 100, 1/*true*/

} psdf_distiller_params;

/* Define PostScript/PDF versions, corresponding roughly to Adobe versions. */
typedef enum {
    psdf_version_level1 = 1000,	/* Red Book Level 1 */
    psdf_version_level1_color = 1100,	/* Level 1 + colorimage + CMYK color */
    psdf_version_level2 = 2000,	/* Red Book Level 2 */
    psdf_version_level2_plus = 2017,	/* Adobe release 2017 */
    psdf_version_ll3 = 3010	/* LanguageLevel 3, release 3010 */
} psdf_version;

/* Define the extended device structure. */
#define gx_device_psdf_common\
	gx_device_vector_common;\
	psdf_version version;\
	bool binary_ok;		/* derived from ASCII85EncodePages */\
	psdf_distiller_params params
typedef struct gx_device_psdf_s {
    gx_device_psdf_common;
} gx_device_psdf;

#define psdf_initial_values(version, ascii)\
	vector_initial_values,\
	version,\
	!(ascii),\
	 { psdf_general_param_defaults(ascii),\
	   psdf_color_image_param_defaults,\
	   psdf_gray_image_param_defaults,\
	   psdf_mono_image_param_defaults,\
	   psdf_font_param_defaults\
	 }

/* st_device_psdf is never instantiated per se, but we still need to */
/* extern its descriptor for the sake of subclasses. */
extern_st(st_device_psdf);
#define public_st_device_psdf()	/* in gdevpsdu.c */\
  BASIC_PTRS(device_psdf_ptrs) {\
    GC_OBJ_ELT2(gx_device_psdf, params.ColorImage.ACSDict,\
		params.ColorImage.Dict),\
    GC_CONST_STRING_ELT(gx_device_psdf, params.CalCMYKProfile),\
    GC_CONST_STRING_ELT(gx_device_psdf, params.CalGrayProfile),\
    GC_CONST_STRING_ELT(gx_device_psdf, params.CalRGBProfile),\
    GC_CONST_STRING_ELT(gx_device_psdf, params.sRGBProfile),\
    GC_OBJ_ELT2(gx_device_psdf, params.GrayImage.ACSDict,\
		params.GrayImage.Dict),\
    GC_OBJ_ELT2(gx_device_psdf, params.MonoImage.ACSDict,\
		params.MonoImage.Dict),\
    GC_OBJ_ELT2(gx_device_psdf, params.AlwaysEmbed.data,\
		params.NeverEmbed.data)\
  };\
  gs_public_st_basic_super_final(st_device_psdf, gx_device_psdf,\
    "gx_device_psdf", device_psdf_ptrs, device_psdf_data,\
    &st_device_vector, 0, gx_device_finalize)
#define st_device_psdf_max_ptrs (st_device_vector_max_ptrs + 12)

/* Get/put parameters. */
dev_proc_get_params(gdev_psdf_get_params);
dev_proc_put_params(gdev_psdf_put_params);

/* ---------------- Vector implementation procedures ---------------- */

	/* Imager state */
int psdf_setlinewidth(P2(gx_device_vector * vdev, floatp width));
int psdf_setlinecap(P2(gx_device_vector * vdev, gs_line_cap cap));
int psdf_setlinejoin(P2(gx_device_vector * vdev, gs_line_join join));
int psdf_setmiterlimit(P2(gx_device_vector * vdev, floatp limit));
int psdf_setdash(P4(gx_device_vector * vdev, const float *pattern,
		    uint count, floatp offset));
int psdf_setflat(P2(gx_device_vector * vdev, floatp flatness));
int psdf_setlogop(P3(gx_device_vector * vdev, gs_logical_operation_t lop,
		     gs_logical_operation_t diff));

	/* Other state */
int psdf_setfillcolor(P2(gx_device_vector * vdev, const gx_drawing_color * pdc));
int psdf_setstrokecolor(P2(gx_device_vector * vdev, const gx_drawing_color * pdc));

	/* Paths */
#define psdf_dopath gdev_vector_dopath
int psdf_dorect(P6(gx_device_vector * vdev, fixed x0, fixed y0, fixed x1,
		   fixed y1, gx_path_type_t type));
int psdf_beginpath(P2(gx_device_vector * vdev, gx_path_type_t type));
int psdf_moveto(P6(gx_device_vector * vdev, floatp x0, floatp y0,
		   floatp x, floatp y, gx_path_type_t type));
int psdf_lineto(P6(gx_device_vector * vdev, floatp x0, floatp y0,
		   floatp x, floatp y, gx_path_type_t type));
int psdf_curveto(P10(gx_device_vector * vdev, floatp x0, floatp y0,
		     floatp x1, floatp y1, floatp x2,
		     floatp y2, floatp x3, floatp y3, gx_path_type_t type));
int psdf_closepath(P6(gx_device_vector * vdev, floatp x0, floatp y0,
		      floatp x_start, floatp y_start, gx_path_type_t type));

/* ---------------- Binary (image) data procedures ---------------- */

/* Define the structure for writing binary data. */
typedef struct psdf_binary_writer_s {
    gs_memory_t *memory;
    stream *A85E;		/* optional ASCII85Encode stream */
    stream *target;		/* underlying stream */
    stream *strm;		/* may point to A85E.s */
    gx_device_psdf *dev;	/* may be unused */
} psdf_binary_writer;
extern_st(st_psdf_binary_writer);
#define public_st_psdf_binary_writer() /* in gdevpsdu.c */\
  gs_public_st_ptrs4(st_psdf_binary_writer, psdf_binary_writer,\
    "psdf_binary_writer", psdf_binary_writer_enum_ptrs,\
    psdf_binary_writer_reloc_ptrs, A85E, target, strm, dev)
#define psdf_binary_writer_max_ptrs 4

/* Begin writing binary data. */
int psdf_begin_binary(P2(gx_device_psdf * pdev, psdf_binary_writer * pbw));

/* Add an encoding filter.  The client must have allocated the stream state, */
/* if any, using pdev->v_memory. */
int psdf_encode_binary(P3(psdf_binary_writer * pbw,
		      const stream_template * template, stream_state * ss));

/* Add a 2-D CCITTFax encoding filter. */
/* Set EndOfBlock iff the stream is not ASCII85 encoded. */
int psdf_CFE_binary(P4(psdf_binary_writer * pbw, int w, int h, bool invert));

/*
 * Acquire parameters, and optionally set up the filter for, a DCTEncode
 * filter.  This is a separate procedure so it can be used to validate
 * filter parameters when they are set, rather than waiting until they are
 * used.  pbw = NULL means just set up the stream state.
 */
int psdf_DCT_filter(P6(gs_param_list *plist /* may be NULL */,
		       stream_state /*stream_DCTE_state*/ *st,
		       int Columns, int Rows, int Colors,
		       psdf_binary_writer *pbw /* may be NULL */));

/* Set up compression and downsampling filters for an image. */
/* Note that this may modify the image parameters. */
/* If pctm is NULL, downsampling is not used. */
/* pis only provides UCR and BG information for CMYK => RGB conversion. */
int psdf_setup_image_filters(P5(gx_device_psdf *pdev, psdf_binary_writer *pbw,
				gs_pixel_image_t *pim, const gs_matrix *pctm,
				const gs_imager_state * pis));

/* Set up compression filters for a lossless image, with no downsampling, */
/* no color space conversion, and only lossless filters. */
/* Note that this may modify the image parameters. */
int psdf_setup_lossless_filters(P3(gx_device_psdf *pdev,
				   psdf_binary_writer *pbw,
				   gs_pixel_image_t *pim));

/* Finish writing binary data. */
int psdf_end_binary(P1(psdf_binary_writer * pbw));

/* ---------------- Symbolic data printing ---------------- */

/* Backward compatibility definitions. */
#define psdf_write_string(s, str, size, print_ok)\
  s_write_ps_string(s, str, size, print_ok)
#define psdf_alloc_position_stream(ps, mem)\
  s_alloc_position_stream(ps, mem)
#define psdf_alloc_param_printer(pplist, ppp, s, mem)\
  s_alloc_param_printer(pplist, ppp, s, mem)
#define psdf_free_param_printer(plist)\
  s_free_param_printer(plist)

/* ---------------- Other procedures ---------------- */

/* Define the commands for setting the fill or stroke color. */
typedef struct psdf_set_color_commands_s {
    const char *setgray;
    const char *setrgbcolor;
    const char *setcmykcolor;
    const char *setcolorspace;
    const char *setcolor;
    const char *setcolorn;
} psdf_set_color_commands_t;
/* Define the standard color-setting commands (with PDF names). */
extern const psdf_set_color_commands_t
    psdf_set_fill_color_commands, psdf_set_stroke_color_commands;

/*
 * Adjust a gx_color_index to compensate for the fact that the bit pattern
 * of gx_color_index isn't representable.
 */
gx_color_index psdf_adjust_color_index(P2(gx_device_vector *vdev,
					  gx_color_index color));

/* Set the fill or stroke color. */
int psdf_set_color(P3(gx_device_vector *vdev, const gx_drawing_color *pdc,
		      const psdf_set_color_commands_t *ppscc));

#endif /* gdevpsdf_INCLUDED */

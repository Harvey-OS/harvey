/* Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevpsdf.h,v 1.3 2000/03/16 01:21:24 lpd Exp $ */
/* Common output syntax and parameters for PostScript and PDF writers */

#ifndef gdevpsdf_INCLUDED
#  define gdevpsdf_INCLUDED

#include "gdevvec.h"
#include "gsparam.h"
#include "strimpl.h"
#include "sa85x.h"
#include "scfx.h"
#include "spsdf.h"

/*#define POST60*/

extern const stream_template s_DCTE_template; /* don't want all of sdct.h */

/*
 * NOTE: the default filter for color and gray images should be DCTEncode.
 * We don't currently set this, because the management of the parameters
 * for this filter simply doesn't work.
 */

/* ---------------- Distiller parameters ---------------- */

#ifdef POST60
#define POST60_VALUE(v) v,
#else
#define POST60_VALUE(v) /* */
#endif

/* Parameters for controlling distillation of images. */
typedef struct psdf_image_params_s {
    stream_state *ACSDict;	/* JPEG */
    bool AntiAlias;
    bool AutoFilter;
    int Depth;
    stream_state *Dict;		/* JPEG or CCITTFax */
    bool Downsample;
#ifdef POST60
    float DownsampleThreshold;
#endif
    enum psdf_downsample_type {
	ds_Average,
	ds_Bicubic,
	ds_Subsample
    } DownsampleType;
    bool Encode;
    const char *Filter;
    int Resolution;
    const stream_template *filter_template;
} psdf_image_params;

#define psdf_image_param_defaults(af, res, dst, f, ft)\
  NULL/*ACSDict*/, 0/*false*/, af, -1, NULL/*Dict*/, 0/*false*/,\
  POST60_VALUE(dst) ds_Subsample, 1/*true*/, f, res, ft

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
#ifdef POST60
    enum psdf_binding {
	binding_Left,
	binding_Right
    } Binding;
#endif
    bool CompressPages;
#ifdef POST60
    enum psdf_default_rendering_intent {
	ri_Default,
	ri_Perceptual,
	ri_Saturation,
	ri_RelativeColorimetric,
	ri_AbsoluteColorimetric
    } DefaultRenderingIntent;
    bool DetectBlends;
    bool DoThumbnails;
    int EndPage;
#endif
    long ImageMemory;
#ifdef POST60
    bool LockDistillerParams;
#endif
    bool LZWEncodePages;
#ifdef POST60
    int OPM;
#endif
    bool PreserveHalftoneInfo;
    bool PreserveOPIComments;
    bool PreserveOverprintSettings;
#ifdef POST60
    int StartPage;
#endif
    enum psdf_transfer_function_info {
	tfi_Preserve,
	tfi_Apply,
	tfi_Remove
    } TransferFunctionInfo;
    enum psdf_ucr_and_bg_info {
	ucrbg_Preserve,
	ucrbg_Remove
    } UCRandBGInfo;
    bool UseFlateCompression;
#define psdf_general_param_defaults(ascii)\
  ascii, arp_None, POST60_VALUE(binding_Left) 1/*true*/,\
  POST60_VALUE(ri_Default) POST60_VALUE(0 /*false*/)\
  POST60_VALUE(0 /*false*/) POST60_VALUE(-1)\
  250000, POST60_VALUE(0 /*false*/) 0/*false*/, POST60_VALUE(0)\
  0/*false*/, 0/*false*/, 0/*false*/, POST60_VALUE(-1)\
  tfi_Apply, ucrbg_Remove, 1 /*true*/

    /* Color sampled image parameters */

    psdf_image_params ColorImage;
#ifdef POST60
    /* We're guessing that the xxxProfile parameters are ICC profiles. */
    gs_const_string CalCMYKProfile;
    gs_const_string CalGrayProfile;
    gs_const_string CalRGBProfile;
    gs_const_string sRGBProfile;
#endif
    enum psdf_color_conversion_strategy {
	ccs_LeaveColorUnchanged,
#ifdef POST60
	ccs_UseDeviceIndependentColor,
	ccs_UseDeviceIndependentColorForImages,
	ccs_sRGB
#else
	ccs_UseDeviceDependentColor,
	ccs_UseDeviceIndependentColor
#endif
    } ColorConversionStrategy;
    bool ConvertCMYKImagesToRGB;
    bool ConvertImagesToIndexed;
#define psdf_color_image_param_defaults\
  { psdf_image_param_defaults(1/*true*/, 72, 1.5, 0/*"DCTEncode"*/, 0/*&s_DCTE_template*/) },\
  POST60_VALUE({0}) POST60_VALUE({0}) POST60_VALUE({0}) POST60_VALUE({0})\
  ccs_LeaveColorUnchanged, 1/*true*/, 0/*false */

    /* Grayscale sampled image parameters */

    psdf_image_params GrayImage;
#define psdf_gray_image_param_defaults\
  { psdf_image_param_defaults(1/*true*/, 72, 2.0, 0/*"DCTEncode"*/, 0/*&s_DCTE_template*/) }

    /* Monochrome sampled image parameters */

    psdf_image_params MonoImage;
#define psdf_mono_image_param_defaults\
  { psdf_image_param_defaults(0/*false*/, 300, 2.0, "CCITTFaxEncode", &s_CFE_template) }

    /* Font embedding parameters */

    gs_param_string_array AlwaysEmbed;
    gs_param_string_array NeverEmbed;
#ifdef POST60
    enum psdf_cannot_embed_font_policy {
	cefp_OK,
	cefp_Warning,
	cefp_Error
    } CannotEmbedFontPolicy;
#endif
    bool EmbedAllFonts;
    int MaxSubsetPct;
    bool SubsetFonts;
#define psdf_font_param_defaults\
  {0}, {0}, POST60_VALUE(cefp_OK) 1/*true*/, 35, 1/*true*/

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
#define public_st_device_psdf()	/* in gdevpsdf.c */\
  BASIC_PTRS(device_psdf_ptrs) {\
    GC_OBJ_ELT2(gx_device_psdf, params.ColorImage.ACSDict,\
		params.ColorImage.Dict),\
    POST60_VALUE(GC_CONST_STRING_ELT(gx_device_psdf, CalCMYKProfile))\
    POST60_VALUE(GC_CONST_STRING_ELT(gx_device_psdf, CalGrayProfile))\
    POST60_VALUE(GC_CONST_STRING_ELT(gx_device_psdf, CalRGBProfile))\
    POST60_VALUE(GC_CONST_STRING_ELT(gx_device_psdf, sRGBProfile))\
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
#define public_st_psdf_binary_writer() /* in gdevpsdf.c */\
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

/* Set up compression and downsampling filters for an image. */
/* Note that this may modify the image parameters. */
/* If pctm is NULL, downsampling is not used. */
/* pis only provides UCR and BG information for CMYK => RGB conversion. */
int psdf_setup_image_filters(P5(gx_device_psdf *pdev, psdf_binary_writer *pbw,
				gs_image_t *pim, const gs_matrix *pctm,
				const gs_imager_state * pis));

/* Finish writing binary data. */
int psdf_end_binary(P1(psdf_binary_writer * pbw));

/* ---------------- Embedded font writing ---------------- */

typedef struct psdf_glyph_enum_s {
    gs_font *font;
    gs_glyph *subset_glyphs;
    uint subset_size;
    gs_glyph_space_t glyph_space;
    ulong index;
} psdf_glyph_enum_t;

/*
 * Begin enumerating the glyphs in a font or a font subset.  If subset_size
 * > 0 but subset_glyphs == 0, enumerate all glyphs in [0 .. subset_size-1]
 * (as integer glyphs, i.e., offset by gs_min_cid_glyph).
 */
void psdf_enumerate_glyphs_begin(P5(psdf_glyph_enum_t *ppge, gs_font *font,
				    gs_glyph *subset_glyphs,
				    uint subset_size,
				    gs_glyph_space_t glyph_space));

/*
 * Reset a glyph enumeration.
 */
void psdf_enumerate_glyphs_reset(P1(psdf_glyph_enum_t *ppge));

/*
 * Enumerate the next glyph in a font or a font subset.
 * Return 0 if more glyphs, 1 if done, <0 if error.
 */
int psdf_enumerate_glyphs_next(P2(psdf_glyph_enum_t *ppge, gs_glyph *pglyph));

/*
 * Get the set of referenced glyphs (indices) for writing a subset font.
 * Does not sort or remove duplicates.
 */
int psdf_subset_glyphs(P3(gs_glyph glyphs[256], gs_font *font,
			  const byte used[32]));

/*
 * Add composite glyph pieces to a list of glyphs.  Does not sort or
 * remove duplicates.  max_pieces is the maximum number of pieces that a
 * single glyph can have: if this value is not known, the caller should
 * use max_count.
 */
int psdf_add_subset_pieces(P5(gs_glyph *glyphs, uint *pcount, uint max_count,
			      uint max_pieces, gs_font *font));

/*
 * Sort a list of glyphs and remove duplicates.  Return the number of glyphs
 * in the result.
 */
int psdf_sort_glyphs(P2(gs_glyph *glyphs, int count));

/*
 * Determine whether a sorted list of glyphs includes a given glyph.
 */
bool psdf_sorted_glyphs_include(P3(const gs_glyph *glyphs, int count,
				   gs_glyph glyph));

/* ------ Exported by gdevpsd1.c ------ */

/*
 * Write out a Type 1 font definition.  This procedure does not allocate
 * or free any data.
 */
#ifndef gs_font_type1_DEFINED
#  define gs_font_type1_DEFINED
typedef struct gs_font_type1_s gs_font_type1;
#endif
#define WRITE_TYPE1_EEXEC 1
#define WRITE_TYPE1_ASCIIHEX 2  /* use ASCII hex rather than binary */
#define WRITE_TYPE1_EEXEC_PAD 4  /* add 512 0s */
#define WRITE_TYPE1_EEXEC_MARK 8  /* assume 512 0s will be added */
#define WRITE_TYPE1_POSTSCRIPT 16  /* don't observe ATM restrictions */
int psdf_write_type1_font(P7(stream *s, gs_font_type1 *pfont, int options,
			     gs_glyph *subset_glyphs, uint subset_size,
			     const gs_const_string *alt_font_name,
			     int lengths[3]));

/* ------ Exported by gdevpsdt.c ------ */

/*
 * Write out a TrueType (Type 42) font definition.  This procedure does
 * not allocate or free any data.
 */
#ifndef gs_font_type42_DEFINED
#  define gs_font_type42_DEFINED
typedef struct gs_font_type42_s gs_font_type42;
#endif
#define WRITE_TRUETYPE_CMAP 1	/* generate cmap from the Encoding */
#define WRITE_TRUETYPE_NAME 2	/* generate name if missing */
#define WRITE_TRUETYPE_POST 4	/* generate post if missing */
#define WRITE_TRUETYPE_NO_TRIMMED_TABLE 8  /* not OK to use cmap format 6 */
int psdf_write_truetype_font(P6(stream *s, gs_font_type42 *pfont, int options,
				gs_glyph *subset_glyphs, uint subset_size,
				const gs_const_string *alt_font_name));

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

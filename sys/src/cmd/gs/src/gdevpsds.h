/* Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpsds.h,v 1.12 2005/02/26 18:07:43 igor Exp $ */
/* Image processing stream interface for PostScript and PDF writers */

#ifndef gdevpsds_INCLUDED
#  define gdevpsds_INCLUDED

#include "strimpl.h"
#include "gsiparam.h"

/* ---------------- Depth conversion ---------------- */

/* Convert between 1/2/4/12 bits and 8 bits. */
typedef struct stream_1248_state_s {
    stream_state_common;
    /* The following are set at initialization time. */
    uint samples_per_row;	/* >0 */
    int bits_per_sample;	/* 1, 2, 4, 12 */
    /* The following are updated dynamically. */
    uint left;			/* # of samples left in current row */
} stream_1248_state;

/* Convert N (1, 2, 4, 12) bits to 8. */
extern const stream_template s_1_8_template;
extern const stream_template s_2_8_template;
extern const stream_template s_4_8_template;
extern const stream_template s_12_8_template;

/* Reduce 8 bits to N (1, 2, 4). */
/* We do not currently support converting 8 bits to 12. */
extern const stream_template s_8_1_template;
extern const stream_template s_8_2_template;
extern const stream_template s_8_4_template;

/* Initialize an expansion or reduction stream. */
int s_1248_init(stream_1248_state *ss, int Columns, int samples_per_pixel);

/* ---------------- Color space conversion ---------------- */

/* Convert (8-bit) CMYK to RGB. */
typedef struct stream_C2R_state_s {
    stream_state_common;
    /* The following are set at initialization time. */
    const gs_imager_state *pis;	/* for UCR & BG */
} stream_C2R_state;

#define private_st_C2R_state()	/* in gdevpsds.c */\
  gs_private_st_ptrs1(st_C2R_state, stream_C2R_state, "stream_C2R_state",\
    c2r_enum_ptrs, c2r_reloc_ptrs, pis)
extern const stream_template s_C2R_template;

/* Initialize a CMYK => RGB conversion stream. */
int s_C2R_init(stream_C2R_state *ss, const gs_imager_state *pis);

/* Convert an image to indexed form (IndexedEncode filter). */
typedef struct stream_IE_state_s {
    stream_state_common;
    /* The client sets the following before initializing the stream. */
    int BitsPerComponent;	/* 1, 2, 4, 8 */
    int NumComponents;
    int Width;			/* pixels per scan line, > 0 */
    int BitsPerIndex;		/* 1, 2, 4, 8 */
    /*
     * Note: this is not quite the same as the Decode array for images:
     * [0..1] designates the range of the corresponding component of the
     * color space, not the literal values 0..1.  This is the same for
     * all color spaces except Lab, where the default values here are
     * [0 1 0 1 0 1] rather than [0 100 amin amax bmin bmax].
     */
    const float *Decode;
    /*
     * The client must provide a Table whose size is at least
     * ((1 << BitsPerIndex) + 1) * NumComponents.  After the stream is
     * closed, the first (N + 1) * NumComponents bytes of the Table
     * will hold the palette, where N is the contents of the last byte of
     * the Table.
     */
    gs_bytestring Table;
    /* The following change dynamically. */
    int hash_table[400];	/* holds byte offsets in Table */
    int next_index;		/* next Table offset to assign */
    uint byte_in;
    int in_bits_left;
    int next_component;
    uint byte_out;
    int x;
} stream_IE_state;

#define private_st_IE_state()	/* in gdevpsds.c */\
  gs_public_st_composite(st_IE_state, stream_IE_state, "stream_IE_state",\
    ie_state_enum_ptrs, ie_state_reloc_ptrs)

extern const stream_template s_IE_template;

/* ---------------- Downsampling ---------------- */

/* Downsample, possibly with anti-aliasing. */
#define stream_Downsample_state_common\
	stream_state_common;\
		/* The client sets the following before initialization. */\
	int Colors;\
	int WidthIn, HeightIn;\
	int XFactor, YFactor;\
	bool AntiAlias;\
	bool padX, padY;	/* keep excess samples */\
		/* The following are updated dynamically. */\
	int x, y		/* position within input image */
#define s_Downsample_set_defaults_inline(ss)\
  ((ss)->AntiAlias = (ss)->padX = (ss)->padY = false)
typedef struct stream_Downsample_state_s {
    stream_Downsample_state_common;
} stream_Downsample_state;

/* Return the number of samples after downsampling. */
int s_Downsample_size_out(int size_in, int factor, bool pad);

/* Subsample */
typedef struct stream_Subsample_state_s {
    stream_Downsample_state_common;
} stream_Subsample_state;
extern const stream_template s_Subsample_template;

/* Average */
typedef struct stream_Average_state_s {
    stream_Downsample_state_common;
    uint sum_size;
    uint copy_size;
    uint *sums;			/* accumulated sums for average */
} stream_Average_state;

#define private_st_Average_state()	/* in gdevpsds.c */\
  gs_private_st_ptrs1(st_Average_state, stream_Average_state,\
    "stream_Average_state", avg_enum_ptrs, avg_reloc_ptrs, sums)
extern const stream_template s_Average_template;

/* ---------------- Image compression chooser ---------------- */

typedef struct stream_compr_chooser_state_s {
    stream_state_common;
    uint choice;
    uint width, height, depth, bits_per_sample;
    uint samples_count, bits_left;
    ulong packed_data;
    byte *sample;
    ulong upper_plateaus, lower_plateaus;
    ulong gradients;
} stream_compr_chooser_state;

#define private_st_compr_chooser_state()	/* in gdevpsds.c */\
  gs_private_st_ptrs1(st_compr_chooser_state, stream_compr_chooser_state, \
    "stream_compr_chooser_state",\
    compr_chooser_enum_ptrs, compr_chooser_reloc_ptrs, sample)

extern const stream_template s_compr_chooser_template;

/* Set image dimensions. */
int
s_compr_chooser_set_dimensions(stream_compr_chooser_state * st, int width, 
			       int height, int depth, int bits_per_sample);

/* Get choice */
uint s_compr_chooser__get_choice(stream_compr_chooser_state *st, bool force);

/* ---------------- Am image color conversion filter ---------------- */

#ifndef gx_device_DEFINED
#  define gx_device_DEFINED
typedef struct gx_device_s gx_device;
#endif

typedef struct stream_image_colors_state_s stream_image_colors_state;

struct stream_image_colors_state_s {
    stream_state_common;
    uint width, height, depth, bits_per_sample;
    byte output_bits_buffer;
    uint output_bits_buffered;
    uint output_component_bits_written;
    uint output_component_index;
    uint output_depth, output_bits_per_sample;
    uint raster;
    uint row_bits;
    uint row_bits_passed;
    uint row_alignment_bytes;
    uint row_alignment_bytes_left;
    uint input_component_index;
    uint input_bits_buffer;
    uint input_bits_buffered;
    uint input_color[GS_IMAGE_MAX_COLOR_COMPONENTS];
    uint output_color[GS_IMAGE_MAX_COLOR_COMPONENTS];
    uint MaskColor[GS_IMAGE_MAX_COLOR_COMPONENTS * 2];
    float Decode[GS_IMAGE_MAX_COLOR_COMPONENTS * 2];
    const gs_color_space *pcs;
    gx_device *pdev;
    const gs_imager_state *pis;
    int (*convert_color)(stream_image_colors_state *);
};

#define private_st_image_colors_state()	/* in gdevpsds.c */\
  gs_private_st_ptrs3(st_stream_image_colors_state, stream_image_colors_state,\
    "stream_image_colors_state", stream_image_colors_enum_ptrs,\
    stream_image_colors_reloc_ptrs, pcs, pdev, pis)

extern const stream_template s_image_colors_template;

void s_image_colors_set_dimensions(stream_image_colors_state * st, 
			       int width, int height, int depth, int bits_per_sample);

void s_image_colors_set_mask_colors(stream_image_colors_state * ss, uint *MaskColor);

void s_image_colors_set_color_space(stream_image_colors_state * ss, gx_device *pdev,
			       const gs_color_space *pcs, const gs_imager_state *pis,
			       float *Decode);

extern const stream_template s__image_colors_template;

#endif /* gdevpsds_INCLUDED */

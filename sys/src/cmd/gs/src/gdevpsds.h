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

/*$Id: gdevpsds.h,v 1.4 2000/09/19 19:00:21 lpd Exp $ */
/* Image processing stream interface for PostScript and PDF writers */

#ifndef gdevpsds_INCLUDED
#  define gdevpsds_INCLUDED

#include "strimpl.h"

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
int s_1248_init(P3(stream_1248_state *ss, int Columns, int samples_per_pixel));

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
int s_C2R_init(P2(stream_C2R_state *ss, const gs_imager_state *pis));

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
int s_Downsample_size_out(P3(int size_in, int factor, bool pad));

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

#endif /* gdevpsds_INCLUDED */

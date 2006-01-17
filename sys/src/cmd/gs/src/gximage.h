/* Copyright (C) 1989, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gximage.h,v 1.8 2005/06/08 14:00:32 igor Exp $ */
/* Default image rendering state structure */
/* Requires gxcpath.h, gxdevmem.h, gxdcolor.h, gzpath.h */

#ifndef gximage_INCLUDED
#  define gximage_INCLUDED

#include "gsiparam.h"
#include "gxcspace.h"
#include "strimpl.h"		/* for sisparam.h */
#include "sisparam.h"
#include "gxdda.h"
#include "gxiclass.h"
#include "gxiparam.h"
#include "gxsample.h"

/* Define the abstract type for the image enumerator state. */
/*typedef struct gx_image_enum_s gx_image_enum;*/  /* in gxiclass.h */

/*
 * Incoming samples may go through two different transformations:
 *
 *      - For N-bit input samples with N <= 8, N-to-8-bit expansion
 *      may involve a lookup map.  Currently this map is either an
 *      identity function or a subtraction from 1 (inversion).
 *
 *      - The 8-bit or frac expanded sample may undergo decoding (a linear
 *      transformation) before being handed off to the color mapping
 *      machinery.
 *
 * If the decoding function's range is [0..1], we fold it into the
 * expansion lookup; otherwise we must compute it separately.
 * For speed, we distinguish 3 different cases of the decoding step:
 */
typedef enum {
    sd_none,			/* decoded during expansion */
    sd_lookup,			/* use lookup_decode table */
    sd_compute			/* compute using base and factor */
} sample_decoding;
struct sample_map_s {

    sample_lookup_t table;

    /*
     * If an 8-bit fraction doesn't represent the decoded value
     * accurately enough, but the samples have 4 bits or fewer,
     * we precompute the decoded values into a table.
     * Different entries are used depending on bits/sample:
     *      1,8,12 bits/sample: 0,15
     *      2 bits/sample: 0,5,10,15
     *      4 bits/sample: all
     */

    float decode_lookup[16];
#define decode_base decode_lookup[0]
#define decode_max decode_lookup[15]

    /*
     * In the worst case, we have to do the decoding on the fly.
     * The value is base + sample * factor, where the sample is
     * an 8-bit (unsigned) integer or a frac.
     */

    double decode_factor;

    sample_decoding decoding;

    /*
     * If decoding is sd_none for a non-mask image, we still need to know
     * whether the table includes an inversion, so that we can transform
     * mask values correctly.
     */

    bool inverted;

};

#ifndef sample_map_DEFINED
#define sample_map_DEFINED
typedef struct sample_map_s sample_map;
#endif

/* Decode an 8-bit sample into a floating point color component. */
/* penum points to the gx_image_enum structure. */
#define decode_sample(sample_value, cc, i)\
  switch ( penum->map[i].decoding )\
  {\
  case sd_none:\
    cc.paint.values[i] = (sample_value) * (1.0 / 255.0);  /* faster than / */\
    break;\
  case sd_lookup:	/* <= 4 significant bits */\
    cc.paint.values[i] =\
      penum->map[i].decode_lookup[(sample_value) >> 4];\
    break;\
  case sd_compute:\
    cc.paint.values[i] =\
      penum->map[i].decode_base + (sample_value) * penum->map[i].decode_factor;\
  }

/* Decode a frac value similarly. */
#define decode_frac(frac_value, cc, i)\
  cc.paint.values[i] =\
    penum->map[i].decode_base + (frac_value) * penum->map[i].decode_factor

/*
 * Declare the pointer that holds the 12-bit unpacking procedure
 * if 12-bit samples are supported, 0 otherwise.
 */
extern const sample_unpack_proc_t sample_unpack_12_proc;

/*
 * Declare the pointer that holds the 16-bit unpacking procedure
 * if 16-bit samples are supported, 0 otherwise.
 */
extern const sample_unpack_proc_t sample_unpack_16_proc;

/* Define the distinct postures of an image. */
/* Each posture includes its reflected variant. */
typedef enum {
    image_portrait = 0,		/* 0 or 180 degrees */
    image_landscape,		/* 90 or 270 degrees */
    image_skewed		/* any other transformation */
} image_posture;

/*
 * Define an entry in the image color table.  For single-source-plane
 * images, the table index is the sample value, and the key is not used;
 * for multiple-plane (color) images, the table index is a hash of the key,
 * which is the concatenation of the source pixel components.
 * "Clue" = Color LookUp Entry (by analogy with CLUT).
 */
typedef struct gx_image_clue_s {
    gx_device_color dev_color;
    bits32 key;
} gx_image_clue;

/* Main state structure */

#ifndef gx_device_clip_DEFINED
#  define gx_device_clip_DEFINED
typedef struct gx_device_clip_s gx_device_clip;
#endif

#ifndef gx_device_rop_texture_DEFINED
#  define gx_device_rop_texture_DEFINED
typedef struct gx_device_rop_texture_s gx_device_rop_texture;
#endif

struct gx_image_enum_s {
    gx_image_enum_common;
    /* We really want the map structure to be long-aligned, */
    /* so we choose shorter types for some flags. */
    /* Following are set at structure initialization */
    byte bps;			/* bits per sample: 1, 2, 4, 8, 12 */
    byte unpack_bps;		/* bps for computing unpack proc, */
				/* set to 8 if no unpacking */
    byte log2_xbytes;		/* log2(bytes per expanded sample): */
				/* 0 if bps <= 8, log2(sizeof(frac)) */
				/* if bps > 8 */
    byte spp;			/* samples per pixel */
    gs_image_alpha_t alpha;	/* Alpha from image structure */
    struct mc_ {
	uint values[GS_IMAGE_MAX_COMPONENTS * 2]; /* MaskColor values, */
				/* always as ranges, guaranteed in range */
				/* and in order (v0 <= v1) */
	bits32 mask, test;	/* (if spp > 1, bps <= 8) */
				/* mask & test value for quick filtering */
	bool exact;		/* (if spp > 1, bps <= 8) */
				/* if true, mask/test filter is exact */
    } mask_color;		/* (if ImageType 4) */
    byte use_mask_color;	/* true if color masking is being used */
    /*byte num_planes; */	/* (in common part) */
    byte spread;		/* (spp if multi-plane, 1 if not) */
				/* << log2_xbytes */
    byte masked;		/* 0 = [color]image, 1 = imagemask */
    byte interpolate;		/* true if Interpolate requested */
    gs_matrix matrix;		/* image space -> device space */
    struct r_ {
	int x, y, w, h;		/* subrectangle being rendered */
    } rect;
    gs_fixed_point x_extent, y_extent;	/* extent of one row of rect */
    SAMPLE_UNPACK_PROC((*unpack));
    irender_proc((*render));
    const gs_imager_state *pis;
    const gs_color_space *pcs;	/* color space of image */
    gs_memory_t *memory;
    byte *buffer;		/* for expanding samples to a */
				/* byte or frac */
    uint buffer_size;
    byte *line;			/* buffer for an output scan line */
    uint line_size;
    uint line_width;		/* width of line in device pixels */
    image_posture posture;
    byte use_rop;		/* true if CombineWithColor requested */
    byte clip_image;		/* mask, see below */
    /* Either we are clipping to a rectangle, in which case */
    /* the individual x/y flags may be set, or we are clipping */
    /* to a general region, in which case only clip_region */
    /* is set. */
#define image_clip_xmin 1
#define image_clip_xmax 2
#define image_clip_ymin 4
#define image_clip_ymax 8
#define image_clip_region 0x10
    byte slow_loop;		/* true if must use slower loop */
				/* (if needed) */
    byte device_color;		/* true if device color space and */
				/* standard decoding */
    gs_fixed_rect clip_outer;	/* outer box of clip path */
    gs_fixed_rect clip_inner;	/* inner box of clip path */
    gs_logical_operation_t log_op;	/* logical operation */
    fixed adjust;		/* adjustment when rendering */
				/* characters */
    fixed dxx, dxy;		/* fixed versions of matrix */
				/* components (as needed) */
    gx_device_clip *clip_dev;	/* clipping device (if needed) */
    gx_device_rop_texture *rop_dev;	/* RasterOp device (if needed) */
    stream_image_scale_state *scaler;	/* scale state for Interpolate */
				/* (if needed) */
    /* Following are updated dynamically */
    int y;			/* next source y */
    gs_int_point used;		/* amount of data already used, if */
				/* interrupted by error */
    gs_fixed_point cur, prev;	/* device x, y of current & */
				/* previous row */
    struct dd_ {
	gx_dda_fixed_point row;	/* DDA for row origin, has been */
				/* advanced when render proc called */
	gx_dda_fixed_point strip;  /* row + rect.x */
	gx_dda_fixed_point pixel0;	/* DDA for first pixel to render, */
				/* strip + used.x */
    } dda;
    int line_xy;		/* x or y value at start of buffered line */
    int xi_next;		/* expected xci of next row */
				/* (landscape only) */
    gs_int_point xyi;		/* integer origin of row */
				/* (Interpolate only) */
    int yci, hci;		/* integer y & h of row (portrait) */
    int xci, wci;		/* integer x & w of row (landscape) */
    /* The maps are set at initialization.  We put them here */
    /* so that the scalars will have smaller offsets. */
    sample_map map[GS_IMAGE_MAX_COMPONENTS];
    /* Entries 0 and 255 of the following are set at initialization */
    /* for monochrome images; other entries are updated dynamically. */
    gx_image_clue clues[256];
#define icolor0 clues[0].dev_color
#define icolor1 clues[255].dev_color
};

/* Enumerate the pointers in an image enumerator. */
#define gx_image_enum_do_ptrs(m)\
  m(0,pis) m(1,pcs) m(2,dev) m(3,buffer) m(4,line)\
  m(5,clip_dev) m(6,rop_dev) m(7,scaler)
#define gx_image_enum_num_ptrs 8
#define private_st_gx_image_enum() /* in gsimage.c */\
  gs_private_st_composite(st_gx_image_enum, gx_image_enum, "gx_image_enum",\
    image_enum_enum_ptrs, image_enum_reloc_ptrs)

/* Compare two device colors for equality. */
/* We can special-case this for speed later if we care. */
#define dev_color_eq(devc1, devc2)\
  gx_device_color_equal(&(devc1), &(devc2))

/*
 * Scale a pair of mask_color values to match the scaling of each sample to
 * a full byte, and complement and swap them if the map incorporates
 * a Decode = [1 0] inversion.
 */
void gx_image_scale_mask_colors(gx_image_enum *penum,
				int component_index);

/*
 * Do common initialization for processing an ImageType 1 or 4 image.
 * Allocate the enumerator and fill in the following members:
 *	rect
 */
int
gx_image_enum_alloc(const gs_image_common_t * pic,
		    const gs_int_rect * prect,
		    gs_memory_t * mem, gx_image_enum **ppenum);

/*
 * Finish initialization for processing an ImageType 1 or 4 image.
 * Assumes the following members of *penum are set in addition to those
 * set by gx_image_enum_alloc:
 *	alpha, use_mask_color, mask_color (if use_mask_color is true),
 *	masked, adjust
 */
int
gx_image_enum_begin(gx_device * dev, const gs_imager_state * pis,
		    const gs_matrix *pmat, const gs_image_common_t * pic,
		    const gx_drawing_color * pdcolor,
		    const gx_clip_path * pcpath,
		    gs_memory_t * mem, gx_image_enum *penum);

/*
 * Clear the relevant clues. Exported for use by image_render_*
 * when ht_tile cache is invalidated.
 */
void
image_init_clues(gx_image_enum * penum, int bps, int spp);

#endif /* gximage_INCLUDED */

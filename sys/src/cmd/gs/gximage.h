/* Copyright (C) 1989, 1990, 1991, 1992, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gximage.h */
/* Internal definitions for image rendering */
/* Requires gxcpath.h, gxdevmem.h, gxdcolor.h, gzpath.h */
#include "gscspace.h"
#include "gsimage.h"
#include "gsiscale.h"

/* Interface for routine used to unpack and shuffle incoming samples. */
/* The Unix C compiler can't handle typedefs for procedure */
/* (as opposed to pointer-to-procedure) types, */
/* so we have to do it with macros instead. */
#define iunpack_proc(proc)\
  void proc(P6(const gs_image_enum *penum, const sample_map *pmap, byte *bptr, const byte *data, uint dsize, uint inpos))
/* Interface for routine used to render a (source) scan line. */
#define irender_proc(proc)\
  int proc(P4(gs_image_enum *penum, byte *buffer, uint w, int h))

/*
 * Incoming samples may go through two different transformations:
 *
 *	- For N-bit input samples with N <= 8, N-to-8-bit expansion
 *	may involve a lookup map.  Currently this map is either an
 *	identity function or a subtraction from 1 (inversion).
 *
 *	- The 8-bit or frac expanded sample may undergo decoding (a linear
 *	transformation) before being handed off to the color mapping
 *	machinery.
 *
 * If the decoding function's range is [0..1], we fold it into the
 * expansion lookup; otherwise we must compute it separately.
 * For speed, we distinguish 3 different cases of the decoding step:
 */
typedef enum {
	sd_none,		/* decoded during expansion */
	sd_lookup,		/* use lookup_decode table */
	sd_compute		/* compute using base and factor */
} sample_decoding;
typedef struct sample_map_s {

	/* The following union implements the expansion of sample */
	/* values from N bits to 8, and a possible inversion. */

	union {

		bits32 lookup4x1to32[16]; /* 1 bit/sample, not spreading */
		bits16 lookup2x2to16[16]; /* 2 bits/sample, not spreading */
		byte lookup8[256];	/* 1 bit/sample, spreading [2] */
					/* 2 bits/sample, spreading [4] */
					/* 4 bits/sample [16] */
					/* 8 bits/sample [256] */

	} table;

	/* If an 8-bit fraction doesn't represent the decoded value */
	/* accurately enough, but the samples have 4 bits or fewer, */
	/* we precompute the decoded values into a table. */
	/* Different entries are used depending on bits/sample: */
	/*	1,8,12 bits/sample: 0,15	*/
	/*	2 bits/sample: 0,5,10,15	*/
	/*	4 bits/sample: all	*/

	float decode_lookup[16];
#define decode_base decode_lookup[0]
#define decode_max decode_lookup[15]

	/* In the worst case, we have to do the decoding on the fly. */
	/* The value is base + sample * factor, where the sample is */
	/* an 8-bit (unsigned) integer or a frac. */

	double decode_factor;

	sample_decoding decoding;

} sample_map;

/* Decode an 8-bit sample into a floating point color component. */
/* penum points to the gs_image_enum structure. */
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

/* Define the distinct postures of an image. */
/* Each posture includes its reflected variant. */
typedef enum {
  image_portrait = 0,			/* 0 or 180 degrees */
  image_landscape,			/* 90 or 270 degrees */
  image_skewed				/* any other transformation */
} image_posture;

/* Main state structure */
struct gs_image_enum_s {
	/* We really want the map structure to be long-aligned, */
	/* so we choose shorter types for some flags. */
	/* Following are set at structure initialization */
	int width;
	int height;
	int bps;			/* bits per sample: 1, 2, 4, 8, 12 */
	int log2_xbytes;		/* log2(bytes per expanded sample): */
					/* 0 if bps <= 8, log2(sizeof(frac)) */
					/* if bps > 8 */
	int spp;			/* samples per pixel: 1, 3, or 4 */
	int num_planes;			/* spp if colors are separated, */
					/* 1 otherwise */
	int spread;			/* num_planes << log2_xbytes */
	int masked;			/* 0 = [color]image, 1 = imagemask */
	fixed fxx, fxy, fyx, fyy, mtx, mty;	/* fixed version of matrix */
	iunpack_proc((*unpack));
	irender_proc((*render));
	gs_state *pgs;
	gs_fixed_rect clip_box;		/* pgs->clip_path.path->bbox, */
					/* possibly translated */
	byte *buffer;			/* for expanding samples to a */
					/* byte or frac */
	uint buffer_size;
	byte *line;			/* buffer for an output scan line */
	uint line_size;
	uint line_width;		/* width of line in device pixels */
	uint bytes_per_row;		/* # of input bytes per row */
					/* (per plane, if spp == 1 and */
					/* num_planes > 1) */
	image_posture posture;
	byte interpolate;		/* true if Interpolate requested */
	byte never_clip;		/* true if entire image fits */
	byte slow_loop;			/* true if !(skewed | */
					/* imagemask with a halftone) */
	byte device_color;		/* true if device color space and */
					/* standard decoding */
	fixed adjust;			/* adjustment when rendering */
					/* characters */
	gx_device_clip *clip_dev;	/* clipping device (if needed) */
	gs_image_scale_state scale_state;	/* scale state for */
					/* Interpolate (if needed) */
	/* Following are updated dynamically */
	gs_const_string planes[4];	/* separated color data, */
				/* all planes have the same size */
	int plane_index;		/* current plane index, [0..spp) */
	uint byte_in_row;		/* current input byte position in row */
	fixed xcur, ycur;		/* device x, y of current row */
	union {
	  struct { int c, d; } y;	/* integer y & h of row (skew=0) */
	  struct { int c, d; } x;	/* integer x & w of row (skew=2) */
	} i;
#define yci i.y.c
#define hci i.y.d
#define xci i.x.c
#define wci i.x.d
	int y;
	/* The maps are set at initialization.  We put them here */
	/* so that the scalars will have smaller offsets. */
	sample_map map[4];
	/* Entries 0 and 255 of the following are set at initialization */
	/* for monochrome images; other entries are updated dynamically. */
	gx_device_color dev_colors[256];
#define icolor0 dev_colors[0]
#define icolor1 dev_colors[255]
};
#define private_st_gs_image_enum() /* in gsimage.c */\
  gs_private_st_composite(st_gs_image_enum, gs_image_enum, "gs_image_enum",\
    image_enum_enum_ptrs, image_enum_reloc_ptrs)

/* Compare two device colors for equality. */
#define dev_color_eq(devc1, devc2)\
  (devc1.type == &gx_dc_pure ?\
   devc2.type == &gx_dc_pure &&\
   devc1.colors.pure == devc2.colors.pure :\
   devc1.type == &gx_dc_ht_binary ?\
   devc2.type == &gx_dc_ht_binary &&\
   devc1.colors.binary.color[0] == devc2.colors.binary.color[0] &&\
   devc1.colors.binary.color[1] == devc2.colors.binary.color[1] &&\
   devc1.colors.binary.b_level == devc2.colors.binary.b_level :\
   0)

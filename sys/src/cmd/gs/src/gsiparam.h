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

/* $Id: gsiparam.h,v 1.8 2002/08/22 07:12:29 henrys Exp $ */
/* Image parameter definition */

#ifndef gsiparam_INCLUDED
#  define gsiparam_INCLUDED

#include "gsccolor.h"		/* for GS_CLIENT_COLOR_MAX_COMPONENTS */
#include "gsmatrix.h"
#include "gsstype.h"		/* for extern_st */

/* ---------------- Image parameters ---------------- */

/*
 * Unfortunately, we defined the gs_image_t type as designating an ImageType
 * 1 image or mask before we realized that there were going to be other
 * ImageTypes.  We could redefine this type to include a type field without
 * perturbing clients, but it would break implementations of driver
 * begin_image procedures, since they are currently only prepared to handle
 * ImageType 1 images and would have to be modified to check the ImageType.
 * Therefore, we use gs_image_common_t for an abstract image type, and
 * gs_image<n>_t for the various ImageTypes.
 */

/*
 * Define the data common to all image types.  The type structure is
 * opaque here, defined in gxiparam.h.
 */
#ifndef gx_image_type_DEFINED
#  define gx_image_type_DEFINED
typedef struct gx_image_type_s gx_image_type_t;

#endif
#define gs_image_common\
	const gx_image_type_t *type;\
		/*\
		 * Define the transformation from user space to image space.\
		 */\
	gs_matrix ImageMatrix
typedef struct gs_image_common_s {
    gs_image_common;
} gs_image_common_t;

#define public_st_gs_image_common() /* in gximage.c */\
  gs_public_st_simple(st_gs_image_common, gs_image_common_t,\
    "gs_image_common_t")

/*
 * Define the maximum number of components in image data.
 * The +1 is for either color + alpha or mask + color.
 */
#define GS_IMAGE_MAX_COLOR_COMPONENTS GS_CLIENT_COLOR_MAX_COMPONENTS
#define GS_IMAGE_MAX_COMPONENTS (GS_IMAGE_MAX_COLOR_COMPONENTS + 1)
/* Backward compatibility */
#define gs_image_max_components GS_IMAGE_MAX_COMPONENTS

/*
 * Define the maximum number of planes in image data.  Since we support
 * allocating a plane for each bit, the maximum value is the maximum number
 * of components (see above) times the maximum depth per component
 * (currently 8 for multi-component bit-planar images, but could be 16
 * someday; 32 or maybe 64 for DevicePixel images).
 */
#define GS_IMAGE_MAX_PLANES (GS_IMAGE_MAX_COMPONENTS * 8)
/* Backward compatibility */
#define gs_image_max_planes GS_IMAGE_MAX_PLANES

/*
 * Define the structure for defining data common to ImageType 1 images,
 * ImageType 3 DataDicts and MaskDicts, and ImageType 4 images -- i.e.,
 * all the image types that use explicitly supplied data.  It follows
 * closely the discussion on pp. 219-223 of the PostScript Language
 * Reference Manual, Second Edition, with the following exceptions:
 *
 *      DataSource and MultipleDataSources are not members of this
 *      structure, since the structure doesn't take a position on
 *      how the data are actually supplied.
 */
#define gs_data_image_common\
	gs_image_common;\
		/*\
		 * Define the width of source image in pixels.\
		 */\
	int Width;\
		/*\
		 * Define the height of source image in pixels.\
		 */\
	int Height;\
		/*\
		 * Define B, the number of bits per pixel component.\
		 * Currently this must be 1 for masks.\
		 */\
	int BitsPerComponent;\
		/*\
		 * Define the linear remapping of the input values.\
		 * For the I'th pixel component, we start by treating\
		 * the B bits of component data as a fraction F between\
		 * 0 and 1; the actual component value is then\
		 * Decode[I*2] + F * (Decode[I*2+1] - Decode[I*2]).\
		 * For masks, only the first two entries are used;\
		 * they must be 1,0 for write-0s masks, 0,1 for write-1s.\
		 */\
	float Decode[GS_IMAGE_MAX_COMPONENTS * 2];\
		/*\
		 * Define whether to smooth the image.\
		 */\
	bool Interpolate
typedef struct gs_data_image_s {
    gs_data_image_common;
} gs_data_image_t;

#define public_st_gs_data_image() /* in gximage.c */\
  gs_public_st_simple(st_gs_data_image, gs_data_image_t,\
    "gs_data_image_t")

/*
 * Define the data common to ImageType 1 images, ImageType 3 DataDicts,
 * and ImageType 4 images -- i.e., all the image types that provide pixel
 * (as opposed to mask) data.  The following are added to the PostScript
 * image parameters:
 *
 *      format is not PostScript or PDF standard: it is normally derived
 *      from MultipleDataSources.
 *
 *      ColorSpace is added from PDF.
 *
 *      CombineWithColor is not PostScript or PDF standard: see the
 *      RasterOp section of Language.htm for a discussion of
 *      CombineWithColor.
 */
typedef enum {
    /* Single plane, chunky pixels. */
    gs_image_format_chunky = 0,
    /* num_components planes, chunky components. */
    gs_image_format_component_planar = 1,
    /* BitsPerComponent * num_components planes, 1 bit per plane */
    gs_image_format_bit_planar = 2
} gs_image_format_t;

/* Define an opaque type for a color space. */
#ifndef gs_color_space_DEFINED
#  define gs_color_space_DEFINED
typedef struct gs_color_space_s gs_color_space;

#endif

#define gs_pixel_image_common\
	gs_data_image_common;\
		/*\
		 * Define how the pixels are divided up into planes.\
		 */\
	gs_image_format_t format;\
		/*\
		 * Define the source color space (must be NULL for masks).\
		 */\
	const gs_color_space *ColorSpace;\
		/*\
		 * Define whether to use the drawing color as the\
		 * "texture" for RasterOp.  For more information,\
		 * see the discussion of RasterOp in Language.htm.\
		 */\
	bool CombineWithColor
typedef struct gs_pixel_image_s {
    gs_pixel_image_common;
} gs_pixel_image_t;

extern_st(st_gs_pixel_image);
#define public_st_gs_pixel_image() /* in gximage.c */\
  gs_public_st_ptrs1(st_gs_pixel_image, gs_pixel_image_t,\
    "gs_data_image_t", pixel_image_enum_ptrs, pixel_image_reloc_ptrs,\
    ColorSpace)

/*
 * Define an ImageType 1 image.  ImageMask is an added member from PDF.
 * adjust and Alpha are not PostScript or PDF standard.
 */
typedef enum {
    /* No alpha.  This must be 0 for true-false tests. */
    gs_image_alpha_none = 0,
    /* Alpha precedes color components. */
    gs_image_alpha_first,
    /* Alpha follows color components. */
    gs_image_alpha_last
} gs_image_alpha_t;

typedef struct gs_image1_s {
    gs_pixel_image_common;
    /*
     * Define whether this is a mask or a solid image.
     * For masks, Alpha must be 'none'.
     */
    bool ImageMask;
    /*
     * Define whether to expand each destination pixel, to make
     * masked characters look better.  Only used for masks.
     */
    bool adjust;
    /*
     * Define whether there is an additional component providing
     * alpha information for each pixel, in addition to the
     * components implied by the color space.
     */
    gs_image_alpha_t Alpha;
} gs_image1_t;

/* The descriptor is public for soft masks. */
extern_st(st_gs_image1);
#define public_st_gs_image1()	/* in gximage1.c */\
  gs_public_st_suffix_add0(st_gs_image1, gs_image1_t, "gs_image1_t",\
    image1_enum_ptrs, image1_reloc_ptrs, st_gs_pixel_image)

/*
 * In standard PostScript Level 1 and 2, this is the only defined ImageType.
 */
typedef gs_image1_t gs_image_t;

/*
 * Define procedures for initializing the standard forms of image structures
 * to default values.  Note that because these structures may add more
 * members in the future, all clients constructing gs_*image*_t values
 * *must* start by initializing the value by calling one of the following
 * procedures.  Note also that these procedures do not set the image type.
 */
void
  /*
   * Sets ImageMatrix to the identity matrix.
   */
     gs_image_common_t_init(gs_image_common_t * pic),
  /*
   * Also sets Width = Height = 0, BitsPerComponent = 1,
   * format = chunky, Interpolate = false.
   * If num_components = N > 0, sets the first N elements of Decode to (0, 1);
   * if num_components = N < 0, sets the first -N elements of Decode to (1, 0);
   * if num_components = 0, doesn't set Decode.
   */
     gs_data_image_t_init(gs_data_image_t * pim, int num_components),
  /*
   * Also sets CombineWithColor = false, ColorSpace = color_space, Alpha =
   * none.  num_components is obtained from ColorSpace; if ColorSpace =
   * NULL or ColorSpace is a Pattern space, num_components is taken as 0
   * (Decode is not initialized).
   */
    gs_pixel_image_t_init(gs_pixel_image_t * pim,
			  const gs_color_space * color_space);

/*
 * Initialize an ImageType 1 image (or imagemask).  Also sets ImageMask,
 * adjust, and Alpha, and the image type.  For masks, write_1s = false
 * paints 0s, write_1s = true paints 1s.  This is consistent with the
 * "polarity" operand of the PostScript imagemask operator.
 *
 * init and init_mask initialize adjust to true.  This is a bad decision
 * which unfortunately we can't undo without breaking backward
 * compatibility.  That is why we added init_adjust and init_mask_adjust.
 * Note that for init and init_adjust, adjust is only relevant if
 * pim->ImageMask is true.
 */
void gs_image_t_init_adjust(gs_image_t * pim, const gs_color_space * pcs,
			    bool adjust);
#define gs_image_t_init(pim, pcs)\
  gs_image_t_init_adjust(pim, pcs, true)
void gs_image_t_init_mask_adjust(gs_image_t * pim, bool write_1s,
				 bool adjust);
#define gs_image_t_init_mask(pim, write_1s)\
  gs_image_t_init_mask_adjust(pim, write_1s, true)


/****** REMAINDER OF FILE UNDER CONSTRUCTION. PROCEED AT YOUR OWN RISK. ******/

#if 0

/* ---------------- Services ---------------- */

/*
   In order to make the driver's life easier, we provide the following callback
   procedure:
 */

int gx_map_image_color(gx_device * dev,
		       const gs_image_t * pim,
		       const gx_color_rendering_info * pcri,
		       const uint components[GS_IMAGE_MAX_COMPONENTS],
		       gx_drawing_color * pdcolor);

/*
  Map a source color to a drawing color.  The components are simply the
  pixel component values from the input data, i.e., 1 to
  GS_IMAGE_MAX_COMPONENTS B-bit numbers from the source data.  Return 0 if
  the operation succeeded, or a negative error code.
 */

#endif /*************************************************************** */

#endif /* gsiparam_INCLUDED */

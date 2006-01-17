/* Copyright (C) 1997, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsiparm2.h,v 1.5 2002/06/16 08:45:42 lpd Exp $ */
/* ImageType 2 image parameter definition */

#ifndef gsiparm2_INCLUDED
#  define gsiparm2_INCLUDED

#include "gsiparam.h"

/* Opaque type for a path */
#ifndef gx_path_DEFINED
#  define gx_path_DEFINED
typedef struct gx_path_s gx_path;
#endif

/*
 * See Section 7.1 of the Adobe PostScript Version 3010 Supplement
 * for a definition of ImageType 2 images.
 */

typedef struct gs_image2_s {
    gs_image_common;
    gs_state *DataSource;
    float XOrigin, YOrigin;
    float Width, Height;
    /*
     * If UnpaintedPath is not 0, any unpainted path will be appended to it.
     */
    gx_path *UnpaintedPath;
    bool PixelCopy;
} gs_image2_t;
#define private_st_gs_image2()	/* in gximage2.c */\
  extern_st(st_gs_image_common);\
  gs_private_st_suffix_add2(st_gs_image2, gs_image2_t, "gs_image2_t",\
    image2_enum_ptrs, image2_reloc_ptrs, st_gs_image_common,\
    DataSource, UnpaintedPath)

/*
 * Initialize an ImageType 2 image.  Defaults:
 *      UnpaintedPath = 0
 *      PixelCopy = false
 */
void gs_image2_t_init(gs_image2_t * pim);

#endif /* gsiparm2_INCLUDED */

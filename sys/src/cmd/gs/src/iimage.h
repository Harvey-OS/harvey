/* Copyright (C) 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: iimage.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Image operator entry points */
/* Requires gscspace.h, gxiparam.h */

#ifndef iimage_INCLUDED
#  define iimage_INCLUDED

/* These procedures are exported by zimage.c for other modules. */

/* Exported for zcolor1.c */
int zimage_opaque_setup(P6(i_ctx_t *i_ctx_p, os_ptr op, bool multi,
			   gs_image_alpha_t alpha, const gs_color_space * pcs,
			   int npop));

/* Exported for zimage2.c */
int zimage_setup(P5(i_ctx_t *i_ctx_p, const gs_pixel_image_t * pim,
		    const ref * sources, bool uses_color, int npop));

/* Exported for zcolor3.c */
int zimage_data_setup(P5(i_ctx_t *i_ctx_p, const gs_pixel_image_t * pim,
			 gx_image_enum_common_t * pie,
			 const ref * sources, int npop));

/* Exported for zcolor1.c and zdpnext.c. */
int zimage_multiple(P2(i_ctx_t *i_ctx_p, bool has_alpha));

#endif /* iimage_INCLUDED */

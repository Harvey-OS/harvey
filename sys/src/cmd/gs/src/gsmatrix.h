/* Copyright (C) 1989, 1995, 1997, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsmatrix.h,v 1.5 2002/06/16 08:45:42 lpd Exp $ */
/* Definition of matrices and client interface to matrix routines */

#ifndef gsmatrix_INCLUDED
#  define gsmatrix_INCLUDED

/* See p. 65 of the PostScript manual for the semantics of */
/* transformation matrices. */

/* Structure for a transformation matrix. */
#define _matrix_body\
  float xx, xy, yx, yy, tx, ty
struct gs_matrix_s {
    _matrix_body;
};

#ifndef gs_matrix_DEFINED
#  define gs_matrix_DEFINED
typedef struct gs_matrix_s gs_matrix;
#endif

/* Macro for initializing constant matrices */
#define constant_matrix_body(xx, xy, yx, yy, tx, ty)\
  (float)(xx), (float)(xy), (float)(yx),\
  (float)(yy), (float)(tx), (float)(ty)

/* Macros for testing whether matrix coefficients are zero, */
/* for shortcuts when the matrix is simple. */
#define is_xxyy(pmat) is_fzero2((pmat)->xy, (pmat)->yx)
#define is_xyyx(pmat) is_fzero2((pmat)->xx, (pmat)->yy)

/* The identity matrix (for structure initialization) */
#define identity_matrix_body\
  constant_matrix_body(1, 0, 0, 1, 0, 0)

/* Matrix creation */
void gs_make_identity(gs_matrix *);
int gs_make_translation(floatp, floatp, gs_matrix *),
    gs_make_scaling(floatp, floatp, gs_matrix *),
    gs_make_rotation(floatp, gs_matrix *);

/* Matrix arithmetic */
int gs_matrix_multiply(const gs_matrix *, const gs_matrix *, gs_matrix *),
    gs_matrix_invert(const gs_matrix *, gs_matrix *),
    gs_matrix_translate(const gs_matrix *, floatp, floatp, gs_matrix *),
    gs_matrix_scale(const gs_matrix *, floatp, floatp, gs_matrix *),
    gs_matrix_rotate(const gs_matrix *, floatp, gs_matrix *);

/* Coordinate transformation */
int gs_point_transform(floatp, floatp, const gs_matrix *, gs_point *),
    gs_point_transform_inverse(floatp, floatp, const gs_matrix *, gs_point *),
    gs_distance_transform(floatp, floatp, const gs_matrix *, gs_point *),
    gs_distance_transform_inverse(floatp, floatp, const gs_matrix *, gs_point *),
    gs_points_bbox(const gs_point[4], gs_rect *),
    gs_bbox_transform_only(const gs_rect *, const gs_matrix *, gs_point[4]),
    gs_bbox_transform(const gs_rect *, const gs_matrix *, gs_rect *),
    gs_bbox_transform_inverse(const gs_rect *, const gs_matrix *, gs_rect *);

/* Serialization */
#ifndef stream_DEFINED
#  define stream_DEFINED
typedef struct stream_s stream;
#endif
int sget_matrix(stream *, gs_matrix *);
int sput_matrix(stream *, const gs_matrix *);

#endif /* gsmatrix_INCLUDED */

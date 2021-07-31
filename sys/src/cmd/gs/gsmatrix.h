/* Copyright (C) 1989, 1990, 1991, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* gsmatrix.h */
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

/* The identity matrix (for structure initialization) */
#define identity_matrix_body\
	constant_matrix_body(1, 0, 0, 1, 0, 0)

/* Matrix creation */
void	gs_make_identity(P1(gs_matrix *));
int	gs_make_translation(P3(floatp, floatp, gs_matrix *)),
	gs_make_scaling(P3(floatp, floatp, gs_matrix *)),
	gs_make_rotation(P2(floatp, gs_matrix *));

/* Matrix arithmetic */
int	gs_matrix_multiply(P3(const gs_matrix *, const gs_matrix *, gs_matrix *)),
	gs_matrix_invert(P2(const gs_matrix *, gs_matrix *)),
	gs_matrix_rotate(P3(const gs_matrix *, floatp, gs_matrix *));

/* Coordinate transformation */
int	gs_point_transform(P4(floatp, floatp, const gs_matrix *, gs_point *)),
	gs_point_transform_inverse(P4(floatp, floatp, const gs_matrix *, gs_point *)),
	gs_distance_transform(P4(floatp, floatp, const gs_matrix *, gs_point *)),
	gs_distance_transform_inverse(P4(floatp, floatp, const gs_matrix *, gs_point *)),
	gs_bbox_transform(P3(const gs_rect *, const gs_matrix *, gs_rect *)),
	gs_bbox_transform_inverse(P3(const gs_rect *, const gs_matrix *, gs_rect *));

#endif					/* gsmatrix_INCLUDED */

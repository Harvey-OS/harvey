/* Copyright (C) 1989, 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* gscoord.h */
/* Client interface to coordinate system operations */
/* Requires gsmatrix.h and gsstate.h */

/* Coordinate system modification */
int	gs_initmatrix(P1(gs_state *)),
	gs_defaultmatrix(P2(const gs_state *, gs_matrix *)),
	gs_currentmatrix(P2(const gs_state *, gs_matrix *)),
	gs_currentcharmatrix(P3(gs_state *, gs_matrix *, int)),
	gs_setmatrix(P2(gs_state *, const gs_matrix *)),
	gs_setcharmatrix(P2(gs_state *, const gs_matrix *)),
	gs_settocharmatrix(P1(gs_state *)),
	gs_translate(P3(gs_state *, floatp, floatp)),
	gs_scale(P3(gs_state *, floatp, floatp)),
	gs_rotate(P2(gs_state *, floatp)),
	gs_concat(P2(gs_state *, const gs_matrix *));

/* Coordinate transformation */
int	gs_transform(P4(gs_state *, floatp, floatp, gs_point *)),
	gs_dtransform(P4(gs_state *, floatp, floatp, gs_point *)),
	gs_itransform(P4(gs_state *, floatp, floatp, gs_point *)),
	gs_idtransform(P4(gs_state *, floatp, floatp, gs_point *));

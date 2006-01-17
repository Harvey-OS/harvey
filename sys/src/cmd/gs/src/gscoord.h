/* Copyright (C) 1989, 1996 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gscoord.h,v 1.5 2002/06/16 08:45:42 lpd Exp $ */
/* Interface to graphics state CTM procedures */
/* Requires gsmatrix.h and gsstate.h */

#ifndef gscoord_INCLUDED
#  define gscoord_INCLUDED

/* Coordinate system modification */
int gs_initmatrix(gs_state *),
    gs_defaultmatrix(const gs_state *, gs_matrix *),
    gs_currentmatrix(const gs_state *, gs_matrix *),
    gs_setmatrix(gs_state *, const gs_matrix *),
    gs_translate(gs_state *, floatp, floatp),
    gs_scale(gs_state *, floatp, floatp),
    gs_rotate(gs_state *, floatp),
    gs_concat(gs_state *, const gs_matrix *);

/* Extensions */
int gs_setdefaultmatrix(gs_state *, const gs_matrix *),
    gs_currentcharmatrix(gs_state *, gs_matrix *, bool),
    gs_setcharmatrix(gs_state *, const gs_matrix *),
    gs_settocharmatrix(gs_state *);

/* Coordinate transformation */
int gs_transform(gs_state *, floatp, floatp, gs_point *),
    gs_dtransform(gs_state *, floatp, floatp, gs_point *),
    gs_itransform(gs_state *, floatp, floatp, gs_point *),
    gs_idtransform(gs_state *, floatp, floatp, gs_point *);

#ifndef gs_imager_state_DEFINED
#  define gs_imager_state_DEFINED
typedef struct gs_imager_state_s gs_imager_state;
#endif

int gs_imager_setmatrix(gs_imager_state *, const gs_matrix *);
int gs_imager_idtransform(const gs_imager_state *, floatp, floatp, gs_point *);

#endif /* gscoord_INCLUDED */

/* Copyright (C) 1992, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gscolor2.h,v 1.3 2000/09/19 19:00:26 lpd Exp $ */
/* Client interface to Level 2 color facilities */
/* (requires gscspace.h, gsmatrix.h) */

#ifndef gscolor2_INCLUDED
#  define gscolor2_INCLUDED

#include "gscindex.h"
#include "gsptype1.h"

/* ---------------- Graphics state ---------------- */

/*
 * Note that setcolorspace and setcolor copy the (top level of) their
 * structure argument, so if the client allocated it on the heap, the
 * client should free it after setting it in the graphics state.
 */

/* General color routines */
const gs_color_space *gs_currentcolorspace(P1(const gs_state *));
int gs_setcolorspace(P2(gs_state *, const gs_color_space *));
const gs_client_color *gs_currentcolor(P1(const gs_state *));
int gs_setcolor(P2(gs_state *, const gs_client_color *));

/*
 * gs_currentcolorspace_index returns the index of the current color space
 * *before* any substitution.
 */
gs_color_space_index gs_currentcolorspace_index(P1(const gs_state *));

/* CIE-specific routines */
#ifndef gs_cie_render_DEFINED
#  define gs_cie_render_DEFINED
typedef struct gs_cie_render_s gs_cie_render;
#endif
const gs_cie_render *gs_currentcolorrendering(P1(const gs_state *));
int gs_setcolorrendering(P2(gs_state *, gs_cie_render *));

#endif /* gscolor2_INCLUDED */

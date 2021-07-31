/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gsht1.h */
/* Extended public interface to halftones */

#ifndef gsht1_INCLUDED
#  define gsht1_INCLUDED

#include "gsht.h"

/* Procedural interface */
int	gs_setcolorscreen(P2(gs_state *, gs_colorscreen_halftone *));
int	gs_currentcolorscreen(P2(gs_state *, gs_colorscreen_halftone *));

/* We include sethalftone here, even though it is a Level 2 feature, */
/* because it turns out to be convenient to define setcolorscreen */
/* using sethalftone. */
int	gs_sethalftone(P2(gs_state *, gs_halftone *));
int	gs_currenthalftone(P2(gs_state *, gs_halftone *));

#endif					/* gsht1_INCLUDED */

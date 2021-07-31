/* Copyright (C) 1991, 1992, 1993, 1996, 1997 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gscolor.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Client interface to color routines */

#ifndef gscolor_INCLUDED
#  define gscolor_INCLUDED

#include "gxtmap.h"

/* Color and gray interface */
int gs_setgray(P2(gs_state *, floatp));
float gs_currentgray(P1(const gs_state *));
int gs_setrgbcolor(P4(gs_state *, floatp, floatp, floatp)), gs_currentrgbcolor(P2(const gs_state *, float[3]));
int gs_setnullcolor(P1(gs_state *));

/* Transfer function */
int gs_settransfer(P2(gs_state *, gs_mapping_proc)), gs_settransfer_remap(P3(gs_state *, gs_mapping_proc, bool));
gs_mapping_proc gs_currenttransfer(P1(const gs_state *));

#endif /* gscolor_INCLUDED */

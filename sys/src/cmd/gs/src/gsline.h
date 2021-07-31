/* Copyright (C) 1994, 1995, 1996, 1997, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gsline.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Line parameter and quality definitions */

#ifndef gsline_INCLUDED
#  define gsline_INCLUDED

#include "gslparam.h"

/* Procedures */
int gs_setlinewidth(P2(gs_state *, floatp));
float gs_currentlinewidth(P1(const gs_state *));
int gs_setlinecap(P2(gs_state *, gs_line_cap));
gs_line_cap gs_currentlinecap(P1(const gs_state *));
int gs_setlinejoin(P2(gs_state *, gs_line_join));
gs_line_join gs_currentlinejoin(P1(const gs_state *));
int gs_setmiterlimit(P2(gs_state *, floatp));
float gs_currentmiterlimit(P1(const gs_state *));
int gs_setdash(P4(gs_state *, const float *, uint, floatp));
uint gs_currentdash_length(P1(const gs_state *));
const float *gs_currentdash_pattern(P1(const gs_state *));
float gs_currentdash_offset(P1(const gs_state *));
int gs_setflat(P2(gs_state *, floatp));
float gs_currentflat(P1(const gs_state *));
int gs_setstrokeadjust(P2(gs_state *, bool));
bool gs_currentstrokeadjust(P1(const gs_state *));

/* Extensions - device-independent */
void gs_setdashadapt(P2(gs_state *, bool));
bool gs_currentdashadapt(P1(const gs_state *));
int gs_setcurvejoin(P2(gs_state *, int));
int gs_currentcurvejoin(P1(const gs_state *));

/* Extensions - device-dependent */
void gs_setaccuratecurves(P2(gs_state *, bool));
bool gs_currentaccuratecurves(P1(const gs_state *));
int gs_setdotlength(P3(gs_state *, floatp, bool));
float gs_currentdotlength(P1(const gs_state *));
bool gs_currentdotlength_absolute(P1(const gs_state *));
int gs_setdotorientation(P1(gs_state *));
int gs_dotorientation(P1(gs_state *));

/* Imager-level procedures */
#ifndef gs_imager_state_DEFINED
#  define gs_imager_state_DEFINED
typedef struct gs_imager_state_s gs_imager_state;
#endif
int gs_imager_setflat(P2(gs_imager_state *, floatp));
bool gs_imager_currentdashadapt(P1(const gs_imager_state *));
bool gs_imager_currentaccuratecurves(P1(const gs_imager_state *));

#endif /* gsline_INCLUDED */

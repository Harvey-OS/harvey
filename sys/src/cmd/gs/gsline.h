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

/* gsline.h */
/* Line parameter and quality definitions */

#ifndef gsline_INCLUDED
#  define gsline_INCLUDED

/* Line cap values */
typedef enum {
	gs_cap_butt = 0,
	gs_cap_round = 1,
	gs_cap_square = 2
} gs_line_cap;

/* Line join values */
typedef enum {
	gs_join_miter = 0,
	gs_join_round = 1,
	gs_join_bevel = 2
} gs_line_join;

/* Procedures */
int	gs_setlinewidth(P2(gs_state *, floatp));
float	gs_currentlinewidth(P1(const gs_state *));
int	gs_setlinecap(P2(gs_state *, gs_line_cap));
gs_line_cap
	gs_currentlinecap(P1(const gs_state *));
int	gs_setlinejoin(P2(gs_state *, gs_line_join));
gs_line_join
	gs_currentlinejoin(P1(const gs_state *));
int	gs_setmiterlimit(P2(gs_state *, floatp));
float	gs_currentmiterlimit(P1(const gs_state *));
int	gs_setdash(P4(gs_state *, const float *, uint, floatp));
uint	gs_currentdash_length(P1(const gs_state *));
const float *
	gs_currentdash_pattern(P1(const gs_state *));
float	gs_currentdash_offset(P1(const gs_state *));
int	gs_setflat(P2(gs_state *, floatp));
float	gs_currentflat(P1(const gs_state *));
int	gs_setstrokeadjust(P2(gs_state *, bool));
bool	gs_currentstrokeadjust(P1(const gs_state *));

#endif					/* gsline_INCLUDED */

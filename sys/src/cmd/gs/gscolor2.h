/* Copyright (C) 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* gscolor2.h */
/* Client interface to Level 2 color facilities */
/* (requires gscspace.h, gsmatrix.h) */
#include "gsccolor.h"
#include "gsuid.h"		/* for pattern template */

/* Note: clients should use rc_alloc_struct_0 (in gsrefct.h) to allocate */
/* CIE color spaces or rendering structures; makepattern uses */
/* rc_alloc_struct_1 to allocate pattern instances. */

/* General color routines */
const gs_color_space *gs_currentcolorspace(P1(const gs_state *));
int	gs_setcolorspace(P2(gs_state *, gs_color_space *));
const gs_client_color *gs_currentcolor(P1(const gs_state *));
int	gs_setcolor(P2(gs_state *, const gs_client_color *));
bool	gs_currentoverprint(P1(const gs_state *));
void	gs_setoverprint(P2(gs_state *, bool));

/* CIE-specific routines */
#ifndef gs_cie_render_DEFINED
#  define gs_cie_render_DEFINED
typedef struct gs_cie_render_s gs_cie_render;
#endif
const gs_cie_render *gs_currentcolorrendering(P1(const gs_state *));
int	gs_setcolorrendering(P2(gs_state *, gs_cie_render *));

/* Pattern template */
typedef struct gs_client_pattern_s {
	gs_uid uid;		/* XUID or nothing */
	int PaintType;
	int TilingType;
	gs_rect BBox;
	float XStep;
	float YStep;
	int (*PaintProc)(P2(const gs_client_color *, gs_state *));
	void *client_data;		/* additional client data */
} gs_client_pattern;
#define private_st_client_pattern() /* in gspcolor.c */\
  gs_private_st_ptrs1(st_client_pattern, gs_client_pattern,\
    "client pattern", client_pattern_enum_ptrs, client_pattern_reloc_ptrs,\
    client_data)
#define st_client_pattern_max_ptrs 1

/* Pattern-specific routines */
int	gs_makepattern(P4(gs_client_color *, const gs_client_pattern *, const gs_matrix *, gs_state *));
int	gs_setpattern(P2(gs_state *, const gs_client_color *));
int	gs_setpatternspace(P1(gs_state *));
const gs_client_pattern	*gs_getpattern(P1(const gs_client_color *));

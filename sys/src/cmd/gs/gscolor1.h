/* Copyright (C) 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* gscolor1.h */
/* Client interface to Level 1 extended color facilities */
/* Requires gscolor.h */

#ifndef gscolor1_INCLUDED
#  define gscolor1_INCLUDED

/* Color and gray interface */
int	gs_setcmykcolor(P5(gs_state *, floatp, floatp, floatp, floatp)),
	gs_currentcmykcolor(P2(const gs_state *, float [4])),
	gs_setblackgeneration(P2(gs_state *, gs_mapping_proc)),
	gs_setblackgeneration_remap(P3(gs_state *, gs_mapping_proc, bool));
gs_mapping_proc	gs_currentblackgeneration(P1(const gs_state *));
int	gs_setundercolorremoval(P2(gs_state *, gs_mapping_proc)),
	gs_setundercolorremoval_remap(P3(gs_state *, gs_mapping_proc, bool));
gs_mapping_proc	gs_currentundercolorremoval(P1(const gs_state *));
/* Transfer function */
int	gs_setcolortransfer(P5(gs_state *, gs_mapping_proc /*red*/,
			gs_mapping_proc	/*green*/, gs_mapping_proc /*blue*/,
			gs_mapping_proc	/*gray*/)),
	gs_setcolortransfer_remap(P6(gs_state *, gs_mapping_proc /*red*/,
			gs_mapping_proc	/*green*/, gs_mapping_proc /*blue*/,
			gs_mapping_proc	/*gray*/, bool));
void	gs_currentcolortransfer(P2(const gs_state *, gs_mapping_proc [4]));

#endif					/* gscolor1_INCLUDED */

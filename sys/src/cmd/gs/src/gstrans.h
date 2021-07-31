/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gstrans.h,v 1.6 2000/09/19 19:00:32 lpd Exp $ */
/* Transparency definitions and interface */

#ifndef gstrans_INCLUDED
#  define gstrans_INCLUDED

#include "gstparam.h"

/* Access transparency-related graphics state elements. */
int gs_setblendmode(P2(gs_state *, gs_blend_mode_t));
gs_blend_mode_t gs_currentblendmode(P1(const gs_state *));
int gs_setopacityalpha(P2(gs_state *, floatp));
float gs_currentopacityalpha(P1(const gs_state *));
int gs_setshapealpha(P2(gs_state *, floatp));
float gs_currentshapealpha(P1(const gs_state *));
int gs_settextknockout(P2(gs_state *, bool));
bool gs_currenttextknockout(P1(const gs_state *));

/*
 * Manage transparency group and mask rendering.  Eventually these will be
 * driver procedures, taking dev + pis instead of pgs.
 */

gs_transparency_state_type_t
    gs_current_transparency_type(P1(const gs_state *pgs));

/*
 * We have to abbreviate the procedure name because procedure names are
 * only unique to 23 characters on VMS.
 */
void gs_trans_group_params_init(P1(gs_transparency_group_params_t *ptgp));

int gs_begin_transparency_group(P3(gs_state *pgs,
				   const gs_transparency_group_params_t *ptgp,
				   const gs_rect *pbbox));

int gs_end_transparency_group(P1(gs_state *pgs));

void gs_trans_mask_params_init(P2(gs_transparency_mask_params_t *ptmp,
				  gs_transparency_mask_subtype_t subtype));

int gs_begin_transparency_mask(P3(gs_state *pgs,
				  const gs_transparency_mask_params_t *ptmp,
				  const gs_rect *pbbox));

int gs_end_transparency_mask(P2(gs_state *pgs,
				gs_transparency_channel_selector_t csel));

int gs_init_transparency_mask(P2(gs_state *pgs,
				 gs_transparency_channel_selector_t csel));

int gs_discard_transparency_layer(P1(gs_state *pgs));

#endif /* gstrans_INCLUDED */

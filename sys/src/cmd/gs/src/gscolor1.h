/* Copyright (C) 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gscolor1.h,v 1.5 2002/06/16 08:45:42 lpd Exp $ */
/* Client interface to Level 1 extended color facilities */
/* Requires gscolor.h */

#ifndef gscolor1_INCLUDED
#  define gscolor1_INCLUDED

/* Color and gray interface */
int gs_setcmykcolor(gs_state *, floatp, floatp, floatp, floatp),
    gs_currentcmykcolor(const gs_state *, float[4]),
    gs_setblackgeneration(gs_state *, gs_mapping_proc),
    gs_setblackgeneration_remap(gs_state *, gs_mapping_proc, bool);
gs_mapping_proc gs_currentblackgeneration(const gs_state *);
int gs_setundercolorremoval(gs_state *, gs_mapping_proc),
    gs_setundercolorremoval_remap(gs_state *, gs_mapping_proc, bool);
gs_mapping_proc gs_currentundercolorremoval(const gs_state *);

/* Transfer function */
int gs_setcolortransfer(gs_state *, gs_mapping_proc /*red */ ,
			gs_mapping_proc /*green */ ,
			gs_mapping_proc /*blue */ ,
			gs_mapping_proc /*gray */ ),
    gs_setcolortransfer_remap(gs_state *, gs_mapping_proc /*red */ ,
			      gs_mapping_proc /*green */ ,
			      gs_mapping_proc /*blue */ ,
			      gs_mapping_proc /*gray */ , bool);
void gs_currentcolortransfer(const gs_state *, gs_mapping_proc[4]);

#endif /* gscolor1_INCLUDED */

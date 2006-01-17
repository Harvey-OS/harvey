/* Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: icharout.h,v 1.10 2004/09/22 13:52:33 igor Exp $ */
/* Interface to zcharout.c */

#ifndef icharout_INCLUDED
#  define icharout_INCLUDED

/* Execute an outline defined by a PostScript procedure. */
int zchar_exec_char_proc(i_ctx_t *);

/*
 * Get the metrics for a character from the Metrics dictionary of a base
 * font.  If present, store the l.s.b. in psbw[0,1] and the width in
 * psbw[2,3].
 */
typedef enum {
    metricsNone = 0,
    metricsWidthOnly = 1,
    metricsSideBearingAndWidth = 2
} metrics_present;
int /*metrics_present*/
  zchar_get_metrics(const gs_font_base * pbfont, const ref * pcnref,
		    double psbw[4]);

/* Get the vertical metrics for a character from Metrics2, if present. */
int /*metrics_present*/
  zchar_get_metrics2(const gs_font_base * pbfont, const ref * pcnref,
		     double pwv[4]);
/*  Get CDevProc. */
bool zchar_get_CDevProc(const gs_font_base * pbfont, ref **ppcdevproc);

/*
 * Consult Metrics2 and CDevProc, and call setcachedevice[2].  Return
 * o_push_estack if we had to call a CDevProc, or if we are skipping the
 * rendering process (only getting the metrics).
 */
int zchar_set_cache(i_ctx_t *i_ctx_p, const gs_font_base * pbfont,
		    const ref * pcnref, const double psb[2],
		    const double pwidth[2], const gs_rect * pbbox,
    		    op_proc_t cont, op_proc_t *exec_cont,
		    const double Metrics2_sbw_default[4]);

/*
 * Get the CharString data corresponding to a glyph.  Return typecheck
 * if it isn't a string.
 */
int zchar_charstring_data(gs_font *font, const ref *pgref,
			  gs_glyph_data_t *pgd);

/*
 * Enumerate the next glyph from a directory.  This is essentially a
 * wrapper around dict_first/dict_next to implement the enumerate_glyph
 * font procedure.
 */
int zchar_enumerate_glyph(const gs_memory_t *mem, const ref *prdict, int *pindex, gs_glyph *pglyph);

#endif /* icharout_INCLUDED */

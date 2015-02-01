/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Copyright (C) 2002 artofcode LLC.  All rights reserved.
 * See toolbin/encs2c.ps for the complete license notice.
 *
 * $Id: gdevpdtv.h,v 1.1 2003/07/31 20:14:37 alexcher Exp $
 *
 * This file contains substantial parts of toolbin/encs2c.ps,
 * which generated the remainder of the file mechanically from
 *   gs_std_e.ps  gs_il1_e.ps  gs_sym_e.ps  gs_dbt_e.ps
 *   gs_wan_e.ps  gs_mro_e.ps  gs_mex_e.ps  gs_mgl_e.ps
 *   gs_lgo_e.ps  gs_lgx_e.ps  gs_css_e.ps
 */

#ifndef gdevpdtv_INCLUDED
#define gdevpdtv_INCLUDED

#define GS_C_PDF_MAX_GOOD_GLYPH 21894
#define GS_C_PDF_GOOD_GLYPH_MASK 1
#define GS_C_PDF_GOOD_NON_SYMBOL_MASK 2

extern const unsigned char gs_c_pdf_glyph_type[];

#endif /* gdevpdtv_INCLUDED */

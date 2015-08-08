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
 * $Id: gdevpdtv.c,v 1.1 2003/07/31 20:14:37 alexcher Exp $
 *
 * This file contains substantial parts of toolbin/encs2c.ps,
 * which generated the remainder of the file mechanically from
 *   gs_std_e.ps  gs_il1_e.ps  gs_sym_e.ps  gs_dbt_e.ps
 *   gs_wan_e.ps  gs_mro_e.ps  gs_mex_e.ps  gs_mgl_e.ps
 *   gs_lgo_e.ps  gs_lgx_e.ps  gs_css_e.ps
 */

/*
 * Glyph attributes for every glyph <= GS_C_PDF_MAX_GOOD_GLYPH
 * packed 4 per byte least significant bits first.
 */
const unsigned char gs_c_pdf_glyph_type[] = {
    124, 241, 255, 255, 255, 0, 0, 0, 12,  0,   0,   0,   0,  0, 0, 0,
    12,  0,   0,   0,   0,   0, 0, 0, 76,  0,   0,   0,   0,  0, 0, 0,
    28,  3,   0,   0,   0,   0, 0, 0, 12,  4,   0,   0,   0,  0, 0, 0,
    220, 0,   0,   0,   0,   0, 0, 0, 12,  192, 0,   0,   0,  0, 0, 0,
    60,  1,   3,   0,   0,   0, 0, 0, 12,  0,   12,  0,   0,  0, 0, 0,
    28,  12,  48,  0,   0,   0, 0, 0, 12,  0,   192, 0,   0,  0, 0, 0,
    92,  49,  0,   0,   0,   0, 0, 0, 12,  0,   0,   12,  0,  0, 0, 0,
    12,  0,   0,   48,  0,   0, 0, 0, 76,  4,   0,   192, 0,  0, 0, 0,
    12,  0,   1,   0,   3,   0, 0, 0, 12,  0,   0,   0,   12, 0, 0, 0,
    76,  48,  12,  0,   48,  0, 0, 0, 12,  0,   0,   0,   0,  0, 0, 0,
    12,  4,   48,  0,   0,   0, 0, 0, 76,  0,   0,   0,   0,  0, 0, 0,
    12,  0,   192, 0,   0,   0, 0, 0, 12,  0,   0,   0,   0,  0, 0, 0,
    12,  48,  1,   0,   0,   0, 0, 0, 12,  4,   0,   0,   0,  0, 0, 0,
    12,  0,   0,   0,   0,   0, 0, 0, 12,  0,   12,  0,   0,  0, 0, 0,
    12,  0,   0,   48,  0,   0, 0, 0, 12,  0,   0,   0,   0,  0, 0, 0,
    12,  52,  48,  0,   0,   0, 0, 0, 12,  0,   0,   0,   0,  0, 0, 0,
    60,  0,   0,   0,   3,   0, 0, 0, 12,  0,   192, 0,   0,  0, 0, 0,
    60,  0,   0,   0,   12,  0, 0, 0, 12,  4,   0,   0,   0,  0, 0, 0,
    60,  0,   12,  0,   0,   0, 0, 0, 12,  0,   0,   0,   0,  0, 0, 0,
    60,  0,   0,   0,   0,   0, 0, 0, 12,  0,   0,   0,   0,  0, 0, 0,
    60,  4,   1,   0,   0,   0, 0, 0, 12,  0,   0,   0,   0,  0, 0, 0,
    60,  64,  0,   48,  0,   0, 0, 0, 12,  0,   0,   0,   0,  0, 0, 0,
    28,  0,   192, 0,   0,   0, 0, 0, 12,  12,  12,  0,   0,  0, 0, 0,
    60,  0,   0,   0,   0,   0, 0, 0, 12,  0,   0,   0,   0,  0, 0, 0,
    28,  48,  3,   0,   3,   0, 0, 0, 12,  0,   0,   0,   0,  0, 0, 0,
    28,  0,   48,  0,   0,   0, 0, 0, 12,  0,   0,   0,   12, 0, 0, 0,
    0,   0,   0,   4,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   12,  0,   0,   0, 0, 0, 0,   12,  192, 0,   0,  0, 0, 0,
    0,   0,   3,   48,  0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   4,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   192, 12,  0,   0,  0, 0, 0,
    0,   0,   3,   0,   3,   0, 0, 0, 0,   4,   0,   4,   0,  0, 0, 0,
    0,   48,  192, 0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   4,   48,  48,  0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  15,  0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   12,  0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   64,  192, 0,   0,  0, 0, 0,
    0,   0,   0,   4,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   12,  51,  0,   0,   0, 0, 0, 0,   0,   4,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   48,  0,   0, 0, 0, 0,   12,  0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   195, 0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   60,  0,   0,   0,   0, 0, 0, 0,   0,   0,   4,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   12,  0,   0,   0,  0, 0, 0,
    0,   48,  1,   1,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   64,  0,   48,  0,   0, 0, 0, 0,   0,   196, 0,   0,  0, 0, 0,
    0,   4,   16,  0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   3,   4,   0,   0, 0, 0, 0,   12,  0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  4,   1,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   12,  208, 0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   1,   16,  0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   4,   12,  0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   64,  0,   0,   0,  0, 0, 0,
    0,   52,  17,  1,   0,   0, 0, 0, 0,   0,   192, 0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   12,  0,   0,   0,  0, 0, 0,
    0,   112, 12,  16,  0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   1,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   4,   48,  12,  0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  192, 1,   0,   0, 0, 0, 0,   192, 0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   4,   12,  0,   0,  0, 0, 0,
    0,   0,   3,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   68,  48,  16,  0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   192, 12,  0,  0, 0, 0,
    0,   48,  13,  1,   0,   0, 0, 0, 0,   4,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   12,  16,  0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   3,   0,   0,   0, 0, 0, 0,   0,   12,  0,   0,  0, 0, 0,
    0,   64,  192, 16,  0,   0, 0, 0, 0,   4,   0,   0,   0,  0, 0, 0,
    0,   16,  0,   5,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   12,  51,  0,   0,   0, 0, 0, 0,   64,  0,   0,   0,  0, 0, 0,
    0,   0,   12,  0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   12,  192, 0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  3,   17,  0,   0, 0, 0, 0,   0,   0,   12,  0,  0, 0, 0,
    0,   4,   0,   0,   0,   0, 0, 0, 0,   0,   4,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   0,   0,   0, 0, 0, 0,   4,   0,   0,   0,  0, 0, 0,
    0,   0,   192, 0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   60,  60,  1,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   64,  0,   28,  0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   3,   0,   0,   0, 0, 0, 0,   12,  0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   192, 0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   192, 12,  0,   0,  0, 0, 0,
    0,   12,  48,  0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   1,   3,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   4,   0,   12,  0,  0, 0, 0,
    0,   0,   0,   48,  0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  204, 0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   48,  0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   3,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   12,  0,   0,  0, 0, 0,
    0,   0,   0,   12,  0,   0, 0, 0, 0,   0,   192, 0,   0,  0, 0, 0,
    0,   112, 16,  48,  0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  12,  3,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   208, 0,   0,   0, 0, 0, 0,   0,   0,   12,  0,  0, 0, 0,
    0,   48,  0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   48,  0,   0, 0, 0, 0,   0,   12,  0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   3,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   16,  0,   0,   0, 0, 0, 0,   64,  192, 0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  12,  0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   192, 0,   48,  0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  16,  3,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   192, 0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   192, 0,   0,   0,  0, 0, 0,
    0,   48,  0,   0,   0,   0, 0, 0, 0,   0,   0,   4,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   48,  0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   64,  12,  51,  0,   0, 0, 0, 0,   0,   192, 0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 192, 0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    64,  0,   0,   0,   0,   0, 0, 0, 0,   64,  0,   0,   0,  0, 0, 0,
    0,   0,   16,  12,  0,   0, 0, 0, 64,  0,   4,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    192, 48,  192, 3,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   192, 0,   48,  0,   0, 0, 0, 192, 0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    192, 0,   52,  0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   64,  0,   12,  0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   192, 0,   0,  0, 0, 0,
    192, 48,  0,   3,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 64,  0,   12,  0,   0,  0, 0, 0,
    0,   0,   48,  48,  0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    64,  48,  0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 64,  0,   0,   0,   0,  0, 0, 0,
    0,   0,   64,  4,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    192, 0,   12,  3,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   48,  0,   0,   0, 0, 0, 64,  0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    192, 112, 0,   48,  0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 192, 0,   68,  0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   12,  0,  0, 0, 0,
    0,   48,  48,  3,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  12,  0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   192, 48,  0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   48,  0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   12,  0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   12,  0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   0,   0,   0, 0, 0, 0,   0,   64,  0,   0,  0, 0, 0,
    0,   0,   48,  0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   48,  0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  12,  1,   0,   0, 0, 0, 0,   0,   0,   12,  0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  64,  0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   12,  0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   51,  0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   12,  0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   192, 0,   0,  0, 0, 0,
    0,   48,  12,  0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   1,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   16,  0,   0, 0, 0, 0,   0,   0,   12,  0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  12,  0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   192, 0,   0,  0, 0, 0,
    0,   0,   0,   60,  0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   12,  0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   64,  0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   12,  0,  0, 0, 0,
    0,   48,  12,  16,  0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   0,   0,   0, 0, 0, 0,   0,   192, 0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   12,  0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   4,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   16,  0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   68,  3,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   12,  0,   0,  0, 0, 0,
    0,   0,   0,   48,  0,   0, 0, 0, 0,   0,   192, 0,   0,  0, 0, 0,
    0,   48,  0,   3,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   12,  0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   192, 0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   1,   0,   3,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   12,  0,   0,  0, 0, 0,
    0,   3,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   1,   0,   0,   0,   0, 0, 0, 0,   0,   192, 0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   51,  4,   3,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   3,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   1,   192, 0,   0,   0, 0, 0, 0,   0,   4,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   51,  0,   1,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   3,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   12,  0,   0,   0, 0, 0, 0,   0,   192, 0,   0,  0, 0, 0,
    0,   1,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   51,  0,   1,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   12,  0,   0,  0, 0, 0,
    0,   3,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  192, 0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   3,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   49,  12,  1,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   192, 0,   0,  0, 0, 0,
    0,   48,  0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   12,  0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   1,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   64,  0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   4,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   1,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   76,  0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   3,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   64,  0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   16,  0,   3,   0,   0, 0, 0, 0,   0,   64,  0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  192, 3,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   192, 0,   0,  0, 0, 0,
    0,   16,  0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   192, 0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   1,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   192, 0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   1,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   192, 0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   16,  0,   0,   0,   0, 0, 0, 0,   0,   192, 0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   3,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  192, 0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   192, 0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   3,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   192, 0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   3,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   64,  0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   16,  0,   3,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   16,  0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   3,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   16,  0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48,  0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   0,   0,   0,   0,   0, 0, 0, 0,   0,   0,   0,   0,  0, 0, 0,
    0,   48};

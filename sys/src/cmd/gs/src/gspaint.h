/* Copyright (C) 1989, 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gspaint.h,v 1.5 2002/06/16 08:45:42 lpd Exp $ */
/* Erase/fill/stroke procedures */
/* Requires gsstate.h */

#ifndef gspaint_INCLUDED
#  define gspaint_INCLUDED

/* Painting */
int gs_erasepage(gs_state *),
    gs_fillpage(gs_state *),
    gs_fill(gs_state *),
    gs_eofill(gs_state *),
    gs_stroke(gs_state *);

/* Image tracing */
int gs_imagepath(gs_state *, int, int, const byte *);

#endif /* gspaint_INCLUDED */

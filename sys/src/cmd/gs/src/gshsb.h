/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gshsb.h,v 1.5 2002/06/16 08:45:42 lpd Exp $ */
/* Client interface to HSB color routines */

#ifndef gshsb_INCLUDED
#  define gshsb_INCLUDED

int gs_sethsbcolor(gs_state *, floatp, floatp, floatp),
    gs_currenthsbcolor(const gs_state *, float[3]);

#endif /* gshsb_INCLUDED */

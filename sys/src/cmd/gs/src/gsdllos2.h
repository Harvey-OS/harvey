/* Copyright (C) 1994-1996, Russell Lang.  All rights reserved.
  
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

/* $Id: gsdllos2.h,v 1.4 2002/02/21 22:24:52 giles Exp $ */
/* gsdll extension for OS/2 platforms */

#ifndef gsdllos2_INCLUDED
#  define gsdllos2_INCLUDED

/* DLL exported functions */
/* for load time dynamic linking */
unsigned long gsdll_get_bitmap(unsigned char *device, unsigned char **pbitmap);

/* Function pointer typedefs */
/* for run time dynamic linking */
typedef long (*GSDLLAPI PFN_gsdll_get_bitmap) (unsigned char *, unsigned char **);

#endif /* gsdllos2_INCLUDED */

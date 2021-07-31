/* Copyright (C) 1994-1996, Russell Lang.  All rights reserved.

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

/*$Id: gsdllos2.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
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

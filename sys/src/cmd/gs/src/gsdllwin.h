/* Copyright (C) 1994-2001, Ghostgum Software Pty Ltd.  All rights reserved.
  
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

/*$Id: gsdllwin.h,v 1.3 2001/03/13 07:09:29 ghostgum Exp $ */
/* gsdll extension for Microsoft Windows platforms */

#ifndef gsdllwin_INCLUDED
#  define gsdllwin_INCLUDED

/* DLL exported functions */
/* for load time dynamic linking */
GSDLLEXPORT HGLOBAL GSDLLAPI gsdll_copy_dib(unsigned char * device);
GSDLLEXPORT HPALETTE GSDLLAPI gsdll_copy_palette(unsigned char * device);
GSDLLEXPORT void GSDLLAPI gsdll_draw(unsigned char * device, HDC hdc, LPRECT dest,
			 LPRECT src);
GSDLLEXPORT int GSDLLAPI gsdll_get_bitmap_row(unsigned char *device,
				  LPBITMAPINFOHEADER pbmih,
				  LPRGBQUAD prgbquad, LPBYTE * ppbyte,
				  unsigned int row);

/* Function pointer typedefs */
/* for run time dynamic linking */
typedef HGLOBAL (GSDLLAPI * PFN_gsdll_copy_dib)(unsigned char *);
typedef HPALETTE (GSDLLAPI * PFN_gsdll_copy_palette)(unsigned char *);
typedef void (GSDLLAPI * PFN_gsdll_draw) (unsigned char *, HDC, LPRECT,
					  LPRECT);
typedef int (GSDLLAPI * PFN_gsdll_get_bitmap_row)
     (unsigned char *device, LPBITMAPINFOHEADER pbmih, LPRGBQUAD prgbquad,
      LPBYTE * ppbyte, unsigned int row);

#endif /* gsdllwin_INCLUDED */

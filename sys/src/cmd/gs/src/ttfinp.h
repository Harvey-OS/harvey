/* Copyright (C) 2003 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: ttfinp.h,v 1.1 2003/10/01 13:44:56 igor Exp $ */
/* A TT font input support. */

#ifndef incl_ttfinp
#define incl_ttfinp

unsigned char  ttfReader__Byte(ttfReader *r);
signed   char  ttfReader__SignedByte(ttfReader *r);
unsigned short ttfReader__UShort(ttfReader *r);
unsigned int   ttfReader__UInt(ttfReader *r);
signed   short ttfReader__Short(ttfReader *r);
signed   int   ttfReader__Int(ttfReader *r);

#endif

/* Copyright (C) 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevmeds.h,v 1.5 2002/06/16 07:25:26 lpd Exp $ */
/* Interface for gdevmeds.c */

#ifndef gdevmeds_INCLUDED
#  define gdevmeds_INCLUDED

#include "gdevprn.h"

int select_medium(gx_device_printer *pdev, const char **available,
		  int default_index);

#endif /* gdevmeds_INCLUDED */

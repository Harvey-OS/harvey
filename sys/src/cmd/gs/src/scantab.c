/* Copyright (C) 1994, 1995, 1997 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: scantab.c,v 1.5 2004/09/06 07:26:41 ray Exp $ */
/* Scanner table for PostScript/PDF tokens */
#include "stdpre.h"
#include "scommon.h"
#include "scanchar.h"		/* defines interface */

/* Define the character scanning table (see scanchar.h). */
const byte scan_char_array[max_stream_exception + 256] =
{stream_exception_repeat(ctype_exception),
		/* Control characters 0-31. */
 ctype_space,			/* NULL - standard only in Level 2 */
 ctype_name, ctype_name, ctype_name,
 ctype_other,                 /* EOT == ctrl-d <04> */
 ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_space,			/* TAB (\t) */
 ctype_space,			/* LF (\n) */
 ctype_name,
 ctype_space,			/* FF (\f) */
 ctype_space,			/* CR (\r) */
 ctype_name, ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_name, ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_name, ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_name, ctype_name, ctype_name,
		/* Printable characters 32-63 */
 ctype_space,			/* space (\s) */
 ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_other,			/* % */
 ctype_name, ctype_name,
 ctype_other,			/* ( */
 ctype_other,			/* ) */
 ctype_name, ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_other,			/* / */
 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,	/* digits 0-9 */
 ctype_name, ctype_name,
 ctype_other,			/* < */
 ctype_name,
 ctype_other,			/* > */
 ctype_name,
		/* Printable characters 64-95 */
 ctype_name,
 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
 30, 31, 32, 33, 34, 35,
 ctype_other,			/* [ */
 ctype_name,
 ctype_other,			/* ] */
 ctype_name, ctype_name,
		/* Printable characters 96-126 and DEL */
 ctype_name,
 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
 30, 31, 32, 33, 34, 35,
 ctype_other,			/* { */
 ctype_name,
 ctype_other,			/* } */
 ctype_name, ctype_name,
		/* Characters 128-159, binary tokens */
 ctype_btoken, ctype_btoken, ctype_btoken, ctype_btoken, ctype_btoken,
 ctype_btoken, ctype_btoken, ctype_btoken, ctype_btoken, ctype_btoken,
 ctype_btoken, ctype_btoken, ctype_btoken, ctype_btoken, ctype_btoken,
 ctype_btoken, ctype_btoken, ctype_btoken, ctype_btoken, ctype_btoken,
 ctype_btoken, ctype_btoken, ctype_btoken, ctype_btoken, ctype_btoken,
 ctype_btoken, ctype_btoken, ctype_btoken, ctype_btoken, ctype_btoken,
 ctype_btoken, ctype_btoken,
		/* Characters 160-191, not defined */
 ctype_name, ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_name, ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_name, ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_name, ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_name, ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_name, ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_name, ctype_name,
		/* Characters 192-223, not defined */
 ctype_name, ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_name, ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_name, ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_name, ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_name, ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_name, ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_name, ctype_name,
		/* Characters 224-255, not defined */
 ctype_name, ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_name, ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_name, ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_name, ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_name, ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_name, ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_name, ctype_name
};

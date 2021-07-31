/* Copyright (C) 1994, 1995, 1997 Aladdin Enterprises.  All rights reserved.

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

/*$Id: scantab.c,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* Scanner table for PostScript/PDF tokens */
#include "stdpre.h"
#include "scommon.h"
#include "scanchar.h"		/* defines interface */

/* Define the character scanning table (see scanchar.h). */
const byte scan_char_array[max_stream_exception + 256] =
{stream_exception_repeat(ctype_exception),
		/* Control characters 0-31. */
 ctype_space,			/* NULL - standard only in Level 2 */
 ctype_name, ctype_name, ctype_name, ctype_name, ctype_name,
 ctype_name, ctype_name, ctype_name,
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

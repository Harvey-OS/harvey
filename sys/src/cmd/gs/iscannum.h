/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* iscannum.h */
/* Interface to Ghostscript number scanner */

/* Scan a number for cvi or cvr. */
/* The first argument is a t_string. */
int scan_number_only(P2(const ref *psref, ref *pref));

/* Scan a number.  If the number consumes the entire string, return 0; */
/* if not, set *psp to the first character beyond the number and return 1. */
int scan_number(P5(const byte *sp, const byte *end, int sign, ref *pref,
  const byte **psp));

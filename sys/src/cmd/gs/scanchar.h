/* Copyright (C) 1990, 1991, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* scanchar.h */
/* Character scanning table for Ghostscript */
/* Requires scommon.h */

/* An array for fast scanning of names, numbers, and hex strings. */
/*  Indexed by character code (including exceptions), it contains: */
/*	0 - max_radix-1 for valid digits, */
/*	ctype_name for other characters valid in names, */
/*	ctype_btoken for characters introducing binary tokens */
/*	  (if the binary token feature is enabled), */
/*	ctype_space for whitespace characters, */
/*	ctype_exception for exceptions (see scommon.h), and */
/*	ctype_other for everything else. */
/* This table is defined in iscan.c, used in iscan.c and stream.c. */
/* NOTE: If any of the values below change, you must edit the table. */
/* Exceptions are negative values; we bias the table origin accordingly. */
extern const byte scan_char_array[max_stream_exception + 256];
#define scan_char_decoder (&scan_char_array[max_stream_exception])
#define min_radix 2
#define max_radix 36
#define ctype_name 100
#define ctype_btoken 101
#define ctype_space 102
#define ctype_other 103
#define ctype_exception 104
/* Special characters with no \xxx representation */
#define char_NULL 0
#define char_VT 013			/* ^K, vertical tab */
#define char_DOS_EOF 032		/* ^Z */
/*
 * Most systems define '\n' as 0x0a and '\r' as 0x0d; however, OS-9
 * has '\n' = '\r' = 0x0d and '\l' = 0x0a.  To deal with this,
 * we introduce abstract characters char_CR and char_EOL such that
 * any of [char_CR], [char_CR char_EOL], or [char_EOL] is recognized
 * as an end-of-line sequence.
 */
#define char_CR '\r'
#if '\r' == '\n'
#  define char_EOL '\l'
#else
#  define char_EOL '\n'
#endif

/* Copyright (C) 1990, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* bseq.h */
/* Definitions for Level 2 binary object sequences */

/* Binary object sequence element types */
typedef enum {
  bs_null = 0,
  bs_integer = 1,
  bs_real = 2,
  bs_name = 3,
  bs_boolean = 4,
  bs_string = 5,
  bs_eval_name = 6,
  bs_array = 9,
  bs_mark = 10,
	/*
	 * We extend the PostScript language definition by allowing
	 * dictionaries in binary object sequences.  The data for
	 * a dictionary is like that for an array, with the following
	 * changes:
	 *	- If the size is an even number, the value is the index of
	 * the first of a series of alternating keys and values.
	 *	- If the size is 1, the value is the index of another
	 * object (which must also be a dictionary, and must not have
	 * size = 1); this object represents the same object as that one.
	 */
  bs_dictionary = 15
} bin_seq_type;
#define bs_executable 128

/* Definition of an object in a binary object sequence. */
typedef struct {
  byte tx;			/* type and executable flag */
  byte unused;
  union {
    bits16 w;
    byte b[2];
  } size;
  union {
    bits32 w;
    float f;
    byte b[4];
  } value;
} bin_seq_obj;

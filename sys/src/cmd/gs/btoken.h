/* Copyright (C) 1990 Aladdin Enterprises.  All rights reserved.
  
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

/* btoken.h */
/* Definitions for Level 2 binary tokens */

/* Binary token types */
typedef enum {
  bt_seq = 128,
    bt_seq_IEEE_msb = 128,	/* binary object sequence, */
				/* IEEE floats, big-endian */
    bt_seq_IEEE_lsb = 129,	/* ditto, little-endian */
    bt_seq_native_msb = 130,	/* ditto, native floats, big-endian */
    bt_seq_native_lsb = 131,	/* ditto, little-endian */
  bt_int32_msb = 132,
  bt_int32_lsb = 133,
  bt_int16_msb = 134,
  bt_int16_lsb = 135,
  bt_int8 = 136,
  bt_fixed = 137,
  bt_float_IEEE_msb = 138,
  bt_float_IEEE_lsb = 139,
  bt_float_native = 140,
  bt_boolean = 141,
  bt_string_256 = 142,
  bt_string_64k_msb = 143,
  bt_string_64k_lsb = 144,
  bt_litname_system = 145,
  bt_execname_system = 146,
  bt_litname_user = 147,
  bt_execname_user = 148,
  bt_num_array = 149
} bt_char;
#define bt_char_min 128
#define bt_char_max 159

/* Define the number of required initial bytes for binary tokens */
/* (including the token type byte). */
extern const byte bin_token_bytes[];	/* in iscan2.c */
#define bin_token_bytes_values\
  4, 4, 4, 4, 5, 5, 3, 3, 2, 2, 5, 5, 5,\
  2, 2, 3, 3, 2, 2, 2, 2, 4,\
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1	/* undefined */
#define binary_token_bytes(btchar)\
  (bin_token_bytes[(btchar) - bt_char_min])

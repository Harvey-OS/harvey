/* Copyright (C) 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* scf.h */
/* Common definitions for CCITTFax encoding and decoding filters */
#include "shc.h"

/*
 * The CCITT Group 3 (T.4) and Group 4 (T.6) fax specifications map
 * run lengths to Huffman codes.  White and black have different mappings.
 * If the run length is 64 or greater, two or more codes are needed:
 *	- One or more 'make-up' codes for 2560 pixels;
 *	- A 'make-up' code that encodes the multiple of 64;
 *	- A 'termination' code for the remainder.
 * For runs of 63 or less, only the 'termination' code is needed.
 */

/* ------ Encoding tables ------ */

/* Define the maximum length of a scan line that we can encode. */
/* The natural values for this are 2560 * N + 63. */
#define cfe_max_width (2560 * 2 + 63)
#define cfe_max_makeups (cfe_max_width / 2560)
#define cfe_max_code_bytes (cfe_max_makeups * 2 + 2) /* conservative */

typedef hce_code cfe_run;
#define cfe_entry(c, len) hce_entry(c, len)

/* Codes common to 1-D and 2-D encoding. */
/* The decoding algorithms know that EOL is 0....01. */
#define run_eol_code_length 12
#define run_eol_code_value 1
extern const cfe_run cf_run_eol;
extern const cfe_run far_data cf_white_termination[64];
extern const cfe_run far_data cf_white_make_up[41];
extern const cfe_run far_data cf_black_termination[64];
extern const cfe_run far_data cf_black_make_up[41];
extern const cfe_run cf_uncompressed[6];
extern const cfe_run cf_uncompressed_exit[10];	/* indexed by 2 x length of */
			/* white run + (1 if next run black, 0 if white) */
/* 1-D encoding. */
extern const cfe_run cf1_run_uncompressed;
/* 2-D encoding. */
extern const cfe_run cf2_run_pass;
#define cf2_run_vertical_offset 3
extern const cfe_run cf2_run_vertical[7];	/* indexed by b1 - a1 + offset */
extern const cfe_run cf2_run_horizontal;
#define cf2_run_horizontal_length 3
extern const cfe_run cf2_run_uncompressed;
/* 2-D Group 3 encoding. */
extern const cfe_run cf2_run_eol_1d;
extern const cfe_run cf2_run_eol_2d;

/* ------ Decoding tables ------ */

typedef hcd_code cfd_node;
#define run_length value

/*
 * The value in the decoding tables is either a white or black run length,
 * or a (negative) exceptional value.
 */
#define run_error (-1)
#define run_zeros (-2)	/* EOL follows, possibly with more padding first */
#define run_uncompressed (-3)
/* 2-D codes */
#define run2_pass (-4)
#define run2_horizontal (-5)

#define cfd_white_initial_bits 8
extern const cfd_node far_data cf_white_decode[];
#define cfd_black_initial_bits 7
extern const cfd_node far_data cf_black_decode[];
#define cfd_2d_initial_bits 7
extern const cfd_node far_data cf_2d_decode[];
#define cfd_uncompressed_initial_bits 6		/* must be 6 */
extern const cfd_node far_data cf_uncompressed_decode[];

/* ------ Run detection macros ------ */

/*
 * For the run detection macros:
 *   white_byte is 0 or 0xff for BlackIs1 or !BlackIs1 respectively;
 *   data holds p[-1], inverted if !BlackIs1;
 *   count is the number of valid bits remaining in the scan line.
 */

/* Tables in scftab.c. */
extern const byte cf_left_bits[8];
extern const byte cf_top_bit[256];
	  
/* Skip over white pixels to find the next black pixel in the input. */
/* There are a lot more white pixels than black pixels, */
/* so we go to some extra trouble to make this efficient. */

#define skip_white_pixels(data, p, count, white_byte)\
{ byte top = data & cf_left_bits[count & 7];\
  if ( top ) count = ((count - 1) & ~7) + cf_top_bit[top];\
  else\
   { if ( white_byte == 0 )\
      for ( ; ; p += 4, count -= 32 )\
      { if ( p[0] ) { data = p[0]; p += 1; count -= 9; break; }\
	if ( p[1] ) { data = p[1]; p += 2; count -= 17; break; }\
	if ( p[2] ) { data = p[2]; p += 3; count -= 25; break; }\
	if ( p[3] ) { data = p[3]; p += 4; count -= 33; break; }\
      }\
      else\
      for ( ; ; p += 4, count -= 32 )\
      { if ( p[0] != 0xff ) { data = (byte)~p[0]; p += 1; count -= 9; break; }\
	if ( p[1] != 0xff ) { data = (byte)~p[1]; p += 2; count -= 17; break; }\
	if ( p[2] != 0xff ) { data = (byte)~p[2]; p += 3; count -= 25; break; }\
	if ( p[3] != 0xff ) { data = (byte)~p[3]; p += 4; count -= 33; break; }\
      }\
     count = (count & ~7) + cf_top_bit[data];\
   }\
}

/* Skip over black pixels to find the next white pixel in the input. */

#define skip_black_pixels(data, p, count, white_byte)\
{ byte top = ~data & cf_left_bits[count & 7];\
  if ( top ) count = ((count - 1) & ~7) + cf_top_bit[top];\
  else\
   { if ( white_byte == 0 )\
       while ( (data = *p++) == 0xff ) count -= 8;\
     else\
       while ( (data = (byte)~*p++) == 0xff ) count -= 8;\
     count = ((count - 9) & ~7) + cf_top_bit[data ^ 0xff];\
   }\
}

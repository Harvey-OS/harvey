/* Copyright (C) 1992, 1995, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: scf.h,v 1.2 2000/09/19 19:00:48 lpd Exp $ */
/* Common definitions for CCITTFax encoding and decoding filters */

#ifndef scf_INCLUDED
#  define scf_INCLUDED

#include "shc.h"

/*
 * The CCITT Group 3 (T.4) and Group 4 (T.6) fax specifications map
 * run lengths to Huffman codes.  White and black have different mappings.
 * If the run length is 64 or greater, two or more codes are needed:
 *      - One or more 'make-up' codes for 2560 pixels;
 *      - A 'make-up' code that encodes the multiple of 64;
 *      - A 'termination' code for the remainder.
 * For runs of 63 or less, only the 'termination' code is needed.
 */

/* ------ Encoding tables ------ */

/*
 * The maximum possible length of a scan line is determined by the
 * requirement that 3 runs have to fit into the stream buffer.
 * A run of length N requires approximately ceil(N / 2560) makeup codes,
 * hence 1.5 * ceil(N / 2560) bytes.  Taking the largest safe stream
 * buffer size as 32K, we arrive at the following maximum width:
 */
#if arch_sizeof_int > 2
#  define cfe_max_width (2560 * 32000 * 2 / 3)
#else
#  define cfe_max_width (max_int - 40)	/* avoid overflows */
#endif
/* The +5 in cfe_max_code_bytes is a little conservative. */
#define cfe_max_code_bytes(width) ((width) / 2560 * 3 / 2 + 5)

typedef hce_code cfe_run;

/* Codes common to 1-D and 2-D encoding. */
/* The decoding algorithms know that EOL is 0....01. */
#define run_eol_code_length 12
#define run_eol_code_value 1
extern const cfe_run cf_run_eol;
typedef struct cf_runs_s {
    cfe_run termination[64];
    cfe_run make_up[41];
} cf_runs;
extern const cf_runs
      cf_white_runs, cf_black_runs;
extern const cfe_run cf_uncompressed[6];
extern const cfe_run cf_uncompressed_exit[10];	/* indexed by 2 x length of */

			/* white run + (1 if next run black, 0 if white) */
/* 1-D encoding. */
extern const cfe_run cf1_run_uncompressed;

/* 2-D encoding. */
extern const cfe_run cf2_run_pass;

#define cf2_run_pass_length 4
#define cf2_run_pass_value 0x1
#define cf2_run_vertical_offset 3
extern const cfe_run cf2_run_vertical[7];	/* indexed by b1 - a1 + offset */
extern const cfe_run cf2_run_horizontal;

#define cf2_run_horizontal_value 1
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
#define cfd_white_min_bits 4	/* shortest white run */
extern const cfd_node cf_white_decode[];

#define cfd_black_initial_bits 7
#define cfd_black_min_bits 2	/* shortest black run */
extern const cfd_node cf_black_decode[];

#define cfd_2d_initial_bits 7
#define cfd_2d_min_bits 4	/* shortest non-H/V 2-D run */
extern const cfd_node cf_2d_decode[];

#define cfd_uncompressed_initial_bits 6		/* must be 6 */
extern const cfd_node cf_uncompressed_decode[];

/* ------ Run detection macros ------ */

/*
 * For the run detection macros:
 *   white_byte is 0 or 0xff for BlackIs1 or !BlackIs1 respectively;
 *   data holds p[-1], inverted if !BlackIs1;
 *   count is the number of valid bits remaining in the scan line.
 */

/* Aliases for bit processing tables. */
#define cf_byte_run_length byte_bit_run_length_neg
#define cf_byte_run_length_0 byte_bit_run_length_0

/* Skip over white pixels to find the next black pixel in the input. */
/* Store the run length in rlen, and update data, p, and count. */
/* There are many more white pixels in typical input than black pixels, */
/* and the runs of white pixels tend to be much longer, so we use */
/* substantially different loops for the two cases. */

#define skip_white_pixels(data, p, count, white_byte, rlen)\
BEGIN\
    rlen = cf_byte_run_length[count & 7][data ^ 0xff];\
    if ( rlen >= 8 ) {		/* run extends past byte boundary */\
	if ( white_byte == 0 ) {\
	    if ( p[0] ) { data = p[0]; p += 1; rlen -= 8; }\
	    else if ( p[1] ) { data = p[1]; p += 2; }\
	    else {\
		while ( !(p[2] | p[3] | p[4] | p[5]) )\
		    p += 4, rlen += 32;\
		if ( p[2] ) {\
		    data = p[2]; p += 3; rlen += 8;\
		} else if ( p[3] ) {\
		    data = p[3]; p += 4; rlen += 16;\
		} else if ( p[4] ) {\
		    data = p[4]; p += 5; rlen += 24;\
		} else /* p[5] */ {\
		    data = p[5]; p += 6; rlen += 32;\
		}\
	    }\
	} else {\
	    if ( p[0] != 0xff ) { data = (byte)~p[0]; p += 1; rlen -= 8; }\
	    else if ( p[1] != 0xff ) { data = (byte)~p[1]; p += 2; }\
	    else {\
		while ( (p[2] & p[3] & p[4] & p[5]) == 0xff )\
		    p += 4, rlen += 32;\
		if ( p[2] != 0xff ) {\
		    data = (byte)~p[2]; p += 3; rlen += 8;\
		} else if ( p[3] != 0xff ) {\
		    data = (byte)~p[3]; p += 4; rlen += 16;\
		} else if ( p[4] != 0xff ) {\
		    data = (byte)~p[4]; p += 5; rlen += 24;\
		} else /* p[5] != 0xff */ {\
		    data = (byte)~p[5]; p += 6; rlen += 32;\
		}\
	    }\
	}\
	rlen += cf_byte_run_length_0[data ^ 0xff];\
    }\
    count -= rlen;\
END

/* Skip over black pixels to find the next white pixel in the input. */
/* Store the run length in rlen, and update data, p, and count. */

#define skip_black_pixels(data, p, count, white_byte, rlen)\
BEGIN\
    rlen = cf_byte_run_length[count & 7][data];\
    if ( rlen >= 8 ) {\
	if ( white_byte == 0 )\
	    for ( ; ; p += 4, rlen += 32 ) {\
		if ( p[0] != 0xff ) { data = p[0]; p += 1; rlen -= 8; break; }\
		if ( p[1] != 0xff ) { data = p[1]; p += 2; break; }\
		if ( p[2] != 0xff ) { data = p[2]; p += 3; rlen += 8; break; }\
		if ( p[3] != 0xff ) { data = p[3]; p += 4; rlen += 16; break; }\
	    }\
	else\
	    for ( ; ; p += 4, rlen += 32 ) {\
		if ( p[0] ) { data = (byte)~p[0]; p += 1; rlen -= 8; break; }\
		if ( p[1] ) { data = (byte)~p[1]; p += 2; break; }\
		if ( p[2] ) { data = (byte)~p[2]; p += 3; rlen += 8; break; }\
		if ( p[3] ) { data = (byte)~p[3]; p += 4; rlen += 16; break; }\
	    }\
	rlen += cf_byte_run_length_0[data];\
    }\
    count -= rlen;\
END

#endif /* scf_INCLUDED */

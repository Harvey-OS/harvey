/* Copyright (C) 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsbittab.c,v 1.5 2002/06/16 05:48:55 lpd Exp $ */
/* Tables for bit operations */
#include "stdpre.h"
#include "gsbittab.h"

/* ---------------- Byte processing tables ---------------- */

/*
 * byte_reverse_bits[B] = the byte B with the order of bits reversed.
 */
const byte byte_reverse_bits[256] = {
    bit_table_8(0, 1, 2, 4, 8, 0x10, 0x20, 0x40, 0x80)
};

/*
 * byte_right_mask[N] = a byte with N trailing 1s, 0 <= N <= 8.
 */
const byte byte_right_mask[9] = {
    0, 1, 3, 7, 0xf, 0x1f, 0x3f, 0x7f, 0xff
};

/*
 * byte_count_bits[B] = the number of 1-bits in a byte with value B.
 */
const byte byte_count_bits[256] = {
    bit_table_8(0, 1, 1, 1, 1, 1, 1, 1, 1)
};

/* ---------------- Scanning tables ---------------- */

/*
 * byte_bit_run_length_N[B], for 0 <= N <= 7, gives the length of the
 * run of 1-bits starting at bit N in a byte with value B,
 * numbering the bits in the byte as 01234567.  If the run includes
 * the low-order bit (i.e., might be continued into a following byte),
 * the run length is increased by 8.
 */

#define t8(n) n,n,n,n,n+1,n+1,n+2,n+11
#define r8(n) n,n,n,n,n,n,n,n
#define r16(n) r8(n),r8(n)
#define r32(n) r16(n),r16(n)
#define r64(n) r32(n),r32(n)
#define r128(n) r64(n),r64(n)
const byte byte_bit_run_length_0[256] = {
    r128(0), r64(1), r32(2), r16(3), r8(4), t8(5)
};
const byte byte_bit_run_length_1[256] = {
    r64(0), r32(1), r16(2), r8(3), t8(4),
    r64(0), r32(1), r16(2), r8(3), t8(4)
};
const byte byte_bit_run_length_2[256] = {
    r32(0), r16(1), r8(2), t8(3),
    r32(0), r16(1), r8(2), t8(3),
    r32(0), r16(1), r8(2), t8(3),
    r32(0), r16(1), r8(2), t8(3)
};
const byte byte_bit_run_length_3[256] = {
    r16(0), r8(1), t8(2), r16(0), r8(1), t8(2),
    r16(0), r8(1), t8(2), r16(0), r8(1), t8(2),
    r16(0), r8(1), t8(2), r16(0), r8(1), t8(2),
    r16(0), r8(1), t8(2), r16(0), r8(1), t8(2)
};
const byte byte_bit_run_length_4[256] = {
    r8(0), t8(1), r8(0), t8(1), r8(0), t8(1), r8(0), t8(1),
    r8(0), t8(1), r8(0), t8(1), r8(0), t8(1), r8(0), t8(1),
    r8(0), t8(1), r8(0), t8(1), r8(0), t8(1), r8(0), t8(1),
    r8(0), t8(1), r8(0), t8(1), r8(0), t8(1), r8(0), t8(1),
};

#define rr8(a,b,c,d,e,f,g,h)\
  a,b,c,d,e,f,g,h, a,b,c,d,e,f,g,h, a,b,c,d,e,f,g,h, a,b,c,d,e,f,g,h,\
  a,b,c,d,e,f,g,h, a,b,c,d,e,f,g,h, a,b,c,d,e,f,g,h, a,b,c,d,e,f,g,h,\
  a,b,c,d,e,f,g,h, a,b,c,d,e,f,g,h, a,b,c,d,e,f,g,h, a,b,c,d,e,f,g,h,\
  a,b,c,d,e,f,g,h, a,b,c,d,e,f,g,h, a,b,c,d,e,f,g,h, a,b,c,d,e,f,g,h,\
  a,b,c,d,e,f,g,h, a,b,c,d,e,f,g,h, a,b,c,d,e,f,g,h, a,b,c,d,e,f,g,h,\
  a,b,c,d,e,f,g,h, a,b,c,d,e,f,g,h, a,b,c,d,e,f,g,h, a,b,c,d,e,f,g,h,\
  a,b,c,d,e,f,g,h, a,b,c,d,e,f,g,h, a,b,c,d,e,f,g,h, a,b,c,d,e,f,g,h,\
  a,b,c,d,e,f,g,h, a,b,c,d,e,f,g,h, a,b,c,d,e,f,g,h, a,b,c,d,e,f,g,h
const byte byte_bit_run_length_5[256] = {
    rr8(0, 0, 0, 0, 1, 1, 2, 11)
};
const byte byte_bit_run_length_6[256] = {
    rr8(0, 0, 1, 10, 0, 0, 1, 10)
};
const byte byte_bit_run_length_7[256] = {
    rr8(0, 9, 0, 9, 0, 9, 0, 9)
};

/* Pointer tables indexed by bit number. */

const byte *const byte_bit_run_length[8] = {
    byte_bit_run_length_0, byte_bit_run_length_1,
    byte_bit_run_length_2, byte_bit_run_length_3,
    byte_bit_run_length_4, byte_bit_run_length_5,
    byte_bit_run_length_6, byte_bit_run_length_7
};
const byte *const byte_bit_run_length_neg[8] = {
    byte_bit_run_length_0, byte_bit_run_length_7,
    byte_bit_run_length_6, byte_bit_run_length_5,
    byte_bit_run_length_4, byte_bit_run_length_3,
    byte_bit_run_length_2, byte_bit_run_length_1
};

/*
 * byte_acegbdfh_to_abcdefgh[acegbdfh] = abcdefgh, where the letters
 * denote the individual bits of the byte.
 */
const byte byte_acegbdfh_to_abcdefgh[256] = {
    bit_table_8(0, 0x80, 0x20, 0x08, 0x02, 0x40, 0x10, 0x04, 0x01)
};

/* Some C compilers insist on having executable code in every file.... */
void gsbittab_dummy(void);	/* for picky compilers */
void
gsbittab_dummy(void)
{
}

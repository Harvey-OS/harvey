/* Copyright (C) 1990, 1996, 1997, 2001 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: ibnum.h,v 1.8 2004/08/04 19:36:12 stefan Exp $ */
/* Encoded number definitions and support */
/* Requires stream.h */

#ifndef ibnum_INCLUDED
#  define ibnum_INCLUDED

/*
 * There is a bug in all Adobe interpreters that causes them to byte-swap
 * native reals in binary object sequences iff the native real format is
 * IEEE.  We emulate this bug (it will be added to the PLRM errata at some
 * point), but under a conditional so that it is clear where this is being
 * done.
 */
#define BYTE_SWAP_IEEE_NATIVE_REALS 1

/*
 * Define the byte that begins an encoded number string.
 * (This is the same as the value of bt_num_array in btoken.h.)
 */
#define bt_num_array_value 149

/*
 * Define the homogenous number array formats.  The default for numbers is
 * big-endian.  Note that these values are defined by the PostScript
 * Language Reference Manual: they are not arbitrary.
 */
#define num_int32 0		/* [0..31] */
#define num_int16 32		/* [32..47] */
#define num_float 48
#define num_float_IEEE num_float
/* Note that num_msb / num_lsb is ignored for num_float_native. */
#define num_float_native (num_float + 1)
#define num_msb 0
#define num_lsb 128
#define num_is_lsb(format) ((format) >= num_lsb)
#define num_is_valid(format) (((format) & 127) <= 49)
/*
 * Special "format" for reading from an array.
 * num_msb/lsb is not used in this case.
 */
#define num_array 256
/* Define the number of bytes for a given format of encoded number. */
extern const byte enc_num_bytes[];	/* in ibnum.c */

#define enc_num_bytes_values\
  4, 4, 2, 4, 0, 0, 0, 0,\
  4, 4, 2, 4, 0, 0, 0, 0,\
  sizeof(ref)
#define encoded_number_bytes(format)\
  (enc_num_bytes[(format) >> 4])

/* Read from an array or encoded number string. */
int num_array_format(const ref *);	/* returns format or error */
uint num_array_size(const ref *, int);
int num_array_get(const gs_memory_t *mem, const ref *, int, uint, ref *);

/* Decode a number from a string with appropriate byte swapping. */
int sdecode_number(const byte *, int, ref *);
int sdecodeshort(const byte *, int);
uint sdecodeushort(const byte *, int);
long sdecodelong(const byte *, int);
float sdecodefloat(const byte *, int);

#endif /* ibnum_INCLUDED */

/* Copyright (C) 1990, 1996, 1997 Aladdin Enterprises.  All rights reserved.

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

/*$Id: ibnum.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Encoded number definitions and support */
/* Requires stream.h */

#ifndef ibnum_INCLUDED
#  define ibnum_INCLUDED

/* Define the byte that begins an encoded number string. */
/* (This is the same as the value of bt_num_array in btoken.h.) */
#define bt_num_array_value 149

/* Homogenous number array formats. */
/* The default for numbers is big-endian. */
#define num_int32 0		/* [0..31] */
#define num_int16 32		/* [32..47] */
#define num_float 48
#define num_float_IEEE num_float
#define num_float_native (num_float + 1)
#define num_msb 0
#define num_lsb 128
#define num_is_lsb(format) ((format) >= num_lsb)
#define num_is_valid(format) (((format) & 127) <= 49)
/* Special "format" for reading from an array. */
/* num_msb/lsb is not used in this case. */
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
int num_array_format(P1(const ref *));	/* returns format or error */
uint num_array_size(P2(const ref *, int));
int num_array_get(P4(const ref *, int, uint, ref *));

/* Decode a number from a string with appropriate byte swapping. */
int sdecode_number(P3(const byte *, int, ref *));
int sdecodeshort(P2(const byte *, int));
uint sdecodeushort(P2(const byte *, int));
long sdecodelong(P2(const byte *, int));
float sdecodefloat(P2(const byte *, int));

#endif /* ibnum_INCLUDED */

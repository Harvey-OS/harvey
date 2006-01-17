/* Copyright (C) 2002 artofcode LLC.  All rights reserved.

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
/* $Id: gsserial.c,v 1.1 2002/08/22 07:12:29 henrys Exp $ */
/* some utilities useful for converting objects to serial form */

#include "stdpre.h"
#include "gstypes.h"
#include "gsserial.h"


/*
 * Procedures for converint between integers and a variable-length,
 * little-endian string representation thereof. This scheme uses a
 * base-128 format with the high-order bit of each byte used as a
 * continuation flag ((b & 0x80) == 0 ==> this is the last byte of the
 * current number). See gsserial.h for complete information.
 */

/*
 * Determine the size of the string representation of an unsigned or
 * signed integer.
 */
int
enc_u_size_uint(uint uval)
{
    int     i = 1;

    while ((uval >>= enc_u_shift) > 0)
        ++i;
    return i;
}

int
enc_s_size_int(int ival)
{
    /* MIN_INT must be handled specially */
    if (ival < 0) {
        if (ival == enc_s_min_int)
            return enc_s_sizew_max;
        ival = -ival;
    }
    return enc_u_sizew((uint)ival << 1);
}

/*
 * Encode a signed or unsigned integer. The array pointed to by ptr is
 * assumed to be large enough. The returned pointer immediately follows
 * the encoded number.
 */
byte *
enc_u_put_uint(uint uval, byte * ptr)
{
    int     tmp_v;

    for (;;) {
        tmp_v = uval & (enc_u_lim_1b - 1);
        if ((uval >>= enc_u_shift) == 0)
            break;
        *ptr++ = tmp_v | enc_u_lim_1b;
    }
    *ptr++ = tmp_v;
    return ptr;
}

byte *
enc_s_put_int(int ival, byte * ptr)
{
    uint    uval, tmp_v;

    /* MIN_INT must be handled specially */
    if (ival < 0 && ival != enc_s_min_int)
        uval = (uint)-ival;
    else
        uval = (uint)ival;

    tmp_v = (uval & enc_s_max_1b) | (ival < 0 ? enc_s_max_1b + 1 : 0);
    if (uval > enc_s_max_1b) {
        *ptr++ = tmp_v | enc_u_lim_1b;
        return enc_u_put_uint(uval >> enc_s_shift0, ptr);
    } else {
        *ptr++ = tmp_v;
        return ptr;
    }
}


/*
 * Decode an integer string for a signed or unsigned integer. Note that
 * two forms of this procedure are provide, to allow both const and non-
 * const byte pointers to be handled (the former is far more common).
 */
const byte *
enc_u_get_uint(uint * pval, const byte * ptr)
{
    uint    uval = 0, tmp_val;
    int     shift = 0;

    while (((tmp_val = *ptr++) & enc_u_lim_1b) != 0) {
        uval |= (tmp_val & (enc_u_lim_1b - 1)) << shift;
        shift += enc_u_shift;
    }
    *pval = uval | (tmp_val << shift);
    
    return ptr;
}

byte *
enc_u_get_uint_nc(uint * pval, byte * ptr)
{
    const byte *    tmp_ptr = ptr;

    tmp_ptr = enc_u_get_uint(pval, tmp_ptr);
    return ptr += tmp_ptr - ptr;
}

const byte *
enc_s_get_int(int * pval, const byte * ptr)
{
    int     ival = *ptr++;
    bool    neg = false;

    if ((ival & (enc_s_max_1b + 1)) != 0) {
        ival ^= enc_s_max_1b + 1;
        neg = true;
    }
    if ((ival & enc_u_lim_1b) != 0) {
        uint     tmp_val;

        ival ^= enc_u_lim_1b;
        ptr = enc_u_get_uint(&tmp_val, ptr);
        ival |= tmp_val << enc_s_shift0;
    }
    if (neg && ival >= 0)    /* >= check required for enc_s_min_int */
        ival = -ival;

    *pval = ival;
    return ptr;
}

byte *
enc_s_get_int_nc(int * pval, byte * ptr)
{
    const byte *    tmp_ptr = ptr;

    tmp_ptr = enc_s_get_int(pval, tmp_ptr);
    return ptr += tmp_ptr - ptr;
}

#ifdef UNIT_TEST

#include <stdio.h>
#include <string.h>


/*
 * Encoding and decoding of integers is verified using a round-trip process,
 * integer ==> string ==> integer. The string size is separately checked to
 * verify that it is not too large (it can't be too small if the round-trip
 * check works). If an integer x is represented by nbytes, then it must be
 * that x >= 1U << (7 * (n - 1)) (unsigned; 1U << (7 * (n - 2) + 6) for
 * signed integers; there is no need to check 1-byte encodings).
 *
 * It is possible to check every value, but this is not necessary. Any
 * failures that arise will do so in the vicinty of powers of 2.
 */

/* check the length of an encoded string */
void
check_u_sizew(uint uval, int len)
{
    if (len != enc_u_sizew(uval))
        fprintf( stderr,
                 "Size calculation error for (usigned) %u (%d != %d)\n",
                 uval,
                 len,
                 enc_u_sizew(uval) );
    if ( len > 1                                                           &&
         (len > enc_u_sizew_max  || uval < 1U << (enc_u_shift * (len - 1)))  )
        fprintf( stderr, "unsigned encoding too large for %u (%d bytes)\n",
                 uval,
                 len );
}

void
check_s_sizew(int ival, int len)
{
    uint    uval;

    if (len != enc_s_sizew(ival))
        fprintf( stderr,
                 "Size calculation error for (signed) %d (%d != %d)\n",
                 ival,
                 len,
                 enc_s_sizew(ival) );
    if (len <= 1)
        return;
    if (ival < 0 && ival != enc_s_min_int)
        uval = (uint)-ival;
    else
        uval = (uint)ival;
    if ( len > enc_s_sizew_max                                 ||
         uval < 1U << (enc_s_shift1 * (len - 2) + enc_s_shift0)  )
        fprintf( stderr,
                 "signed encoding too large for %d (%d bytes)\n",
                 ival,
                 len );
}

/* check the encode and decode procedures on a value */
void
check_u(uint uval)
{
    byte            buff[32];   /* generous size */
    byte *          cp0 = buff;
    const byte *    cp1 = buff;
    byte *          cp2 = buff;
    uint            res_val;

    memset(buff, 0, sizeof(buff));
    enc_u_putw(uval, cp0);
    check_u_sizew(uval, cp0 - buff);
    memset(cp0, (uval == 0 ? 0x7f : 0), sizeof(buff) - (cp0 - buff));

    enc_u_getw(res_val, cp1);
    if (cp1 != cp0)
        fprintf( stderr,
                 "encoded length disparity (const) for "
                 "(unsigned) %u (%d != %d)\n",
                 uval,
                 cp0 - buff,
                 cp1 - buff );
    if (res_val != uval)
        fprintf( stderr,
                 "decode error (const) for (unsigned) %u (!= %u)\n",
                 uval,
                 res_val );

    enc_u_getw_nc(res_val, cp2);
    if (cp2 != cp0)
        fprintf( stderr,
                 "encoded length disparity (non-const) for "
                 "(unsigned) %u (%d != %d)\n",
                 uval,
                 cp0 - buff,
                 cp1 - buff );
    if (res_val != uval)
        fprintf( stderr,
                 "decode error (non-const) for (unsigned) %u (!= %u)\n",
                 uval,
                 res_val );
}

void
check_s(int ival)
{
    byte            buff[32];   /* generous size */
    byte *          cp0 = buff;
    const byte *    cp1 = buff;
    byte *          cp2 = buff;
    int             res_val;

    memset(buff, 0, sizeof(buff));
    enc_s_putw(ival, cp0);
    check_s_sizew(ival, cp0 - buff);
    memset(cp0, (ival == 0 ? 0x7f : 0), sizeof(buff) - (cp0 - buff));

    enc_s_getw(res_val, cp1);
    if (cp1 != cp0)
        fprintf( stderr,
                 "encoded length disparity (const) for "
                 "(signed) %d (%d != %d)\n",
                 ival,
                 cp0 - buff,
                 cp1 - buff );
    if (res_val != ival)
        fprintf( stderr,
                 "decode error (const) for (signed) %d (!= %d)\n",
                 ival,
                 res_val );

    enc_s_getw_nc(res_val, cp2);
    if (cp1 != cp0)
        fprintf( stderr,
                 "encoded length disparity (non-const) for "
                 "(signed) %d (%d != %d)\n",
                 ival,
                 cp0 - buff,
                 cp1 - buff );
    if (res_val != ival)
        fprintf( stderr,
                 "decode error (non-const) for (unsigned) %d (!= %d)\n",
                 ival,
                 res_val );
}

/* test the provided value and some surrounding values */
void
check_u_vals(uint uval)
{
    uint    diff = 1;

    check_u(uval);
    do {
        check_u(uval - diff);
        check_u(uval + diff);
    } while ((diff <<= 1) < uval);
}

void
check_s_vals(int ival)
{
    int     diff = 1;

    check_s(ival);
    if (ival == enc_s_min_int) {
        do {
            check_s(ival - diff);
            check_s(ival + diff);
        } while ((diff <<= 1) != enc_s_min_int);
    } else {
        int     abs_val = (ival < 0 ? -ival : ival);

        do {
            check_s(ival - diff);
            check_s(ival + diff);
        } while ((diff <<= 1) < abs_val);
    }
}


int
main(void)
{
    uint     uval;
    int      ival;

    check_u_vals(0);
    for (uval = 1; uval != 0; uval <<= 1)
        check_u_vals(uval);

    check_s_vals(0);
    for (ival = 1; ival != 0; ival <<= 1) {
        check_s_vals(ival);
        if (ival != enc_s_min_int)
            check_s_vals(-ival);
    }

    fprintf(stderr, "all done\n");
    return 0;
}

#endif  /* UNIT_TEST */

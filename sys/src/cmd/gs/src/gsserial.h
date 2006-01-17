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
/* $Id: gsserial.h,v 1.1 2002/08/22 07:12:29 henrys Exp $ */
/* some general structures useful for converting objects to serial form */

#ifndef gsserial_INCLUDED
#  define gsserial_INCLUDED

/*
 * A variable-length, little-endian format for encoding (unsigned)
 * integers.
 *
 * This format represents integers is a base-128 form that is quite
 * compact for parameters whose values are typically small.
 *
 * A number x is represented by the string of bytes s[0], ... s[n],
 * where:
 *
 *    s[i] & 0x80 == 0x80 for i = 0, ..., n - 1,
 *    s[n] & 0x80 == 0x00 
 *
 * and
 *
 *    x == s[0] + (s[1] << 7) + (s[2] << 14) + ... (s[n] << (n * 7))
 *
 * In words, the high-order bit is used as a continue bit; it is off
 * only for the last byte of the string. The low-order 7 bits of each
 * byte for the base-128 digit, with the low-order digits preceding
 * the high-order digits.
 *
 * The encoding considers all numbers as unsigned. It may be used with
 * signed quantities (just change the interpretation), though obviously
 * it is inefficient with negative numbers.
 *
 * This code was originally part of the command list device module.
 * Though the names used here differ from those used from those used in
 * that module, they follow the same pattern, which accounts for certain
 * peculiarities.
 */
#define enc_u_shift     7
#define enc_u_lim_1b    (1U << enc_u_shift)
#define enc_u_lim_2b    (1U << (2 * enc_u_shift))

/* determine the encoded size of an (unsigned) integer */
extern  int     enc_u_size_uint(uint);

#define enc_u_sizew(w)                                      \
    ( (uint)(w) < enc_u_lim_1b                              \
        ? 1                                                 \
        : (uint)(w) < enc_u_lim_2b ? 2 : enc_u_size_uint(w) ) 

/* similarly, for a pair of values (frequently used for points) */
#define enc_u_size2w(w1, w2)                        \
    ( ((uint)(w1) | (uint)(w2)) < enc_u_lim_1b      \
        ? 2                                         \
        : enc_u_size_uint(w1) + enc_u_size_uint(w2) )

#define enc_u_sizexy(xy)    enc_u_size2w((xy).x, (xy).y)

/* the maximum size of an encoded uint */
#define enc_u_sizew_max ((8 * sizeof(uint) + enc_u_shift - 1) / enc_u_shift)

/* encode and decode an unsigned integer; note special handling of const */
extern  byte *          enc_u_put_uint(uint, byte *);
extern  const byte *    enc_u_get_uint(uint *, const byte *);
extern  byte *          enc_u_get_uint_nc(uint *, byte *);

#define enc_u_putw(w, p)                                        \
    BEGIN                                                       \
        if ((uint)(w) < enc_u_lim_1b)                           \
            *(p)++ = (byte)(w);                                 \
        else if ((uint)(w) < enc_u_lim_2b) {                    \
            *(p)++ = enc_u_lim_1b | ((w) & (enc_u_lim_1b - 1)); \
            *(p)++ = (w) >> enc_u_shift;                        \
        } else                                                  \
            (p) = enc_u_put_uint((w), (p));                     \
    END

/* encode a pair of integers; this is often used with points */
#define enc_u_put2w(w1, w2, p)                          \
    BEGIN                                               \
        if (((uint)(w1) | (uint)(w2)) < enc_u_lim_1b) { \
            *(p)++ = (w1);                              \
            *(p)++ = (w2);                              \
        } else {                                        \
            (p) = enc_u_put_uint((w1), (p));            \
            (p) = enc_u_put_uint((w2), (p));            \
        }                                               \
    END

#define enc_u_putxy(xy, p)    enc_u_put2w((xy).x, (xy).y, (p))


/* decode an unsigned integer */
#define enc_u_getw(w, p)                        \
    BEGIN                                       \
        if (((w) = *(p)) >= enc_u_lim_1b) {     \
            uint    tmp_w;                      \
                                                \
            (p) = enc_u_get_uint(&tmp_w, (p));  \
            (w) = tmp_w;                        \
        } else                                  \
            ++(p);                              \
    END

#define enc_u_getw_nc(w, p)                         \
    BEGIN                                           \
        if (((w) = *(p)) >= enc_u_lim_1b) {         \
            uint    tmp_w;                          \
                                                    \
            (p) = enc_u_get_uint_nc(&tmp_w, (p));   \
            (w) = tmp_w;                            \
        } else                                      \
            ++(p);                                  \
    END

/* decode a pair of unsigned integers; this is often used for points */
#define enc_u_get2w(w1, w2, p)  \
    BEGIN                       \
        enc_u_getw((w1), (p));  \
        enc_u_getw((w2), (p));  \
    END

#define enc_u_get2w_nc(w1, w2, p)   \
    BEGIN                           \
        enc_u_getw_nc((w1), (p));   \
        enc_u_getw_nc((w2), (p));   \
    END

#define enc_u_getxy(xy, p)      enc_u_get2w((xy).x, (xy).y, (p))
#define enc_u_getxy_nc(xy, p)   enc_u_get2w_nc((xy).x, (xy).y, (p))


/*
 * An encoding mechanism similar to that above for signed integers. This
 * makes use of the next-to-highest order bit of the first byte to encode
 * the sign of the number. Thus, the number x is represented by the bytes
 * s[0], ... s[n], where:
 *
 *    s[i] & 0x80 == 0x80 for i = 0, ..., n - 1,
 *    s[n] & 0x80 == 0x00,
 *
 *    s[0] & 0x40 == 0x40 if x < 0,
 *    s[0] & 0x40 == 0x00 if x >- 0,
 *
 * and
 *
 *    abs(x) = s[0] + (s[1] << 6) + (s[2] << 13) + ... (s[n] * (n * 7 - 1))
 *
 * This encoding is less efficient than the unsigned encoding for non-
 * negative numbers but is much more efficient for numbers that might be
 * negative.
 *
 * There are no special 2-value versions of these macros, as it is not
 * possible to test both values against the limit simultaneously. We do,
 * however, provide point encoding macros.
 */
#define enc_s_shift0    6
#define enc_s_shift1    (enc_s_shift0 + 1)
#define enc_s_max_1b    ((1U << enc_s_shift0) - 1)
#define enc_s_min_1b    (-(int)enc_s_max_1b)
#define enc_s_max_2b    ((1U << (enc_s_shift0 + enc_s_shift1) - 1))
#define enc_s_min_2b    (-enc_s_max_2b)
#define enc_s_min_int   ((int)(1U << (8 * sizeof(int) - 1)))

/* determine the encoded size of a signed integer */
extern  int     enc_s_size_int(int);

/* the maximum size of encoded integer */
#define enc_s_sizew_max   ((8 * sizeof(int)) / enc_s_shift1 + 1)

#define enc_s_sizew(v)                                                  \
    ( (v) >= 0 ? enc_u_sizew((uint)(v) << 1)                            \
               : (v) != enc_s_min_int ? enc_u_sizew((uint)-(v) << 1)    \
                                      : enc_s_sizew_max )

#define enc_s_sizexy(xy)    (enc_s_sizew((xy).x) + enc_s_sizew((xy).y))


/* encode and decode a signed integfer; note special handling of const */
extern  byte *          enc_s_put_int(int, byte *);
extern  const byte *    enc_s_get_int(int *, const byte *);
extern  byte *          enc_s_get_int_nc(int *, byte *);

#define enc_s_putw(v, p)                                            \
    BEGIN                                                           \
        if ((int)(v) <= enc_s_max_1b && (int)(v) >= enc_s_min_1b)   \
            *(p)++ =  ((v) & enc_s_max_1b)                          \
                    | ((v) < 0 ? (enc_s_max_1b + 1) : 0);           \
        else                                                        \
            (p) = enc_s_put_int((v), (p));                          \
    END

#define enc_s_putxy(xy, p)          \
    BEGIN                           \
        enc_s_putw((xy).x, (p));    \
        enc_s_putw((xy).y, (p));    \
    END

#define enc_s_getw(v, p)                                \
    BEGIN                                               \
        if (((v = *p) & (1U << enc_s_shift1)) != 0) {   \
            int     tmp_v;                              \
                                                        \
            (p) = enc_s_get_int(&tmp_v, (p));           \
            (v) = tmp_v;                                \
        } else {                                        \
            if (((v) & (1U << enc_s_shift0)) != 0)      \
                (v) = -((v) & enc_s_max_1b);            \
            ++(p);                                      \
        }                                               \
    END

#define enc_s_getw_nc(v, p)                             \
    BEGIN                                               \
        if (((v = *p) & (1U << enc_s_shift1)) != 0) {   \
            int     tmp_v;                              \
                                                        \
            (p) = enc_s_get_int_nc(&tmp_v, (p));        \
            (v) = tmp_v;                                \
        } else {                                        \
            if (((v) & (1U << enc_s_shift0)) != 0)      \
                (v) = -((v) & enc_s_max_1b);            \
            ++(p);                                      \
        }                                               \
    END

#define enc_s_getxy(xy, p)          \
    BEGIN                           \
        enc_s_getw((xy).x, (p));    \
        enc_s_getw((xy).y, (p));    \
    END

#define enc_s_getxy_nc(xy, p)       \
    BEGIN                           \
        enc_s_getw_nc((xy).x, (p)); \
        enc_s_getw_nc((xy).y, (p)); \
    END

#endif  /* gsserial_INCLUDED */

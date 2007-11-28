/* -*- mode: C; mode: fold -*- */

/* $Id: pcm.c,v 1.7 2001/02/17 14:30:56 aleidinger Exp $ */

/*
 *  There are a lot of not tested return codes.
 *  This is currently intention to make the code more readable
 *  in the design phase.
 *  Keine Berücksichtigung des Ein- und Ausschwingens des Downsamplefilters am
 *  Anfang und am Ende wie bei meinem Resample-Programm
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "bitstream.h"
#include "id3tag.h"

#define FN(x)	do { static unsigned int cnt = 0; if (cnt < 100000) fprintf (stderr,"[%3u]>>>%s<<<\n",++cnt,(x)), fflush(stderr); } while (0)

#ifdef KLEMM_44

/*{{{ #includes                       */

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <memory.h>
#include <unistd.h>
#include "pcm.h"

/*}}}*/
/*{{{ bitstream object                */

/*
 *  About handling of octetstreams:
 *
 *  CHAR_BIT = 8: data points to a normal memory block which can be written to
 *                disk via fwrite
 *  CHAR_BIT > 8: On every char only the bits from 0...7 are used. Depending on the
 *                host I/O it can be possible that you must first pack the data
 *                stream before writing to disk/network/...
 *  CHAR_BIT < 8: Not allowed by C
 */

octetstream_t*  octetstream_open ( OUT size_t  size )
{
    octetstream_t*  ret = (octetstream_t*) calloc ( 1, sizeof(octetstream_t) );
    FN("octetstream_open");
    ret -> data = (uint8_t*) calloc ( 1, size );
    ret -> size = size;
    return ret;
}

int  octetstream_resize ( INOUT octetstream_t* const  os, OUT size_t  size )
{
    FN("octetstream_resize");
    if ( size > os->size ) {
        os -> data = (uint8_t*) realloc ( os -> data, size );
        memset ( os -> data + os -> size, 0, size - os -> size );
        os -> size = size;
    } else if ( size < os->size ) {
	os -> data = (uint8_t*) realloc ( os -> data, os->size = size );
    }
    return 0;
}

int  octetstream_close ( INOUT octetstream_t* const  os )
{
    FN("octetstream_close");
    if ( os == NULL )
        return -1;
    if ( os -> data != NULL ) {
        free (os -> data);
        os -> data = NULL;
        free (os);
        return 0;
    } else {
        free (os);
        return -1;
    }
}

/*}}}*/
/*{{{ encode one frame                */

static inline int  lame_encode_frame (
        INOUT lame_t*     lame,
        OUTTR sample_t**  inbuf,
        IN    uint8_t*    mp3buf,
        OUT   size_t      mp3buf_size )
{
    int  ret;
#if 1
    int           i;
    static int    j  = 0;
    static FILE*  fp = NULL;
    if ( fp == NULL )
        fp = fopen ("pcm_data.txt", "w");
    for ( i = 0; i < lame->frame_size; i++ )
        fprintf ( fp, "%7d %11.4f %11.4f\n", j++, inbuf[0][i], inbuf[1][i] );
    fprintf ( fp, "\n" );
    fflush ( fp );
#endif
    
    FN("lame_encode_frame");

    switch ( lame -> coding ) {
#ifdef HAVE_MPEG_LAYER1
    case coding_MPEG_Layer_1:
        ret = lame_encode_mp1_frame ( lame->global_flags, inbuf[0], inbuf[1], mp3buf, mp3buf_size );
        break;
#endif
#ifdef HAVE_MPEG_LAYER2
    case coding_MPEG_Layer_2:
        ret = lame_encode_mp2_frame ( lame->global_flags, inbuf[0], inbuf[1], mp3buf, mp3buf_size );
        break;
#endif
    case coding_MPEG_Layer_3:
        ret = lame_encode_mp3_frame ( lame->global_flags, inbuf[0], inbuf[1], mp3buf, mp3buf_size );
        break;
#ifdef HAVE_MPEG_PLUS
    case coding_MPEG_plus:
        ret = lame_encode_mpp_frame ( lame->global_flags, inbuf[0], inbuf[1], mp3buf, mp3buf_size );
        break;
#endif
#ifdef HAVE_AAC
    case coding_MPEG_AAC:
        ret = lame_encode_aac_frame ( lame->global_flags, inbuf[0], inbuf[1], mp3buf, mp3buf_size );
        break;
#endif
#ifdef HAVE_VORBIS
    case coding_Ogg_Vorbis:
        ret = lame_encode_ogg_frame ( lame->global_flags, inbuf[0], inbuf[1], mp3buf, mp3buf_size );
        break;
#endif
    default:
        ret = -5;
        break;
    }
    
    if ( ret >= 0 ) {
        lame->frame_count++;
        if ( lame->analyzer_callback != NULL )
            lame->analyzer_callback ( lame, lame->frame_size );
    }
    return ret;
}

/*}}}*/
/*{{{ demultiplexing tools            */

/*
 * Now there are following the so called peek functions. They got a pointer and returning
 * the data to this pointer as a sample. The size of the memory object can be from
 * 8 up to 80 bits. One sample must be fully determined by it's bits, not by the history
 * or by other channels or things like that. Also the input must have a integral byte size.
 *
 * That means:
 *
 *   - 4 or 6 bit PCM can't be decoded
 *   - APCM can't be decoded
 *   - ulaw/alaw *can* be decoded
 *
 * Note: In the future there will be a SMART define supported which only
 *       support native endian shorts.
 */

static const int16_t  ulaw [256] = {
    -32124, -31100, -30076, -29052, -28028, -27004, -25980, -24956,
    -23932, -22908, -21884, -20860, -19836, -18812, -17788, -16764,
    -15996, -15484, -14972, -14460, -13948, -13436, -12924, -12412,
    -11900, -11388, -10876, -10364,  -9852,  -9340,  -8828,  -8316,
     -7932,  -7676,  -7420,  -7164,  -6908,  -6652,  -6396,  -6140,
     -5884,  -5628,  -5372,  -5116,  -4860,  -4604,  -4348,  -4092,
     -3900,  -3772,  -3644,  -3516,  -3388,  -3260,  -3132,  -3004,
     -2876,  -2748,  -2620,  -2492,  -2364,  -2236,  -2108,  -1980,
     -1884,  -1820,  -1756,  -1692,  -1628,  -1564,  -1500,  -1436,
     -1372,  -1308,  -1244,  -1180,  -1116,  -1052,   -988,   -924,
      -876,   -844,   -812,   -780,   -748,   -716,   -684,   -652,
      -620,   -588,   -556,   -524,   -492,   -460,   -428,   -396,
      -372,   -356,   -340,   -324,   -308,   -292,   -276,   -260,
      -244,   -228,   -212,   -196,   -180,   -164,   -148,   -132,
      -120,   -112,   -104,    -96,    -88,    -80,    -72,    -64,
       -56,    -48,    -40,    -32,    -24,    -16,     -8,      0,
     32124,  31100,  30076,  29052,  28028,  27004,  25980,  24956,
     23932,  22908,  21884,  20860,  19836,  18812,  17788,  16764,
     15996,  15484,  14972,  14460,  13948,  13436,  12924,  12412,
     11900,  11388,  10876,  10364,   9852,   9340,   8828,   8316,
      7932,   7676,   7420,   7164,   6908,   6652,   6396,   6140,
      5884,   5628,   5372,   5116,   4860,   4604,   4348,   4092,
      3900,   3772,   3644,   3516,   3388,   3260,   3132,   3004,
      2876,   2748,   2620,   2492,   2364,   2236,   2108,   1980,
      1884,   1820,   1756,   1692,   1628,   1564,   1500,   1436,
      1372,   1308,   1244,   1180,   1116,   1052,    988,    924,
       876,    844,    812,    780,    748,    716,    684,    652,
       620,    588,    556,    524,    492,    460,    428,    396,
       372,    356,    340,    324,    308,    292,    276,    260,
       244,    228,    212,    196,    180,    164,    148,    132,
       120,    112,    104,     96,     88,     80,     72,     64,
        56,     48,     40,     32,     24,     16,      8,      0,
};

static const int16_t  alaw [256] = {
     -5504,  -5248,  -6016,  -5760,  -4480,  -4224,  -4992,  -4736,
     -7552,  -7296,  -8064,  -7808,  -6528,  -6272,  -7040,  -6784,
     -2752,  -2624,  -3008,  -2880,  -2240,  -2112,  -2496,  -2368,
     -3776,  -3648,  -4032,  -3904,  -3264,  -3136,  -3520,  -3392,
    -22016, -20992, -24064, -23040, -17920, -16896, -19968, -18944,
    -30208, -29184, -32256, -31232, -26112, -25088, -28160, -27136,
    -11008, -10496, -12032, -11520,  -8960,  -8448,  -9984,  -9472,
    -15104, -14592, -16128, -15616, -13056, -12544, -14080, -13568,
      -344,   -328,   -376,   -360,   -280,   -264,   -312,   -296,
      -472,   -456,   -504,   -488,   -408,   -392,   -440,   -424,
       -88,    -72,   -120,   -104,    -24,     -8,    -56,    -40,
      -216,   -200,   -248,   -232,   -152,   -136,   -184,   -168,
     -1376,  -1312,  -1504,  -1440,  -1120,  -1056,  -1248,  -1184,
     -1888,  -1824,  -2016,  -1952,  -1632,  -1568,  -1760,  -1696,
      -688,   -656,   -752,   -720,   -560,   -528,   -624,   -592,
      -944,   -912,  -1008,   -976,   -816,   -784,   -880,   -848,
      5504,   5248,   6016,   5760,   4480,   4224,   4992,   4736,
      7552,   7296,   8064,   7808,   6528,   6272,   7040,   6784,
      2752,   2624,   3008,   2880,   2240,   2112,   2496,   2368,
      3776,   3648,   4032,   3904,   3264,   3136,   3520,   3392,
     22016,  20992,  24064,  23040,  17920,  16896,  19968,  18944,
     30208,  29184,  32256,  31232,  26112,  25088,  28160,  27136,
     11008,  10496,  12032,  11520,   8960,   8448,   9984,   9472,
     15104,  14592,  16128,  15616,  13056,  12544,  14080,  13568,
       344,    328,    376,    360,    280,    264,    312,    296,
       472,    456,    504,    488,    408,    392,    440,    424,
        88,     72,    120,    104,     24,      8,     56,     40,
       216,    200,    248,    232,    152,    136,    184,    168,
      1376,   1312,   1504,   1440,   1120,   1056,   1248,   1184,
      1888,   1824,   2016,   1952,   1632,   1568,   1760,   1696,
       688,    656,    752,    720,    560,    528,    624,    592,
       944,    912,   1008,    976,    816,    784,    880,    848,
};

/*
 *  These macros get 1...4 bytes and calculating a value between -32768 and +32768.
 *  The first argument of the macros is the MSB, the last the LSB.
 *  These are used to decode integer PCM (if there is no faster code available).
 */

#define BYTES1(x1)          ((((int8_t)(x1)) << 8))
#define BYTES2(x1,x2)       ((((int8_t)(x1)) << 8) + (uint8_t)(x2))
#define BYTES3(x1,x2,x3)    ((((int8_t)(x1)) << 8) + (uint8_t)(x2) + 1/256.*(uint8_t)(x3))
#define BYTES4(x1,x2,x3,x4) ((((int8_t)(x1)) << 8) + (uint8_t)(x2) + 1/256.*(uint8_t)(x3) + 1/65536.*(uint8_t)(x4))

/*
 * The next two functions can read a 32 and 64 bit floating pointer number
 * of an opposite byte order machine. This is done by byte swaping and using
 * the normal way to read such a variable.
 * Also note: peek function got their data from a variable called src, which is a pointer
 * to const unsigned char.
 */

static inline sample_t  read_opposite_float32 ( OUT uint8_t* const src )
{
    uint8_t  tmp [4];
    tmp[0] = src[3];
    tmp[1] = src[2];
    tmp[2] = src[1];
    tmp[3] = src[0];
    return *(const float32_t*)tmp;
}

static inline sample_t  read_opposite_float64 ( OUT uint8_t* const src )
{
    uint8_t  tmp [8];
    tmp[0] = src[7];
    tmp[1] = src[6];
    tmp[2] = src[5];
    tmp[3] = src[4];
    tmp[4] = src[3];
    tmp[5] = src[2];
    tmp[6] = src[1];
    tmp[7] = src[0];
    return *(const float64_t*)tmp;
}

/*
 *  The next two functions can read a 80 bit extended precision
 *  IEEE-854 floating point number. This code also can read the numbers
 *  on a machine not supporting 80 bit floats.
 */

static sample_t  read_float80_le ( OUT uint8_t* const src )
{
    int         sign     = src [9] & 128  ?  -1  :  +1;
    long        exponent =(src [9] & 127) * 256 +
                           src [8] - 16384 - 62;
    long double mantisse = src [7] * 72057594037927936.L +
                           src [6] * 281474976710656.L +
                           src [5] * 1099511627776.L +
                           src [4] * 4294967296.L +
                           src [3] * 16777216.L +
                           src [2] * 65536.L +
                           src [1] * 256.L +
                           src [0];
    return sign * ldexp (mantisse, exponent);
}

static sample_t  read_float80_be ( OUT uint8_t* const src )
{
    int         sign     = src [0] & 128  ?  -1  :  +1;
    long        exponent =(src [0] & 127) * 256 +
                           src [1] - 16384 - 62;
    long double mantisse = src [2] * 72057594037927936.L +
                           src [3] * 281474976710656.L +
                           src [4] * 1099511627776.L +
                           src [5] * 4294967296.L +
                           src [6] * 16777216.L +
                           src [7] * 65536.L +
                           src [8] * 256.L +
                           src [9];
    return sign * ldexp (mantisse, exponent);
}

static inline sample_t  read_opposite_float80 ( OUT uint8_t* const src )
{
    uint8_t  tmp [10];
    tmp[0] = src[9];
    tmp[1] = src[8];
    tmp[2] = src[7];
    tmp[3] = src[6];
    tmp[4] = src[5];
    tmp[5] = src[4];
    tmp[6] = src[3];
    tmp[7] = src[2];
    tmp[8] = src[1];
    tmp[9] = src[0];
    return *(const float80_t*)tmp;
}


/*
 *  Now we are building all resorting functions. The macro wants the name of the function
 *  and a peek function or macro. The created function wants the following input:
 *
 *     destination: Where to store the result as sample_t vector? The distance between
 *                  the elements is sizeof(sample_t)
 *     source:      Where to get the bytes which should converted to destination
 *     step size:   The distance between the input samples. This is at least the size
 *                  of one input element, but can be more due to
 *                    - alignment
 *                    - interleaved multi-channel input
 *     length:      The number of elements to process. Number must be positive.
 */

typedef void (*demux_t) ( IN sample_t* dst, OUT uint8_t* src, OUT ssize_t step, OUT size_t len );

#define FUNCTION(name,expr)       \
static void  name (               \
        IN  sample_t*  dst,       \
	OUT uint8_t*   src,       \
	OUT ssize_t    step,      \
	OUT size_t     len )      \
{                                 \
    size_t  i = len;              \
    do {                          \
        *dst++ = (expr);          \
        src   += (step);          \
    } while (--i);                \
}

/* Some of these function can be implemented byte order independent ... */

FUNCTION ( copy_silence, 0 )
FUNCTION ( copy_u8     , BYTES1 (src[0]-128) )
FUNCTION ( copy_s8     , BYTES1 (src[0]    ) )
FUNCTION ( copy_alaw   , alaw[*src] )
FUNCTION ( copy_ulaw   , ulaw[*src] )
FUNCTION ( copy_s24_le , BYTES3 (src[2], src[1], src[0] ) )
FUNCTION ( copy_s24_be , BYTES3 (src[0], src[1], src[2] ) )
FUNCTION ( copy_u24_le , BYTES3 (src[2]-128, src[1], src[0] ) )
FUNCTION ( copy_u24_be , BYTES3 (src[0]-128, src[1], src[2] ) )
FUNCTION ( copy_f80_le , read_float80_le (src) )
FUNCTION ( copy_f80_be , read_float80_be (src) )

/* ... and some are not or there are faster but byte order dependent possiblities */


#ifndef WORDS_BIGENDIAN

/* little endian */
FUNCTION ( copy_s16_le , *(const int16_t*)src )
FUNCTION ( copy_s16_be , BYTES2 (src[0], src[1] ) )
FUNCTION ( copy_u16_le , *(const uint16_t*)src - 32768 )
FUNCTION ( copy_u16_be , BYTES2 (src[0]-128, src[1] ) )
FUNCTION ( copy_s32_le , 1/65536. * *(const int32_t*)src )
FUNCTION ( copy_s32_be , BYTES4 (src[0], src[1], src[2], src[3] ) )
FUNCTION ( copy_u32_le , 1/65536. * *(const uint32_t*)src - 32768. )
FUNCTION ( copy_u32_be , BYTES4 (src[0]-128, src[1], src[2], src[3] ) )
FUNCTION ( copy_f32_le , *(const float32_t*)src )
FUNCTION ( copy_f32_be , read_opposite_float32 (src) )
FUNCTION ( copy_f64_le , *(const float64_t*)src )
FUNCTION ( copy_f64_be , read_opposite_float64 (src) )

#else

/* big endian */
FUNCTION ( copy_s16_le , BYTES2 (src[1], src[0] ) )
FUNCTION ( copy_s16_be , *(const int16_t*)src )
FUNCTION ( copy_u16_le , BYTES2 (src[1]-128, src[0] ) )
FUNCTION ( copy_u16_be , *(const uint16_t*)src - 32768 )
FUNCTION ( copy_s32_le , BYTES4 (src[3], src[2], src[1], src[0] ) )
FUNCTION ( copy_s32_be , 1/65536. * *(const int32_t*)src )
FUNCTION ( copy_u32_le , BYTES4 (src[3]-128, src[2], src[1], src[0] ) )
FUNCTION ( copy_u32_be , 1/65536. * *(const uint32_t*)src - 32768. )
FUNCTION ( copy_f32_le , read_opposite_float32 (src) )
FUNCTION ( copy_f32_be , *(const float32_t*)src )
FUNCTION ( copy_f64_le , read_opposite_float64 (src) )
FUNCTION ( copy_f64_be , *(const float64_t*)src )

#endif

/*
 *  The global collection of channel demultiplexer.
 *  For every data type we have the size of one memory object and two
 *  demultiplexer, one for big endians, one for little endians.
 *  The demultiplexers
 *    - 1st argument is the destination of the data converted to sample_t (and normalized to +/-32768)
 *    - 2nd argument is a pointer to the source data, in any format
 *    - 3rd argument is 1st argument of the table (multiplied by the channel count in the case of 
 *      interleaved data)
 *    - 4th argument is the count of samples per channel to convert
 */

typedef struct {
    const ssize_t  size;
    const demux_t  demultiplexer_be;
    const demux_t  demultiplexer_le;
} demux_info_t;

const demux_info_t  demux_info [] = {
    { -1, NULL        , NULL         }, /* 00: */
    {  0, copy_silence, copy_silence }, /* 01: */
    {  1, copy_u8     , copy_u8      }, /* 02: */
    {  1, copy_s8     , copy_s8      }, /* 03: */
    {  2, copy_u16_be , copy_u16_le  }, /* 04: */
    {  2, copy_s16_be , copy_s16_le  }, /* 05: */
    {  3, copy_u24_be , copy_u24_le  }, /* 06: */
    {  3, copy_s24_be , copy_s24_le  }, /* 07: */
    {  4, copy_u32_be , copy_u32_le  }, /* 08: */
    {  4, copy_s32_be , copy_s32_le  }, /* 09: */
    { -1, NULL        , NULL         }, /* 0A: */
    { -1, NULL        , NULL         }, /* 0B: */
    { -1, NULL        , NULL         }, /* 0C: */
    { -1, NULL        , NULL         }, /* 0D: */
    { -1, NULL        , NULL         }, /* 0E: */
    { -1, NULL        , NULL         }, /* 0F: */
    { -1, NULL        , NULL         }, /* 10: */
    { -1, NULL        , NULL         }, /* 11: */
    { -1, NULL        , NULL         }, /* 12: */
    { -1, NULL        , NULL         }, /* 13: */
    {  4, copy_f32_be , copy_f32_le  }, /* 14: */
    { -1, NULL        , NULL         }, /* 15: */
    { -1, NULL        , NULL         }, /* 16: */
    { -1, NULL        , NULL         }, /* 17: */
    {  8, copy_f64_be , copy_f64_le  }, /* 18: */
    { -1, NULL        , NULL         }, /* 19: */
    { 10, copy_f80_be , copy_f80_le  }, /* 1A: */
    { -1, NULL        , NULL         }, /* 1B: */
    { 12, copy_f80_be , copy_f80_le  }, /* 1C: */
    { -1, NULL        , NULL         }, /* 1D: */
    { -1, NULL        , NULL         }, /* 1E: */
    { -1, NULL        , NULL         }, /* 1F: */
    { 16, copy_f80_be , copy_f80_le  }, /* 20: */

    { sizeof(short)      , NULL        , NULL         }, /* 21: */
    { sizeof(int)        , NULL        , NULL         }, /* 22: */
    { sizeof(long)       , NULL        , NULL         }, /* 23: */
    { sizeof(float)      , copy_f32_be , copy_f32_le  }, /* 24: */
    { sizeof(double)     , copy_f64_be , copy_f64_le  }, /* 25: */
    { sizeof(long double), NULL        , NULL         }, /* 26: */

    { -1, NULL        , NULL         }, /* 27: */
    { -1, NULL        , NULL         }, /* 28: */
    { -1, NULL        , NULL         }, /* 29: */
    { -1, NULL        , NULL         }, /* 2A: */
    { -1, NULL        , NULL         }, /* 2B: */
    { -1, NULL        , NULL         }, /* 2C: */
    { -1, NULL        , NULL         }, /* 2D: */
    { -1, NULL        , NULL         }, /* 2E: */
    { -1, NULL        , NULL         }, /* 2F: */
    { -1, NULL        , NULL         }, /* 30: */
    {  1, copy_alaw   , copy_alaw    }, /* 31: */
    {  1, copy_ulaw   , copy_ulaw    }, /* 32: */
    { -1, NULL        , NULL         }, /* 33: */
    { -1, NULL        , NULL         }, /* 34: */
    { -1, NULL        , NULL         }, /* 35: */
    { -1, NULL        , NULL         }, /* 36: */
    { -1, NULL        , NULL         }, /* 37: */
    { -1, NULL        , NULL         }, /* 38: */
    { -1, NULL        , NULL         }, /* 39: */
    { -1, NULL        , NULL         }, /* 3A: */
    { -1, NULL        , NULL         }, /* 3B: */
    { -1, NULL        , NULL         }, /* 3C: */
    { -1, NULL        , NULL         }, /* 3D: */
    { -1, NULL        , NULL         }, /* 3E: */
    { -1, NULL        , NULL         }, /* 3F: */
};

/*
 *  Selects the right demultiplexer from the attribute field
 *  (currently endian and type information is used, rest is done in
 *  lame_encode_pcm)
 */

static inline int  select_demux ( OUT uint32_t mode, IN demux_t* retf, IN ssize_t* size )
{
    int                  big    = mode >> 24;
    const demux_info_t*  tabptr = demux_info + ((mode >> 16) & 0x3F);

    FN("select_demux");

#ifdef WORDS_BIGENDIAN
    /* big endian */
    big  = (big >> 1) ^ big ^ 1;        // 0=big, 1=little, 2=little, 3=big
#else
    /* little endian */                 // 0=little, 1=big, 2=little, 3=big
#endif

    *size = tabptr -> size;
    *retf = big & 1  ?  tabptr -> demultiplexer_be
                     :  tabptr -> demultiplexer_le;
    return *retf != NULL  ?  0  :  -1;
}

/*}}}*/
/*{{{ amplify/resample stuff          */

/*
 *  routine to feed EXACTLY one frame (lame->frame_size) worth of data to the
 *  encoding engine. All buffering, resampling, etc, handled by calling program.
 */

static inline int  internal_lame_encoding_pcm (
        INOUT lame_t*           const lame,
        INOUT octetstream_t*    const os,
        OUT   sample_t* const * const data,
        OUT   size_t                  len )
{
    size_t           i;
    double           ampl;
    double           dampl;
    int              ampl_on;
    size_t           ch;
    size_t           mf_needed;
    int              ret;
    size_t           n_in;
    size_t           n_out;
    size_t           remaining = len;
    sample_t*        mfbuf [MAX_CHANNELS];
    const sample_t*  pdata [MAX_CHANNELS];

    FN("internal_lame_encoding_pcm");

    /// this should be moved to lame_init_params();
    // some sanity checks
#if ENCDELAY < MDCTDELAY
# error ENCDELAY is less than MDCTDELAY, see <encoder.h>
#endif
#if FFTOFFSET > BLKSIZE
# error FFTOFFSET is greater than BLKSIZE, see <encoder.h>
#endif

    mf_needed = BLKSIZE + lame->frame_size - FFTOFFSET;           // amount needed for FFT
    mf_needed = MAX ( mf_needed, 286 + 576 * (1+lame->mode_gr) ); // amount needed for MDCT/filterbank
    assert ( mf_needed <= MFSIZE );
    ///

    os -> length = 0;
    pdata [0]    = data [0];
    pdata [1]    = data [1];
    mfbuf [0]    = lame->mfbuf [0];
    mfbuf [1]    = lame->mfbuf [1];

    ampl    = lame->last_ampl;
    dampl   = 0.;
    if ( lame->ampl != ampl ) {
        if (remaining <= 1) {           // constant amplification
            ampl_on = 1;
        } else {                        // fading
            ampl_on = 2;
            dampl   = (lame->ampl - ampl) / remaining * lame->sampfreq_out / lame->sampfreq_in;
        }
        lame->last_ampl = lame->ampl;
    } else if ( lame->ampl != 1. ) {    // constant amplification
        ampl_on = 1;
    } else {                            // no manipulation (fastest)
        ampl_on = 0;
    }

    while ( (int) remaining > 0 ) {

        FN("internal_lame_encoding_pcm 3");

        /* copy in new samples into mfbuf, with resampling if necessary */
        if ( lame->resample_in != NULL )  {
	
            FN("internal_lame_encoding_pcm 10");
	
            for ( ch = 0; ch < lame->channels_out; ch++ ) {
                n_in  = remaining;
                n_out = lame->frame_size;
		
                FN("internal_lame_encoding_pcm 12");

                // resample filter virtually should have no delay !
                ret   = resample_buffer (
                            lame->resample_in,
                            mfbuf [ch] + lame->mf_size, &n_out,
                            pdata [ch]                , &n_in,
                            ch );
                if ( ret < 0 )
                    return ret;
                pdata [ch] += n_in;
            }
	    
            FN("internal_lame_encoding_pcm 13");
	    
        }
        else {
	
            FN("internal_lame_encoding_pcm 14");
	
            n_in = n_out = MIN ( lame->frame_size, remaining );

            FN("internal_lame_encoding_pcm 15");

            for ( ch = 0; ch < lame->channels_out; ch++ ) {
                memcpy (  mfbuf [ch] + lame->mf_size,
                          pdata [ch],
                          n_out * sizeof (**mfbuf) );
                pdata [ch] += n_in;
            }
	    
            FN("internal_lame_encoding_pcm 16");
	    
        }

        FN("internal_lame_encoding_pcm 4");

        switch ( ampl_on ) {
        case 0:
            break;
        case 1:
            for ( ch = 0; ch < lame->channels_out; ch++ )
                for ( i = 0; i < n_out; i++ )
                    mfbuf [ch] [lame->mf_size + i] *= ampl;
            break;
        case 2:
            for ( i = 0; i < n_out; i++, ampl += dampl )
                for ( ch = 0; ch < lame->channels_out; ch++ )
                    mfbuf [ch] [lame->mf_size + i] *= ampl;
            break;
        default:
            assert (0);
            break;
        }

        FN("internal_lame_encoding_pcm 4");

        fprintf ( stderr, "n_in=%d, n_out=%d, remaining=%d, remaining=%d\n", n_in, n_out, remaining, remaining-n_in );

        remaining                  -= n_in;
        lame->mf_size              += n_out;
        lame->mf_samples_to_encode += n_out;

        // encode ONE frame if enough data available
        if ( lame->mf_size >= mf_needed ) {
            // Enlarge octetstream buffer if (possibly) needed by (25% + 16K)
            if ( os->size < 16384 + os->length )
                octetstream_resize ( os, os->size + os->size/4 + 16384 );
            // Encode one frame
            ret = lame_encode_frame ( lame, mfbuf,
                                      os->data + os->length, os->size - os->length );

            if (ret < 0)
                return ret;
            os->length += ret;

            FN("internal_lame_encoding_pcm 5");

            // shift out old samples
            lame->mf_size              -= lame->frame_size;
            lame->mf_samples_to_encode -= lame->frame_size;
            for ( ch = 0; ch < lame->channels_out; ch++ )
                memmove ( mfbuf [ch] + 0, mfbuf [ch] + lame->frame_size, lame->mf_size * sizeof (**mfbuf) );
		
            FN("internal_lame_encoding_pcm 6");

        }
    }
    assert (remaining == 0);
    return 0;
}

/*}}}*/
/*{{{ demultiplexing stuff            */

static inline void  average ( sample_t* dst, const sample_t* src1, const sample_t* src2, size_t len )
{
    FN("average");

    while (len--)
        *dst++ = (*src1++ + *src2++) * 0.5;
}

int  lame_encode_pcm (
        lame_t* const   lame,
        octetstream_t*  os,
        const void*     pcm,
        size_t          len,
        uint32_t        flags )
{
    sample_t*           data [2];
    demux_t             retf;
    ssize_t             size;
    const uint8_t*      p;
    const uint8_t* const*  q;
    int                 ret;
    size_t              channels_in = flags & 0xFFFF;

    FN("lame_encode_pcm");

    if (len == 0)
        return 0;

    if (select_demux ( flags, &retf, &size ) < 0)
        return -1;

    data[0] = (sample_t*) calloc (sizeof(sample_t), len);
    data[1] = (sample_t*) calloc (sizeof(sample_t), len);

    switch (flags >> 28) {
    case LAME_INTERLEAVED >> 28:
        p = (const uint8_t*) pcm;
        switch ( lame->channels_out ) {
        case 1:
            switch ( channels_in ) {
            case 1:
                retf ( data[0], p+0*size, 1*size, len );
                break;
            case 2:
                retf ( data[0], p+0*size, 2*size, len );
                retf ( data[1], p+1*size, 2*size, len );
                average ( data[0], data[0], data[1], len );
                memset ( data[1], 0, sizeof(sample_t)*len );
                break;
            default:
                return -1;
                break;
            }
            break;
        case 2:
            switch ( channels_in ) {
            case 1:
                retf ( data[0], p+0*size, 1*size, len );
                memcpy ( data[1], data[0], sizeof(sample_t)*len );
                break;
            case 2:
                retf ( data[0], p+0*size, 2*size, len );
                retf ( data[1], p+1*size, 2*size, len );
                break;
            default:
                return -1;
                break;
            }
            break;
        default:
            return -1;
        }
        break;

    case LAME_CHAINED >> 28:
        p = (const uint8_t*) pcm;
        switch ( lame->channels_out ) {
        case 1:
            switch ( channels_in ) {
            case 1:
                retf ( data[0], p+0*size*len, size, len );
                break;
            case 2:
                retf ( data[0], p+0*size*len, size, len );
                retf ( data[1], p+1*size*len, size, len );
                average ( data[0], data[0], data[1], len );
                memset ( data[1], 0, sizeof(sample_t)*len );
                break;
            default:
                return -1;
                break;
            }
            break;
        case 2:
            switch ( channels_in ) {
            case 1:
                retf ( data[0], p+0*size*len, size, len );
                memcpy ( data[1], data[0], sizeof(sample_t)*len );
                break;
            case 2:
                retf ( data[0], p+0*size*len, size, len );
                retf ( data[1], p+1*size*len, size, len );
                break;
            default:
                return -1;
                break;
            }
            break;
        default:
            return -1;
        }
        break;

    case LAME_INDIRECT >> 28:
        q = (const uint8_t* const*) pcm;
        switch ( lame->channels_out ) {
        case 1:
            switch ( channels_in ) {
            case 1:
                retf ( data[0], q[0], size, len );
                break;
            case 2:
                retf ( data[0], q[0], size, len );
                retf ( data[1], q[1], size, len );
                average ( data[0], data[0], data[1], len );
                memset ( data[1], 0, sizeof(sample_t)*len );
                break;
            default:
                return -1;
                break;
            }
            break;
        case 2:
            switch ( channels_in ) {
            case 1:
                retf ( data[0], q[0], size, len );
                memcpy ( data[1], data[0], sizeof(sample_t)*len );
                break;
            case 2:
                retf ( data[0], q[0], size, len );
                retf ( data[1], q[1], size, len );
                break;
            default:
                return -1;
                break;
            }
            break;
        default:
            return -1;
        }
        break;

    default:
        return -1;
    }

    ret = internal_lame_encoding_pcm ( lame, os, (sample_t const* const*) data, len );

    free ( data[0] );
    free ( data[1] );
    return ret;
}

/*}}}*/
/*{{{ lame_encode_pcm_flush           */

int  lame_encode_pcm_flush (
        lame_t*        const  lame,
        octetstream_t* const  os )
{
    int  ret;
    int  ret_cb;

    FN("lame_encode_pcm_flush");

    ret = lame_encode_pcm ( lame, os, NULL, 
                            lame->mf_samples_to_encode + lame->frame_size, 
			    LAME_INTERLEAVED | LAME_SILENCE | 1 );


    // ugly, not encapsulated
    flush_bitstream (lame -> global_flags);       // mp3 related stuff. bit buffer might still contain some mp3 data
    id3tag_write_v1 (lame -> global_flags);       // write a ID3 tag to the bitstream
    ret_cb = copy_buffer (os->data, os->size - os->length, &lame->bs);
    if (ret_cb < 0) 
        return ret_cb;
    os->length += ret_cb;


    return ret;
}

/*
 *  Data flow
 *  ~~~~~~~~~
 *  lame_encode_buffer (public)
 *  lame_encode_buffer_interleaved (public)
 *      - do some little preparations and calls lame_encode_pcm
 *  lame_encode_pcm (public)
 *      - demultiplexing
 *      - type converting
 *      - endian handling
 *      - alignment handling
 *      - channel number converting (currently averaging or duplicating)
 *  internal_lame_encoding_pcm
 *      - resampling
 *      - level attenuator/amplifier, fader
 *      - divide input into frames
 *      - merge encoded data to one stream
 *      - does a lot of things which should be done in the setup
 *  lame_encode_frame
 *      - calls the right next stage encoder
 *      - counts the number of encoded frames
 *      - analyzer call back
 *  lame_encode_???_frame
 *      - out of the scope of this file
 */

/*}}}*/
/*{{{ Legacy stuff                    */

static lame_t*  pointer2lame ( void* const handle )
{
    lame_t*             lame = (lame_t*)handle;
    lame_global_flags*  gfp  = (lame_global_flags*)handle;

    FN("pointer2lame");

    if ( lame == NULL )
        return NULL;
    if ( lame->Class_ID == LAME_ID )
        return lame;

    if ( gfp->num_channels == 1  ||  gfp->num_channels == 2 ) {
        lame = (lame_t*) (gfp->internal_flags);

        if ( lame == NULL )
            return NULL;
        if ( lame->Class_ID == LAME_ID )
            return lame;
    }
    return NULL;
}


int  lame_encode_buffer (
        void* const    gfp,
        const int16_t  buffer_l [],
        const int16_t  buffer_r [],
        const size_t   nsamples,
        void* const    mp3buf,
        const size_t   mp3buf_size )
{
    const int16_t*  pcm [2];
    octetstream_t*  os;
    int             ret;
    lame_t*         lame;

    FN("lame_encode_buffer");

    lame    = pointer2lame (gfp);
    os      = octetstream_open (mp3buf_size);
    pcm [0] = buffer_l;
    pcm [1] = buffer_r;

    ret     = lame_encode_pcm ( lame, os, pcm, nsamples,
                                LAME_INDIRECT | LAME_NATIVE_ENDIAN | LAME_INT16 |
                                lame->channels_in );

    memcpy ( mp3buf, os->data, os->length );
    if (ret == 0) ret = os->length;
    octetstream_close (os);
    return ret;
}


int  lame_encode_buffer_interleaved (
        void* const    gfp,
        const int16_t  buffer [],
        size_t         nsamples,
        void* const    mp3buf,
        const size_t   mp3buf_size )
{
    octetstream_t*  os;
    int             ret;
    lame_t*         lame;

    FN("lame_encode_buffer_interleaved");

    lame    = pointer2lame (gfp);
    os      = octetstream_open (mp3buf_size);

    ret     = lame_encode_pcm ( lame, os, buffer, nsamples,
                                LAME_INTERLEAVED | LAME_NATIVE_ENDIAN | LAME_INT16 |
                                lame->channels_in );

    memcpy ( mp3buf, os->data, os->length );
    if (ret == 0) ret = os->length;
    octetstream_close (os);
    return ret;
}


int  lame_encode_flush (
        void* const    gfp,
        void* const    mp3buf,
        const size_t   mp3buf_size )
{
    octetstream_t*  os;
    int             ret;
    lame_t*         lame;

    FN("lame_encode_flush");

    lame    = pointer2lame (gfp);
    os      = octetstream_open (mp3buf_size);

    ret     = lame_encode_pcm_flush ( lame, os );

    memcpy ( mp3buf, os->data, os->length );
    if (ret == 0) ret = os->length;
    octetstream_close (os);
    return ret;
}

/*}}}*/
/*{{{ sin/cos/sinc/rounding functions */

static inline long  round_nearest ( long double x )
{
    if ( x >= 0. )
        return (long)(x+0.5);
    else
        return (long)(x-0.5);
}


static inline long  round_down ( long double x )
{
    if ( x >= 0. )
        return +(long)(+x);
    else
        return -(long)(-x);
}


static inline long double  sinpi ( long double x )
{
    x -= round_down (x) & ~1;

    switch ( (int)(4.*x) ) {
    default: assert (0);
    case  0: return +SIN ( M_PIl * (0.0+x) );
    case  1: return +COS ( M_PIl * (0.5-x) );
    case  2: return +COS ( M_PIl * (x-0.5) );
    case  3: return +SIN ( M_PIl * (1.0-x) );
    case  4: return -SIN ( M_PIl * (x-1.0) );
    case  5: return -COS ( M_PIl * (1.5-x) );
    case  6: return -COS ( M_PIl * (x-1.5) );
    case  7: return -SIN ( M_PIl * (2.0-x) );
    }
}


static inline long double  cospi ( long double x )
{
    x -= round_down (x) & ~1;

    switch ( (int)(4.*x) ) {
    default: assert (0);
    case  0: return +COS ( M_PIl * (x-0.0) );
    case  1: return +SIN ( M_PIl * (0.5-x) );
    case  2: return -SIN ( M_PIl * (x-0.5) );
    case  3: return -COS ( M_PIl * (1.0-x) );
    case  4: return -COS ( M_PIl * (x-1.0) );
    case  5: return -SIN ( M_PIl * (1.5-x) );
    case  6: return +SIN ( M_PIl * (x-1.5) );
    case  7: return +COS ( M_PIl * (2.0-x) );
    }
}


static inline long double  sinc ( long double x )
{
    if ( x == 0. )
        return 1.;

    if ( x <  0. )
        x = -x;

    return sinpi ( x ) / ( M_PIl * x );
}

/*}}}*/
/*{{{ some window functions           */

static inline  double  hanning ( double x )
{
    if ( fabs (x) >= 1 )
        return 0.;

    x = cospi (0.5 * x);
    return x * x;
}

static inline  double  hamming ( double x )
{
    if ( fabs (x) >= 1 )
        return 0.;

    x = cospi (x);
    return 0.54 + 0.46 * x;
}

static inline  double  blackman ( double x )
{
    if ( fabs (x) >= 1 )
        return 0.;

    x = cospi (x);
    return (0.16 * x + 0.50) * x + 0.34;        // using addition theorem of arc functions
}

static inline  double  blackman1 ( double x )
{
    if ( fabs (x) >= 1 )
        return 0.;

    x += 1.;
    return 0.42 - 0.50*cospi(x) + 0.08*cospi(2*x);
}

static inline  double  blackman2 ( double x )
{
    if ( fabs (x) >= 1 )
        return 0.;

    x += 1.;
    return 0.375 - 0.50*cospi(x) + 0.125*cospi(2*x);
}

static inline  double  blackmanharris_nuttall ( double x )
{
    if ( fabs (x) >= 1 )
        return 0.;

    x += 1.;
    return (10 - 15*cospi (x) + 6*cospi (2*x) - cospi (3*x) ) * (1./32);
}

static inline  double  blackmanharris_min4 ( double x )
{
    if ( fabs (x) >= 1 )
        return 0.;

    x += 1.;
    return 0.355768 - 0.487396*cospi (x) + 0.144232*cospi (2*x) - 0.012604*cospi (3*x);
}

/*
 * Blackman-Harris windows, which have the general equation:
 *
 *          w(n) = a0 - a1 cos(2pn/N) + a2 cos(4pn/N) - a3 cos(6pn/N).
 *
 * When Nuttall set the ak coefficients to the "minimum 4-term" values of a0 = 0.355768, a1 =
 * 0.487396, a2 = 0.144232, and a3 = 0.012604, he obtained the window and the frequency
 * response shown in Figure 8. The minimum-4-term window loses some frequency
 * resolution because it has a wide main lobe, but the first side lobe drops to -93 db.
 *
 * If you require fast side-lobe rolloff in an application, consider Nuttall's Equation (30). It
 * applies the following coefficients to the general equation above: a0 = 10/32, a1 = 15/32, a2 =
 * 6/32, and a3 = 1/32. Figure 8 also includes the window and frequency response for Nuttall's
 * Equation (30). This window has a first side-lobe level of -61 dB and a significant side-lobe
 * rolloff of -42 dB/octave.
 *
 * The first investigators of windowing functions determined that window functions should
 * have zero values at their boundaries and so should their successive derivatives. If a
 * window's kth derivative is zero at the boundaries, the peaks of the window's side lobes will
 * decay at a rate of 6(k+2) dB/octave. T&MW.
 */

/*}}}*/
/*{{{ scalar stuff                    */


/**********************************************************
 *                                                        *
 *           Functions taken from scalar.nas              *
 *                                                        *
 **********************************************************/

/*
 * The scalarxx versions with xx=04,08,12,16,20,24 are fixed size for xx element scalars
 * The scalar1n version is suitable for every non negative length
 * The scalar4n version is only suitable for positive lengths which are a multiple of 4
 *
 * The following are equivalent:
 *    scalar12 (p, q);
 *    scalar4n (p, q, 3);
 *    scalar1n (p, q, 12);
 */

float_t  scalar04_float32 ( const sample_t* p, const sample_t* q )
{
    return  p[0]*q[0] + p[1]*q[1] + p[2]*q[2] + p[3]*q[3];
}

float_t  scalar08_float32 ( const sample_t* p, const sample_t* q )
{
    return  p[0]*q[0] + p[1]*q[1] + p[2]*q[2] + p[3]*q[3]
          + p[4]*q[4] + p[5]*q[5] + p[6]*q[6] + p[7]*q[7];
}

float_t  scalar12_float32 ( const sample_t* p, const sample_t* q )
{
    return  p[0]*q[0] + p[1]*q[1] + p[ 2]*q[ 2] + p[ 3]*q[ 3]
          + p[4]*q[4] + p[5]*q[5] + p[ 6]*q[ 6] + p[ 7]*q[ 7]
          + p[8]*q[8] + p[9]*q[9] + p[10]*q[10] + p[11]*q[11];
}

float_t  scalar16_float32 ( const sample_t* p, const sample_t* q )
{
    return  p[ 0]*q[ 0] + p[ 1]*q[ 1] + p[ 2]*q[ 2] + p[ 3]*q[ 3]
          + p[ 4]*q[ 4] + p[ 5]*q[ 5] + p[ 6]*q[ 6] + p[ 7]*q[ 7]
          + p[ 8]*q[ 8] + p[ 9]*q[ 9] + p[10]*q[10] + p[11]*q[11]
          + p[12]*q[12] + p[13]*q[13] + p[14]*q[14] + p[15]*q[15];
}

float_t  scalar20_float32 ( const sample_t* p, const sample_t* q )
{
    return  p[ 0]*q[ 0] + p[ 1]*q[ 1] + p[ 2]*q[ 2] + p[ 3]*q[ 3]
          + p[ 4]*q[ 4] + p[ 5]*q[ 5] + p[ 6]*q[ 6] + p[ 7]*q[ 7]
          + p[ 8]*q[ 8] + p[ 9]*q[ 9] + p[10]*q[10] + p[11]*q[11]
          + p[12]*q[12] + p[13]*q[13] + p[14]*q[14] + p[15]*q[15]
          + p[16]*q[16] + p[17]*q[17] + p[18]*q[18] + p[19]*q[19];
}

float_t  scalar24_float32 ( const sample_t* p, const sample_t* q )
{
    return  p[ 0]*q[ 0] + p[ 1]*q[ 1] + p[ 2]*q[ 2] + p[ 3]*q[ 3]
          + p[ 4]*q[ 4] + p[ 5]*q[ 5] + p[ 6]*q[ 6] + p[ 7]*q[ 7]
          + p[ 8]*q[ 8] + p[ 9]*q[ 9] + p[10]*q[10] + p[11]*q[11]
          + p[12]*q[12] + p[13]*q[13] + p[14]*q[14] + p[15]*q[15]
          + p[16]*q[16] + p[17]*q[17] + p[18]*q[18] + p[19]*q[19]
          + p[20]*q[20] + p[21]*q[21] + p[22]*q[22] + p[23]*q[23];
}

float_t  scalar32_float32 ( const sample_t* p, const sample_t* q )
{
    return  p[ 0]*q[ 0] + p[ 1]*q[ 1] + p[ 2]*q[ 2] + p[ 3]*q[ 3]
          + p[ 4]*q[ 4] + p[ 5]*q[ 5] + p[ 6]*q[ 6] + p[ 7]*q[ 7]
          + p[ 8]*q[ 8] + p[ 9]*q[ 9] + p[10]*q[10] + p[11]*q[11]
          + p[12]*q[12] + p[13]*q[13] + p[14]*q[14] + p[15]*q[15]
          + p[16]*q[16] + p[17]*q[17] + p[18]*q[18] + p[19]*q[19]
          + p[20]*q[20] + p[21]*q[21] + p[22]*q[22] + p[23]*q[23]
          + p[24]*q[24] + p[25]*q[25] + p[26]*q[26] + p[27]*q[27]
          + p[28]*q[28] + p[29]*q[29] + p[30]*q[30] + p[31]*q[31];
}

float_t  scalar4n_float32 ( const sample_t* p, const sample_t* q, size_t len )
{
    double sum = p[0]*q[0] + p[1]*q[1] + p[2]*q[2] + p[3]*q[3];

    while (--len) {
        p   += 4;
        q   += 4;
        sum += p[0]*q[0] + p[1]*q[1] + p[2]*q[2] + p[3]*q[3];
    }
    return sum;
}

float_t  scalar1n_float32 ( const sample_t* p, const sample_t* q, size_t len )
{
    float_t sum;

    switch (len & 3) {
    case 0:
        sum  = 0.;
        break;
    case 1:
        sum  = p[0]*q[0];
        p   += 1;
        q   += 1;
        break;
    case 2:
        sum  = p[0]*q[0] + p[1]*q[1];
        p   += 2;
        q   += 2;
        break;
    default:
        sum  = p[0]*q[0] + p[1]*q[1] + p[2]*q[2];
        p   += 3;
        q   += 3;
        break;
    }
    
    for ( len >>= 2; len--; ) {
        sum += p[0]*q[0] + p[1]*q[1] + p[2]*q[2] + p[3]*q[3];
        p   += 4;
        q   += 4;
    }
    return sum;
}


scalar_t   scalar04 = scalar04_float32;
scalar_t   scalar08 = scalar08_float32;
scalar_t   scalar12 = scalar12_float32;
scalar_t   scalar16 = scalar16_float32;
scalar_t   scalar20 = scalar20_float32;
scalar_t   scalar24 = scalar24_float32;
scalar_t   scalar32 = scalar32_float32;
scalarn_t  scalar4n = scalar4n_float32;
scalarn_t  scalar1n = scalar1n_float32;


void  init_scalar_functions ( OUT lame_t* const lame )
{
    if ( lame -> CPU_features.i387 ) {
        scalar04 = scalar04_float32_i387;
        scalar08 = scalar08_float32_i387;
        scalar12 = scalar12_float32_i387;
        scalar16 = scalar16_float32_i387;
        scalar20 = scalar20_float32_i387;
        scalar24 = scalar24_float32_i387;
        scalar32 = scalar32_float32_i387;
        scalar4n = scalar4n_float32_i387;
        scalar1n = scalar1n_float32_i387;
    }

    if ( lame -> CPU_features.AMD_3DNow ) {
        scalar04 = scalar04_float32_3DNow;
        scalar08 = scalar08_float32_3DNow;
        scalar12 = scalar12_float32_3DNow;
        scalar16 = scalar16_float32_3DNow;
        scalar20 = scalar20_float32_3DNow;
        scalar24 = scalar24_float32_3DNow;
        scalar32 = scalar32_float32_3DNow;
        scalar4n = scalar4n_float32_3DNow;
        scalar1n = scalar1n_float32_3DNow;
    }

    if ( lame -> CPU_features.SIMD ) {
        scalar04 = scalar04_float32_SIMD;
        scalar08 = scalar08_float32_SIMD;
        scalar12 = scalar12_float32_SIMD;
        scalar16 = scalar16_float32_SIMD;
        scalar20 = scalar20_float32_SIMD;
        scalar24 = scalar24_float32_SIMD;
        scalar32 = scalar32_float32_SIMD;
        scalar4n = scalar4n_float32_SIMD;
        scalar1n = scalar1n_float32_SIMD;
    }
}

/*}}}*/
/*{{{ factorize                       */

/*
 *  Tries to find the best integral ratio for two sampling frequencies
 *  It searches the best ratio a:b with b <= MAX_TABLES
 */

static double  factorize ( 
        OUT long double  f1, 
	OUT long double  f2, 
	IN  int* const   x1, 
	IN  int* const   x2 )
{
    unsigned     i;
    long         ltmp;
    long double  ftmp;
    double       minerror = 1.;
    double       abserror = 1.;

    assert ( f1 > 0. );
    assert ( f2 > 0. );
    assert ( x1 != NULL );
    assert ( x2 != NULL );

    for ( i = 1; i <= MAX_TABLES; i++ ) {
        ftmp  = f2 * i / f1;
        ltmp  = (long) ( ftmp + 0.5 );
        if ( fabs ( (ltmp-ftmp)/ftmp ) < minerror ) {
            *x1      = i;
            *x2      = (int)ltmp;
            abserror = (ltmp-ftmp) / ftmp;
            minerror = 0.9999847412109375 * fabs (abserror);
        }
    }
    return abserror;
}

/*
 *  Some programmers and some file format only supporting integral sampling frequencies
 *  This patch tries to find the right sampling frequency for some know rounding victims.
 *  Better is to support directly 64 or 80 bit FPU precision.
 */

long double  unround_samplefrequency ( OUT long double freq )
{
    if ( freq != (unsigned short)freq )
        return freq;

    switch ( (unsigned short)freq ) {
    case 48662: case 48663: return 1411200/ 29.L;  // SB 128 "48 kHz"         48662.06896551724137931034
    case 44055: case 44056: return 2863636/ 65.L;  // NTSC PCM/LD PCM         44055.93846153846153846153
    case 32072: case 32073: return 1411200/ 44.L;  // SB 128 "32 kHz"         32072.72727272727272727272
    case 31468: case 31469: return 2863636/ 91.L;  // NTSC Hi8 Digital Audio  31468.52747252747252747252
    case 23918: case 23919: return 1411200/ 59.L;  // SB 128 "24 kHz"         23918.64406779661016949152
    case 22254: case 22255: return  244800/ 11.L;  // MAC HQ                  22254.54545454545454545454
    case 16036: case 16037: return 1411200/ 88.L;  // SB 128 "16 kHz"         16036.36363636363636363636
    case 11959: case 11960: return 1411200/118.L;  // SB 128 "12 kHz"         11959.32203389830508474576
    case 11127: case 11128: return  122400/ 11.L;  // MAC LQ                  11127.27272727272727272727
    case  8018: case  8019: return 1411200/176.L;  // SB 128  "8 kHz"          8018.18181818181818181818
    case  8012: case  8013: return  312500/ 39.L;  // NeXT/Telco               8012.82051282051282051282
    case  5512: case  5513: return   44100/  8.L;  // 1/8 CD                   5512.50000000000000000000
    }

    return freq;
}

/*}}}*/
/*{{{ resampling stuff                */

static inline void  Calc_Coeffs (
        IN  sample_t*  Coeff,
        OUT int        iNew,
        OUT int        iOld,
        OUT long       i,
        OUT long       k,
        OUT double     bandwidth )   // 0.0 ... 1.0: used bandwidth from 0...fs_out/2
{
    long double  w   = 1.L / (WINDOW_SIZE + 0.5);
    long double  sum = 0.;
    long double  tmp1;
    long double  tmp2;
    long         j;

    for ( j = 0; j <= 2*WINDOW_SIZE; j++ ) {
        tmp1 = ((k+j)*iNew - i*iOld) / (long double)iNew * bandwidth;
        tmp2 = ((k+j)*iNew - i*iOld) / (long double)iNew;
        sum += Coeff [j] = sinc (tmp1) * WINDOW (tmp2 * w);
    }

    for ( j = 0; j <= 2*WINDOW_SIZE; j++ )
        Coeff [j] /= sum;
}


resample_t*  resample_open (
        OUT long double  sampfreq_in,    // [Hz]
        OUT long double  sampfreq_out,   // [Hz]
        OUT double       lowpass_freq,   // [Hz] or <0 for auto mode
        OUT int          quality )       // Proposal: 0: default, 1: sample select, 2: linear interpolation
                                         //           4: 4-point interpolation, 32: 32-point interpolation
{
    resample_t* const  ret = calloc ( sizeof(resample_t), 1 );
    long               i;
    sample_t*          p;
    double             err;

    FN("resample_open");

    if ( sampfreq_in == sampfreq_out )
        return NULL;

    err = factorize ( sampfreq_out, sampfreq_in, &(ret->scale_out), &(ret->scale_in) );
    fprintf ( stderr, "(%.6g => %.6g Hz, Ratio %d:%d",
              (double)sampfreq_in, (double)sampfreq_out, ret->scale_in, ret->scale_out );
    if ( fabs (err) > 5.e-10 )                                            // 16 msec/year
        fprintf ( stderr, ", Error %+.3f ppm", 1.e6*err );
    fprintf ( stderr, ")\n" );
    fflush  ( stderr );

    if ( ret->scale_out == ret->scale_in )
        return NULL;

    ret->Class_ID        = RESAMPLE_ID;
    ret->samplefreq_in   = sampfreq_in;
    ret->samplefreq_out  = sampfreq_out;
    ret->taps            = TAPS;
    ret->lowpass_freq    = lowpass_freq < 0.  ?  0.50 * MIN (sampfreq_in, sampfreq_out)  :  lowpass_freq;
    ret->in_old [0]      = calloc ( sizeof(sample_t*)     , 2*TAPS );
    ret->in_old [1]      = calloc ( sizeof(sample_t*)     , 2*TAPS );
    ret->src_step        = calloc ( sizeof(unsigned char*), (unsigned)ret->scale_out );
    ret->fir             = calloc ( sizeof(sample_t*)     , (unsigned)ret->scale_out );
    ret->firfree         = calloc ( sizeof(sample_t)      , TAPS * ret->scale_out + 64/sizeof(sample_t) );
    p                    = (sample_t*) ( ((ptrdiff_t)(ret->firfree) | 63) + 1 ); /* 64 byte alignment */

    for ( i = 0; i < ret->scale_out; i++, p += TAPS ) {
        ret->src_step [i] = round_nearest ( (i+1.L) * ret->scale_in / ret->scale_out )
                          - round_nearest ( (i+0.L) * ret->scale_in / ret->scale_out );
                          
        Calc_Coeffs ( ret->fir [i] = p,
                      ret->scale_out, ret->scale_in,
                      i, round_nearest ( (double)i * ret->scale_in / ret->scale_out - WINDOW_SIZE ),
                      2.*ret->lowpass_freq / sampfreq_out );
    }

    return ret;
}


/* Should be done with more sanity checks */

int  resample_close ( INOUT resample_t* const r )
{
    FN("resample_close");

    if ( r == NULL )
        return -1;
    free ( r->in_old [0] );
    free ( r->in_old [1] );
    free ( r->fir [0]    );
    free ( r->fir        );
    free ( r->src_step   );
    return 0;
}


// in the future some of the copies should be avoided by usage of mfbuf
// also by the fill_buffer_resample() routine

/*
 *  Current working scheme:
 *
 *  | old | new                data in in_buf, consist of old *and new data
 *  |0 1 2|3 4 5 6 ...
 *
 *   <--1-->
 *     <--2-->
 *       <--3-->
 *
 *        | new                data in in, only new data
 *        |0 1 2 3 4 5 6
 *         <--4-->
 *           <--5-->
 *             <--6-->
 */

int  resample_buffer (                          // return code, 0 for success
        INOUT resample_t *const   r,            // internal structure
        IN    sample_t   *const   out,          // where to write the output data
        INOUT size_t     *const   out_req_len,  // requested output data len/really written output data len
        OUT   sample_t   *const   in,           // where are the input data?
        INOUT size_t     *const   in_avail_len, // available input data len/consumed input data len
        OUT   size_t              channel )     // number of the channel (needed for internal buffering)
{
    sample_t*  p           = out;
    sample_t*  in_old      = r->in_old [channel];
    size_t     len         = *in_avail_len;
    size_t     desired_len = *out_req_len;
    size_t     i;
    size_t     k;
    int        l;

    FN("resample_buffer");

    // copy TAPS samples to the second half of in_old
    memcpy ( in_old + TAPS, in, MIN (len*sizeof(*in), TAPS*sizeof(*in)) );

    // calculating k and l
    l = r->fir_stepper [channel];
    k = r->inp_stepper [channel];

    // doing converting for all samples which can be done in 'in_old'
    for ( i = 0; i < desired_len  &&  k < TAPS; i++ ) {
        *p++ = scalar32 ( in_old + k, r->fir [l] );
        k   += r->src_step [l];
        if ( ++l >= r->scale_out ) l = 0;
    }

    // doing converting for all samples which can be done in 'in'
    k -= TAPS;
    for ( ; i < desired_len  &&  k < len - TAPS; i++ ) {
        *p++ = scalar32 ( in + k, r->fir [l] );
        k   += r->src_step [l];
        if ( ++l >= r->scale_out ) l = 0;
    }

    // writing back processed in and out
    *in_avail_len  = k - r->inp_stepper [channel];
    *out_req_len   = p - out;

    r->fir_stepper [channel] = l;
    r->inp_stepper [channel] = k - r->inp_stepper [channel] - len;

    // make of copy of the overlap
    if ( len >= TAPS )
        memcpy  ( in_old, in + len - TAPS, TAPS*sizeof(sample_t) );
    else
        memmove ( in_old, in_old + len   , TAPS*sizeof(sample_t) );

    return 0;
}

/*}}}*/

#endif /* KLEMM_44 */

/* end of pcm.c */

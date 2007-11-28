/*{{{ #defines                        */

#include <limits.h>
#include "lame.h"
#include "util.h"

#define ENCDELAY      576
#define MDCTDELAY     48
#define BLKSIZE       1024
#define HBLKSIZE      (BLKSIZE/2 + 1)
#define BLKSIZE_s     256
#define HBLKSIZE_s    (BLKSIZE_s/2 + 1)
#define MAX_TABLES    1002
#define TAPS          32
#define WINDOW_SIZE   15.5
#define WINDOW        hanning
#define inline        __inline

#define MIN(a,b)      ((a) < (b) ? (a) : (b))
#define MAX(a,b)      ((a) > (b) ? (a) : (b))

#ifndef M_PIl
# define M_PIl          3.1415926535897932384626433832795029L
#endif

#define SIN             sin
#define COS             cos

/*}}}*/
/*{{{ object ID's                     */

#define RESAMPLE_ID     0x52455341LU
#define PSYCHO_ID       0x50535943LU
#define BITSTREAM_ID    0x42495453LU

/*}}}*/
/*{{{ typedef's                       */

typedef float         float32_t;                   // IEEE-754 32 bit floating point
typedef double        float64_t;                   // IEEE-754 64 bit floating point
typedef long double   float80_t;                   // IEEE-854 80 bit floating point, if available
typedef long double   float_t;                     // temporarly results of float operations
typedef long double   double_t;                    // temporarly results of double operations
typedef long double   longdouble_t;                // temporarly results of long double operations

typedef float_t (*scalar_t)  ( const sample_t* p, const sample_t* q );
typedef float_t (*scalarn_t) ( const sample_t* p, const sample_t* q, size_t len );

/*}}}*/
/*{{{ data direction attributes       */

/*
 * These are data stream direction attributes like used in Ada83/Ada95 and in RPC
 * The data direction is seen from the caller to the calling function.
 * Examples:
 *
 *    size_t  fread  ( void INOUT* buffer, size_t items, size_t itemsize, FILE INOUT* fp );
 *    size_t  fwrite ( void OUT  * buffer, size_t items, size_t itemsize, FILE INOUT* fp );
 *    size_t  memset ( void IN   * buffer, unsigned char value, size_t size );
 *
 * Return values are implizit IN (note that here C uses the opposite attribute).
 * Arguments not transmitted via references are implizit OUT.
 */

#define OUT            /* [out]   */ const
#define INOUT          /* [inout] */
#define IN             /* [in]    */
#define OUTTR          /* [out]: data is modified like [inout], but you don't get any useful back */

/*}}}*/
/*{{{ Test some error conditions      */

#ifndef __LOC__
# define _STR2(x)      #x
# define _STR1(x)      _STR2(x)
# define __LOC__       __FILE__ "(" _STR1(__LINE__) ") : warning: "
#endif

/* The current code doesn't work on machines with non 8 bit char's in any way, so abort */

#if CHAR_BIT != 8
# pragma message ( __LOC__ "Machines with CHAR_BIT != 8 not yet supported" )
# pragma error
#endif

/*}}}*/

/*
 *  Now some information how PCM data can be specified. PCM data
 *  is specified by 3 attributes: pointer, length information
 *  and attributes.
 *    - Audio is always stored in 2D arrays, which are collapsing to 1D
 *      in the case of monaural input
 *    - 2D arrays can be stored as 2D arrays or as pointers to 1D arrays.
 *    - 2D data can be stored as samples*channels or as channels*samples
 *    - This gives 4 kinds of storing PCM data:
 *      + pcm [samples][channels]       (LAME_INTERLEAVED)
 *      + pcm [channels][samples]       (LAME_CHAINED)
 *      + (*pcm) [samples]              (LAME_INDIRECT)
 *      + (*pcm) [channels]
 *    - The last I have never seen and it have a huge overhead (67% ... 200%),
 *      so the first three are implemented.
 *    - The monaural 1D cases can also be handled by the first two attributes
 */

#define  LAME_INTERLEAVED       0x10000000
#define  LAME_CHAINED           0x20000000
#define  LAME_INDIRECT          0x30000000

/*
 *  Now we need some information about the byte order of the data.
 *  There are 4 cases possible (if you are not fully support such strange
 *  Machines like the PDPs):
 *    - You know the absolute byte order of the data (LAME_LITTLE_ENDIAN, LAME_BIG_ENDIAN)
 *    - You know the byte order from the view of the current machine
 *      (LAME_NATIVE_ENDIAN, LAME_OPPOSITE_ENDIAN)
 *    - The use of LAME_OPPOSITE_ENDIAN is NOT recommended because it is
 *      is a breakable attribute, use LAME_LITTLE_ENDIAN or LAME_BIG_ENDIAN
 *      instead
 */

#define  LAME_NATIVE_ENDIAN     0x00000000
#define  LAME_OPPOSITE_ENDIAN   0x01000000
#define  LAME_LITTLE_ENDIAN     0x02000000
#define  LAME_BIG_ENDIAN        0x03000000

/*
 *  The next attribute is the data type of the input data.
 *  There are currently 2 kinds of input data:
 *    - C based:
 *          LAME_{SHORT,INT,LONG}
 *          LAME_{FLOAT,DOUBLE,LONGDOUBLE}
 *    - Binary representation based:
 *          LAME_{UINT,INT}{8,16,24,32}
 *          LAME_{A,U}LAW
 *          LAME_FLOAT{32,64,80_ALIGN{2,4,8}}
 *
 *  Don't use the C based for external data.
 */

#define  LAME_SILENCE           0x00010000
#define  LAME_UINT8             0x00020000
#define  LAME_INT8              0x00030000
#define  LAME_UINT16            0x00040000
#define  LAME_INT16             0x00050000
#define  LAME_UINT24            0x00060000
#define  LAME_INT24             0x00070000
#define  LAME_UINT32            0x00080000
#define  LAME_INT32             0x00090000
#define  LAME_FLOAT32           0x00140000
#define  LAME_FLOAT64           0x00180000
#define  LAME_FLOAT80_ALIGN2    0x001A0000
#define  LAME_FLOAT80_ALIGN4    0x001C0000
#define  LAME_FLOAT80_ALIGN8    0x00200000
#define  LAME_SHORT             0x00210000
#define  LAME_INT               0x00220000
#define  LAME_LONG              0x00230000
#define  LAME_FLOAT             0x00240000
#define  LAME_DOUBLE            0x00250000
#define  LAME_LONGDOUBLE        0x00260000
#define  LAME_ALAW              0x00310000
#define  LAME_ULAW              0x00320000

/*
 *  The last attribute is the number of input channels. Currently
 *  1...65535 channels are possible, but only 1 and 2 are supported.
 *  So matrixing or MPEG-2 MultiChannelSupport are not a big problem.
 *
 *  Note: Some people use the word 'stereo' for 2 channel stereophonic sound.
 *        But stereophonic sound is a collection of ALL methods to restore the
 *        stereophonic sound field starting from 2 channels up to audio
 *        holography.
 */

#define LAME_MONO               0x00000001
#define LAME_STEREO             0x00000002
#define LAME_STEREO_2_CHANNELS  0x00000002
#define LAME_STEREO_3_CHANNELS  0x00000003
#define LAME_STEREO_4_CHANNELS  0x00000004
#define LAME_STEREO_5_CHANNELS  0x00000005
#define LAME_STEREO_6_CHANNELS  0x00000006
#define LAME_STEREO_7_CHANNELS  0x00000007
#define LAME_STEREO_65535_CHANNELS\
                                0x0000FFFF


extern scalar_t   scalar4;
extern scalar_t   scalar8;
extern scalar_t   scalar12;
extern scalar_t   scalar16;
extern scalar_t   scalar20;
extern scalar_t   scalar24;
extern scalar_t   scalar32;
extern scalarn_t  scalar4n;
extern scalarn_t  scalar1n;

float_t  scalar04_float32_i387  ( const float32_t* p, const float32_t* q );
float_t  scalar08_float32_i387  ( const float32_t* p, const float32_t* q );
float_t  scalar12_float32_i387  ( const float32_t* p, const float32_t* q );
float_t  scalar16_float32_i387  ( const float32_t* p, const float32_t* q );
float_t  scalar20_float32_i387  ( const float32_t* p, const float32_t* q );
float_t  scalar24_float32_i387  ( const float32_t* p, const float32_t* q );
float_t  scalar32_float32_i387  ( const float32_t* p, const float32_t* q );
float_t  scalar4n_float32_i387  ( const float32_t* p, const float32_t* q, const size_t len );
float_t  scalar1n_float32_i387  ( const float32_t* p, const float32_t* q, const size_t len );

float_t  scalar04_float32_3DNow ( const float32_t* p, const float32_t* q );
float_t  scalar08_float32_3DNow ( const float32_t* p, const float32_t* q );
float_t  scalar12_float32_3DNow ( const float32_t* p, const float32_t* q );
float_t  scalar16_float32_3DNow ( const float32_t* p, const float32_t* q );
float_t  scalar20_float32_3DNow ( const float32_t* p, const float32_t* q );
float_t  scalar24_float32_3DNow ( const float32_t* p, const float32_t* q );
float_t  scalar32_float32_3DNow ( const float32_t* p, const float32_t* q );
float_t  scalar4n_float32_3DNow ( const float32_t* p, const float32_t* q, const size_t len );
float_t  scalar1n_float32_3DNow ( const float32_t* p, const float32_t* q, const size_t len );

float_t  scalar04_float32_SIMD  ( const float32_t* p, const float32_t* q );
float_t  scalar08_float32_SIMD  ( const float32_t* p, const float32_t* q );
float_t  scalar12_float32_SIMD  ( const float32_t* p, const float32_t* q );
float_t  scalar16_float32_SIMD  ( const float32_t* p, const float32_t* q );
float_t  scalar20_float32_SIMD  ( const float32_t* p, const float32_t* q );
float_t  scalar24_float32_SIMD  ( const float32_t* p, const float32_t* q );
float_t  scalar32_float32_SIMD  ( const float32_t* p, const float32_t* q );
float_t  scalar4n_float32_SIMD  ( const float32_t* p, const float32_t* q, const size_t len );
float_t  scalar1n_float32_SIMD  ( const float32_t* p, const float32_t* q, const size_t len );

float_t  scalar04_float32       ( const float32_t* p, const float32_t* q );
float_t  scalar08_float32       ( const float32_t* p, const float32_t* q );
float_t  scalar12_float32       ( const float32_t* p, const float32_t* q );
float_t  scalar16_float32       ( const float32_t* p, const float32_t* q );
float_t  scalar20_float32       ( const float32_t* p, const float32_t* q );
float_t  scalar24_float32       ( const float32_t* p, const float32_t* q );
float_t  scalar32_float32       ( const float32_t* p, const float32_t* q );
float_t  scalar4n_float32       ( const float32_t* p, const float32_t* q, const size_t len );
float_t  scalar1n_float32       ( const float32_t* p, const float32_t* q, const size_t len );


/*{{{ some prototypes                 */

resample_t*  resample_open (
        OUT long double  sampfreq_in,    // [Hz]
        OUT long double  sampfreq_out,   // [Hz]
        OUT double       lowpass_freq,   // [Hz] or <0 for auto mode
        OUT int          quality );      // Proposal: 0 default, 1 sample select, 2 linear interpol, 4 4-point interpolation, 32 32-point interpolation
        
int  resample_buffer (                          // return code, 0 for success
        INOUT resample_t *const   r,            // internal structure
        IN    sample_t   *const   out,          // where to write the output data
        INOUT size_t     *const   out_req_len,  // requested output data len/really written output data len
        OUT   sample_t   *const   in,           // where are the input data?
        INOUT size_t     *const   in_avail_len, // available input data len/consumed input data len
        OUT   size_t              channel );    // number of the channel (needed for buffering)

int  resample_close ( INOUT resample_t* const r );

void         init_scalar_functions   ( OUT lame_t* const lame );
long double  unround_samplefrequency ( OUT long double freq );

#if 0
int  lame_encode_mp3_frame (             // return code, 0 for success
        INOUT lame_global_flags*,        // internal context structure
        OUTTR sample_t * inbuf_l,        // data for left  channel
        OUTTR sample_t * inbuf_r,        // data for right channel
        IN    uint8_t  * mp3buf,         // where to write the coded data
        OUT   size_t     mp3buf_size );  // maximum size of coded data
#endif

int  lame_encode_ogg_frame (             // return code, 0 for success
        INOUT lame_global_flags*,        // internal context structure
        OUT   sample_t * inbuf_l,        // data for left  channel
        OUT   sample_t * inbuf_r,        // data for right channel
        IN    uint8_t  * mp3buf,         // where to write the coded data
        OUT   size_t     mp3buf_size );  // maximum size of coded data

/*}}}*/

/* end of pcm.h */

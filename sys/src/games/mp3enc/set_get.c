/* -*- mode: C; mode: fold -*- */
/*
 * set/get functions for lame_global_flags
 *
 * Copyright (c) 2001 Alexander Leidinger
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* $Id: set_get.c,v 1.2 2001/03/12 04:38:35 markt Exp $ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include "lame.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


/*
 * input stream description
 */

/* number of samples */
/* it's unlikely for this function to return an error */
int
lame_set_num_samples( lame_global_flags*  gfp,
                      unsigned long       num_samples)
{
    /* default = 2^32-1 */

    gfp->num_samples = num_samples;

    return 0;
}

unsigned long
lame_get_num_samples( const lame_global_flags* gfp )
{
    return gfp->num_samples;
}


/* input samplerate */
int
lame_set_in_samplerate( lame_global_flags*  gfp,
                        int                 in_samplerate )
{
    /* input sample rate in Hz,  default = 44100 Hz */
    gfp->in_samplerate = in_samplerate;

    return 0;
}

int
lame_get_in_samplerate( const lame_global_flags*  gfp )
{
    return gfp->in_samplerate;
}


/* number of channels in input stream */
int
lame_set_num_channels( lame_global_flags*  gfp,
                       int                 num_channels )
{
    /* default = 2 */

    if ( 2 < num_channels || 0 == num_channels )
        return -1;    /* we didn't support more than 2 channels */

    gfp->num_channels = num_channels;

    return 0;
}

int
lame_get_num_channels( const lame_global_flags*  gfp )
{
    return gfp->num_channels;
}


/* scale the input by this amount before encoding (not used for decoding) */
int
lame_set_scale( lame_global_flags*  gfp,
                float               scale )
{
    /* default = 0 */
    gfp->scale = scale;

    return 0;
}

float
lame_get_scale( const lame_global_flags*  gfp )
{
    return gfp->scale;
}


/* output sample rate in Hz */
int
lame_set_out_samplerate( lame_global_flags*  gfp,
                         int                 out_samplerate )
{
    /*
     * default = 0: LAME picks best value based on the amount
     *              of compression
     * MPEG only allows:
     *  MPEG1    32, 44.1,   48khz
     *  MPEG2    16, 22.05,  24
     *  MPEG2.5   8, 11.025, 12
     *
     * (not used by decoding routines)
     */
    gfp->out_samplerate = out_samplerate;

    return 0;
}

int
lame_get_out_samplerate( const lame_global_flags*  gfp )
{
    return gfp->out_samplerate;
}




/*
 * general control parameters
 */

/*collect data for an MP3 frame analzyer */
int
lame_set_analysis( lame_global_flags*  gfp,
                   int                 analysis )
{
    /* default = 0 */

    /* enforce disable/enable meaning, if we need more than two values
       we need to switch to an enum to have an apropriate representation
       of the possible meanings of the value */
    if ( 0 > analysis || 1 < analysis )
        return -1;

    gfp->analysis = analysis;

    return 0;
}

int
lame_get_analysis( const lame_global_flags*  gfp )
{
    assert( 0 <= gfp->analysis && 1 >= gfp->analysis );

    return gfp->analysis;
}


/* write a Xing VBR header frame */
int
lame_set_bWriteVbrTag( lame_global_flags*  gfp,
                       int bWriteVbrTag )
{
    /* default = 1 (on) for VBR/ABR modes, 0 (off) for CBR mode */

    /* enforce disable/enable meaning, if we need more than two values
       we need to switch to an enum to have an apropriate representation
       of the possible meanings of the value */
    if ( 0 > bWriteVbrTag || 1 < bWriteVbrTag )
        return -1;

    gfp->bWriteVbrTag = bWriteVbrTag;

    return 0;
}

int
lame_get_bWriteVbrTag( const lame_global_flags*  gfp )
{
    assert( 0 <= gfp->bWriteVbrTag && 1 >= gfp->bWriteVbrTag );

    return gfp->bWriteVbrTag;
}


/* disable writing a wave header with *decoding* */
int
lame_set_disable_waveheader( lame_global_flags*  gfp,
                             int                 disable_waveheader )
{
    /* default = 0 (disabled) */

    /* enforce disable/enable meaning, if we need more than two values
       we need to switch to an enum to have an apropriate representation
       of the possible meanings of the value */
    if ( 0 > disable_waveheader || 1 < disable_waveheader )
        return -1;

    gfp->disable_waveheader = disable_waveheader;

    return 0;
}

int
lame_get_disable_waveheader( const lame_global_flags*  gfp )
{
    assert( 0 <= gfp->disable_waveheader && 1 >= gfp->disable_waveheader );

    return gfp->disable_waveheader;
}


/* decode only, use lame/mpglib to convert mp3/ogg to wav */
int
lame_set_decode_only( lame_global_flags*  gfp,
                      int                 decode_only )
{
    /* default = 0 (disabled) */

    /* enforce disable/enable meaning, if we need more than two values
       we need to switch to an enum to have an apropriate representation
       of the possible meanings of the value */
    if ( 0 > decode_only || 1 < decode_only )
        return -1;

    gfp->decode_only = decode_only;

    return 0;
}

int
lame_get_decode_only( const lame_global_flags*  gfp )
{
    assert( 0 <= gfp->decode_only && 1 >= gfp->decode_only );

    return gfp->decode_only;
}


/* encode a Vorbis .ogg file */
int
lame_set_ogg( lame_global_flags*  gfp,
              int                 ogg )
{
    /* default = 0 (disabled) */

    /* enforce disable/enable meaning, if we need more than two values
       we need to switch to an enum to have an apropriate representation
       of the possible meanings of the value */
    if ( 0 > ogg || 1 < ogg )
        return -1;

    gfp->ogg = ogg;

    return 0;
}

int
lame_get_ogg( const lame_global_flags*  gfp )
{
    assert( 0 <= gfp->ogg && 1 >= gfp->ogg );

    return gfp->ogg;
}


/*
 * Internal algorithm selection.
 * True quality is determined by the bitrate but this variable will effect
 * quality by selecting expensive or cheap algorithms.
 * quality=0..9.  0=best (very slow).  9=worst.  
 * recommended:  2     near-best quality, not too slow
 *               5     good quality, fast
 *               7     ok quality, really fast
 */
int
lame_set_quality( lame_global_flags*  gfp,
                  int                 quality )
{
    gfp->quality = quality;

    return 0;
}

int
lame_get_quality( const lame_global_flags*  gfp )
{
    return gfp->quality;
}


/* mode = STEREO, JOINT_STEREO, DUAL_CHANNEL (not supported), MONO */
int
lame_set_mode( lame_global_flags*  gfp,
               MPEG_mode           mode )
{
    /* default: lame chooses based on compression ratio and input channels */

    if( 0 > mode || MAX_INDICATOR <= mode )
        return -1;  /* Unknown MPEG mode! */

    gfp->mode = mode;

    return 0;
}

MPEG_mode
lame_get_mode( const lame_global_flags*  gfp )
{
    assert( 0 <= gfp->mode && MAX_INDICATOR > gfp->mode );

    return gfp->mode;
}


/* Us a M/S mode with a switching threshold based on compression ratio */
int
lame_set_mode_automs( lame_global_flags*  gfp,
                      int                 mode_automs )
{
    /* default = 0 (disabled) */

    /* enforce disable/enable meaning, if we need more than two values
       we need to switch to an enum to have an apropriate representation
       of the possible meanings of the value */
    if ( 0 > mode_automs || 1 < mode_automs )
        return -1;

    gfp->mode_automs = mode_automs;

    return 0;
}

int
lame_get_mode_automs( const lame_global_flags*  gfp )
{
    assert( 0 <= gfp->mode_automs && 1 >= gfp->mode_automs );

    return gfp->mode_automs;
}


// force_ms.  Force M/S for all frames.  For testing only.
// default = 0 (disabled)
int
lame_set_force_ms( lame_global_flags*  gfp,
                   int                 force_ms );
int
lame_get_force_ms( const lame_global_flags*  gfp );


// use free_format?  default = 0 (disabled)
int
lame_set_free_format( lame_global_flags*  gfp,
                      int                 free_format );
int
lame_get_free_format( const lame_global_flags*  gfp );


/* message handlers */
int
lame_set_errorf( lame_global_flags*  gfp,
                 void                (*func)( const char*, va_list ) )
{
    gfp->report.errorf = func;

    return 0;
}

int
lame_set_debugf( lame_global_flags*  gfp,
                 void                (*func)( const char*, va_list ) )
{
    gfp->report.debugf = func;

    return 0;
}

int
lame_set_msgf( lame_global_flags*  gfp,
               void                (*func)( const char *, va_list ) )
{
    gfp->report.msgf = func;

    return 0;
}


/* set one of brate compression ratio.  default is compression ratio of 11.  */
int
lame_set_brate( lame_global_flags*  gfp,
                int                 brate );
int
lame_get_brate( const lame_global_flags*  gfp );
int
lame_set_compression_ratio( lame_global_flags*  gfp,
                            float               compression_ratio );
float
lame_get_compression_ratio( const lame_global_flags*  gfp );




/*
 * frame parameters
 */

// mark as copyright.  default=0
int
lame_set_copyright( lame_global_flags*  gfp,
                    int                 copyright );
int
lame_get_copyright( const lame_global_flags*  gfp );


// mark as original.  default=1
int
lame_set_original( lame_global_flags*  gfp,
                   int                 original );
int
lame_get_original( const lame_global_flags*  gfp );


// error_protection.  Use 2 bytes from each fraome for CRC checksum. default=0
int
lame_set_error_protection( lame_global_flags*  gfp,
                           int                 error_protection );
int
lame_get_error_protection( const lame_global_flags*  gfp );


// padding_type.  0=pad no frames  1=pad all frames 2=adjust padding(default)
int
lame_set_padding_type( lame_global_flags*  gfp,
                       int                 padding_type );
int
lame_get_padding_type( const lame_global_flags*  gfp );


// MP3 'private extension' bit  Meaningless
int
lame_set_extension( lame_global_flags*  gfp,
                    int                 extension );
int
lame_get_extension( const lame_global_flags*  gfp );


// enforce strict ISO compliance.  default=0
int
lame_set_strict_ISO( lame_global_flags*  gfp,
                     int                 strict_ISO );
int
lame_get_strict_ISO( const lame_global_flags*  gfp );
 



/********************************************************************
 * quantization/noise shaping 
 ***********************************************************************/

// disable the bit reservoir. For testing only. default=0
int
lame_set_disable_reservoir( lame_global_flags*  gfp,
                            int                 disable_reservoir );
int
lame_get_disable_reservoir( const lame_global_flags*  gfp );


// select a different "best quantization" function. default=0 
int
lame_set_experimentalX( lame_global_flags*  gfp,
                        int                 experimentalX );
int
lame_get_experimentalX( const lame_global_flags*  gfp );


// another experimental option.  for testing only
int
lame_set_experimentalY( lame_global_flags*  gfp,
                        int                 experimentalY );
int
lame_get_experimentalY( const lame_global_flags*  gfp );


// another experimental option.  for testing only
int
lame_set_experimentalZ( lame_global_flags*  gfp,
                        int                 experimentalZ );
int
lame_get_experimentalZ( const lame_global_flags*  gfp );


// Naoki's psycho acoustic model.  default=0
int
lame_set_exp_nspsytune( lame_global_flags*  gfp,
                        int                 exp_nspsytune );
int
lame_get_exp_nspsytune( const lame_global_flags*  gfp );




/********************************************************************
 * VBR control
 ***********************************************************************/

// Types of VBR.  default = vbr_off = CBR
int
lame_set_VBR( lame_global_flags*  gfp,
              vbr_mode            VBR );
vbr_mode
lame_get_exp_VBR( const lame_global_flags*  gfp );


// VBR quality level.  0=highest  9=lowest 
int
lame_set_VBR_q( lame_global_flags*  gfp,
                int                 VBR_q );
int
lame_get_VBR_q( const lame_global_flags*  gfp );


// Ignored except for VBR=vbr_abr (ABR mode)
int
lame_set_VBR_mean_bitrate_kbps( lame_global_flags*  gfp,
                                int                 VBR_mean_bitrate_kbps );
int
lame_get_VBR_mean_bitrate_kbps( const lame_global_flags*  gfp );

int
lame_set_VBR_min_bitrate_kbps( lame_global_flags*  gfp,
                               int                 VBR_min_bitrate_kbps );
int
lame_get_VBR_min_bitrate_kbps( const lame_global_flags*  gfp );

int
lame_set_VBR_max_bitrate_kbps( lame_global_flags*  gfp,
                               int                 VBR_max_bitrate_kbps );
int
lame_get_VBR_max_bitrate_kbps( const lame_global_flags*  gfp );


// 1=stricetly enforce VBR_min_bitrate.  Normally it will be violated for
// analog silence
int
lame_set_VBR_hard_min( lame_global_flags*  gfp,
                       int                 VBR_hard_min );
int
lame_get_VBR_hard_min( const lame_global_flags*  gfp );


/********************************************************************
 * Filtering control
 ***********************************************************************/

// freq in Hz to apply lowpass. Default = 0 = lame chooses.  -1 = disabled
int
lame_set_lowpassfreq( lame_global_flags*  gfp,
                      int                 lowpassfreq );
int
lame_get_lowpassfreq( const lame_global_flags*  gfp );


// width of transition band, in Hz.  Default = one polyphase filter band
int
lame_set_lowpasswidth( lame_global_flags*  gfp,
                       int                 lowpasswidth );
int
lame_get_lowpasswidth( const lame_global_flags*  gfp );


// freq in Hz to apply highpass. Default = 0 = lame chooses.  -1 = disabled
int
lame_set_highpassfreq( lame_global_flags*  gfp,
                       int                 highpassfreq );
int
lame_get_highpassfreq( const lame_global_flags*  gfp );


// width of transition band, in Hz.  Default = one polyphase filter band
int
lame_set_highpasswidth( lame_global_flags*  gfp,
                        int                 highpasswidth );
int
lame_get_highpasswidth( const lame_global_flags*  gfp );




/*
 * psycho acoustics and other arguments which you should not change 
 * unless you know what you are doing
 */

// only use ATH for masking
int
lame_set_ATHonly( lame_global_flags*  gfp,
                  int                 ATHonly );
int
lame_get_ATHonly( const lame_global_flags*  gfp );


// only use ATH for short blocks
int
lame_set_ATHshort( lame_global_flags*  gfp,
                   int                 ATHshort );
int
lame_get_ATHshort( const lame_global_flags*  gfp );


// disable ATH
int
lame_set_noATH( lame_global_flags*  gfp,
                int                 noATH );
int
lame_get_noATH( const lame_global_flags*  gfp );


// select ATH formula
int
lame_set_ATHtype( lame_global_flags*  gfp,
                  int                 ATHtype );
int
lame_get_ATHtype( const lame_global_flags*  gfp );


// lower ATH by this many db
int
lame_set_ATHlower( lame_global_flags*  gfp,
                   float               ATHlower );
float
lame_get_ATHlower( const lame_global_flags*  gfp );


// predictability limit (ISO tonality formula)
int
lame_set_cwlimit( lame_global_flags*  gfp,
                  int                 cwlimit );
int
lame_get_cwlimit( const lame_global_flags*  gfp );


// allow blocktypes to differ between channels?
// default: 0 for jstereo, 1 for stereo
int
lame_set_allow_diff_short( lame_global_flags*  gfp,
                           int                 allow_diff_short );
int
lame_get_allow_diff_short( const lame_global_flags*  gfp );


// use temporal masking effect (default = 1)
int
lame_set_useTemporal( lame_global_flags*  gfp,
                      int                 useTemporal );
int
lame_get_useTemporal( const lame_global_flags*  gfp );


// disable short blocks
int
lame_set_no_short_blocks( lame_global_flags*  gfp,
                          int                 no_short_blocks );
int
lame_get_no_short_blocks( const lame_global_flags*  gfp );


/* Input PCM is emphased PCM (for instance from one of the rarely
   emphased CDs), it is STRONGLY not recommended to use this, because
   psycho does not take it into account, and last but not least many decoders
   ignore these bits */
int
lame_set_emphasis( lame_global_flags*  gfp,
                   int                 emphasis );
int
lame_get_emphasis( const lame_global_flags*  gfp );




/***************************************************************/
/* internal variables, cannot be set...                        */
/* provided because they may be of use to calling application  */
/***************************************************************/

// version  0=MPEG-2  1=MPEG-1  (2=MPEG-2.5)    
int
lame_get_version( const lame_global_flags* gfp );


// encoder delay
int
lame_get_encoder_delay( const lame_global_flags*  gfp );


// size of MPEG frame
int
lame_get_framesize( const lame_global_flags*  gfp );


// number of frames encoded so far
int
lame_get_frameNum( const lame_global_flags*  gfp );


// lame's estimate of the total number of frames to be encoded
// only valid if calling program set num_samples 
int
lame_get_totalframes( const lame_global_flags*  gfp );

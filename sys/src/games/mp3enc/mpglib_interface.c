/* $Id: mpglib_interface.c,v 1.18 2001/01/06 01:00:33 markt Exp $ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef HAVE_MPGLIB

#include <limits.h>
#include <stdlib.h>
#include <assert.h>

#include "interface.h"
#include "lame.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


MPSTR           mp;
plotting_data*  mpg123_pinfo = NULL;


int lame_decode_init( void )
{
    InitMP3 ( &mp );
    return 0;
}

/*
 * For lame_decode:  return code
 * -1     error
 *  0     ok, but need more data before outputing any samples
 *  n     number of samples output.  either 576 or 1152 depending on MP3 file.
 */
 
int lame_decode1_headers(
        unsigned char*   buffer,
        int              len,
        short            pcm_l [],
        short            pcm_r [],
        mp3data_struct*  mp3data )
{
    static const int smpls [2] [4] = {
    /* Layer   I    II   III */
        { 0, 384, 1152, 1152 }, /* MPEG-1     */
        { 0, 384, 1152,  576 }  /* MPEG-2(.5) */
    };
    static char        out  [8192];
    signed short int*  p = (signed short int*) out;
    int                processed_bytes;
    int                processed_samples;  // processed samples per channel
    int                ret;
    int                i;

    mp3data->header_parsed = 0;
  
    ret = decodeMP3 ( &mp, buffer, len, (char*)p, sizeof(out), &processed_bytes );

    if ( mp.header_parsed ) {
        mp3data->header_parsed = 1;
        mp3data->stereo        = mp.fr.stereo;
        mp3data->samplerate    = freqs [mp.fr.sampling_frequency];
        mp3data->mode          = mp.fr.mode;
        mp3data->mode_ext      = mp.fr.mode_ext;
        mp3data->framesize     = smpls [mp.fr.lsf] [mp.fr.lay];
	
        if (mp.fsizeold > 0)      /* works for free format and fixed, no overrun, temporal results are < 400.e6 */
            mp3data->bitrate   = 8 * (4 + mp.fsizeold) * mp3data->samplerate /
	                         ( 1.e3 * mp3data->framesize ) + 0.5;
        else
            mp3data->bitrate   = tabsel_123 [mp.fr.lsf] [mp.fr.lay-1] [mp.fr.bitrate_index];

        if (mp.num_frames>0) {
	  /* Xing VBR header found and num_frames was set */
	  mp3data->totalframes = mp.num_frames;
          mp3data->nsamp=mp3data->framesize * mp.num_frames;  
	}
    }

    switch ( ret ) {
    case MP3_OK:
        switch ( mp.fr.stereo ) {
	case 1:
            processed_samples = processed_bytes >> 1;
            for ( i = 0; i < processed_samples; i++ ) 
	        pcm_l [i] = *p++;
	    break;
	case 2:
            processed_samples = processed_bytes >> 2;
            for ( i = 0; i < processed_samples; i++ ) {
	        pcm_l [i] = *p++;
		pcm_r [i] = *p++;
	    }
	    break;
	default:
            processed_samples = -1;
	    assert (0);
	    break;
        }    
	break;
	
    case MP3_NEED_MORE:
        processed_samples = 0;
	break;
	
    default:
        assert (0);
    case MP3_ERR: 
        processed_samples = -1;
	break;
	
    }
  
    //  printf ( "ok, more, err:  %i %i %i\n", MP3_OK, MP3_NEED_MORE, MP3_ERR );
    //  printf ( "ret = %i out=%i\n", ret, totsize );
    return processed_samples;
}


/*
 * For lame_decode:  return code
 *  -1     error
 *   0     ok, but need more data before outputing any samples
 *   n     number of samples output.  Will be at most one frame of
 *         MPEG data.  
 */
 
int lame_decode1( 
        unsigned char*  buffer,
	int    len,
	short  pcm_l [],
	short  pcm_r [] )
{
  mp3data_struct mp3data;
  
  return lame_decode1_headers ( buffer, len, pcm_l, pcm_r, &mp3data );
}


/*
 * For lame_decode:  return code
 *  -1     error
 *   0     ok, but need more data before outputing any samples
 *   n     number of samples output.  a multiple of 576 or 1152 depending on MP3 file.
 */
 
int lame_decode_headers( 
        unsigned char*   buffer,
	int              len,
	short            pcm_l [],
	short            pcm_r [],
        mp3data_struct*  mp3data )
{
    int  ret;
    int  totsize = 0;   // number of decoded samples per channel

    while (1) {
        switch ( ret = lame_decode1_headers ( buffer, len, pcm_l+totsize, pcm_r+totsize, mp3data ) ) {
        case -1: return ret;
        case  0: return totsize;
        default: totsize += ret;
                 len      = 0;  /* future calls to decodeMP3 are just to flush buffers */
                 break;
        }
    }
}


int lame_decode(
        unsigned char*  buffer,
	int    len,
	short  pcm_l [],
	short  pcm_r [] )
{
    mp3data_struct  mp3data;
    
    return lame_decode_headers ( buffer, len, pcm_l, pcm_r, &mp3data );
}


#endif

/* end of mpglib_interface.c */

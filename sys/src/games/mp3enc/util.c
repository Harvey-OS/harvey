/*
 *	lame utility library source file
 *
 *	Copyright (c) 1999 Albert L Faber
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* $Id: util.c,v 1.67 2001/03/20 00:42:56 markt Exp $ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define PRECOMPUTE

#include "util.h"
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>

#if defined(__FreeBSD__) && !defined(__alpha__)
# include <machine/floatingpoint.h>
#endif

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

/***********************************************************************
*
*  Global Function Definitions
*
***********************************************************************/
/*empty and close mallocs in gfc */

void  freegfc ( lame_internal_flags* const gfc )   /* bit stream structure */
{
    int  i;

#ifdef KLEMM_44
    if (gfc->resample_in != NULL) {
        resample_close(gfc->resample_in);
        gfc->resample_in = NULL;
    }
    free(gfc->mfbuf[0]);
    free(gfc->mfbuf[1]);
#endif

    for ( i = 0 ; i <= 2*BPC; i++ )
        if ( gfc->blackfilt[i] != NULL ) {
            free ( gfc->blackfilt[i] );
	    gfc->blackfilt[i] = NULL;
	}
    if ( gfc->inbuf_old[0] ) { 
        free ( gfc->inbuf_old[0] );
	gfc->inbuf_old[0] = NULL;
    }
    if ( gfc->inbuf_old[1] ) { 
        free ( gfc->inbuf_old[1] );
	gfc->inbuf_old[1] = NULL;
    }

    if ( gfc->bs.buf != NULL ) {
        free ( gfc->bs.buf );
        gfc->bs.buf = NULL;
    }

    if ( gfc->VBR_seek_table.bag ) {
        free ( gfc->VBR_seek_table.bag );
    }
    if ( gfc->ATH ) {
        free ( gfc->ATH );
    }
    free ( gfc );
}

FLOAT8 ATHformula_old(FLOAT8 f)
{
  FLOAT8 ath;
  f /= 1000;  // convert to khz
  f  = Max(0.01, f);
  f  = Min(18.0, f);

  /* from Painter & Spanias, 1997 */
  /* minimum: (i=77) 3.3kHz = -5db */
  ath =    3.640 * pow(f,-0.8)
         - 6.500 * exp(-0.6*pow(f-3.3,2.0))
         + 0.001 * pow(f,4.0);
  return ath;
}

FLOAT8 ATHformula_GB(FLOAT8 f)
{
  FLOAT8 ath;
  f /= 1000;  // convert to khz
  f  = Max(0.01, f);
  f  = Min(18.0, f);

  /* from Painter & Spanias, 1997 */
  /* modified by Gabriel Bouvigne to better fit to the reality */
  ath =    3.640 * pow(f,-0.8)
         - 6.800 * exp(-0.6*pow(f-3.4,2.0))
         + 6.000 * exp(-0.15*pow(f-8.7,2.0))
         + 0.6* 0.001 * pow(f,4.0);
  return ath;
}

FLOAT8 ATHformula_GBtweak(FLOAT8 f)
{
  FLOAT8 ath;
  f /= 1000;  // convert to khz
  f  = Max(0.01, f);
  f  = Min(18.0, f);

  /* from Painter & Spanias, 1997 */
  /* modified by Gabriel Bouvigne to better fit to the reality */
  ath =    3.640 * pow(f,-0.8)
         - 6.800 * exp(-0.6*pow(f-3.4,2.0))
         + 6.000 * exp(-0.15*pow(f-8.7,2.0))
         + 0.57* 0.001 * pow(f,4.0) //0.57 to maximize HF importance
         + 6; //std --athlower -6 for
  return ath;
}


/* 
 *  Klemm 1994 and 1997. Experimental data. Sorry, data looks a little bit
 *  dodderly. Data below 30 Hz is extrapolated from other material, above 18
 *  kHz the ATH is limited due to the original purpose (too much noise at
 *  ATH is not good even if it's theoretically inaudible).
 */

FLOAT8  ATHformula_Frank( FLOAT8 freq )
{
    /*
     * one value per 100 cent = 1
     * semitone = 1/4
     * third = 1/12
     * octave = 1/40 decade
     * rest is linear interpolated, values are currently in decibel rel. 20 µPa
     */
    static FLOAT tab [] = {
        /*    10.0 */  96.69, 96.69, 96.26, 95.12,
        /*    12.6 */  93.53, 91.13, 88.82, 86.76,
        /*    15.8 */  84.69, 82.43, 79.97, 77.48,
        /*    20.0 */  74.92, 72.39, 70.00, 67.62,
        /*    25.1 */  65.29, 63.02, 60.84, 59.00,
        /*    31.6 */  57.17, 55.34, 53.51, 51.67,
        /*    39.8 */  50.04, 48.12, 46.38, 44.66,
        /*    50.1 */  43.10, 41.73, 40.50, 39.22,
        /*    63.1 */  37.23, 35.77, 34.51, 32.81,
        /*    79.4 */  31.32, 30.36, 29.02, 27.60,
        /*   100.0 */  26.58, 25.91, 24.41, 23.01,
        /*   125.9 */  22.12, 21.25, 20.18, 19.00,
        /*   158.5 */  17.70, 16.82, 15.94, 15.12,
        /*   199.5 */  14.30, 13.41, 12.60, 11.98,
        /*   251.2 */  11.36, 10.57,  9.98,  9.43,
        /*   316.2 */   8.87,  8.46,  7.44,  7.12,
        /*   398.1 */   6.93,  6.68,  6.37,  6.06,
        /*   501.2 */   5.80,  5.55,  5.29,  5.02,
        /*   631.0 */   4.75,  4.48,  4.22,  3.98,
        /*   794.3 */   3.75,  3.51,  3.27,  3.22,
        /*  1000.0 */   3.12,  3.01,  2.91,  2.68,
        /*  1258.9 */   2.46,  2.15,  1.82,  1.46,
        /*  1584.9 */   1.07,  0.61,  0.13, -0.35,
        /*  1995.3 */  -0.96, -1.56, -1.79, -2.35,
        /*  2511.9 */  -2.95, -3.50, -4.01, -4.21,
        /*  3162.3 */  -4.46, -4.99, -5.32, -5.35,
        /*  3981.1 */  -5.13, -4.76, -4.31, -3.13,
        /*  5011.9 */  -1.79,  0.08,  2.03,  4.03,
        /*  6309.6 */   5.80,  7.36,  8.81, 10.22,
        /*  7943.3 */  11.54, 12.51, 13.48, 14.21,
        /* 10000.0 */  14.79, 13.99, 12.85, 11.93,
        /* 12589.3 */  12.87, 15.19, 19.14, 23.69,
        /* 15848.9 */  33.52, 48.65, 59.42, 61.77,
        /* 19952.6 */  63.85, 66.04, 68.33, 70.09,
        /* 25118.9 */  70.66, 71.27, 71.91, 72.60,
    };
    FLOAT8    freq_log;
    unsigned  index;
    
    if ( freq <    10. ) freq =    10.;
    if ( freq > 29853. ) freq = 29853.;
    
    freq_log = 40. * log10 (0.1 * freq);   /* 4 steps per third, starting at 10 Hz */
    index    = (unsigned) freq_log;
    assert ( index < sizeof(tab)/sizeof(*tab) );
    return tab [index] * (1 + index - freq_log) + tab [index+1] * (freq_log - index);
}

FLOAT8 ATHformula(FLOAT8 f,lame_global_flags *gfp)
{
  switch(gfp->ATHtype)
    {
    case 0:
      return ATHformula_old(f);
    case 1:
      return ATHformula_Frank(f);
    case 2:
      return ATHformula_GB(f);
    case 3:
      return ATHformula_GBtweak(f);
    }

  return ATHformula_Frank(f);
}

/* see for example "Zwicker: Psychoakustik, 1982; ISBN 3-540-11401-7 */
FLOAT8 freq2bark(FLOAT8 freq)
{
  /* input: freq in hz  output: barks */
    if (freq<0) freq=0;
    freq = freq * 0.001;
    return 13.0*atan(.76*freq) + 3.5*atan(freq*freq/(7.5*7.5));
}

/* see for example "Zwicker: Psychoakustik, 1982; ISBN 3-540-11401-7 */
FLOAT8 freq2cbw(FLOAT8 freq)
{
  /* input: freq in hz  output: critical band width */
    freq = freq * 0.001;
    return 25+75*pow(1+1.4*(freq*freq),0.69);
}






/***********************************************************************
 * compute bitsperframe and mean_bits for a layer III frame 
 **********************************************************************/
void getframebits(lame_global_flags *gfp, int *bitsPerFrame, int *mean_bits) 
{
  lame_internal_flags *gfc=gfp->internal_flags;
  int  whole_SpF;  /* integral number of Slots per Frame without padding */
  int  bit_rate;
  
  /* get bitrate in kbps [?] */
  if (gfc->bitrate_index) 
    bit_rate = bitrate_table[gfp->version][gfc->bitrate_index];
  else
    bit_rate = gfp->brate;
  assert ( bit_rate <= 550 );
  
  // bytes_per_frame = bitrate * 1000 / ( gfp->out_samplerate / (gfp->version == 1  ?  1152  :  576 )) / 8;
  // bytes_per_frame = bitrate * 1000 / gfp->out_samplerate * (gfp->version == 1  ?  1152  :  576 ) / 8;
  // bytes_per_frame = bitrate * ( gfp->version == 1  ?  1152/8*1000  :  576/8*1000 ) / gfp->out_samplerate;
  
  whole_SpF = (gfp->version+1)*72000*bit_rate / gfp->out_samplerate;
  
  // There must be somewhere code toggling gfc->padding on and off
  
  /* main encoding routine toggles padding on and off */
  /* one Layer3 Slot consists of 8 bits */
  *bitsPerFrame = 8 * (whole_SpF + gfc->padding);
  
  // sideinfo_len
  *mean_bits = (*bitsPerFrame - 8*gfc->sideinfo_len) / gfc->mode_gr;
}




#define ABS(A) (((A)>0) ? (A) : -(A))

int FindNearestBitrate(
int bRate,        /* legal rates from 32 to 448 */
int version,      /* MPEG-1 or MPEG-2 LSF */
int samplerate)   /* convert bitrate in kbps to index */
{
    int  bitrate = 0;
    int  i;
  
    for ( i = 1; i <= 14; i++ )
        if ( ABS (bitrate_table[version][i] - bRate) < ABS (bitrate - bRate) )
            bitrate = bitrate_table [version] [i];
	    
    return bitrate;
}


/* map frequency to a valid MP3 sample frequency
 *
 * Robert.Hegemann@gmx.de 2000-07-01
 */
int map2MP3Frequency(int freq)
{
    if (freq <=  8000) return  8000;
    if (freq <= 11025) return 11025;
    if (freq <= 12000) return 12000;
    if (freq <= 16000) return 16000;
    if (freq <= 22050) return 22050;
    if (freq <= 24000) return 24000;
    if (freq <= 32000) return 32000;
    if (freq <= 44100) return 44100;
    
    return 48000;
}

int BitrateIndex(
int bRate,        /* legal rates from 32 to 448 kbps */
int version,      /* MPEG-1 or MPEG-2/2.5 LSF */
int samplerate)   /* convert bitrate in kbps to index */
{
    int  i;

    for ( i = 0; i <= 14; i++)
        if ( bitrate_table [version] [i] == bRate )
            return i;
	    
    return -1;
}

/* convert samp freq in Hz to index */

int SmpFrqIndex ( int sample_freq, int* const version )
{
    switch ( sample_freq ) {
    case 44100: *version = 1; return  0;
    case 48000: *version = 1; return  1;
    case 32000: *version = 1; return  2;
    case 22050: *version = 0; return  0;
    case 24000: *version = 0; return  1;
    case 16000: *version = 0; return  2;
    case 11025: *version = 0; return  0;
    case 12000: *version = 0; return  1;
    case  8000: *version = 0; return  2;
    default:    *version = 0; return -1;
    }
}


/*****************************************************************************
*
*  End of bit_stream.c package
*
*****************************************************************************/

/* reorder the three short blocks By Takehiro TOMINAGA */
/*
  Within each scalefactor band, data is given for successive
  time windows, beginning with window 0 and ending with window 2.
  Within each window, the quantized values are then arranged in
  order of increasing frequency...
*/
void freorder(int scalefac_band[],FLOAT8 ix_orig[576]) {
  int i,sfb, window, j=0;
  FLOAT8 ix[576];
  for (sfb = 0; sfb < SBMAX_s; sfb++) {
    int start = scalefac_band[sfb];
    int end   = scalefac_band[sfb + 1];
    for (window = 0; window < 3; window++) {
      for (i = start; i < end; ++i) {
	ix[j++] = ix_orig[3*i+window];
      }
    }
  }
  memcpy(ix_orig,ix,576*sizeof(FLOAT8));
}







#ifndef KLEMM_44


/* resampling via FIR filter, blackman window */
inline static FLOAT8 blackman(FLOAT8 x,FLOAT8 fcn,int l)
{
  /* This algorithm from:
SIGNAL PROCESSING ALGORITHMS IN FORTRAN AND C
S.D. Stearns and R.A. David, Prentice-Hall, 1992
  */
  FLOAT8 bkwn,x2;
  FLOAT8 wcn = (PI * fcn);
  
  x /= l;
  if (x<0) x=0;
  if (x>1) x=1;
  x2 = x - .5;

  bkwn = 0.42 - 0.5*cos(2*x*PI)  + 0.08*cos(4*x*PI);
  if (fabs(x2)<1e-9) return wcn/PI;
  else 
    return  (  bkwn*sin(l*wcn*x2)  / (PI*l*x2)  );


}

/* gcd - greatest common divisor */
/* Joint work of Euclid and M. Hendry */

int gcd ( int i, int j )
{
//    assert ( i > 0  &&  j > 0 );
    return j ? gcd(j, i % j) : i;
}



/* copy in new samples from in_buffer into mfbuf, with resampling & scaling 
   if necessary.  n_in = number of samples from the input buffer that
   were used.  n_out = number of samples copied into mfbuf  */

void fill_buffer(lame_global_flags *gfp,
		 sample_t *mfbuf[2],
		 sample_t *in_buffer[2],
		 int nsamples, int *n_in, int *n_out)
{
    lame_internal_flags *gfc = gfp->internal_flags;
    int ch,i;

    /* copy in new samples into mfbuf, with resampling if necessary */
    if (gfc->resample_ratio != 1.0) {
	for (ch = 0; ch < gfc->channels_out; ch++) {
	    *n_out =
		fill_buffer_resample(gfp, &mfbuf[ch][gfc->mf_size],
				     gfp->framesize, in_buffer[ch],
				     nsamples, n_in, ch);
	}
    }
    else {
	*n_out = Min(gfp->framesize, nsamples);
	*n_in = *n_out;
	for (i = 0; i < *n_out; ++i) {
	    mfbuf[0][gfc->mf_size + i] = in_buffer[0][i];
	    if (gfc->channels_out == 2)
		mfbuf[1][gfc->mf_size + i] = in_buffer[1][i];
	}
    }

    /* user selected scaling of the samples */
    if (gfp->scale != 0) {
	for (i=0 ; i<*n_out; ++i) {
	    mfbuf[0][gfc->mf_size+i] *= gfp->scale;
	    if (gfc->channels_out == 2)
		mfbuf[1][gfc->mf_size + i] *= gfp->scale;
	}
    }

}
    



int fill_buffer_resample(
       lame_global_flags *gfp,
       sample_t *outbuf,
       int desired_len,
       sample_t *inbuf,
       int len,
       int *num_used,
       int ch) 
{

  
  lame_internal_flags *gfc=gfp->internal_flags;
  int BLACKSIZE;
  FLOAT8 offset,xvalue;
  int i,j=0,k;
  int filter_l;
  FLOAT8 fcn,intratio;
  FLOAT *inbuf_old;
  int bpc;   /* number of convolution functions to pre-compute */
  bpc = gfp->out_samplerate/gcd(gfp->out_samplerate,gfp->in_samplerate);
  if (bpc>BPC) bpc = BPC;

  intratio=( fabs(gfc->resample_ratio - floor(.5+gfc->resample_ratio)) < .0001 );
  fcn = 1.00/gfc->resample_ratio;
  if (fcn>1.00) fcn=1.00;
  filter_l = gfp->quality < 7 ? 31 : 7;
  filter_l = 31;
  if (0==filter_l % 2 ) --filter_l;/* must be odd */
  filter_l += intratio;            /* unless resample_ratio=int, it must be even */


  BLACKSIZE = filter_l+1;  /* size of data needed for FIR */
  
  if ( gfc->fill_buffer_resample_init == 0 ) {
    gfc->inbuf_old[0]=calloc(BLACKSIZE,sizeof(gfc->inbuf_old[0][0]));
    gfc->inbuf_old[1]=calloc(BLACKSIZE,sizeof(gfc->inbuf_old[0][0]));
    for (i=0; i<=2*bpc; ++i)
      gfc->blackfilt[i]=calloc(BLACKSIZE,sizeof(gfc->blackfilt[0][0]));

    gfc->itime[0]=0;
    gfc->itime[1]=0;

    /* precompute blackman filter coefficients */
    for ( j = 0; j <= 2*bpc; j++ ) {
        FLOAT8 sum = 0.; 
        offset = (j-bpc) / (2.*bpc);
        for ( i = 0; i <= filter_l; i++ ) 
            sum += 
	    gfc->blackfilt[j][i]  = blackman(i-offset,fcn,filter_l);
	for ( i = 0; i <= filter_l; i++ ) 
	  gfc->blackfilt[j][i] /= sum;
    }
    gfc->fill_buffer_resample_init = 1;
  }
  
  inbuf_old=gfc->inbuf_old[ch];

  /* time of j'th element in inbuf = itime + j/ifreq; */
  /* time of k'th element in outbuf   =  j/ofreq */
  for (k=0;k<desired_len;k++) {
    FLOAT time0;
    int joff;

    time0 = k*gfc->resample_ratio;       /* time of k'th output sample */
    j = floor( time0 -gfc->itime[ch]  );

    /* check if we need more input data */
    if ((filter_l + j - filter_l/2) >= len) break;

    /* blackman filter.  by default, window centered at j+.5(filter_l%2) */
    /* but we want a window centered at time0.   */
    offset = ( time0 -gfc->itime[ch] - (j + .5*(filter_l%2)));
    assert(fabs(offset)<=.500001);

    /* find the closest precomputed window for this offset: */
    joff = floor((offset*2*bpc) + bpc +.5);

    xvalue = 0.;
    for (i=0 ; i<=filter_l ; ++i) {
      int j2 = i+j-filter_l/2;
      int y;
      assert(j2<len);
      assert(j2+BLACKSIZE >= 0);
      y = (j2<0) ? inbuf_old[BLACKSIZE+j2] : inbuf[j2];
#define PRECOMPUTE
#ifdef PRECOMPUTE
      xvalue += y*gfc->blackfilt[joff][i];
#else
      xvalue += y*blackman(i-offset,fcn,filter_l);  /* very slow! */
#endif
    }
    outbuf[k]=xvalue;
  }

  
  /* k = number of samples added to outbuf */
  /* last k sample used data from [j-filter_l/2,j+filter_l-filter_l/2]  */

  /* how many samples of input data were used:  */
  *num_used = Min(len,filter_l+j-filter_l/2);

  /* adjust our input time counter.  Incriment by the number of samples used,
   * then normalize so that next output sample is at time 0, next
   * input buffer is at time itime[ch] */
  gfc->itime[ch] += *num_used - k*gfc->resample_ratio;

  /* save the last BLACKSIZE samples into the inbuf_old buffer */
  if (*num_used >= BLACKSIZE) {
      for (i=0;i<BLACKSIZE;i++)
	  inbuf_old[i]=inbuf[*num_used + i -BLACKSIZE];
  }else{
      /* shift in *num_used samples into inbuf_old  */
       int n_shift = BLACKSIZE-*num_used;  /* number of samples to shift */

       /* shift n_shift samples by *num_used, to make room for the
	* num_used new samples */
       for (i=0; i<n_shift; ++i ) 
	   inbuf_old[i] = inbuf_old[i+ *num_used];

       /* shift in the *num_used samples */
       for (j=0; i<BLACKSIZE; ++i, ++j ) 
	   inbuf_old[i] = inbuf[j];

       assert(j==*num_used);
  }
  return k;  /* return the number samples created at the new samplerate */
}


#endif /* ndef KLEMM_44 */



/***********************************************************************
*
*  Message Output
*
***********************************************************************/
void  lame_debugf (const lame_internal_flags *gfc, const char* format, ... )
{
    va_list  args;

    va_start ( args, format );

    if ( gfc->report.debugf != NULL ) {
        gfc->report.debugf( format, args );
    } else {
        (void) vfprintf ( stderr, format, args );
        fflush ( stderr );      /* an debug function should flush immediately */
    }

    va_end   ( args );
}


void  lame_msgf (const lame_internal_flags *gfc, const char* format, ... )
{
    va_list  args;

    va_start ( args, format );
   
    if ( gfc->report.msgf != NULL ) {
        gfc->report.msgf( format, args );
    } else {
        (void) vfprintf ( stderr, format, args );
        fflush ( stderr );     /* we print to stderr, so me may want to flush */
    }

    va_end   ( args );
}


void  lame_errorf (const lame_internal_flags *gfc, const char* format, ... )
{
    va_list  args;

    va_start ( args, format );
    
    if ( gfc->report.errorf != NULL ) {
        gfc->report.errorf( format, args );
    } else {
        (void) vfprintf ( stderr, format, args );
        fflush   ( stderr );    /* an error function should flush immediately */
    }

    va_end   ( args );
}



/***********************************************************************
 *
 *      routines to detect CPU specific features like 3DNow, MMX, SIMD
 *
 *  donated by Frank Klemm
 *  added Robert Hegemann 2000-10-10
 *
 ***********************************************************************/

int  has_i387 ( void )
{
#ifdef HAVE_NASM 
    return 1;
#else
    return 0;   /* don't know, assume not */
#endif
}    

int  has_MMX ( void )
{
#ifdef HAVE_NASM 
    extern int has_MMX_nasm ( void );
    return has_MMX_nasm ();
#else
    return 0;   /* don't know, assume not */
#endif
}    

int  has_3DNow ( void )
{
#ifdef HAVE_NASM 
    extern int has_3DNow_nasm ( void );
    return has_3DNow_nasm ();
#else
    return 0;   /* don't know, assume not */
#endif
}    

int  has_SIMD ( void )
{
#ifdef HAVE_NASM 
    extern int has_SIMD_nasm ( void );
    return has_SIMD_nasm ();
#else
    return 0;   /* don't know, assume not */
#endif
}    

int  has_SIMD2 ( void )
{
#ifdef HAVE_NASM 
    extern int has_SIMD2_nasm ( void );
    return has_SIMD2_nasm ();
#else
    return 0;   /* don't know, assume not */
#endif
}    

/***********************************************************************
 *
 *  some simple statistics
 *
 *  bitrate index 0: free bitrate -> not allowed in VBR mode
 *  : bitrates, kbps depending on MPEG version
 *  bitrate index 15: forbidden
 *
 *  mode_ext:
 *  0:  LR
 *  1:  LR-i
 *  2:  MS
 *  3:  MS-i
 *
 ***********************************************************************/
 
void updateStats( lame_internal_flags * const gfc )
{
    assert ( gfc->bitrate_index < 16u );
    assert ( gfc->mode_ext      <  4u );
    
    /* count bitrate indices */
    gfc->bitrate_stereoMode_Hist [gfc->bitrate_index] [4] ++;
    
    /* count 'em for every mode extension in case of 2 channel encoding */
    if (gfc->channels_out == 2)
        gfc->bitrate_stereoMode_Hist [gfc->bitrate_index] [gfc->mode_ext]++;
}



/*  caution: a[] will be resorted!!
 */
int select_kth_int(int a[], int N, int k)
{
    int i, j, l, r, v, w;
    
    l = 0;
    r = N-1;
    while (r > l) {
        v = a[r];
        i = l-1;
        j = r;
        for (;;) {
            while (a[++i] < v) /*empty*/;
            while (a[--j] > v) /*empty*/;
            if (i >= j) 
                break;
            /* swap i and j */
            w = a[i];
            a[i] = a[j];
            a[j] = w;
        }
        /* swap i and r */
        w = a[i];
        a[i] = a[r];
        a[r] = w;
        if (i >= k) 
            r = i-1;
        if (i <= k) 
            l = i+1;
    }
    return a[k];
}



void disable_FPE(void) {
/* extremly system dependent stuff, move to a lib to make the code readable */
/*==========================================================================*/

    /*
     *  Disable floating point exceptions
     */
#if defined(__FreeBSD__) && !defined(__alpha__)
    {
        /* seet floating point mask to the Linux default */
        fp_except_t mask;
        mask = fpgetmask();
        /* if bit is set, we get SIGFPE on that error! */
        fpsetmask(mask & ~(FP_X_INV | FP_X_DZ));
        /*  DEBUGF("FreeBSD mask is 0x%x\n",mask); */
    }
#endif

#if defined(__riscos__) && !defined(ABORTFP)
    /* Disable FPE's under RISC OS */
    /* if bit is set, we disable trapping that error! */
    /*   _FPE_IVO : invalid operation */
    /*   _FPE_DVZ : divide by zero */
    /*   _FPE_OFL : overflow */
    /*   _FPE_UFL : underflow */
    /*   _FPE_INX : inexact */
    DisableFPETraps(_FPE_IVO | _FPE_DVZ | _FPE_OFL);
#endif

    /*
     *  Debugging stuff
     *  The default is to ignore FPE's, unless compiled with -DABORTFP
     *  so add code below to ENABLE FPE's.
     */

#if defined(ABORTFP)
#if defined(_MSC_VER)
    {
#include <float.h>
        unsigned int mask;
        mask = _controlfp(0, 0);
        mask &= ~(_EM_OVERFLOW | _EM_UNDERFLOW | _EM_ZERODIVIDE | _EM_INVALID);
        mask = _controlfp(mask, _MCW_EM);
    }
#elif defined(__CYGWIN__)
#  define _FPU_GETCW(cw) __asm__ ("fnstcw %0" : "=m" (*&cw))
#  define _FPU_SETCW(cw) __asm__ ("fldcw %0" : : "m" (*&cw))

#  define _EM_INEXACT     0x00000020 /* inexact (precision) */
#  define _EM_UNDERFLOW   0x00000010 /* underflow */
#  define _EM_OVERFLOW    0x00000008 /* overflow */
#  define _EM_ZERODIVIDE  0x00000004 /* zero divide */
#  define _EM_INVALID     0x00000001 /* invalid */
    {
        unsigned int mask;
        _FPU_GETCW(mask);
        /* Set the FPU control word to abort on most FPEs */
        mask &= ~(_EM_OVERFLOW | _EM_ZERODIVIDE | _EM_INVALID);
        _FPU_SETCW(mask);
    }
# elif defined(__linux__)
    {

#  include <fpu_control.h>
#  ifndef _FPU_GETCW
#  define _FPU_GETCW(cw) __asm__ ("fnstcw %0" : "=m" (*&cw))
#  endif
#  ifndef _FPU_SETCW
#  define _FPU_SETCW(cw) __asm__ ("fldcw %0" : : "m" (*&cw))
#  endif

        /* 
         * Set the Linux mask to abort on most FPE's
         * if bit is set, we _mask_ SIGFPE on that error!
         *  mask &= ~( _FPU_MASK_IM | _FPU_MASK_ZM | _FPU_MASK_OM | _FPU_MASK_UM );
         */

        unsigned int mask;
        _FPU_GETCW(mask);
        mask &= ~(_FPU_MASK_IM | _FPU_MASK_ZM | _FPU_MASK_OM);
        _FPU_SETCW(mask);
    }
#endif
#endif /* ABORTFP */
}


/* end of util.c */

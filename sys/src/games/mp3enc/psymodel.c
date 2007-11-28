/*
 *	psymodel.c
 *
 *	Copyright (c) 1999 Mark Taylor
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

/* $Id: psymodel.c,v 1.75 2001/03/25 21:37:00 shibatch Exp $ */


/*
PSYCHO ACOUSTICS


This routine computes the psycho acoustics, delayed by
one granule.  

Input: buffer of PCM data (1024 samples).  

This window should be centered over the 576 sample granule window.
The routine will compute the psycho acoustics for
this granule, but return the psycho acoustics computed
for the *previous* granule.  This is because the block
type of the previous granule can only be determined
after we have computed the psycho acoustics for the following
granule.  

Output:  maskings and energies for each scalefactor band.
block type, PE, and some correlation measures.  
The PE is used by CBR modes to determine if extra bits
from the bit reservoir should be used.  The correlation
measures are used to determine mid/side or regular stereo.


Notation:

barks:  a non-linear frequency scale.  Mapping from frequency to
        barks is given by freq2bark()

scalefactor bands: The spectrum (frequencies) are broken into 
                   SBMAX "scalefactor bands".  Thes bands
                   are determined by the MPEG ISO spec.  In
                   the noise shaping/quantization code, we allocate
                   bits among the partition bands to achieve the
                   best possible quality

partition bands:   The spectrum is also broken into about
                   64 "partition bands".  Each partition 
                   band is about .34 barks wide.  There are about 2-5
                   partition bands for each scalefactor band.

LAME computes all psycho acoustic information for each partition
band.  Then at the end of the computations, this information
is mapped to scalefactor bands.  The energy in each scalefactor
band is taken as the sum of the energy in all partition bands
which overlap the scalefactor band.  The maskings can be computed
in the same way (and thus represent the average masking in that band)
or by taking the minmum value multiplied by the number of
partition bands used (which represents a minimum masking in that band).


The general outline is as follows:


1. compute the energy in each partition band
2. compute the tonality in each partition band
3. compute the strength of each partion band "masker"
4. compute the masking (via the spreading function applied to each masker)
5. Modifications for mid/side masking.  

Each partition band is considiered a "masker".  The strength
of the i'th masker in band j is given by:

    s3(bark(i)-bark(j))*strength(i)

The strength of the masker is a function of the energy and tonality.
The more tonal, the less masking.  LAME uses a simple linear formula
(controlled by NMT and TMN) which says the strength is given by the
energy divided by a linear function of the tonality.


s3() is the "spreading function".  It is given by a formula
determined via listening tests.  

The total masking in the j'th partition band is the sum over
all maskings i.  It is thus given by the convolution of
the strength with s3(), the "spreading function."

masking(j) = sum_over_i  s3(i-j)*strength(i)  = s3 o strength

where "o" = convolution operator.  s3 is given by a formula determined
via listening tests.  It is normalized so that s3 o 1 = 1.

Note: instead of a simple convolution, LAME also has the
option of using "additive masking"

The most critical part is step 2, computing the tonality of each
partition band.  LAME has two tonality estimators.  The first
is based on the ISO spec, and measures how predictiable the
signal is over time.  The more predictable, the more tonal.
The second measure is based on looking at the spectrum of
a single granule.  The more peaky the spectrum, the more
tonal.  By most indications, the latter approach is better.

Finally, in step 5, the maskings for the mid and side
channel are possibly increased.  Under certain circumstances,
noise in the mid & side channels is assumed to also
be masked by strong maskers in the L or R channels.


Other data computed by the psy-model:

ms_ratio        side-channel / mid-channel masking ratio (for previous granule)
ms_ratio_next   side-channel / mid-channel masking ratio for this granule

percep_entropy[2]     L and R values (prev granule) of PE - A measure of how 
                      much pre-echo is in the previous granule
percep_entropy_MS[2]  mid and side channel values (prev granule) of percep_entropy
energy[4]             L,R,M,S energy in each channel, prev granule
blocktype_d[2]        block type to use for previous granule


*/




#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "util.h"
#include "encoder.h"
#include "psymodel.h"
#include "l3side.h"
#include <assert.h>
#include "tables.h"
#include "fft.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#define NSFIRLEN 21
#define rpelev 2
#define rpelev2 16

/* size of each partition band, in barks: */
#define DELBARK .34


#if 1
    /* AAC values, results in more masking over MP3 values */
# define TMN 18
# define NMT 6
#else
    /* MP3 values */
# define TMN 29
# define NMT 6
#endif

#define NBPSY_l  (SBMAX_l)
#define NBPSY_s  (SBMAX_s)


#ifdef M_LN10
#define		LN_TO_LOG10		(M_LN10/10)
#else
#define         LN_TO_LOG10             0.2302585093
#endif





/*
   L3psycho_anal.  Compute psycho acoustics.

   Data returned to the calling program must be delayed by one 
   granule. 

   This is done in two places.  
   If we do not need to know the blocktype, the copying
   can be done here at the top of the program: we copy the data for
   the last granule (computed during the last call) before it is
   overwritten with the new data.  It looks like this:
  
   0. static psymodel_data 
   1. calling_program_data = psymodel_data
   2. compute psymodel_data
    
   For data which needs to know the blocktype, the copying must be
   done at the end of this loop, and the old values must be saved:
   
   0. static psymodel_data_old 
   1. compute psymodel_data
   2. compute possible block type of this granule
   3. compute final block type of previous granule based on #2.
   4. calling_program_data = psymodel_data_old
   5. psymodel_data_old = psymodel_data
*/
int L3psycho_anal( lame_global_flags * gfp,
                    const sample_t *buffer[2], int gr_out, 
                    FLOAT8 *ms_ratio,
                    FLOAT8 *ms_ratio_next,
		    III_psy_ratio masking_ratio[2][2],
		    III_psy_ratio masking_MS_ratio[2][2],
		    FLOAT8 percep_entropy[2],FLOAT8 percep_MS_entropy[2], 
                    FLOAT8 energy[4],
                    int blocktype_d[2])
{
/* to get a good cache performance, one has to think about
 * the sequence, in which the variables are used.  
 * (Note: these static variables have been moved to the gfc-> struct,
 * and their order in memory is layed out in util.h)
 */
  lame_internal_flags *gfc=gfp->internal_flags;


  /* fft and energy calculation   */
  FLOAT (*wsamp_l)[BLKSIZE];
  FLOAT (*wsamp_s)[3][BLKSIZE_s];

  /* convolution   */
  FLOAT8 eb[CBANDS];
  FLOAT8 cb[CBANDS];
  FLOAT8 thr[CBANDS];
  
  /* ratios    */
  FLOAT8 ms_ratio_l=0,ms_ratio_s=0;

  /* block type  */
  int blocktype[2],uselongblock[2];
  
  /* usual variables like loop indices, etc..    */
  int numchn, chn;
  int   b, i, j, k;
  int	sb,sblock;


  if(gfc->psymodel_init==0) {
      psymodel_init(gfp);
      init_fft(gfc);
      gfc->psymodel_init=1;
  }
  


  
  
  numchn = gfc->channels_out;
  /* chn=2 and 3 = Mid and Side channels */
  if (gfp->mode == JOINT_STEREO) numchn=4;

  for (chn=0; chn<numchn; chn++) {
      for (i=0; i<numchn; ++i) {
	  energy[chn]=gfc->tot_ener[chn];
      }
  }
  
  for (chn=0; chn<numchn; chn++) {

      /* there is a one granule delay.  Copy maskings computed last call
       * into masking_ratio to return to calling program.
       */
      if (chn < 2) {    
	  /* LR maskings  */
	  percep_entropy            [chn]       = gfc -> pe  [chn]; 
	  masking_ratio    [gr_out] [chn]  .en  = gfc -> en  [chn];
	  masking_ratio    [gr_out] [chn]  .thm = gfc -> thm [chn];
      } else {
	  /* MS maskings  */
	  percep_MS_entropy         [chn-2]     = gfc -> pe  [chn]; 
	  masking_MS_ratio [gr_out] [chn-2].en  = gfc -> en  [chn];
	  masking_MS_ratio [gr_out] [chn-2].thm = gfc -> thm [chn];
      }
      
      

    /**********************************************************************
     *  compute FFTs
     **********************************************************************/
    wsamp_s = gfc->wsamp_S+(chn & 1);
    wsamp_l = gfc->wsamp_L+(chn & 1);
    if (chn<2) {    
      fft_long ( gfc, *wsamp_l, chn, buffer);
      fft_short( gfc, *wsamp_s, chn, buffer);
    } 
    /* FFT data for mid and side channel is derived from L & R */
    if (chn == 2)
      {
        for (j = BLKSIZE-1; j >=0 ; --j)
        {
          FLOAT l = gfc->wsamp_L[0][j];
          FLOAT r = gfc->wsamp_L[1][j];
          gfc->wsamp_L[0][j] = (l+r)*(FLOAT)(SQRT2*0.5);
          gfc->wsamp_L[1][j] = (l-r)*(FLOAT)(SQRT2*0.5);
        }
        for (b = 2; b >= 0; --b)
        {
          for (j = BLKSIZE_s-1; j >= 0 ; --j)
          {
            FLOAT l = gfc->wsamp_S[0][b][j];
            FLOAT r = gfc->wsamp_S[1][b][j];
            gfc->wsamp_S[0][b][j] = (l+r)*(FLOAT)(SQRT2*0.5);
            gfc->wsamp_S[1][b][j] = (l-r)*(FLOAT)(SQRT2*0.5);
          }
        }
      }


    /**********************************************************************
     *  compute energies
     **********************************************************************/
    
    
    
    gfc->energy[0]  = (*wsamp_l)[0];
    gfc->energy[0] *= gfc->energy[0];
    
    gfc->tot_ener[chn] = gfc->energy[0]; /* sum total energy at nearly no extra cost */
    
    for (j=BLKSIZE/2-1; j >= 0; --j)
    {
      FLOAT re = (*wsamp_l)[BLKSIZE/2-j];
      FLOAT im = (*wsamp_l)[BLKSIZE/2+j];
      gfc->energy[BLKSIZE/2-j] = (re * re + im * im) * (FLOAT)0.5;

      if (BLKSIZE/2-j > 10)
	gfc->tot_ener[chn] += gfc->energy[BLKSIZE/2-j];
    }
    for (b = 2; b >= 0; --b)
    {
      gfc->energy_s[b][0]  = (*wsamp_s)[b][0];
      gfc->energy_s[b][0] *=  gfc->energy_s [b][0];
      for (j=BLKSIZE_s/2-1; j >= 0; --j)
      {
        FLOAT re = (*wsamp_s)[b][BLKSIZE_s/2-j];
        FLOAT im = (*wsamp_s)[b][BLKSIZE_s/2+j];
        gfc->energy_s[b][BLKSIZE_s/2-j] = (re * re + im * im) * (FLOAT)0.5;
      }
    }


  if (gfp->analysis) {
    for (j=0; j<HBLKSIZE ; j++) {
      gfc->pinfo->energy[gr_out][chn][j]=gfc->energy_save[chn][j];
      gfc->energy_save[chn][j]=gfc->energy[j];
    }
  }
    
    /**********************************************************************
     *    compute unpredicatability of first six spectral lines            * 
     **********************************************************************/
    for ( j = 0; j < gfc->cw_lower_index; j++ )
      {	 /* calculate unpredictability measure cw */
	FLOAT an, a1, a2;
	FLOAT bn, b1, b2;
	FLOAT rn, r1, r2;
	FLOAT numre, numim, den;

	a2 = gfc-> ax_sav[chn][1][j];
	b2 = gfc-> bx_sav[chn][1][j];
	r2 = gfc-> rx_sav[chn][1][j];
	a1 = gfc-> ax_sav[chn][1][j] = gfc-> ax_sav[chn][0][j];
	b1 = gfc-> bx_sav[chn][1][j] = gfc-> bx_sav[chn][0][j];
	r1 = gfc-> rx_sav[chn][1][j] = gfc-> rx_sav[chn][0][j];
	an = gfc-> ax_sav[chn][0][j] = (*wsamp_l)[j];
	bn = gfc-> bx_sav[chn][0][j] = j==0 ? (*wsamp_l)[0] : (*wsamp_l)[BLKSIZE-j];  
	rn = gfc-> rx_sav[chn][0][j] = sqrt(gfc->energy[j]);

	{ /* square (x1,y1) */
	  if( r1 != 0 ) {
	    numre = (a1*b1);
	    numim = (a1*a1-b1*b1)*(FLOAT)0.5;
	    den = r1*r1;
	  } else {
	    numre = 1;
	    numim = 0;
	    den = 1;
	  }
	}
	
	{ /* multiply by (x2,-y2) */
	  if( r2 != 0 ) {
	    FLOAT tmp2 = (numim+numre)*(a2+b2)*(FLOAT)0.5;
	    FLOAT tmp1 = -a2*numre+tmp2;
	    numre =       -b2*numim+tmp2;
	    numim = tmp1;
	    den *= r2;
	  } else {
	    /* do nothing */
	  }
	}
	
	{ /* r-prime factor */
	  FLOAT tmp = (2*r1-r2)/den;
	  numre *= tmp;
	  numim *= tmp;
	}
	den=rn+fabs(2*r1-r2);
	if( den != 0 ) {
	  numre = (an+bn)*(FLOAT)0.5-numre;
	  numim = (an-bn)*(FLOAT)0.5-numim;
	  den = sqrt(numre*numre+numim*numim)/den;
	}
	gfc->cw[j] = den;
      }



    /**********************************************************************
     *     compute unpredicatibility of next 200 spectral lines            *
     **********************************************************************/ 
    for ( j = gfc->cw_lower_index; j < gfc->cw_upper_index; j += 4 )
      {/* calculate unpredictability measure cw */
	FLOAT rn, r1, r2;
	FLOAT numre, numim, den;
	
	k = (j+2) / 4; 
	
	{ /* square (x1,y1) */
	  r1 = gfc->energy_s[0][k];
	  if( r1 != 0 ) {
	    FLOAT a1 = (*wsamp_s)[0][k]; 
	    FLOAT b1 = (*wsamp_s)[0][BLKSIZE_s-k]; /* k is never 0 */
	    numre = (a1*b1);
	    numim = (a1*a1-b1*b1)*(FLOAT)0.5;
	    den = r1;
	    r1 = sqrt(r1);
	  } else {
	    numre = 1;
	    numim = 0;
	    den = 1;
	  }
	}
	
	
	{ /* multiply by (x2,-y2) */
	  r2 = gfc->energy_s[2][k];
	  if( r2 != 0 ) {
	    FLOAT a2 = (*wsamp_s)[2][k]; 
	    FLOAT b2 = (*wsamp_s)[2][BLKSIZE_s-k];
	    
	    
	    FLOAT tmp2 = (numim+numre)*(a2+b2)*(FLOAT)0.5;
	    FLOAT tmp1 = -a2*numre+tmp2;
	    numre =       -b2*numim+tmp2;
	    numim = tmp1;
	    
	    r2 = sqrt(r2);
	    den *= r2;
	  } else {
	    /* do nothing */
	  }
	}
	
	{ /* r-prime factor */
	  FLOAT tmp = (2*r1-r2)/den;
	  numre *= tmp;
	  numim *= tmp;
	}
	
	rn = sqrt(gfc->energy_s[1][k]);
	den=rn+fabs(2*r1-r2);
	if( den != 0 ) {
	  FLOAT an = (*wsamp_s)[1][k]; 
	  FLOAT bn = (*wsamp_s)[1][BLKSIZE_s-k];
	  numre = (an+bn)*(FLOAT)0.5-numre;
	  numim = (an-bn)*(FLOAT)0.5-numim;
	  den = sqrt(numre*numre+numim*numim)/den;
	}
	
	gfc->cw[j+1] = gfc->cw[j+2] = gfc->cw[j+3] = gfc->cw[j] = den;
      }
    
#if 0
    for ( j = 14; j < HBLKSIZE-4; j += 4 )
      {/* calculate energy from short ffts */
	FLOAT8 tot,ave;
	k = (j+2) / 4; 
	for (tot=0, sblock=0; sblock < 3; sblock++)
	  tot+=gfc->energy_s[sblock][k];
	ave = gfc->energy[j+1]+ gfc->energy[j+2]+ gfc->energy[j+3]+ gfc->energy[j];
	ave /= 4.;
	gfc->energy[j+1] = gfc->energy[j+2] = gfc->energy[j+3] =  gfc->energy[j]=tot;
      }
#endif
    
    /**********************************************************************
     *    Calculate the energy and the unpredictability in the threshold   *
     *    calculation partitions                                           *
     **********************************************************************/

    b = 0;
    for (j = 0; j < gfc->cw_upper_index && gfc->numlines_l[b] && b<gfc->npart_l_orig; )
      {
	FLOAT8 ebb, cbb;

	ebb = gfc->energy[j];
	cbb = gfc->energy[j] * gfc->cw[j];
	j++;

	for (i = gfc->numlines_l[b] - 1; i > 0; i--)
	  {
	    ebb += gfc->energy[j];
	    cbb += gfc->energy[j] * gfc->cw[j];
	    j++;
	  }
	eb[b] = ebb;
	cb[b] = cbb;
	b++;
      }

    for (; b < gfc->npart_l_orig; b++ )
      {
	FLOAT8 ebb = gfc->energy[j++];
	assert(gfc->numlines_l[b]);
	for (i = gfc->numlines_l[b] - 1; i > 0; i--)
	  {
	    ebb += gfc->energy[j++];
	  }
	eb[b] = ebb;
	cb[b] = ebb * 0.4;
      }

    /**********************************************************************
     *      convolve the partitioned energy and unpredictability           *
     *      with the spreading function, s3_l[b][k]                        *
     ******************************************************************** */
    gfc->pe[chn] = 0;		/*  calculate percetual entropy */
    for ( b = 0;b < gfc->npart_l; b++ )
      {
	FLOAT8 tbb,ecb,ctb;

	ecb = 0;
	ctb = 0;

	for ( k = gfc->s3ind[b][0]; k <= gfc->s3ind[b][1]; k++ )
	  {
	    ecb += gfc->s3_l[b][k] * eb[k];	/* sprdngf for Layer III */
	    ctb += gfc->s3_l[b][k] * cb[k];
	  }

	/* calculate the tonality of each threshold calculation partition 
	 * calculate the SNR in each threshold calculation partition 
	 * tonality = -0.299 - .43*log(ctb/ecb);
	 * tonality = 0:           use NMT   (lots of masking)
	 * tonality = 1:           use TMN   (little masking)
	 */

/* ISO values */
#define CONV1 (-.299)
#define CONV2 (-.43)

	tbb = ecb;
	if (tbb != 0)
	  {
	    tbb = ctb / tbb;
	    if (tbb <= exp((1-CONV1)/CONV2)) 
	      { /* tonality near 1 */
		tbb = exp(-LN_TO_LOG10 * TMN);
	      }
	    else if (tbb >= exp((0-CONV1)/CONV2)) 
	      {/* tonality near 0 */
		tbb = exp(-LN_TO_LOG10 * NMT);
	      }
	    else
	      {
		/* convert to tonality index */
		/* tonality small:   tbb=1 */
		/* tonality large:   tbb=-.299 */
		tbb = CONV1 + CONV2*log(tbb);
		tbb = exp(-LN_TO_LOG10 * ( TMN*tbb + (1-tbb)*NMT) );
	      }
	  }

	/* at this point, tbb represents the amount the spreading function
	 * will be reduced.  The smaller the value, the less masking.
	 * minval[] = 1 (0db)     says just use tbb.
	 * minval[]= .01 (-20db)  says reduce spreading function by 
	 *                        at least 20db.  
	 */

	tbb = Min(gfc->minval[b], tbb);
	ecb *= tbb;

	/* long block pre-echo control.   */
	/* rpelev=2.0, rpelev2=16.0 */
	/* note: all surges in PE are because of this pre-echo formula
	 * for thr[b].  If it this is not used, PE is always around 600
	 */
	/* dont use long block pre-echo control if previous granule was 
	 * a short block.  This is to avoid the situation:   
	 * frame0:  quiet (very low masking)  
	 * frame1:  surge  (triggers short blocks)
	 * frame2:  regular frame.  looks like pre-echo when compared to 
	 *          frame0, but all pre-echo was in frame1.
	 */
	/* chn=0,1   L and R channels
	   chn=2,3   S and M channels.  
	*/
        
        if (vbr_mtrh == gfp->VBR) {
            thr[b] = Min (rpelev*gfc->nb_1[chn][b], rpelev2*gfc->nb_2[chn][b]);
            thr[b] = Max (thr[b], gfc->ATH->adjust * gfc->ATH->cb[b]);
            thr[b] = Min (thr[b], ecb);
	}
        else if (gfc->blocktype_old[chn>1 ? chn-2 : chn] == SHORT_TYPE )
	  thr[b] = ecb; /* Min(ecb, rpelev*gfc->nb_1[chn][b]); */
	else
	  thr[b] = Min(ecb, Min(rpelev*gfc->nb_1[chn][b],rpelev2*gfc->nb_2[chn][b]) );

	gfc->nb_2[chn][b] = gfc->nb_1[chn][b];
	gfc->nb_1[chn][b] = ecb;

	{
	  FLOAT8 thrpe;
	  thrpe = Max(thr[b],gfc->ATH->cb[b]);
	  /*
	    printf("%i thr=%e   ATH=%e  \n",b,thr[b],gfc->ATH->cb[b]);
	  */
	  if (thrpe < eb[b])
	    gfc->pe[chn] -= gfc->numlines_l[b] * log(thrpe / eb[b]);
	}
      }

    /*************************************************************** 
     * determine the block type (window type) based on L & R channels
     * 
     ***************************************************************/
    {  /* compute PE for all 4 channels */
      FLOAT mn,mx,ma=0,mb=0,mc=0;
      
      for ( j = HBLKSIZE_s/2; j < HBLKSIZE_s; j ++)
	{
	  ma += gfc->energy_s[0][j];
	  mb += gfc->energy_s[1][j];
	  mc += gfc->energy_s[2][j];
	}
      mn = Min(ma,mb);
      mn = Min(mn,mc);
      mx = Max(ma,mb);
      mx = Max(mx,mc);
      
      /* bit allocation is based on pe.  */
      if (mx>mn) {
	FLOAT8 tmp = 400*log(mx/(1e-12+mn));
	if (tmp>gfc->pe[chn]) gfc->pe[chn]=tmp;
      }

      /* block type is based just on L or R channel */      
      if (chn<2) {
	uselongblock[chn] = 1;
	
	/* tuned for t1.wav.  doesnt effect most other samples */
	if (gfc->pe[chn] > 3000) 
	  uselongblock[chn]=0;
	
	if ( mx > 30*mn ) 
	  {/* big surge of energy - always use short blocks */
	    uselongblock[chn] = 0;
	  } 
	else if ((mx > 10*mn) && (gfc->pe[chn] > 1000))
	  {/* medium surge, medium pe - use short blocks */
	    uselongblock[chn] = 0;
	  }
	
	/* disable short blocks */
	if (gfp->no_short_blocks)
	  uselongblock[chn]=1;
      }
    }

    if (gfp->analysis) {
      FLOAT mn,mx,ma=0,mb=0,mc=0;

      for ( j = HBLKSIZE_s/2; j < HBLKSIZE_s; j ++)
      {
        ma += gfc->energy_s[0][j];
        mb += gfc->energy_s[1][j];
        mc += gfc->energy_s[2][j];
      }
      mn = Min(ma,mb);
      mn = Min(mn,mc);
      mx = Max(ma,mb);
      mx = Max(mx,mc);

      gfc->pinfo->ers[gr_out][chn]=gfc->ers_save[chn];
      gfc->ers_save[chn]=(mx/(1e-12+mn));
      gfc->pinfo->pe[gr_out][chn]=gfc->pe_save[chn];
      gfc->pe_save[chn]=gfc->pe[chn];
    }


    /*************************************************************** 
     * compute masking thresholds for both short and long blocks
     ***************************************************************/
    /* longblock threshold calculation (part 2) */
    for ( sb = 0; sb < NBPSY_l; sb++ )
      {
	FLOAT8 enn = gfc->w1_l[sb] * eb[gfc->bu_l[sb]] + gfc->w2_l[sb] * eb[gfc->bo_l[sb]];
	FLOAT8 thmm = gfc->w1_l[sb] *thr[gfc->bu_l[sb]] + gfc->w2_l[sb] * thr[gfc->bo_l[sb]];

        for ( b = gfc->bu_l[sb]+1; b < gfc->bo_l[sb]; b++ )
          {
            enn  += eb[b];
            thmm += thr[b];
          }

	gfc->en [chn].l[sb] = enn;
	gfc->thm[chn].l[sb] = thmm;
      }
    

    /* threshold calculation for short blocks */
    for ( sblock = 0; sblock < 3; sblock++ )
      {
	j = 0;
	for ( b = 0; b < gfc->npart_s_orig; b++ )
	  {
	    FLOAT ecb = gfc->energy_s[sblock][j++];
	    for (i = 1 ; i<gfc->numlines_s[b]; ++i)
	      {
		ecb += gfc->energy_s[sblock][j++];
	      }
	    eb[b] = ecb;
	  }

	for ( b = 0; b < gfc->npart_s; b++ )
	  {
	    FLOAT8 ecb = 0;
	    for ( k = gfc->s3ind_s[b][0]; k <= gfc->s3ind_s[b][1]; k++ ) {
		ecb += gfc->s3_s[b][k] * eb[k];
	    }
            ecb *= gfc->SNR_s[b];
	    thr[b] = Max (1e-6, ecb);
	  }

	for ( sb = 0; sb < NBPSY_s; sb++ ){
            FLOAT8 enn  = gfc->w1_s[sb] * eb[gfc->bu_s[sb]] + gfc->w2_s[sb] * eb[gfc->bo_s[sb]];
	    FLOAT8 thmm = gfc->w1_s[sb] *thr[gfc->bu_s[sb]] + gfc->w2_s[sb] * thr[gfc->bo_s[sb]];
	    
            for ( b = gfc->bu_s[sb]+1; b < gfc->bo_s[sb]; b++ ) {
		enn  += eb[b];
		thmm += thr[b];
	    }

	    gfc->en [chn].s[sb][sblock] = enn;
	    gfc->thm[chn].s[sb][sblock] = thmm;
	  }
      }
  } /* end loop over chn */


  /* compute M/S thresholds from Johnston & Ferreira 1992 ICASSP paper */
  if (gfp->mode == JOINT_STEREO) {
    FLOAT8 rside,rmid,mld;
    int chmid=2,chside=3; 
    
    for ( sb = 0; sb < NBPSY_l; sb++ ) {
      /* use this fix if L & R masking differs by 2db or less */
      /* if db = 10*log10(x2/x1) < 2 */
      /* if (x2 < 1.58*x1) { */
      if (gfc->thm[0].l[sb] <= 1.58*gfc->thm[1].l[sb]
	  && gfc->thm[1].l[sb] <= 1.58*gfc->thm[0].l[sb]) {

	mld = gfc->mld_l[sb]*gfc->en[chside].l[sb];
	rmid = Max(gfc->thm[chmid].l[sb], Min(gfc->thm[chside].l[sb],mld));

	mld = gfc->mld_l[sb]*gfc->en[chmid].l[sb];
	rside = Max(gfc->thm[chside].l[sb],Min(gfc->thm[chmid].l[sb],mld));

	gfc->thm[chmid].l[sb]=rmid;
	gfc->thm[chside].l[sb]=rside;
      }
    }
    for ( sb = 0; sb < NBPSY_s; sb++ ) {
      for ( sblock = 0; sblock < 3; sblock++ ) {
	if (gfc->thm[0].s[sb][sblock] <= 1.58*gfc->thm[1].s[sb][sblock]
	    && gfc->thm[1].s[sb][sblock] <= 1.58*gfc->thm[0].s[sb][sblock]) {

	  mld = gfc->mld_s[sb]*gfc->en[chside].s[sb][sblock];
	  rmid = Max(gfc->thm[chmid].s[sb][sblock],Min(gfc->thm[chside].s[sb][sblock],mld));

	  mld = gfc->mld_s[sb]*gfc->en[chmid].s[sb][sblock];
	  rside = Max(gfc->thm[chside].s[sb][sblock],Min(gfc->thm[chmid].s[sb][sblock],mld));

	  gfc->thm[chmid].s[sb][sblock]=rmid;
	  gfc->thm[chside].s[sb][sblock]=rside;
	}
      }
    }
  }

  if (gfp->mode == JOINT_STEREO)  {
    /* determin ms_ratio from masking thresholds*/
    /* use ms_stereo (ms_ratio < .35) if average thresh. diff < 5 db */
    FLOAT8 db,x1,x2,sidetot=0,tot=0;
    for (sb= NBPSY_l/4 ; sb< NBPSY_l; sb ++ ) {
      x1 = Min(gfc->thm[0].l[sb],gfc->thm[1].l[sb]);
      x2 = Max(gfc->thm[0].l[sb],gfc->thm[1].l[sb]);
      /* thresholds difference in db */
      if (x2 >= 1000*x1)  db=3;
      else db = log10(x2/x1);  
      /*  DEBUGF(gfc,"db = %f %e %e  \n",db,gfc->thm[0].l[sb],gfc->thm[1].l[sb]);*/
      sidetot += db;
      tot++;
    }
    ms_ratio_l= (sidetot/tot)*0.7; /* was .35*(sidetot/tot)/5.0*10 */
    ms_ratio_l = Min(ms_ratio_l,0.5);
    
    sidetot=0; tot=0;
    for ( sblock = 0; sblock < 3; sblock++ )
      for ( sb = NBPSY_s/4; sb < NBPSY_s; sb++ ) {
	x1 = Min(gfc->thm[0].s[sb][sblock],gfc->thm[1].s[sb][sblock]);
	x2 = Max(gfc->thm[0].s[sb][sblock],gfc->thm[1].s[sb][sblock]);
	/* thresholds difference in db */
	if (x2 >= 1000*x1)  db=3;
	else db = log10(x2/x1);  
	sidetot += db;
	tot++;
      }
    ms_ratio_s = (sidetot/tot)*0.7; /* was .35*(sidetot/tot)/5.0*10 */
    ms_ratio_s = Min(ms_ratio_s,.5);
  }

  /*************************************************************** 
   * determine final block type
   ***************************************************************/

  for (chn=0; chn<gfc->channels_out; chn++) {
    blocktype[chn] = NORM_TYPE;
  }


  if (gfc->channels_out==2) {
    if (!gfp->allow_diff_short || gfp->mode==JOINT_STEREO) {
      /* force both channels to use the same block type */
      /* this is necessary if the frame is to be encoded in ms_stereo.  */
      /* But even without ms_stereo, FhG  does this */
      int bothlong= (uselongblock[0] && uselongblock[1]);
      if (!bothlong) {
	uselongblock[0]=0;
	uselongblock[1]=0;
      }
    }
  }

  
  
  /* update the blocktype of the previous granule, since it depends on what
   * happend in this granule */
  for (chn=0; chn<gfc->channels_out; chn++) {
    if ( uselongblock[chn])
      {				/* no attack : use long blocks */
	assert( gfc->blocktype_old[chn] != START_TYPE );
	switch( gfc->blocktype_old[chn] ) 
	  {
	  case NORM_TYPE:
	  case STOP_TYPE:
	    blocktype[chn] = NORM_TYPE;
	    break;
	  case SHORT_TYPE:
	    blocktype[chn] = STOP_TYPE; 
	    break;
	  }
      } else   {
	/* attack : use short blocks */
	blocktype[chn] = SHORT_TYPE;
	if ( gfc->blocktype_old[chn] == NORM_TYPE ) {
	  gfc->blocktype_old[chn] = START_TYPE;
	}
	if ( gfc->blocktype_old[chn] == STOP_TYPE ) {
	  gfc->blocktype_old[chn] = SHORT_TYPE ;
	}
      }
    
    blocktype_d[chn] = gfc->blocktype_old[chn];  /* value returned to calling program */
    gfc->blocktype_old[chn] = blocktype[chn];    /* save for next call to l3psy_anal */
  }
  
  if (blocktype_d[0]==2) 
    *ms_ratio = gfc->ms_ratio_s_old;
  else
    *ms_ratio = gfc->ms_ratio_l_old;

  gfc->ms_ratio_s_old = ms_ratio_s;
  gfc->ms_ratio_l_old = ms_ratio_l;

  /* we dont know the block type of this frame yet - assume long */
  *ms_ratio_next = ms_ratio_l;

  return 0;
}

/* addition of simultaneous masking   Naoki Shibata 2000/7 */
inline static FLOAT8 mask_add(FLOAT8 m1,FLOAT8 m2,int k,int b, lame_internal_flags * const gfc)
{
  static const FLOAT8 table1[] = {
    3.3246 *3.3246 ,3.23837*3.23837,3.15437*3.15437,3.00412*3.00412,2.86103*2.86103,2.65407*2.65407,2.46209*2.46209,2.284  *2.284  ,
    2.11879*2.11879,1.96552*1.96552,1.82335*1.82335,1.69146*1.69146,1.56911*1.56911,1.46658*1.46658,1.37074*1.37074,1.31036*1.31036,
    1.25264*1.25264,1.20648*1.20648,1.16203*1.16203,1.12765*1.12765,1.09428*1.09428,1.0659 *1.0659 ,1.03826*1.03826,1.01895*1.01895,
    1
  };

  static const FLOAT8 table2[] = {
    1.33352*1.33352,1.35879*1.35879,1.38454*1.38454,1.39497*1.39497,1.40548*1.40548,1.3537 *1.3537 ,1.30382*1.30382,1.22321*1.22321,
    1.14758*1.14758
  };

  static const FLOAT8 table3[] = {
    2.35364*2.35364,2.29259*2.29259,2.23313*2.23313,2.12675*2.12675,2.02545*2.02545,1.87894*1.87894,1.74303*1.74303,1.61695*1.61695,
    1.49999*1.49999,1.39148*1.39148,1.29083*1.29083,1.19746*1.19746,1.11084*1.11084,1.03826*1.03826
  };


  int i;
  FLOAT8 m;

  if (m1 == 0) return m2;

  if (b < 0) b = -b;

  i = 10*log10(m2 / m1)/10*16;
  m = 10*log10((m1+m2)/gfc->ATH->cb[k]);

  if (i < 0) i = -i;

  if (b <= 3) {  /* approximately, 1 bark = 3 partitions */
    if (i > 8) return m1+m2;
    return (m1+m2)*table2[i];
  }

  if (m<15) {
    if (m > 0) {
      FLOAT8 f=1.0,r;
      if (i > 24) return m1+m2;
      if (i > 13) f = 1; else f = table3[i];
      r = (m-0)/15;
      return (m1+m2)*(table1[i]*r+f*(1-r));
    }
    if (i > 13) return m1+m2;
    return (m1+m2)*table3[i];
  }

  if (i > 24) return m1+m2;
  return (m1+m2)*table1[i];
}

int L3psycho_anal_ns( lame_global_flags * gfp,
                    const sample_t *buffer[2], int gr_out, 
                    FLOAT8 *ms_ratio,
                    FLOAT8 *ms_ratio_next,
		    III_psy_ratio masking_ratio[2][2],
		    III_psy_ratio masking_MS_ratio[2][2],
		    FLOAT8 percep_entropy[2],FLOAT8 percep_MS_entropy[2], 
		    FLOAT8 energy[4], 
                    int blocktype_d[2])
{
/* to get a good cache performance, one has to think about
 * the sequence, in which the variables are used.  
 * (Note: these static variables have been moved to the gfc-> struct,
 * and their order in memory is layed out in util.h)
 */
  lame_internal_flags *gfc=gfp->internal_flags;

  /* fft and energy calculation   */
  FLOAT (*wsamp_l)[BLKSIZE];
  FLOAT (*wsamp_s)[3][BLKSIZE_s];

  /* convolution   */
  FLOAT8 eb[CBANDS],eb2[CBANDS];
  FLOAT8 thr[CBANDS];
  
  FLOAT8 max[CBANDS],avg[CBANDS],tonality2[CBANDS];

  /* ratios    */
  FLOAT8 ms_ratio_l=0,ms_ratio_s=0;

  /* block type  */
  int blocktype[2],uselongblock[2];
  
  /* usual variables like loop indices, etc..    */
  int numchn, chn;
  int   b, i, j, k;
  int	sb,sblock;

  /* variables used for --nspsytune */
  int ns_attacks[4];
  FLOAT ns_hpfsmpl[4][576+576/3+NSFIRLEN];
  FLOAT pe_l[4],pe_s[4];
  FLOAT pcfact;


  if(gfc->psymodel_init==0) {
    psymodel_init(gfp);
    init_fft(gfc);
    gfc->psymodel_init=1;
  }


  numchn = gfc->channels_out;
  /* chn=2 and 3 = Mid and Side channels */
  if (gfp->mode == JOINT_STEREO) numchn=4;

  if (gfp->VBR==vbr_off) pcfact = gfc->ResvMax == 0 ? 0 : ((FLOAT)gfc->ResvSize)/gfc->ResvMax*0.5;
  else if (gfp->VBR == vbr_rh  ||  gfp->VBR == vbr_mtrh  ||  gfp->VBR == vbr_mt) {
    static const FLOAT8 pcQns[10]={1.0,1.0,1.0,0.8,0.6,0.5,0.4,0.3,0.2,0.1};
    pcfact = pcQns[gfp->VBR_q];
  } else pcfact = 1;

  /**********************************************************************
   *  Apply HPF of fs/4 to the input signal.
   *  This is used for attack detection / handling.
   **********************************************************************/

  {
    static const FLOAT fircoef[] = {
      -8.65163e-18,-0.00851586,-6.74764e-18, 0.0209036,
      -3.36639e-17,-0.0438162 ,-1.54175e-17, 0.0931738,
      -5.52212e-17,-0.313819  , 0.5        ,-0.313819,
      -5.52212e-17, 0.0931738 ,-1.54175e-17,-0.0438162,
      -3.36639e-17, 0.0209036 ,-6.74764e-18,-0.00851586,
      -8.65163e-18,
    };

    for(chn=0;chn<gfc->channels_out;chn++)
      {
	FLOAT firbuf[576+576/3+NSFIRLEN];

	/* apply high pass filter of fs/4 */
	
	for(i=-NSFIRLEN;i<576+576/3;i++)
	  firbuf[i+NSFIRLEN] = buffer[chn][576-350+(i)];

	for(i=0;i<576+576/3-NSFIRLEN;i++)
          {
	    FLOAT sum = 0;
	    for(j=0;j<NSFIRLEN;j++)
	      sum += fircoef[j] * firbuf[i+j];
	    ns_hpfsmpl[chn][i] = sum;
	  }
	for(;i<576+576/3;i++)
	  ns_hpfsmpl[chn][i] = 0;
      }

    if (gfp->mode == JOINT_STEREO) {
      for(i=0;i<576+576/3;i++)
	{
	  ns_hpfsmpl[2][i] = ns_hpfsmpl[0][i]+ns_hpfsmpl[1][i];
	  ns_hpfsmpl[3][i] = ns_hpfsmpl[0][i]-ns_hpfsmpl[1][i];
	}
    }
  }
  


  /* there is a one granule delay.  Copy maskings computed last call
   * into masking_ratio to return to calling program.
   */
  for (chn=0; chn<numchn; chn++) {
      for (i=0; i<numchn; ++i) {
	  energy[chn]=gfc->tot_ener[chn];
      }
  }


  for (chn=0; chn<numchn; chn++) {
    /* there is a one granule delay.  Copy maskings computed last call
     * into masking_ratio to return to calling program.
     */
    pe_l[chn] = gfc->nsPsy.pe_l[chn];
    pe_s[chn] = gfc->nsPsy.pe_s[chn];

    if (chn < 2) {    
      /* LR maskings  */
      //percep_entropy            [chn]       = gfc -> pe  [chn]; 
      masking_ratio    [gr_out] [chn]  .en  = gfc -> en  [chn];
      masking_ratio    [gr_out] [chn]  .thm = gfc -> thm [chn];
    } else {
      /* MS maskings  */
      //percep_MS_entropy         [chn-2]     = gfc -> pe  [chn]; 
      masking_MS_ratio [gr_out] [chn-2].en  = gfc -> en  [chn];
      masking_MS_ratio [gr_out] [chn-2].thm = gfc -> thm [chn];
    }
  }

  for (chn=0; chn<numchn; chn++) {

    /**********************************************************************
     *  compute FFTs
     **********************************************************************/

    wsamp_s = gfc->wsamp_S+(chn & 1);
    wsamp_l = gfc->wsamp_L+(chn & 1);

    if (chn<2) {    
      fft_long ( gfc, *wsamp_l, chn, buffer);
      fft_short( gfc, *wsamp_s, chn, buffer);
    } 

    /* FFT data for mid and side channel is derived from L & R */

    if (chn == 2)
      {
        for (j = BLKSIZE-1; j >=0 ; --j)
	  {
	    FLOAT l = gfc->wsamp_L[0][j];
	    FLOAT r = gfc->wsamp_L[1][j];
	    gfc->wsamp_L[0][j] = (l+r)*(FLOAT)(SQRT2*0.5);
	    gfc->wsamp_L[1][j] = (l-r)*(FLOAT)(SQRT2*0.5);
	  }
        for (b = 2; b >= 0; --b)
	  {
	    for (j = BLKSIZE_s-1; j >= 0 ; --j)
	      {
		FLOAT l = gfc->wsamp_S[0][b][j];
		FLOAT r = gfc->wsamp_S[1][b][j];
		gfc->wsamp_S[0][b][j] = (l+r)*(FLOAT)(SQRT2*0.5);
		gfc->wsamp_S[1][b][j] = (l-r)*(FLOAT)(SQRT2*0.5);
	      }
	  }
      }


    /**********************************************************************
     *  compute energies for each spectral line
     **********************************************************************/
    
    /* long block */

    gfc->energy[0]  = (*wsamp_l)[0];
    gfc->energy[0] *= gfc->energy[0];
    
    gfc->tot_ener[chn] = gfc->energy[0]; /* sum total energy at nearly no extra cost */
    
    for (j=BLKSIZE/2-1; j >= 0; --j)
    {
      FLOAT re = (*wsamp_l)[BLKSIZE/2-j];
      FLOAT im = (*wsamp_l)[BLKSIZE/2+j];
      gfc->energy[BLKSIZE/2-j] = (re * re + im * im) * (FLOAT)0.5;

      if (BLKSIZE/2-j > 10)
	gfc->tot_ener[chn] += gfc->energy[BLKSIZE/2-j];
    }


    /* short block */

    for (b = 2; b >= 0; --b)
    {
      gfc->energy_s[b][0]  = (*wsamp_s)[b][0];
      gfc->energy_s[b][0] *=  gfc->energy_s [b][0];
      for (j=BLKSIZE_s/2-1; j >= 0; --j)
      {
        FLOAT re = (*wsamp_s)[b][BLKSIZE_s/2-j];
        FLOAT im = (*wsamp_s)[b][BLKSIZE_s/2+j];
        gfc->energy_s[b][BLKSIZE_s/2-j] = (re * re + im * im) * (FLOAT)0.5;
      }
    }


    /* output data for analysis */

    if (gfp->analysis) {
      for (j=0; j<HBLKSIZE ; j++) {
	gfc->pinfo->energy[gr_out][chn][j]=gfc->energy_save[chn][j];
	gfc->energy_save[chn][j]=gfc->energy[j];
      }
    }
    

    /**********************************************************************
     *    Calculate the energy and the tonality of each partition.
     **********************************************************************/

    for (b=0, j=0; b<gfc->npart_l_orig; b++)
      {
	FLOAT8 ebb,m,a;
  
	ebb = gfc->energy[j];
	m = a = gfc->energy[j];
	j++;
  
	for (i = gfc->numlines_l[b] - 1; i > 0; i--)
	  {
	    FLOAT8 el = gfc->energy[j];
	    ebb += gfc->energy[j];
	    a += el;
	    m = m < el ? el : m;
	    j++;
	  }
	eb[b] = ebb;
	max[b] = m;
	avg[b] = a / gfc->numlines_l[b];
      }
  
    j = 0;
    for (b=0; b < gfc->npart_l_orig; b++ )
      {
	int c1,c2;
	FLOAT8 m,a;
	tonality2[b] = 0;
	c1 = c2 = 0;
	m = a = 0;
	for(k=b-1;k<=b+1;k++)
	  {
	    if (k >= 0 && k < gfc->npart_l_orig) {
	      c1++;
	      c2 += gfc->numlines_l[k];
	      a += avg[k];
	      m = m < max[k] ? max[k] : m;
	    }
	  }
 
	a /= c1;
	tonality2[b] = a == 0 ? 0 : (m / a - 1)/(c2-1);
      }

    for (b=0; b < gfc->npart_l_orig; b++ )
      {
#if 0
	static FLOAT8 tab[20] =
	{  0,  1,  2,  2,  2,  2,  2,  6,9.3,9.3,9.3,9.3,9.3,9.3,9.3,9.3,9.3,9.3,9.3,9.3};

	static int init = 1;
	if (init) {
	  int j;
	  for(j=0;j<20;j++) {
	    tab[j] = pow(10.0,-tab[j]/10.0);
	  }
	  init = 0;
	}
#else
	static FLOAT8 tab[20] = {
	  1,0.79433,0.63096,0.63096,0.63096,0.63096,0.63096,0.25119,0.11749,0.11749,
	  0.11749,0.11749,0.11749,0.11749,0.11749,0.11749,0.11749,0.11749,0.11749,0.11749
	};
#endif

	int t = 20*tonality2[b];
	if (t > 19) t = 19;
	eb2[b] = eb[b] * tab[t];
      }


    /**********************************************************************
     *      convolve the partitioned energy and unpredictability
     *      with the spreading function, s3_l[b][k]
     ******************************************************************** */

    for ( b = 0;b < gfc->npart_l; b++ )
      {
	FLOAT8 ecb;

	/****   convolve the partitioned energy with the spreading function   ****/

	ecb = 0;

#if 1
	for ( k = gfc->s3ind[b][0]; k <= gfc->s3ind[b][1]; k++ )
	  {
	    ecb = mask_add(ecb,gfc->s3_l[b][k] * eb2[k],k,k-b,gfc);
	  }

	ecb *= 0.158489319246111; // pow(10,-0.8)
#endif

#if 0
	for ( k = gfc->s3ind[b][0]; k <= gfc->s3ind[b][1]; k++ )
	  {
	    ecb += gfc->s3_l[k][b] * eb2[k];
	  }

	ecb *= 0.223872113856834; // pow(10,-0.65);
#endif

	/****   long block pre-echo control   ****/

	/* dont use long block pre-echo control if previous granule was 
	 * a short block.  This is to avoid the situation:   
	 * frame0:  quiet (very low masking)  
	 * frame1:  surge  (triggers short blocks)
	 * frame2:  regular frame.  looks like pre-echo when compared to 
	 *          frame0, but all pre-echo was in frame1.
	 */

	/* chn=0,1   L and R channels
	   chn=2,3   S and M channels.  
	*/

#define NS_INTERP(x,y,r) (pow((x),(r))*pow((y),1-(r)))

	if (gfc->blocktype_old[chn>1 ? chn-2 : chn] == SHORT_TYPE )
	  thr[b] = ecb; /* Min(ecb, rpelev*gfc->nb_1[chn][b]); */
	else
	  thr[b] = NS_INTERP(Min(ecb, Min(rpelev*gfc->nb_1[chn][b],rpelev2*gfc->nb_2[chn][b])),ecb,pcfact);

	gfc->nb_2[chn][b] = gfc->nb_1[chn][b];
	gfc->nb_1[chn][b] = ecb;
      }


    /*************************************************************** 
     * determine the block type (window type)
     ***************************************************************/

    {
      static int count=0;
      FLOAT en_subshort[12];
      FLOAT attack_intensity[12];
      int ns_uselongblock = 1;

      /* calculate energies of each sub-shortblocks */

      k = 0;
      for(i=0;i<12;i++)
	{
	  en_subshort[i] = 0;
	  for(j=0;j<576/9;j++)
	    {
	      en_subshort[i] += ns_hpfsmpl[chn][k] * ns_hpfsmpl[chn][k];
	      k++;
	    }

	  if (en_subshort[i] < 100) en_subshort[i] = 100;
	}

      /* compare energies between sub-shortblocks */

#define NSATTACKTHRE 150
#define NSATTACKTHRE_S 300

      for(i=0;i<2;i++) {
	attack_intensity[i] = en_subshort[i] / gfc->nsPsy.last_en_subshort[chn][7+i];
      }

      for(;i<12;i++) {
	attack_intensity[i] = en_subshort[i] / en_subshort[i-2];
      }

      ns_attacks[0] = ns_attacks[1] = ns_attacks[2] = ns_attacks[3] = 0;

      for(i=0;i<12;i++)
	{
	  if (!ns_attacks[i/3] && attack_intensity[i] > (chn == 3 ? NSATTACKTHRE_S : NSATTACKTHRE)) ns_attacks[i/3] = (i % 3)+1;
	}

      if (ns_attacks[0] && gfc->nsPsy.last_attacks[chn][2]) ns_attacks[0] = 0;
      if (ns_attacks[1] && ns_attacks[0]) ns_attacks[1] = 0;
      if (ns_attacks[2] && ns_attacks[1]) ns_attacks[2] = 0;
      if (ns_attacks[3] && ns_attacks[2]) ns_attacks[3] = 0;

      if (gfc->nsPsy.last_attacks[chn][2] == 3 ||
	  ns_attacks[0] || ns_attacks[1] || ns_attacks[2] || ns_attacks[3]) ns_uselongblock = 0;

      if (chn < 4) count++;

      for(i=0;i<9;i++)
	{
	  gfc->nsPsy.last_en_subshort[chn][i] = en_subshort[i];
	  gfc->nsPsy.last_attack_intensity[chn][i] = attack_intensity[i];
	}

      if (gfp->no_short_blocks) {
	uselongblock[chn] = 1;
      } else {
	if (chn < 2) {
	  uselongblock[chn] = ns_uselongblock;
	} else {
	  if (ns_uselongblock == 0) uselongblock[0] = uselongblock[1] = 0;
	}
      }
    }

    if (gfp->analysis) {
      FLOAT mn,mx,ma=0,mb=0,mc=0;

      for ( j = HBLKSIZE_s/2; j < HBLKSIZE_s; j ++)
      {
        ma += gfc->energy_s[0][j];
        mb += gfc->energy_s[1][j];
        mc += gfc->energy_s[2][j];
      }
      mn = Min(ma,mb);
      mn = Min(mn,mc);
      mx = Max(ma,mb);
      mx = Max(mx,mc);

      gfc->pinfo->ers[gr_out][chn]=gfc->ers_save[chn];
      gfc->ers_save[chn]=(mx/(1e-12+mn));
    }


    /*************************************************************** 
     * compute masking thresholds for long blocks
     ***************************************************************/

    for ( sb = 0; sb < NBPSY_l; sb++ )
      {
	FLOAT8 enn = gfc->w1_l[sb] * eb[gfc->bu_l[sb]] + gfc->w2_l[sb] * eb[gfc->bo_l[sb]];
	FLOAT8 thmm = gfc->w1_l[sb] *thr[gfc->bu_l[sb]] + gfc->w2_l[sb] * thr[gfc->bo_l[sb]];

        for ( b = gfc->bu_l[sb]+1; b < gfc->bo_l[sb]; b++ )
          {
            enn  += eb[b];
            thmm += thr[b];
          }

	gfc->en [chn].l[sb] = enn;
	gfc->thm[chn].l[sb] = thmm;
      }
    

    /*************************************************************** 
     * compute masking thresholds for short blocks
     ***************************************************************/

    for ( sblock = 0; sblock < 3; sblock++ )
      {
	j = 0;
	for ( b = 0; b < gfc->npart_s_orig; b++ )
	  {
	    FLOAT ecb = gfc->energy_s[sblock][j++];
	    for (i = 1 ; i<gfc->numlines_s[b]; ++i)
	      {
		ecb += gfc->energy_s[sblock][j++];
	      }
	    eb[b] = ecb;
	  }

	for ( b = 0; b < gfc->npart_s; b++ )
	  {
	    FLOAT8 ecb = 0;
	    for ( k = gfc->s3ind_s[b][0]; k <= gfc->s3ind_s[b][1]; k++ )
	      {
		ecb += gfc->s3_s[b][k] * eb[k];
	      }
	    thr[b] = Max (1e-6, ecb);
	  }

	for ( sb = 0; sb < NBPSY_s; sb++ )
	  {
            FLOAT8 enn  = gfc->w1_s[sb] * eb[gfc->bu_s[sb]] + gfc->w2_s[sb] * eb[gfc->bo_s[sb]];
	    FLOAT8 thmm = gfc->w1_s[sb] *thr[gfc->bu_s[sb]] + gfc->w2_s[sb] * thr[gfc->bo_s[sb]];
	    
            for ( b = gfc->bu_s[sb]+1; b < gfc->bo_s[sb]; b++ )
	      {
		enn  += eb[b];
		thmm += thr[b];
	      }

	    /****   short block pre-echo control   ****/

#define NS_PREECHO_ATT0 0.8
#define NS_PREECHO_ATT1 0.6
#define NS_PREECHO_ATT2 0.3

	    thmm *= NS_PREECHO_ATT0;

	    if (ns_attacks[sblock] >= 2) {
	      if (sblock != 0) {
		double p = NS_INTERP(gfc->thm[chn].s[sb][sblock-1],thmm,NS_PREECHO_ATT1*pcfact);
		thmm = Min(thmm,p);
	      } else {
		double p = NS_INTERP(gfc->nsPsy.last_thm[chn][sb][2],thmm,NS_PREECHO_ATT1*pcfact);
		thmm = Min(thmm,p);
	      }
	    } else if (ns_attacks[sblock+1] == 1) {
	      if (sblock != 0) {
		double p = NS_INTERP(gfc->thm[chn].s[sb][sblock-1],thmm,NS_PREECHO_ATT1*pcfact);
		thmm = Min(thmm,p);
	      } else {
		double p = NS_INTERP(gfc->nsPsy.last_thm[chn][sb][2],thmm,NS_PREECHO_ATT1*pcfact);
		thmm = Min(thmm,p);
	      }
	    }

	    if (ns_attacks[sblock] == 1) {
	      double p = sblock == 0 ? gfc->nsPsy.last_thm[chn][sb][2] : gfc->thm[chn].s[sb][sblock-1];
	      p = NS_INTERP(p,thmm,NS_PREECHO_ATT2*pcfact);
	      thmm = Min(thmm,p);
	    } else if ((sblock != 0 && ns_attacks[sblock-1] == 3) ||
		       (sblock == 0 && gfc->nsPsy.last_attacks[chn][2] == 3)) {
	      double p = sblock <= 1 ? gfc->nsPsy.last_thm[chn][sb][sblock+1] : gfc->thm[chn].s[sb][0];
	      p = NS_INTERP(p,thmm,NS_PREECHO_ATT2*pcfact);
	      thmm = Min(thmm,p);
	    }

	    gfc->en [chn].s[sb][sblock] = enn;
	    gfc->thm[chn].s[sb][sblock] = thmm;
	  }
      }


    /*************************************************************** 
     * save some values for analysis of the next granule
     ***************************************************************/

    for ( sblock = 0; sblock < 3; sblock++ )
      {
	for ( sb = 0; sb < NBPSY_s; sb++ )
	  {
	    gfc->nsPsy.last_thm[chn][sb][sblock] = gfc->thm[chn].s[sb][sblock];
	  }
      }

    for(i=0;i<3;i++)
      gfc->nsPsy.last_attacks[chn][i] = ns_attacks[i];

  } /* end loop over chn */



  /*************************************************************** 
   * compute M/S thresholds
   ***************************************************************/

  /* from Johnston & Ferreira 1992 ICASSP paper */

  if ( numchn==4 /* mid/side and r/l */) {
    FLOAT8 rside,rmid,mld;
    int chmid=2,chside=3; 
    
    for ( sb = 0; sb < NBPSY_l; sb++ ) {
      /* use this fix if L & R masking differs by 2db or less */
      /* if db = 10*log10(x2/x1) < 2 */
      /* if (x2 < 1.58*x1) { */
      if (gfc->thm[0].l[sb] <= 1.58*gfc->thm[1].l[sb]
	  && gfc->thm[1].l[sb] <= 1.58*gfc->thm[0].l[sb]) {

	mld = gfc->mld_l[sb]*gfc->en[chside].l[sb];
	rmid = Max(gfc->thm[chmid].l[sb], Min(gfc->thm[chside].l[sb],mld));

	mld = gfc->mld_l[sb]*gfc->en[chmid].l[sb];
	rside = Max(gfc->thm[chside].l[sb],Min(gfc->thm[chmid].l[sb],mld));

	gfc->thm[chmid].l[sb]=rmid;
	gfc->thm[chside].l[sb]=rside;
      }
    }
    for ( sb = 0; sb < NBPSY_s; sb++ ) {
      for ( sblock = 0; sblock < 3; sblock++ ) {
	if (gfc->thm[0].s[sb][sblock] <= 1.58*gfc->thm[1].s[sb][sblock]
	    && gfc->thm[1].s[sb][sblock] <= 1.58*gfc->thm[0].s[sb][sblock]) {

	  mld = gfc->mld_s[sb]*gfc->en[chside].s[sb][sblock];
	  rmid = Max(gfc->thm[chmid].s[sb][sblock],Min(gfc->thm[chside].s[sb][sblock],mld));

	  mld = gfc->mld_s[sb]*gfc->en[chmid].s[sb][sblock];
	  rside = Max(gfc->thm[chside].s[sb][sblock],Min(gfc->thm[chmid].s[sb][sblock],mld));

	  gfc->thm[chmid].s[sb][sblock]=rmid;
	  gfc->thm[chside].s[sb][sblock]=rside;
	}
      }
    }
  }


  /* Naoki Shibata 2000 */

#define NS_MSFIX 3.5
  
  if (numchn == 4) {
    FLOAT msfix = NS_MSFIX;
    if (gfc->nsPsy.safejoint) msfix = 1;

    for ( sb = 0; sb < NBPSY_l; sb++ )
      {
	FLOAT8 thmL,thmR,thmM,thmS,ath;
	ath  = (gfc->ATH->cb[(gfc->bu_l[sb] + gfc->bo_l[sb])/2])*pow(10,-gfp->ATHlower/10.0);
	thmL = Max(gfc->thm[0].l[sb],ath);
	thmR = Max(gfc->thm[1].l[sb],ath);
	thmM = Max(gfc->thm[2].l[sb],ath);
	thmS = Max(gfc->thm[3].l[sb],ath);

	if (thmL*msfix < (thmM+thmS)/2) {
	  FLOAT8 f = thmL*msfix / ((thmM+thmS)/2);
	  thmM *= f;
	  thmS *= f;
	}
	if (thmR*msfix < (thmM+thmS)/2) {
	  FLOAT8 f = thmR*msfix / ((thmM+thmS)/2);
	  thmM *= f;
	  thmS *= f;
	}

	gfc->thm[2].l[sb] = Min(thmM,gfc->thm[2].l[sb]);
	gfc->thm[3].l[sb] = Min(thmS,gfc->thm[3].l[sb]);
      }

    for ( sb = 0; sb < NBPSY_s; sb++ ) {
      for ( sblock = 0; sblock < 3; sblock++ ) {
	FLOAT8 thmL,thmR,thmM,thmS,ath;
	ath  = (gfc->ATH->cb[(gfc->bu_s[sb] + gfc->bo_s[sb])/2])*pow(10,-gfp->ATHlower/10.0);
	thmL = Max(gfc->thm[0].s[sb][sblock],ath);
	thmR = Max(gfc->thm[1].s[sb][sblock],ath);
	thmM = Max(gfc->thm[2].s[sb][sblock],ath);
	thmS = Max(gfc->thm[3].s[sb][sblock],ath);

	if (thmL*msfix < (thmM+thmS)/2) {
	  FLOAT8 f = thmL*msfix / ((thmM+thmS)/2);
	  thmM *= f;
	  thmS *= f;
	}
	if (thmR*msfix < (thmM+thmS)/2) {
	  FLOAT8 f = thmR*msfix / ((thmM+thmS)/2);
	  thmM *= f;
	  thmS *= f;
	}

	gfc->thm[2].s[sb][sblock] = Min(gfc->thm[2].s[sb][sblock],thmM);
	gfc->thm[3].s[sb][sblock] = Min(gfc->thm[3].s[sb][sblock],thmS);
      }
    }
  }

  ms_ratio_l = 0;
  ms_ratio_s = 0;


  /*************************************************************** 
   * compute estimation of the amount of bit used in the granule
   ***************************************************************/

  for(chn=0;chn<numchn;chn++)
    {
      {
	static FLOAT8 regcoef[] = {
	  1124.23,10.0583,10.7484,7.29006,16.2714,6.2345,4.09743,3.05468,3.33196,2.54688,
	  3.68168,5.83109,2.93817,-8.03277,-10.8458,8.48777,9.13182,2.05212,8.6674,50.3937,73.267,97.5664,0
	};

	FLOAT8 msum = regcoef[0]/4;
	int sb;

	for ( sb = 0; sb < NBPSY_l; sb++ )
	  {
	    FLOAT8 t;
	      
	    if (gfc->thm[chn].l[sb]*gfc->masking_lower != 0 &&
		gfc->en[chn].l[sb]/(gfc->thm[chn].l[sb]*gfc->masking_lower) > 1)
	      t = log(gfc->en[chn].l[sb]/(gfc->thm[chn].l[sb]*gfc->masking_lower));
	    else
	      t = 0;
	    msum += regcoef[sb+1] * t;
	  }

	gfc->nsPsy.pe_l[chn] = msum;
      }

      {
	static FLOAT8 regcoef[] = {
	  1236.28,0,0,0,0.434542,25.0738,0,0,0,19.5442,19.7486,60,100,0
	};

	FLOAT8 msum = regcoef[0]/4;
	int sb,sblock;

	for(sblock=0;sblock<3;sblock++)
	  {
	    for ( sb = 0; sb < NBPSY_s; sb++ )
	      {
		FLOAT8 t;
	      
		if (gfc->thm[chn].s[sb][sblock] * gfc->masking_lower != 0 &&
		    gfc->en[chn].s[sb][sblock] / (gfc->thm[chn].s[sb][sblock] * gfc->masking_lower) > 1)
		  t = log(gfc->en[chn].s[sb][sblock] / (gfc->thm[chn].s[sb][sblock] * gfc->masking_lower));
		else
		  t = 0;
		msum += regcoef[sb+1] * t;
	      }
	  }

	gfc->nsPsy.pe_s[chn] = msum;
      }

      //gfc->pe[chn] -= 150;
    }

  /*************************************************************** 
   * determine final block type
   ***************************************************************/

  for (chn=0; chn<gfc->channels_out; chn++) {
    blocktype[chn] = NORM_TYPE;
  }

  if (gfc->channels_out==2) {
    if (!gfp->allow_diff_short || gfp->mode==JOINT_STEREO) {
      /* force both channels to use the same block type */
      /* this is necessary if the frame is to be encoded in ms_stereo.  */
      /* But even without ms_stereo, FhG  does this */
      int bothlong= (uselongblock[0] && uselongblock[1]);
      if (!bothlong) {
	uselongblock[0]=0;
	uselongblock[1]=0;
      }
    }
  }

  /* update the blocktype of the previous granule, since it depends on what
   * happend in this granule */
  for (chn=0; chn<gfc->channels_out; chn++) {
    if ( uselongblock[chn])
      {				/* no attack : use long blocks */
	assert( gfc->blocktype_old[chn] != START_TYPE );
	switch( gfc->blocktype_old[chn] ) 
	  {
	  case NORM_TYPE:
	  case STOP_TYPE:
	    blocktype[chn] = NORM_TYPE;
	    break;
	  case SHORT_TYPE:
	    blocktype[chn] = STOP_TYPE; 
	    break;
	  }
      } else   {
	/* attack : use short blocks */
	blocktype[chn] = SHORT_TYPE;
	if ( gfc->blocktype_old[chn] == NORM_TYPE ) {
	  gfc->blocktype_old[chn] = START_TYPE;
	}
	if ( gfc->blocktype_old[chn] == STOP_TYPE ) {
	  gfc->blocktype_old[chn] = SHORT_TYPE ;
	}
      }
    
    blocktype_d[chn] = gfc->blocktype_old[chn];  /* value returned to calling program */
    gfc->blocktype_old[chn] = blocktype[chn];    /* save for next call to l3psy_anal */
  }
  
  /*********************************************************************
   * compute the value of PE to return (one granule delay)
   *********************************************************************/
  for(chn=0;chn<numchn;chn++)
    {
      if (chn < 2) {
	if (blocktype_d[chn] == SHORT_TYPE) {
	  percep_entropy[chn] = pe_s[chn];
	} else {
	  percep_entropy[chn] = pe_l[chn];
	}
	if (gfp->analysis) gfc->pinfo->pe[gr_out][chn] = percep_entropy[chn];
      } else {
	if (blocktype_d[0] == SHORT_TYPE) {
	  percep_MS_entropy[chn-2] = pe_s[chn];
	} else {
	  percep_MS_entropy[chn-2] = pe_l[chn];
	}
	if (gfp->analysis) gfc->pinfo->pe[gr_out][chn] = percep_MS_entropy[chn-2];
      }
    }


  return 0;
}





/* 
 *   The spreading function.  Values returned in units of energy
 */
FLOAT8 s3_func(FLOAT8 bark) {
    
    FLOAT8 tempx,x,tempy,temp;
    tempx = bark;
    if (tempx>=0) tempx *= 3;
    else tempx *=1.5; 
    
    if(tempx>=0.5 && tempx<=2.5)
	{
	    temp = tempx - 0.5;
	    x = 8.0 * (temp*temp - 2.0 * temp);
	}
    else x = 0.0;
    tempx += 0.474;
    tempy = 15.811389 + 7.5*tempx - 17.5*sqrt(1.0+tempx*tempx);
    
    if (tempy <= -60.0) return  0.0;

    tempx = exp( (x + tempy)*LN_TO_LOG10 ); 

    /* Normalization.  The spreading function should be normalized so that:
         +inf
           /
           |  s3 [ bark ]  d(bark)   =  1
           /
         -inf
    */
    tempx /= .6609193;
    return tempx;
    
}








int L3para_read(lame_global_flags * gfp, FLOAT8 sfreq, int *numlines_l,int *numlines_s, 
FLOAT8 *minval,
FLOAT8 s3_l[CBANDS][CBANDS], FLOAT8 s3_s[CBANDS][CBANDS],
FLOAT8 *SNR, 
int *bu_l, int *bo_l, FLOAT8 *w1_l, FLOAT8 *w2_l, 
int *bu_s, int *bo_s, FLOAT8 *w1_s, FLOAT8 *w2_s,
int *npart_l_orig,int *npart_l,int *npart_s_orig,int *npart_s)
{
  lame_internal_flags *gfc=gfp->internal_flags;


  FLOAT8 freq_tp;
  FLOAT8 bval_l[CBANDS], bval_s[CBANDS];
  FLOAT8 bval_l_width[CBANDS], bval_s_width[CBANDS];
  int   cbmax=0, cbmax_tp;
  int  sbmax ;
  int  i,j;
  int freq_scale=1;
  int partition[HBLKSIZE]; 
  int loop, k2;



  /* compute numlines, the number of spectral lines in each partition band */
  /* each partition band should be about DELBARK wide. */
  j=0;
  for(i=0;i<CBANDS;i++)
    {
      FLOAT8 ji, bark1,bark2;
      int k,j2;

      j2 = j;
      j2 = Min(j2,BLKSIZE/2);
      
      do {
	/* find smallest j2 >= j so that  (bark - bark_l[i-1]) < DELBARK */
	ji = j;
	bark1 = freq2bark(sfreq*ji/BLKSIZE);
	
	++j2;
	ji = j2;
	bark2  = freq2bark(sfreq*ji/BLKSIZE);
      } while ((bark2 - bark1) < DELBARK  && j2<=BLKSIZE/2);

      for (k=j; k<j2; ++k)
	partition[k]=i;
      numlines_l[i]=(j2-j);
      j = j2;
      if (j > BLKSIZE/2) break;
    }
  *npart_l_orig = i+1;
  assert(*npart_l_orig <= CBANDS);

  /* compute which partition bands are in which scalefactor bands */
  { int i1,i2,sfb,start,end;
    FLOAT8 freq1,freq2;
    for ( sfb = 0; sfb < SBMAX_l; sfb++ ) {
      start = gfc->scalefac_band.l[ sfb ];
      end   = gfc->scalefac_band.l[ sfb+1 ];
      freq1 = sfreq*(start-.5)/(2*576);
      freq2 = sfreq*(end-1+.5)/(2*576);
		     
      i1 = floor(.5 + BLKSIZE*freq1/sfreq);
      if (i1<0) i1=0;
      i2 = floor(.5 + BLKSIZE*freq2/sfreq);
      if (i2>BLKSIZE/2) i2=BLKSIZE/2;

      //      DEBUGF(gfc,"longblock:  old: (%i,%i)  new: (%i,%i) %i %i \n",bu_l[sfb],bo_l[sfb],partition[i1],partition[i2],i1,i2);

      w1_l[sfb]=.5;
      w2_l[sfb]=.5;
      bu_l[sfb]=partition[i1];
      bo_l[sfb]=partition[i2];

    }
  }


  /* compute bark value and ATH of each critical band */
  j = 0;
  for ( i = 0; i < *npart_l_orig; i++ ) {
      int     k;
      FLOAT8  bark1,bark2;
      /* FLOAT8 mval,freq; */

      // Calculating the medium bark scaled frequency of the spectral lines
      // from j ... j + numlines[i]-1  (=spectral lines in parition band i)

      k         = numlines_l[i] - 1;
      bark1 = freq2bark(sfreq*(j+0)/BLKSIZE);
      bark2 = freq2bark(sfreq*(j+k)/BLKSIZE);
      bval_l[i] = .5*(bark1+bark2);

      bark1 = freq2bark(sfreq*(j+0-.5)/BLKSIZE);
      bark2 = freq2bark(sfreq*(j+k+.5)/BLKSIZE);
      bval_l_width[i] = bark2-bark1;

      gfc->ATH->cb [i] = 1.e37; // preinit for minimum search
      for (k=0; k < numlines_l[i]; k++, j++) {
	FLOAT8  freq = sfreq*j/(1000.0*BLKSIZE);
	FLOAT8  level;
	assert( freq <= 24 );              // or only '<'
	//	freq = Min(.1,freq);       // ATH below 100 Hz constant, not further climbing
	level  = ATHformula (freq*1000, gfp) - 20;   // scale to FFT units; returned value is in dB
	level  = pow ( 10., 0.1*level );   // convert from dB -> energy
	level *= numlines_l [i];
	if ( level < gfc->ATH->cb [i] )
	    gfc->ATH->cb [i] = level;
      }


    }

  /* MINVAL.  For low freq, the strength of the masking is limited by minval
   * this is an ISO MPEG1 thing, dont know if it is really needed */
  for(i=0;i<*npart_l_orig;i++){
    double x = (-20+bval_l[i]*20.0/10.0);
    if (bval_l[i]>10) x = 0;
    minval[i]=pow(10.0,x/10);
  }







  /************************************************************************/
  /* SHORT BLOCKS */
  /************************************************************************/

  /* compute numlines */
  j=0;
  for(i=0;i<CBANDS;i++)
    {
      FLOAT8 ji, bark1,bark2;
      int k,j2;

      j2 = j;
      j2 = Min(j2,BLKSIZE_s/2);
      
      do {
	/* find smallest j2 >= j so that  (bark - bark_s[i-1]) < DELBARK */
	ji = j;
	bark1  = freq2bark(sfreq*ji/BLKSIZE_s);
	
	++j2;
	ji = j2;
	bark2  = freq2bark(sfreq*ji/BLKSIZE_s);

      } while ((bark2 - bark1) < DELBARK  && j2<=BLKSIZE_s/2);

      for (k=j; k<j2; ++k)
	partition[k]=i;
      numlines_s[i]=(j2-j);
      j = j2;
      if (j > BLKSIZE_s/2) break;
    }
  *npart_s_orig = i+1;
  assert(*npart_s_orig <= CBANDS);

  /* compute which partition bands are in which scalefactor bands */
  { int i1,i2,sfb,start,end;
    FLOAT8 freq1,freq2;
    for ( sfb = 0; sfb < SBMAX_s; sfb++ ) {
      start = gfc->scalefac_band.s[ sfb ];
      end   = gfc->scalefac_band.s[ sfb+1 ];
      freq1 = sfreq*(start-.5)/(2*192);
      freq2 = sfreq*(end-1+.5)/(2*192);
		     
      i1 = floor(.5 + BLKSIZE_s*freq1/sfreq);
      if (i1<0) i1=0;
      i2 = floor(.5 + BLKSIZE_s*freq2/sfreq);
      if (i2>BLKSIZE_s/2) i2=BLKSIZE_s/2;

      //DEBUGF(gfc,"shortblock: old: (%i,%i)  new: (%i,%i) %i %i \n",bu_s[sfb],bo_s[sfb], partition[i1],partition[i2],i1,i2);

      w1_s[sfb]=.5;
      w2_s[sfb]=.5;
      bu_s[sfb]=partition[i1];
      bo_s[sfb]=partition[i2];

    }
  }





  /* compute bark values of each critical band */
  j = 0;
  for(i=0;i<*npart_s_orig;i++)
    {
      int     k;
      FLOAT8  bark1,bark2,snr;
      k    = numlines_s[i] - 1;

      bark1 = freq2bark (sfreq*(j+0)/BLKSIZE_s);
      bark2 = freq2bark (sfreq*(j+k)/BLKSIZE_s); 
      bval_s[i] = .5*(bark1+bark2);

      bark1 = freq2bark (sfreq*(j+0-.5)/BLKSIZE_s);
      bark2 = freq2bark (sfreq*(j+k+.5)/BLKSIZE_s); 
      bval_s_width[i] = bark2-bark1;
      j        += k+1;
      
      /* SNR formula */
      if (bval_s[i]<13)
          snr=-8.25;
      else 
	  snr  = -4.5 * (bval_s[i]-13)/(24.0-13.0)  + 
	      -8.25*(bval_s[i]-24)/(13.0-24.0);

      SNR[i]=pow(10.0,snr/10.0);
    }






  /************************************************************************
   * Now compute the spreading function, s[j][i], the value of the spread-*
   * ing function, centered at band j, for band i, store for later use    *
   ************************************************************************/
  /* i.e.: sum over j to spread into signal barkval=i  
     NOTE: i and j are used opposite as in the ISO docs */
  for(i=0;i<*npart_l_orig;i++)    {
      for(j=0;j<*npart_l_orig;j++) 	{
  	  s3_l[i][j]=s3_func(bval_l[i]-bval_l[j])*bval_l_width[j];
      }
  }
  for(i=0;i<*npart_s_orig;i++)     {
      for(j=0;j<*npart_s_orig;j++) 	{
  	  s3_s[i][j]=s3_func(bval_s[i]-bval_s[j])*bval_s_width[j];
      }
  }
  



  /* compute: */
  /* npart_l_orig   = number of partition bands before convolution */
  /* npart_l  = number of partition bands after convolution */
  
  *npart_l=bo_l[NBPSY_l-1]+1;
  *npart_s=bo_s[NBPSY_s-1]+1;
  
  assert(*npart_l <= *npart_l_orig);
  assert(*npart_s <= *npart_s_orig);


    /* setup stereo demasking thresholds */
    /* formula reverse enginerred from plot in paper */
    for ( i = 0; i < NBPSY_s; i++ ) {
      FLOAT8 arg,mld;
      arg = freq2bark(sfreq*gfc->scalefac_band.s[i]/(2*192));
      arg = (Min(arg, 15.5)/15.5);

      mld = 1.25*(1-cos(PI*arg))-2.5;
      gfc->mld_s[i] = pow(10.0,mld);
    }
    for ( i = 0; i < NBPSY_l; i++ ) {
      FLOAT8 arg,mld;
      arg = freq2bark(sfreq*gfc->scalefac_band.l[i]/(2*576));
      arg = (Min(arg, 15.5)/15.5);

      mld = 1.25*(1-cos(PI*arg))-2.5;
      gfc->mld_l[i] = pow(10.0,mld);
    }

#define temporalmask_sustain_sec 0.01

    /* setup temporal masking */
    gfc->decay = exp(-1.0*LOG10/(temporalmask_sustain_sec*sfreq/192.0));

    return 0;
}



















int psymodel_init(lame_global_flags *gfp)
{
    lame_internal_flags *gfc=gfp->internal_flags;
    int i,j,b,sb,k,samplerate;
    FLOAT cwlimit;

    samplerate = gfp->out_samplerate;
    gfc->ms_ener_ratio_old=.25;
    gfc->blocktype_old[0]=STOP_TYPE;
    gfc->blocktype_old[1]=STOP_TYPE;
    gfc->blocktype_old[0]=SHORT_TYPE;
    gfc->blocktype_old[1]=SHORT_TYPE;

    for (i=0; i<4; ++i) {
      for (j=0; j<CBANDS; ++j) {
	gfc->nb_1[i][j]=1e20;
	gfc->nb_2[i][j]=1e20;
      }
      for ( sb = 0; sb < NBPSY_l; sb++ ) {
	gfc->en[i].l[sb] = 1e20;
	gfc->thm[i].l[sb] = 1e20;
      }
      for (j=0; j<3; ++j) {
	for ( sb = 0; sb < NBPSY_s; sb++ ) {
	  gfc->en[i].s[sb][j] = 1e20;
	  gfc->thm[i].s[sb][j] = 1e20;
	}
      }
    }
    for (i=0; i<4; ++i) {
      for (j=0; j<3; ++j) {
	for ( sb = 0; sb < NBPSY_s; sb++ ) {
	  gfc->nsPsy.last_thm[i][sb][j] = 1e20;
	}
      }
    }
    for(i=0;i<4;i++) {
      for(j=0;j<9;j++)
	gfc->nsPsy.last_en_subshort[i][j] = 100;
      for(j=0;j<3;j++)
	gfc->nsPsy.last_attacks[i][j] = 0;
      gfc->nsPsy.pe_l[i] = gfc->nsPsy.pe_s[i] = 0;
    }



    
    /*  gfp->cwlimit = sfreq*j/1024.0;  */
    gfc->cw_lower_index=6;
    if (gfp->cwlimit>0) 
      cwlimit=gfp->cwlimit;
    else
      cwlimit=(FLOAT)8871.7;
    gfc->cw_upper_index = cwlimit*1024.0/gfp->out_samplerate;
    gfc->cw_upper_index=Min(HBLKSIZE-4,gfc->cw_upper_index);      /* j+3 < HBLKSIZE-1 */
    gfc->cw_upper_index=Max(6,gfc->cw_upper_index);

    for ( j = 0; j < HBLKSIZE; j++ )
      gfc->cw[j] = 0.4f;
    
    

    i=L3para_read( gfp,(FLOAT8) samplerate,gfc->numlines_l,gfc->numlines_s,
          gfc->minval,gfc->s3_l,gfc->s3_s,gfc->SNR_s,gfc->bu_l,
          gfc->bo_l,gfc->w1_l,gfc->w2_l, gfc->bu_s,gfc->bo_s,
          gfc->w1_s,gfc->w2_s,&gfc->npart_l_orig,&gfc->npart_l,
          &gfc->npart_s_orig,&gfc->npart_s );
    if (i!=0) return -1;
    


    /* npart_l_orig   = number of partition bands before convolution */
    /* npart_l  = number of partition bands after convolution */
    
    for (i=0; i<gfc->npart_l; i++) {
      for (j = 0; j < gfc->npart_l_orig; j++) {
	if (gfc->s3_l[i][j] != 0.0)
	  break;
      }
      gfc->s3ind[i][0] = j;
      
      for (j = gfc->npart_l_orig - 1; j > 0; j--) {
	if (gfc->s3_l[i][j] != 0.0)
	  break;
      }
      gfc->s3ind[i][1] = j;
    }


    for (i=0; i<gfc->npart_s; i++) {
      for (j = 0; j < gfc->npart_s_orig; j++) {
	if (gfc->s3_s[i][j] != 0.0)
	  break;
      }
      gfc->s3ind_s[i][0] = j;
      
      for (j = gfc->npart_s_orig - 1; j > 0; j--) {
	if (gfc->s3_s[i][j] != 0.0)
	  break;
      }
      gfc->s3ind_s[i][1] = j;
    }
    
    
    /*  
      #include "debugscalefac.c"
    */
    


    if (gfc->nsPsy.use) {
	/* long block spreading function normalization */
	for ( b = 0;b < gfc->npart_l; b++ ) {
	    for ( k = gfc->s3ind[b][0]; k <= gfc->s3ind[b][1]; k++ ) {
		// spreading function has been properly normalized by
		// multiplying by DELBARK/.6609193 = .515.  
		// It looks like Naoki was
                // way ahead of me and added this factor here!
		// it is no longer needed.
		//gfc->s3_l[b][k] *= 0.5;
	    }
	}
	/* short block spreading function normalization */
	// no longer needs to be normalized, but nspsytune wants 
	// SNR_s applied here istead of later to save CPU cycles
	for ( b = 0;b < gfc->npart_s; b++ ) {
	    FLOAT8 norm=0;
	    for ( k = gfc->s3ind_s[b][0]; k <= gfc->s3ind_s[b][1]; k++ ) {
		norm += gfc->s3_s[b][k];
	    }
	    for ( k = gfc->s3ind_s[b][0]; k <= gfc->s3ind_s[b][1]; k++ ) {
		gfc->s3_s[b][k] *= gfc->SNR_s[b] /* / norm */;
	    }
	}
    }



    if (gfc->nsPsy.use) {
#if 1
	/* spread only from npart_l bands.  Normally, we use the spreading
	 * function to convolve from npart_l_orig down to npart_l bands 
	 */
	for(b=0;b<gfc->npart_l;b++)
	    if (gfc->s3ind[b][1] > gfc->npart_l-1) gfc->s3ind[b][1] = gfc->npart_l-1;
#endif
    }

    return 0;
}







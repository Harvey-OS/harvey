/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 *      encoder.h include file
 *
 *      Copyright (c) 2000 Mark Taylor
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


#ifndef LAME_ENCODER_H
#define LAME_ENCODER_H

/***********************************************************************
*
*  encoder and decoder delays
*
***********************************************************************/

/*
 * layer III enc->dec delay:  1056 (1057?)   (observed)
 * layer  II enc->dec delay:   480  (481?)   (observed)
 *
 * polyphase 256-16             (dec or enc)        = 240
 * mdct      256+32  (9*32)     (dec or enc)        = 288
 * total:    512+16
 *
 * My guess is that delay of polyphase filterbank is actualy 240.5
 * (there are technical reasons for this, see postings in mp3encoder).
 * So total Encode+Decode delay = ENCDELAY + 528 + 1
 */

/*
 * ENCDELAY  The encoder delay.
 *
 * Minimum allowed is MDCTDELAY (see below)
 *
 * The first 96 samples will be attenuated, so using a value less than 96
 * will result in corrupt data for the first 96-ENCDELAY samples.
 *
 * suggested: 576
 * set to 1160 to sync with FhG.
 */

#define ENCDELAY      576

/*
 * delay of the MDCT used in mdct.c
 * original ISO routines had a delay of 528!
 * Takehiro's routines:
 */

#define MDCTDELAY     48
#define FFTOFFSET     (224+MDCTDELAY)

/*
 * Most decoders, including the one we use, have a delay of 528 samples.
 */

#define DECDELAY      528


/* number of subbands */
#define SBLIMIT       32

/* parition bands bands */
#define CBANDS        64

/* number of critical bands/scale factor bands where masking is computed*/
#define SBPSY_l       21
#define SBPSY_s       12

/* total number of scalefactor bands encoded */
#define SBMAX_l       22
#define SBMAX_s       13



/* FFT sizes */
#define BLKSIZE       1024
#define HBLKSIZE      (BLKSIZE/2 + 1)
#define BLKSIZE_s     256
#define HBLKSIZE_s    (BLKSIZE_s/2 + 1)


/* #define switch_pe        1800 */
#define NORM_TYPE     0
#define START_TYPE    1
#define SHORT_TYPE    2
#define STOP_TYPE     3

/*
 * Mode Extention:
 * When we are in stereo mode, there are 4 possible methods to store these
 * two channels. The stereo modes -m? are using a subset of them.
 *
 *  -ms: MPG_MD_LR_LR
 *  -mj: MPG_MD_LR_LR and MPG_MD_MS_LR
 *  -mf: MPG_MD_MS_LR
 *  -mi: all
 */

#define MPG_MD_LR_LR  0
#define MPG_MD_LR_I   1
#define MPG_MD_MS_LR  2
#define MPG_MD_MS_I   3


#include "machine.h"
#include "lame.h"

int  lame_encode_mp3_frame (
        lame_global_flags*  const gfp,
        sample_t*           inbuf_l,
        sample_t*           inbuf_r,
        unsigned char*      mp3buf,
	int                 mp3buf_size );

#endif /* LAME_ENCODER_H */

/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 *	GTK plotting routines source file
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

#ifndef LAME_GTKANAL_H
#define LAME_GTKANAL_H

#include "encoder.h"

#define READ_AHEAD 10             /* number of frames to read ahead */
#define MAXMPGLAG READ_AHEAD      /* if the mpg123 lag becomes bigger than this 				     we have to stop */
#define NUMBACK 6                 /* number of frames we can back up */
#define NUMPINFO (NUMBACK+READ_AHEAD+1)



typedef struct {
  int frameNum;           /* current frame number */
  int frameNum123;
  int num_samples;        /* number of pcm samples read for this frame */
  double frametime;       /* starting time of frame, in seconds */
  double pcmdata[2][1600];
  double pcmdata2[2][1152+1152-DECDELAY];
  double xr[2][2][576];
  double mpg123xr[2][2][576];
  double ms_ratio[2];
  double ms_ener_ratio[2];

  /* L,R, M and S values */
  double energy[2][4][BLKSIZE];
  double pe[2][4];
  double thr[2][4][SBMAX_l];
  double en[2][4][SBMAX_l];
  double thr_s[2][4][3*SBMAX_s];
  double en_s[2][4][3*SBMAX_s];
  double ers[2][4];

  double sfb[2][2][SBMAX_l];
  double sfb_s[2][2][3*SBMAX_s];
  double LAMEsfb[2][2][SBMAX_l];
  double LAMEsfb_s[2][2][3*SBMAX_s];

  int LAMEqss[2][2];
  int qss[2][2];
  int big_values[2][2];
  int sub_gain[2][2][3];

  double xfsf[2][2][SBMAX_l];
  double xfsf_s[2][2][3*SBMAX_s];

  int over[2][2];
  double tot_noise[2][2];
  double max_noise[2][2];
  double over_noise[2][2];
  int blocktype[2][2];
  int scalefac_scale[2][2];
  int preflag[2][2];
  int mpg123blocktype[2][2];
  int mixed[2][2];
  int mainbits[2][2];
  int sfbits[2][2];
  int LAMEmainbits[2][2];
  int LAMEsfbits[2][2];
  int framesize,stereo,js,ms_stereo,i_stereo,emph,bitrate,sampfreq,maindata;
  int crc,padding;
  int scfsi[2],mean_bits,resvsize;
  int totbits;
} plotting_data;


extern plotting_data *pinfo;

#endif

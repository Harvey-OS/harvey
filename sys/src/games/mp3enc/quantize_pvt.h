/*
 *	quantize_pvt include file
 *
 *	Copyright (c) 1999 Takehiro TOMINAGA
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

#ifndef LAME_QUANTIZE_PVT_H
#define LAME_QUANTIZE_PVT_H

#include "l3side.h"
#define IXMAX_VAL 8206  /* ix always <= 8191+15.    see count_bits() */

/* buggy Winamp decoder cannot handle values > 8191 */
/* #define IXMAX_VAL 8191 */

#define PRECALC_SIZE (IXMAX_VAL+2)


extern const int nr_of_sfb_block[6][3][4];
extern const char pretab[SBMAX_l];
extern const char slen1_tab[16];
extern const char slen2_tab[16];

extern const scalefac_struct sfBandIndex[9];

extern FLOAT8 pow43[PRECALC_SIZE];
extern FLOAT8 adj43[PRECALC_SIZE];
extern FLOAT8 adj43asm[PRECALC_SIZE];

#define Q_MAX 330

extern FLOAT8 pow20[Q_MAX];
extern FLOAT8 ipow20[Q_MAX];

typedef struct calc_noise_result_t {
    int     over_count;      /* number of quantization noise > masking */
    int     tot_count;       /* all */
    FLOAT8  over_noise;      /* sum of quantization noise > masking */
    FLOAT8  tot_noise;       /* sum of all quantization noise */
    FLOAT8  max_noise;       /* max quantization noise */
    float   klemm_noise;
    FLOAT8  dist_l[SBMAX_l];
    FLOAT8  dist_s[3][SBMAX_s];
} calc_noise_result;

void    compute_ath (lame_global_flags * gfp, FLOAT8 ATH_l[SBPSY_l],
                     FLOAT8 ATH_s[SBPSY_l]);

void    ms_convert (FLOAT8 xr[2][576], FLOAT8 xr_org[2][576]);

int     on_pe (lame_global_flags *gfp, FLOAT8 pe[2][2], III_side_info_t * l3_side,
               int targ_bits[2], int mean_bits, int gr);

void    reduce_side (int targ_bits[2], FLOAT8 ms_ener_ratio, int mean_bits,
                     int max_bits);


int     bin_search_StepSize (lame_internal_flags * gfc, gr_info * cod_info,
                             int desired_rate, int start,
                             const FLOAT8 xrpow[576], int l3enc[576]);

int     inner_loop (lame_internal_flags * gfc, gr_info * cod_info, int max_bits,
                    const FLOAT8 xrpow[576], int l3enc[576]);

void    iteration_init (lame_global_flags *gfp);


int     calc_xmin (lame_global_flags *gfp, const FLOAT8 xr[576],
                   const III_psy_ratio * ratio, const gr_info * cod_info,
                   III_psy_xmin * l3_xmin);

int     calc_noise (const lame_internal_flags * gfc, const FLOAT8 xr[576],
                    const int ix[576], const gr_info * cod_info,
                    const III_psy_xmin * l3_xmin,
                    const III_scalefac_t * scalefac,
                    III_psy_xmin * distort, calc_noise_result * res);

void    set_frame_pinfo (lame_global_flags *gfp, FLOAT8 xr[2][2][576],
                         III_psy_ratio ratio[2][2], int l3_enc[2][2][576],
                         III_scalefac_t scalefac[2][2]);


void    quantize_xrpow (const FLOAT8 xr[576], int ix[576], FLOAT8 istep);

void    quantize_xrpow_ISO (const FLOAT8 xr[576], int ix[576], FLOAT8 istep);



/* takehiro.c */

int     count_bits (lame_internal_flags * gfc, int *ix, const FLOAT8 xr[576],
                    gr_info * cod_info);


void    best_huffman_divide (const lame_internal_flags * gfc, int gr, int ch,
                             gr_info * cod_info, int *ix);

void    best_scalefac_store (const lame_internal_flags * gfc, int gr, int ch,
                             int l3_enc[2][2][576], III_side_info_t * l3_side,
                             III_scalefac_t scalefac[2][2]);

int     scale_bitcount (III_scalefac_t * scalefac, gr_info * cod_info);

int     scale_bitcount_lsf (const lame_internal_flags *gfp, const III_scalefac_t * scalefac,
                            gr_info * cod_info);

void    huffman_init (lame_internal_flags * gfc);

#define LARGE_BITS 100000

#endif /* LAME_QUANTIZE_PVT_H */

/*
 * Copyright (C) 2002 artofcode LLC.  All rights reserved.
 * See toolbin/encs2c.ps for the complete license notice.
 *
 * $Id: gscedata.h,v 1.4 2004/10/04 17:28:33 igor Exp $
 *
 * This file contains substantial parts of toolbin/encs2c.ps,
 * which generated the remainder of the file mechanically from
 *   gs_std_e.ps  gs_il1_e.ps  gs_sym_e.ps  gs_dbt_e.ps
 *   gs_wan_e.ps  gs_mro_e.ps  gs_mex_e.ps  gs_mgl_e.ps
 *   gs_lgo_e.ps  gs_lgx_e.ps  gs_css_e.ps
 */

#ifndef gscedata_INCLUDED
#  define gscedata_INCLUDED

#define NUM_LEN_BITS 5

#define N(len,offset) (((offset) << NUM_LEN_BITS) + (len))
#define N_LEN(e) ((e) & ((1 << NUM_LEN_BITS) - 1))
#define N_OFFSET(e) ((e) >> NUM_LEN_BITS)

extern const char gs_c_known_encoding_chars[];
extern const int gs_c_known_encoding_total_chars;
extern const int gs_c_known_encoding_max_length;
extern const ushort gs_c_known_encoding_offsets[];
extern const int gs_c_known_encoding_count;
extern const ushort *const gs_c_known_encodings[];
extern const ushort *const gs_c_known_encodings_reverse[];
extern const ushort gs_c_known_encoding_lengths[];
extern const ushort gs_c_known_encoding_reverse_lengths[];

#endif /* gscedata_INCLUDED */

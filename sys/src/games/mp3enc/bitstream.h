/*
 *	MP3 bitstream Output interface for LAME
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

#ifndef LAME_BITSTREAM_H
#define LAME_BITSTREAM_H
#include "util.h"

int format_bitstream(lame_global_flags *gfp, int i,
		     int              l3_enc[2][2][576],
		     III_scalefac_t   scalefac[2][2] );

void flush_bitstream(lame_global_flags *gfp);
void add_dummy_byte ( lame_global_flags* const gfp, unsigned char val );

int  copy_buffer(unsigned char *buffer,int buffer_size,Bit_stream_struc *bs);
void init_bit_stream_w(lame_internal_flags *gfc);
void main_CRC_init (void);   // remove sometimes

#endif

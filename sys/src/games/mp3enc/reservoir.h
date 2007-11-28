/*
 *	bit reservoir include file
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

#ifndef LAME_RESERVOIR_H
#define LAME_RESERVOIR_H

int ResvFrameBegin(lame_global_flags *gfp,III_side_info_t *l3_side, int mean_bits, int frameLength );
void ResvMaxBits(lame_global_flags *gfp, int mean_bits, int *targ_bits, int *max_bits);
void ResvAdjust(lame_internal_flags *gfc,gr_info *gi, III_side_info_t *l3_side, int mean_bits );
void ResvFrameEnd(lame_internal_flags *gfc,III_side_info_t *l3_side, int mean_bits );

#endif /* LAME_RESERVOIR_H */

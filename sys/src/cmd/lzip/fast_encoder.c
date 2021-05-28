/*  Clzip - LZMA lossless data compressor
    Copyright (C) 2010-2017 Antonio Diaz Diaz.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include "lzip.h"
#include "encoder_base.h"
#include "fast_encoder.h"

int
FLZe_longest_match_len(FLZ_encoder *fe, int *distance)
{
	enum { len_limit = 16 	};
	uchar *data = Mb_ptr_to_current_pos(&fe->eb.mb);
	int32_t * ptr0 = fe->eb.mb.pos_array + fe->eb.mb.cyclic_pos;
	int pos1 = fe->eb.mb.pos + 1;
	int maxlen = 0, newpos1, count;
	int available = min(Mb_avail_bytes(&fe->eb.mb), max_match_len);

	if (available < len_limit)
		return 0;

	fe->key4 = ((fe->key4 << 4) ^ data[3]) & fe->eb.mb.key4_mask;
	newpos1 = fe->eb.mb.prev_positions[fe->key4];
	fe->eb.mb.prev_positions[fe->key4] = pos1;

	for (count = 4; ;) {
		int32_t * newptr;
		int delta;

		if (newpos1 <= 0 || --count < 0 ||
		    (delta = pos1 - newpos1) > fe->eb.mb.dict_size) {
			*ptr0 = 0;
			break;
		}
		newptr = fe->eb.mb.pos_array +
		    (fe->eb.mb.cyclic_pos - delta +
		    ((fe->eb.mb.cyclic_pos >= delta) ? 0 : fe->eb.mb.dict_size + 1));

		if (data[maxlen-delta] == data[maxlen]) {
			int	len = 0;
			while (len < available && data[len-delta] == data[len])
				++len;
			if (maxlen < len) {
				maxlen = len;
				*distance = delta - 1;
				if (maxlen >= len_limit) {
					*ptr0 = *newptr;
					break;
				}
			}
		}

		*ptr0 = newpos1;
		ptr0 = newptr;
		newpos1 = *ptr0;
	}
	return maxlen;
}

bool
FLZe_encode_member(FLZ_encoder *fe, uvlong member_size)
{
	uvlong member_size_limit = member_size - Ft_size - max_marker_size;
	int rep = 0, i;
	int reps[num_rep_distances];
	State state = 0;

	for (i = 0; i < num_rep_distances; ++i)
		reps[i] = 0;

	if (Mb_data_position(&fe->eb.mb) != 0 ||
	    Re_member_position(&fe->eb.renc) != Fh_size)
		return false;			/* can be called only once */

	if (!Mb_data_finished(&fe->eb.mb))	/* encode first byte */ {
		uchar prev_byte = 0;
		uchar cur_byte = Mb_peek(&fe->eb.mb, 0);
		Re_encode_bit(&fe->eb.renc, &fe->eb.bm_match[state][0], 0);
		LZeb_encode_literal(&fe->eb, prev_byte, cur_byte);
		CRC32_update_byte(&fe->eb.crc, cur_byte);
		FLZe_reset_key4(fe);
		FLZe_update_and_move(fe, 1);
	}

	while (!Mb_data_finished(&fe->eb.mb) &&
	    Re_member_position(&fe->eb.renc) < member_size_limit) {
		int	match_distance;
		int main_len = FLZe_longest_match_len(fe, &match_distance);
		int pos_state = Mb_data_position(&fe->eb.mb) & pos_state_mask;
		int	len = 0;

		for (i = 0; i < num_rep_distances; ++i) {
			int tlen = Mb_true_match_len(&fe->eb.mb, 0, reps[i] + 1);
			if (tlen > len) {
				len = tlen;
				rep = i;
			}
		}
		if (len > min_match_len && len + 3 > main_len) {
			CRC32_update_buf(&fe->eb.crc, Mb_ptr_to_current_pos(&fe->eb.mb), len);
			Re_encode_bit(&fe->eb.renc, &fe->eb.bm_match[state][pos_state], 1);
			Re_encode_bit(&fe->eb.renc, &fe->eb.bm_rep[state], 1);
			Re_encode_bit(&fe->eb.renc, &fe->eb.bm_rep0[state], rep != 0);
			if (rep == 0)
				Re_encode_bit(&fe->eb.renc, &fe->eb.bm_len[state][pos_state], 1);
			else {
				int	distance;
				Re_encode_bit(&fe->eb.renc, &fe->eb.bm_rep1[state], rep > 1);
				if (rep > 1)
					Re_encode_bit(&fe->eb.renc, &fe->eb.bm_rep2[state], rep > 2);
				distance = reps[rep];
				for (i = rep; i > 0; --i)
					reps[i] = reps[i-1];
				reps[0] = distance;
			}
			state = St_set_rep(state);
			Re_encode_len(&fe->eb.renc, &fe->eb.rep_len_model, len, pos_state);
			Mb_move_pos(&fe->eb.mb);
			FLZe_update_and_move(fe, len - 1);
			continue;
		}

		if (main_len > min_match_len) {
			CRC32_update_buf(&fe->eb.crc, Mb_ptr_to_current_pos(&fe->eb.mb), main_len);
			Re_encode_bit(&fe->eb.renc, &fe->eb.bm_match[state][pos_state], 1);
			Re_encode_bit(&fe->eb.renc, &fe->eb.bm_rep[state], 0);
			state = St_set_match(state);
			for (i = num_rep_distances - 1; i > 0; --i)
				reps[i] = reps[i-1];
			reps[0] = match_distance;
			LZeb_encode_pair(&fe->eb, match_distance, main_len, pos_state);
			Mb_move_pos(&fe->eb.mb);
			FLZe_update_and_move(fe, main_len - 1);
			continue;
		}

		{
			uchar prev_byte = Mb_peek(&fe->eb.mb, 1);
			uchar cur_byte = Mb_peek(&fe->eb.mb, 0);
			uchar match_byte = Mb_peek(&fe->eb.mb, reps[0] + 1);
			Mb_move_pos(&fe->eb.mb);
			CRC32_update_byte(&fe->eb.crc, cur_byte);

			if (match_byte == cur_byte) {
				int short_rep_price = price1(fe->eb.bm_match[state][pos_state]) +
				    price1(fe->eb.bm_rep[state]) +
				    price0(fe->eb.bm_rep0[state]) +
				    price0(fe->eb.bm_len[state][pos_state]);
				int	price = price0(fe->eb.bm_match[state][pos_state]);
				if (St_is_char(state))
					price += LZeb_price_literal(&fe->eb, prev_byte, cur_byte);
				else
					price += LZeb_price_matched(&fe->eb, prev_byte, cur_byte, match_byte);
				if (short_rep_price < price) {
					Re_encode_bit(&fe->eb.renc, &fe->eb.bm_match[state][pos_state], 1);
					Re_encode_bit(&fe->eb.renc, &fe->eb.bm_rep[state], 1);
					Re_encode_bit(&fe->eb.renc, &fe->eb.bm_rep0[state], 0);
					Re_encode_bit(&fe->eb.renc, &fe->eb.bm_len[state][pos_state], 0);
					state = St_set_short_rep(state);
					continue;
				}
			}

			/* literal byte */
			Re_encode_bit(&fe->eb.renc, &fe->eb.bm_match[state][pos_state], 0);
			if (St_is_char(state))
				LZeb_encode_literal(&fe->eb, prev_byte, cur_byte);
			else
				LZeb_encode_matched(&fe->eb, prev_byte, cur_byte, match_byte);
			state = St_set_char(state);
		}
	}

	LZeb_full_flush(&fe->eb, state);
	return true;
}

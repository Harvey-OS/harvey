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

Dis_slots dis_slots;
Prob_prices prob_prices;

bool
Mb_read_block(Matchfinder_base *mb)
{
	if (!mb->at_stream_end && mb->stream_pos < mb->buffer_size) {
		int size = mb->buffer_size - mb->stream_pos;
		int rd = readblock(mb->infd, mb->buffer + mb->stream_pos, size);

		mb->stream_pos += rd;
		if (rd != size && errno) {
			show_error( "Read error", errno, false );
			cleanup_and_fail(1);
		}
		if (rd < size) {
			mb->at_stream_end = true;
			mb->pos_limit = mb->buffer_size;
		}
	}
	return mb->pos < mb->stream_pos;
}

void
Mb_normalize_pos(Matchfinder_base *mb)
{
	if (mb->pos > mb->stream_pos)
		internal_error( "pos > stream_pos in Mb_normalize_pos." );
	if (!mb->at_stream_end) {
		int i, offset = mb->pos - mb->before_size - mb->dict_size;
		int size = mb->stream_pos - offset;

		memmove(mb->buffer, mb->buffer + offset, size);
		mb->partial_data_pos += offset;
		mb->pos -= offset;	/* pos = before_size + dict_size */
		mb->stream_pos -= offset;
		for (i = 0; i < mb->num_prev_positions; ++i)
			if (mb->prev_positions[i] < offset)
				mb->prev_positions[i] = 0;
			else
				mb->prev_positions[i] -= offset;
		for (i = 0; i < mb->pos_array_size; ++i)
			if (mb->pos_array[i] < offset)
				mb->pos_array[i] = 0;
			else
				mb->pos_array[i] -= offset;
		Mb_read_block(mb);
	}
}

bool
Mb_init(Matchfinder_base *mb, int before, int dict_size, int after_size, int dict_factor, int num_prev_positions23, int pos_array_factor, int ifd)
{
	int buffer_size_limit = (dict_factor * dict_size) + before + after_size;
	unsigned size;
	int i;

	mb->partial_data_pos = 0;
	mb->before_size = before;
	mb->pos = 0;
	mb->cyclic_pos = 0;
	mb->stream_pos = 0;
	mb->infd = ifd;
	mb->at_stream_end = false;

	mb->buffer_size = max(65536, dict_size);
	mb->buffer = (uchar *)malloc(mb->buffer_size);
	if (!mb->buffer)
		return false;
	if (Mb_read_block(mb) && !mb->at_stream_end &&
	    mb->buffer_size < buffer_size_limit) {
		uchar * tmp;
		mb->buffer_size = buffer_size_limit;
		tmp = (uchar *)realloc(mb->buffer, mb->buffer_size);
		if (!tmp) {
			free(mb->buffer);
			return false;
		}
		mb->buffer = tmp;
		Mb_read_block(mb);
	}
	if (mb->at_stream_end && mb->stream_pos < dict_size)
		mb->dict_size = max(min_dict_size, mb->stream_pos);
	else
		mb->dict_size = dict_size;
	mb->pos_limit = mb->buffer_size;
	if (!mb->at_stream_end)
		mb->pos_limit -= after_size;
	size = real_bits(mb->dict_size - 1) - 2;
	if (size < 16)
		size = 16;
	size = 1 << size;
//	if (mb->dict_size > (1 << 26))		/* 64 MiB */
//		size >>= 1;
	mb->key4_mask = size - 1;
	size += num_prev_positions23;

	mb->num_prev_positions = size;
	mb->pos_array_size = pos_array_factor * (mb->dict_size + 1);
	size += mb->pos_array_size;
	if (size * sizeof mb->prev_positions[0] <= size)
		mb->prev_positions = 0;
	else
		mb->prev_positions =
		    (int32_t *)malloc(size * sizeof mb->prev_positions[0]);
	if (!mb->prev_positions) {
		free(mb->buffer);
		return false;
	}
	mb->pos_array = mb->prev_positions + mb->num_prev_positions;
	for (i = 0; i < mb->num_prev_positions; ++i)
		mb->prev_positions[i] = 0;
	return true;
}

void
Mb_reset(Matchfinder_base *mb)
{
	int	i;

	if (mb->stream_pos > mb->pos)
		memmove(mb->buffer, mb->buffer + mb->pos, mb->stream_pos - mb->pos);
	mb->partial_data_pos = 0;
	mb->stream_pos -= mb->pos;
	mb->pos = 0;
	mb->cyclic_pos = 0;
	for (i = 0; i < mb->num_prev_positions; ++i)
		mb->prev_positions[i] = 0;
	Mb_read_block(mb);
}

void
Re_flush_data(Range_encoder *renc)
{
	if (renc->pos > 0) {
		if (renc->outfd >= 0 &&
		    writeblock(renc->outfd, renc->buffer, renc->pos) != renc->pos) {
			show_error( "Write error", errno, false );
			cleanup_and_fail(1);
		}
		renc->partial_member_pos += renc->pos;
		renc->pos = 0;
		show_progress(0, 0, 0, 0);
	}
}

/* End Of Stream mark => (dis == 0xFFFFFFFFU, len == min_match_len) */
void
LZeb_full_flush(LZ_encoder_base *eb, State state)
{
	int	i;
	int pos_state = Mb_data_position(&eb->mb) & pos_state_mask;
	File_trailer trailer;
	Re_encode_bit(&eb->renc, &eb->bm_match[state][pos_state], 1);
	Re_encode_bit(&eb->renc, &eb->bm_rep[state], 0);
	LZeb_encode_pair(eb, 0xFFFFFFFFU, min_match_len, pos_state);
	Re_flush(&eb->renc);
	Ft_set_data_crc(trailer, LZeb_crc(eb));
	Ft_set_data_size(trailer, Mb_data_position(&eb->mb));
	Ft_set_member_size(trailer, Re_member_position(&eb->renc) + Ft_size);
	for (i = 0; i < Ft_size; ++i)
		Re_put_byte(&eb->renc, trailer[i]);
	Re_flush_data(&eb->renc);
}

void
LZeb_reset(LZ_encoder_base *eb)
{
	Mb_reset(&eb->mb);
	eb->crc = 0xFFFFFFFFU;
	Bm_array_init(eb->bm_literal[0], (1 << literal_context_bits) * 0x300);
	Bm_array_init(eb->bm_match[0], states * pos_states);
	Bm_array_init(eb->bm_rep, states);
	Bm_array_init(eb->bm_rep0, states);
	Bm_array_init(eb->bm_rep1, states);
	Bm_array_init(eb->bm_rep2, states);
	Bm_array_init(eb->bm_len[0], states * pos_states);
	Bm_array_init(eb->bm_dis_slot[0], len_states * (1 << dis_slot_bits));
	Bm_array_init(eb->bm_dis, modeled_distances - end_dis_model + 1);
	Bm_array_init(eb->bm_align, dis_align_size);
	Lm_init(&eb->match_len_model);
	Lm_init(&eb->rep_len_model);
	Re_reset(&eb->renc);
}

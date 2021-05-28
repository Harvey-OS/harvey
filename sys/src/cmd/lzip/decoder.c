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
#include "decoder.h"

void
Pp_show_msg(Pretty_print *pp, char *msg)
{
	if (verbosity >= 0) {
		if (pp->first_post) {
			unsigned i;

			pp->first_post = false;
			fprintf(stderr, "%s: ", pp->name);
			for (i = strlen(pp->name); i < pp->longest_name; ++i)
				fputc(' ', stderr);
			if (!msg)
				fflush(stderr);
		}
		if (msg)
			fprintf(stderr, "%s\n", msg);
	}
}

/* Returns the number of bytes really read.
   If returned value < size and no read error, means EOF was reached.
 */
int
readblock(int fd, uchar *buf, int size)
{
	int n, sz;

	for (sz = 0; sz < size; sz += n) {
		n = read(fd, buf + sz, size - sz);
		if (n <= 0)
			break;
	}
	return sz;
}

/* Returns the number of bytes really written.
   If (returned value < size), it is always an error.
 */
int
writeblock(int fd, uchar *buf, int size)
{
	int n, sz;

	for (sz = 0; sz < size; sz += n) {
		n = write(fd, buf + sz, size - sz);
		if (n != size - sz)
			break;
	}
	return sz;
}

bool
Rd_read_block(Range_decoder *rdec)
{
	if (!rdec->at_stream_end) {
		rdec->stream_pos = readblock(rdec->infd, rdec->buffer, rd_buffer_size);
		if (rdec->stream_pos != rd_buffer_size && errno) {
			show_error( "Read error", errno, false );
			cleanup_and_fail(1);
		}
		rdec->at_stream_end = (rdec->stream_pos < rd_buffer_size);
		rdec->partial_member_pos += rdec->pos;
		rdec->pos = 0;
	}
	return rdec->pos < rdec->stream_pos;
}

void
LZd_flush_data(LZ_decoder *d)
{
	if (d->pos > d->stream_pos) {
		int size = d->pos - d->stream_pos;
		CRC32_update_buf(&d->crc, d->buffer + d->stream_pos, size);
		if (d->outfd >= 0 &&
		    writeblock(d->outfd, d->buffer + d->stream_pos, size) != size) {
			show_error( "Write error", errno, false );
			cleanup_and_fail(1);
		}
		if (d->pos >= d->dict_size) {
			d->partial_data_pos += d->pos;
			d->pos = 0;
			d->pos_wrapped = true;
		}
		d->stream_pos = d->pos;
	}
}

static bool
LZd_verify_trailer(LZ_decoder *d, Pretty_print *pp)
{
	File_trailer trailer;
	int	size = Rd_read_data(d->rdec, trailer, Ft_size);
	uvlong data_size = LZd_data_position(d);
	uvlong member_size = Rd_member_position(d->rdec);
	bool error = false;

	if (size < Ft_size) {
		error = true;
		if (verbosity >= 0) {
			Pp_show_msg(pp, 0);
			fprintf( stderr, "Trailer truncated at trailer position %d;"
			    " some checks may fail.\n", size );
		}
		while (size < Ft_size)
			trailer[size++] = 0;
	}

	if (Ft_get_data_crc(trailer) != LZd_crc(d)) {
		error = true;
		if (verbosity >= 0) {
			Pp_show_msg(pp, 0);
			fprintf( stderr, "CRC mismatch; trailer says %08X, data CRC is %08X\n",
			    Ft_get_data_crc(trailer), LZd_crc(d));
		}
	}
	if (Ft_get_data_size(trailer) != data_size) {
		error = true;
		if (verbosity >= 0) {
			Pp_show_msg(pp, 0);
			fprintf( stderr, "Data size mismatch; trailer says %llud, data size is %llud (0x%lluX)\n",
			    Ft_get_data_size(trailer), data_size, data_size);
		}
	}
	if (Ft_get_member_size(trailer) != member_size) {
		error = true;
		if (verbosity >= 0) {
			Pp_show_msg(pp, 0);
			fprintf(stderr, "Member size mismatch; trailer says %llud, member size is %llud (0x%lluX)\n",
			    Ft_get_member_size(trailer), member_size, member_size);
		}
	}
	if (0 && !error && verbosity >= 2 && data_size > 0 && member_size > 0)
		fprintf(stderr, "%6.3f:1, %6.3f bits/byte, %5.2f%% saved.  ",
		    (double)data_size / member_size,
		    (8.0 * member_size) / data_size,
		    100.0 * (1.0 - (double)member_size / data_size));
	if (!error && verbosity >= 4)
		fprintf( stderr, "CRC %08X, decompressed %9llud, compressed %8llud.  ",
		    LZd_crc(d), data_size, member_size);
	return !error;
}

/* Return value: 0 = OK, 1 = decoder error, 2 = unexpected EOF,
                 3 = trailer error, 4 = unknown marker found. */
int
LZd_decode_member(LZ_decoder *d, Pretty_print *pp)
{
	Range_decoder *rdec = d->rdec;
	Bit_model bm_literal[1<<literal_context_bits][0x300];
	Bit_model bm_match[states][pos_states];
	Bit_model bm_rep[states];
	Bit_model bm_rep0[states];
	Bit_model bm_rep1[states];
	Bit_model bm_rep2[states];
	Bit_model bm_len[states][pos_states];
	Bit_model bm_dis_slot[len_states][1<<dis_slot_bits];
	Bit_model bm_dis[modeled_distances-end_dis_model+1];
	Bit_model bm_align[dis_align_size];
	Len_model match_len_model;
	Len_model rep_len_model;
	unsigned	rep0 = 0;		/* rep[0-3] latest four distances */
	unsigned	rep1 = 0;		/* used for efficient coding of */
	unsigned	rep2 = 0;		/* repeated distances */
	unsigned	rep3 = 0;
	State state = 0;

	Bm_array_init(bm_literal[0], (1 << literal_context_bits) * 0x300);
	Bm_array_init(bm_match[0], states * pos_states);
	Bm_array_init(bm_rep, states);
	Bm_array_init(bm_rep0, states);
	Bm_array_init(bm_rep1, states);
	Bm_array_init(bm_rep2, states);
	Bm_array_init(bm_len[0], states * pos_states);
	Bm_array_init(bm_dis_slot[0], len_states * (1 << dis_slot_bits));
	Bm_array_init(bm_dis, modeled_distances - end_dis_model + 1);
	Bm_array_init(bm_align, dis_align_size);
	Lm_init(&match_len_model);
	Lm_init(&rep_len_model);

	Rd_load(rdec);
	while (!Rd_finished(rdec)) {
		int pos_state = LZd_data_position(d) & pos_state_mask;
		if (Rd_decode_bit(rdec, &bm_match[state][pos_state]) == 0)	/* 1st bit */ {
			Bit_model * bm = bm_literal[get_lit_state(LZd_peek_prev(d))];
			if (St_is_char(state)) {
				state -= (state < 4) ? state : 3;
				LZd_put_byte(d, Rd_decode_tree8(rdec, bm));
			} else {
				state -= (state < 10) ? 3 : 6;
				LZd_put_byte(d, Rd_decode_matched(rdec, bm, LZd_peek(d, rep0)));
			}
		} else	/* match or repeated match */      {
			int	len;
			if (Rd_decode_bit(rdec, &bm_rep[state]) != 0)		/* 2nd bit */ {
				if (Rd_decode_bit(rdec, &bm_rep0[state]) == 0)	/* 3rd bit */ {
					if (Rd_decode_bit(rdec, &bm_len[state][pos_state]) == 0) /* 4th bit */ {
						state = St_set_short_rep(state);
						LZd_put_byte(d, LZd_peek(d, rep0));
						continue;
					}
				} else {
					unsigned	distance;
					if (Rd_decode_bit(rdec, &bm_rep1[state]) == 0)	/* 4th bit */
						distance = rep1;
					else {
						if (Rd_decode_bit(rdec, &bm_rep2[state]) == 0)	/* 5th bit */
							distance = rep2;
						else {
							distance = rep3;
							rep3 = rep2;
						}
						rep2 = rep1;
					}
					rep1 = rep0;
					rep0 = distance;
				}
				state = St_set_rep(state);
				len = min_match_len + Rd_decode_len(rdec, &rep_len_model, pos_state);
			} else	/* match */        {
				unsigned	distance;
				len = min_match_len + Rd_decode_len(rdec, &match_len_model, pos_state);
				distance = Rd_decode_tree6(rdec, bm_dis_slot[get_len_state(len)]);
				if (distance >= start_dis_model) {
					unsigned dis_slot = distance;
					int direct_bits = (dis_slot >> 1) - 1;
					distance = (2 | (dis_slot & 1)) << direct_bits;
					if (dis_slot < end_dis_model)
						distance += Rd_decode_tree_reversed(rdec,
						    bm_dis + (distance - dis_slot), direct_bits);
					else {
						distance +=
						    Rd_decode(rdec, direct_bits - dis_align_bits) << dis_align_bits;
						distance += Rd_decode_tree_reversed4(rdec, bm_align);
						if (distance == 0xFFFFFFFFU)		/* marker found */ {
							Rd_normalize(rdec);
							LZd_flush_data(d);
							if (len == min_match_len)	/* End Of Stream marker */ {
								if (LZd_verify_trailer(d, pp))
/* code folded from here */
	return 0;
/* unfolding */
								else
/* code folded from here */
	return 3;
/* unfolding */
							}
							if (len == min_match_len + 1)	/* Sync Flush marker */ {
								Rd_load(rdec);
								continue;
							}
							if (verbosity >= 0) {
								Pp_show_msg(pp, 0);
								fprintf( stderr, "Unsupported marker code '%d'\n", len );
							}
							return 4;
						}
					}
				}
				rep3 = rep2;
				rep2 = rep1;
				rep1 = rep0;
				rep0 = distance;
				state = St_set_match(state);
				if (rep0 >= d->dict_size || (rep0 >= d->pos && !d->pos_wrapped)) {
					LZd_flush_data(d);
					return 1;
				}
			}
			LZd_copy_block(d, rep0, len);
		}
	}
	LZd_flush_data(d);
	return 2;
}


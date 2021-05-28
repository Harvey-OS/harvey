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
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lzip.h"

static void
Dis_slots_init(void)
{
	int	i, size, slot;
	for (slot = 0; slot < 4; ++slot)
		dis_slots[slot] = slot;
	for (i = 4, size = 2, slot = 4; slot < 20; slot += 2) {
		memset(&dis_slots[i], slot, size);
		memset(&dis_slots[i+size], slot + 1, size);
		size <<= 1;
		i += size;
	}
}

static uchar
get_slot(unsigned dis)
{
	if (dis < (1 << 10))
		return dis_slots[dis];
	if (dis < (1 << 19))
		return dis_slots[dis>> 9] + 18;
	if (dis < (1 << 28))
		return dis_slots[dis>>18] + 36;
	return dis_slots[dis>>27] + 54;
}

static void
Prob_prices_init(void)
{
	int	i, j;
	for (i = 0; i < bit_model_total >> price_step_bits; ++i) {
		unsigned val = (i * price_step) + (price_step / 2);
		int bits = 0;			/* base 2 logarithm of val */

		for (j = 0; j < price_shift_bits; ++j) {
			val = val * val;
			bits <<= 1;
			while (val >= (1 << 16)) {
				val >>= 1;
				++bits;
			}
		}
		bits += 15;			/* remaining bits in val */
		prob_prices[i] = (bit_model_total_bits << price_shift_bits) - bits;
	}
}

static int
price_symbol3(Bit_model bm[], int symbol)
{
	int price;
	bool bit = symbol & 1;

	symbol |= 8;
	symbol >>= 1;
	price = price_bit(bm[symbol], bit);
	bit = symbol & 1;
	symbol >>= 1;
	price += price_bit(bm[symbol], bit);
	return price + price_bit(bm[1], symbol & 1);
}

static int
price_symbol6(Bit_model bm[], unsigned symbol)
{
	int	price;
	bool bit = symbol & 1;

	symbol |= 64;
	symbol >>= 1;
	price = price_bit(bm[symbol], bit);
	bit = symbol & 1;
	symbol >>= 1;
	price += price_bit(bm[symbol], bit);
	bit = symbol & 1;
	symbol >>= 1;
	price += price_bit(bm[symbol], bit);
	bit = symbol & 1;
	symbol >>= 1;
	price += price_bit(bm[symbol], bit);
	bit = symbol & 1;
	symbol >>= 1;
	price += price_bit(bm[symbol], bit);
	return price + price_bit(bm[1], symbol & 1);
}

static int
price_symbol8(Bit_model bm[], int symbol)
{
	int	price;
	bool bit = symbol & 1;
	symbol |= 0x100;
	symbol >>= 1;
	price = price_bit(bm[symbol], bit);
	bit = symbol & 1;
	symbol >>= 1;
	price += price_bit(bm[symbol], bit);
	bit = symbol & 1;
	symbol >>= 1;
	price += price_bit(bm[symbol], bit);
	bit = symbol & 1;
	symbol >>= 1;
	price += price_bit(bm[symbol], bit);
	bit = symbol & 1;
	symbol >>= 1;
	price += price_bit(bm[symbol], bit);
	bit = symbol & 1;
	symbol >>= 1;
	price += price_bit(bm[symbol], bit);
	bit = symbol & 1;
	symbol >>= 1;
	price += price_bit(bm[symbol], bit);
	return price + price_bit(bm[1], symbol & 1);
}

static int
price_symbol_reversed(Bit_model bm[], int symbol, int num_bits)
{
	int	price = 0;
	int	model = 1;
	int	i;

	for (i = num_bits; i > 0; --i) {
		bool bit = symbol & 1;
		symbol >>= 1;
		price += price_bit(bm[model], bit);
		model = (model << 1) | bit;
	}
	return price;
}

static int
price_matched(Bit_model bm[], unsigned symbol, unsigned match_byte)
{
	int price = 0;
	unsigned mask = 0x100;

	symbol |= mask;
	for (;;) {
		unsigned match_bit = (match_byte <<= 1) & mask;
		bool bit = (symbol <<= 1) & 0x100;

		price += price_bit(bm[(symbol>>9) + match_bit + mask], bit);
		if (symbol >= 0x10000)
			return price;
		mask &= ~(match_bit ^ symbol);
		/* if(match_bit != bit) mask = 0; */
	}
}

struct Matchfinder_base {
	uvlong partial_data_pos;
	uchar * buffer;		/* input buffer */
	int32_t * prev_positions;	/* 1 + last seen position of key. else 0 */
	int32_t * pos_array;		/* may be tree or chain */
	int	before_size;		/* bytes to keep in buffer before dictionary */
	int	buffer_size;
	int	dict_size;	/* bytes to keep in buffer before pos */
	int	pos;			/* current pos in buffer */
	int	cyclic_pos;		/* cycles through [0, dict_size] */
	int	stream_pos;		/* first byte not yet read from file */
	int	pos_limit;		/* when reached, a new block must be read */
	int	key4_mask;
	int	num_prev_positions;	/* size of prev_positions */
	int	pos_array_size;
	int	infd;			/* input file descriptor */
	bool at_stream_end;		/* stream_pos shows real end of file */
};

bool	Mb_read_block(Matchfinder_base *mb);
void	Mb_normalize_pos(Matchfinder_base *mb);
bool	Mb_init(Matchfinder_base *mb, int before, int dict_size, int after_size, int dict_factor, int num_prev_positions23, int pos_array_factor, int ifd);

static void
Mb_free(Matchfinder_base *mb)
{
	free(mb->prev_positions);
	free(mb->buffer);
}

static int
Mb_avail_bytes(Matchfinder_base *mb)
{
	return mb->stream_pos - mb->pos;
}

static uvlong
Mb_data_position(Matchfinder_base *mb)
{
	return mb->partial_data_pos + mb->pos;
}

static bool
Mb_data_finished(Matchfinder_base *mb)
{
	return mb->at_stream_end && mb->pos >= mb->stream_pos;
}

static int
Mb_true_match_len(Matchfinder_base *mb, int index, int distance)
{
	uchar * data = mb->buffer + mb->pos;
	int	i = index;
	int len_limit = min(Mb_avail_bytes(mb), max_match_len);
	while (i < len_limit && data[i-distance] == data[i])
		++i;
	return i;
}

static void
Mb_move_pos(Matchfinder_base *mb)
{
	if (++mb->cyclic_pos > mb->dict_size)
		mb->cyclic_pos = 0;
	if (++mb->pos >= mb->pos_limit)
		Mb_normalize_pos(mb);
}

void	Mb_reset(Matchfinder_base *mb);

enum {	re_buffer_size = 65536 };

typedef struct LZ_encoder_base LZ_encoder_base;
typedef struct Matchfinder_base Matchfinder_base;
typedef struct Range_encoder Range_encoder;

struct Range_encoder {
	uvlong low;
	uvlong partial_member_pos;
	uchar * buffer;		/* output buffer */
	int	pos;			/* current pos in buffer */
	uint32_t range;
	unsigned	ff_count;
	int	outfd;			/* output file descriptor */
	uchar cache;
	File_header header;
};

void	Re_flush_data(Range_encoder *renc);

static void
Re_put_byte(Range_encoder *renc, uchar b)
{
	renc->buffer[renc->pos] = b;
	if (++renc->pos >= re_buffer_size)
		Re_flush_data(renc);
}

static void
Re_shift_low(Range_encoder *renc)
{
	if (renc->low >> 24 != 0xFF) {
		bool carry = (renc->low > 0xFFFFFFFFU);
		Re_put_byte(renc, renc->cache + carry);
		for (; renc->ff_count > 0; --renc->ff_count)
			Re_put_byte(renc, 0xFF + carry);
		renc->cache = renc->low >> 24;
	} else
		++renc->ff_count;
	renc->low = (renc->low & 0x00FFFFFFU) << 8;
}

static void
Re_reset(Range_encoder *renc)
{
	int	i;
	renc->low = 0;
	renc->partial_member_pos = 0;
	renc->pos = 0;
	renc->range = 0xFFFFFFFFU;
	renc->ff_count = 0;
	renc->cache = 0;
	for (i = 0; i < Fh_size; ++i)
		Re_put_byte(renc, renc->header[i]);
}

static bool
Re_init(Range_encoder *renc, unsigned dict_size, int ofd)
{
	renc->buffer = (uchar *)malloc(re_buffer_size);
	if (!renc->buffer)
		return false;
	renc->outfd = ofd;
	Fh_set_magic(renc->header);
	Fh_set_dict_size(renc->header, dict_size);
	Re_reset(renc);
	return true;
}

static void
Re_free(Range_encoder *renc)
{
	free(renc->buffer);
}

static uvlong
Re_member_position(Range_encoder *renc)
{
	return renc->partial_member_pos + renc->pos + renc->ff_count;
}

static void
Re_flush(Range_encoder *renc)
{
	int	i;
	for (i = 0; i < 5; ++i)
		Re_shift_low(renc);
}

static void
Re_encode(Range_encoder *renc, int symbol, int num_bits)
{
	unsigned	mask;
	for (mask = 1 << (num_bits - 1); mask > 0; mask >>= 1) {
		renc->range >>= 1;
		if (symbol & mask)
			renc->low += renc->range;
		if (renc->range <= 0x00FFFFFFU) {
			renc->range <<= 8;
			Re_shift_low(renc);
		}
	}
}

static void
Re_encode_bit(Range_encoder *renc, Bit_model *probability, bool bit)
{
	Bit_model prob = *probability;
	uint32_t bound = (renc->range >> bit_model_total_bits) * prob;

	if (!bit) {
		renc->range = bound;
		*probability += (bit_model_total - prob) >> bit_model_move_bits;
	} else {
		renc->low += bound;
		renc->range -= bound;
		*probability -= prob >> bit_model_move_bits;
	}
	if (renc->range <= 0x00FFFFFFU) {
		renc->range <<= 8;
		Re_shift_low(renc);
	}
}

static void
Re_encode_tree3(Range_encoder *renc, Bit_model bm[], int symbol)
{
	int model = 1;
	bool bit = (symbol >> 2) & 1;

	Re_encode_bit(renc, &bm[model], bit);
	model = (model << 1) | bit;
	bit = (symbol >> 1) & 1;
	Re_encode_bit(renc, &bm[model], bit);
	model = (model << 1) | bit;
	Re_encode_bit(renc, &bm[model], symbol & 1);
}

static void
Re_encode_tree6(Range_encoder *renc, Bit_model bm[], unsigned symbol)
{
	int	model = 1;
	bool bit = (symbol >> 5) & 1;
	Re_encode_bit(renc, &bm[model], bit);
	model = (model << 1) | bit;
	bit = (symbol >> 4) & 1;
	Re_encode_bit(renc, &bm[model], bit);
	model = (model << 1) | bit;
	bit = (symbol >> 3) & 1;
	Re_encode_bit(renc, &bm[model], bit);
	model = (model << 1) | bit;
	bit = (symbol >> 2) & 1;
	Re_encode_bit(renc, &bm[model], bit);
	model = (model << 1) | bit;
	bit = (symbol >> 1) & 1;
	Re_encode_bit(renc, &bm[model], bit);
	model = (model << 1) | bit;
	Re_encode_bit(renc, &bm[model], symbol & 1);
}

static void
Re_encode_tree8(Range_encoder *renc, Bit_model bm[], int symbol)
{
	int	model = 1;
	int	i;
	for (i = 7; i >= 0; --i) {
		bool bit = (symbol >> i) & 1;
		Re_encode_bit(renc, &bm[model], bit);
		model = (model << 1) | bit;
	}
}

static void
Re_encode_tree_reversed(Range_encoder *renc, Bit_model bm[], int symbol, int num_bits)
{
	int	model = 1;
	int	i;
	for (i = num_bits; i > 0; --i) {
		bool bit = symbol & 1;
		symbol >>= 1;
		Re_encode_bit(renc, &bm[model], bit);
		model = (model << 1) | bit;
	}
}

static void
Re_encode_matched(Range_encoder *renc, Bit_model bm[], unsigned symbol, unsigned match_byte)
{
	unsigned	mask = 0x100;
	symbol |= mask;
	while (true) {
		unsigned match_bit = (match_byte <<= 1) & mask;
		bool bit = (symbol <<= 1) & 0x100;
		Re_encode_bit(renc, &bm[(symbol>>9)+match_bit+mask], bit);
		if (symbol >= 0x10000)
			break;
		mask &= ~(match_bit ^ symbol);
		/* if(match_bit != bit) mask = 0; */
	}
}

static void
Re_encode_len(struct Range_encoder *renc, Len_model *lm, int symbol, int pos_state)
{
	bool bit = ((symbol -= min_match_len) >= len_low_syms);
	Re_encode_bit(renc, &lm->choice1, bit);
	if (!bit)
		Re_encode_tree3(renc, lm->bm_low[pos_state], symbol);
	else {
		bit = ((symbol -= len_low_syms) >= len_mid_syms);
		Re_encode_bit(renc, &lm->choice2, bit);
		if (!bit)
			Re_encode_tree3(renc, lm->bm_mid[pos_state], symbol);
		else
			Re_encode_tree8(renc, lm->bm_high, symbol - len_mid_syms);
	}
}

enum {
	max_marker_size = 16,
	num_rep_distances = 4		/* must be 4 */
};

struct LZ_encoder_base {
	struct Matchfinder_base mb;
	uint32_t crc;

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
	struct Len_model match_len_model;
	struct Len_model rep_len_model;
	struct Range_encoder renc;
};

void	LZeb_reset(LZ_encoder_base *eb);

static bool
LZeb_init(LZ_encoder_base *eb, int before, int dict_size, int after_size, int dict_factor, int num_prev_positions23, int pos_array_factor, int ifd, int outfd)
{
	if (!Mb_init(&eb->mb, before, dict_size, after_size, dict_factor,
	    num_prev_positions23, pos_array_factor, ifd))
		return false;
	if (!Re_init(&eb->renc, eb->mb.dict_size, outfd))
		return false;
	LZeb_reset(eb);
	return true;
}

static void
LZeb_free(LZ_encoder_base *eb)
{
	Re_free(&eb->renc);
	Mb_free(&eb->mb);
}

static unsigned
LZeb_crc(LZ_encoder_base *eb)
{
	return eb->crc ^ 0xFFFFFFFFU;
}

static int
LZeb_price_literal(LZ_encoder_base *eb, uchar prev_byte, uchar symbol)
{
	return price_symbol8(eb->bm_literal[get_lit_state(prev_byte)], symbol);
}

static int
LZeb_price_matched(LZ_encoder_base *eb, uchar prev_byte, uchar symbol, uchar match_byte)
{
	return price_matched(eb->bm_literal[get_lit_state(prev_byte)], symbol,
	    match_byte);
}

static void
LZeb_encode_literal(LZ_encoder_base *eb, uchar prev_byte, uchar symbol)
{
	Re_encode_tree8(&eb->renc, eb->bm_literal[get_lit_state(prev_byte)],
	    symbol);
}

static void
LZeb_encode_matched(LZ_encoder_base *eb, uchar prev_byte, uchar symbol, uchar match_byte)
{
	Re_encode_matched(&eb->renc, eb->bm_literal[get_lit_state(prev_byte)],
	    symbol, match_byte);
}

static void
LZeb_encode_pair(LZ_encoder_base *eb, unsigned dis, int len, int pos_state)
{
	unsigned dis_slot = get_slot(dis);
	Re_encode_len(&eb->renc, &eb->match_len_model, len, pos_state);
	Re_encode_tree6(&eb->renc, eb->bm_dis_slot[get_len_state(len)], dis_slot);

	if (dis_slot >= start_dis_model) {
		int direct_bits = (dis_slot >> 1) - 1;
		unsigned base = (2 | (dis_slot & 1)) << direct_bits;
		unsigned direct_dis = dis - base;

		if (dis_slot < end_dis_model)
			Re_encode_tree_reversed(&eb->renc, eb->bm_dis + (base - dis_slot),
			    direct_dis, direct_bits);
		else {
			Re_encode(&eb->renc, direct_dis >> dis_align_bits,
			    direct_bits - dis_align_bits);
			Re_encode_tree_reversed(&eb->renc, eb->bm_align, direct_dis, dis_align_bits);
		}
	}
}

void	LZeb_full_flush(LZ_encoder_base *eb, State state);

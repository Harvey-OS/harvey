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
#include "encoder.h"

CRC32 crc32;

/*
 * starting at data[len] and data[len-delta], what's the longest match
 * up to len_limit?
 */
int
maxmatch(uchar *data, int delta, int len, int len_limit)
{
	uchar *pdel, *p;

	p = &data[len];
	pdel = p - delta;
	while (*pdel++ == *p++ && len < len_limit)
		++len;
	return len;
}

static int
findpairmaxlen(LZ_encoder *e, Pair **pairsp, int *npairsp, int maxlen, int pos1,
	int len_limit, int min_pos, uchar *data, int np2, int np3)
{
	int num_pairs;
	Pair *pairs;

	pairs = *pairsp;
	num_pairs = *npairsp;
	if (np2 > min_pos && e->eb.mb.buffer[np2-1] == data[0]) {
		pairs[0].dis = e->eb.mb.pos - np2;
		pairs[0].len = maxlen = 2;
		num_pairs = 1;
	}
	if (np2 != np3 && np3 > min_pos && e->eb.mb.buffer[np3-1] == data[0]) {
		maxlen = 3;
		np2 = np3;
		pairs[num_pairs].dis = e->eb.mb.pos - np2;
		++num_pairs;
	}
	if (num_pairs > 0) {
		maxlen = maxmatch(data, pos1 - np2, maxlen, len_limit);
		pairs[num_pairs-1].len = maxlen;
		if (maxlen >= len_limit)
			*pairsp = nil;	/* done. now just skip */
	}
	if (maxlen < 3)
		maxlen = 3;
	*npairsp = num_pairs;
	return maxlen;
}

int
LZe_get_match_pairs(LZ_encoder *e, Pair *pairs)
{
	int len = 0, len0, len1, maxlen, num_pairs, len_limit, avail;
	int pos1, min_pos, cyclic_pos, delta, count, key2, key3, key4, newpos1;
	int32_t *ptr0, *ptr1, *newptr, *prevpos;
	uchar *data;
	uchar *p;
	unsigned tmp;

	len_limit = e->match_len_limit;
	avail = Mb_avail_bytes(&e->eb.mb);
	if (len_limit > avail) {
		len_limit = avail;
		if (len_limit < 4)
			return 0;
	}

	data = Mb_ptr_to_current_pos(&e->eb.mb);
	tmp = crc32[data[0]] ^ data[1];
	key2 = tmp & (Nprevpos2 - 1);
	tmp ^= (unsigned)data[2] << 8;
	key3 = Nprevpos2 + (tmp & (Nprevpos3 - 1));
	key4 = Nprevpos2 + Nprevpos3 +
	    ((tmp ^ (crc32[data[3]] << 5)) & e->eb.mb.key4_mask);

	min_pos = (e->eb.mb.pos > e->eb.mb.dict_size) ?
	    e->eb.mb.pos - e->eb.mb.dict_size : 0;
	pos1 = e->eb.mb.pos + 1;
	prevpos = e->eb.mb.prev_positions;
	maxlen = 0;
	num_pairs = 0;
	if (pairs)
		maxlen = findpairmaxlen(e, &pairs, &num_pairs, maxlen, pos1,
			len_limit, min_pos, data, prevpos[key2], prevpos[key3]);
	newpos1 = prevpos[key4];
	prevpos[key2] = prevpos[key3] = prevpos[key4] = pos1;

	cyclic_pos = e->eb.mb.cyclic_pos;
	ptr0 = e->eb.mb.pos_array + (cyclic_pos << 1);
	ptr1 = ptr0 + 1;
	len0 = len1 = 0;
	for (count = e->cycles; ;) {
		if (newpos1 <= min_pos || --count < 0) {
			*ptr0 = *ptr1 = 0;
			break;
		}

		delta = pos1 - newpos1;
		newptr = e->eb.mb.pos_array + ((cyclic_pos - delta +
		    (cyclic_pos >= delta? 0: e->eb.mb.dict_size + 1)) << 1);
		p = &data[len];
		if (p[-delta] == *p) {
			len = maxmatch(data, delta, len + 1, len_limit);
			if (pairs && maxlen < len) {
				pairs[num_pairs].dis = delta - 1;
				pairs[num_pairs].len = maxlen = len;
				++num_pairs;
			}
			if (len >= len_limit) {
				*ptr0 = newptr[0];
				*ptr1 = newptr[1];
				break;
			}
			p = &data[len];
		}
		if (p[-delta] < *p) {
			*ptr0 = newpos1;
			ptr0 = newptr + 1;
			newpos1 = *ptr0;
			len0 = len;
			if (len1 < len)
				len = len1;
		} else {
			*ptr1 = newpos1;
			ptr1 = newptr;
			newpos1 = *ptr1;
			len1 = len;
			if (len0 < len)
				len = len0;
		}
	}
	return num_pairs;
}

static void
LZe_update_distance_prices(LZ_encoder *e)
{
	int dis, len_state;

	for (dis = start_dis_model; dis < modeled_distances; ++dis) {
		int dis_slot = dis_slots[dis];
		int direct_bits = (dis_slot >> 1) - 1;
		int base = (2 | (dis_slot & 1)) << direct_bits;
		int price = price_symbol_reversed(e->eb.bm_dis + (base - dis_slot),
		    dis - base, direct_bits);

		for (len_state = 0; len_state < len_states; ++len_state)
			e->dis_prices[len_state][dis] = price;
	}

	for (len_state = 0; len_state < len_states; ++len_state) {
		int *dsp = e->dis_slot_prices[len_state];
		int *dp = e->dis_prices[len_state];
		Bit_model * bmds = e->eb.bm_dis_slot[len_state];
		int slot = 0;

		for (; slot < end_dis_model; ++slot)
			dsp[slot] = price_symbol6(bmds, slot);
		for (; slot < e->num_dis_slots; ++slot)
			dsp[slot] = price_symbol6(bmds, slot) +
			    ((((slot >> 1) - 1) - dis_align_bits) << price_shift_bits);

		for (dis = 0; dis < start_dis_model; ++dis)
			dp[dis] = dsp[dis];
		for (; dis < modeled_distances; ++dis)
			dp[dis] += dsp[dis_slots[dis]];
	}
}

static int
pricestate2(LZ_encoder *e, int price, int *ps2p, State *st2p, int len2)
{
	int pos_state2;
	State state2;

	state2 = *st2p;
	pos_state2 = *ps2p;

	pos_state2 = (pos_state2 + 1) & pos_state_mask;
	state2 = St_set_char(state2);
	price += price1(e->eb.bm_match[state2][pos_state2]) +
		price1(e->eb.bm_rep[state2]) +
		LZe_price_rep0_len(e, len2, state2, pos_state2);

	*ps2p = pos_state2;
	*st2p = state2;
	return price;
}

static int
encinit(LZ_encoder *e, int reps[num_rep_distances],
	int replens[num_rep_distances], State state, int main_len,
	int num_pairs, int rep_index, int *ntrialsp)
{
	int i, rep, num_trials, len;
	int pos_state = Mb_data_position(&e->eb.mb) & pos_state_mask;
	int match_price = price1(e->eb.bm_match[state][pos_state]);
	int rep_match_price = match_price + price1(e->eb.bm_rep[state]);
	uchar prev_byte = Mb_peek(&e->eb.mb, 1);
	uchar cur_byte = Mb_peek(&e->eb.mb, 0);
	uchar match_byte = Mb_peek(&e->eb.mb, reps[0] + 1);

	e->trials[1].price = price0(e->eb.bm_match[state][pos_state]);
	if (St_is_char(state))
		e->trials[1].price += LZeb_price_literal(&e->eb,
			prev_byte, cur_byte);
	else
		e->trials[1].price += LZeb_price_matched(&e->eb,
			prev_byte, cur_byte, match_byte);
	e->trials[1].dis4 = -1;				/* literal */

	if (match_byte == cur_byte)
		Tr_update(&e->trials[1], rep_match_price +
		    LZeb_price_shortrep(&e->eb, state, pos_state), 0, 0);
	num_trials = replens[rep_index];
	if (num_trials < main_len)
		num_trials = main_len;
	*ntrialsp = num_trials;
	if (num_trials < min_match_len) {
		e->trials[0].price = 1;
		e->trials[0].dis4 = e->trials[1].dis4;
		Mb_move_pos(&e->eb.mb);
		return 1;
	}

	e->trials[0].state = state;
	for (i = 0; i < num_rep_distances; ++i)
		e->trials[0].reps[i] = reps[i];

	for (len = min_match_len; len <= num_trials; ++len)
		e->trials[len].price = infinite_price;

	for (rep = 0; rep < num_rep_distances; ++rep) {
		int price, replen;

		if (replens[rep] < min_match_len)
			continue;
		price = rep_match_price + LZeb_price_rep(&e->eb, rep,
			state, pos_state);
		replen = replens[rep];
		for (len = min_match_len; len <= replen; ++len)
			Tr_update(&e->trials[len], price +
			    Lp_price(&e->rep_len_prices, len, pos_state), rep, 0);
	}

	if (main_len > replens[0]) {
		int dis, normal_match_price = match_price +
			price0(e->eb.bm_rep[state]);
		int replp1 = replens[0] + 1;
		int i = 0, len = max(replp1, min_match_len);

		while (len > e->pairs[i].len)
			++i;
		for (;;) {
			dis = e->pairs[i].dis;
			Tr_update(&e->trials[len], normal_match_price +
			    LZe_price_pair(e, dis, len, pos_state),
			    dis + num_rep_distances, 0);
			if (++len > e->pairs[i].len && ++i >= num_pairs)
				break;
		}
	}
	return 0;
}

static void
finalvalues(LZ_encoder *e, int cur, Trial *cur_trial, State *cstatep)
{
	int i;
	int dis4 = cur_trial->dis4;
	int prev_index = cur_trial->prev_index;
	int prev_index2 = cur_trial->prev_index2;
	State cur_state;

	if (prev_index2 == single_step_trial) {
		cur_state = e->trials[prev_index].state;
		if (prev_index + 1 == cur) {	/* len == 1 */
			if (dis4 == 0)
				cur_state = St_set_short_rep(cur_state);
			else
				cur_state = St_set_char(cur_state);	/* literal */
		} else if (dis4 < num_rep_distances)
			cur_state = St_set_rep(cur_state);
		else
			cur_state = St_set_match(cur_state);
	} else {
		if (prev_index2 == dual_step_trial)	/* dis4 == 0 (rep0) */
			--prev_index;
		else	/* prev_index2 >= 0 */
			prev_index = prev_index2;
		cur_state = 8;		/* St_set_char_rep(); */
	}
	cur_trial->state = cur_state;
	for (i = 0; i < num_rep_distances; ++i)
		cur_trial->reps[i] = e->trials[prev_index].reps[i];
	mtf_reps(dis4, cur_trial->reps);	/* literal is ignored */
	*cstatep = cur_state;
}

static int
litrep0(LZ_encoder *e, State cur_state, int cur, Trial *cur_trial,
	int num_trials, int triable_bytes, int pos_state, int next_price)
{
	int len = 1, endtrials, limit, mlpl1, dis;
	uchar *data = Mb_ptr_to_current_pos(&e->eb.mb);

	dis = cur_trial->reps[0] + 1;
	mlpl1 = e->match_len_limit + 1;
	limit = min(mlpl1, triable_bytes);
	len = maxmatch(data, dis, len, limit);
	if (--len >= min_match_len) {
		int pos_state2, price;
		State state2;

		pos_state2 = (pos_state + 1) & pos_state_mask;
		state2 = St_set_char(cur_state);
		price = next_price + price1(e->eb.bm_match[state2][pos_state2])+
			price1(e->eb.bm_rep[state2]) +
			LZe_price_rep0_len(e, len, state2, pos_state2);
		endtrials = cur + 1 + len;
		while (num_trials < endtrials)
			e->trials[++num_trials].price = infinite_price;
		Tr_update2(&e->trials[endtrials], price, cur + 1);
	}
	return num_trials;
}

static int
repdists(LZ_encoder *e, State cur_state, int cur, Trial *cur_trial,
	int num_trials, int triable_bytes, int pos_state,
	int rep_match_price, int len_limit, int *stlenp)
{
	int i, rep, len, price, dis, start_len;

	start_len = *stlenp;
	for (rep = 0; rep < num_rep_distances; ++rep) {
		uchar *data = Mb_ptr_to_current_pos(&e->eb.mb);

		dis = cur_trial->reps[rep] + 1;
		if (data[0-dis] != data[0] || data[1-dis] != data[1])
			continue;
		len = maxmatch(data, dis, min_match_len, len_limit);
		while (num_trials < cur + len)
			e->trials[++num_trials].price = infinite_price;
		price = rep_match_price + LZeb_price_rep(&e->eb, rep,
			cur_state, pos_state);
		for (i = min_match_len; i <= len; ++i)
			Tr_update(&e->trials[cur+i], price +
			    Lp_price(&e->rep_len_prices, i, pos_state), rep, cur);

		if (rep == 0)
			start_len = len + 1; /* discard shorter matches */

		/* try rep + literal + rep0 */
		{
			int pos_state2, endtrials, limit, mlpl2, len2;
			State state2;

			len2 = len + 1;
			mlpl2 = e->match_len_limit + len2;
			limit = min(mlpl2, triable_bytes);
			len2 = maxmatch(data, dis, len2, limit);
			len2 -= len + 1;
			if (len2 < min_match_len)
				continue;

			pos_state2 = (pos_state + len) & pos_state_mask;
			state2 = St_set_rep(cur_state);
			price += Lp_price(&e->rep_len_prices, len, pos_state) +
			    price0(e->eb.bm_match[state2][pos_state2]) +
			    LZeb_price_matched(&e->eb, data[len-1],
				data[len], data[len-dis]);
			price = pricestate2(e, price, &pos_state2,
				&state2, len2);
			endtrials = cur + len + 1 + len2;
			while (num_trials < endtrials)
				e->trials[++num_trials].price = infinite_price;
			Tr_update3(&e->trials[endtrials], price, rep,
				endtrials - len2, cur);
		}
	}
	*stlenp = start_len;
	return num_trials;
}

static int
trymatches(LZ_encoder *e, State cur_state, int cur, int num_trials,
	int triable_bytes, int pos_state, int num_pairs,
	int normal_match_price, int start_len)
{
	int i, dis, len, price;

	i = 0;
	while (e->pairs[i].len < start_len)
		++i;
	dis = e->pairs[i].dis;
	for (len = start_len; ; ++len) {
		price = normal_match_price + LZe_price_pair(e, dis, len, pos_state);
		Tr_update(&e->trials[cur+len], price, dis + num_rep_distances, cur);

		/* try match + literal + rep0 */
		if (len == e->pairs[i].len) {
			uchar *data = Mb_ptr_to_current_pos(&e->eb.mb);
			int endtrials, mlpl2, limit;
			int dis2 = dis + 1, len2 = len + 1;

			mlpl2 = e->match_len_limit + len2;
			limit = min(mlpl2, triable_bytes);
			len2 = maxmatch(data, dis2, len2, limit);
			len2 -= len + 1;
			if (len2 >= min_match_len) {
				int pos_state2 = (pos_state + len) &pos_state_mask;
				State state2 = St_set_match(cur_state);

	 			price += price0(e->eb.bm_match[state2][pos_state2]) +
				    LZeb_price_matched(&e->eb, data[len-1], data[len], data[len-dis2]);
				price = pricestate2(e, price,
				    &pos_state2, &state2, len2);
				endtrials = cur + len + 1 + len2;
				while (num_trials < endtrials)
					e->trials[++num_trials].price = infinite_price;
				Tr_update3(&e->trials[endtrials],
					price, dis +
					num_rep_distances,
					endtrials - len2, cur);
			}
			if (++i >= num_pairs)
				break;
			dis = e->pairs[i].dis;
		}
	}
	return num_trials;
}

/*
 * Returns the number of bytes advanced (ahead).
   trials[0]..trials[ahead-1] contain the steps to encode.
   (trials[0].dis4 == -1) means literal.
   A match/rep longer or equal than match_len_limit finishes the sequence.
 */
static int
LZe_sequence_optimizer(LZ_encoder *e, int reps[num_rep_distances], State state)
{
	int main_len, num_pairs, i, num_trials;
	int rep_index = 0, cur = 0;
	int replens[num_rep_distances];

	if (e->pending_num_pairs > 0) {		/* from previous call */
		num_pairs = e->pending_num_pairs;
		e->pending_num_pairs = 0;
	} else
		num_pairs = LZe_read_match_distances(e);
	main_len = (num_pairs > 0) ? e->pairs[num_pairs-1].len : 0;

	for (i = 0; i < num_rep_distances; ++i) {
		replens[i] = Mb_true_match_len(&e->eb.mb, 0, reps[i] + 1);
		if (replens[i] > replens[rep_index])
			rep_index = i;
	}
	if (replens[rep_index] >= e->match_len_limit) {
		e->trials[0].price = replens[rep_index];
		e->trials[0].dis4 = rep_index;
		LZe_move_and_update(e, replens[rep_index]);
		return replens[rep_index];
	}

	if (main_len >= e->match_len_limit) {
		e->trials[0].price = main_len;
		e->trials[0].dis4 = e->pairs[num_pairs-1].dis + num_rep_distances;
		LZe_move_and_update(e, main_len);
		return main_len;
	}

	if (encinit(e, reps, replens, state, main_len, num_pairs, rep_index,
	    &num_trials) > 0)
		return 1;

	/*
	 * Optimize price.
	 */
	for (;;) {
		Trial *cur_trial, *next_trial;
		int newlen, pos_state, triable_bytes, len_limit;
		int next_price, match_price, rep_match_price;
		int start_len = min_match_len;
		State cur_state;
		uchar prev_byte, cur_byte, match_byte;

		Mb_move_pos(&e->eb.mb);
		if (++cur >= num_trials) {	/* no more initialized trials */
			LZe_backward(e, cur);
			return cur;
		}

		num_pairs = LZe_read_match_distances(e);
		newlen = num_pairs > 0? e->pairs[num_pairs-1].len: 0;
		if (newlen >= e->match_len_limit) {
			e->pending_num_pairs = num_pairs;
			LZe_backward(e, cur);
			return cur;
		}

		/* give final values to current trial */
		cur_trial = &e->trials[cur];
		finalvalues(e, cur, cur_trial, &cur_state);

		pos_state = Mb_data_position(&e->eb.mb) & pos_state_mask;
		prev_byte = Mb_peek(&e->eb.mb, 1);
		cur_byte = Mb_peek(&e->eb.mb, 0);
		match_byte = Mb_peek(&e->eb.mb, cur_trial->reps[0] + 1);

		next_price = cur_trial->price +
		    price0(e->eb.bm_match[cur_state][pos_state]);
		if (St_is_char(cur_state))
			next_price += LZeb_price_literal(&e->eb, prev_byte, cur_byte);
		else
			next_price += LZeb_price_matched(&e->eb, prev_byte,
				cur_byte, match_byte);

		/* try last updates to next trial */
		next_trial = &e->trials[cur+1];

		Tr_update(next_trial, next_price, -1, cur);	/* literal */

		match_price = cur_trial->price +
			price1(e->eb.bm_match[cur_state][pos_state]);
		rep_match_price = match_price + price1(e->eb.bm_rep[cur_state]);

		if (match_byte == cur_byte && next_trial->dis4 != 0 &&
		    next_trial->prev_index2 == single_step_trial) {
			int price = rep_match_price +
			    LZeb_price_shortrep(&e->eb, cur_state, pos_state);
			if (price <= next_trial->price) {
				next_trial->price = price;
				next_trial->dis4 = 0;		/* rep0 */
				next_trial->prev_index = cur;
			}
		}

		int trm1mcur = max_num_trials - 1 - cur;

		triable_bytes = Mb_avail_bytes(&e->eb.mb);
		if (triable_bytes > trm1mcur)
			triable_bytes = trm1mcur;
		if (triable_bytes < min_match_len)
			continue;

		len_limit = min(e->match_len_limit, triable_bytes);

		/* try literal + rep0 */
		if (match_byte != cur_byte && next_trial->prev_index != cur)
			num_trials = litrep0(e, cur_state, cur, cur_trial,
				num_trials, triable_bytes, pos_state, next_price);

		/* try rep distances */
		num_trials = repdists(e, cur_state, cur, cur_trial,
			num_trials, triable_bytes, pos_state,
			rep_match_price, len_limit, &start_len);

		/* try matches */
		if (newlen >= start_len && newlen <= len_limit) {
			int normal_match_price = match_price +
			    price0(e->eb.bm_rep[cur_state]);

			while (num_trials < cur + newlen)
				e->trials[++num_trials].price = infinite_price;

			num_trials = trymatches(e, cur_state, cur, num_trials,
				triable_bytes, pos_state, num_pairs,
				normal_match_price, start_len);
		}
	}
}

static int
encrepmatch(LZ_encoder *e, State state, int len, int dis, int pos_state)
{
	int bit = (dis == 0);

	Re_encode_bit(&e->eb.renc, &e->eb.bm_rep0[state], !bit);
	if (bit)
		Re_encode_bit(&e->eb.renc, &e->eb.bm_len[state][pos_state],
			len > 1);
	else {
		Re_encode_bit(&e->eb.renc, &e->eb.bm_rep1[state], dis > 1);
		if (dis > 1)
			Re_encode_bit(&e->eb.renc, &e->eb.bm_rep2[state],
				dis > 2);
	}
	if (len == 1)
		state = St_set_short_rep(state);
	else {
		Re_encode_len(&e->eb.renc, &e->eb.rep_len_model, len, pos_state);
		Lp_decr_counter(&e->rep_len_prices, pos_state);
		state = St_set_rep(state);
	}
	return state;
}

bool
LZe_encode_member(LZ_encoder *e, uvlong member_size)
{
	uvlong member_size_limit = member_size - Ft_size - max_marker_size;
	bool best = (e->match_len_limit > 12);
	int dis_price_count = best? 1: 512;
	int align_price_count = best? 1: dis_align_size;
	int price_count = (e->match_len_limit > 36? 1013 : 4093);
	int price_counter = 0;		/* counters may decrement below 0 */
	int dis_price_counter = 0;
	int align_price_counter = 0;
	int ahead, i;
	int reps[num_rep_distances];
	State state = 0;

	for (i = 0; i < num_rep_distances; ++i)
		reps[i] = 0;

	if (Mb_data_position(&e->eb.mb) != 0 ||
	    Re_member_position(&e->eb.renc) != Fh_size)
		return false;			/* can be called only once */

	if (!Mb_data_finished(&e->eb.mb)) {	/* encode first byte */
		uchar prev_byte = 0;
		uchar cur_byte = Mb_peek(&e->eb.mb, 0);

		Re_encode_bit(&e->eb.renc, &e->eb.bm_match[state][0], 0);
		LZeb_encode_literal(&e->eb, prev_byte, cur_byte);
		CRC32_update_byte(&e->eb.crc, cur_byte);
		LZe_get_match_pairs(e, 0);
		Mb_move_pos(&e->eb.mb);
	}

	while (!Mb_data_finished(&e->eb.mb)) {
		if (price_counter <= 0 && e->pending_num_pairs == 0) {
			/* recalculate prices every these many bytes */
			price_counter = price_count;
			if (dis_price_counter <= 0) {
				dis_price_counter = dis_price_count;
				LZe_update_distance_prices(e);
			}
			if (align_price_counter <= 0) {
				align_price_counter = align_price_count;
				for (i = 0; i < dis_align_size; ++i)
					e->align_prices[i] = price_symbol_reversed(
						e->eb.bm_align, i, dis_align_bits);
			}
			Lp_update_prices(&e->match_len_prices);
			Lp_update_prices(&e->rep_len_prices);
		}

		ahead = LZe_sequence_optimizer(e, reps, state);
		price_counter -= ahead;

		for (i = 0; ahead > 0;) {
			int pos_state = (Mb_data_position(&e->eb.mb) - ahead) &
				pos_state_mask;
			int len = e->trials[i].price;
			int dis = e->trials[i].dis4;
			bool bit = (dis < 0);

			Re_encode_bit(&e->eb.renc, &e->eb.bm_match[state][pos_state],
				!bit);
			if (bit) {			/* literal byte */
				uchar prev_byte = Mb_peek(&e->eb.mb, ahead+1);
				uchar cur_byte = Mb_peek(&e->eb.mb, ahead);

				CRC32_update_byte(&e->eb.crc, cur_byte);
				if (St_is_char(state))
					LZeb_encode_literal(&e->eb, prev_byte,
						cur_byte);
				else {
					uchar match_byte = Mb_peek(&e->eb.mb,
						ahead + reps[0] + 1);

					LZeb_encode_matched(&e->eb, prev_byte,
						cur_byte, match_byte);
				}
				state = St_set_char(state);
			} else {		/* match or repeated match */
				CRC32_update_buf(&e->eb.crc,
					Mb_ptr_to_current_pos(&e->eb.mb) - ahead,
					len);
				mtf_reps(dis, reps);
				bit = (dis < num_rep_distances);
				Re_encode_bit(&e->eb.renc, &e->eb.bm_rep[state],
					bit);
				if (bit) 		 /* repeated match */
					state = encrepmatch(e, state, len, dis,
						pos_state);
				else {			/* match */
					dis -= num_rep_distances;
					LZeb_encode_pair(&e->eb, dis, len,
						pos_state);
					if (dis >= modeled_distances)
						--align_price_counter;
					--dis_price_counter;
					Lp_decr_counter(
						&e->match_len_prices, pos_state);
					state = St_set_match(state);
				}
			}
			ahead -= len;
			i += len;
			if (Re_member_position(&e->eb.renc) >= member_size_limit) {
				if (!Mb_dec_pos(&e->eb.mb, ahead))
					return false;
				LZeb_full_flush(&e->eb, state);
				return true;
			}
		}
	}
	LZeb_full_flush(&e->eb, state);
	return true;
}

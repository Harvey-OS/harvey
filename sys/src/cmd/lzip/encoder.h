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

typedef struct Len_prices Len_prices;
struct Len_prices {
	struct Len_model *lm;
	int	len_syms;
	int	count;
	int	prices[pos_states][max_len_syms];
	int	counters[pos_states];		/* may decrement below 0 */
};

static void
Lp_update_low_mid_prices(Len_prices *lp, int pos_state)
{
	int	*pps = lp->prices[pos_state];
	int	tmp = price0(lp->lm->choice1);
	int	len = 0;
	for (; len < len_low_syms && len < lp->len_syms; ++len)
		pps[len] = tmp + price_symbol3(lp->lm->bm_low[pos_state], len);
	if (len >= lp->len_syms)
		return;
	tmp = price1(lp->lm->choice1) + price0(lp->lm->choice2);
	for (; len < len_low_syms + len_mid_syms && len < lp->len_syms; ++len)
		pps[len] = tmp +
		    price_symbol3(lp->lm->bm_mid[pos_state], len - len_low_syms);
}

static void
Lp_update_high_prices(Len_prices *lp)
{
	int tmp = price1(lp->lm->choice1) + price1(lp->lm->choice2);
	int	len;
	for (len = len_low_syms + len_mid_syms; len < lp->len_syms; ++len)
		/* using 4 slots per value makes "Lp_price" faster */
		lp->prices[3][len] = lp->prices[2][len] =
		    lp->prices[1][len] = lp->prices[0][len] = tmp +
		    price_symbol8(lp->lm->bm_high, len - len_low_syms - len_mid_syms);
}

static void
Lp_reset(Len_prices *lp)
{
	int	i;
	for (i = 0; i < pos_states; ++i)
		lp->counters[i] = 0;
}

static void
Lp_init(Len_prices *lp, Len_model *lm, int match_len_limit)
{
	lp->lm = lm;
	lp->len_syms = match_len_limit + 1 - min_match_len;
	lp->count = (match_len_limit > 12) ? 1 : lp->len_syms;
	Lp_reset(lp);
}

static void
Lp_decr_counter(Len_prices *lp, int pos_state)
{
	--lp->counters[pos_state];
}

static void
Lp_update_prices(Len_prices *lp)
{
	int	pos_state;
	bool high_pending = false;

	for (pos_state = 0; pos_state < pos_states; ++pos_state)
		if (lp->counters[pos_state] <= 0) {
			lp->counters[pos_state] = lp->count;
			Lp_update_low_mid_prices(lp, pos_state);
			high_pending = true;
		}
	if (high_pending && lp->len_syms > len_low_syms + len_mid_syms)
		Lp_update_high_prices(lp);
}

typedef struct Pair Pair;
struct Pair {			/* distance-length pair */
	int	dis;
	int	len;
};

enum {
	infinite_price = 0x0FFFFFFF,
	max_num_trials = 1 << 13,
	single_step_trial = -2,
	dual_step_trial = -1
};

typedef struct Trial Trial;
struct Trial {
	State	state;
	int	price;	/* dual use var; cumulative price, match length */
	int	dis4;	/* -1 for literal, or rep, or match distance + 4 */
	int	prev_index;	/* index of prev trial in trials[] */
	int	prev_index2;	/*   -2  trial is single step */
	/*   -1  literal + rep0 */
	/* >= 0  (rep or match) + literal + rep0 */
	int	reps[num_rep_distances];
};

static void
Tr_update2(Trial *trial, int pr, int p_i)
{
	if (pr < trial->price) {
		trial->price = pr;
		trial->dis4 = 0;
		trial->prev_index = p_i;
		trial->prev_index2 = dual_step_trial;
	}
}

static void
Tr_update3(Trial *trial, int pr, int distance4, int p_i, int p_i2)
{
	if (pr < trial->price) {
		trial->price = pr;
		trial->dis4 = distance4;
		trial->prev_index = p_i;
		trial->prev_index2 = p_i2;
	}
}

typedef struct LZ_encoder LZ_encoder;
struct LZ_encoder {
	LZ_encoder_base eb;
	int	cycles;
	int	match_len_limit;
	Len_prices match_len_prices;
	Len_prices rep_len_prices;
	int	pending_num_pairs;
	Pair	pairs[max_match_len+1];
	Trial	trials[max_num_trials];

	int	dis_slot_prices[len_states][2*max_dict_bits];
	int	dis_prices[len_states][modeled_distances];
	int	align_prices[dis_align_size];
	int	num_dis_slots;
};

static bool
Mb_dec_pos(struct Matchfinder_base *mb, int ahead)
{
	if (ahead < 0 || mb->pos < ahead)
		return false;
	mb->pos -= ahead;
	if (mb->cyclic_pos < ahead)
		mb->cyclic_pos += mb->dict_size + 1;
	mb->cyclic_pos -= ahead;
	return true;
}

int	LZe_get_match_pairs(struct LZ_encoder *e, struct Pair *pairs);

/* move-to-front dis in/into reps; do nothing if(dis4 <= 0) */
static void
mtf_reps(int dis4, int reps[num_rep_distances])
{
	if (dis4 >= num_rep_distances)			/* match */ {
		reps[3] = reps[2];
		reps[2] = reps[1];
		reps[1] = reps[0];
		reps[0] = dis4 - num_rep_distances;
	} else if (dis4 > 0)				/* repeated match */ {
		int distance = reps[dis4];
		int	i;
		for (i = dis4; i > 0; --i)
			reps[i] = reps[i-1];
		reps[0] = distance;
	}
}

static int
LZeb_price_shortrep(struct LZ_encoder_base *eb, State state, int pos_state)
{
	return price0(eb->bm_rep0[state]) + price0(eb->bm_len[state][pos_state]);
}

static int
LZeb_price_rep(struct LZ_encoder_base *eb, int rep, State state, int pos_state)
{
	int	price;
	if (rep == 0)
		return price0(eb->bm_rep0[state]) +
		    price1(eb->bm_len[state][pos_state]);
	price = price1(eb->bm_rep0[state]);
	if (rep == 1)
		price += price0(eb->bm_rep1[state]);
	else {
		price += price1(eb->bm_rep1[state]);
		price += price_bit(eb->bm_rep2[state], rep - 2);
	}
	return price;
}

static int
LZe_price_rep0_len(struct LZ_encoder *e, int len, State state, int pos_state)
{
	return LZeb_price_rep(&e->eb, 0, state, pos_state) +
	    Lp_price(&e->rep_len_prices, len, pos_state);
}

static int
LZe_price_pair(struct LZ_encoder *e, int dis, int len, int pos_state)
{
	int price = Lp_price(&e->match_len_prices, len, pos_state);
	int len_state = get_len_state(len);
	if (dis < modeled_distances)
		return price + e->dis_prices[len_state][dis];
	else
		return price + e->dis_slot_prices[len_state][get_slot(dis)] +
		    e->align_prices[dis & (dis_align_size - 1)];
}

static int
LZe_read_match_distances(struct LZ_encoder *e)
{
	int num_pairs = LZe_get_match_pairs(e, e->pairs);
	if (num_pairs > 0) {
		int len = e->pairs[num_pairs-1].len;
		if (len == e->match_len_limit && len < max_match_len)
			e->pairs[num_pairs-1].len =
			    Mb_true_match_len(&e->eb.mb, len, e->pairs[num_pairs-1].dis + 1);
	}
	return num_pairs;
}

static void
LZe_move_and_update(struct LZ_encoder *e, int n)
{
	while (true) {
		Mb_move_pos(&e->eb.mb);
		if (--n <= 0)
			break;
		LZe_get_match_pairs(e, 0);
	}
}

static void
LZe_backward(struct LZ_encoder *e, int cur)
{
	int	dis4 = e->trials[cur].dis4;
	while (cur > 0) {
		int prev_index = e->trials[cur].prev_index;
		struct Trial *prev_trial = &e->trials[prev_index];

		if (e->trials[cur].prev_index2 != single_step_trial) {
			prev_trial->dis4 = -1;					/* literal */
			prev_trial->prev_index = prev_index - 1;
			prev_trial->prev_index2 = single_step_trial;
			if (e->trials[cur].prev_index2 >= 0) {
				struct Trial *prev_trial2 = &e->trials[prev_index-1];
				prev_trial2->dis4 = dis4;
				dis4 = 0;			/* rep0 */
				prev_trial2->prev_index = e->trials[cur].prev_index2;
				prev_trial2->prev_index2 = single_step_trial;
			}
		}
		prev_trial->price = cur - prev_index;			/* len */
		cur = dis4;
		dis4 = prev_trial->dis4;
		prev_trial->dis4 = cur;
		cur = prev_index;
	}
}

enum {
	Nprevpos3 = 1 << 16,
	Nprevpos2 = 1 << 10
};

static bool
LZe_init(struct LZ_encoder *e, int dict_size, int len_limit, int ifd, int outfd)
{
	enum {
		before = max_num_trials,
		/* bytes to keep in buffer after pos */
		after_size = (2 *max_match_len) + 1,
		dict_factor = 2,
		Nprevpos23 = Nprevpos2 + Nprevpos3,
		pos_array_factor = 2 	
	};

	if (!LZeb_init(&e->eb, before, dict_size, after_size, dict_factor,
	    Nprevpos23, pos_array_factor, ifd, outfd))
		return false;
	e->cycles = (len_limit < max_match_len) ? 16 + (len_limit / 2) : 256;
	e->match_len_limit = len_limit;
	Lp_init(&e->match_len_prices, &e->eb.match_len_model, e->match_len_limit);
	Lp_init(&e->rep_len_prices, &e->eb.rep_len_model, e->match_len_limit);
	e->pending_num_pairs = 0;
	e->num_dis_slots = 2 * real_bits(e->eb.mb.dict_size - 1);
	e->trials[1].prev_index = 0;
	e->trials[1].prev_index2 = single_step_trial;
	return true;
}

static void
LZe_reset(struct LZ_encoder *e)
{
	LZeb_reset(&e->eb);
	Lp_reset(&e->match_len_prices);
	Lp_reset(&e->rep_len_prices);
	e->pending_num_pairs = 0;
}

bool	LZe_encode_member(struct LZ_encoder *e, uvlong member_size);

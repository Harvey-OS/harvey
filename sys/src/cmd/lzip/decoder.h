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

enum { rd_buffer_size = 16384 };

typedef struct LZ_decoder LZ_decoder;
typedef struct Range_decoder Range_decoder;

struct Range_decoder {
	uvlong partial_member_pos;
	uchar * buffer;	/* input buffer */
	int	pos;		/* current pos in buffer */
	int	stream_pos;	/* when reached, a new block must be read */
	uint32_t code;
	uint32_t range;
	int	infd;		/* input file descriptor */
	bool at_stream_end;
};

bool	Rd_read_block(Range_decoder *rdec);

static bool
Rd_init(Range_decoder *rdec, int ifd)
{
	rdec->partial_member_pos = 0;
	rdec->buffer = (uchar *)malloc(rd_buffer_size);
	if (!rdec->buffer)
		return false;
	rdec->pos = 0;
	rdec->stream_pos = 0;
	rdec->code = 0;
	rdec->range = 0xFFFFFFFFU;
	rdec->infd = ifd;
	rdec->at_stream_end = false;
	return true;
}

static void
Rd_free(Range_decoder *rdec)
{
	free(rdec->buffer);
}

static bool
Rd_finished(Range_decoder *rdec)
{
	return rdec->pos >= rdec->stream_pos && !Rd_read_block(rdec);
}

static uvlong
Rd_member_position(Range_decoder *rdec)
{
	return rdec->partial_member_pos + rdec->pos;
}

static void
Rd_reset_member_position(Range_decoder *rdec)
{
	rdec->partial_member_pos = 0;
	rdec->partial_member_pos -= rdec->pos;
}

static uchar
Rd_get_byte(Range_decoder *rdec)
{
	/* 0xFF avoids decoder error if member is truncated at EOS marker */
	if (Rd_finished(rdec))
		return 0xFF;
	return rdec->buffer[rdec->pos++];
}

static int Rd_read_data(Range_decoder *rdec, uchar *outbuf, int size)
{
	int sz = 0;

	while (sz < size && !Rd_finished(rdec)) {
		int rd, rsz = size - sz;
		int rpos = rdec->stream_pos - rdec->pos;

		if (rsz < rpos)
			rd = rsz;
		else
			rd = rpos;
		memcpy(outbuf + sz, rdec->buffer + rdec->pos, rd);
		rdec->pos += rd;
		sz += rd;
	}
	return sz;
}

static void
Rd_load(Range_decoder *rdec)
{
	int	i;
	rdec->code = 0;
	for (i = 0; i < 5; ++i)
		rdec->code = (rdec->code << 8) | Rd_get_byte(rdec);
	rdec->range = 0xFFFFFFFFU;
	rdec->code &= rdec->range;	/* make sure that first byte is discarded */
}

static void
Rd_normalize(Range_decoder *rdec)
{
	if (rdec->range <= 0x00FFFFFFU) {
		rdec->range <<= 8;
		rdec->code = (rdec->code << 8) | Rd_get_byte(rdec);
	}
}

static unsigned
Rd_decode(Range_decoder *rdec, int num_bits)
{
	unsigned	symbol = 0;
	int	i;
	for (i = num_bits; i > 0; --i) {
		bool bit;
		Rd_normalize(rdec);
		rdec->range >>= 1;
		/*    symbol <<= 1; */
		/*    if(rdec->code >= rdec->range) { rdec->code -= rdec->range; symbol |= 1; } */
		bit = (rdec->code >= rdec->range);
		symbol = (symbol << 1) + bit;
		rdec->code -= rdec->range & (0U - bit);
	}
	return symbol;
}

static unsigned
Rd_decode_bit(Range_decoder *rdec, Bit_model *probability)
{
	uint32_t bound;
	Rd_normalize(rdec);
	bound = (rdec->range >> bit_model_total_bits) * *probability;
	if (rdec->code < bound) {
		rdec->range = bound;
		*probability += (bit_model_total - *probability) >> bit_model_move_bits;
		return 0;
	} else {
		rdec->range -= bound;
		rdec->code -= bound;
		*probability -= *probability >> bit_model_move_bits;
		return 1;
	}
}

static unsigned
Rd_decode_tree3(Range_decoder *rdec, Bit_model bm[])
{
	unsigned	symbol = 1;
	symbol = (symbol << 1) | Rd_decode_bit(rdec, &bm[symbol]);
	symbol = (symbol << 1) | Rd_decode_bit(rdec, &bm[symbol]);
	symbol = (symbol << 1) | Rd_decode_bit(rdec, &bm[symbol]);
	return symbol & 7;
}

static unsigned
Rd_decode_tree6(Range_decoder *rdec, Bit_model bm[])
{
	unsigned	symbol = 1;
	symbol = (symbol << 1) | Rd_decode_bit(rdec, &bm[symbol]);
	symbol = (symbol << 1) | Rd_decode_bit(rdec, &bm[symbol]);
	symbol = (symbol << 1) | Rd_decode_bit(rdec, &bm[symbol]);
	symbol = (symbol << 1) | Rd_decode_bit(rdec, &bm[symbol]);
	symbol = (symbol << 1) | Rd_decode_bit(rdec, &bm[symbol]);
	symbol = (symbol << 1) | Rd_decode_bit(rdec, &bm[symbol]);
	return symbol & 0x3F;
}

static unsigned
Rd_decode_tree8(Range_decoder *rdec, Bit_model bm[])
{
	unsigned	symbol = 1;
	int	i;
	for (i = 0; i < 8; ++i)
		symbol = (symbol << 1) | Rd_decode_bit(rdec, &bm[symbol]);
	return symbol & 0xFF;
}

static unsigned
Rd_decode_tree_reversed(Range_decoder *rdec, Bit_model bm[], int num_bits)
{
	unsigned model = 1;
	unsigned symbol = 0;
	int	i;
	for (i = 0; i < num_bits; ++i) {
		unsigned bit = Rd_decode_bit(rdec, &bm[model]);
		model = (model << 1) + bit;
		symbol |= (bit << i);
	}
	return symbol;
}

static unsigned
Rd_decode_tree_reversed4(Range_decoder *rdec, Bit_model bm[])
{
	unsigned	symbol = Rd_decode_bit(rdec, &bm[1]);
	unsigned	model = 2 + symbol;
	unsigned	bit = Rd_decode_bit(rdec, &bm[model]);
	model = (model << 1) + bit;
	symbol |= (bit << 1);
	bit = Rd_decode_bit(rdec, &bm[model]);
	model = (model << 1) + bit;
	symbol |= (bit << 2);
	symbol |= (Rd_decode_bit(rdec, &bm[model]) << 3);
	return symbol;
}

static unsigned
Rd_decode_matched(Range_decoder *rdec, Bit_model bm[], unsigned match_byte)
{
	unsigned	symbol = 1;
	unsigned	mask = 0x100;
	while (true) {
		unsigned match_bit = (match_byte <<= 1) & mask;
		unsigned bit = Rd_decode_bit(rdec, &bm[symbol+match_bit+mask]);
		symbol = (symbol << 1) + bit;
		if (symbol > 0xFF)
			return symbol & 0xFF;
		mask &= ~(match_bit ^ (bit << 8));	/* if(match_bit != bit) mask = 0; */
	}
}

static unsigned
Rd_decode_len(struct Range_decoder *rdec, Len_model *lm, int pos_state)
{
	if (Rd_decode_bit(rdec, &lm->choice1) == 0)
		return Rd_decode_tree3(rdec, lm->bm_low[pos_state]);
	if (Rd_decode_bit(rdec, &lm->choice2) == 0)
		return len_low_syms + Rd_decode_tree3(rdec, lm->bm_mid[pos_state]);
	return len_low_syms + len_mid_syms + Rd_decode_tree8(rdec, lm->bm_high);
}

struct LZ_decoder {
	uvlong	partial_data_pos;
	struct Range_decoder *rdec;
	unsigned dict_size;
	uchar * buffer;		/* output buffer */
	unsigned pos;			/* current pos in buffer */
	unsigned stream_pos;		/* first byte not yet written to file */
	uint32_t crc;
	int	outfd;			/* output file descriptor */
	bool pos_wrapped;
};

void	LZd_flush_data(LZ_decoder *d);

static uchar
LZd_peek_prev(LZ_decoder *d)
{
	if (d->pos > 0)
		return d->buffer[d->pos-1];
	if (d->pos_wrapped)
		return d->buffer[d->dict_size-1];
	return 0;			/* prev_byte of first byte */
}

static uchar
LZd_peek(LZ_decoder *d,
unsigned distance)
{
	unsigned i = ((d->pos > distance) ? 0 : d->dict_size) +
	    d->pos - distance - 1;
	return d->buffer[i];
}

static void
LZd_put_byte(LZ_decoder *d, uchar b)
{
	d->buffer[d->pos] = b;
	if (++d->pos >= d->dict_size)
		LZd_flush_data(d);
}

static void
LZd_copy_block(LZ_decoder *d, unsigned distance, unsigned len)
{
	unsigned	lpos = d->pos, i = lpos -distance -1;
	bool fast, fast2;

	if (lpos > distance) {
		fast = (len < d->dict_size - lpos);
		fast2 = (fast && len <= lpos - i);
	} else {
		i += d->dict_size;
		fast = (len < d->dict_size - i); /* (i == pos) may happen */
		fast2 = (fast && len <= i - lpos);
	}
	if (fast)				/* no wrap */ {
		d->pos += len;
		if (fast2)			/* no wrap, no overlap */
			memcpy(d->buffer + lpos, d->buffer + i, len);
		else
			for (; len > 0; --len)
				d->buffer[lpos++] = d->buffer[i++];
	} else
		for (; len > 0; --len) {
			d->buffer[d->pos] = d->buffer[i];
			if (++d->pos >= d->dict_size)
				LZd_flush_data(d);
			if (++i >= d->dict_size)
				i = 0;
		}
}

static bool
LZd_init(struct LZ_decoder *d, Range_decoder *rde, unsigned dict_size, int ofd)
{
	d->partial_data_pos = 0;
	d->rdec = rde;
	d->dict_size = dict_size;
	d->buffer = (uchar *)malloc(d->dict_size);
	if (!d->buffer)
		return false;
	d->pos = 0;
	d->stream_pos = 0;
	d->crc = 0xFFFFFFFFU;
	d->outfd = ofd;
	d->pos_wrapped = false;
	return true;
}

static void
LZd_free(LZ_decoder *d)
{
	free(d->buffer);
}

static unsigned
LZd_crc(LZ_decoder *d)
{
	return d->crc ^ 0xFFFFFFFFU;
}

static uvlong
LZd_data_position(LZ_decoder *d)
{
	return d->partial_data_pos + d->pos;
}

int	LZd_decode_member(struct LZ_decoder *d, Pretty_print *pp);

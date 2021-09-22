/* included by expand and 9boot with different header files */

/*
 *  lunzip - decompressor for the LZMA lzip format
 *	(plan 9 in-memory boot decompressor version)
 *  Copyright (C) 2010-2017 Antonio Diaz Diaz.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define max(x,y) ((x) >= (y)? (x): (y))
#define min(x,y) ((x) <= (y)? (x): (y))

/* silliness */
typedef int bool;
enum { false, true };

typedef int	Bit_model;
typedef uchar	File_header[6];		/* 0-3 magic bytes */
typedef uchar	File_trailer[20];
typedef struct Input Input;
typedef struct Len_model Len_model;
typedef struct LZ_decoder LZ_decoder;
typedef char	State;

enum {
	/*
	 * Return values 0 = OK, 1 = decoder error, 2 = unexpected EOF,
	 *               3 = trailer error, 4 = unknown marker found.
	 *		-1 = perform a "continue;" in the caller.
	 *		-2 = just keep going in the caller.
	 */
	Retok,			/* order matters */
	Retdecerr,
	Reteoftoosoon,		
	Rettrailerr,
	Retbadmarker,
	Retcontin = -1,
	Retkeepon = -2,

	States = 12,
	Min_dict_bits = 12,
	Min_dict_size = 1 << Min_dict_bits, 	/* >= Modeled_distances */
	Max_dict_bits = 29,
	Max_dict_size = 1 << Max_dict_bits,
	Min_member_size = 36,
	Lit_context_bits = 3,
//	Lit_pos_state_bits = 0,
	Pos_state_bits = 2,
	Pos_states = 1 << Pos_state_bits,
	Pos_state_mask = Pos_states -1,

	Len_states = 4,
	Dis_slot_bits = 6,
	Start_dis_model = 4,
	End_dis_model = 14,
	Modeled_distances = 1 << (End_dis_model / 2), 	/* 128 */
	Dis_align_bits = 4,
	Dis_align_size = 1 << Dis_align_bits,

	Len_low_bits = 3,
	Len_mid_bits = 3,
	Len_high_bits = 8,
	Len_low_syms = 1 << Len_low_bits,
	Len_mid_syms = 1 << Len_mid_bits,
	Len_high_syms = 1 << Len_high_bits,
	Max_len_syms = Len_low_syms + Len_mid_syms + Len_high_syms,

	Min_match_len = 2, 				/* must be 2 */
	Max_match_len = Min_match_len + Max_len_syms -1, /* 273 */
	Min_match_len_limit = 5,

	Bit_model_move_bits = 5,
	Bit_model_total_bits = 11,
	Bit_model_total = 1 << Bit_model_total_bits,
};

struct Input {
	ulong	range;
	ulong	code;
	uchar	*buffer;	/* input buffer */
	int	pos;		/* current pos in buffer */
	int	stream_pos;	/* end of buffer */
	bool	at_stream_end;
	/* unused in practice, I believe */
	uvlong	partial_member_pos;
};

struct LZ_decoder {
	uchar	*buffer;	/* output buffer */
	Input	*rdec;
	unsigned dict_size;
	unsigned buffer_size;
	unsigned pos;		/* current pos in buffer */
	unsigned stream_pos;	/* first byte not yet written to file */
	ulong	crc;
	ulong	crcset;		/* flag */
	/* unused in practice, I believe */
	bool	pos_wrapped;
	bool	pos_wrapped_dic;
	uvlong	partial_data_pos;
};

struct Len_model {
	Bit_model choice1;
	Bit_model choice2;
	Bit_model bm_low[Pos_states][Len_low_syms];
	Bit_model bm_mid[Pos_states][Len_mid_syms];
	Bit_model bm_high[Len_high_syms];
};

/*   4 version */
/*   5 coded_dict_size */
enum { Fh_size = 6 };

/*  0-3  CRC32 of the uncompressed data */
/*  4-11 size of the uncompressed data */
/* 12-19 member size including header and trailer */
enum { Ft_size = 20 };

char *argv0 = "lunzip";

static ulong *crc32;		/* Table of CRCs of all 8-bit messages. */
static int verbosity;
static ulong rd_buffer_size = 1 << 26;

static char *bad_dict_msg = "bad dict size in member header";
static uchar magic_string[4] = { 'L', 'Z', 'I', 'P' };
static char overfull[] = "overfilling output buffer";
static char seekerr[] = "seek error";

#define Rd_normalize(rdec) \
{ \
	if ((rdec)->range <= ((1<<24) - 1)) \
		(rdec)->range <<= 8, \
		(rdec)->code = ((rdec)->code << 8) | Rd_get_byte(rdec); \
}
#define Rd_finished(rdec) ((rdec)->pos >= (rdec)->stream_pos)
#define Rd_member_position(rdec) ((rdec)->partial_member_pos + (rdec)->pos)
/* 0xFF avoids decoder error if member is truncated at EOS marker */
#define Rd_get_byte(rd) (Rd_finished(rd)? 0xFF: (rd)->buffer[(rd)->pos++])

#define LZd_data_position(d) ((d)->partial_data_pos + (d)->pos)
#define LZd_put_byte(d, b) \
{ \
	(d)->buffer[(d)->pos++] = (b); \
	if ((d)->pos >= (d)->buffer_size) \
		panic(overfull); \
}

#define St_is_char(st) ((st) < 7)
#define St_set_match(st) ((st) < 7? 7: 10)
#define get_lit_state(prevbyte) ((uchar)(prevbyte) >> (8 - Lit_context_bits))

void	LZd_flush_data(LZ_decoder *d);
unsigned seek_read_back(LZ_decoder *, uchar *, int, vlong);

/* set up input from memory only. */
static bool
Rd_init(Input *rdec, uchar *inlunzip, unsigned insz)
{
	rdec->buffer = inlunzip;
	rdec->stream_pos = rd_buffer_size = insz;
	rdec->pos = 0;
	rdec->partial_member_pos = 0;
	rdec->code = 0;
	rdec->range = ~0ul;
	rdec->at_stream_end = false;
	return true;
}

static void
Rd_reset_member_position(Input *rdec)
{
	rdec->partial_member_pos = 0;
	rdec->partial_member_pos -= rdec->pos;
}

static int
Rd_read_data(Input *rdec, uchar *outbuf, int size)
{
	int sz = 0;

	while (sz < size && !Rd_finished(rdec)) {
		int rd = min(size - sz, rdec->stream_pos - rdec->pos);

		memmove(outbuf + sz, rdec->buffer + rdec->pos, rd);
		rdec->pos += rd;
		sz += rd;
	}
	return sz;
}

static void
Rd_load(Input *rdec)
{
	int i;

	rdec->code = 0;
	for (i = 0; i < 5; ++i)
		rdec->code = (rdec->code << 8) | Rd_get_byte(rdec);
	rdec->range = (ulong)~0ul;
	rdec->code &= rdec->range; /* make sure that first byte is discarded */
}

static unsigned
Rd_decode(Input *rdec, int num_bits)
{
	unsigned symbol = 0;
	int i;

	for (i = num_bits; i > 0; --i) {
		bool bit;

		Rd_normalize(rdec);
		rdec->range >>= 1;
		bit = (rdec->code >= rdec->range);
		symbol = (symbol << 1) + bit;
		rdec->code -= rdec->range & (0U - bit);
	}
	return symbol;
}

/*
 * this is a profiling hot-spot.
 * we expand Rd_normalize in-line here for speed.
 */
static unsigned
Rd_decode_bit(Input *rdec, Bit_model *probability)
{
	Bit_model prob;
	ulong bound, code, range;

	range = rdec->range;
	code  = rdec->code;
	if (range <= ((1<<24) - 1)) {
		range <<= 8;
		code = (code << 8) | Rd_get_byte(rdec);
	}

	prob = *probability;
	bound = (range >> Bit_model_total_bits) * prob;
	if (code < bound) {
		rdec->range = bound;
		rdec->code  = code;
		*probability += (Bit_model_total - prob) >> Bit_model_move_bits;
		return 0;
	} else {
		rdec->range = range - bound;
		rdec->code  = code  - bound;
		*probability -= prob >> Bit_model_move_bits;
		return 1;
	}
}

static unsigned
Rd_decode_tree3(Input *rdec, Bit_model bm[])
{
	int i;
	unsigned symbol = 1;

	for (i = 3; i-- > 0; )
		symbol = (symbol << 1) | Rd_decode_bit(rdec, &bm[symbol]);
	return symbol & ((1<<3)-1);
}

static unsigned
Rd_decode_tree6(Input *rdec, Bit_model bm[])
{
	int i;
	unsigned symbol = 1;

	for (i = 6; i-- > 0; )
		symbol = (symbol << 1) | Rd_decode_bit(rdec, &bm[symbol]);
	return symbol & ((1<<6)-1);
}

static unsigned
Rd_decode_tree8(Input *rdec, Bit_model bm[])
{
	int i;
	unsigned symbol = 1;

	for (i = 8; i-- > 0; )
		symbol = (symbol << 1) | Rd_decode_bit(rdec, &bm[symbol]);
	return symbol & ((1<<8)-1);
}

static unsigned
Rd_decode_tree_reversed(Input *rdec, Bit_model bm[], int num_bits)
{
	unsigned model = 1, symbol = 0;
	int	i;

	for (i = 0; i < num_bits; ++i) {
		unsigned bit = Rd_decode_bit(rdec, &bm[model]);

		model = (model << 1) + bit;
		symbol |= (bit << i);
	}
	return symbol;
}

static unsigned
Rd_decode_tree_reversed4(Input *rdec, Bit_model bm[])
{
	unsigned symbol = Rd_decode_bit(rdec, &bm[1]);
	unsigned model = 2 + symbol;
	unsigned bit = Rd_decode_bit(rdec, &bm[model]);

	model = (model << 1) + bit;
	symbol |= (bit << 1);
	bit = Rd_decode_bit(rdec, &bm[model]);
	model = (model << 1) + bit;
	symbol |= (bit << 2);
	symbol |= (Rd_decode_bit(rdec, &bm[model]) << 3);
	return symbol;
}

static unsigned
Rd_decode_matched(Input *rdec, Bit_model bm[], unsigned match_byte)
{
	unsigned symbol = 1, mask = 0x100;

	for (;;) {
		unsigned match_bit = (match_byte <<= 1) & mask;
		unsigned bit = Rd_decode_bit(rdec, &bm[symbol+match_bit+mask]);

		symbol = (symbol << 1) | bit;
		if (symbol > 0xFF)
			return symbol & 0xFF;
		mask &= ~(match_bit ^ (bit << 8));
		/* if(match_bit != bit) mask = 0; */
	}
}

static unsigned
Rd_decode_len(Input *rdec, Len_model *lm, int pos_state)
{
	if (Rd_decode_bit(rdec, &lm->choice1) == 0)
		return Rd_decode_tree3(rdec, lm->bm_low[pos_state]);
	if (Rd_decode_bit(rdec, &lm->choice2) == 0)
		return Len_low_syms + Rd_decode_tree3(rdec,
			lm->bm_mid[pos_state]);
	return Len_low_syms + Len_mid_syms + Rd_decode_tree8(rdec,
		lm->bm_high);
}

static uchar
LZd_peek_prev(LZ_decoder *d)
{
	if (d->pos > 0)
		return d->buffer[d->pos-1];
	if (d->pos_wrapped)
		return d->buffer[d->buffer_size-1];
	return 0;			/* prev_byte of first byte */
}

/* read back something we wrote earlier */
static uchar
LZd_peek(LZ_decoder *d, unsigned distance)
{
	uchar b;

	if (d->pos > distance)
		b = d->buffer[d->pos-distance - 1];
	else if (d->buffer_size > distance)
		b = d->buffer[d->buffer_size + d->pos-distance - 1];
	else if (seek_read_back(d, &b, 1, distance + 1 + d->stream_pos - d->pos)
	    != 1)
		panic(seekerr);
	return b;
}

static void
LZd_copy_block(LZ_decoder *d, unsigned distance, unsigned len)
{
	bool fast, fast2;
	unsigned lpos = d->pos, i = lpos - distance - 1;
	unsigned bufsz = d->buffer_size;
	uchar *buf = d->buffer;

	if (lpos > distance) {
		fast = (len < bufsz - lpos);
		fast2 = (fast && len <= lpos - i);
	} else {
		i += bufsz;
		fast = (len < bufsz - i); /* (i == pos) may happen */
		fast2 = (fast && len <= i - lpos);
	}
	if (fast) {				/* no wrap */
		d->pos += len;
		if (fast2)			/* no wrap, no overlap */
			memmove(buf + lpos, buf + i, len);
		else
			while (len-- > 0)
				buf[lpos++] = buf[i++];
	} else
		while (len-- > 0) {
			buf[d->pos++] = buf[i++];
			if (d->pos >= bufsz)
				LZd_flush_data(d);
			if (i >= bufsz)
				i = 0;
		}
}

static void
LZd_copy_block2(LZ_decoder *d, unsigned distance, unsigned len)
{
	if (d->buffer_size > distance) {	/* block is in buffer */
		LZd_copy_block(d, distance, len);
		return;
	}
	if (len < d->buffer_size - d->pos) {	/* no wrap */
		unsigned offset = distance + 1 + d->stream_pos - d->pos;

		if (len <= offset)		/* block is in file */ {
			if (seek_read_back(d, d->buffer + d->pos, len, offset)
			    != len)
				panic(seekerr);
			d->pos += len;
			return;
		}
	}
	while (len-- > 0)
		LZd_put_byte(d, LZd_peek(d, distance));
}

/* output buffer needs to be quite a lot bigger than input buffer. */
static bool
LZd_init(LZ_decoder *d, Input *rde, char *outbuf, unsigned outbufsz,
	unsigned dict_size)
{
	d->rdec = rde;			/* input stream */
	d->dict_size = dict_size;
	d->buffer_size = max(outbufsz, dict_size);
	d->buffer = (void *)outbuf;
	d->pos = 0;
	d->stream_pos = 0;
	d->partial_data_pos = 0;
	d->crc = ~0ul;
	d->crcset = 0;
	d->pos_wrapped = d->pos_wrapped_dic = false;
	return true;
}

static ulong
LZd_crc(LZ_decoder *d)
{
	return d->crc ^ (ulong)~0ul;
}

static State
St_set_char(State st)
{
	static State next[States] = { 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 4, 5 };

	return next[st];
}

static State
St_set_rep(State st)
{
	return ((st < 7) ? 8 : 11);
}

static State
St_set_short_rep(State st)
{
	return ((st < 7) ? 9 : 11);
}

static int
get_len_state(int len)
{
	return min(len - Min_match_len, Len_states - 1);
}

static void
Bm_init(Bit_model *probability)
{
	*probability = Bit_model_total / 2;
}

static void
Bm_array_init(Bit_model bm[], int size)
{
	int i;

	for (i = 0; i < size; ++i)
		Bm_init(&bm[i]);
}

void
Lm_init(Len_model *lm)
{
	Bm_init(&lm->choice1);
	Bm_init(&lm->choice2);
	Bm_array_init(lm->bm_low[0], Pos_states * Len_low_syms);
	Bm_array_init(lm->bm_mid[0], Pos_states * Len_mid_syms);
	Bm_array_init(lm->bm_high, Len_high_syms);
}

bool
isvalid_ds(unsigned dict_size)
{
	return dict_size >= Min_dict_size && dict_size <= Max_dict_size;
}

static bool
Fh_verify_magic(File_header data)
{
	return memcmp(data, magic_string, 4) == 0;
}

static unsigned
Fh_get_dict_size(File_header data)
{
	unsigned sz = 1 << (data[5] & 0x1F);

	if (sz > Min_dict_size)
		sz -= (sz / 16) * ((data[5] >> 5) & 7);
	return sz;
}

/* detect truncated header */
static bool
Fh_verify_prefix(File_header data, int size)
{
	int i;

	for (i = 0; i < size && i < 4; ++i)
		if (data[i] != magic_string[i])
			return false;
	return size > 0;
}

static uchar
Fh_version(File_header data)
{
	return data[4];
}

static bool
Fh_verify_version(File_header data)
{
	return data[4] == 1;
}

static unsigned
Ft_get_data_crc(File_trailer data)
{
	unsigned tmp = 0;
	int i;

	for (i = 3; i >= 0; --i) {
		tmp <<= 8;
		tmp += data[i];
	}
	return tmp;
}

static uvlong
Ft_get_data_size(File_trailer data)
{
	uvlong tmp = 0;
	int i;

	for (i = 11; i >= 4; --i) {
		tmp <<= 8;
		tmp += data[i];
	}
	return tmp;
}

uvlong
Ft_get_member_size(File_trailer data)
{
	uvlong tmp = 0;
	int i;

	for (i = 19; i >= 12; --i) {
		tmp <<= 8;
		tmp += data[i];
	}
	return tmp;
}

void
CRC32_init(void)
{
	unsigned n, c;
	int k;

	if (crc32 == nil)
		crc32 = malloc(256 * sizeof(ulong));
	for (n = 0; n < 256; ++n) {
		c = n;
		for (k = 0; k < 8; ++k) {
			if (c & 1)
				c = 0xEDB88320U ^ (c >> 1);
			else
				c >>= 1;
		}
		crc32[n] = c;
	}
}

void
CRC32_update_buf(ulong *crc, uchar *buffer, int size)
{
	int i;
	ulong c = *crc;

	for (i = 0; i < size; ++i)
		c = crc32[(c ^ buffer[i]) & 0xFF] ^ (c >> 8);
	*crc = c;
}

int
readblock(Input *d, uchar *buf, int size)
{
	int left;

	left = d->stream_pos - d->pos;
	if (size > left)
		size = left;
	memmove(buf, &d->buffer[d->pos], size);
	d->pos += size;
	print("lunzip: readblock called!\n");
	return size;
}

static int
writeblock(LZ_decoder *d, uchar *buf, int size)
{
	int left;

	left = d->stream_pos - d->pos;
	if (size > left)
		size = left;
	memmove(&d->buffer[d->pos], buf, size);
	d->pos += size;
	print("lunzip: writeblock called!\n");
	return size;
}

/* read back something we wrote earlier */
unsigned
seek_read_back(LZ_decoder *d, uchar *buf, int size, vlong offset)
{
	int left;

	d->pos = d->stream_pos - offset;
	left = d->stream_pos - d->pos;
	if (size > left)
		size = left;
	memmove(buf, &d->buffer[d->pos], size);
	d->pos += size;
	print("lunzip: seek_read_back called!\n");
	return size;
}

bool
Rd_read_block(Input *rdec)
{
	char errbuf[1];

	print("lunzip: Rd_read_block called!\n");
	if (!rdec->at_stream_end) {
		rdec->stream_pos = readblock(rdec, rdec->buffer, rd_buffer_size);
		errbuf[0] = '\0';
		if (rdec->stream_pos != rd_buffer_size && errbuf[0]) {
			print("%s: read error\n", argv0);
			_exits("err");
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
	if (!d->crcset) {
		CRC32_update_buf(&d->crc, d->buffer, d->pos);
		d->crcset = 1;
	}
	if (d->pos >= d->buffer_size) {
		d->partial_data_pos += d->pos;
		d->pos = 0;
		d->pos_wrapped = true;
		if (d->partial_data_pos >= d->dict_size)
			d->pos_wrapped_dic = true;
	}
	d->stream_pos = d->pos;
}

static void
prmismatch(char *type, long trsize, long actsize)
{
	if (verbosity >= 0)
		print("%s size mismatch; trailer says %lud, %s size is %lud (%#lux)\n",
			type, trsize, type, actsize, actsize);
}

static bool
LZd_verify_trailer(LZ_decoder *d)
{
	File_trailer trailer;
	int size = Rd_read_data(d->rdec, trailer, Ft_size);
	uvlong data_size = LZd_data_position(d);
	uvlong member_size = Rd_member_position(d->rdec);
	bool error = false;

	if (size < Ft_size) {
		error = true;
		if (verbosity >= 0)
			print("Trailer truncated at trailer position %d;"
			    " some checks may fail.\n", size );
		while (size < Ft_size)
			trailer[size++] = 0;
	}

	LZd_flush_data(d);
	if (!d->crcset)
		print("crc not computed!\n");
	if (Ft_get_data_crc(trailer) != LZd_crc(d)) {
		error = true;
		if (verbosity >= 0)
			print("CRC mismatch: trailer crc %X != computed CRC %X\n",
				Ft_get_data_crc(trailer), (int)LZd_crc(d));
	}
	if (Ft_get_data_size(trailer) != data_size) {
		error = true;
		prmismatch("data", Ft_get_data_size(trailer), data_size);
	}
	if (Ft_get_member_size(trailer) != member_size) {
		error = true;
		prmismatch("member", Ft_get_member_size(trailer), member_size);
	}
	return !error;
}

static int
decodematch(LZ_decoder *d, int pos_state, Len_model *match_len_model,
	Bit_model bm_dis_slot[Len_states][1<<Dis_slot_bits],
	Bit_model *bm_dis, Bit_model *bm_align, unsigned *distp, int *lenp)
{
	int len, direct_bits;
	unsigned dis_slot, distance;
	Input *rdec = d->rdec;

	*lenp = len = Min_match_len + Rd_decode_len(rdec, match_len_model,
		pos_state);
	*distp = distance = Rd_decode_tree6(rdec, bm_dis_slot[get_len_state(len)]);
	if (distance < Start_dis_model)
		return Retkeepon;

	dis_slot = distance;
	direct_bits = (dis_slot >> 1) - 1;
	distance = (2 | (dis_slot & 1)) << direct_bits;
	if (dis_slot < End_dis_model) {
		distance += Rd_decode_tree_reversed(rdec, bm_dis +
			(distance - dis_slot), direct_bits);
		*distp = distance;
		return Retkeepon;
	}
	distance += Rd_decode(rdec, direct_bits - Dis_align_bits) <<
		Dis_align_bits;
	distance += Rd_decode_tree_reversed4(rdec, bm_align);
	*distp = distance;
	if (distance != (ulong)~0ul)
		return Retkeepon;

	/* marker found */
	Rd_normalize(rdec);
	LZd_flush_data(d);
	if (len == Min_match_len)	/* End Of Stream marker */
		if (LZd_verify_trailer(d))
			return Retok;
		else
			return Rettrailerr;
	if (len == Min_match_len + 1) {	/* Sync Flush marker */
		Rd_load(rdec);
		return Retcontin;	/* caller must "continue;" */
	}
	if (verbosity >= 0)
		print("Unsupported marker code '%d'\n", len);
	return Retbadmarker;
}

int
LZd_decode_member(LZ_decoder *d)
{
	unsigned rep0 = 0;		/* rep[0-3] latest four distances */
	unsigned rep1 = 0;		/* used for efficient coding of */
	unsigned rep2 = 0;		/* repeated distances */
	unsigned rep3 = 0;
	State state = 0;
	Input *rdec = d->rdec;
	void (*copy_block)(LZ_decoder *d, unsigned distance, unsigned len) =
	    (d->buffer_size >= d->dict_size? LZd_copy_block: LZd_copy_block2);
	Bit_model bm_literal[1<<Lit_context_bits][0x300];
	Bit_model bm_match[States][Pos_states];
	Bit_model bm_rep[States], bm_rep0[States], bm_rep1[States], bm_rep2[States];
	Bit_model bm_len[States][Pos_states];
	Bit_model bm_dis_slot[Len_states][1<<Dis_slot_bits];
	Bit_model bm_dis[Modeled_distances-End_dis_model+1];
	Bit_model bm_align[Dis_align_size];
	Len_model match_len_model, rep_len_model;

	Bm_array_init(bm_literal[0], (1 << Lit_context_bits) * 0x300);
	Bm_array_init(bm_match[0], States * Pos_states);
	Bm_array_init(bm_rep, States);
	Bm_array_init(bm_rep0, States);
	Bm_array_init(bm_rep1, States);
	Bm_array_init(bm_rep2, States);
	Bm_array_init(bm_len[0], States * Pos_states);
	Bm_array_init(bm_dis_slot[0], Len_states * (1 << Dis_slot_bits));
	Bm_array_init(bm_dis, Modeled_distances - End_dis_model + 1);
	Bm_array_init(bm_align, Dis_align_size);
	Lm_init(&match_len_model);
	Lm_init(&rep_len_model);

	Rd_load(rdec);
	while (!Rd_finished(rdec)) {
		int len, rv, pos_state;
		unsigned distance;

		pos_state = LZd_data_position(d) & Pos_state_mask;
		/* 1st bit */
		if (Rd_decode_bit(rdec, &bm_match[state][pos_state]) == 0) {
			Bit_model *bm = bm_literal[get_lit_state(
				LZd_peek_prev(d))];

			if (St_is_char(state)) {
				state -= state < 4? state: 3;
				LZd_put_byte(d, Rd_decode_tree8(rdec, bm));
			} else {
				state -= state < 10? 3: 6;
				LZd_put_byte(d, Rd_decode_matched(rdec, bm,
					LZd_peek(d, rep0)));
			}
			continue;
		}

		/* match or repeated match */
		if (Rd_decode_bit(rdec, &bm_rep[state]) != 0) {	/* 2nd bit */
			/* 3rd bit */
			if (Rd_decode_bit(rdec, &bm_rep0[state]) == 0) {
				/* 4th bit */
				if (Rd_decode_bit(rdec, &bm_len[
				    state][pos_state]) == 0) {
					state = St_set_short_rep(state);
					LZd_put_byte(d, LZd_peek(d, rep0));
					continue;
				}
			} else {
				/* 4th bit */
				if (Rd_decode_bit(rdec, &bm_rep1[state]) == 0)
					distance = rep1;
				else {
					/* 5th bit */
					if (Rd_decode_bit(rdec, &bm_rep2[state])
					    == 0)
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
			len = Min_match_len + Rd_decode_len(rdec,
				&rep_len_model, pos_state);
		} else {				/* match */
			rv = decodematch(d, pos_state, &match_len_model,
				bm_dis_slot, bm_dis, bm_align, &distance, &len);
			if (rv >= Retok)
				return rv;
			if (rv == Retcontin)
				continue;
			rep3 = rep2;
			rep2 = rep1;
			rep1 = rep0;
			rep0 = distance;
			state = St_set_match(state);
			if (rep0 >= d->dict_size ||
			    (rep0 >= LZd_data_position(d) &&
			     !d->pos_wrapped_dic)) {
				LZd_flush_data(d);
				return Retdecerr;
			}
		}
		copy_block(d, rep0, len);
	}
	LZd_flush_data(d);
	return Reteoftoosoon;
}

static void
prnomem(void)
{
	print("%s: out of memory\n", argv0);
}

int
decompress(uchar *outbuf, unsigned outbufsz, uchar *inlunzip, unsigned inbufsz)
{
	int result, size, rv = Retok;
	bool first_member;
	unsigned dict_size;
	uvlong partial_file_pos = 0;
	File_header header;
	LZ_decoder decoder;
	Input rdec;

	USED(outbufsz);
	if (!Rd_init(&rdec, inlunzip, inbufsz)) {
		prnomem();
		_exits("err");
	}

	for (first_member = true; ; first_member = false) {
		Rd_reset_member_position(&rdec);
		size = Rd_read_data(&rdec, header, Fh_size);
		if (Rd_finished(&rdec)) {		/* End Of File */
			if (first_member || Fh_verify_prefix(header, size)) {
				print("File ends unexpectedly at member header.");
				rv = Reteoftoosoon;
			} else if (size > 0)
				rv = Reteoftoosoon;
			break;
		}
		if (!Fh_verify_magic(header)) {
			if (first_member) {
				print("%s: bad magic; not lzip format\n", argv0);
				rv = Reteoftoosoon;
			} else {
				/*
				 * this is normal; we don't know how big the
				 * lzipped kernel is.
				 */
				// print("%s: trailing garbage\n", argv0);
			}
			break;
		}
		if (!Fh_verify_version(header)) {
			print("lzip version %d member format not supported.\n",
				Fh_version(header));
			rv = Reteoftoosoon;
			break;
		}
		dict_size = Fh_get_dict_size(header);
		if (!isvalid_ds(dict_size)) {
			print("%s", bad_dict_msg);
			rv = Reteoftoosoon;
			break;
		}

		if (!LZd_init(&decoder, &rdec, (void *)outbuf, inbufsz, dict_size)) {
			prnomem();
			rv = Retdecerr;
			break;
		}

		result = LZd_decode_member(&decoder);
		partial_file_pos += Rd_member_position(&rdec);
		if (result != Retok) {
			if (verbosity >= 0 && result <= Reteoftoosoon)
				print("%s at pos %lud\n",
					(result == Reteoftoosoon?
				    "File ends unexpectedly": "Decoder error"),
					(long)partial_file_pos);
			rv = Retok;
			break;
		}
	}
	return rv;
}

int
lunzip(uchar *out, int outn, uchar *in, int inn)
{
	CRC32_init();
	if (decompress(out, outn, in, inn) != Retok)
		return -1;
	return inn;
}

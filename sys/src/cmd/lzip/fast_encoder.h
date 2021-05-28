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

typedef struct FLZ_encoder FLZ_encoder;
struct FLZ_encoder {
	struct LZ_encoder_base eb;
	unsigned key4;			/* key made from latest 4 bytes */
};

static void
FLZe_reset_key4(FLZ_encoder *fe)
{
	int	i;
	fe->key4 = 0;
	for (i = 0; i < 3 && i < Mb_avail_bytes(&fe->eb.mb); ++i)
		fe->key4 = (fe->key4 << 4) ^ fe->eb.mb.buffer[i];
}

int	FLZe_longest_match_len(FLZ_encoder *fe, int *distance);

static void
FLZe_update_and_move(FLZ_encoder *fe, int n)
{
	while (--n >= 0) {
		if (Mb_avail_bytes(&fe->eb.mb) >= 4) {
			fe->key4 = ((fe->key4 << 4) ^ fe->eb.mb.buffer[fe->eb.mb.pos+3]) &
			    fe->eb.mb.key4_mask;
			fe->eb.mb.pos_array[fe->eb.mb.cyclic_pos] = fe->eb.mb.prev_positions[fe->key4];
			fe->eb.mb.prev_positions[fe->key4] = fe->eb.mb.pos + 1;
		}
		Mb_move_pos(&fe->eb.mb);
	}
}

static bool
FLZe_init(FLZ_encoder *fe, int ifd, int outfd)
{
	enum {
		before = 0,
		dict_size = 65536,
		/* bytes to keep in buffer after pos */
		after_size = max_match_len,
		dict_factor = 16,
		num_prev_positions23 = 0,
		pos_array_factor = 1
	};

	return LZeb_init(&fe->eb, before, dict_size, after_size, dict_factor,
	    num_prev_positions23, pos_array_factor, ifd, outfd);
}

static void
FLZe_reset(FLZ_encoder *fe)
{
	LZeb_reset(&fe->eb);
}

bool	FLZe_encode_member(FLZ_encoder *fe, uvlong member_size);

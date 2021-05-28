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

#ifndef _LZIP_H
#define _LZIP_H

#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <ctype.h>

#define exit(n) exits((n) == 0? 0: "err")
#define isatty(fd) 0
#define lseek seek

#ifndef max
#define max(x,y) ((x) >= (y) ? (x) : (y))
#endif
#ifndef min
#define min(x,y) ((x) <= (y) ? (x) : (y))
#endif

typedef int	State;
typedef long int32_t;
typedef ulong uint32_t;
typedef int bool;

enum { false, true };

enum { states = 12 };
enum {
	min_dict_bits = 12,
	min_dict_size = 1 << min_dict_bits,	/* >= modeled_distances */
	max_dict_bits = 29,
	max_dict_size = 1 << max_dict_bits,
	min_member_size = 36,
	literal_context_bits = 3,
	literal_pos_state_bits = 0, 			/* not used */
	pos_state_bits = 2,
	pos_states = 1 << pos_state_bits,
	pos_state_mask = pos_states -1,

	len_states = 4,
	dis_slot_bits = 6,
	start_dis_model = 4,
	end_dis_model = 14,
	modeled_distances = 1 << (end_dis_model / 2), 	/* 128 */
	dis_align_bits = 4,
	dis_align_size = 1 << dis_align_bits,

	len_low_bits = 3,
	len_mid_bits = 3,
	len_high_bits = 8,
	len_low_syms = 1 << len_low_bits,
	len_mid_syms = 1 << len_mid_bits,
	len_high_syms = 1 << len_high_bits,
	max_len_syms = len_low_syms + len_mid_syms + len_high_syms,

	min_match_len = 2, 					/* must be 2 */
	max_match_len = min_match_len + max_len_syms - 1, 	/* 273 */
	min_match_len_limit = 5,

	bit_model_move_bits = 5,
	bit_model_total_bits = 11,
	bit_model_total = 1 << bit_model_total_bits,
};

typedef struct Len_model Len_model;
typedef struct Pretty_print Pretty_print;
typedef struct Matchfinder_base Matchfinder_base;
typedef int	Bit_model;

struct Len_model {
	Bit_model choice1;
	Bit_model choice2;
	Bit_model bm_low[pos_states][len_low_syms];
	Bit_model bm_mid[pos_states][len_mid_syms];
	Bit_model bm_high[len_high_syms];
};
struct Pretty_print {
	char	*name;
	char	*stdin_name;
	ulong	longest_name;
	bool	first_post;
};

typedef ulong CRC32[256];	/* Table of CRCs of all 8-bit messages. */

extern CRC32 crc32;

#define errno 0

static uchar magic_string[4] = { "LZIP" };

typedef uchar File_header[6];		/* 0-3 magic bytes */
/*   4 version */
/*   5 coded_dict_size */
enum { Fh_size = 6 };

typedef uchar File_trailer[20];
/*  0-3  CRC32 of the uncompressed data */
/*  4-11 size of the uncompressed data */
/* 12-19 member size including header and trailer */

enum { Ft_size = 20 };

enum {
	price_shift_bits = 6,
	price_step_bits = 2,
	price_step = 1 << price_step_bits,
};

typedef uchar Dis_slots[1<<10];
typedef short	Prob_prices[bit_model_total >> price_step_bits];

extern Dis_slots dis_slots;
extern Prob_prices prob_prices;

#define get_price(prob)		prob_prices[(prob) >> price_step_bits]
#define price0(prob)		get_price(prob)
#define price1(prob)		get_price(bit_model_total - (prob))
#define price_bit(bm, bit)	((bit)? price1(bm): price0(bm))

#define Mb_ptr_to_current_pos(mb) ((mb)->buffer + (mb)->pos)
#define Mb_peek(mb, distance) (mb)->buffer[(mb)->pos - (distance)]

#define Lp_price(lp, len, pos_state) \
	(lp)->prices[pos_state][(len) - min_match_len]

#define Tr_update(trial, pr, distance4, p_i) \
{ \
	if ((pr) < (trial)->price) { \
		(trial)->price = pr; \
		(trial)->dis4 = distance4; \
		(trial)->prev_index = p_i; \
		(trial)->prev_index2 = single_step_trial; \
	} else { \
	} \
}

/* these functions are now extern and must be defined exactly once */
#ifdef	_DEFINE_INLINES
#define	_INLINES_DEFINED

int
get_len_state(int len)
{
	int lenstm1, lenmmm;

	lenmmm = len - min_match_len;
	lenstm1 = len_states - 1;
	if (lenmmm < lenstm1)
		return lenmmm;
	else
		return lenstm1;
}

State
St_set_char(State st)
{
	static State next[states] = { 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 4, 5 };	

	assert((unsigned)st < nelem(next));
	return next[st];
}

int
get_lit_state(uchar prev_byte)
{
	return prev_byte >> (8 - literal_context_bits);
}

void
Bm_init(Bit_model *probability)
{
	*probability = bit_model_total / 2;
}

void
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
	Bm_array_init(lm->bm_low[0], pos_states * len_low_syms);
	Bm_array_init(lm->bm_mid[0], pos_states * len_mid_syms);
	Bm_array_init(lm->bm_high, len_high_syms);
}

void
Pp_init(Pretty_print *pp, char *filenames[], int num_filenames, int verbosity)
{
	unsigned stdin_name_len;
	int i;

	pp->name = 0;
	pp->stdin_name = "(stdin)";
	pp->longest_name = 0;
	pp->first_post = false;

	if (verbosity <= 0)
		return;
	stdin_name_len = strlen(pp->stdin_name);
	for (i = 0; i < num_filenames; ++i) {
		char *s = filenames[i];
		unsigned len = strcmp(s, "-") == 0? stdin_name_len: strlen(s);

		if (len > pp->longest_name)
			pp->longest_name = len;
	}
	if (pp->longest_name == 0)
		pp->longest_name = stdin_name_len;
}

void
Pp_set_name(Pretty_print *pp, char *filename)
{
	if ( filename && filename[0] && strcmp( filename, "-" ) != 0 )
		pp->name = filename;
	else
		pp->name = pp->stdin_name;
	pp->first_post = true;
}

void
Pp_reset(Pretty_print *pp)
{
	if (pp->name && pp->name[0])
		pp->first_post = true;
}

void
Pp_show_msg(Pretty_print *pp, char *msg);

void
CRC32_init(void)
{
	unsigned n;

	for (n = 0; n < 256; ++n) {
		unsigned	c = n;
		int	k;
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
CRC32_update_byte(uint32_t *crc, uchar byte)
{
	*crc = crc32[(*crc^byte)&0xFF] ^ (*crc >> 8);
}

void
CRC32_update_buf(uint32_t *crc, uchar *buffer, int size)
{
	int	i;
	uint32_t c = *crc;
	for (i = 0; i < size; ++i)
		c = crc32[(c^buffer[i])&0xFF] ^ (c >> 8);
	*crc = c;
}

bool
isvalid_ds(unsigned dict_size)
{
	return (dict_size >= min_dict_size &&
	    dict_size <= max_dict_size);
}

int
real_bits(unsigned value)
{
	int	bits = 0;

	while (value > 0) {
		value >>= 1;
		++bits;
	}
	return bits;
}

void
Fh_set_magic(File_header data)
{
	memcpy(data, magic_string, 4);
	data[4] = 1;
}

bool
Fh_verify_magic(File_header data)
{
	return (memcmp(data, magic_string, 4) == 0);
}

/* detect truncated header */
bool
Fh_verify_prefix(File_header data, int size)
{
	int	i;
	for (i = 0; i < size && i < 4; ++i)
		if (data[i] != magic_string[i])
			return false;
	return (size > 0);
}

uchar
Fh_version(File_header data)
{
	return data[4];
}

bool
Fh_verify_version(File_header data)
{
	return (data[4] == 1);
}

unsigned
Fh_get_dict_size(File_header data)
{
	unsigned	sz = (1 << (data[5] &0x1F));
	if (sz > min_dict_size)
		sz -= (sz / 16) * ((data[5] >> 5) & 7);
	return sz;
}

bool
Fh_set_dict_size(File_header data, unsigned sz)
{
	if (!isvalid_ds(sz))
		return false;
	data[5] = real_bits(sz - 1);
	if (sz > min_dict_size) {
		unsigned base_size = 1 << data[5];
		unsigned fraction = base_size / 16;
		unsigned	i;
		for (i = 7; i >= 1; --i)
			if (base_size - (i * fraction) >= sz) {
				data[5] |= (i << 5);
				break;
			}
	}
	return true;
}

unsigned
Ft_get_data_crc(File_trailer data)
{
	unsigned	tmp = 0;
	int	i;
	for (i = 3; i >= 0; --i) {
		tmp <<= 8;
		tmp += data[i];
	}
	return tmp;
}

void
Ft_set_data_crc(File_trailer data, unsigned crc)
{
	int	i;
	for (i = 0; i <= 3; ++i) {
		data[i] = (uchar)crc;
		crc >>= 8;
	}
}

uvlong
Ft_get_data_size(File_trailer data)
{
	uvlong	tmp = 0;
	int	i;
	for (i = 11; i >= 4; --i) {
		tmp <<= 8;
		tmp += data[i];
	}
	return tmp;
}

void
Ft_set_data_size(File_trailer data, uvlong sz)
{
	int	i;
	for (i = 4; i <= 11; ++i) {
		data[i] = (uchar)sz;
		sz >>= 8;
	}
}

uvlong
Ft_get_member_size(File_trailer data)
{
	uvlong	tmp = 0;
	int	i;
	for (i = 19; i >= 12; --i) {
		tmp <<= 8;
		tmp += data[i];
	}
	return tmp;
}

void
Ft_set_member_size(File_trailer data, uvlong sz)
{
	int	i;
	for (i = 12; i <= 19; ++i) {
		data[i] = (uchar)sz;
		sz >>= 8;
	}
}
#else					/* _DEFINE_INLINES */
void	Bm_array_init(Bit_model bm[], int size);
void	Bm_init(Bit_model *probability);
void	CRC32_init(void);
void	CRC32_update_buf(uint32_t *crc, uchar *buffer, int size);
void	CRC32_update_byte(uint32_t *crc, uchar byte);
unsigned	Fh_get_dict_size(File_header data);
bool	Fh_set_dict_size(File_header data, unsigned sz);
void	Fh_set_magic(File_header data);
bool	Fh_verify_magic(File_header data);
bool	Fh_verify_prefix(File_header data, int size);
bool	Fh_verify_version(File_header data);
uchar	Fh_version(File_header data);
unsigned	Ft_get_data_crc(File_trailer data);
uvlong	Ft_get_data_size(File_trailer data);
uvlong	Ft_get_member_size(File_trailer data);
void	Ft_set_data_crc(File_trailer data, unsigned crc);
void	Ft_set_data_size(File_trailer data, uvlong sz);
void	Ft_set_member_size(File_trailer data, uvlong sz);
void	Lm_init(Len_model *lm);
void	Pp_init(Pretty_print *pp, char *filenames[], int num_filenames, int verbosity);
void	Pp_reset(Pretty_print *pp);
void	Pp_set_name(Pretty_print *pp, char *filename);
void	Pp_show_msg(Pretty_print *pp, char *msg);
State	St_set_char(State st);
int	get_lit_state(uchar prev_byte);
int	get_len_state(int len);
bool	isvalid_ds(unsigned dict_size);
int	real_bits(unsigned value);
#endif					/* _DEFINE_INLINES */

#define St_is_char(state)	((state) < 7)
#define St_set_match(state)	((state) < 7? 7: 10)
#define St_set_rep(state)	((state) < 7? 8: 11)
#define St_set_short_rep(state)	((state) < 7? 9: 11)

static char *bad_magic_msg = "Bad magic number (file not in lzip format).";
static char *bad_dict_msg = "Invalid dictionary size in member header.";
static char *trailing_msg = "Trailing data not allowed.";

/* defined in decoder.c */
int	readblock(int fd, uchar *buf, int size);
int	writeblock(int fd, uchar *buf, int size);

/* defined in main.c */
extern int verbosity;
Dir;
char	*bad_version(unsigned version);
char	*format_ds(unsigned dict_size);
int	open_instream(char *name, Dir *in_statsp, bool no_ofile, bool reg_only);
void	*resize_buffer(void *buf, unsigned min_size);
void	cleanup_and_fail(int retval);
void	show_error(char *msg, int errcode, bool help);
void	show_file_error(char *filename, char *msg, int errcode);
void	internal_error(char *msg);
struct Matchfinder_base;
void	show_progress(uvlong partial_size, Matchfinder_base *m, Pretty_print *p,
	uvlong cfile_size);
#endif

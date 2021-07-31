/* Copyright (C) 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
  This file is part of Aladdin Ghostscript.
  
  Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
  License (the "License") for full details.
  
  Every copy of Aladdin Ghostscript must include a copy of the License,
  normally in a plain ASCII text file named PUBLIC.  The License grants you
  the right to copy, modify and redistribute Aladdin Ghostscript, but only
  under certain conditions described in the License.  Among other things, the
  License requires that the copyright notice and this notice be preserved on
  all copies.
*/

/* slzwx.h */
/* LZW filter state definition */
/* Requires strimpl.h */

typedef struct lzw_decode_s lzw_decode;
typedef struct lzw_encode_table_s lzw_encode_table;
typedef struct stream_LZW_state_s {
	stream_state_common;
		/* The following are set before initialization. */
	int InitialCodeLength;		/* decoding only */
	bool FirstBitLowOrder;		/* decoding only */
	bool BlockData;			/* decoding only */
	int EarlyChange;		/* decoding only */
		/* The following are updated dynamically. */
	uint bits;			/* buffer for input bits */
	int bits_left;			/* # of valid low bits left */
	int bytes_left;			/* # of bytes left in current block */
					/* (arbitrary large # if not GIF) */
	union _lzt {
		lzw_decode *decode;
		lzw_encode_table *encode;
	} table;
	uint next_code;			/* next code to be assigned */
	int code_size;			/* current # of bits per code */
	int prev_code;			/* previous code recognized */
					/* or assigned */
	uint prev_len;			/* length of prev_code */
	int copy_code;			/* code whose string is being */
					/* copied, -1 if none */
	uint copy_len;			/* length of copy_code */
	int copy_left;			/* amount of string left to copy */
	bool first;			/* true if no output yet */
} stream_LZW_state;
extern_st(st_LZW_state);
#define public_st_LZW_state()	/* in slzwd.c */\
  gs_public_st_ptrs1(st_LZW_state, stream_LZW_state,\
    "LZWDecode state", lzwd_enum_ptrs, lzwd_reloc_ptrs, table.decode)
extern const stream_template s_LZWD_template;
extern const stream_template s_LZWE_template;

/* Copyright (C) 1992, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* shc.h */
/* Common definitions for filters using Huffman coding */
/* Requires scommon.h */

#ifndef shc_INCLUDED
#  define shc_INCLUDED

/*
 * These definitions are valid for code lengths up to 16 bits
 * and non-negative decoded values up to 15 bits.
 *
 * We define 3 different representations of the code: encoding tables,
 * decoding tables, and a definition table which can be generated easily
 * from frequency information and which in turn can easily generate
 * the encoding and decoding tables.
 *
 * The definition table has two parts: a list of the number of i-bit
 * codes for each i >= 1, and the decoded values corresponding to
 * the code values in increasing lexicographic order (which will also
 * normally be decreasing code frequency).  Calling these two lists
 * L[1..M] and V[0..N-1] respectively, we have the following invariants:
 *	- 1 <= M <= max_hc_length, N >= 2.
 *	- L[0] = 0.
 *	- for i=1..M, L[i] >= 0.
 *	- sum(i=1..M: L[i]) = N.
 *	- sum(i=1..M: L[i] * 2^-i) = 1.
 *	- V[0..N-1] are a permutation of the integers 0..N-1.
 */
#define max_hc_length 16
typedef struct hc_definition_s {
	ushort *counts;			/* [0..M] */
	uint num_counts;		/* M */
	ushort *values;			/* [0..N-1] */
	uint num_values;		/* N */
} hc_definition;

/* ------ Common state ------ */

/*
 * Define the common stream state for Huffman-coded filters.
 * Invariants when writing:
 *	0 <= bits_left <= hc_bits_size;
 *	Only the leftmost (hc_bits_size - bits_left) bits of bits
 *	  contain valid data.
 */
#define stream_hc_state_common\
	stream_state_common;\
		/* The client sets the following before initialization. */\
	bool FirstBitLowOrder;\
		/* The following are updated dynamically. */\
	uint bits;		/* most recent bits of input or */\
				/* current bits of output */\
	int bits_left		/* # of valid low bits (input) or */\
				/* unused low bits (output) in above */
typedef struct stream_hc_state_s {
	stream_hc_state_common;
} stream_hc_state;

#define hc_bits_size (arch_sizeof_int * 8)
#define s_hce_init_inline(ss)\
  ((ss)->bits = 0, (ss)->bits_left = hc_bits_size)
#define s_hcd_init_inline(ss)\
  ((ss)->bits = 0, (ss)->bits_left = 0)

/* Declare a table for reversing the bit order of each byte. */
extern const byte sbits_reverse_bits[256];

/* ------ Encoding tables ------ */

/* Define the structure for the encoding tables. */
typedef struct hce_code_s {
	ushort code;
	ushort code_length;
} hce_code;
#define hce_entry(c, len) { c, len }

typedef struct hce_table_s {
	uint count;
	hce_code *codes;
} hce_table;

#define hce_bits_available(n)\
  (ss->bits_left >= (n) || wlimit - q > ((n) - ss->bits_left - 1) >> 3)

/* ------ Encoding utilities ------ */

/*
 * Put a code on the output.  The client is responsible for ensuring
 * that q does not exceed pw->limit.
 */

#ifdef DEBUG
#  define hc_print(rp)\
    (gs_debug_c('W') ?\
     (dprintf2("[W]0x%x,%d\n", (rp)->code, (rp)->code_length), 0) : 0)
#else
#  define hc_print(rp) 0
#endif

byte *hc_put_code_proc(P3(stream_hc_state *, byte *, uint));
#define hc_put_code(ss, q, cp)\
  (hc_print(cp),\
   (((ss)->bits_left -= (cp)->code_length) >= 0 ?\
    (ss)->bits += (cp)->code << (ss)->bits_left :\
    (q = hc_put_code_proc((stream_hc_state *)(ss), q, (cp)->code), 0)))

/*
 * Force out the final bits to the output.
 */
byte *hc_put_last_bits(P2(stream_hc_state *, byte *));


/* ------ Decoding tables ------ */

/*
 * Define the structure for the decoding tables.
 * First-level nodes are either leaves, which have
 *	value = decoded value
 *	code_length <= initial_bits
 * or non-leaves, which have
 *	value = the index of a sub-table
 *	code_length = initial_bits + the number of additional dispatch bits
 * Second-level nodes are always leaves, with
 *	code_length = the actual number of bits in the code - initial_bits.
 */

typedef struct hcd_code_s {
	short value;
	ushort code_length;
} hcd_code;

typedef struct hcd_table_s {
	uint count;
	uint initial_bits;
	hcd_code *codes;
} hcd_table;

/* Declare variables that hold the decoder state. */
#define hcd_declare_state\
	register const byte *p;\
	const byte *rlimit;\
	uint bits;\
	int bits_left

/* Load the state from the stream. */
/* Free variables: pr, ss, p, rlimit, bits, bits_left. */
#define hcd_load_state()\
	p = pr->ptr, rlimit = pr->limit,\
	bits = ss->bits, bits_left = ss->bits_left

/* Store the state back in the stream. */
/* Free variables: pr, ss, p, bits, bits_left. */
#define hcd_store_state()\
	pr->ptr = p,\
	ss->bits = bits, ss->bits_left = bits_left

/* Macros to get blocks of bits from the input stream. */
/* Invariants: 0 <= bits_left <= bits_size; */
/* bits [bits_left-1..0] contain valid data. */

#define hcd_bits_available(n)\
  (bits_left >= (n) || rlimit - p > ((n) - bits_left - 1) >> 3)
/* For hcd_ensure_bits, n must not be greater than 8. */
#define hcd_ensure_bits(n, outl)\
  if ( bits_left < n ) hcd_more_bits(outl)

/* Load more bits into the buffer. */
#define hcd_more_bits_1(outl)\
  { int c;\
    if ( p < rlimit ) c = *++p;\
    else goto outl;\
    if ( ss->FirstBitLowOrder ) c = sbits_reverse_bits[c];\
    bits = (bits << 8) + c, bits_left += 8;\
  }
#if hc_bits_size == 16
#  define hcd_more_bits(outl) hcd_more_bits_1(outl)
#else				/* hc_bits_size >= 32 */
#  define hcd_more_bits(outl)\
  { if ( rlimit - p < 3 ) hcd_more_bits_1(outl)\
    else\
    { if ( ss->FirstBitLowOrder )\
	bits = (bits << 24) + ((uint)sbits_reverse_bits[p[1]] << 16) + ((uint)sbits_reverse_bits[p[2]] << 8) + sbits_reverse_bits[p[3]];\
      else\
	bits = (bits << 24) + ((uint)p[1] << 16) + ((uint)p[2] << 8) + p[3];\
      bits_left += 24, p += 3;\
    }\
  }
#endif

#define hcd_peek_bits(n) ((bits >> (bits_left - (n))) & ((1 << (n)) - 1))

#define hcd_peek_var_bits(n)\
  ((bits >> (bits_left - (n))) & sbits_peek_masks[n])
extern const byte sbits_peek_masks[9];

#define hcd_skip_bits(n) (bits_left -= (n))

#endif					/* shc_INCLUDED */

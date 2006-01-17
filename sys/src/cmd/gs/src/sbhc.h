/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: sbhc.h,v 1.4 2002/02/21 22:24:53 giles Exp $ */
/* Definitions for BoundedHuffman filters */
/* Requires strimpl.h */

#ifndef sbhc_INCLUDED
#  define sbhc_INCLUDED

#include "shc.h"

/*
 * The BoundedHuffman filters extend the basic Huffman coding model by
 * providing the ability to encode runs of zeros as a single data item,
 * and by providing an end-of-data (EOD) marker.
 */
#define max_zero_run 100

/* Common state */
#define stream_BHC_state_common\
	stream_hc_state_common;\
	hc_definition definition;\
		/* The client sets the following before initialization. */\
	bool EndOfData;\
	uint EncodeZeroRuns;\
		/* The following are updated dynamically. */\
	int zeros		/* # of zeros scanned or left to output */
typedef struct stream_BHC_state_s {
    stream_BHC_state_common;
} stream_BHC_state;

/* BoundedHuffmanEncode */
typedef struct stream_BHCE_state_s {
    stream_BHC_state_common;
    hce_table encode;
} stream_BHCE_state;

#define private_st_BHCE_state()	/* in sbhc.c */\
  gs_private_st_ptrs3(st_BHCE_state, stream_BHCE_state,\
    "BoundedHuffmanEncode state", bhce_enum_ptrs, bhce_reloc_ptrs,\
    definition.counts, definition.values, encode.codes)
extern const stream_template s_BHCE_template;

#define s_bhce_init_inline(ss)\
  (s_hce_init_inline(ss), (ss)->zeros = 0)

/* BoundedHuffmanDecode */
typedef struct stream_BHCD_state_s {
    stream_BHC_state_common;
    hcd_table decode;
} stream_BHCD_state;

#define private_st_BHCD_state()	/* in sbhc.c */\
  gs_private_st_ptrs3(st_BHCD_state, stream_BHCD_state,\
    "BoundedHuffmanDecode state", bhcd_enum_ptrs, bhcd_reloc_ptrs,\
    definition.counts, definition.values, decode.codes)
extern const stream_template s_BHCD_template;

#define s_bhcd_init_inline(ss)\
  (s_hcd_init_inline(ss), (ss)->zeros = 0)

/* Declare variables that hold the decoder state. */
#define bhcd_declare_state\
	hcd_declare_state;\
	int zeros

/* Load the state from the stream. */
/* Free variables: pr, ss, p, rlimit, bits, bits_left, zeros. */
#define bhcd_load_state()\
	hcd_load_state(), zeros = ss->zeros

/* Store the state back in the stream. */
/* Free variables: pr, ss, p, bits, bits_left, zeros. */
#define bhcd_store_state()\
	hcd_store_state(), ss->zeros = zeros

#endif /* sbhc_INCLUDED */

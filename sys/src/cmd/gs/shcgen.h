/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* shcgen.h */
/* Interface to shcgen.c */
/* Requires shc.h */

/* Compute an optimal Huffman code from an input data set. */
/* The client must have set all the elements of *def. */
/* The definition is guaranteed to be well-behaved. */
int hc_compute(P3(hc_definition *def, const long *freqs, gs_memory_t *mem));

/* Convert a definition to a byte string. */
/* The caller must provide the byte string, of length def->num_values. */
/* Assume (do not check) that the definition is well-behaved. */
/* Return the actual length of the string. */
int hc_bytes_from_definition(P2(byte *dbytes, const hc_definition *def));

/* Extract num_counts and num_values from a byte string. */
void hc_sizes_from_bytes(P3(hc_definition *def, const byte *dbytes, int num_bytes));

/* Convert a byte string back to a definition. */
/* The caller must initialize *def, including allocating counts and values. */
void hc_definition_from_bytes(P2(hc_definition *def, const byte *dbytes));

/* Generate the encoding table from the definition. */
/* The size of the encode array is def->num_values. */
void hc_make_encoding(P2(hce_code *encode, const hc_definition *def));

/* Calculate the size of the decoding table. */
uint hc_sizeof_decoding(P2(const hc_definition *def, int initial_bits));

/* Generate the decoding tables. */
void hc_make_decoding(P3(hcd_code *decode, const hc_definition *def,
  int initial_bits));

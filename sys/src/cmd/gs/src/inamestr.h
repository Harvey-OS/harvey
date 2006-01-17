/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: inamestr.h,v 1.4 2002/02/21 22:24:53 giles Exp $ */
/* Name table strings definition */

#ifndef inamestr_INCLUDED
#  define inamestr_INCLUDED

#include "inameidx.h"

/*
 * In the code below, we use the hashing method described in
 * "Fast Hashing of Variable-Length Text Strings" by Peter K. Pearson,
 * pp. 677-680, CACM 33(6), June 1990.
 */

/*
 * Define a pseudo-random permutation of the integers 0..255.
 * Pearson's article claims this permutation gave good results.
 */
#define NAME_HASH_PERMUTATION_DATA\
   1, 87, 49, 12, 176, 178, 102, 166, 121, 193, 6, 84, 249, 230, 44, 163,\
   14, 197, 213, 181, 161, 85, 218, 80, 64, 239, 24, 226, 236, 142, 38, 200,\
   110, 177, 104, 103, 141, 253, 255, 50, 77, 101, 81, 18, 45, 96, 31, 222,\
   25, 107, 190, 70, 86, 237, 240, 34, 72, 242, 20, 214, 244, 227, 149, 235,\
   97, 234, 57, 22, 60, 250, 82, 175, 208, 5, 127, 199, 111, 62, 135, 248,\
   174, 169, 211, 58, 66, 154, 106, 195, 245, 171, 17, 187, 182, 179, 0, 243,\
   132, 56, 148, 75, 128, 133, 158, 100, 130, 126, 91, 13, 153, 246, 216, 219,\
   119, 68, 223, 78, 83, 88, 201, 99, 122, 11, 92, 32, 136, 114, 52, 10,\
   138, 30, 48, 183, 156, 35, 61, 26, 143, 74, 251, 94, 129, 162, 63, 152,\
   170, 7, 115, 167, 241, 206, 3, 150, 55, 59, 151, 220, 90, 53, 23, 131,\
   125, 173, 15, 238, 79, 95, 89, 16, 105, 137, 225, 224, 217, 160, 37, 123,\
   118, 73, 2, 157, 46, 116, 9, 145, 134, 228, 207, 212, 202, 215, 69, 229,\
   27, 188, 67, 124, 168, 252, 42, 4, 29, 108, 21, 247, 19, 205, 39, 203,\
   233, 40, 186, 147, 198, 192, 155, 33, 164, 191, 98, 204, 165, 180, 117, 76,\
   140, 36, 210, 172, 41, 54, 159, 8, 185, 232, 113, 196, 231, 47, 146, 120,\
   51, 65, 28, 144, 254, 221, 93, 189, 194, 139, 112, 43, 71, 109, 184, 209

/* Compute the hash for a name string.  Assume size >= 1. */
#define NAME_HASH(hash, hperm, ptr, size)\
  BEGIN\
    const byte *p = ptr;\
    uint n = size;\
\
    hash = hperm[*p++];\
    while (--n > 0)\
	hash = (hash << 8) | hperm[(byte)hash ^ *p++];\
  END

/*
 * Define the structure for a name string.  The next_index "pointer" is used
 * both for the chained hash table in the name_table and for the list of
 * free names.
 */
typedef struct name_string_s {
#if EXTEND_NAMES
#  define name_extension_bits 6
#else
#  define name_extension_bits 0
#endif
    unsigned next_index:16 + name_extension_bits; /* next name in chain or 0 */
    unsigned foreign_string:1;	/* string is allocated statically */
    unsigned mark:1;		/* GC mark bit of index for this name */
#define name_string_size_bits (14 - name_extension_bits)
#define max_name_string ((1 << name_string_size_bits) - 1)
    unsigned string_size:name_string_size_bits;
    const byte *string_bytes;
} name_string_t;
#define name_next_index(nidx, pnstr)\
   ((pnstr)->next_index)
#define set_name_next_index(nidx, pnstr, next)\
   ((pnstr)->next_index = (next))

/* Define a sub-table of name strings. */
typedef struct name_string_sub_table_s {
    name_string_t strings[NT_SUB_SIZE];
} name_string_sub_table_t;

/* Define the size of the name hash table. */
#define NT_HASH_SIZE (1024 << (EXTEND_NAMES / 2))  /* must be a power of 2 */

#endif /* inamestr_INCLUDED */

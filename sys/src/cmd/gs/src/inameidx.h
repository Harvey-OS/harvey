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

/* $Id: inameidx.h,v 1.4 2002/02/21 22:24:53 giles Exp $ */
/* Name index definitions */

#ifndef inameidx_INCLUDED
#  define inameidx_INCLUDED

#include "gconfigv.h"		/* defines EXTEND_NAMES */

/*
 * The name table machinery has two slightly different configurations:
 * a faster one that limits the total number of names to 64K and allows
 * names up to 16K in size, and a slightly slower one that limits
 * the total to 4M and restricts names to 256 characters.
 */
#ifndef EXTEND_NAMES		/* # of bits beyond 16 */
#  define EXTEND_NAMES 0
#endif

/* Define the size of a name sub-table. */
#define NT_LOG2_SUB_SIZE (8 + (EXTEND_NAMES / 2))
# define NT_SUB_SIZE (1 << NT_LOG2_SUB_SIZE)
# define NT_SUB_INDEX_MASK (NT_SUB_SIZE - 1)

/*
 * Define the first few entries of the name table.  Entry 0 is left unused.
 * The entry with count = 1 is the entry for the 0-length name.
 * The next NT_1CHAR_SIZE entries (in count order) are 1-character names.
 */
#define NT_1CHAR_SIZE 128
#define NT_1CHAR_FIRST 2
#define NT_1CHAR_NAMES_DATA\
   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,\
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,\
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,\
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,\
  64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,\
  80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,\
  96, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,\
 112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127

/* ---------------- Name count/index mapping ---------------- */

/*
 * We scramble the assignment order within a sub-table, so that
 * dictionary lookup doesn't have to scramble the index.
 * The scrambling algorithm must have three properties:
 *      - It must map 0 to 0;
 *      - It must only scramble the sub-table index;
 *      - It must be a permutation on the sub-table index.
 * Something very simple works just fine.
 */
#define NAME_COUNT_TO_INDEX_FACTOR 23
#define name_count_to_index(cnt)\
  (((cnt) & (-NT_SUB_SIZE)) +\
   (((cnt) * NAME_COUNT_TO_INDEX_FACTOR) & NT_SUB_INDEX_MASK))
/*
 * The reverse permutation requires finding a number R such that
 * NAME_COUNT_TO_INDEX_FACTOR * R = 1 mod NT_SUB_SIZE.
 * The value given below works for NT_SUB_SIZE any power of 2 up to 4096.
 * This is not currently used anywhere.
 */
#define NAME_INDEX_TO_COUNT_FACTOR 1959
#define name_index_to_count(nidx)\
  (((nidx) & (-NT_SUB_SIZE)) +\
   (((nidx) * NAME_INDEX_TO_COUNT_FACTOR) & NT_SUB_INDEX_MASK))

#endif /* inameidx_INCLUDED */

/* Copyright (C) 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* gxcache.h */
/* General-purpose cache schema for Ghostscript */

#ifndef gxcache_INCLUDED
#  define gxcache_INCLUDED

/*
 * Ghostscript caches a wide variety of information internally:
 * *font/matrix pairs, *scaled fonts, rendered characters,
 * binary halftones, *colored halftones, *Patterns,
 * and the results of many procedures (transfer functions, undercolor removal,
 * black generation, CIE color transformations).
 * The caches marked with * all use a similar structure: a chained hash
 * table with a maximum number of entries allocated in a single block,
 * and a roving pointer for purging.
 */
#define cache_members(entry_type, hash_size)\
	uint csize, cmax;\
	uint cnext;\
	uint esize;		/* for generic operations */\
	entry_type *entries;\
	entry_type *hash[hash_size];
#define cache_init(pcache)\
  (memset((char *)(pcache)->hash, 0, sizeof((pcache)->hash)),\
   (pcache)->esize = sizeof(*(pcache)->entries),\
   (pcache)->entries = 0,\
   (pcache)->csize = (pcache)->cmax = (pcache)->cnext = 0)
/*
 * The following operations should be generic, but can't be because
 * C doesn't provide templates: allocate, look up, add, purge at 'restore'.
 */

#endif					/* gxcache_INCLUDED */

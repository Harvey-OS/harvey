/* Copyright (C) 1997, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxsamplp.h,v 1.2 2005/06/08 14:38:21 igor Exp $ */
/* Templates for sample lookup and expansion */

/* This module is allowed to include several times into a single .c file. 
   The following macros to be defined in advance :
	MULTIPLE_MAPS - 1 if num_components_per_plane > 0 and 
			components use different maps, 0 otherwise.
	TEMPLATE_sample_unpack_1 - a name for the function
	TEMPLATE_sample_unpack_2 - a name for the function
	TEMPLATE_sample_unpack_4 - a name for the function
	TEMPLATE_sample_unpack_8 - a name for the function
 */

#if MULTIPLE_MAPS
#   define NEXT_MAP map = smap[++smap_index % num_components_per_plane].table.lookup4x1to32
#   define NEXT_MAP8 map = smap[++smap_index % num_components_per_plane].table.lookup8
#   define DEFINE_SMAP_INDEX int smap_index = 0;
#else
#   define NEXT_MAP
#   define NEXT_MAP8
#   define DEFINE_SMAP_INDEX
#endif

const byte *
TEMPLATE_sample_unpack_1(byte * bptr, int *pdata_x, const byte * data, int data_x,
		uint dsize, const sample_map *smap, int spread,
		int num_components_per_plane)
{
    const sample_lookup_t * ptab = &smap->table;
    const byte *psrc = data + (data_x >> 3);
    int left = dsize - (data_x >> 3);
    DEFINE_SMAP_INDEX

    if (spread == 1) {
	bits32 *bufp = (bits32 *) bptr;
	const bits32 *map = &ptab->lookup4x1to32[0];
	uint b;

	if (left & 1) {
	    b = psrc[0];
	    bufp[0] = map[b >> 4];
	    NEXT_MAP;
	    bufp[1] = map[b & 0xf];
	    NEXT_MAP;
	    psrc++, bufp += 2;
	}
	left >>= 1;
	while (left--) {
	    b = psrc[0];
	    bufp[0] = map[b >> 4];
	    NEXT_MAP;
	    bufp[1] = map[b & 0xf];
	    NEXT_MAP;
	    b = psrc[1];
	    bufp[2] = map[b >> 4];
	    NEXT_MAP;
	    bufp[3] = map[b & 0xf];
	    NEXT_MAP;
	    psrc += 2, bufp += 4;
	}
    } else {
	byte *bufp = bptr;
	const byte *map = &ptab->lookup8[0];

	while (left--) {
	    uint b = *psrc++;

	    *bufp = map[b >> 7];
	    NEXT_MAP8;
	    bufp += spread;
	    *bufp = map[(b >> 6) & 1];
	    NEXT_MAP8;
	    bufp += spread;
	    *bufp = map[(b >> 5) & 1];
	    NEXT_MAP8;
	    bufp += spread;
	    *bufp = map[(b >> 4) & 1];
	    NEXT_MAP8;
	    bufp += spread;
	    *bufp = map[(b >> 3) & 1];
	    NEXT_MAP8;
	    bufp += spread;
	    *bufp = map[(b >> 2) & 1];
	    NEXT_MAP8;
	    bufp += spread;
	    *bufp = map[(b >> 1) & 1];
	    NEXT_MAP8;
	    bufp += spread;
	    *bufp = map[b & 1];
	    NEXT_MAP8;
	    bufp += spread;
	}
    }
    *pdata_x = data_x & 7;
    return bptr;
}

#undef NEXT_MAP

#if MULTIPLE_MAPS
#   define NEXT_MAP  map = smap[++smap_index % num_components_per_plane].table.lookup2x2to16
#else
#   define NEXT_MAP
#endif

const byte *
TEMPLATE_sample_unpack_2(byte * bptr, int *pdata_x, const byte * data, int data_x,
		uint dsize, const sample_map *smap, int spread,
		int num_components_per_plane)
{
    const sample_lookup_t * ptab = &smap->table;
    const byte *psrc = data + (data_x >> 2);
    int left = dsize - (data_x >> 2);
    DEFINE_SMAP_INDEX

    if (spread == 1) {
	bits16 *bufp = (bits16 *) bptr;
	const bits16 *map = &ptab->lookup2x2to16[0];

	while (left--) {
	    uint b = *psrc++;

	    *bufp++ = map[b >> 4];
	    NEXT_MAP;
	    *bufp++ = map[b & 0xf];
	    NEXT_MAP;
	}
    } else {
	byte *bufp = bptr;
	const byte *map = &ptab->lookup8[0];

	while (left--) {
	    unsigned b = *psrc++;

	    *bufp = map[b >> 6];
	    NEXT_MAP8;
	    bufp += spread;
	    *bufp = map[(b >> 4) & 3];
	    NEXT_MAP8;
	    bufp += spread;
	    *bufp = map[(b >> 2) & 3];
	    NEXT_MAP8;
	    bufp += spread;
	    *bufp = map[b & 3];
	    NEXT_MAP8;
	    bufp += spread;
	}
    }
    *pdata_x = data_x & 3;
    return bptr;
}

#undef NEXT_MAP

const byte *
TEMPLATE_sample_unpack_4(byte * bptr, int *pdata_x, const byte * data, int data_x,
		uint dsize, const sample_map *smap, int spread,
		int num_components_per_plane)
{
    const sample_lookup_t * ptab = &smap->table;
    byte *bufp = bptr;
    const byte *psrc = data + (data_x >> 1);
    int left = dsize - (data_x >> 1);
    const byte *map = &ptab->lookup8[0];
    DEFINE_SMAP_INDEX

    while (left--) {
	uint b = *psrc++;

	*bufp = map[b >> 4];
	NEXT_MAP8;
	bufp += spread;
	*bufp = map[b & 0xf];
	NEXT_MAP8;
	bufp += spread;
    }
    *pdata_x = data_x & 1;
    return bptr;
}

const byte *
TEMPLATE_sample_unpack_8(byte * bptr, int *pdata_x, const byte * data, int data_x,
		uint dsize, const sample_map *smap, int spread,
		int num_components_per_plane)
{
    const sample_lookup_t * ptab = &smap->table;
    byte *bufp = bptr;
    const byte *psrc = data + data_x;
    DEFINE_SMAP_INDEX

    *pdata_x = 0;
    if (spread == 1) {
	if (MULTIPLE_MAPS ||
	    ptab->lookup8[0] != 0 ||
	    ptab->lookup8[255] != 255
	    ) {
	    uint left = dsize - data_x;
	    const byte *map = &ptab->lookup8[0];

	    while (left--) {
		*bufp++ = map[*psrc++];
		NEXT_MAP8;
	    }
	} else {		/* No copying needed, and we'll use the data right away. */
	    return psrc;
	}
    } else {
	int left = dsize - data_x;
	const byte *map = &ptab->lookup8[0];

	for (; left--; psrc++, bufp += spread) {
	    *bufp = map[*psrc];
	    NEXT_MAP8;
	}
    }
    return bptr;
}

#undef NEXT_MAP
#undef NEXT_MAP8
#undef DEFINE_SMAP_INDEX

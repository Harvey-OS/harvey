/* Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxfcmap.h,v 1.16 2004/08/04 19:36:12 stefan Exp $ */
/* Internal CMap structure definitions */

/* This file should be called gxcmap.h, except that name is already used. */

#ifndef gxfcmap_INCLUDED
#  define gxfcmap_INCLUDED

#include "gsfcmap.h"
#include "gsuid.h"
#include "gxcid.h"

/*
 * CMaps are the structures that map (possibly variable-length) characters
 * appearing in a text string to glyph numbers in some font-specific space.
 * The structure defined here generally follows Adobe's specifications, but
 * the actual implementation of the code space and the lookup tables is
 * virtual, so that the same interface can be used for direct access to the
 * corresponding "cmap" structure in TrueType fonts, rather than having to
 * convert that structure to the Adobe-based one.
 */

/*
 * A CMap conceptually consists of three parts:
 *
 *	- The code space, used for parsing the input string into (possibly
 *	  variable-length) characters.
 *
 *	- A 'def' map, which maps defined parsed characters to values.
 *
 *	- A 'notdef' map, which maps parsed but undefined characters to
 *	  values.
 *
 * The value of a character may be a string, a name, or a CID.  For more
 * information, see the Adobe documentation.
 */

/* ---------------- Code space ranges ---------------- */

/*
 * A code space is a non-empty, lexicographically sorted sequence of
 * code space ranges.  Ranges must not overlap.  In each range,
 * first[i] <= last[i] for 0 <= i < size.
 */
#define MAX_CMAP_CODE_SIZE 4
typedef struct gx_code_space_range_s {
    byte first[MAX_CMAP_CODE_SIZE];
    byte last[MAX_CMAP_CODE_SIZE];
    int size;			/* 1 .. MAX_CMAP_CODE_SIZE */
} gx_code_space_range_t;

/* ---------------- Lookup tables ---------------- */

/*
 * A lookup table is a non-empty sequence of lookup ranges.  Each range has
 * an associated sorted lookup table, indexed by the num_key_bytes low-order
 * code bytes.  If key_is_range is true, each key is a range (2 x key_size
 * bytes); if false, each key is a single code (key_size bytes).
 *
 * The only difference between CODE_VALUE_CID and CODE_VALUE_NOTDEF is
 * that after looking up a CID in a table, for CODE_VALUE_CID the result
 * is incremented by the difference between the input code and the key
 * (i.e., a single CODE_VALUE_CID entry actually represents a range of
 * CIDs), whereas for CODE_VALUE_NOTDEF, the result is not incremented.
 * The defined-character map for a CMap uses the former behavior; the
 * notdef map uses the latter.
 *
 * CODE_VALUE_GLYPH and CODE_VALUE_CHARS are reserved for
 * rearranged font CMaps, which are not implemented yet.
 */
typedef enum {
    CODE_VALUE_CID,		/* CIDs */
    CODE_VALUE_GLYPH,		/* glyphs */
    CODE_VALUE_CHARS,		/* character(s) */
    CODE_VALUE_NOTDEF		/* CID - for notdef(char|range) dst */
#define CODE_VALUE_MAX CODE_VALUE_NOTDEF
} gx_cmap_code_value_type_t;
typedef struct gx_cmap_lookup_entry_s {
    /* Key */
    byte key[2][MAX_CMAP_CODE_SIZE]; /* [key_is_range + 1][key_size] */
    int key_size;		/* 0 .. MAX_CMAP_CODE_SIZE */
    bool key_is_range;
    /* Value */
    gx_cmap_code_value_type_t value_type;
    gs_const_string value;
    int font_index;		/* for rearranged fonts */
} gx_cmap_lookup_entry_t;

/* ---------------- CMaps proper ---------------- */

/*
 * Define the elements common to all CMaps.  Currently we include all
 * elements from the Adobe specification except for the actual code space
 * ranges and lookup tables.
 *
 * CMapType and id are common to all CMapTypes.  We really only support the
 * single Adobe standard CMap format.  Note that the only documented values
 * of CMapType in the PLRM are 0 and 1, which are equivalent; however, in
 * the second PDF Reference, the CMapType for the example ToUnicode CMap is
 * 2.
 *
 * glyph_name and glyph_name_data are only used if the CMap has lookup
 * entries of type CODE_VALUE_GLYPH.  We deliberately chose to make
 * glyph_name a function pointer rather than including it in the procs
 * virtual functions.  The rationale is that the virtual functions are
 * dependent on the representation of the CMap, so they should be set by the
 * code that must work with this structure.  However, glyph_name is not
 * dependent on the representation of the CMap: it does not need to know
 * anything about how the CMap is stored.  Rather, it is meant to be used by
 * the client who constructs the CMap, who decides how stored
 * CODE_VALUE_GLYPH values correspond to printable glyph names.  The same
 * glyph_name procedure can, in principle, be used with multiple different
 * subclasses of gs_cmap_t.
 */
#ifndef gs_cmap_DEFINED
#  define gs_cmap_DEFINED
typedef struct gs_cmap_s gs_cmap_t;
#endif

#define GS_CMAP_COMMON\
    int CMapType;		/* must be first */\
    gs_id id;			/* internal ID (no relation to UID) */\
	/* End of entries common to all CMapTypes */\
    gs_const_string CMapName;\
    gs_cid_system_info_t *CIDSystemInfo; /* [num_fonts] */\
    int num_fonts;\
    float CMapVersion;\
    gs_uid uid;			/* XUID or nothing */\
    long UIDOffset;\
    int WMode;\
    bool from_Unicode;		/* if true, characters are Unicode */\
    bool ToUnicode;             /* if true, it is a ToUnicode CMap */\
    gs_glyph_name_proc_t glyph_name;  /* glyph name procedure for printing */\
    void *glyph_name_data;	/* closure data */\
    const gs_cmap_procs_t *procs

extern_st(st_cmap);
#define public_st_cmap()	/* in gsfcmap.c */\
  BASIC_PTRS(cmap_ptrs) {\
    GC_CONST_STRING_ELT(gs_cmap_t, CMapName),\
    GC_OBJ_ELT3(gs_cmap_t, CIDSystemInfo, uid.xvalues, glyph_name_data)\
  };\
  gs_public_st_basic(st_cmap, gs_cmap_t, "gs_cmap_t", cmap_ptrs, cmap_data)

typedef struct gs_cmap_ranges_enum_s gs_cmap_ranges_enum_t;
typedef struct gs_cmap_lookups_enum_s gs_cmap_lookups_enum_t;

typedef struct gs_cmap_procs_s {

    /*
     * Decode and map a character from a string using a CMap.
     * See gsfcmap.h for details.
     */

    int (*decode_next)(const gs_cmap_t *pcmap, const gs_const_string *str,
		       uint *pindex, uint *pfidx,
		       gs_char *pchr, gs_glyph *pglyph);

    /*
     * Initialize an enumeration of code space ranges.  See below.
     */

    void (*enum_ranges)(const gs_cmap_t *pcmap,
			gs_cmap_ranges_enum_t *penum);

    /*
     * Initialize an enumeration of lookups.  See below.
     */

    void (*enum_lookups)(const gs_cmap_t *pcmap, int which,
			 gs_cmap_lookups_enum_t *penum);

    /*
     * Check if the cmap is identity.
     */

    bool (*is_identity)(const gs_cmap_t *pcmap, int font_index_only);

} gs_cmap_procs_t;

struct gs_cmap_s {
    GS_CMAP_COMMON;
};

/* ---------------- Enumerators ---------------- */

/*
 * Define enumeration structures for code space ranges and lookup tables.
 * Since all current and currently envisioned implementations are very
 * simple, we don't bother to make this fully general, with subclasses
 * or a "finish" procedure.
 */
typedef struct gs_cmap_ranges_enum_procs_s {
    int (*next_range)(gs_cmap_ranges_enum_t *penum);
} gs_cmap_ranges_enum_procs_t;
struct gs_cmap_ranges_enum_s {
    /*
     * Return the next code space range here.
     */
    gx_code_space_range_t range;
    /*
     * The rest of the information is private to the implementation.
     */
    const gs_cmap_t *cmap;
    const gs_cmap_ranges_enum_procs_t *procs;
    uint index;
};

typedef struct gs_cmap_lookups_enum_procs_s {
    int (*next_lookup)(gs_cmap_lookups_enum_t *penum);
    int (*next_entry)(gs_cmap_lookups_enum_t *penum);
} gs_cmap_lookups_enum_procs_t;
struct gs_cmap_lookups_enum_s {
    /*
     * Return the next lookup and entry here.
     */
    gx_cmap_lookup_entry_t entry;
    /*
     * The rest of the information is private to the implementation.
     */
    const gs_cmap_t *cmap;
    const gs_cmap_lookups_enum_procs_t *procs;
    uint index[2];
    byte temp_value[max(sizeof(gs_glyph), sizeof(gs_char))];
};
/*
 * Define a vacuous next_lookup procedure, useful for the notdef lookups
 * for CMaps that don't have any.
 */
extern const gs_cmap_lookups_enum_procs_t gs_cmap_no_lookups_procs;

/* ---------------- Client procedures ---------------- */

/*
 * Initialize the enumeration of the code space ranges, and enumerate
 * the next range.  enum_next returns 0 if OK, 1 if finished, <0 if error.
 * The intended usage is:
 *
 *	for (gs_cmap_ranges_enum_init(pcmap, &renum);
 *	     (code = gs_cmap_enum_next_range(&renum)) == 0; ) {
 *	    ...
 *	}
 *	if (code < 0) <<error>>
 */
void gs_cmap_ranges_enum_init(const gs_cmap_t *pcmap,
			      gs_cmap_ranges_enum_t *penum);
int gs_cmap_enum_next_range(gs_cmap_ranges_enum_t *penum);

/*
 * Initialize the enumeration of the lookups, and enumerate the next
 * the next lookup or entry.  which = 0 for defined characters,
 * which = 1 for notdef.  next_xxx returns 0 if OK, 1 if finished,
 * <0 if error.  The intended usage is:
 *
 *	for (gs_cmap_lookups_enum_init(pcmap, which, &lenum);
 *	     (code = gs_cmap_enum_next_lookup(&lenum)) == 0; ) {
 *	    while ((code = gs_cmap_enum_next_entry(&lenum)) == 0) {
 *		...
 *	    }
 *	    if (code < 0) <<error>>
 *	}
 *	if (code < 0) <<error>>
 *
 * Note that next_lookup sets (at least) penum->entry.
 *	key_size, key_is_range, value_type, font_index
 * whereas next_entry sets penum->entry.
 *	key[0][*], key[1][*], value
 * Clients must not modify any members of the enumerator.
 * The bytes of the value string may be allocated locally (in the enumerator
 * itself) and not survive from one call to the next.
 */
void gs_cmap_lookups_enum_init(const gs_cmap_t *pcmap, int which,
			       gs_cmap_lookups_enum_t *penum);
int gs_cmap_enum_next_lookup(gs_cmap_lookups_enum_t *penum);
int gs_cmap_enum_next_entry(gs_cmap_lookups_enum_t *penum);

/* ---------------- Implementation procedures ---------------- */

/*
 * Initialize a just-allocated CMap, to ensure that all pointers are clean
 * for the GC.  Note that this only initializes the common part.
 */
void gs_cmap_init(const gs_memory_t *mem, gs_cmap_t *pcmap, int num_fonts);

/*
 * Allocate and initialize (the common part of) a CMap.
 */
int gs_cmap_alloc(gs_cmap_t **ppcmap, const gs_memory_struct_type_t *pstype,
		  int wmode, const byte *map_name, uint name_size,
		  const gs_cid_system_info_t *pcidsi, int num_fonts,
		  const gs_cmap_procs_t *procs, gs_memory_t *mem);

/*
 * Initialize an enumerator with convenient defaults (index = 0).
 */
void gs_cmap_ranges_enum_setup(gs_cmap_ranges_enum_t *penum,
			       const gs_cmap_t *pcmap,
			       const gs_cmap_ranges_enum_procs_t *procs);
void gs_cmap_lookups_enum_setup(gs_cmap_lookups_enum_t *penum,
				const gs_cmap_t *pcmap,
				const gs_cmap_lookups_enum_procs_t *procs);

/* 
 * Check for identity CMap. Uses a fast check for special cases.
 */
bool gs_cmap_is_identity(const gs_cmap_t *pcmap, int font_index_only);

/* 
 * For a random CMap, compute whether it is identity.
 * It is not applicable to gs_cmap_ToUnicode_t due to
 * different sizes of domain keys and range values.
 */
bool gs_cmap_compute_identity(const gs_cmap_t *pcmap, int font_index_only);

#endif /* gxfcmap_INCLUDED */

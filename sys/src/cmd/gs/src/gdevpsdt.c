/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevpsdt.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* Write an embedded TrueType font */
#include "memory_.h"
#include <stdlib.h>		/* for qsort */
#include "gx.h"
#include "gserrors.h"
#include "gsmatrix.h"
#include "gsutil.h"
#include "gxfont.h"
#include "gxfont42.h"
#include "stream.h"
#include "spprint.h"
#include "gdevpsdf.h"

#define ACCESS(base, length, vptr)\
  BEGIN\
    code = string_proc(pfont, (ulong)(base), length, &vptr);\
    if ( code < 0 ) return code;\
  END

/* Pad to a multiple of 4 bytes. */
private void
put_pad(stream *s, uint length)
{
    static const byte pad_to_4[3] = {0, 0, 0};

    pwrite(s, pad_to_4, (uint)(-length & 3));
}

/* Put short and long values on a stream. */
private void
put_ushort(stream *s, uint v)
{
    pputc(s, (byte)(v >> 8));
    pputc(s, (byte)v);
}
private void
put_ulong(stream *s, ulong v)
{
    put_ushort(s, (uint)(v >> 16));
    put_ushort(s, (uint)v);
}
private void
put_loca(stream *s, ulong offset, int indexToLocFormat)
{
    if (indexToLocFormat)
	put_ulong(s, offset);
    else
	put_ushort(s, (uint)(offset >> 1));
}

/* Get or put 2- or 4-byte quantities from/into a table. */
#define U8(p) ((uint)((p)[0]))
#define S8(p) (int)((U8(p) ^ 0x80) - 0x80)
#define U16(p) (((uint)((p)[0]) << 8) + (p)[1])
#define S16(p) (int)((U16(p) ^ 0x8000) - 0x8000)
#define u32(p) get_u32_msb(p)
private void
put_u16(byte *p, uint v)
{
    p[0] = (byte)(v >> 8);
    p[1] = (byte)v;
}
private void
put_u32(byte *p, ulong v)
{
    p[0] = (byte)(v >> 24);
    p[1] = (byte)(v >> 16);
    p[2] = (byte)(v >> 8);
    p[3] = (byte)v;
}
private ulong
put_table(byte tab[16], const char *tname, ulong checksum, ulong offset,
	  uint length)
{
    memcpy(tab, (const byte *)tname, 4);
    put_u32(tab + 4, checksum);
    put_u32(tab + 8, offset + 0x40000000);
    put_u32(tab + 12, (ulong)length);
    return offset + round_up(length, 4);
}

/* Write one range of a TrueType font. */
private int
write_range(stream *s, gs_font_type42 *pfont, ulong start, uint length)
{
    ulong base = start;
    ulong limit = base + length;
    int (*string_proc)(P4(gs_font_type42 *, ulong, uint, const byte **)) =
	pfont->data.string_proc;

    if_debug3('l', "[l]write_range pos = %ld, start = %lu, length = %u\n",
	      stell(s), start, length);
    while (base < limit) {
	uint size = limit - base;
	const byte *ptr;
	int code;

	/* Write the largest block we can access consecutively. */
	while ((code = string_proc(pfont, base, size, &ptr)) < 0) {
	    if (size <= 1)
		return code;
	    size >>= 1;
	}
	pwrite(s, ptr, size);
	base += size;
    }
    return 0;
}

/*
 * Determine the Macintosh glyph number for a given character, if any.
 * If no glyph can be found, return -1 and store the name in *pstr.
 */
private int
mac_glyph_index(gs_font *font, int ch, gs_const_string *pstr)
{
    gs_glyph glyph = font->procs.encode_char(font, (gs_char)ch,
					     GLYPH_SPACE_NAME);

    if (glyph == gs_no_glyph)
	return 0;		/* .notdef */
    pstr->data = (const byte *)
	font->procs.callbacks.glyph_name(glyph, &pstr->size);
    if (glyph < gs_min_cid_glyph) {
	gs_char mac_char;
	gs_glyph mac_glyph;
	gs_const_string mstr;

	/* Look (not very hard) for a match in the Mac glyph space. */
	if (ch >= 32 && ch <= 126)
	    mac_char = ch - 29;
	else if (ch >= 128 && ch <= 255)
	    mac_char = ch - 30;
	else
	    return -1;
	mac_glyph =
	    font->procs.callbacks.known_encode(mac_char,
					       ENCODING_INDEX_MACGLYPH);
	if (mac_glyph == gs_no_glyph)
	    return -1;
	mstr.data =(const byte *)
	    font->procs.callbacks.glyph_name(mac_glyph, &mstr.size);
	if (!bytes_compare(pstr->data, pstr->size, mstr.data, mstr.size))
	    return (int)mac_char;
    }
    return -1;
}

/* Write a generated cmap table. */
static const byte cmap_initial_0[] = {
    0, 0,		/* table version # = 0 */
    0, 2,		/* # of encoding tables = 2 */

	/* First table, Macintosh */
    0, 1,		/* platform ID = Macintosh */
    0, 0,		/* platform encoding ID = ??? */
    0, 0, 0, 4+8+8,	/* offset to table start */
	/* Second table, Windows */
    0, 3,		/* platform ID = Microsoft */
    0, 0,		/* platform encoding ID = unknown */
    0, 0, 1, 4+8+8+6,	/* offset to table start */

	/* Start of Macintosh format 0 table */
    0, 0,		/* format = 0, byte encoding table */
    1, 6,		/* length */
    0, 0		/* version number */
};
static const byte cmap_initial_6[] = {
    0, 0,		/* table version # = 0 */
    0, 2,		/* # of encoding tables = 2 */

	/* First table, Macintosh */
    0, 1,		/* platform ID = Macintosh */
    0, 0,		/* platform encoding ID = ??? */
    0, 0, 0, 4+8+8,	/* offset to table start */
	/* Second table, Windows */
    0, 3,		/* platform ID = Microsoft */
    0, 0,		/* platform encoding ID = unknown */
    0, 0, 0, 4+8+8+10,	/* offset to table start */
			/****** VARIABLE, add 2 x # of entries ******/

	/* Start of Macintosh format 6 table */
    0, 6,		/* format = 6, trimmed table mapping */
    0, 10,		/* length ****** VARIABLE, add 2 x # of entries ******/
    0, 0,		/* version number */
    0, 0,		/* first character code */
    0, 0		/* # of entries ****** VARIABLE ****** */
};
static const byte cmap_initial_4[] = {
    0, 0,		/* table version # = 0 */
    0, 1,		/* # of encoding tables = 2 */

	/* Single table, Windows */
    0, 3,		/* platform ID = Microsoft */
    0, 0,		/* platform encoding ID = unknown */
    0, 0, 0, 4+8	/* offset to table start */
};
static const byte cmap_sub_initial[] = {
    0, 4,		/* format = 4, segment mapping */
    0, 32,		/* length ** VARIABLE, add 2 x # of glyphs ** */
    0, 0,		/* version # */
    0, 4,		/* 2 x segCount */
    0, 4,		/* searchRange = 2 x 2 ^ floor(log2(segCount)) */
    0, 1,		/* floor(log2(segCount)) */
    0, 0,		/* 2 x segCount - searchRange */

    0, 0,		/* endCount[0] **VARIABLE** */
    255, 255,		/* endCount[1] */
    0, 0,		/* reservedPad */
    0, 0,		/* startCount[0] **VARIABLE** */
    255, 255,		/* startCount[1] */
    0, 0,		/* idDelta[0] */
    0, 1,		/* idDelta[1] */
    0, 4,		/* idRangeOffset[0] */
    0, 0		/* idRangeOffset[1] */
};
private void
write_cmap(stream *s, gs_font *font, uint first_code, int num_glyphs,
	   gs_glyph max_glyph, int options, uint cmap_length)
{
    byte cmap_sub[sizeof(cmap_sub_initial)];
    byte entries[256 * 2];
    int first_entry = 0, end_entry = num_glyphs;
    bool can_use_trimmed = !(options & WRITE_TRUETYPE_NO_TRIMMED_TABLE);
    uint merge = 0;
    uint num_entries;
    int i;

    /* Collect the table entries. */

    for (i = 0; i < num_glyphs; ++i) {
	gs_glyph glyph =
	    font->procs.encode_char(font, (gs_char)i, GLYPH_SPACE_INDEX);
	uint glyph_index;

	if (glyph == gs_no_glyph || glyph < gs_min_cid_glyph ||
	    glyph > max_glyph
	    )
	    glyph = gs_min_cid_glyph;
	glyph_index = (uint)(glyph - gs_min_cid_glyph);
	merge |= glyph_index;
	put_u16(entries + 2 * i, glyph_index);
    }
    while (end_entry > first_entry && !U16(entries + 2 * end_entry - 2))
	--end_entry;
    while (first_entry < end_entry && !U16(entries + 2 * first_entry))
	++first_entry;
    num_entries = end_entry - first_entry;

    /* Write the table header and Macintosh sub-table (if any). */
    if (merge == (byte)merge && (num_entries <= 127 || !can_use_trimmed)) {
	/* Use byte encoding format. */
	memset(entries + 2 * num_glyphs, 0,
	       sizeof(entries) - 2 * num_glyphs);
	pwrite(s, cmap_initial_0, sizeof(cmap_initial_0));
	for (i = 0; i <= 0xff; ++i)
	    sputc(s, (byte)entries[2 * i + 1]);
    } else if (can_use_trimmed) {
	/* Use trimmed table format. */
	byte cmap_data[sizeof(cmap_initial_6)];

	memcpy(cmap_data, cmap_initial_6, sizeof(cmap_initial_6));
	put_u16(cmap_data + 18,
		U16(cmap_data + 18) + num_entries * 2);  /* offset */
	put_u16(cmap_data + 22,
		U16(cmap_data + 22) + num_entries * 2);  /* length */
	put_u16(cmap_data + 26, first_code + first_entry);
	put_u16(cmap_data + 28, num_entries);
	pwrite(s, cmap_data, sizeof(cmap_data));
	pwrite(s, entries + first_entry * 2, num_entries * 2);
    } else {
	/*
	 * Punt.  Acrobat Reader 3 can't handle any other Mac table format.
	 * (AR3 for Linux doesn't seem to be able to handle Windows format,
	 * either, but maybe AR3 for Windows can.)
	 */
	pwrite(s, cmap_initial_4, sizeof(cmap_initial_4));
    }

    /* Write the Windows sub-table. */

    memcpy(cmap_sub, cmap_sub_initial, sizeof(cmap_sub_initial));
    put_u16(cmap_sub + 2, U16(cmap_sub + 2) + num_entries * 2); /* length */
    put_u16(cmap_sub + 14, first_code + end_entry - 1); /* endCount[0] */
    put_u16(cmap_sub + 20, first_code + first_entry); /* startCount[0] */
    pwrite(s, cmap_sub, sizeof(cmap_sub));
    pwrite(s, entries + first_entry * 2, num_entries * 2);
    put_pad(s, cmap_length);
}
private uint
size_cmap(gs_font *font, uint first_code, int num_glyphs, gs_glyph max_glyph,
	  int options)
{
    stream poss;

    swrite_position_only(&poss);
    write_cmap(&poss, font, first_code, num_glyphs, max_glyph, options, 0);
    return stell(&poss);
}

/* Write a generated name table. */
static const byte name_initial[] = {
    0, 0,			/* format */
    0, 1,			/* # of records = 1 */
    0, 18,			/* start of string storage */

    0, 2,			/* platform ID = ISO */
    0, 2,			/* encoding ID = ISO 8859-1 */
    0, 0,			/* language ID (none) */
    0, 6,			/* name ID = PostScript name */
    0, 0,			/* length ****** VARIABLE ****** */
    0, 0			/* start of string within string storage */
};
private uint
size_name(const gs_const_string *font_name)
{
    return sizeof(name_initial) + font_name->size;
}
private void
write_name(stream *s, const gs_const_string *font_name)
{
    byte name_bytes[sizeof(name_initial)];

    memcpy(name_bytes, name_initial, sizeof(name_initial));
    put_u16(name_bytes + 14, font_name->size);
    pwrite(s, name_bytes, sizeof(name_bytes));
    pwrite(s, font_name->data, font_name->size);
    put_pad(s, size_name(font_name));
}

/* Write a generated OS/2 table. */
typedef struct OS_2_s {
    byte
	version[2],		/* version 1 */
	xAvgCharWidth[2],
	usWeightClass[2],
	usWidthClass[2],
	fsType[2],
	ySubscriptXSize[2],
	ySubscriptYSize[2],
	ySubscriptXOffset[2],
	ySubscriptYOffset[2],
	ySuperscriptXSize[2],
	ySuperscriptYSize[2],
	ySuperscriptXOffset[2],
	ySuperscriptYOffset[2],
	yStrikeoutSize[2],
	yStrikeoutPosition[2],
	sFamilyClass[2],
	/*panose:*/
	    bFamilyType, bSerifStyle, bWeight, bProportion, bContrast,
	    bStrokeVariation, bArmStyle, bLetterform, bMidline, bXHeight,
	ulUnicodeRanges[16],
	achVendID[4],
	fsSelection[2],
	usFirstCharIndex[2],
	usLastCharIndex[2],
	sTypoAscender[2],
	sTypoDescender[2],
	sTypoLineGap[2],
	usWinAscent[2],
	usWinDescent[2],
	ulCodePageRanges[8];
} OS_2_t;
#define OS_2_LENGTH sizeof(OS_2_t)
private void
update_OS_2(OS_2_t *pos2, uint first_glyph, int num_glyphs)
{
    put_u16(pos2->usFirstCharIndex, first_glyph);
    put_u16(pos2->usLastCharIndex, first_glyph + num_glyphs - 1);
}
private void
write_OS_2(stream *s, gs_font *font, uint first_glyph, int num_glyphs)
{
    OS_2_t os2;

    /*
     * We don't bother to set most of the fields.  The really important
     * ones, which affect character mapping, are usFirst/LastCharIndex.
     * We also need to set usWeightClass and usWidthClass to avoid
     * crashing ttfdump.
     */
    memset(&os2, 0, sizeof(os2));
    put_u16(os2.version, 1);
    put_u16(os2.usWeightClass, 400); /* Normal */
    put_u16(os2.usWidthClass, 5); /* Normal */
    update_OS_2(&os2, first_glyph, num_glyphs);
    if (first_glyph >= 0xf000)
	os2.ulCodePageRanges[3] = 1; /* bit 31, symbolic */
    pwrite(s, &os2, sizeof(os2));
    put_pad(s, sizeof(os2));
}

/* Construct and then write the post table. */
typedef struct post_glyph_s {
    byte char_index;
    byte size;
    ushort glyph_index;
} post_glyph_t;
private int
compare_post_glyphs(const void *pg1, const void *pg2)
{
    gs_glyph g1 = ((const post_glyph_t *)pg1)->glyph_index,
	g2 = ((const post_glyph_t *)pg2)->glyph_index;

    return (g1 < g2 ? -1 : g1 > g2 ? 1 : 0);
}
typedef struct post_s {
    post_glyph_t glyphs[256 + 1];
    int count, glyph_count;
    uint length;
} post_t;

/*
 * If necessary, compute the length of the post table.  Note that we
 * only generate post entries for characters in the Encoding.
 */
private void
compute_post(gs_font *font, post_t *post)
{
    int i;

    for (i = 0, post->length = 32 + 2; i <= 255; ++i) {
	gs_const_string str;
	gs_glyph glyph = font->procs.encode_char(font, (gs_char)i,
						 GLYPH_SPACE_INDEX);
	int mac_index = mac_glyph_index(font, i, &str);

	if (mac_index != 0) {
	    post->glyphs[post->count].char_index = i;
	    post->glyphs[post->count].size =
		(mac_index < 0 ? str.size + 1 : 0);
	    post->glyphs[post->count].glyph_index = glyph - gs_min_cid_glyph;
	    post->count++;
	}
    }
    if (post->count) {
	int j;

	qsort(post->glyphs, post->count, sizeof(post->glyphs[0]),
	      compare_post_glyphs);
	/* Eliminate duplicate references to the same glyph. */
	for (i = j = 0; i < post->count; ++i) {
	    if (i == 0 ||
		post->glyphs[i].glyph_index !=
		post->glyphs[i - 1].glyph_index
		) {
		post->length += post->glyphs[i].size;
		post->glyphs[j++] = post->glyphs[i];
	    }
	}
	post->count = j;
	post->glyph_count = post->glyphs[post->count - 1].glyph_index + 1;
    }
    post->length += post->glyph_count * 2;
}

/* Write the post table */
private void
write_post(stream *s, gs_font *font, post_t *post)
{
    byte post_initial[32 + 2];
    uint name_index;
    uint glyph_index;
    int i;

    memset(post_initial, 0, 32);
    put_u32(post_initial, 0x00020000);
    put_u16(post_initial + 32, post->glyph_count);
    pwrite(s, post_initial, sizeof(post_initial));

    /* Write the name index table. */

    for (i = 0, name_index = 258, glyph_index = 0; i < post->count; ++i) {
	gs_const_string str;
	int ch = post->glyphs[i].char_index;
	int mac_index = mac_glyph_index(font, ch, &str);

	for (; glyph_index < post->glyphs[i].glyph_index; ++glyph_index)
	    put_ushort(s, 0);
	glyph_index++;
	if (mac_index >= 0)
	    put_ushort(s, mac_index);
	else {
	    put_ushort(s, name_index);
	    name_index++;
	}
    }

    /* Write the string names of the glyphs. */

    for (i = 0; i < post->count; ++i) {
	gs_const_string str;
	int ch = post->glyphs[i].char_index;
	int mac_index = mac_glyph_index(font, ch, &str);

	if (mac_index < 0) {
	    spputc(s, str.size);
	    pwrite(s, str.data, str.size);
	}
    }
    put_pad(s, post->length);
}

/* ---------------- Main program ---------------- */

/* Write the definition of a TrueType font. */
private int
compare_table_tags(const void *pt1, const void *pt2)
{
    ulong t1 = u32(pt1), t2 = u32(pt2);

    return (t1 < t2 ? -1 : t1 > t2 ? 1 : 0);
}
int
psdf_write_truetype_font(stream *s, gs_font_type42 *pfont, int options,
			 gs_glyph *orig_subset_glyphs, uint orig_subset_size,
			 const gs_const_string *alt_font_name)
{
    gs_font *const font = (gs_font *)pfont;
    gs_const_string font_name;
    int (*string_proc)(P4(gs_font_type42 *, ulong, uint, const byte **)) =
	pfont->data.string_proc;
    const byte *OffsetTable;
    uint numTables_stored, numTables, numTables_out;
#define MAX_NUM_TABLES 40
    byte tables[MAX_NUM_TABLES * 16];
    uint i;
    psdf_glyph_enum_t genum;
    ulong offset;
    gs_glyph glyph, glyph_prev;
    ulong max_glyph;
    uint glyf_length, glyf_checksum = 0 /****** BOGUS ******/;
    uint loca_length, loca_checksum[2];
    uint numGlyphs;		/* original value from maxp */
    byte head[56];		/* 0 mod 4 */
    post_t post;
    ulong head_checksum, file_checksum = 0;
    int indexToLocFormat;
    bool have_cmap = false,
	have_name = !(options & WRITE_TRUETYPE_NAME),
	have_OS_2 = false,
	have_post = false;
    uint cmap_length;
    ulong OS_2_start;
    uint OS_2_length = OS_2_LENGTH;
    gs_glyph subset_data[256 * 3];  /* *3 for composites */
    gs_glyph *subset_glyphs = orig_subset_glyphs;
    uint subset_size = orig_subset_size;
    int code;

    if (alt_font_name)
	font_name = *alt_font_name;
    else
	font_name.data = font->font_name.chars,
	    font_name.size = font->font_name.size;

    /* Sort the subset glyphs, if any. */

    if (subset_glyphs) {
	/* Add the component glyphs for composites. */
	memcpy(subset_data, orig_subset_glyphs,
	       sizeof(gs_glyph) * subset_size);
	subset_glyphs = subset_data;
	code = psdf_add_subset_pieces(subset_glyphs, &subset_size,
				      countof(subset_data),
				      countof(subset_data),
				      font);
	if (code < 0)
	    return code;
	subset_size = psdf_sort_glyphs(subset_glyphs, subset_size);
    }

    /*
     * Count the number of tables, including the eventual glyf and loca
     * (which may not actually be present in the font), and copy the
     * table directory.
     */

    ACCESS(0, 12, OffsetTable);
    numTables_stored = U16(OffsetTable + 4);
    for (i = numTables = 0; i < numTables_stored; ++i) {
	const byte *tab;
	const byte *data;
	ulong start;
	uint length;

	ACCESS(12 + i * 16, 16, tab);
	start = u32(tab + 8);
	length = u32(tab + 12);
	if (!memcmp(tab, "head", 4)) {
	    if (length != 54)
		return_error(gs_error_invalidfont);
	    ACCESS(start, length, data);
	    memcpy(head, data, length);
	} else if (
	    !memcmp(tab, "gly", 3) /*glyf=synthesized, glyx=Adobe bogus*/ ||
	    !memcmp(tab, "loc", 3) /*loca=synthesized, locx=Adobe bogus*/ ||
	    !memcmp(tab, "gdir", 4) /*Adobe marker*/ ||
	    ((options & WRITE_TRUETYPE_CMAP) && !memcmp(tab, "cmap", 4))
	    )
	    DO_NOTHING;
	else {
	    if (numTables == MAX_NUM_TABLES)
		return_error(gs_error_limitcheck);
	    if (!memcmp(tab, "cmap", 4))
		have_cmap = true;
	    else if (!memcmp(tab, "maxp", 4)) {
		ACCESS(start, length, data);
		numGlyphs = U16(data + 4);
	    } else if (!memcmp(tab, "name", 4))
		have_name = true;
	    else if (!memcmp(tab, "OS/2", 4)) {
		have_OS_2 = true;
		if (length > OS_2_LENGTH)
		    return_error(gs_error_invalidfont);
		OS_2_start = start;
		OS_2_length = length;
		continue;
	    } else if (!memcmp(tab, "post", 4))
		have_post = true;
	    memcpy(&tables[numTables++ * 16], tab, 16);
	}
    }

    /*
     * Enumerate the glyphs to get the size of glyf and loca,
     * and to compute the checksums for these tables.
     */

    /****** NO CHECKSUMS YET ******/
    psdf_enumerate_glyphs_begin(&genum, font, subset_glyphs,
				(subset_glyphs ? subset_size : 0),
				GLYPH_SPACE_INDEX);
    for (max_glyph = 0, glyf_length = 0;
	 (code = psdf_enumerate_glyphs_next(&genum, &glyph)) != 1;
	 ) {
	uint glyph_index;
	gs_const_string glyph_string;

	if (glyph < gs_min_cid_glyph)
	    return_error(gs_error_invalidfont);
	glyph_index = glyph - gs_min_cid_glyph;
	if_debug1('L', "[L]glyph_index %u\n", glyph_index);
	if (pfont->data.get_outline(pfont, glyph_index, &glyph_string) >= 0) {
	    max_glyph = max(max_glyph, glyph_index);
	    glyf_length += glyph_string.size;
	    if_debug1('L', "[L]  size %u\n", glyph_string.size);
	}
    }
    if_debug2('l', "[l]max_glyph = %lu, glyf_length = %lu\n",
	      (ulong)max_glyph, (ulong)glyf_length);
    /*
     * For subset fonts, we should trim the loca table so that it only
     * contains entries through max_glyph.  Unfortunately, this would
     * require changing numGlyphs in maxp, which in turn would affect hdmx,
     * hhea, hmtx, vdmx, vhea, vmtx, and possibly other tables.  This is way
     * more work than we want to do right now.
     */
    /*loca_length = (max_glyph + 2) << 2;*/
    loca_length = (numGlyphs + 1) << 2;
    indexToLocFormat = (glyf_length > 0x1fffc);
    if (!indexToLocFormat)
	loca_length >>= 1;

    /*
     * If necessary, compute the length of the post table.  Note that we
     * only generate post entries for characters in the Encoding.  */

    if (!have_post) {
	memset(&post, 0, sizeof(post));
	if (options & WRITE_TRUETYPE_POST)
	    compute_post(font, &post);
	else
	    post.length = 32;	/* dummy table */
    }

    /* Fix up the head table. */

    memset(head + 8, 0, 4);
    head[51] = (byte)indexToLocFormat;
    memset(head + 54, 0, 2);
    for (head_checksum = 0, i = 0; i < 56; i += 4)
	head_checksum += u32(&head[i]);

    /*
     * Construct the table directory, except for glyf, loca, head, OS/2,
     * and, if necessary, generated cmap, name, and post tables.
     * Note that the existing directory is already sorted by tag.
     */

    numTables_out = numTables + 4 +
	!have_cmap + !have_name + !have_post;
    offset = 12 + numTables_out * 16;
    for (i = 0; i < numTables; ++i) {
	byte *tab = &tables[i * 16];
	ulong length = u32(tab + 12);

	offset += round_up(length, 4);
    }

    /* Make the table directory entries for generated tables. */

    {
	byte *tab = &tables[numTables * 16];

	offset = put_table(tab, "glyf", glyf_checksum, offset, glyf_length);
	tab += 16;

	offset = put_table(tab, "loca", loca_checksum[indexToLocFormat],
			   offset, loca_length);
	tab += 16;

	if (!have_cmap) {
	    cmap_length = size_cmap(font, 0xf000, 256,
				    gs_min_cid_glyph + max_glyph, options);
	    offset = put_table(tab, "cmap", 0L /****** NO CHECKSUM ******/,
			       offset, cmap_length);
	    tab += 16;
	}

	if (!have_name) {
	    offset = put_table(tab, "name", 0L /****** NO CHECKSUM ******/,
			       offset, size_name(&font_name));
	    tab += 16;
	}

	offset = put_table(tab, "OS/2", 0L /****** NO CHECKSUM ******/,
			   offset, OS_2_length);
	tab += 16;

	if (!have_post) {
	    offset = put_table(tab, "post", 0L /****** NO CHECKSUM ******/,
			       offset, post.length);
	    tab += 16;
	}

	offset = put_table(tab, "head", head_checksum, offset, 56);
	tab += 16;
    }
    numTables = numTables_out;

    /* Write the font header. */

    {
	static const byte version[4] = {0, 1, 0, 0};

	pwrite(s, version, 4);
    }
    put_ushort(s, numTables);
    for (i = 0; 1 << i <= numTables; ++i)
	DO_NOTHING;
    --i;
    put_ushort(s, 16 << i);	/* searchRange */
    put_ushort(s, i);		/* entrySelectors */
    put_ushort(s, numTables * 16 - (16 << i)); /* rangeShift */

    /* Write the table directory. */

    qsort(tables, numTables, 16, compare_table_tags);
    offset = 12 + numTables * 16;
    for (i = 0; i < numTables; ++i) {
	const byte *tab = &tables[i * 16];
	byte entry[16];

	memcpy(entry, tab, 16);
	if (entry[8] < 0x40) {
	    /* Not a generated table. */
	    uint length = u32(tab + 12);

	    put_u32(entry + 8, offset);
	    offset += round_up(length, 4);
	} else {
	    entry[8] -= 0x40;
	}
	pwrite(s, entry, 16);
    }

    /* Write tables other than the ones we generate here. */

    for (i = 0; i < numTables; ++i) {
	const byte *tab = &tables[i * 16];

	if (tab[8] < 0x40) {
	    ulong start = u32(tab + 8);
	    uint length = u32(tab + 12);

	    write_range(s, pfont, start, length);
	    put_pad(s, length);
	}
    }

    /* Write glyf. */

    psdf_enumerate_glyphs_begin(&genum, font, subset_glyphs,
				(subset_glyphs ? subset_size : max_glyph + 1),
				GLYPH_SPACE_INDEX);
    for (offset = 0; psdf_enumerate_glyphs_next(&genum, &glyph) != 1; ) {
	gs_const_string glyph_string;

	if (pfont->data.get_outline(pfont, glyph - gs_min_cid_glyph,
				    &glyph_string) >= 0) {
	    pwrite(s, glyph_string.data, glyph_string.size);
	    offset += glyph_string.size;
	    if_debug2('L', "[L]glyf index = %u, size = %u\n",
		      i, glyph_string.size);
	}
    }
    if_debug1('l', "[l]glyf final offset = %lu\n", offset);
    put_pad(s, (uint)offset);

    /* Write loca. */

    psdf_enumerate_glyphs_reset(&genum);
    glyph_prev = gs_min_cid_glyph;
    for (offset = 0; psdf_enumerate_glyphs_next(&genum, &glyph) != 1; ) {
	gs_const_string glyph_string;

	for (; glyph_prev <= glyph; ++glyph_prev)
	    put_loca(s, offset, indexToLocFormat);
	if (pfont->data.get_outline(pfont, glyph - gs_min_cid_glyph,
				    &glyph_string) >= 0)
	    offset += glyph_string.size;
    }
    /* Pad to numGlyphs + 1 entries (including the trailing entry). */
    for (; glyph_prev <= gs_min_cid_glyph + numGlyphs; ++glyph_prev)
	put_loca(s, offset, indexToLocFormat);
    put_pad(s, loca_length);

    /* If necessary, write cmap, name, and OS/2. */

    if (!have_cmap)
	write_cmap(s, font, 0xf000, 256, gs_min_cid_glyph + max_glyph,
		   options, cmap_length);
    if (!have_name)
	write_name(s, &font_name);
    if (!have_OS_2)
	write_OS_2(s, font, 0xf000, 256);
    else if (!have_cmap) {
	/*
	 * Adjust the first and last character indices in the OS/2 table
	 * to reflect the values in the generated cmap.
	 */
	const byte *pos2;
	OS_2_t os2;

	ACCESS(OS_2_start, OS_2_length, pos2);
	memcpy(&os2, pos2, min(OS_2_length, sizeof(os2)));
	update_OS_2(&os2, 0xf000, 256);
	pwrite(s, &os2, OS_2_length);
	put_pad(s, OS_2_length);
    } else {
	/* Just copy the existing OS/2 table. */
	write_range(s, pfont, OS_2_start, OS_2_length);
	put_pad(s, OS_2_length);
    }

    /* If necessary, write post. */

    if (!have_post) {
	if (options & WRITE_TRUETYPE_POST)
	    write_post(s, font, &post);
	else {
	    byte post_initial[32 + 2];

	    memset(post_initial, 0, 32);
	    put_u32(post_initial, 0x00030000);
	    pwrite(s, post_initial, 32);
	}
    }

    /* Write head. */

    /****** CHECKSUM WAS NEVER COMPUTED ******/
    /*
     * The following nonsense is to avoid warnings about the constant
     * 0xb1b0afbaL being "unsigned in ANSI C, signed with -traditional".
     */
#if ARCH_SIZEOF_LONG > ARCH_SIZEOF_INT
#  define HEAD_MAGIC 0xb1b0afbaL
#else
#  define HEAD_MAGIC ((ulong)~0x4e4f5045)
#endif
    put_u32(head + 8, HEAD_MAGIC - file_checksum); /* per spec */
#undef HEAD_MAGIC
    pwrite(s, head, 56);

    return 0;
}

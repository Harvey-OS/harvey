/* Copyright (C) 1999, 2000, 2001 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpsf2.c,v 1.33 2004/11/19 04:39:11 ray Exp $ */
/* Write an embedded CFF font with either Type 1 or Type 2 CharStrings */
#include "math_.h"		/* for fabs */
#include "memory_.h"
#include "gx.h"
#include "gxarith.h"
#include "gscencs.h"
#include "gserrors.h"
#include "gsccode.h"
#include "gscrypt1.h"
#include "gsmatrix.h"
#include "gsutil.h"
#include "gxfixed.h"
#include "gxfont.h"
#include "gxfont1.h"
#include "gxfcid.h"
#include "stream.h"
#include "sfilter.h"
#include "gdevpsf.h"

/* Define additional opcodes used in Dicts, but not in CharStrings. */
#define CD_LONGINT 29
#define CD_REAL 30

/* Define the count of standard strings. */
#define NUM_STD_STRINGS 391

/* Define whether or not to skip writing an empty Subrs Index. */
#define SKIP_EMPTY_SUBRS

/* Define the structure for the string table. */
typedef struct cff_string_item_s {
    gs_const_string key;
    int index1;		/* index + 1, 0 means empty */
} cff_string_item_t;
typedef struct cff_string_table_s {
    cff_string_item_t *items;
    int count;
    int size;
    uint total;
    int reprobe;
} cff_string_table_t;

/* Define the state of the CFF writer. */
typedef struct cff_writer_s {
    int options;
    stream *strm;
    gs_font_base *pfont;	/* type1 or cid0 */
    glyph_data_proc_t glyph_data;
    int offset_size;
    long start_pos;
    cff_string_table_t std_strings;
    cff_string_table_t strings;
    gs_int_rect FontBBox;
} cff_writer_t;
typedef struct cff_glyph_subset_s {
    psf_outline_glyphs_t glyphs;
    int num_encoded;		/* glyphs.subset_data[1..num_e] are encoded */
    int num_encoded_chars;	/* Encoding has num_e_chars defined entries */
} cff_glyph_subset_t;

/* ---------------- Output utilities ---------------- */

/* ------ String tables ------ */

/* Initialize a string table. */
private void
cff_string_table_init(cff_string_table_t *pcst, cff_string_item_t *items,
		      int size)
{
    int reprobe = 17;

    memset(items, 0, size * sizeof(*items));
    pcst->items = items;
    pcst->count = 0;
    pcst->size = size;
    while (reprobe != 1 && igcd(size, reprobe) != 1)
	reprobe = (reprobe * 2 + 1) % size;
    pcst->total = 0;
    pcst->reprobe = reprobe;
}

/* Add a string to a string table. */
private int
cff_string_add(cff_string_table_t *pcst, const byte *data, uint size)
{
    int index;

    if (pcst->count >= pcst->size)
	return_error(gs_error_limitcheck);
    index = pcst->count++;
    pcst->items[index].key.data = data;
    pcst->items[index].key.size = size;
    pcst->total += size;
    return index;
}

/* Look up a string, optionally adding it. */
/* Return 1 if the string was added. */
private int
cff_string_index(cff_string_table_t *pcst, const byte *data, uint size,
		 bool enter, int *pindex)
{
    /****** FAILS IF TABLE FULL AND KEY MISSING ******/
    int j = (size == 0 ? 0 : data[0] * 23 + data[size - 1] * 59 + size);
    int index, c = 0;

    while ((index = pcst->items[j %= pcst->size].index1) != 0) {
	--index;
	if (!bytes_compare(pcst->items[index].key.data,
			   pcst->items[index].key.size, data, size)) {
	    *pindex = index;
	    return 0;
	}
	j += pcst->reprobe;
	if (++c >= pcst->size)
	    break;
    }
    if (!enter)
	return_error(gs_error_undefined);
    index = cff_string_add(pcst, data, size);
    if (index < 0)
	return index;
    pcst->items[j].index1 = index + 1;
    *pindex = index;
    return 1;
}

/* Get the SID for a string or a glyph. */
private int
cff_string_sid(cff_writer_t *pcw, const byte *data, uint size)
{
    int index;
    int code = cff_string_index(&pcw->std_strings, data, size, false, &index);

    if (code < 0) {
	code = cff_string_index(&pcw->strings, data, size, true, &index);
	if (code < 0)
	    return code;
	index += NUM_STD_STRINGS;
    }
    return index;
}
private int
cff_glyph_sid(cff_writer_t *pcw, gs_glyph glyph)
{
    gs_const_string str;
    int code =
	pcw->pfont->procs.glyph_name((gs_font *)pcw->pfont, glyph, &str);

    if (code < 0)
	return code;
    return cff_string_sid(pcw, str.data, str.size);
}

/* ------ Low level ------ */

private void
put_card16(cff_writer_t *pcw, uint c16)
{
    sputc(pcw->strm, (byte)(c16 >> 8));
    sputc(pcw->strm, (byte)c16);
}
private int
offset_size(uint offset)
{
    int size = 1;

    while (offset > 255)
	offset >>= 8, ++size;
    return size;
}
private void
put_offset(cff_writer_t *pcw, int offset)
{
    int i;

    for (i = pcw->offset_size - 1; i >= 0; --i)
	sputc(pcw->strm, (byte)(offset >> (i * 8)));
}
private int
put_bytes(stream * s, const byte *ptr, uint count)
{
    uint used;

    sputs(s, ptr, count, &used);
    return (int)used;
}
private int
check_ioerror(stream * s)
{
    uint used;

    return sputs(s, (byte *)&used, 0, &used);
}

/* ------ Data types ------ */

#define CE_OFFSET 32
private void
cff_put_op(cff_writer_t *pcw, int op)
{
    if (op >= CE_OFFSET) {
	sputc(pcw->strm, cx_escape);
	sputc(pcw->strm, (byte)(op - CE_OFFSET));
    } else
	sputc(pcw->strm, (byte)op);
}
private void
cff_put_int(cff_writer_t *pcw, int i)
{
    stream *s = pcw->strm;

    if (i >= -107 && i <= 107)
	sputc(s, (byte)(i + 139));
    else if (i <= 1131 && i >= 0)
	put_card16(pcw, (c_pos2_0 << 8) + i - 108);
    else if (i >= -1131 && i < 0)
	put_card16(pcw, (c_neg2_0 << 8) - i - 108);
    else if (i >= -32768 && i <= 32767) {
	sputc(s, c2_shortint);
	put_card16(pcw, i & 0xffff);
    } else {
	sputc(s, CD_LONGINT);
	put_card16(pcw, i >> 16);
	put_card16(pcw, i & 0xffff);
    }
}
private void
cff_put_int_value(cff_writer_t *pcw, int i, int op)
{
    cff_put_int(pcw, i);
    cff_put_op(pcw, op);
}
private void
cff_put_int_if_ne(cff_writer_t *pcw, int i, int i_default, int op)
{
    if (i != i_default)
	cff_put_int_value(pcw, i, op);
}
private void
cff_put_bool(cff_writer_t *pcw, bool b)
{
    cff_put_int(pcw, (b ? 1 : 0));
}
private void
cff_put_bool_value(cff_writer_t *pcw, bool b, int op)
{
    cff_put_bool(pcw, b);
    cff_put_op(pcw, op);
}
private void
cff_put_real(cff_writer_t *pcw, floatp f)
{
    if (f == (int)f)
	cff_put_int(pcw, (int)f);
    else {
	/* Use decimal representation. */
	char str[50];
	byte b = 0xff;
	const char *p;

	sprintf(str, "%g", f);
	sputc(pcw->strm, CD_REAL);
	for (p = str; ; ++p) {
	    int digit;

	    switch (*p) {
	    case 0:
		goto done;
	    case '.':
		digit = 0xa; break;
	    case '-':
		digit = 0xe; break;
	    case 'e': case 'E':
		if (p[1] == '-')
		    digit = 0xc, ++p;
		else
		    digit = 0xb;
		break;
	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9':
		digit = *p - '0';
		break;
	    default:		/* can't happen */
		digit = 0xd;	/* invalid */
		break;
	    }
	    if (b == 0xff)
		b = (digit << 4) + 0xf;
	    else {
		sputc(pcw->strm, (byte)((b & 0xf0) + digit));
		b = 0xff;
	    }
	}
    done:
	sputc(pcw->strm, b);
    }
}
private void
cff_put_real_value(cff_writer_t *pcw, floatp f, int op)
{
    cff_put_real(pcw, f);
    cff_put_op(pcw, op);
}
private void
cff_put_real_if_ne(cff_writer_t *pcw, floatp f, floatp f_default, int op)
{
    if ((float)f != (float)f_default)
	cff_put_real_value(pcw, f, op);
}
private void
cff_put_real_deltarray(cff_writer_t *pcw, const float *pf, int count, int op)
{
    float prev = 0;
    int i;

    if (count <= 0)
	return;
    for (i = 0; i < count; ++i) {
	float f = pf[i];

	cff_put_real(pcw, f - prev);
	prev = f;
    }
    cff_put_op(pcw, op);
}
private int
cff_put_string(cff_writer_t *pcw, const byte *data, uint size)
{
    int sid = cff_string_sid(pcw, data, size);

    if (sid < 0)
	return sid;
    cff_put_int(pcw, sid);
    return 0;
}
private int
cff_put_string_value(cff_writer_t *pcw, const byte *data, uint size, int op)
{
    int code = cff_put_string(pcw, data, size);

    if (code >= 0)
	cff_put_op(pcw, op);
    return code;
}
private int
cff_extra_lenIV(const cff_writer_t *pcw, const gs_font_type1 *pfont)
{
    return (pcw->options & WRITE_TYPE2_NO_LENIV ?
	    max(pfont->data.lenIV, 0) : 0);
}
private bool
cff_convert_charstrings(const cff_writer_t *pcw, const gs_font_base *pfont)
{
    return (pfont->FontType != ft_encrypted2 &&
	    (pcw->options & WRITE_TYPE2_CHARSTRINGS) != 0);
}
private int
cff_put_CharString(cff_writer_t *pcw, const byte *data, uint size,
		   gs_font_type1 *pfont)
{
    int lenIV = pfont->data.lenIV;
    stream *s = pcw->strm;

    if (cff_convert_charstrings(pcw, (gs_font_base *)pfont)) {
	gs_glyph_data_t gdata;
	int code;

	gdata.memory = pfont->memory;
	gs_glyph_data_from_string(&gdata, data, size, NULL);
	code = psf_convert_type1_to_type2(s, &gdata, pfont);
	if (code < 0)
	    return code;
    } else if (lenIV < 0 || !(pcw->options & WRITE_TYPE2_NO_LENIV))
	put_bytes(s, data, size);
    else if (size >= lenIV) {
	/* Remove encryption. */
	crypt_state state = crypt_charstring_seed;
	byte buf[50];		/* arbitrary */
	uint left, n;

	for (left = lenIV; left > 0; left -= n) {
	    n = min(left, sizeof(buf));
	    gs_type1_decrypt(buf, data + lenIV - left, n, &state);
	}
	for (left = size - lenIV; left > 0; left -= n) {
	    n = min(left, sizeof(buf));
	    gs_type1_decrypt(buf, data + size - left, n, &state);
	    put_bytes(s, buf, n);
	}
    }
    return 0;
}
private uint
cff_Index_size(uint count, uint total)
{
    return (count == 0 ? 2 :
	    3 + offset_size(total + 1) * (count + 1) + total);
}
private void
cff_put_Index_header(cff_writer_t *pcw, uint count, uint total)
{
    put_card16(pcw, count);
    if (count > 0) {
	pcw->offset_size = offset_size(total + 1);
	sputc(pcw->strm, (byte)pcw->offset_size);
	put_offset(pcw, 1);
    }
}
private void
cff_put_Index(cff_writer_t *pcw, const cff_string_table_t *pcst)
{
    uint j, offset;

    if (pcst->count == 0) {
	put_card16(pcw, 0);
	return;
    }
    cff_put_Index_header(pcw, pcst->count, pcst->total);
    for (j = 0, offset = 1; j < pcst->count; ++j) {
	offset += pcst->items[j].key.size;
	put_offset(pcw, offset);
    }
    for (j = 0; j < pcst->count; ++j)
	put_bytes(pcw->strm, pcst->items[j].key.data, pcst->items[j].key.size);
}

/* ---------------- Main code ---------------- */

/* ------ Header ------ */

/* Write the header, setting offset_size. */
private int
cff_write_header(cff_writer_t *pcw, uint end_offset)
{
    pcw->offset_size = (end_offset > 0x7fff ? 3 : 2);
    put_bytes(pcw->strm, (const byte *)"\001\000\004", 3);
    sputc(pcw->strm, (byte)pcw->offset_size);
    return 0;
}

/* ------ Top Dict ------ */

/*
 * There are 3 variants of this: Type 1 / Type 2 font, CIDFontType 0
 * CIDFont, and FDArray entry for CIDFont.
 */

typedef enum {
    TOP_version = 0,
    TOP_Notice = 1,
    TOP_FullName = 2,
    TOP_FamilyName = 3,
    TOP_Weight = 4,
    TOP_FontBBox = 5,
    TOP_UniqueID = 13,
    TOP_XUID = 14,
    TOP_charset = 15,		/* (offset or predefined index) */
#define charset_ISOAdobe 0
#define charset_Expert 1
#define charset_ExpertSubset 2
#define charset_DEFAULT 0
    TOP_Encoding = 16,		/* (offset or predefined index) */
#define Encoding_Standard 0
#define Encoding_Expert 1
#define Encoding_DEFAULT 0
    TOP_CharStrings = 17,	/* (offset) */
    TOP_Private = 18,		/* (offset) */
    TOP_Copyright = 32,
    TOP_isFixedPitch = 33,
#define isFixedPitch_DEFAULT false
    TOP_ItalicAngle = 34,
#define ItalicAngle_DEFAULT 0
    TOP_UnderlinePosition = 35,
#define UnderlinePosition_DEFAULT (-100)
    TOP_UnderlineThickness = 36,
#define UnderlineThickness_DEFAULT 50
    TOP_PaintType = 37,
#define PaintType_DEFAULT 0
    TOP_CharstringType = 38,
#define CharstringType_DEFAULT 2
    TOP_FontMatrix = 39,	/* default is [0.001 0 0 0.001 0 0] */
    TOP_StrokeWidth = 40,
#define StrokeWidth_DEFAULT 0
    TOP_ROS = 62,
    TOP_CIDFontVersion = 63,
#define CIDFontVersion_DEFAULT 0
    TOP_CIDFontRevision = 64,
#define CIDFontRevision_DEFAULT 0
    TOP_CIDFontType = 65,
#define CIDFontType_DEFAULT 0
    TOP_CIDCount = 66,
#define CIDCount_DEFAULT 8720
    TOP_UIDBase = 67,
    TOP_FDArray = 68,		/* (offset) */
    TOP_FDSelect = 69,		/* (offset) */
    TOP_FontName = 70		/* only used in FDArray "fonts" */
} Top_op;

private int
cff_get_Top_info_common(cff_writer_t *pcw, gs_font_base *pbfont,
			bool full_info, gs_font_info_t *pinfo)
{
    pinfo->Flags_requested = FONT_IS_FIXED_WIDTH;
    /* Preset defaults */
    pinfo->members = 0;
    pinfo->Flags = pinfo->Flags_returned = 0;
    pinfo->ItalicAngle = ItalicAngle_DEFAULT;
    pinfo->UnderlinePosition = UnderlinePosition_DEFAULT;
    pinfo->UnderlineThickness = UnderlineThickness_DEFAULT;
    return pbfont->procs.font_info
	((gs_font *)pbfont, NULL,
	 (full_info ?
	  FONT_INFO_FLAGS | FONT_INFO_ITALIC_ANGLE |
	    FONT_INFO_UNDERLINE_POSITION |
	    FONT_INFO_UNDERLINE_THICKNESS : 0) |
	 (FONT_INFO_COPYRIGHT | FONT_INFO_NOTICE |
	  FONT_INFO_FAMILY_NAME | FONT_INFO_FULL_NAME),
	 pinfo);
}
private void
cff_write_Top_common(cff_writer_t *pcw, gs_font_base *pbfont,
		     bool write_FontMatrix, const gs_font_info_t *pinfo)
{
    /*
     * The Adobe documentation doesn't make it at all clear that if the
     * FontMatrix is missing (defaulted) in a CFF CIDFont, all of the
     * FontMatrices of the subfonts in FDArray are multiplied by 1000.
     * (This is documented for ordinary CIDFonts, but not for CFF CIDFonts.)
     * Because of this, the FontMatrix for a CFF CIDFont must be written
     * even if if is the default.  write_FontMatrix controls this.
     */
    /* (version) */
    if (pinfo->members & FONT_INFO_NOTICE)
	cff_put_string_value(pcw, pinfo->Notice.data, pinfo->Notice.size,
			     TOP_Notice);
    if (pinfo->members & FONT_INFO_FULL_NAME)
	cff_put_string_value(pcw, pinfo->FullName.data, pinfo->FullName.size,
			     TOP_FullName);
    if (pinfo->members & FONT_INFO_FAMILY_NAME)
	cff_put_string_value(pcw, pinfo->FamilyName.data,
			     pinfo->FamilyName.size, TOP_FamilyName);
    if (pcw->FontBBox.p.x != 0 || pcw->FontBBox.p.y != 0 ||
 	pcw->FontBBox.q.x != 0 || pcw->FontBBox.q.y != 0
  	) {
  	/* An omitted FontBBox is equivalent to an empty one. */
 	/*
 	 * Since Acrobat Reader 4 on Solaris doesn't like 
 	 * an omitted FontBBox, we copy it here from
 	 * the font descriptor, because the base font
 	 * is allowed to omit it's FontBBox.
 	 */
 	cff_put_real(pcw, pcw->FontBBox.p.x);
 	cff_put_real(pcw, pcw->FontBBox.p.y);
 	cff_put_real(pcw, pcw->FontBBox.q.x);
 	cff_put_real(pcw, pcw->FontBBox.q.y);
  	cff_put_op(pcw, TOP_FontBBox);
      }
    if (uid_is_UniqueID(&pbfont->UID))
	cff_put_int_value(pcw, pbfont->UID.id, TOP_UniqueID);
    else if (uid_is_XUID(&pbfont->UID)) {
	int j;

	for (j = 0; j < uid_XUID_size(&pbfont->UID); ++j)
	    cff_put_int(pcw, uid_XUID_values(&pbfont->UID)[j]);
	cff_put_op(pcw, TOP_XUID);
    }
    /*
     * Acrobat Reader 3 gives an error if a CFF font includes any of the
     * following opcodes.
     */
    if (!(pcw->options & WRITE_TYPE2_AR3)) {
	if (pinfo->members & FONT_INFO_COPYRIGHT)
	    cff_put_string_value(pcw, pinfo->Copyright.data,
				 pinfo->Copyright.size, TOP_Copyright);
	if (pinfo->Flags & pinfo->Flags_returned & FONT_IS_FIXED_WIDTH)
	    cff_put_bool_value(pcw, true, TOP_isFixedPitch);
	cff_put_real_if_ne(pcw, pinfo->ItalicAngle, ItalicAngle_DEFAULT,
			   TOP_ItalicAngle);
	cff_put_int_if_ne(pcw, pinfo->UnderlinePosition,
			  UnderlinePosition_DEFAULT, TOP_UnderlinePosition);
	cff_put_int_if_ne(pcw, pinfo->UnderlineThickness,
			  UnderlineThickness_DEFAULT, TOP_UnderlineThickness);
	cff_put_int_if_ne(pcw, pbfont->PaintType, PaintType_DEFAULT,
			  TOP_PaintType);
    }
    {
	static const gs_matrix fm_default = {
	    constant_matrix_body(0.001, 0, 0, 0.001, 0, 0)
	};

	if (write_FontMatrix ||
	    pbfont->FontMatrix.xx != fm_default.xx ||
	    pbfont->FontMatrix.xy != 0 || pbfont->FontMatrix.yx != 0 ||
	    pbfont->FontMatrix.yy != fm_default.yy ||
	    pbfont->FontMatrix.tx != 0 || pbfont->FontMatrix.ty != 0
	    ) {
	    cff_put_real(pcw, pbfont->FontMatrix.xx);
	    cff_put_real(pcw, pbfont->FontMatrix.xy);
	    cff_put_real(pcw, pbfont->FontMatrix.yx);
	    cff_put_real(pcw, pbfont->FontMatrix.yy);
	    cff_put_real(pcw, pbfont->FontMatrix.tx);
	    cff_put_real(pcw, pbfont->FontMatrix.ty);
	    cff_put_op(pcw, TOP_FontMatrix);
	}
    }
    cff_put_real_if_ne(pcw, pbfont->StrokeWidth, StrokeWidth_DEFAULT,
		       TOP_StrokeWidth);
}

/* Type 1 or Type 2 font */
private void
cff_write_Top_font(cff_writer_t *pcw, uint Encoding_offset,
		   uint charset_offset, uint CharStrings_offset,
		   uint Private_offset, uint Private_size)
{
    gs_font_base *pbfont = (gs_font_base *)pcw->pfont;
    gs_font_info_t info;

    cff_get_Top_info_common(pcw, pbfont, true, &info);
    cff_write_Top_common(pcw, pbfont, false, &info);
    cff_put_int(pcw, Private_size);
    cff_put_int_value(pcw, Private_offset, TOP_Private);
    cff_put_int_value(pcw, CharStrings_offset, TOP_CharStrings);
    cff_put_int_if_ne(pcw, charset_offset, charset_DEFAULT, TOP_charset);
    cff_put_int_if_ne(pcw, Encoding_offset, Encoding_DEFAULT, TOP_Encoding);
    {
	int type = (pcw->options & WRITE_TYPE2_CHARSTRINGS ? 2 :
		    pbfont->FontType == ft_encrypted2 ? 2 : 1);

	cff_put_int_if_ne(pcw, type, CharstringType_DEFAULT,
			  TOP_CharstringType);
    }
}

/* CIDFontType 0 CIDFont */
private void
cff_write_ROS(cff_writer_t *pcw, const gs_cid_system_info_t *pcidsi)
{
    cff_put_string(pcw, pcidsi->Registry.data, pcidsi->Registry.size);
    cff_put_string(pcw, pcidsi->Ordering.data, pcidsi->Ordering.size);
    cff_put_int_value(pcw, pcidsi->Supplement, TOP_ROS);
}
private void
cff_write_Top_cidfont(cff_writer_t *pcw, uint charset_offset,
		      uint CharStrings_offset, uint FDSelect_offset,
		      uint Font_offset, const gs_font_info_t *pinfo)
{
    gs_font_base *pbfont = (gs_font_base *)pcw->pfont;
    gs_font_cid0 *pfont = (gs_font_cid0 *)pbfont;

    cff_write_ROS(pcw, &pfont->cidata.common.CIDSystemInfo);
    cff_write_Top_common(pcw, pbfont, true, pinfo); /* full_info = true */
    cff_put_int_if_ne(pcw, charset_offset, charset_DEFAULT, TOP_charset);
    cff_put_int_value(pcw, CharStrings_offset, TOP_CharStrings);
    /*
     * CIDFontVersion and CIDFontRevision aren't used consistently,
     * so we don't currently write them.  CIDFontType is always 0.
     */
    cff_put_int_if_ne(pcw, pfont->cidata.common.CIDCount, CIDCount_DEFAULT,
		      TOP_CIDCount);
    /* We don't use UIDBase. */
    cff_put_int_value(pcw, Font_offset, TOP_FDArray);
    cff_put_int_value(pcw, FDSelect_offset, TOP_FDSelect);
}

/* FDArray Index for CIDFont (offsets only) */
private void
cff_write_FDArray_offsets(cff_writer_t *pcw, uint *FDArray_offsets,
			  int num_fonts)
{
    int j;

    cff_put_Index_header(pcw, num_fonts,
			 FDArray_offsets[num_fonts] - FDArray_offsets[0]);
    for (j = 1; j <= num_fonts; ++j)
	put_offset(pcw, FDArray_offsets[j] - FDArray_offsets[0] + 1);
}

/* FDArray entry for CIDFont */
private void
cff_write_Top_fdarray(cff_writer_t *pcw, gs_font_base *pbfont,
		      uint Private_offset, uint Private_size)
{
    const gs_font_name *pfname = &pbfont->font_name;
    gs_font_info_t info;

    cff_get_Top_info_common(pcw, pbfont, false, &info);
    cff_write_Top_common(pcw, pbfont, false, &info);
    cff_put_int(pcw, Private_size);
    cff_put_int_value(pcw, Private_offset, TOP_Private);
    if (pfname->size == 0)
	pfname = &pbfont->key_name;
    if (pfname->size) {
	cff_put_string(pcw, pfname->chars, pfname->size);
	cff_put_op(pcw, TOP_FontName);
    }
}

/* ------ Private Dict ------ */

/* Defaults are noted in comments. */
typedef enum {
    PRIVATE_BlueValues = 6,	/* (deltarray) */
    PRIVATE_OtherBlues = 7,	/* (deltarray) */
    PRIVATE_FamilyBlues = 8,	/* (deltarray) */
    PRIVATE_FamilyOtherBlues = 9, /* (deltarray) */
    PRIVATE_StdHW = 10,
    PRIVATE_StdVW = 11,
    PRIVATE_Subrs = 19,		/* (offset, relative to Private Dict) */
    PRIVATE_defaultWidthX = 20,
#define defaultWidthX_DEFAULT fixed_0
    PRIVATE_nominalWidthX = 21,
#define nominalWidthX_DEFAULT fixed_0
    PRIVATE_BlueScale = 41,
#define BlueScale_DEFAULT 0.039625
    PRIVATE_BlueShift = 42,
#define BlueShift_DEFAULT 7
    PRIVATE_BlueFuzz = 43,
#define BlueFuzz_DEFAULT 1
    PRIVATE_StemSnapH = 44,	/* (deltarray) */
    PRIVATE_StemSnapV = 45,	/* (deltarray) */
    PRIVATE_ForceBold = 46,
#define ForceBold_DEFAULT false
    PRIVATE_ForceBoldThreshold = 47,
#define ForceBoldThreshold_DEFAULT 0
    PRIVATE_lenIV = 48,
#define lenIV_DEFAULT (-1)
    PRIVATE_LanguageGroup = 49,
#define LanguageGroup_DEFAULT 0
    PRIVATE_ExpansionFactor = 50,
#define ExpansionFactor_DEFAULT 0.06
    PRIVATE_initialRandomSeed = 51
#define initialRandomSeed_DEFAULT 0
} Private_op;

private void
cff_write_Private(cff_writer_t *pcw, uint Subrs_offset,
		  const gs_font_type1 *pfont)
{
#define PUT_FLOAT_TABLE(member, op)\
    cff_put_real_deltarray(pcw, pfont->data.member.values,\
			   pfont->data.member.count, op)
				
    PUT_FLOAT_TABLE(BlueValues, PRIVATE_BlueValues);
    PUT_FLOAT_TABLE(OtherBlues, PRIVATE_OtherBlues);
    PUT_FLOAT_TABLE(FamilyBlues, PRIVATE_FamilyBlues);
    PUT_FLOAT_TABLE(FamilyOtherBlues, PRIVATE_FamilyOtherBlues);
    if (pfont->data.StdHW.count > 0)
	cff_put_real_value(pcw, pfont->data.StdHW.values[0], PRIVATE_StdHW);
    if (pfont->data.StdVW.count > 0)
	cff_put_real_value(pcw, pfont->data.StdVW.values[0], PRIVATE_StdVW);
    if (Subrs_offset)
	cff_put_int_value(pcw, Subrs_offset, PRIVATE_Subrs);
    if (pfont->FontType != ft_encrypted) {
	if (pfont->data.defaultWidthX != defaultWidthX_DEFAULT)
	    cff_put_real_value(pcw, fixed2float(pfont->data.defaultWidthX),
			       PRIVATE_defaultWidthX);
	if (pfont->data.nominalWidthX != nominalWidthX_DEFAULT)
	    cff_put_real_value(pcw, fixed2float(pfont->data.nominalWidthX),
			       PRIVATE_nominalWidthX);
	cff_put_int_if_ne(pcw, pfont->data.initialRandomSeed,
			  initialRandomSeed_DEFAULT,
			  PRIVATE_initialRandomSeed);
    }
    cff_put_real_if_ne(pcw, pfont->data.BlueScale, BlueScale_DEFAULT,
		       PRIVATE_BlueScale);
    cff_put_real_if_ne(pcw, pfont->data.BlueShift, BlueShift_DEFAULT,
		       PRIVATE_BlueShift);
    cff_put_int_if_ne(pcw, pfont->data.BlueFuzz, BlueFuzz_DEFAULT,
		      PRIVATE_BlueFuzz);
    PUT_FLOAT_TABLE(StemSnapH, PRIVATE_StemSnapH);
    PUT_FLOAT_TABLE(StemSnapV, PRIVATE_StemSnapV);
    if (pfont->data.ForceBold != ForceBold_DEFAULT)
	cff_put_bool_value(pcw, pfont->data.ForceBold,
			   PRIVATE_ForceBold);
    /* (ForceBoldThreshold) */
    if (!(pcw->options & WRITE_TYPE2_NO_LENIV))
	cff_put_int_if_ne(pcw, pfont->data.lenIV, lenIV_DEFAULT,
			  PRIVATE_lenIV);
    cff_put_int_if_ne(pcw, pfont->data.LanguageGroup, LanguageGroup_DEFAULT,
		      PRIVATE_LanguageGroup);
    cff_put_real_if_ne(pcw, pfont->data.ExpansionFactor,
		       ExpansionFactor_DEFAULT, PRIVATE_ExpansionFactor);
    /* initialRandomSeed was handled above */

#undef PUT_FLOAT_TABLE
}

/* ------ CharStrings Index ------ */

/* These are separate procedures only for readability. */
private int
cff_write_CharStrings_offsets(cff_writer_t *pcw, psf_glyph_enum_t *penum,
			      uint *pcount)
{
    gs_font_base *pfont = pcw->pfont;
    int offset;
    gs_glyph glyph;
    uint count;
    stream poss;
    int code;

    s_init(&poss, NULL);
    psf_enumerate_glyphs_reset(penum);
    for (glyph = gs_no_glyph, count = 0, offset = 1;
	 (code = psf_enumerate_glyphs_next(penum, &glyph)) != 1;
	 ++count) {
	gs_glyph_data_t gdata;
	gs_font_type1 *pfd;
	int gcode;

	gdata.memory = pfont->memory;
	if (code == 0 &&
	    (gcode = pcw->glyph_data(pfont, glyph, &gdata, &pfd)) >= 0
	    ) {
	    int extra_lenIV;

	    if (gdata.bits.size >= (extra_lenIV = cff_extra_lenIV(pcw, pfd))) {
		if (cff_convert_charstrings(pcw, (gs_font_base *)pfd)) {
		    swrite_position_only(&poss);
		    code = psf_convert_type1_to_type2(&poss, &gdata, pfd);
		    if (code < 0)
			return code;
		    offset += stell(&poss);
		} else
		    offset += gdata.bits.size - extra_lenIV;
	    }
	    gs_glyph_data_free(&gdata, "cff_write_CharStrings_offsets");
	}
	put_offset(pcw, offset);
    }
    *pcount = count;
    return offset - 1;
}
private void
cff_write_CharStrings(cff_writer_t *pcw, psf_glyph_enum_t *penum,
		      uint charstrings_count, uint charstrings_size)
{
    gs_font_base *pfont = pcw->pfont;
    uint ignore_count;
    gs_glyph glyph;
    int code;

    cff_put_Index_header(pcw, charstrings_count, charstrings_size);
    cff_write_CharStrings_offsets(pcw, penum, &ignore_count);
    psf_enumerate_glyphs_reset(penum);
    for (glyph = gs_no_glyph;
	 (code = psf_enumerate_glyphs_next(penum, &glyph)) != 1;
	 ) {
	gs_glyph_data_t gdata;
	gs_font_type1 *pfd;

	gdata.memory = pfont->memory;
	if (code == 0 &&
	    (code = pcw->glyph_data(pfont, glyph, &gdata, &pfd)) >= 0
	    ) {
	    cff_put_CharString(pcw, gdata.bits.data, gdata.bits.size, pfd);
	    gs_glyph_data_free(&gdata, "cff_write_CharStrings");
	}
    }
}

/* ------ [G]Subrs Index ------ */

/*
 * Currently, we always write all the Subrs, even for subsets.
 * We will fix this someday.
 */

private uint
cff_write_Subrs_offsets(cff_writer_t *pcw, uint *pcount, gs_font_type1 *pfont,
			bool global)
{
    int extra_lenIV = cff_extra_lenIV(pcw, pfont);
    int j, offset;
    int code;
    gs_glyph_data_t gdata;

    gdata.memory = pfont->memory;
    for (j = 0, offset = 1;
	 (code = pfont->data.procs.subr_data(pfont, j, global, &gdata)) !=
	     gs_error_rangecheck;
	 ++j) {
	if (code >= 0 && gdata.bits.size >= extra_lenIV)
	    offset += gdata.bits.size - extra_lenIV;
	put_offset(pcw, offset);
	if (code >= 0)
	    gs_glyph_data_free(&gdata, "cff_write_Subrs_offsets");
    }
    *pcount = j;
    return offset - 1;
}

private void
cff_write_Subrs(cff_writer_t *pcw, uint subrs_count, uint subrs_size,
		gs_font_type1 *pfont, bool global)
{
    int j;
    uint ignore_count;
    gs_glyph_data_t gdata;
    int code;

    gdata.memory = pfont->memory;
    cff_put_Index_header(pcw, subrs_count, subrs_size);
    cff_write_Subrs_offsets(pcw, &ignore_count, pfont, global);
    for (j = 0;
	 (code = pfont->data.procs.subr_data(pfont, j, global, &gdata)) !=
	     gs_error_rangecheck;
	 ++j) {
	if (code >= 0) {
	    cff_put_CharString(pcw, gdata.bits.data, gdata.bits.size, pfont);
	    gs_glyph_data_free(&gdata, "cff_write_Subrs");
	}
    }
}

/* ------ Encoding/charset ------ */

private uint
cff_Encoding_size(int num_encoded, int num_encoded_chars)
{
    int n = min(num_encoded, 255);

    return 2 + n +
	(num_encoded_chars > n ?
	 1 + (num_encoded_chars - n) * 3 : 0);
}

private int
cff_write_Encoding(cff_writer_t *pcw, cff_glyph_subset_t *pgsub)
{
    stream *s = pcw->strm;
    /* This procedure is only used for Type 1 / Type 2 fonts. */
    gs_font_type1 *pfont = (gs_font_type1 *)pcw->pfont;
    byte used[255], index[255], supplement[256];
    int num_enc = min(pgsub->num_encoded, sizeof(index));
    int num_enc_chars = pgsub->num_encoded_chars;
    int nsupp = 0;
    int j;

    memset(used, 0, num_enc);
    for (j = 0; j < 256; ++j) {
	gs_glyph glyph = pfont->procs.encode_char((gs_font *)pfont,
						  (gs_char)j,
						  GLYPH_SPACE_NAME);
	int i;

	if (glyph == gs_no_glyph || glyph == pgsub->glyphs.notdef)
	    continue;
	i = psf_sorted_glyphs_index_of(pgsub->glyphs.subset_data + 1,
				       pgsub->num_encoded, glyph);
	if (i < 0)
	    continue;		/* encoded but not in subset */
	if (i >= sizeof(used) || used[i])
	    supplement[nsupp++] = j;
	else
	    index[i] = j, used[i] = 1;
    }
    sputc(s, (byte)(nsupp ? 0x80 : 0));
    sputc(s, (byte)num_enc);
#ifdef DEBUG
    if (nsupp != num_enc_chars - num_enc)
	lprintf3("nsupp = %d, num_enc_chars = %d, num_enc = %d\n",
		 nsupp, num_enc_chars, num_enc);
    for (j = 0; j < num_enc; ++j)
	if (!used[j])
	    lprintf2("glyph %d = 0x%lx not used\n", j,
		     pgsub->glyphs.subset_data[j + 1]);
#endif
    put_bytes(s, index, num_enc);
    if (nsupp) {
	/* Write supplementary entries for multiply-encoded glyphs. */
	sputc(s, (byte)nsupp);
	for (j = 0; j < nsupp; ++j) {
	    byte chr = supplement[j];

	    sputc(s, chr);
	    put_card16(pcw,
		cff_glyph_sid(pcw,
			      pfont->procs.encode_char((gs_font *)pfont,
						       (gs_char)chr,
						       GLYPH_SPACE_NAME)));
	}
    }
    return 0;
}

private int
cff_write_charset(cff_writer_t *pcw, cff_glyph_subset_t *pgsub)
{
    int j;

    sputc(pcw->strm, 0);
    for (j = 1; j < pgsub->glyphs.subset_size; ++j)
	put_card16(pcw, cff_glyph_sid(pcw, pgsub->glyphs.subset_data[j]));
    return 0;
}
private int
cff_write_cidset(cff_writer_t *pcw, psf_glyph_enum_t *penum)
{
    gs_glyph glyph;
    int code;

    sputc(pcw->strm, 0);
    psf_enumerate_glyphs_reset(penum);
    while ((code = psf_enumerate_glyphs_next(penum, &glyph)) == 0) {
	/* Skip glyph 0 (the .notdef glyph), which is always first. */
	if (glyph != gs_min_cid_glyph)
	    put_card16(pcw, (uint)(glyph - gs_min_cid_glyph));
    }
    return min(code, 0);
}

/* ------ FDSelect ------ */

/* Determine the size of FDSelect. */
private uint
cff_FDSelect_size(cff_writer_t *pcw, psf_glyph_enum_t *penum, uint *pformat)
{
    gs_font_cid0 *const pfont = (gs_font_cid0 *)pcw->pfont;
    gs_font_base *const pbfont = (gs_font_base *)pfont;
    gs_glyph glyph;
    int prev = -1;
    uint linear_size = 1, range_size = 5;
    int code;

    /* Determine whether format 0 or 3 is more efficient. */
    psf_enumerate_glyphs_reset(penum);
    while ((code = psf_enumerate_glyphs_next(penum, &glyph)) == 0) {
	int font_index;

	code = pfont->cidata.glyph_data(pbfont, glyph, NULL, &font_index);
	if (code >= 0) {
	    if (font_index != prev)
		range_size += 3, prev = font_index;
	    ++linear_size;
	}
    }
    if (range_size < linear_size) {
	*pformat = 3;
	return range_size;
    } else {
	*pformat = 0;
	return linear_size;
    }
}

/* Write FDSelect.  size and format were returned by cff_FDSelect_size. */
private int
cff_write_FDSelect(cff_writer_t *pcw, psf_glyph_enum_t *penum, uint size,
		   int format)
{
    stream *s = pcw->strm;
    gs_font_cid0 *const pfont = (gs_font_cid0 *)pcw->pfont;
    gs_font_base *const pbfont = (gs_font_base *)pfont;
    gs_glyph glyph;
    int prev = -1;
    uint cid_count = 0;
    int code;

    spputc(s, (byte)format);
    psf_enumerate_glyphs_reset(penum);
    switch (format) {
    case 3:			/* ranges */
	put_card16(pcw, (size - 5) / 3);
	while ((code = psf_enumerate_glyphs_next(penum, &glyph)) == 0) {
	    int font_index;

	    code = pfont->cidata.glyph_data(pbfont, glyph, NULL, &font_index);
	    if (code >= 0) {
		if (font_index != prev) {
		    put_card16(pcw, cid_count);
		    sputc(s, (byte)font_index);
		    prev = font_index;
		}
		++cid_count;
	    }
	}
	put_card16(pcw, cid_count);
	break;
    case 0:			/* linear table */
	while ((code = psf_enumerate_glyphs_next(penum, &glyph)) == 0) {
	    int font_index;

	    code = pfont->cidata.glyph_data(pbfont, glyph, NULL, &font_index);
	    if (code >= 0)
		sputc(s, (byte)font_index);
	}
	break;
    default:			/* not possible */
	return_error(gs_error_rangecheck);
    }
    return 0;
}

/* ------ Main procedure ------ */

/* Write the CFF definition of a Type 1 or Type 2 font. */
int
psf_write_type2_font(stream *s, gs_font_type1 *pfont, int options,
		      gs_glyph *subset_glyphs, uint subset_size,
		      const gs_const_string *alt_font_name,
		      gs_int_rect *FontBBox)
{
    gs_font_base *const pbfont = (gs_font_base *)pfont;
    cff_writer_t writer;
    cff_glyph_subset_t subset;
    cff_string_item_t *std_string_items;
    cff_string_item_t *string_items;
    gs_const_string font_name;
    stream poss;
    uint charstrings_count, charstrings_size;
    uint subrs_count, subrs_size;
    uint gsubrs_count, gsubrs_size, encoding_size, charset_size;
    uint number_of_glyphs = 0, number_of_strings;
    /*
     * Set the offsets and sizes to the largest reasonable values
     * (see below).
     */
    uint
	Top_size = 0x7fffff,
	GSubrs_offset,
	Encoding_offset,
	charset_offset,
	CharStrings_offset,
	Private_offset,
	Private_size = 0x7fffff,
	Subrs_offset,
	End_offset = 0x7fffff;
    int j;
    psf_glyph_enum_t genum;
    gs_glyph glyph;
    long start_pos;
    uint offset;
    int code;

    /* Allocate the string tables. */
    psf_enumerate_glyphs_begin(&genum, (gs_font *)pfont,
			       NULL, 0, GLYPH_SPACE_NAME);
    while ((code = psf_enumerate_glyphs_next(&genum, &glyph)) != 1)
	number_of_glyphs++;
    subset.glyphs.subset_data = (gs_glyph *)gs_alloc_bytes(pfont->memory,
		    number_of_glyphs * sizeof(glyph), "psf_write_type2_font");
    number_of_strings = number_of_glyphs + MAX_CFF_MISC_STRINGS;
    std_string_items = (cff_string_item_t *)gs_alloc_bytes(pfont->memory,
		    (MAX_CFF_STD_STRINGS + number_of_strings) * sizeof(cff_string_item_t), 
		    "psf_write_type2_font");
    if (std_string_items == NULL || subset.glyphs.subset_data == NULL)
	return_error(gs_error_VMerror);
    string_items = std_string_items + MAX_CFF_STD_STRINGS;

    /* Get subset glyphs. */
    code = psf_get_type1_glyphs(&subset.glyphs, pfont, subset_glyphs,
			      subset_size);
    if (code < 0)
	return code;
    if (subset.glyphs.notdef == gs_no_glyph)
	return_error(gs_error_rangecheck); /* notdef is required */

    /* If we're writing Type 2 CharStrings, don't encrypt them. */
    if (options & WRITE_TYPE2_CHARSTRINGS) {
	options |= WRITE_TYPE2_NO_LENIV;
	if (pfont->FontType != ft_encrypted2)
	    pfont->data.defaultWidthX = pfont->data.nominalWidthX = 0;
    }
    writer.options = options;
    s_init(&poss, NULL);
    swrite_position_only(&poss);
    writer.strm = &poss;
    writer.pfont = pbfont;
    writer.glyph_data = psf_type1_glyph_data;
    writer.offset_size = 1;	/* arbitrary */
    writer.start_pos = stell(s);
    writer.FontBBox = *FontBBox;

    /* Initialize the enumeration of the glyphs. */
    psf_enumerate_glyphs_begin(&genum, (gs_font *)pfont,
				subset.glyphs.subset_glyphs,
				(subset.glyphs.subset_glyphs ?
				 subset.glyphs.subset_size : 0),
				GLYPH_SPACE_NAME);

    /* Shuffle the glyphs into the order .notdef, encoded, unencoded. */
    {
	gs_glyph encoded[256];
	int num_enc, num_enc_chars;

	/* Get the list of encoded glyphs. */
	for (j = 0, num_enc_chars = 0; j < 256; ++j) {
	    glyph = pfont->procs.encode_char((gs_font *)pfont, (gs_char)j,
					     GLYPH_SPACE_NAME);
	    if (glyph != gs_no_glyph && glyph != subset.glyphs.notdef &&
		(subset.glyphs.subset_glyphs == 0 ||
		 psf_sorted_glyphs_include(subset.glyphs.subset_data,
					    subset.glyphs.subset_size, glyph)))
		encoded[num_enc_chars++] = glyph;
	}
	subset.num_encoded_chars = num_enc_chars;
	subset.num_encoded = num_enc =
	    psf_sort_glyphs(encoded, num_enc_chars);

	/* Get the complete list of glyphs if we don't have it already. */
	if (!subset.glyphs.subset_glyphs) {
	    int num_glyphs = 0;

	    psf_enumerate_glyphs_reset(&genum);
	    while ((code = psf_enumerate_glyphs_next(&genum, &glyph)) != 1)
		if (code == 0) {
		    if (num_glyphs == number_of_glyphs)
			return_error(gs_error_limitcheck);
		    subset.glyphs.subset_data[num_glyphs++] = glyph;
		}
	    subset.glyphs.subset_size =
		psf_sort_glyphs(subset.glyphs.subset_data, num_glyphs);
	    subset.glyphs.subset_glyphs = subset.glyphs.subset_data;
	}

	/* Move the unencoded glyphs to the top of the list. */
	/*
	 * We could do this in time N rather than N log N with a two-finger
	 * algorithm, but it doesn't seem worth the trouble right now.
	 */
	{
	    int from = subset.glyphs.subset_size;
	    int to = from;

	    while (from > 0) {
		glyph = subset.glyphs.subset_data[--from];
		if (glyph != subset.glyphs.notdef &&
		    !psf_sorted_glyphs_include(encoded, num_enc, glyph))
		    subset.glyphs.subset_data[--to] = glyph;
	    }
#ifdef DEBUG
	    if (to != num_enc + 1)
		lprintf2("to = %d, num_enc + 1 = %d\n", to, num_enc + 1);
#endif
	}

	/* Move .notdef and the encoded glyphs to the bottom of the list. */
	subset.glyphs.subset_data[0] = subset.glyphs.notdef;
	memcpy(subset.glyphs.subset_data + 1, encoded,
	       sizeof(encoded[0]) * num_enc);
    }

    /* Set the font name. */
    if (alt_font_name)
	font_name = *alt_font_name;
    else
	font_name.data = pfont->font_name.chars,
	    font_name.size = pfont->font_name.size;

    /* Initialize the string tables. */
    cff_string_table_init(&writer.std_strings, std_string_items,
			  MAX_CFF_STD_STRINGS);
    for (j = 0; (glyph = gs_c_known_encode((gs_char)j,
				ENCODING_INDEX_CFFSTRINGS)) != gs_no_glyph;
	 ++j) {
	gs_const_string str;
	int ignore;

	gs_c_glyph_name(glyph, &str);
	cff_string_index(&writer.std_strings, str.data, str.size, true,
			 &ignore);
    }
    cff_string_table_init(&writer.strings, string_items, number_of_strings);

    /* Enter miscellaneous strings in the string table. */
    cff_write_Top_font(&writer, 0, 0, 0, 0, 0);

    /* Enter the glyph names in the string table. */
    /* (Note that we have changed the glyph list.) */
    psf_enumerate_glyphs_begin(&genum, (gs_font *)pfont,
			       subset.glyphs.subset_data,
			       subset.glyphs.subset_size,
			       GLYPH_SPACE_NAME);
    while ((code = psf_enumerate_glyphs_next(&genum, &glyph)) != 1)
	if (code == 0) {
	    code = cff_glyph_sid(&writer, glyph);
	    if (code < 0)
		return code;
	}

    /*
     * The CFF specification says that the Encoding, charset, CharStrings,
     * Private, and Local Subr sections may be in any order.  To minimize
     * the risk of incompatibility with Adobe software, we produce them in
     * the order just mentioned.
     */

    /*
     * Compute the size of the GSubrs Index, if not omitted.
     */
    if ((options & WRITE_TYPE2_NO_GSUBRS) != 0 ||
	cff_convert_charstrings(&writer, pbfont) /* we expand all Subrs */
	)
	gsubrs_count = 0, gsubrs_size = 0;
    else
	gsubrs_size = cff_write_Subrs_offsets(&writer, &gsubrs_count,
					      pfont, true);

    /*
     * Compute the size of the Encoding.  For simplicity, we currently
     * always store the Encoding explicitly.  Note that because CFF stores
     * the Encoding in an "inverted" form, we need to count the number of
     * glyphs that occur at more than one place in the Encoding.
     */
    encoding_size = cff_Encoding_size(subset.num_encoded,
				      subset.num_encoded_chars);

    /*
     * Compute the size of the charset.  For simplicity, we currently
     * always store the charset explicitly.
     */
    charset_size = 1 + (subset.glyphs.subset_size - 1) * 2;

    /* Compute the size of the CharStrings Index. */
    code = cff_write_CharStrings_offsets(&writer, &genum, &charstrings_count);
    if (code < 0)
	return code;
    charstrings_size = (uint)code;

    /* Compute the size of the (local) Subrs Index. */
#ifdef SKIP_EMPTY_SUBRS
    subrs_size =
	(cff_convert_charstrings(&writer, pbfont) ? 0 :
	 cff_write_Subrs_offsets(&writer, &subrs_count, pfont, false));
#else
    if (cff_convert_charstrings(&writer, pbfont))
	subrs_count = 0;	/* we expand all Subrs */
    subrs_size = cff_write_Subrs_offsets(&writer, &subrs_count, pfont, false);
#endif

    /*
     * The offsets of the Private Dict and the CharStrings Index
     * depend on the size of the Top Dict; the offset of the Subrs also
     * depends on the size of the Private Dict.  However, the size of the
     * Top Dict depends on the offsets of the CharStrings Index, the
     * charset, and the Encoding, and on the offset and size of the Private
     * Dict, because of the variable-length encoding of the offsets and
     * size; for the same reason, the size of the Private Dict depends on
     * the offset of the Subrs.  Fortunately, the relationship between the
     * value of an offset or size and the size of its encoding is monotonic.
     * Therefore, we start by assuming the largest reasonable value for all
     * the sizes and iterate until everything converges.
     */
 iter:
    swrite_position_only(&poss);
    writer.strm = &poss;

    /* Compute the offsets. */
    GSubrs_offset = 4 + cff_Index_size(1, font_name.size) +
	cff_Index_size(1, Top_size) +
	cff_Index_size(writer.strings.count, writer.strings.total);
    Encoding_offset = GSubrs_offset +
	cff_Index_size(gsubrs_count, gsubrs_size);
    charset_offset = Encoding_offset + encoding_size;
    CharStrings_offset = charset_offset + charset_size;
    Private_offset = CharStrings_offset +
	cff_Index_size(charstrings_count, charstrings_size);
    Subrs_offset = Private_size;  /* relative to Private Dict */

 write:
    if(check_ioerror(writer.strm))
	return_error(gs_error_ioerror);
    start_pos = stell(writer.strm);
    /* Write the header, setting offset_size. */
    cff_write_header(&writer, End_offset);

    /* Write the names Index. */
    cff_put_Index_header(&writer, 1, font_name.size);
    put_offset(&writer, font_name.size + 1);
    put_bytes(writer.strm, font_name.data, font_name.size);

    /* Write the Top Index. */
    cff_put_Index_header(&writer, 1, Top_size);
    put_offset(&writer, Top_size + 1);
    offset = stell(writer.strm) - start_pos;
    cff_write_Top_font(&writer, Encoding_offset, charset_offset,
		       CharStrings_offset,
		       Private_offset, Private_size);
    Top_size = stell(writer.strm) - start_pos - offset;

    /* Write the strings Index. */
    cff_put_Index(&writer, &writer.strings);
    if(check_ioerror(writer.strm))
	return_error(gs_error_ioerror);

    /* Write the GSubrs Index, if any, checking the offset. */
    offset = stell(writer.strm) - start_pos;
    if_debug2('l', "[l]GSubrs = %u => %u\n", GSubrs_offset, offset);
    if (offset > GSubrs_offset)
	return_error(gs_error_rangecheck);
    GSubrs_offset = offset;
    if (gsubrs_count == 0 || cff_convert_charstrings(&writer, pbfont))
	cff_put_Index_header(&writer, 0, 0);
    else
	cff_write_Subrs(&writer, gsubrs_count, gsubrs_size, pfont, true);

    /* Write the Encoding. */
    cff_write_Encoding(&writer, &subset);

    /* Write the charset. */
    cff_write_charset(&writer, &subset);

    /* Write the CharStrings Index, checking the offset. */
    offset = stell(writer.strm) - start_pos;
    if (offset > CharStrings_offset)
	return_error(gs_error_rangecheck);
    CharStrings_offset = offset;
    cff_write_CharStrings(&writer, &genum, charstrings_count,
			  charstrings_size);
    if(check_ioerror(writer.strm))
	return_error(gs_error_ioerror);

    /* Write the Private Dict, checking the offset. */
    offset = stell(writer.strm) - start_pos;
    if (offset > Private_offset)
	return_error(gs_error_rangecheck);
    Private_offset = offset;
    cff_write_Private(&writer, (subrs_size == 0 ? 0 : Subrs_offset), pfont);
    Private_size = stell(writer.strm) - start_pos - offset;

    /* Write the Subrs Index, checking the offset. */
    offset = stell(writer.strm) - (start_pos + Private_offset);
    if (offset > Subrs_offset)
	return_error(gs_error_rangecheck);
    Subrs_offset = offset;
    if (cff_convert_charstrings(&writer, pbfont))
	cff_put_Index_header(&writer, 0, 0);
    else if (subrs_size != 0)
	cff_write_Subrs(&writer, subrs_count, subrs_size, pfont, false);

    /* Check the final offset. */
    if(check_ioerror(writer.strm))
	return_error(gs_error_ioerror);
    offset = stell(writer.strm) - start_pos;
    if (offset > End_offset)
	return_error(gs_error_rangecheck);
    if (offset == End_offset) {
	/* The iteration has converged.  Write the result. */
	if (writer.strm == &poss) {
	    writer.strm = s;
	    goto write;
	}
    } else {
	/* No convergence yet. */
	End_offset = offset;
	goto iter;
    }

    /* All done. */
    gs_free_object(pfont->memory, std_string_items, "psf_write_type2_font");
    gs_free_object(pfont->memory, subset.glyphs.subset_data, "psf_write_type2_font");
    return 0;
}

/* Write the CFF definition of a CIDFontType 0 font (CIDFont). */
private int
cid0_glyph_data(gs_font_base *pbfont, gs_glyph glyph, gs_glyph_data_t *pgd,
		gs_font_type1 **ppfont)
{
    gs_font_cid0 *const pfont = (gs_font_cid0 *)pbfont;
    int font_index;
    int code = pfont->cidata.glyph_data(pbfont, glyph, pgd, &font_index);

    if (code >= 0)
	*ppfont = pfont->cidata.FDArray[font_index];
    return code;
}
#ifdef DEBUG
private int
offset_error(const char *msg)
{
    if_debug1('l', "[l]%s offset error\n", msg);
    return gs_error_rangecheck;
}
#else
#  define offset_error(msg) gs_error_rangecheck
#endif
int
psf_write_cid0_font(stream *s, gs_font_cid0 *pfont, int options,
		    const byte *subset_cids, uint subset_size,
		    const gs_const_string *alt_font_name)
{
    /*
     * CIDFontType 0 fonts differ from ordinary Type 1 / Type 2 fonts
     * as follows:
     *   The TOP Dict starts with a ROS operator.
     *   The TOP Dict must include FDArray and FDSelect operators.
     *   The TOP Dict may include CIDFontVersion, CIDFontRevision,
     *     CIDFontType, CIDCount, and UIDBase operators.
     *   The TOP Dict must not include an Encoding operator.
     *   The charset is defined in terms of CIDs rather than SIDs.
     *   FDArray references a Font Index in which each element is a Dict
     *     defining a font without charset, Encoding, or CharStrings.
     *   FDSelect references a structure mapping CIDs to font numbers.
     */
    gs_font_base *const pbfont = (gs_font_base *)pfont;
    cff_writer_t writer;
    cff_string_item_t std_string_items[500]; /* 391 entries used */
    /****** HOW TO DETERMINE THE SIZE OF STRINGS? ******/
    cff_string_item_t string_items[500 /* character names */ +
				   40 /* misc. values */];
    gs_const_string font_name;
    stream poss;
    uint charstrings_count, charstrings_size;
    uint gsubrs_count, gsubrs_size;
    uint charset_size, fdselect_size, fdselect_format;
    uint subrs_count[256], subrs_size[256];
    /*
     * Set the offsets and sizes to the largest reasonable values
     * (see below).
     */
    uint
	Top_size = 0x7fffff,
	GSubrs_offset = 0x7fffff,
	charset_offset = 0x7fffff,
	FDSelect_offset = 0x7fffff,
	CharStrings_offset = 0x7fffff,
	Font_offset = 0x7fffff,
	FDArray_offsets[257],
	Private_offsets[257],
	Subrs_offsets[257],
	End_offset = 0x7fffff;
    int j;
    psf_glyph_enum_t genum;
    gs_font_info_t info;
    long start_pos;
    uint offset;
    int num_fonts = pfont->cidata.FDArray_size;
    int code;


    /* Initialize the enumeration of the glyphs. */
    psf_enumerate_cids_begin(&genum, (gs_font *)pfont, subset_cids,
			     subset_size);

    /* Check that the font can be written. */
    code = psf_check_outline_glyphs((gs_font_base *)pfont, &genum,
				    cid0_glyph_data);
    if (code < 0)
	return code;
    /* The .notdef glyph (glyph 0) must be included. */
    if (subset_cids && subset_size > 0 && !(subset_cids[0] & 0x80))
	return_error(gs_error_rangecheck);

    writer.options = options;
    s_init(&poss, NULL);
    swrite_position_only(&poss);
    writer.strm = &poss;
    writer.pfont = pbfont;
    writer.glyph_data = cid0_glyph_data;
    writer.offset_size = 1;	/* arbitrary */
    writer.start_pos = stell(s);
    writer.FontBBox.p.x = writer.FontBBox.p.y = 0;
    writer.FontBBox.q.x = writer.FontBBox.q.y = 0;

    /* Set the font name. */
    if (alt_font_name)
	font_name = *alt_font_name;
    else if (pfont->font_name.size)
	font_name.data = pfont->font_name.chars,
	    font_name.size = pfont->font_name.size;
    else
	font_name.data = pfont->key_name.chars,
	    font_name.size = pfont->key_name.size;

    /* Initialize the string tables. */
    cff_string_table_init(&writer.std_strings, std_string_items,
			  countof(std_string_items));
    cff_string_table_init(&writer.strings, string_items,
			  countof(string_items));

    /* Make all entries in the string table. */
    cff_write_ROS(&writer, &pfont->cidata.common.CIDSystemInfo);
    for (j = 0; j < num_fonts; ++j) {
	gs_font_type1 *pfd = pfont->cidata.FDArray[j];

	cff_write_Top_fdarray(&writer, (gs_font_base *)pfd, 0, 0);
    }

    /*
     * The CFF specification says that sections after the initial Indexes
     * may be in any order.  To minimize the risk of incompatibility with
     * Adobe software, we produce them in the order illustrated in the
     * specification.
     */

    /* Initialize the offset arrays. */
    for (j = 0; j <= num_fonts; ++j)
	FDArray_offsets[j] = Private_offsets[j] = Subrs_offsets[j] =
	    0x7effffff / num_fonts * j + 0x1000000;

    /*
     * Compute the size of the GSubrs Index, if not omitted.
     * Arbitrarily use FDArray[0] to access the GSubrs and to determine
     * the CharString type.
     */
    if ((options & WRITE_TYPE2_NO_GSUBRS) != 0 ||
	cff_convert_charstrings(&writer,
			(const gs_font_base *)pfont->cidata.FDArray[0])
				/* we expand all Subrs */
	)
	gsubrs_count = 0, gsubrs_size = 0;
    else
	gsubrs_size = cff_write_Subrs_offsets(&writer, &gsubrs_count,
					      pfont->cidata.FDArray[0], true);

    /*
     * Compute the size of the charset.  For simplicity, we currently
     * always store the charset explicitly.
     */
    swrite_position_only(&poss);
    cff_write_cidset(&writer, &genum);
    charset_size = stell(&poss);

    /* Compute the size of the FDSelect strucure. */
    fdselect_size = cff_FDSelect_size(&writer, &genum, &fdselect_format);

    /* Compute the size of the CharStrings Index. */
    charstrings_size =
	cff_write_CharStrings_offsets(&writer, &genum, &charstrings_count);

    /* Compute the size of the (local) Subrs Indexes. */
    for (j = 0; j < num_fonts; ++j) {
	gs_font_type1 *pfd = pfont->cidata.FDArray[j];

#ifdef SKIP_EMPTY_SUBRS
	subrs_size[j] =
	    (cff_convert_charstrings(&writer, (gs_font_base *)pfd) ? 0 :
	     cff_write_Subrs_offsets(&writer, &subrs_count[j], pfd, false));
#else
	if (cff_convert_charstrings(&writer, (gs_font_base *)pfd))
	    subrs_count[j] = 0;  /* we expand all Subrs */
	subrs_size[j] = cff_write_Subrs_offsets(&writer, &subrs_count[j], pfd, false);
#endif
    }

    /* Get the font_info once, since it may be expensive. */
    cff_get_Top_info_common(&writer, (gs_font_base *)pfont, true, &info);

    /*
     * The offsets of the Private Dict and the CharStrings Index
     * depend on the size of the Top Dict; the offset of the Subrs also
     * depends on the size of the Private Dict.  However, the size of the
     * Top Dict depends on the offsets of the CharStrings Index and the
     * charset, and on the offset and size of the Private Dict,
     * because of the variable-length encoding of the offsets and
     * size; for the same reason, the size of the Private Dict depends on
     * the offset of the Subrs.  Fortunately, the relationship between the
     * value of an offset or size and the size of its encoding is monotonic.
     * Therefore, we start by assuming the largest reasonable value for all
     * the sizes and iterate until everything converges.
     */
 iter:
    swrite_position_only(&poss);
    writer.strm = &poss;

    /* Compute the offsets. */
    GSubrs_offset = 4 + cff_Index_size(1, font_name.size) +
	cff_Index_size(1, Top_size) +
	cff_Index_size(writer.strings.count, writer.strings.total);
    charset_offset = GSubrs_offset +
	cff_Index_size(gsubrs_count, gsubrs_size);
    FDSelect_offset = charset_offset + charset_size;
    CharStrings_offset = FDSelect_offset + fdselect_size;
    if_debug4('l', "[l]GSubrs at %u, charset at %u, FDSelect at %u, CharStrings at %u\n",
	      GSubrs_offset, charset_offset, FDSelect_offset, CharStrings_offset);

 write:
    start_pos = stell(writer.strm);
    if_debug1('l', "[l]start_pos = %ld\n", start_pos);
    /* Write the header, setting offset_size. */
    cff_write_header(&writer, End_offset);

    /* Write the names Index. */
    cff_put_Index_header(&writer, 1, font_name.size);
    put_offset(&writer, font_name.size + 1);
    put_bytes(writer.strm, font_name.data, font_name.size);

    /* Write the Top Index. */
    cff_put_Index_header(&writer, 1, Top_size);
    put_offset(&writer, Top_size + 1);
    offset = stell(writer.strm) - start_pos;
    cff_write_Top_cidfont(&writer, charset_offset, CharStrings_offset,
			  FDSelect_offset, Font_offset, &info);
    Top_size = stell(writer.strm) - start_pos - offset;
    if_debug1('l', "[l]Top_size = %u\n", Top_size);

    /* Write the strings Index. */
    cff_put_Index(&writer, &writer.strings);

    /* Write the GSubrs Index, if any, checking the offset. */
    offset = stell(writer.strm) - start_pos;
    if_debug2('l', "[l]GSubrs = %u => %u\n", GSubrs_offset, offset);
    if (offset > GSubrs_offset)
	return_error(gs_error_rangecheck);
    GSubrs_offset = offset;
    if (gsubrs_count == 0 ||
	cff_convert_charstrings(&writer,
			(const gs_font_base *)pfont->cidata.FDArray[0])
	)
	cff_put_Index_header(&writer, 0, 0);
    else
	cff_write_Subrs(&writer, gsubrs_count, gsubrs_size,
			pfont->cidata.FDArray[0], true);

    /* Write the charset. */
    if_debug1('l', "[l]charset = %u\n", stell(writer.strm) - start_pos);
    cff_write_cidset(&writer, &genum);

    /* Write the FDSelect structure, checking the offset. */
    offset = stell(writer.strm) - start_pos;
    if_debug2('l', "[l]FDSelect = %u => %u\n", FDSelect_offset, offset);
    if (offset > FDSelect_offset)
	return_error(offset_error("FDselect"));
    FDSelect_offset = offset;
    cff_write_FDSelect(&writer, &genum, fdselect_size, fdselect_format);

    /* Write the CharStrings Index, checking the offset. */
    offset = stell(writer.strm) - start_pos;
    if_debug2('l', "[l]CharStrings = %u => %u\n", CharStrings_offset, offset);
    if (offset > CharStrings_offset)
	return_error(offset_error("CharStrings"));
    CharStrings_offset = offset;
    cff_write_CharStrings(&writer, &genum, charstrings_count,
			  charstrings_size);

    /* Write the Font Dict Index. */
    offset = stell(writer.strm) - start_pos;
    if_debug2('l', "[l]Font = %u => %u\n", Font_offset, offset);
    if (offset > Font_offset)
	return_error(offset_error("Font"));
    Font_offset = offset;
    cff_write_FDArray_offsets(&writer, FDArray_offsets, num_fonts);
    offset = stell(writer.strm) - start_pos;
    if_debug2('l', "[l]FDArray[0] = %u => %u\n", FDArray_offsets[0], offset);
    if (offset > FDArray_offsets[0])
	return_error(offset_error("FDArray[0]"));
    FDArray_offsets[0] = offset;
    for (j = 0; j < num_fonts; ++j) {
	gs_font_type1 *pfd = pfont->cidata.FDArray[j];

	/* If we're writing Type 2 CharStrings, don't encrypt them. */
	if (options & WRITE_TYPE2_CHARSTRINGS) {
	    options |= WRITE_TYPE2_NO_LENIV;
	    if (pfd->FontType != ft_encrypted2)
		pfd->data.defaultWidthX = pfd->data.nominalWidthX = 0;
	}
	cff_write_Top_fdarray(&writer, (gs_font_base *)pfd, Private_offsets[j],
			      Private_offsets[j + 1] - Private_offsets[j]);
	offset = stell(writer.strm) - start_pos;
	if_debug3('l', "[l]FDArray[%d] = %u => %u\n", j + 1,
		  FDArray_offsets[j + 1], offset);
	if (offset > FDArray_offsets[j + 1])
	    return_error(offset_error("FDArray"));
	FDArray_offsets[j + 1] = offset;
    }

    /* Write the Private Dicts, checking the offset. */
    for (j = 0; ; ++j) {
	gs_font_type1 *pfd;

	offset = stell(writer.strm) - start_pos;
	if_debug3('l', "[l]Private[%d] = %u => %u\n",
		  j, Private_offsets[j], offset);
	if (offset > Private_offsets[j])
	    return_error(offset_error("Private"));
	Private_offsets[j] = offset;
	if (j == num_fonts)
	    break;
	pfd = pfont->cidata.FDArray[j];
	cff_write_Private(&writer,
			  (subrs_size[j] == 0 ? 0 : Subrs_offsets[j]), pfd);
    }

    /* Write the Subrs Indexes, checking the offsets. */
    for (j = 0; ; ++j) {
	gs_font_type1 *pfd;

	offset = stell(writer.strm) - (start_pos + Private_offsets[j]);
	if_debug3('l', "[l]Subrs[%d] = %u => %u\n",
		  j, Subrs_offsets[j], offset);
	if (offset > Subrs_offsets[j])
	    return_error(offset_error("Subrs"));
	Subrs_offsets[j] = offset;
	if (j == num_fonts)
	    break;
	pfd = pfont->cidata.FDArray[j];
	if (cff_convert_charstrings(&writer, (gs_font_base *)pfd))
	    cff_put_Index_header(&writer, 0, 0);
	else if (subrs_size[j] != 0)
	    cff_write_Subrs(&writer, subrs_count[j], subrs_size[j], pfd, false);
    }

    /* Check the final offset. */
    offset = stell(writer.strm) - start_pos;
    if_debug2('l', "[l]End = %u => %u\n", End_offset, offset);
    if (offset > End_offset)
	return_error(offset_error("End"));
    if (offset == End_offset) {
	/* The iteration has converged.  Write the result. */
	if (writer.strm == &poss) {
	    writer.strm = s;
	    goto write;
	}
    } else {
	/* No convergence yet. */
	End_offset = offset;
	goto iter;
    }

    /* All done. */
    return 0;
}

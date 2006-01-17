/* Copyright (C) 1991, 1992, 1993, 1997, 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gschar0.c,v 1.8 2002/10/31 08:34:51 ray Exp $ */
/* Composite font decoding for Ghostscript library */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gsfcmap.h"
#include "gxfcmap.h"
#include "gxfixed.h"
#include "gxdevice.h"
#include "gxfont.h"
#include "gxfont0.h"
#include "gxtext.h"

/* Stack up modal composite fonts, down to a non-modal or base font. */
private int
gs_stack_modal_fonts(gs_text_enum_t *pte)
{
    int fdepth = pte->fstack.depth;
    gs_font *cfont = pte->fstack.items[fdepth].font;

    while (cfont->FontType == ft_composite) {
	gs_font_type0 *const cmfont = (gs_font_type0 *) cfont;

	if (!fmap_type_is_modal(cmfont->data.FMapType))
	    break;
	if (fdepth == MAX_FONT_STACK)
	    return_error(gs_error_invalidfont);
	fdepth++;
	cfont = cmfont->data.FDepVector[cmfont->data.Encoding[0]];
	pte->fstack.items[fdepth].font = cfont;
	pte->fstack.items[fdepth].index = 0;
	if_debug2('j', "[j]stacking depth=%d font=0x%lx\n",
		  fdepth, (ulong) cfont);
    }
    pte->fstack.depth = fdepth;
    return 0;
}
/* Initialize the composite font stack for a show enumerator. */
/* Return an error if the data is not a byte string. */
int
gs_type0_init_fstack(gs_text_enum_t *pte, gs_font * pfont)
{
    if (!(pte->text.operation & (TEXT_FROM_STRING | TEXT_FROM_BYTES)))
	return_error(gs_error_invalidfont);
    if_debug1('j', "[j]stacking depth=0 font=0x%lx\n",
	      (ulong) pfont);
    pte->fstack.depth = 0;
    pte->fstack.items[0].font = pfont;
    pte->fstack.items[0].index = 0;
    return gs_stack_modal_fonts(pte);
}

/* Select the appropriate descendant of a font. */
/* Uses free variables: pte. */
/* Uses pdata, uses & updates fdepth, sets pfont. */
#define select_descendant(pfont, pdata, fidx, fdepth)\
  if (fidx >= pdata->encoding_size)\
    return_error(gs_error_rangecheck);\
  if (fdepth == MAX_FONT_STACK)\
    return_error(gs_error_invalidfont);\
  pfont = pdata->FDepVector[pdata->Encoding[fidx]];\
  if (++fdepth > orig_depth || pfont != pte->fstack.items[fdepth].font ||\
      orig_index != fidx)\
    pte->fstack.items[fdepth].font = pfont, changed = 1;\
  pte->fstack.items[fdepth].index = fidx

/* Get the root EscChar of a composite font, which overrides the EscChar */
/* of descendant fonts. */
private uint
root_esc_char(const gs_text_enum_t *pte)
{
    return ((gs_font_type0 *) (pte->fstack.items[0].font))->data.EscChar;
}

/* Get the next character or glyph from a composite string. */
/* If we run off the end of the string in the middle of a */
/* multi-byte sequence, return gs_error_rangecheck. */
/* If the string is empty, return 2. */
/* If the current (base) font changed, return 1.  Otherwise, return 0. */
int
gs_type0_next_char_glyph(gs_text_enum_t *pte, gs_char *pchr, gs_glyph *pglyph)
{
    const byte *str = pte->text.data.bytes;
    const byte *p = str + pte->index;
    const byte *end = str + pte->text.size;
    int fdepth = pte->fstack.depth;
    int orig_depth = fdepth;
    int orig_index = pte->fstack.items[fdepth].index;
    gs_font *pfont;

#define pfont0 ((gs_font_type0 *)pfont)
    gs_type0_data *pdata;
    uint fidx;
    gs_char chr;
    gs_glyph glyph = gs_no_glyph;
    int changed = 0;

    pte->FontBBox_as_Metrics2.x = pte->FontBBox_as_Metrics2.y = 0;

#define need_left(n)\
  if ( end - p < n ) return_error(gs_error_rangecheck)

    /*
     * Although the Adobe documentation doesn't say anything about this,
     * if the root font is modal and the very first character of the
     * string being decoded is an escape or shift character, then
     * font selection via the escape mechanism works down from the root,
     * rather than up from the lowest modal font.  (This was first
     * reported by Norio Katayama, and confirmed by someone at Adobe.)
     */

    if (pte->index == 0) {
	int idepth = 0;

	pfont = pte->fstack.items[0].font;
	for (; pfont->FontType == ft_composite;) {
	    fmap_type fmt = (pdata = &pfont0->data)->FMapType;

	    if (p == end)
		return 2;
	    chr = *p;
	    switch (fmt) {
		case fmap_escape:
		    if (chr != root_esc_char(pte))
			break;
		    need_left(2);
		    fidx = p[1];
		    p += 2;
		    if_debug1('j', "[j]from root: escape %d\n", fidx);
		  rdown:select_descendant(pfont, pdata, fidx, idepth);
		    if_debug2('j', "[j]... new depth=%d, new font=0x%lx\n",
			      idepth, (ulong) pfont);
		    continue;
		case fmap_double_escape:
		    if (chr != root_esc_char(pte))
			break;
		    need_left(2);
		    fidx = p[1];
		    p += 2;
		    if (fidx == chr) {
			need_left(1);
			fidx = *p++ + 256;
		    }
		    if_debug1('j', "[j]from root: double escape %d\n", fidx);
		    goto rdown;
		case fmap_shift:
		    if (chr == pdata->ShiftIn)
			fidx = 0;
		    else if (chr == pdata->ShiftOut)
			fidx = 1;
		    else
			break;
		    p++;
		    if_debug1('j', "[j]from root: shift %d\n", fidx);
		    goto rdown;
		default:
		    break;
	    }
	    break;
	}
	/* If we saw any initial escapes or shifts, */
	/* compute a new initial base font. */
	if (idepth != 0) {
	    int code;

	    pte->fstack.depth = idepth;
	    code = gs_stack_modal_fonts(pte);
	    if (code < 0)
		return code;
	    if (pte->fstack.depth > idepth)
		changed = 1;
	    orig_depth = fdepth = pte->fstack.depth;
	}
    }
    /* Handle initial escapes or shifts. */

  up:if (p == end)
	return 2;
    chr = *p;
    while (fdepth > 0) {
	pfont = pte->fstack.items[fdepth - 1].font;
	pdata = &pfont0->data;
	switch (pdata->FMapType) {
	    default:		/* non-modal */
		fdepth--;
		continue;

	    case fmap_escape:
		if (chr != root_esc_char(pte))
		    break;
		need_left(2);
		fidx = *++p;
		if_debug1('j', "[j]next: escape %d\n", fidx);
		/* Per Adobe, if we get an escape at the root, */
		/* treat it as an ordinary character (font index). */
		if (fidx == chr && fdepth > 1) {
		    fdepth--;
		    goto up;
		}
	      down:if (++p == end)
		    return 2;
		chr = *p;
		fdepth--;
		do {
		    select_descendant(pfont, pdata, fidx, fdepth);
		    if_debug3('j', "[j]down from modal: new depth=%d, index=%d, new font=0x%lx\n",
			      fdepth, fidx, (ulong) pfont);
		    if (pfont->FontType != ft_composite)
			break;
		    pdata = &pfont0->data;
		    fidx = 0;
		}
		while (pdata->FMapType == fmap_escape);
		continue;

	    case fmap_double_escape:
		if (chr != root_esc_char(pte))
		    break;
		need_left(2);
		fidx = *++p;
		if (fidx == chr) {
		    need_left(2);
		    fidx = *++p + 256;
		}
		if_debug1('j', "[j]next: double escape %d\n", fidx);
		goto down;

	    case fmap_shift:
		if (chr == pdata->ShiftIn)
		    fidx = 0;
		else if (chr == pdata->ShiftOut)
		    fidx = 1;
		else
		    break;
		if_debug1('j', "[j]next: shift %d\n", fidx);
		goto down;
	}
	break;
    }
    /* At this point, chr == *p. */
    /* (This is important to know for CMap'ed fonts.) */
    p++;

    /*
     * Now handle non-modal descendants.
     * The PostScript language manual has some confusing
     * wording about the parent supplying the "first part"
     * of the child's decoding information; what this means
     * is not (as one might imagine) the font index, but
     * simply the first byte of the data.
     */

    while ((pfont = pte->fstack.items[fdepth].font)->FontType == ft_composite) {
	pdata = &pfont0->data;
	switch (pdata->FMapType) {
	    default:		/* can't happen */
		return_error(gs_error_invalidfont);

	    case fmap_8_8:
		need_left(1);
		fidx = chr;
		chr = *p++;
		if_debug2('J', "[J]8/8 index=%d, char=%ld\n",
			  fidx, chr);
		break;

	    case fmap_1_7:
		fidx = chr >> 7;
		chr &= 0x7f;
		if_debug2('J', "[J]1/7 index=%d, char=%ld\n",
			  fidx, chr);
		break;

	    case fmap_9_7:
		need_left(1);
		fidx = ((uint) chr << 1) + (*p >> 7);
		chr = *p & 0x7f;
		if_debug2('J', "[J]9/7 index=%d, char=%ld\n",
			  fidx, chr);
		p++;
		break;

	    case fmap_SubsVector:
		{
		    int width = pdata->subs_width;
		    uint subs_count = pdata->subs_size;
		    const byte *psv = pdata->SubsVector.data;

#define subs_loop(subs_elt, width)\
  while ( subs_count != 0 && tchr >= (schr = subs_elt) )\
    subs_count--, tchr -= schr, psv += width;\
  chr = tchr; p += width - 1; break

		    switch (width) {
			default:	/* can't happen */
			    return_error(gs_error_invalidfont);
			case 1:
			    {
				byte tchr = (byte) chr, schr;

				subs_loop(*psv, 1);
			    }
			case 2:
			    need_left(1);
#define w2(p) (((ushort)*p << 8) + p[1])
			    {
				ushort tchr = ((ushort) chr << 8) + *p,
				       schr;

				subs_loop(w2(psv), 2);
			    }
			case 3:
			    need_left(2);
#define w3(p) (((ulong)*p << 16) + ((uint)p[1] << 8) + p[2])
			    {
				ulong tchr = ((ulong) chr << 16) + w2(p),
				      schr;

				subs_loop(w3(psv), 3);
			    }
			case 4:
			    need_left(3);
#define w4(p) (((ulong)*p << 24) + ((ulong)p[1] << 16) + ((uint)p[2] << 8) + p[3])
			    {
				ulong tchr = ((ulong) chr << 24) + w3(p),
				      schr;

				subs_loop(w4(psv), 4);
			    }
#undef w2
#undef w3
#undef w4
#undef subs_loop
		    }
		    fidx = pdata->subs_size - subs_count;
		    if_debug2('J', "[J]SubsVector index=%d, char=%ld\n",
			      fidx, chr);
		    break;
		}

	    case fmap_CMap:
		{
		    gs_const_string cstr;
		    uint mindex = p - str - 1;	/* p was incremented */
		    int code;

                    /*
                     * When decoding an FMapType4 or 5, the value
                     * of chr is modified; when an FMapType9 (CMap)
                     * composite font is used as a decendant font,
                     * we have to pass the text including a modified
                     * chr. Check whether chr has been modified, and
                     * if so, construct and pass a modified buffer.
                     */
		    if (*(p - 1) != chr) {
			byte substr[MAX_CMAP_CODE_SIZE];
			int submindex = 0;
			if_debug2('j', "[j] *(p-1) 0x%02x != chr 0x%02x, modified str should be passed\n",
				*(p-1), (byte)chr);
			memcpy(substr, p - 1,
				min(MAX_CMAP_CODE_SIZE, end - p + 1));
			substr[0] = chr;
			cstr.data = substr;
			cstr.size = min(MAX_CMAP_CODE_SIZE, end - p + 1);
			if (gs_debug_c('j')) {
			    dlprintf("[j] original str(");
			    debug_print_string_hex(str, end - str);
			    dlprintf(") -> modified substr(");
			    debug_print_string_hex(cstr.data, cstr.size);
			    dlprintf(")\n");
			}
			code = gs_cmap_decode_next(pdata->CMap, &cstr,
					(uint*) &submindex, &fidx, &chr, &glyph);
			mindex += submindex;
		    } else {
			cstr.data = str;
			cstr.size = end - str;
			code = gs_cmap_decode_next(pdata->CMap, &cstr, &mindex,
					       &fidx, &chr, &glyph);
		    }
		    if (code < 0)
			return code;
		    pte->cmap_code = code; /* hack for widthshow */
		    p = str + mindex;
		    if_debug3('J', "[J]CMap returns %d, chr=0x%lx, glyph=0x%lx\n",
			      code, (ulong) chr, (ulong) glyph);
		    if (code == 0) {
			if (glyph == gs_no_glyph) {
			    glyph = gs_min_cid_glyph;
			    if_debug0('J', "... undefined\n");
			    goto done;
			}
		    } else
			chr = (gs_char) glyph, glyph = gs_no_glyph;
		    /****** RESCAN chr IF DESCENDANT IS CMAP'ED ******/
		    break;
		}
	}

	select_descendant(pfont, pdata, fidx, fdepth);
	if_debug2('J', "... new depth=%d, new font=0x%lx\n",
		  fdepth, (ulong) pfont);
	/* FontBBox may be used as metrics2 with WMode=1 :
	*/
	if (pfont->FontType == ft_CID_encrypted ||
	    pfont->FontType == ft_CID_TrueType
	    ) {
	    gs_font_base *pfb = (gs_font_base *)pfont;

	    pte->FontBBox_as_Metrics2 = pfb->FontBBox.q;
	}
    }
done:
    *pchr = chr;
    *pglyph = glyph;
    /* Update the pointer into the original string, but only if */
    /* we didn't switch over to parsing a code from a CMap. */
    if (str == pte->text.data.bytes)
	pte->index = p - str;
    pte->fstack.depth = fdepth;
    if_debug4('J', "[J]depth=%d font=0x%lx index=%d changed=%d\n",
	      fdepth, (ulong) pte->fstack.items[fdepth].font,
	      pte->fstack.items[fdepth].index, changed);
    return changed;
}
#undef pfont0

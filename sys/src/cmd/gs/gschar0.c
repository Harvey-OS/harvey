/* Copyright (C) 1991, 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* gschar0.c */
/* Composite font decoding for Ghostscript library */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gxfixed.h"
#include "gxdevice.h"
#include "gxdevmem.h"			/* for gxchar.h */
#include "gxchar.h"
#include "gxfont.h"
#include "gxfont0.h"

/* Stack up modal composite fonts, down to a non-modal or base font. */
private int
gs_stack_modal_fonts(gs_show_enum *penum)
{	int fdepth = penum->fstack.depth;
	gs_font *cfont = penum->fstack.items[fdepth].font;
#define cmfont ((gs_font_type0 *)cfont)
	while ( cfont->FontType == ft_composite &&
		fmap_type_is_modal(cmfont->data.FMapType)
	      )
	{	if ( fdepth == max_font_depth )
			return_error(gs_error_invalidfont);
		fdepth++;
		cfont = cmfont->data.FDepVector[cmfont->data.Encoding[0]];
		penum->fstack.items[fdepth].font = cfont;
		penum->fstack.items[fdepth].index = 0;
		if_debug2('j', "[j]stacking depth=%d font=0x%lx\n",
			  fdepth, (ulong)cfont);
	}
	penum->fstack.depth = fdepth;
	return 0;
#undef cmfont
}
/* Initialize the composite font stack for a show enumerator. */
int
gs_type0_init_fstack(gs_show_enum *penum, gs_font *pfont)
{	if_debug1('j', "[j]stacking depth=0 font=0x%lx\n",
		  (ulong)pfont);
	penum->fstack.depth = 0;
	penum->fstack.items[0].font = pfont;
	penum->fstack.items[0].index = 0;
	return gs_stack_modal_fonts(penum);
}

/* Select the appropriate descendant of a font. */
/* Uses free variables: penum. */
/* Uses pdata, uses & updates fdepth, sets pfont. */
#define select_descendant(pfont, pdata, fidx, fdepth)\
  if ( fidx >= pdata->encoding_size )\
    return_error(gs_error_rangecheck);\
  if ( fdepth == max_font_depth )\
    return_error(gs_error_invalidfont);\
  pfont = pdata->FDepVector[pdata->Encoding[fidx]];\
  if ( ++fdepth > orig_depth || pfont != penum->fstack.items[fdepth].font )\
    penum->fstack.items[fdepth].font = pfont,\
    changed = 1;\
  penum->fstack.items[fdepth].index = fidx

/* Get the next character from a composite string. */
/* If we run off the end of the string in the middle of a */
/* multi-byte sequence, return gs_error_rangecheck. */
/* If the string is empty, return 2. */
/* If the current (base) font changed, return 1.  Otherwise, return 0. */
int
gs_type0_next_char(register gs_show_enum *penum, gs_char *pchr)
{	const byte *p = penum->str.data + penum->index;
	const byte *end = penum->str.data + penum->str.size;
	int fdepth = penum->fstack.depth;
	int orig_depth = fdepth;
	gs_font *pfont;
#define pfont0 ((gs_font_type0 *)pfont)
	gs_type0_data *pdata;
	uint fidx;
	gs_char chr;
	int changed = 0;
	bool first = true;
#define need_left(n)\
  if ( end - p < n ) return_error(gs_error_rangecheck)
#define root_EscChar\
  (((gs_font_type0 *)(penum->fstack.items[0].font))->data.EscChar)	/* root overrides */

	/*
	 * Although the Adobe documentation doesn't say anything about this,
	 * if the root font is modal and the very first character of the
	 * string being decoded is an escape or shift character, then
	 * font selection via the escape mechanism works down from the root,
	 * rather than up from the lowest modal font.  (This was first
	 * reported by Norio Katayama, and confirmed by someone at Adobe.)
	 */

	if ( penum->index == 0 )
	  { int idepth = 0;
	    pfont = penum->fstack.items[0].font;
	    for ( ; pfont->FontType == ft_composite; )
	      { fmap_type fmt = (pdata = &pfont0->data)->FMapType;
		if ( p == end )
		  return 2;
		chr = *p;
		switch ( fmt )
		  {
		  case fmap_escape:
		    if ( chr != root_EscChar )
		      break;
		    need_left(2);
		    fidx = p[1];
		    p += 2;
		    if_debug1('j', "[j]from root: escape %d\n", fidx);
rdown:		    select_descendant(pfont, pdata, fidx, idepth);
		    if_debug2('j', "[j]... new depth=%d, new font=0x%lx\n",
			      idepth, (ulong)pfont);
		    continue;
		  case fmap_double_escape:
		    if ( chr != root_EscChar )
		      break;
		    need_left(2);
		    fidx = p[1];
		    p += 2;
		    if ( fidx == chr )
		      {	need_left(1);
			fidx = *p++ + 256;
		      }
		    if_debug1('j', "[j]from root: double escape %d\n", fidx);
		    goto rdown;
		  case fmap_shift:
		    if ( chr == pdata->ShiftIn )
		      fidx = 0;
		    else if ( chr == pdata->ShiftOut )
		      fidx = 1;
		    else break;
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
	    if ( idepth != 0 )
	      { int code;
		penum->fstack.depth = idepth;
		code = gs_stack_modal_fonts(penum);
		if ( code < 0 )
		  return code;
		if ( penum->fstack.depth > idepth )
		  changed = 1;
		orig_depth = fdepth = penum->fstack.depth;
	      }
	  }

		/* Handle initial escapes or shifts. */

up:	if ( p == end )
		return 2;
	chr = *p;
	while ( fdepth > 0 )
	{	pfont = penum->fstack.items[fdepth - 1].font;
		pdata = &pfont0->data;
		switch ( pdata->FMapType )
		{
		default:			/* non-modal */
			fdepth--;
			continue;

		case fmap_escape:
			if ( chr != root_EscChar )
				break;
			need_left(2);
			fidx = *++p;
			if_debug1('j', "[j]next: escape %d\n", fidx);
			/* Per Adobe, if we get an escape at the root, */
			/* treat it as an ordinary character (font index). */
			if ( fidx == chr && fdepth > 1 )
			{	fdepth--;
				goto up;
			}
down:			if ( ++p == end )
				return 2;
			chr = *p;
			fdepth--;
			do
			{	select_descendant(pfont, pdata, fidx, fdepth);
				if_debug3('j', "[j]down from modal: new depth=%d, index=%d, new font=0x%lx\n",
					  fdepth, fidx, (ulong)pfont);
				if ( pfont->FontType != ft_composite )
					break;
				pdata = &pfont0->data;
				fidx = 0;
			}
			while ( pdata->FMapType == fmap_escape );
			continue;

		case fmap_double_escape:
			if ( chr != root_EscChar )
				break;
			need_left(2);
			fidx = *++p;
			if ( fidx == chr )
			{	need_left(2);
				fidx = *++p + 256;
			}
			if_debug1('j', "[j]next: double escape %d\n", fidx);
			goto down;

		case fmap_shift:
			if ( chr == pdata->ShiftIn )
				fidx = 0;
			else if ( chr == pdata->ShiftOut )
				fidx = 1;
			else break;
			if_debug1('j', "[j]next: shift %d\n", fidx);
			goto down;
		}
		break;
	}
	p++;

		/*
		 * Now handle non-modal descendants.
		 * The PostScript language manual has some confusing
		 * wording about the parent supplying the "first part"
		 * of the child's decoding information; what this means
		 * is not (as one might imagine) the font index, but
		 * simply the first byte of the data.
		 */

	while ( (pfont = penum->fstack.items[fdepth].font)->FontType == ft_composite )
	{	pdata = &pfont0->data;
		switch ( pdata->FMapType )
		{
		default:			/* can't happen */
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
			fidx = ((uint)chr << 1) + (*p >> 7);
			chr = *p & 0x7f;
			if_debug2('J', "[J]9/7 index=%d, char=%ld\n",
				  fidx, chr);
			p++;
			break;

		case fmap_SubsVector:
		{	int width = pdata->subs_width;
			uint subs_count = pdata->subs_size;
			const byte *psv = pdata->SubsVector.data;
#define subs_loop(subs_elt, width)\
  while ( subs_count != 0 && tchr >= (schr = subs_elt) )\
    subs_count--, tchr -= schr, psv += width;\
  chr = tchr; p += width - 1; break
			switch ( width )
			{
			default:		/* can't happen */
				return_error(gs_error_invalidfont);
			case 1:
			{	byte tchr = (byte)chr, schr;
				subs_loop(*psv, 1);
			}
			case 2:
				need_left(1);
#define w2(p) (((ushort)*p << 8) + p[1])
			{	ushort tchr = ((ushort)chr << 8) + *p, schr;
				subs_loop(w2(psv), 2);
			}
			case 3:
				need_left(2);
#define w3(p) (((ulong)*p << 16) + ((uint)p[1] << 8) + p[2])
			{	ulong tchr = ((ulong)chr << 16) + w2(p), schr;
				subs_loop(w3(psv), 3);
			}
			case 4:
				need_left(3);
#define w4(p) (((ulong)*p << 24) + ((ulong)p[1] << 16) + ((uint)p[2] << 8) + p[3])
			{	ulong tchr = ((ulong)chr << 24) + w3(p), schr;
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

		}

		select_descendant(pfont, pdata, fidx, fdepth);
		if_debug2('J', "... new depth=%d, new font=0x%lx\n",
			  fdepth, (ulong)pfont);
		first = false;
	}
	*pchr = chr;
	penum->index = p - penum->str.data;
	penum->fstack.depth = fdepth;
	if_debug4('J', "[J]depth=%d font=0x%lx index=%d changed=%d\n",
		  fdepth, (ulong)penum->fstack.items[fdepth].font,
		  penum->fstack.items[fdepth].index, changed);
	return changed;
#undef pfont0
}

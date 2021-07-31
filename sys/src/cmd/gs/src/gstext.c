/* Copyright (C) 1998, 2000 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gstext.c,v 1.2 2000/03/10 04:41:47 lpd Exp $ */
/* Driver text interface support */
#include "memory_.h"
#include "gstypes.h"
#include "gdebug.h"
#include "gserror.h"
#include "gserrors.h"
#include "gsmemory.h"
#include "gsstruct.h"
#include "gstypes.h"
#include "gxdevcli.h"
#include "gxfont.h"		/* for init_fstack */
#include "gxpath.h"
#include "gxtext.h"
#include "gzstate.h"

/* GC descriptors */
public_st_gs_text_params();
public_st_gs_text_enum();

private 
ENUM_PTRS_WITH(text_params_enum_ptrs, gs_text_params_t *tptr) return 0;
case 0:
if (tptr->operation & TEXT_FROM_STRING) {
    return ENUM_CONST_STRING2(tptr->data.bytes, tptr->size);
}
if (tptr->operation & TEXT_FROM_BYTES)
    return ENUM_OBJ(tptr->data.bytes);
if (tptr->operation & TEXT_FROM_CHARS)
    return ENUM_OBJ(tptr->data.chars);
if (tptr->operation & TEXT_FROM_GLYPHS)
    return ENUM_OBJ(tptr->data.glyphs);
return ENUM_OBJ(NULL);
case 1:
return ENUM_OBJ(tptr->operation & TEXT_REPLACE_WIDTHS ?
		tptr->x_widths : NULL);
case 2:
return ENUM_OBJ(tptr->operation & TEXT_REPLACE_WIDTHS ?
		tptr->y_widths : NULL);
ENUM_PTRS_END
private RELOC_PTRS_WITH(text_params_reloc_ptrs, gs_text_params_t *tptr)
{
    if (tptr->operation & TEXT_FROM_STRING) {
	gs_const_string str;

	str.data = tptr->data.bytes;
	str.size = tptr->size;
	RELOC_CONST_STRING_VAR(str);
	tptr->data.bytes = str.data;
    } else if (tptr->operation & TEXT_FROM_BYTES)
	RELOC_OBJ_VAR(tptr->data.bytes);
    else if (tptr->operation & TEXT_FROM_CHARS)
	RELOC_OBJ_VAR(tptr->data.chars);
    else if (tptr->operation & TEXT_FROM_GLYPHS)
	RELOC_OBJ_VAR(tptr->data.glyphs);
    if (tptr->operation & TEXT_REPLACE_WIDTHS) {
	RELOC_OBJ_VAR(tptr->x_widths);
	RELOC_OBJ_VAR(tptr->y_widths);
    }
}
RELOC_PTRS_END

private ENUM_PTRS_WITH(text_enum_enum_ptrs, gs_text_enum_t *eptr)
{
    index -= 7;
    if (index <= eptr->fstack.depth)
	ENUM_RETURN(eptr->fstack.items[index].font);
    index -= eptr->fstack.depth + 1;
     return ENUM_USING(st_gs_text_params, &eptr->text, sizeof(eptr->text), index);
}
case 0: return ENUM_OBJ(gx_device_enum_ptr(eptr->dev));
ENUM_PTR3(1, gs_text_enum_t, pis, orig_font, path);
ENUM_PTR3(4, gs_text_enum_t, pdcolor, pcpath, current_font);
ENUM_PTRS_END

private RELOC_PTRS_WITH(text_enum_reloc_ptrs, gs_text_enum_t *eptr)
{
    int i;

    RELOC_USING(st_gs_text_params, &eptr->text, sizeof(eptr->text));
    eptr->dev = gx_device_reloc_ptr(eptr->dev, gcst);
    RELOC_PTR3(gs_text_enum_t, pis, orig_font, path);
    RELOC_PTR3(gs_text_enum_t, pdcolor, pcpath, current_font);
    for (i = 0; i <= eptr->fstack.depth; i++)
	RELOC_PTR(gs_text_enum_t, fstack.items[i].font);
}
RELOC_PTRS_END

/* Begin processing text. */
int
gx_device_text_begin(gx_device * dev, gs_imager_state * pis,
		     const gs_text_params_t * text, gs_font * font,
		     gx_path * path,	/* unless DO_NONE & !RETURN_WIDTH */
		     const gx_device_color * pdcolor,	/* DO_DRAW */
		     const gx_clip_path * pcpath,	/* DO_DRAW */
		     gs_memory_t * mem, gs_text_enum_t ** ppte)
{
    if (TEXT_PARAMS_ARE_INVALID(text))
	return_error(gs_error_rangecheck);
    {
	gx_path *tpath =
	    ((text->operation & TEXT_DO_NONE) &&
	     !(text->operation & TEXT_RETURN_WIDTH) ? 0 : path);
	const gx_device_color *tcolor =
	    (text->operation & TEXT_DO_DRAW ? pdcolor : 0);
	const gx_clip_path *tcpath =
	    (text->operation & TEXT_DO_DRAW ? pcpath : 0);
	return dev_proc(dev, text_begin)
	    (dev, pis, text, font, tpath, tcolor, tcpath, mem, ppte);
    }
}

/* 
 * Initialize a newly created text enumerator.  Implementations of
 * text_begin must call this just after allocating the enumerator.
 */
private int
gs_text_enum_init_dynamic(gs_text_enum_t *pte, gs_font *font)
{
    pte->current_font = font;
    pte->index = 0;
    pte->xy_index = 0;
    return font->procs.init_fstack(pte, font);
}
int
gs_text_enum_init(gs_text_enum_t *pte, const gs_text_enum_procs_t *procs,
		  gx_device *dev, gs_imager_state *pis,
		  const gs_text_params_t *text, gs_font *font, gx_path *path,
		  const gx_device_color *pdcolor, const gx_clip_path *pcpath,
		  gs_memory_t *mem)
{
    int code;

    pte->text = *text;
    pte->dev = dev;
    pte->pis = pis;
    pte->orig_font = font;
    pte->path = path;
    pte->pdcolor = pdcolor;
    pte->pcpath = pcpath;
    pte->memory = mem;
    pte->procs = procs;
    /* text_begin procedure sets rc */
    /* init_dynamic sets current_font */
    pte->log2_scale.x = pte->log2_scale.y = 0;
    /* init_dynamic sets index, xy_index, fstack */
    code = gs_text_enum_init_dynamic(pte, font);
    if (code >= 0)
	rc_increment(dev);
    return code;
}

/*
 * Copy the dynamically changing elements from one enumerator to another.
 * This is useful primarily for enumerators that sometimes pass the
 * operation to a subsidiary enumerator.
 */
void
gs_text_enum_copy_dynamic(gs_text_enum_t *pto, const gs_text_enum_t *pfrom,
			  bool for_return)
{
    int depth = pfrom->fstack.depth;

    pto->current_font = pfrom->current_font;
    pto->index = pfrom->index;
    pto->xy_index = pfrom->xy_index;
    pto->fstack.depth = depth;
    if (depth >= 0)
	memcpy(pto->fstack.items, pfrom->fstack.items,
	       (depth + 1) * sizeof(pto->fstack.items[0]));
    if (for_return) {
	pto->cmap_code = pfrom->cmap_code;
	pto->returned = pfrom->returned;
    }
}

/* Begin processing text based on a graphics state. */
int
gs_text_begin(gs_state * pgs, const gs_text_params_t * text,
	      gs_memory_t * mem, gs_text_enum_t ** ppte)
{
    gx_clip_path *pcpath = 0;

    if (text->operation & TEXT_DO_DRAW) {
	int code = gx_effective_clip_path(pgs, &pcpath);

	if (code < 0)
	    return code;
	gx_set_dev_color(pgs);
    }
    return gx_device_text_begin(pgs->device, (gs_imager_state *) pgs,
				text, pgs->font, pgs->path, pgs->dev_color,
				pcpath, mem, ppte);
}

/* Begin PostScript-equivalent text operations. */
int
gs_show_begin(gs_state * pgs, const byte * str, uint size,
	      gs_memory_t * mem, gs_text_enum_t ** ppte)
{
    gs_text_params_t text;

    text.operation = TEXT_FROM_STRING | TEXT_DO_DRAW | TEXT_RETURN_WIDTH;
    text.data.bytes = str, text.size = size;
    return gs_text_begin(pgs, &text, mem, ppte);
}
int
gs_ashow_begin(gs_state * pgs, floatp ax, floatp ay, const byte * str, uint size,
	       gs_memory_t * mem, gs_text_enum_t ** ppte)
{
    gs_text_params_t text;

    text.operation = TEXT_FROM_STRING | TEXT_ADD_TO_ALL_WIDTHS |
	TEXT_DO_DRAW | TEXT_RETURN_WIDTH;
    text.data.bytes = str, text.size = size;
    text.delta_all.x = ax;
    text.delta_all.y = ay;
    return gs_text_begin(pgs, &text, mem, ppte);
}
int
gs_widthshow_begin(gs_state * pgs, floatp cx, floatp cy, gs_char chr,
		   const byte * str, uint size,
		   gs_memory_t * mem, gs_text_enum_t ** ppte)
{
    gs_text_params_t text;

    text.operation = TEXT_FROM_STRING | TEXT_ADD_TO_SPACE_WIDTH |
	TEXT_DO_DRAW | TEXT_RETURN_WIDTH;
    text.data.bytes = str, text.size = size;
    text.delta_space.x = cx;
    text.delta_space.y = cy;
    text.space.s_char = chr;
    return gs_text_begin(pgs, &text, mem, ppte);
}
int
gs_awidthshow_begin(gs_state * pgs, floatp cx, floatp cy, gs_char chr,
		    floatp ax, floatp ay, const byte * str, uint size,
		    gs_memory_t * mem, gs_text_enum_t ** ppte)
{
    gs_text_params_t text;

    text.operation = TEXT_FROM_STRING |
	TEXT_ADD_TO_ALL_WIDTHS | TEXT_ADD_TO_SPACE_WIDTH |
	TEXT_DO_DRAW | TEXT_RETURN_WIDTH;
    text.data.bytes = str, text.size = size;
    text.delta_space.x = cx;
    text.delta_space.y = cy;
    text.space.s_char = chr;
    text.delta_all.x = ax;
    text.delta_all.y = ay;
    return gs_text_begin(pgs, &text, mem, ppte);
}
int
gs_kshow_begin(gs_state * pgs, const byte * str, uint size,
	       gs_memory_t * mem, gs_text_enum_t ** ppte)
{
    gs_text_params_t text;

    text.operation = TEXT_FROM_STRING | TEXT_DO_DRAW | TEXT_INTERVENE |
	TEXT_RETURN_WIDTH;
    text.data.bytes = str, text.size = size;
    return gs_text_begin(pgs, &text, mem, ppte);
}
int
gs_xyshow_begin(gs_state * pgs, const byte * str, uint size,
		const float *x_widths, const float *y_widths,
		uint widths_size, gs_memory_t * mem, gs_text_enum_t ** ppte)
{
    gs_text_params_t text;

    text.operation = TEXT_FROM_STRING | TEXT_REPLACE_WIDTHS |
	TEXT_DO_DRAW | TEXT_RETURN_WIDTH;
    text.data.bytes = str, text.size = size;
    text.x_widths = x_widths;
    text.y_widths = y_widths;
    text.widths_size = widths_size;
    return gs_text_begin(pgs, &text, mem, ppte);
}
int
gs_glyphshow_begin(gs_state * pgs, gs_glyph glyph,
		   gs_memory_t * mem, gs_text_enum_t ** ppte)
{
    gs_text_params_t text;

    text.operation = TEXT_FROM_SINGLE_GLYPH | TEXT_DO_DRAW | TEXT_RETURN_WIDTH;
    text.data.d_glyph = glyph;
    text.size = 1;
    return gs_text_begin(pgs, &text, mem, ppte);
}
int
gs_cshow_begin(gs_state * pgs, const byte * str, uint size,
	       gs_memory_t * mem, gs_text_enum_t ** ppte)
{
    gs_text_params_t text;

    text.operation = TEXT_FROM_STRING | TEXT_DO_NONE | TEXT_INTERVENE;
    text.data.bytes = str, text.size = size;
    return gs_text_begin(pgs, &text, mem, ppte);
}
int
gs_stringwidth_begin(gs_state * pgs, const byte * str, uint size,
		     gs_memory_t * mem, gs_text_enum_t ** ppte)
{
    gs_text_params_t text;

    text.operation = TEXT_FROM_STRING | TEXT_DO_NONE | TEXT_RETURN_WIDTH;
    text.data.bytes = str, text.size = size;
    return gs_text_begin(pgs, &text, mem, ppte);
}
int
gs_charpath_begin(gs_state * pgs, const byte * str, uint size, bool stroke_path,
		  gs_memory_t * mem, gs_text_enum_t ** ppte)
{
    gs_text_params_t text;

    text.operation = TEXT_FROM_STRING | TEXT_RETURN_WIDTH |
	(stroke_path ? TEXT_DO_TRUE_CHARPATH : TEXT_DO_FALSE_CHARPATH);
    text.data.bytes = str, text.size = size;
    return gs_text_begin(pgs, &text, mem, ppte);
}
int
gs_charboxpath_begin(gs_state * pgs, const byte * str, uint size,
		bool stroke_path, gs_memory_t * mem, gs_text_enum_t ** ppte)
{
    gs_text_params_t text;

    text.operation = TEXT_FROM_STRING | TEXT_RETURN_WIDTH |
	(stroke_path ? TEXT_DO_TRUE_CHARBOXPATH : TEXT_DO_FALSE_CHARBOXPATH);
    text.data.bytes = str, text.size = size;
    return gs_text_begin(pgs, &text, mem, ppte);
}
int
gs_glyphpath_begin(gs_state * pgs, gs_glyph glyph, bool stroke_path,
		   gs_memory_t * mem, gs_text_enum_t ** ppte)
{
    gs_text_params_t text;

    text.operation = TEXT_FROM_SINGLE_GLYPH | TEXT_RETURN_WIDTH |
	(stroke_path ? TEXT_DO_TRUE_CHARPATH : TEXT_DO_FALSE_CHARPATH);
    text.data.d_glyph = glyph;
    text.size = 1;
    return gs_text_begin(pgs, &text, mem, ppte);
}
int
gs_glyphwidth_begin(gs_state * pgs, gs_glyph glyph,
		    gs_memory_t * mem, gs_text_enum_t ** ppte)
{
    gs_text_params_t text;

    text.operation = TEXT_FROM_SINGLE_GLYPH | TEXT_DO_NONE | TEXT_RETURN_WIDTH;
    text.data.d_glyph = glyph;
    text.size = 1;
    return gs_text_begin(pgs, &text, mem, ppte);
}

/* Restart processing with different parameters. */
int
gs_text_restart(gs_text_enum_t *pte, const gs_text_params_t *text)
{
    gs_text_enum_t tenum;

    tenum = *pte;
    tenum.text = *text;
    gs_text_enum_init_dynamic(&tenum, pte->orig_font);
    return gs_text_resync(pte, &tenum);
}

/*
 * Resync text processing with new parameters and string position.
 */
int
gs_text_resync(gs_text_enum_t *pte, const gs_text_enum_t *pfrom)
{
    return pte->procs->resync(pte, pfrom);
}

/* Process text after 'begin'. */
int
gs_text_process(gs_text_enum_t * pte)
{
    return pte->procs->process(pte);
}

/* Access elements of the enumerator. */
gs_font *
gs_text_current_font(const gs_text_enum_t *pte)
{
    return pte->current_font;
}
gs_char
gs_text_current_char(const gs_text_enum_t *pte)
{
    return pte->returned.current_char;
}
gs_char
gs_text_next_char(const gs_text_enum_t *pte)
{
    const uint operation = pte->text.operation;

    if (pte->index >= pte->text.size)
	return gs_no_char;	/* rangecheck */
    if (operation & (TEXT_FROM_STRING | TEXT_FROM_BYTES))
	return pte->text.data.bytes[pte->index];
    if (operation & TEXT_FROM_CHARS)
	return pte->text.data.chars[pte->index];
    return gs_no_char;		/* rangecheck */
}
gs_glyph
gs_text_current_glyph(const gs_text_enum_t *pte)
{
    return pte->returned.current_glyph;
}
int 
gs_text_total_width(const gs_text_enum_t *pte, gs_point *pwidth)
{
    *pwidth = pte->returned.total_width;
    return 0;
}

/* Assuming REPLACE_WIDTHS is set, return the width of the i'th character. */
int
gs_text_replaced_width(const gs_text_params_t *text, uint index,
		       gs_point *pwidth)
{
    const float *x_widths = text->x_widths;
    const float *y_widths = text->y_widths;

    if (index > text->size)
	return_error(gs_error_rangecheck);
    if (x_widths == y_widths) {
	if (x_widths) {
	    index *= 2;
	    pwidth->x = x_widths[index];
	    pwidth->y = x_widths[index + 1];
	}
	else
	    pwidth->x = pwidth->y = 0;
    } else {
	pwidth->x = (x_widths ? x_widths[index] : 0.0);
	pwidth->y = (y_widths ? y_widths[index] : 0.0);
    }
    return 0;
}

/* Determine whether only the width is needed. */
bool
gs_text_is_width_only(const gs_text_enum_t * pte)
{
    return pte->procs->is_width_only(pte);
}

/* Return the width of the current character. */
int
gs_text_current_width(const gs_text_enum_t * pte, gs_point *pwidth)
{
    return pte->procs->current_width(pte, pwidth);
}

/* Set text metrics and optionally enable caching. */
int
gs_text_set_cache(gs_text_enum_t * pte, const double *values,
		  gs_text_cache_control_t control)
{
    return pte->procs->set_cache(pte, values, control);
}
int
gs_text_setcharwidth(gs_text_enum_t * pte, const double wxy[2])
{
    return pte->procs->set_cache(pte, wxy, TEXT_SET_CHAR_WIDTH);
}
int
gs_text_setcachedevice(gs_text_enum_t * pte, const double wbox[6])
{
    return pte->procs->set_cache(pte, wbox, TEXT_SET_CACHE_DEVICE);
}
int
gs_text_setcachedevice2(gs_text_enum_t * pte, const double wbox2[10])
{
    return pte->procs->set_cache(pte, wbox2, TEXT_SET_CACHE_DEVICE2);
}

/* Retry processing the last character without caching. */
int
gs_text_retry(gs_text_enum_t * pte)
{
    return pte->procs->retry(pte);
}

/* Release the text processing structures. */
void
gx_default_text_release(gs_text_enum_t *pte, client_name_t cname)
{
    rc_decrement_only(pte->dev, cname);
}
void
rc_free_text_enum(gs_memory_t * mem, void *obj, client_name_t cname)
{
    gs_text_enum_t *pte = obj;

    pte->procs->release(pte, cname);
    rc_free_struct_only(mem, obj, cname);
}
void
gs_text_release(gs_text_enum_t * pte, client_name_t cname)
{
    rc_decrement_only(pte, cname);
}

/* ---------------- Default font rendering procedures ---------------- */

/* Default fstack initialization procedure. */
int
gs_default_init_fstack(gs_text_enum_t *pte, gs_font *pfont)
{
    pte->fstack.depth = -1;
    return 0;
}

/* Default next-glyph procedure. */
int
gs_default_next_char_glyph(gs_text_enum_t *pte, gs_char *pchr, gs_glyph *pglyph)
{
    if (pte->index >= pte->text.size)
	return 2;
    if (pte->text.operation & (TEXT_FROM_STRING | TEXT_FROM_BYTES)) {
	/* ordinary string */
	*pchr = pte->text.data.bytes[pte->index];
	*pglyph = gs_no_glyph;
    } else if (pte->text.operation & TEXT_FROM_SINGLE_GLYPH) {
	/* glyphshow or glyphpath */
	*pchr = gs_no_char;
	*pglyph = pte->text.data.d_glyph;
    } else if (pte->text.operation & TEXT_FROM_GLYPHS) {
	*pchr = gs_no_char;
	*pglyph = pte->text.data.glyphs[pte->index];
    } else if (pte->text.operation & TEXT_FROM_SINGLE_CHAR) {
	*pchr = pte->text.data.d_char;
	*pglyph = gs_no_glyph;
    } else if (pte->text.operation & TEXT_FROM_CHARS) {
	*pchr = pte->text.data.chars[pte->index];
	*pglyph = gs_no_glyph;
    } else
	return_error(gs_error_rangecheck); /* shouldn't happen */
    pte->index++;
    return 0;
}

/* Dummy (ineffective) BuildChar/BuildGlyph procedure */
int
gs_no_build_char(gs_text_enum_t *pte, gs_state *pgs, gs_font *pfont,
		 gs_char chr, gs_glyph glyph)
{
    return 1;			/* failure, but not error */
}

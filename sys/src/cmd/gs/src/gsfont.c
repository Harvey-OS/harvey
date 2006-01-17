/* Copyright (C) 1989, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsfont.c,v 1.37 2005/07/27 11:24:38 igor Exp $ */
/* Font operators for Ghostscript library */
#include "gx.h"
#include "memory_.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gsutil.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gzstate.h"		/* must precede gxdevice */
#include "gxdevice.h"		/* must precede gxfont */
#include "gxfont.h"
#include "gxfcache.h"
#include "gzpath.h"		/* for default implementation */

/* Define the sizes of the various aspects of the font/character cache. */
/*** Big memory machines ***/
#define smax_LARGE 50		/* smax - # of scaled fonts */
#define bmax_LARGE 500000	/* bmax - space for cached chars */
#define mmax_LARGE 200		/* mmax - # of cached font/matrix pairs */
#define cmax_LARGE 5000		/* cmax - # of cached chars */
#define blimit_LARGE 2500	/* blimit/upper - max size of a single cached char */
/*** Small memory machines ***/
#define smax_SMALL 20		/* smax - # of scaled fonts */
#define bmax_SMALL 25000	/* bmax - space for cached chars */
#define mmax_SMALL 40		/* mmax - # of cached font/matrix pairs */
#define cmax_SMALL 500		/* cmax - # of cached chars */
#define blimit_SMALL 100	/* blimit/upper - max size of a single cached char */

/* Define a default procedure vector for fonts. */
const gs_font_procs gs_font_procs_default = {
    gs_no_define_font,		/* (actually a default) */
    gs_no_make_font,		/* (actually a default) */
    gs_default_font_info,
    gs_default_same_font,
    gs_no_encode_char,
    gs_no_decode_glyph,
    gs_no_enumerate_glyph,
    gs_default_glyph_info,
    gs_no_glyph_outline,
    gs_no_glyph_name,
    gs_default_init_fstack,
    gs_default_next_char_glyph,
    gs_no_build_char
};

private_st_font_dir();
private struct_proc_enum_ptrs(font_enum_ptrs);
private struct_proc_reloc_ptrs(font_reloc_ptrs);

public_st_gs_font_info();
public_st_gs_font();
public_st_gs_font_base();
private_st_gs_font_ptr();
public_st_gs_font_ptr_element();

/*
 * Garbage collection of fonts poses some special problems.  On the one
 * hand, we need to keep track of all existing base (not scaled) fonts,
 * using the next/prev list whose head is the orig_fonts member of the font
 * directory; on the other hand, we want these to be "weak" pointers that
 * don't keep fonts in existence if the fonts aren't referenced from
 * anywhere else.  We accomplish this as follows:
 *
 *     We don't trace through gs_font_dir.orig_fonts or gs_font.{next,prev}
 * during the mark phase of the GC.
 *
 *     When we finalize a base gs_font, we unlink it from the list.  (A
 * gs_font is a base font iff its base member points to itself.)
 *
 *     We *do* relocate the orig_fonts and next/prev pointers during the
 * relocation phase of the GC.  */

/* Font directory GC procedures */
private 
ENUM_PTRS_WITH(font_dir_enum_ptrs, gs_font_dir *dir)
{
    /* Enumerate pointers from cached characters to f/m pairs, */
    /* and mark the cached character glyphs. */
    /* See gxfcache.h for why we do this here. */
    uint cci = index - st_font_dir_max_ptrs;
    uint offset, count;
    uint tmask = dir->ccache.table_mask;

    if (cci == 0)
	offset = 0, count = 1;
    else if (cci == dir->enum_index + 1)
	offset = dir->enum_offset + 1, count = 1;
    else
	offset = 0, count = cci;
    for (; offset <= tmask; ++offset) {
	cached_char *cc = dir->ccache.table[offset];

	if (cc != 0 && !--count) {
	    (*dir->ccache.mark_glyph)
		(mem, cc->code, dir->ccache.mark_glyph_data);
	    /****** HACK: break const.  We'll fix this someday. ******/
	    ((gs_font_dir *)dir)->enum_index = cci;
	    ((gs_font_dir *)dir)->enum_offset = offset;
	    ENUM_RETURN(cc_pair(cc) - cc->pair_index);
	}
    }
}
return 0;
#define e1(i,elt) ENUM_PTR(i,gs_font_dir,elt);
font_dir_do_ptrs(e1)
#undef e1
ENUM_PTRS_END
private RELOC_PTRS_WITH(font_dir_reloc_ptrs, gs_font_dir *dir);
    /* Relocate the pointers from cached characters to f/m pairs. */
    /* See gxfcache.h for why we do this here. */
{
    int chi;

    for (chi = dir->ccache.table_mask; chi >= 0; --chi) {
	cached_char *cc = dir->ccache.table[chi];

	if (cc != 0)
	    cc_set_pair_only(cc,
			     (cached_fm_pair *)
			     RELOC_OBJ(cc_pair(cc) - cc->pair_index) +
			     cc->pair_index);
    }
}
    /* We have to relocate the cached characters before we */
    /* relocate dir->ccache.table! */
RELOC_PTR(gs_font_dir, orig_fonts);
#define r1(i,elt) RELOC_PTR(gs_font_dir, elt);
font_dir_do_ptrs(r1)
#undef r1
RELOC_PTRS_END

/* GC procedures for fonts */
/*
 * When we finalize a base font, we unlink it from the orig_fonts list;
 * when we finalize a scaled font, we unlink it from scaled_fonts.
 * See above for more information.
 */
void
gs_font_finalize(void *vptr)
{
    gs_font *const pfont = vptr;
    gs_font **ppfirst;
    gs_font *next = pfont->next;
    gs_font *prev = pfont->prev;

    if_debug4('u', "[u]unlinking font 0x%lx, base=0x%lx, prev=0x%lx, next=0x%lx\n",
	    (ulong) pfont, (ulong) pfont->base, (ulong) prev, (ulong) next);
    /* Notify clients that the font is being freed. */
    gs_notify_all(&pfont->notify_list, NULL);
    if (pfont->dir == 0)
	ppfirst = 0;
    else if (pfont->base == pfont)
	ppfirst = &pfont->dir->orig_fonts;
    else {
	/*
	 * Track the number of cached scaled fonts.  Only decrement the
	 * count if we didn't do this already in gs_makefont.
	 */
	if (next || prev || pfont->dir->scaled_fonts == pfont)
	    pfont->dir->ssize--;
	ppfirst = &pfont->dir->scaled_fonts;
    }
    /*
     * gs_purge_font may have unlinked this font already:
     * don't unlink it twice.
     */
    if (next != 0 && next->prev == pfont)
	next->prev = prev;
    if (prev != 0) {
	if (prev->next == pfont)
	    prev->next = next;
    } else if (ppfirst != 0 && *ppfirst == pfont)
	*ppfirst = next;
    gs_notify_release(&pfont->notify_list);
}
private 
ENUM_PTRS_WITH(font_enum_ptrs, gs_font *pfont) return ENUM_USING(st_gs_notify_list, &pfont->notify_list, sizeof(gs_notify_list_t), index - 5);
	/* We don't enumerate next or prev of base fonts. */
	/* See above for details. */
case 0: ENUM_RETURN((pfont->base == pfont ? 0 : pfont->next));
case 1: ENUM_RETURN((pfont->base == pfont ? 0 : pfont->prev));
ENUM_PTR3(2, gs_font, dir, base, client_data);
ENUM_PTRS_END
private RELOC_PTRS_WITH(font_reloc_ptrs, gs_font *pfont);
RELOC_USING(st_gs_notify_list, &pfont->notify_list, sizeof(gs_notify_list_t));
	/* We *do* always relocate next and prev. */
	/* Again, see above for details. */
RELOC_PTR(gs_font, next);
RELOC_PTR(gs_font, prev);
RELOC_PTR3(gs_font, dir, base, client_data);
RELOC_PTRS_END

/* Allocate a font directory */
private bool
cc_no_mark_glyph(const gs_memory_t *mem, gs_glyph glyph, void *ignore_data)
{
    return false;
}
gs_font_dir *
gs_font_dir_alloc2(gs_memory_t * struct_mem, gs_memory_t * bits_mem)
{
    gs_font_dir *pdir = 0;

#if !arch_small_memory
#  ifdef DEBUG
    if (!gs_debug_c('.'))
#  endif
    {				/* Try allocating a very large cache. */
	/* If this fails, allocate a small one. */
	pdir = gs_font_dir_alloc2_limits(struct_mem, bits_mem,
					 smax_LARGE, bmax_LARGE, mmax_LARGE,
					 cmax_LARGE, blimit_LARGE);
    }
    if (pdir == 0)
#endif
	pdir = gs_font_dir_alloc2_limits(struct_mem, bits_mem,
					 smax_SMALL, bmax_SMALL, mmax_SMALL,
					 cmax_SMALL, blimit_SMALL);
    if (pdir == 0)
	return 0;
    pdir->ccache.mark_glyph = cc_no_mark_glyph;
    pdir->ccache.mark_glyph_data = 0;
    return pdir;
}
gs_font_dir *
gs_font_dir_alloc2_limits(gs_memory_t * struct_mem, gs_memory_t * bits_mem,
		     uint smax, uint bmax, uint mmax, uint cmax, uint upper)
{
    gs_font_dir *pdir =
	gs_alloc_struct(struct_mem, gs_font_dir, &st_font_dir,
			"font_dir_alloc(dir)");
    int code;

    if (pdir == 0)
	return 0;
    code = gx_char_cache_alloc(struct_mem, bits_mem, pdir,
			       bmax, mmax, cmax, upper);
    if (code < 0) {
	gs_free_object(struct_mem, pdir, "font_dir_alloc(dir)");
	return 0;
    }
    pdir->orig_fonts = 0;
    pdir->scaled_fonts = 0;
    pdir->ssize = 0;
    pdir->smax = smax;
    pdir->align_to_pixels = false;
    pdir->glyph_to_unicode_table = NULL;
    pdir->grid_fit_tt = 2;
    pdir->memory = struct_mem;
    pdir->tti = 0;
    pdir->san = 0;
    pdir->global_glyph_code = NULL;
    return pdir;
}

/* Allocate and minimally initialize a font. */
gs_font *
gs_font_alloc(gs_memory_t *mem, gs_memory_type_ptr_t pstype,
	      const gs_font_procs *procs, gs_font_dir *dir,
	      client_name_t cname)
{
    gs_font *pfont = gs_alloc_struct(mem, gs_font, pstype, cname);

    if (pfont == 0)
	return 0;
#if 1 /* Clear entire structure to avoid unitialized pointers 
         when the initialization exits prematurely by error. */
    memset(pfont, 0, pstype->ssize);
    pfont->memory = mem;
    pfont->dir = dir;
    gs_font_notify_init(pfont);
    pfont->id = gs_next_ids(mem, 1);
    pfont->base = pfont;
    pfont->ExactSize = pfont->InBetweenSize = pfont->TransformedChar =
	fbit_use_outlines;
    pfont->procs = *procs;
#else
    /* For clarity we leave old initializations here
       to know which fields needs to be initialized. */
    pfont->next = pfont->prev = 0;
    pfont->memory = mem;
    pfont->dir = dir;
    pfont->is_resource = false;
    gs_font_notify_init(pfont);
    pfont->id = gs_next_ids(mem, 1);
    pfont->base = pfont;
    pfont->client_data = 0;
    /* not FontMatrix, FontType */
    pfont->BitmapWidths = false;
    pfont->ExactSize = pfont->InBetweenSize = pfont->TransformedChar =
	fbit_use_outlines;
    pfont->WMode = 0;
    pfont->PaintType = 0;
    pfont->StrokeWidth = 0;
    pfont->procs = *procs;
    memset(&pfont->orig_FontMatrix, 0, sizeof(pfont->orig_FontMatrix));
#endif
    /* not key_name, font_name */
    return pfont;
}
/* Allocate and minimally initialize a base font. */
gs_font_base *
gs_font_base_alloc(gs_memory_t *mem, gs_memory_type_ptr_t pstype,
		   const gs_font_procs *procs, gs_font_dir *dir,
		   client_name_t cname)
{
    gs_font_base *pfont =
	(gs_font_base *)gs_font_alloc(mem, pstype, procs, dir, cname);

    if (pfont == 0)
	return 0;
    pfont->FontBBox.p.x = pfont->FontBBox.p.y =
	pfont->FontBBox.q.x = pfont->FontBBox.q.y = 0;
    uid_set_invalid(&pfont->UID);
    pfont->encoding_index = pfont->nearest_encoding_index = -1;
    return pfont;
}

/* Initialize the notification list for a font. */
void
gs_font_notify_init(gs_font *font)
{
    /*
     * The notification list for a font must be allocated in the font's
     * stable memory, because of the following possible sequence of events:
     *
     *   - Allocate font X in local VM.
     *   - Client A registers for notification when X is freed.
     *   - 'save'
     *   - Client B registers for notification when X is freed.
     *   - 'restore'
     *
     * If the notification list element for client B is allocated in
     * restorable local VM (i.e., the same VM as the font), then when the
     * 'restore' occurs, either the list element will be deleted (not what
     * client B wants, because font X hasn't been freed yet), or there will
     * be a dangling pointer.
     */
    gs_notify_init(&font->notify_list, gs_memory_stable(font->memory));
}


/*
 * Register/unregister a client for notification by a font.  Currently
 * the clients are only notified when a font is freed.  Note that any
 * such client must unregister itself when *it* is freed.
 */
int
gs_font_notify_register(gs_font *font, gs_notify_proc_t proc, void *proc_data)
{
    return gs_notify_register(&font->notify_list, proc, proc_data);
}
int
gs_font_notify_unregister(gs_font *font, gs_notify_proc_t proc, void *proc_data)
{
    return gs_notify_unregister(&font->notify_list, proc, proc_data);
}

/* Link an element at the head of a chain. */
private void
font_link_first(gs_font **pfirst, gs_font *elt)
{
    gs_font *first = elt->next = *pfirst;

    if (first)
	first->prev = elt;
    elt->prev = 0;
    *pfirst = elt;
}

/* definefont */
/* Use this only for original (unscaled) fonts! */
/* Note that it expects pfont->procs.define_font to be set already. */
int
gs_definefont(gs_font_dir * pdir, gs_font * pfont)
{
    int code;

    pfont->dir = pdir;
    pfont->base = pfont;
    code = (*pfont->procs.define_font) (pdir, pfont);
    if (code < 0) {		/* Make sure we don't try to finalize this font. */
	pfont->base = 0;
	return code;
    }
    font_link_first(&pdir->orig_fonts, pfont);
    if_debug2('m', "[m]defining font 0x%lx, next=0x%lx\n",
	      (ulong) pfont, (ulong) pfont->next);
    return 0;
}

/* Find a sililar registered font of same type. */
int
gs_font_find_similar(const gs_font_dir * pdir, const gs_font **ppfont, 
		       int (*similar)(const gs_font *, const gs_font *))
{
    const gs_font *pfont0 = *ppfont;
    const gs_font *pfont1 = pdir->orig_fonts;

    for (; pfont1 != NULL; pfont1 = pfont1->next) {
	if (pfont1 != pfont0 && pfont1->FontType == pfont0->FontType) {
	    int code = similar(pfont0, pfont1);
	    if (code != 0) {
		*ppfont = pfont1;
		return code;
	    }
	}
    }
    return 0;
}

/* scalefont */
int
gs_scalefont(gs_font_dir * pdir, const gs_font * pfont, floatp scale,
	     gs_font ** ppfont)
{
    gs_matrix mat;

    gs_make_scaling(scale, scale, &mat);
    return gs_makefont(pdir, pfont, &mat, ppfont);
}

/* makefont */
int
gs_makefont(gs_font_dir * pdir, const gs_font * pfont,
	    const gs_matrix * pmat, gs_font ** ppfont)
{
    int code;
    gs_font *prev = 0;
    gs_font *pf_out = pdir->scaled_fonts;
    gs_memory_t *mem = pfont->memory;
    gs_matrix newmat;
    bool can_cache;

    if ((code = gs_matrix_multiply(&pfont->FontMatrix, pmat, &newmat)) < 0)
	return code;
    /*
     * Check for the font already being in the scaled font cache.
     * Until version 5.97, we only cached scaled fonts if the base
     * (unscaled) font had a valid UniqueID or XUID; now, we will cache
     * scaled versions of any non-composite font.
     */
#ifdef DEBUG
    if (gs_debug_c('m')) {
	const gs_font_base *const pbfont = (const gs_font_base *)pfont;

	if (pfont->FontType == ft_composite)
	    dlprintf("[m]composite");
	else if (uid_is_UniqueID(&pbfont->UID))
	    dlprintf1("[m]UniqueID=%ld", pbfont->UID.id);
	else if (uid_is_XUID(&pbfont->UID))
	    dlprintf1("[m]XUID(%u)", (uint) (-pbfont->UID.id));
	else
	    dlprintf("[m]no UID");
	dprintf7(", FontType=%d,\n[m]  new FontMatrix=[%g %g %g %g %g %g]\n",
		 pfont->FontType,
		 pmat->xx, pmat->xy, pmat->yx, pmat->yy,
		 pmat->tx, pmat->ty);
    }
#endif
    /*
     * Don't try to cache scaled composite fonts, because of the side
     * effects on FDepVector and descendant fonts that occur in makefont.
     */
    if (pfont->FontType != ft_composite) {
	for (; pf_out != 0; prev = pf_out, pf_out = pf_out->next)
	    if (pf_out->FontType == pfont->FontType &&
		pf_out->base == pfont->base &&
		pf_out->FontMatrix.xx == newmat.xx &&
		pf_out->FontMatrix.xy == newmat.xy &&
		pf_out->FontMatrix.yx == newmat.yx &&
		pf_out->FontMatrix.yy == newmat.yy &&
		pf_out->FontMatrix.tx == newmat.tx &&
		pf_out->FontMatrix.ty == newmat.ty
		) {
		*ppfont = pf_out;
		if_debug1('m', "[m]found font=0x%lx\n", (ulong) pf_out);
		return 0;
	    }
	can_cache = true;
    } else
	can_cache = false;
    pf_out = gs_alloc_struct(mem, gs_font, gs_object_type(mem, pfont),
			     "gs_makefont");
    if (!pf_out)
	return_error(gs_error_VMerror);
    memcpy(pf_out, pfont, gs_object_size(mem, pfont));
    gs_font_notify_init(pf_out);
    pf_out->FontMatrix = newmat;
    pf_out->client_data = 0;
    pf_out->dir = pdir;
    pf_out->base = pfont->base;
    *ppfont = pf_out;
    code = (*pf_out->procs.make_font) (pdir, pfont, pmat, ppfont);
    if (code < 0)
	return code;
    if (can_cache) {
	if (pdir->ssize >= pdir->smax && prev != 0) {
	    /*
	     * We must discard a cached scaled font.
	     * prev points to the last (oldest) font.
	     * (We can't free it, because there might be
	     * other references to it.)
	     */
	    if_debug1('m', "[m]discarding font 0x%lx\n",
		      (ulong) prev);
	    if (prev->prev != 0)
		prev->prev->next = 0;
	    else
		pdir->scaled_fonts = 0;
	    pdir->ssize--;
	    prev->prev = 0;
	    if (prev->FontType != ft_composite) {
		if_debug1('m', "[m]discarding UID 0x%lx\n",
			  (ulong) ((gs_font_base *) prev)->
			  UID.xvalues);
		uid_free(&((gs_font_base *) prev)->UID,
			 prev->memory,
			 "gs_makefont(discarding)");
		uid_set_invalid(&((gs_font_base *) prev)->UID);
	    }
	}
	pdir->ssize++;
	font_link_first(&pdir->scaled_fonts, pf_out);
    } else {			/* Prevent garbage pointers. */
	pf_out->next = pf_out->prev = 0;
    }
    if_debug2('m', "[m]new font=0x%lx can_cache=%s\n",
	      (ulong) * ppfont, (can_cache ? "true" : "false"));
    return 1;
}

/* Set the current font.  This is provided only for the benefit of cshow, */
/* which must reset the current font without disturbing the root font. */
void
gs_set_currentfont(gs_state * pgs, gs_font * pfont)
{
    pgs->font = pfont;
    pgs->char_tm_valid = false;
}

/* setfont */
int
gs_setfont(gs_state * pgs, gs_font * pfont)
{
    pgs->font = pgs->root_font = pfont;
    pgs->char_tm_valid = false;
    return 0;
}

/* currentfont */
gs_font *
gs_currentfont(const gs_state * pgs)
{
    return pgs->font;
}

/* rootfont */
gs_font *
gs_rootfont(const gs_state * pgs)
{
    return pgs->root_font;
}

/* cachestatus */
void
gs_cachestatus(register const gs_font_dir * pdir, register uint pstat[7])
{
    pstat[0] = pdir->ccache.bsize;
    pstat[1] = pdir->ccache.bmax;
    pstat[2] = pdir->fmcache.msize;
    pstat[3] = pdir->fmcache.mmax;
    pstat[4] = pdir->ccache.csize;
    pstat[5] = pdir->ccache.cmax;
    pstat[6] = pdir->ccache.upper;
}

/* setcacheparams */
int
gs_setcachesize(gs_font_dir * pdir, uint size)
{				/* This doesn't delete anything from the cache yet. */
    pdir->ccache.bmax = size;
    return 0;
}
int
gs_setcachelower(gs_font_dir * pdir, uint size)
{
    pdir->ccache.lower = size;
    return 0;
}
int
gs_setcacheupper(gs_font_dir * pdir, uint size)
{
    pdir->ccache.upper = size;
    return 0;
}
int
gs_setaligntopixels(gs_font_dir * pdir, uint v)
{
    pdir->align_to_pixels = v;
    return 0;
}
int
gs_setgridfittt(gs_font_dir * pdir, uint v)
{
    pdir->grid_fit_tt = v;
    return 0;
}

/* currentcacheparams */
uint
gs_currentcachesize(const gs_font_dir * pdir)
{
    return pdir->ccache.bmax;
}
uint
gs_currentcachelower(const gs_font_dir * pdir)
{
    return pdir->ccache.lower;
}
uint
gs_currentcacheupper(const gs_font_dir * pdir)
{
    return pdir->ccache.upper;
}
uint
gs_currentaligntopixels(const gs_font_dir * pdir)
{
    return pdir->align_to_pixels;
}
uint
gs_currentgridfittt(const gs_font_dir * pdir)
{
    return pdir->grid_fit_tt;
}

/* Purge a font from all font- and character-related tables. */
/* This is only used by restore (and, someday, the GC). */
void
gs_purge_font(gs_font * pfont)
{
    gs_font_dir *pdir = pfont->dir;
    gs_font *pf;

    /* Remove the font from its list (orig_fonts or scaled_fonts). */
    gs_font *prev = pfont->prev;
    gs_font *next = pfont->next;

    if (next != 0)
	next->prev = prev, pfont->next = 0;
    if (prev != 0)
	prev->next = next, pfont->prev = 0;
    else if (pdir->orig_fonts == pfont)
	pdir->orig_fonts = next;
    else if (pdir->scaled_fonts == pfont)
	pdir->scaled_fonts = next;
    else {			/* Shouldn't happen! */
	lprintf1("purged font 0x%lx not found\n", (ulong) pfont);
    }

    /* Purge the font from the scaled font cache. */
    for (pf = pdir->scaled_fonts; pf != 0;) {
	if (pf->base == pfont) {
	    gs_purge_font(pf);
	    pf = pdir->scaled_fonts;	/* start over */
	} else
	    pf = pf->next;
    }

    /* Purge the font from the font/matrix pair cache, */
    /* including all cached characters rendered with that font. */
    gs_purge_font_from_char_caches(pdir, pfont);

}

/* Locate a gs_font by gs_id. */
gs_font *
gs_find_font_by_id(gs_font_dir *pdir, gs_id id, gs_matrix *FontMatrix)
 {
    gs_font *pfont = pdir->orig_fonts;

    for(; pfont != NULL; pfont = pfont->next)
	if(pfont->id == id &&
	    !memcmp(&pfont->FontMatrix, FontMatrix, sizeof(pfont->FontMatrix)))
	    return pfont;
    return NULL;
 }

/* ---------------- Default font procedures ---------------- */

/* ------ Font-level procedures ------ */

/* Default (vacuous) definefont handler. */
int
gs_no_define_font(gs_font_dir * pdir, gs_font * pfont)
{
    return 0;
}

/* Default (vacuous) makefont handler. */
int
gs_no_make_font(gs_font_dir * pdir, const gs_font * pfont,
		const gs_matrix * pmat, gs_font ** ppfont)
{
    return 0;
}
/* Makefont handler for base fonts, which must copy the XUID. */
int
gs_base_make_font(gs_font_dir * pdir, const gs_font * pfont,
		  const gs_matrix * pmat, gs_font ** ppfont)
{
    return uid_copy(&((gs_font_base *)*ppfont)->UID, (*ppfont)->memory,
		    "gs_base_make_font(XUID)");
}

/* Default font info procedure */
int
gs_default_font_info(gs_font *font, const gs_point *pscale, int members,
		     gs_font_info_t *info)
{
    int wmode = font->WMode;
    gs_font_base *bfont = (gs_font_base *)font;
    gs_point scale;
    gs_matrix smat;
    const gs_matrix *pmat;

    if (pscale == 0) {
	scale.x = scale.y = 0;
	pmat = 0;
    } else {
	scale = *pscale;
	gs_make_scaling(scale.x, scale.y, &smat);
	pmat = &smat;
    }
    info->members = 0;
    if (members & FONT_INFO_FLAGS)
	info->Flags_returned = 0;
    if (font->FontType == ft_composite)
	return 0;		/* nothing available */
    if (members & FONT_INFO_BBOX) {
	info->BBox.p.x = (int)bfont->FontBBox.p.x;
	info->BBox.p.y = (int)bfont->FontBBox.p.y;
	info->BBox.q.x = (int)bfont->FontBBox.q.x;
	info->BBox.q.y = (int)bfont->FontBBox.q.y;
	info->Flags_returned |= FONT_INFO_BBOX;
    }
    if ((members & FONT_INFO_FLAGS) &&
	(info->Flags_requested & FONT_IS_FIXED_WIDTH)
	) {
	/*
	 * Scan the glyph space to compute the fixed width if any.
	 */
	gs_glyph notdef = gs_no_glyph;
	gs_glyph glyph;
	int fixed_width = 0;
	int index;
	int code = 0; /* Quiet compiler. */

	for (index = 0;
	     fixed_width >= 0 &&
	     (code = font->procs.enumerate_glyph(font, &index, GLYPH_SPACE_NAME, &glyph)) >= 0 &&
		 index != 0;
	     ) {
	    gs_glyph_info_t glyph_info;

	    code = font->procs.glyph_info(font, glyph, pmat,
					  (GLYPH_INFO_WIDTH0 << wmode),
					  &glyph_info);
	    if (code < 0)
		return code;
	    if (notdef == gs_no_glyph && gs_font_glyph_is_notdef(bfont, glyph)) {
		notdef = glyph;
		info->MissingWidth = (int)glyph_info.width[wmode].x;
		info->members |= FONT_INFO_MISSING_WIDTH;
	    }
	    if (glyph_info.width[wmode].y != 0)
		fixed_width = min_int;
	    else if (fixed_width == 0)
		fixed_width = (int)glyph_info.width[wmode].x;
	    else if (glyph_info.width[wmode].x != fixed_width)
		fixed_width = min_int;
	}
	if (code < 0)
	    return code;
	if (fixed_width > 0) {
	    info->Flags |= FONT_IS_FIXED_WIDTH;
	    info->members |= FONT_INFO_AVG_WIDTH | FONT_INFO_MAX_WIDTH |
		FONT_INFO_MISSING_WIDTH;
	    info->AvgWidth = info->MaxWidth = info->MissingWidth = fixed_width;
	}
	info->Flags_returned |= FONT_IS_FIXED_WIDTH;
    } else if (members & FONT_INFO_MISSING_WIDTH) {
	gs_glyph glyph;
	int index;

	for (index = 0;
	     font->procs.enumerate_glyph(font, &index, GLYPH_SPACE_NAME, &glyph) >= 0 &&
		 index != 0;
	     ) {
	    /*
	     * If this is a CIDFont or TrueType font that uses integers as
	     * glyph names, check for glyph 0; otherwise, check for .notdef.
	     */
	    if (!gs_font_glyph_is_notdef(bfont, glyph))
		continue;
	    {
		gs_glyph_info_t glyph_info;
		int code = font->procs.glyph_info(font, glyph, pmat,
						  (GLYPH_INFO_WIDTH0 << wmode),
						  &glyph_info);

		if (code < 0)
		    return code;
		info->MissingWidth = (int)glyph_info.width[wmode].x;
		info->members |= FONT_INFO_MISSING_WIDTH;
		break;
	    }
	}
    }
    return 0;
}

/* Default font similarity testing procedure */
int
gs_default_same_font(const gs_font *font, const gs_font *ofont, int mask)
{
    while (font->base != font)
	font = font->base;
    while (ofont->base != ofont)
	ofont = ofont->base;
    if (ofont == font)
	return mask;
    /* In general, we can't determine similarity. */
    return 0;
}
int
gs_base_same_font(const gs_font *font, const gs_font *ofont, int mask)
{
    int same = gs_default_same_font(font, ofont, mask);

    if (!same) {
	const gs_font_base *const bfont = (const gs_font_base *)font;
	const gs_font_base *const obfont = (const gs_font_base *)ofont;

	if (mask & FONT_SAME_ENCODING) {
	    if (bfont->encoding_index != ENCODING_INDEX_UNKNOWN ||
		obfont->encoding_index != ENCODING_INDEX_UNKNOWN
		) {
		if (bfont->encoding_index == obfont->encoding_index)
		    same |= FONT_SAME_ENCODING;
	    }
	}
    }
    return same;
}

/* ------ Glyph-level procedures ------ */

/*
 * Test whether a glyph is the notdef glyph for a base font.
 * The test is somewhat adhoc: perhaps this should be a virtual procedure.
 */
bool
gs_font_glyph_is_notdef(gs_font_base *bfont, gs_glyph glyph)
{
    gs_const_string gnstr;

    if (glyph == gs_no_glyph)
	return false;
    if (glyph >= gs_min_cid_glyph)
	return (glyph == gs_min_cid_glyph);
    return (bfont->procs.glyph_name((gs_font *)bfont, glyph, &gnstr) >= 0 &&
	    gnstr.size == 7 && !memcmp(gnstr.data, ".notdef", 7));
}

/* Dummy character encoding procedure */
gs_glyph
gs_no_encode_char(gs_font *pfont, gs_char chr, gs_glyph_space_t glyph_space)
{
    return gs_no_glyph;
}

/* Dummy glyph decoding procedure */
gs_char
gs_no_decode_glyph(gs_font *pfont, gs_glyph glyph)
{
    return GS_NO_CHAR;
}

/* Dummy glyph enumeration procedure */
int
gs_no_enumerate_glyph(gs_font *font, int *pindex, gs_glyph_space_t glyph_space,
		      gs_glyph *pglyph)
{
    return_error(gs_error_undefined);
}

/* Default glyph info procedure */
int
gs_default_glyph_info(gs_font *font, gs_glyph glyph, const gs_matrix *pmat,
		      int members, gs_glyph_info_t *info)
{   /* WMode may be inherited from an upper font. */
    gx_path path;
    int returned = 0;
    int code;
    int wmode = ((members & GLYPH_INFO_WIDTH1) != 0);
    double sbw[4] = {0, 0, 0, 0};
    /* Currently glyph_outline retrieves sbw only with type 1,2,9 fonts. */
    bool charstrings_font = (font->FontType == ft_encrypted || 
			     font->FontType == ft_encrypted2 || 
			     font->FontType == ft_CID_encrypted);

    gx_path_init_bbox_accumulator(&path);
    code = gx_path_add_point(&path, fixed_0, fixed_0);
    if (code < 0)
	goto out;
    code = font->procs.glyph_outline(font, wmode, glyph, pmat, &path, sbw);
    if (code < 0)
	goto out;
    if (members & GLYPH_INFO_WIDTHS) {
	int wmode = font->WMode;
	int wmask = GLYPH_INFO_WIDTH0 << wmode;

	if (members & wmask) {
	    gs_fixed_point pt;

	    code = gx_path_current_point(&path, &pt);
	    if (code < 0)
		goto out;
	    info->width[wmode].x = fixed2float(pt.x);
	    info->width[wmode].y = fixed2float(pt.y);
	    returned |= wmask;
	}
    }
    if (members & GLYPH_INFO_BBOX) {
	gs_fixed_rect bbox;

	code = gx_path_bbox(&path, &bbox);
	if (code < 0)
	    goto out;
	info->bbox.p.x = fixed2float(bbox.p.x);
	info->bbox.p.y = fixed2float(bbox.p.y);
	info->bbox.q.x = fixed2float(bbox.q.x);
	info->bbox.q.y = fixed2float(bbox.q.y);
	returned |= GLYPH_INFO_BBOX;
    }
    if (members & (GLYPH_INFO_WIDTH0 << wmode) && charstrings_font) {
	if (pmat == 0) {
	    info->width[wmode].x = sbw[2];
	    info->width[wmode].y = sbw[3];
	} else {
	    code = gs_distance_transform(sbw[2], sbw[3], pmat, &info->width[wmode]);
	    if (code < 0)
		return code;
	}
	returned |= GLYPH_INFO_WIDTH0 << wmode;
    }
    if (members & (GLYPH_INFO_VVECTOR0 << wmode) && charstrings_font) {
	if (pmat == 0) {
	    info->v.x = sbw[0];
	    info->v.y = sbw[1];
	} else {
	    gs_distance_transform(sbw[0], sbw[1], pmat, &info->v);
	    if (code < 0)
		return code;
	}
	returned |= GLYPH_INFO_VVECTOR0 << wmode;
    }
    if (members & GLYPH_INFO_NUM_PIECES) {
	info->num_pieces = 0;
	returned |= GLYPH_INFO_NUM_PIECES;
    }
    returned |= members & GLYPH_INFO_PIECES; /* no pieces stored */
 out:
    info->members = returned;
    return code;
}

/* Dummy glyph outline procedure */
int
gs_no_glyph_outline(gs_font *font, int WMode, gs_glyph glyph, const gs_matrix *pmat,
		    gx_path *ppath, double sbw[4])
{
    return_error(gs_error_undefined);
}

/* Dummy glyph name procedure */
int
gs_no_glyph_name(gs_font *font, gs_glyph glyph, gs_const_string *pstr)
{
    return_error(gs_error_undefined);
}


/* Copyright (C) 1989, 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* gsfont.c */
/* Font operators for Ghostscript library */
#include "gx.h"
#include "memory_.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gzstate.h"			/* must precede gxdevice */
#include "gxdevice.h"			/* must precede gxfont */
#include "gschar.h"
#include "gxfont.h"
#include "gxfcache.h"

/* Imported procedures */
void	gs_purge_font_from_char_caches(P2(gs_font_dir *, const gs_font *));

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

private_st_font_dir();
public_st_gs_font();
public_st_gs_font_base();
private_st_gs_font_ptr();
public_st_gs_font_ptr_element();

/* Font directory GC procedures */
private ENUM_PTRS_BEGIN(font_dir_enum_ptrs) return 0;
#define e1(i,elt) ENUM_PTR(i,gs_font_dir,elt);
	font_dir_do_ptrs(e1)
#undef e1
	case st_font_dir_max_ptrs:
	{	/* Mark the cached character glyphs. */
		/* See gxfcache.h for why we do this here. */
#define dir ((gs_font_dir *)vptr)
		cc_mark_glyph_proc_t mark_glyph = dir->ccache.mark_glyph;
		int chi;
		cached_char *cc;
		for ( chi = dir->ccache.chars_mask; --chi >= 0; )
		  for ( cc = dir->ccache.chars[chi]; cc != 0; cc = cc->next )
		    (*mark_glyph)(cc->code);
	}	return 0;
#undef dir
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(font_dir_reloc_ptrs) ;
	/* Relocate the pointers from cached characters to f/m pairs. */
	/* See gxfcache.h for why we do this here. */
#define dir ((gs_font_dir *)vptr)
	{	int chi;
		for ( chi = dir->ccache.chars_mask; --chi >= 0; )
		{	cached_char *cc = dir->ccache.chars[chi];
			while ( cc != 0 )
			{	cached_fm_pair *pair = cc->head.pair;
				cc->head.pair = (cached_fm_pair *)
				  gs_reloc_struct_ptr(pair - pair->index, gcst) +
				  pair->index;
				cc = cc->next;
			}
		}
	}
#undef dir
	/* We have to relocate the cached characters before we */
	/* relocate dir->ccache.chars! */
#define r1(i,elt) RELOC_PTR(gs_font_dir,elt);
	font_dir_do_ptrs(r1)
#undef r1
RELOC_PTRS_END

/* GC procedures for fonts */
private ENUM_PTRS_BEGIN(font_enum_ptrs) return 0;
	ENUM_PTR(0, gs_font, next);
	ENUM_PTR(1, gs_font, prev);
	ENUM_PTR(2, gs_font, dir);
	ENUM_PTR(3, gs_font, base);
	ENUM_PTR(4, gs_font, client_data);
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(font_reloc_ptrs) ;
	RELOC_PTR(gs_font, next);
	RELOC_PTR(gs_font, prev);
	RELOC_PTR(gs_font, dir);
	RELOC_PTR(gs_font, base);
	RELOC_PTR(gs_font, client_data);
RELOC_PTRS_END

/* Allocate a font directory */
private bool
cc_no_mark_glyph(gs_glyph glyph)
{	return false;
}
gs_font_dir *
gs_font_dir_alloc(gs_memory_t *mem)
{	gs_font_dir *pdir;
#if !arch_ints_are_short
	/* Try allocating a very large cache. */
	/* If this fails, allocate a small one. */
	pdir = gs_font_dir_alloc_limits(mem,
					smax_LARGE, bmax_LARGE, mmax_LARGE,
					cmax_LARGE, blimit_LARGE);
	if ( pdir == 0 )
#endif
	  pdir = gs_font_dir_alloc_limits(mem,
					  smax_SMALL, bmax_SMALL, mmax_SMALL,
					  cmax_SMALL, blimit_SMALL);
	if ( pdir == 0 )
	  return 0;
	pdir->ccache.mark_glyph = cc_no_mark_glyph;
	return pdir;
}
gs_font_dir *
gs_font_dir_alloc_limits(gs_memory_t *mem,
  uint smax, uint bmax, uint mmax, uint cmax, uint upper)
{	register gs_font_dir *pdir =
	  gs_alloc_struct(mem, gs_font_dir, &st_font_dir,
			  "font_dir_alloc(dir)");
	int code;
	if ( pdir == 0 )
	  return 0;
	code = gx_char_cache_alloc(mem, pdir, bmax, mmax, cmax, upper);
	if ( code < 0 )
	{	gs_free_object(mem, pdir, "font_dir_alloc(dir)");
		return 0;
	}
	pdir->orig_fonts = 0;
	pdir->scaled_fonts = 0;
	pdir->ssize = 0;
	pdir->smax = smax;
	return pdir;
}

/* Macro for linking an element at the head of a chain */
#define link_first(first, elt)\
  if ( (elt->next = first) != NULL ) first->prev = elt;\
  elt->prev = 0;\
  first = elt

/* definefont */
/* Use this only for original (unscaled) fonts! */
/* Note that it expects pfont->procs.define_font to be set already. */
int
gs_definefont(gs_font_dir *pdir, gs_font *pfont)
{	int code;
	pfont->dir = pdir;
	pfont->base = pfont;
	code = (*pfont->procs.define_font)(pdir, pfont);
	if ( code < 0 )
		return code;
	link_first(pdir->orig_fonts, pfont);
	if_debug1('m', "[m]defining font 0x%lx\n", (ulong)pfont);
	return 0;
}
/* Default (vacuous) definefont handler. */
int
gs_no_define_font(gs_font_dir *pdir, gs_font *pfont)
{	return 0;
}

/* scalefont */
int
gs_scalefont(gs_font_dir *pdir, const gs_font *pfont, floatp scale,
  gs_font **ppfont)
{	gs_matrix mat;
	gs_make_scaling(scale, scale, &mat);
	return gs_makefont(pdir, pfont, &mat, ppfont);
}

/* makefont */
int
gs_makefont(gs_font_dir *pdir, const gs_font *pfont,
  const gs_matrix *pmat, gs_font **ppfont)
{
#define pbfont ((gs_font_base *)pfont)
	int code;
	gs_font *prev = 0;
	gs_font *pf_out = pdir->scaled_fonts;
	gs_memory_t *mem = pfont->memory;
	gs_matrix newmat;
	bool can_cache;

	if ( (code = gs_matrix_multiply(&pfont->FontMatrix, pmat, &newmat)) < 0 )
	  return code;
	/* Check for the font already being in the scaled font cache. */
	/* Only attempt to share fonts if the current font has */
	/* a valid UniqueID or XUID. */
#ifdef DEBUG
if ( gs_debug_c('m') )
   {	if ( pfont->FontType == ft_composite )
	  dprintf("[m]composite");
	else if ( uid_is_UniqueID(&pbfont->UID) )
	  dprintf1("[m]UniqueID=%ld", pbfont->UID.id);
	else if ( uid_is_XUID(&pbfont->UID) )
	  dprintf1("[m]XUID(%u)", (uint)(-pbfont->UID.id));
	else
	  dprintf("[m]no UID");
	dprintf7(", FontType=%d,\n[m]  new FontMatrix=[%g %g %g %g %g %g]\n",
	  pfont->FontType,
	  pmat->xx, pmat->xy, pmat->yx, pmat->yy,
	  pmat->tx, pmat->ty);
   }
#endif
	/* The UID of a composite font is of no value in caching.... */
	if ( pfont->FontType != ft_composite &&
	     uid_is_valid(&pbfont->UID)
	   )
	{ for ( ; pf_out != 0; prev = pf_out, pf_out = pf_out->next )
	    if ( pf_out->FontType == pfont->FontType &&
		 pf_out->base == pfont->base &&
		 uid_equal(&((gs_font_base *)pf_out)->UID, &pbfont->UID) &&
		 pf_out->FontMatrix.xx == newmat.xx &&
		 pf_out->FontMatrix.xy == newmat.xy &&
		 pf_out->FontMatrix.yx == newmat.yx &&
		 pf_out->FontMatrix.yy == newmat.yy &&
		 pf_out->FontMatrix.tx == newmat.tx &&
		 pf_out->FontMatrix.ty == newmat.ty
	       )
		{	*ppfont = pf_out;
			if_debug1('m', "[m]found font=0x%lx\n", (ulong)pf_out);
			return 0;
		}
	  can_cache = true;
	}
	else
	  can_cache = false;
	pf_out = gs_alloc_struct(mem, gs_font, gs_object_type(mem, pfont),
				 "gs_makefont");
	if ( !pf_out )
		return_error(gs_error_VMerror);
	memcpy(pf_out, pfont, gs_object_size(mem, pfont));
	pf_out->FontMatrix = newmat;
	pf_out->client_data = 0;
	pf_out->dir = pdir;
	pf_out->base = pfont->base;
	*ppfont = pf_out;
	code = (*pf_out->procs.make_font)(pdir, pfont, pmat, ppfont);
	if ( code < 0 )
		return code;
	if ( can_cache )
	{	if ( pdir->ssize == pdir->smax )
		{	/* Must discard a cached scaled font. */
			/* prev points to the last (oldest) font. */
			if_debug1('m', "[m]discarding font 0x%lx\n",
				  (ulong)prev);
			prev->prev->next = 0;
			if ( prev->FontType != ft_composite )
			{	uid_free(&((gs_font_base *)prev)->UID,
					 prev->memory,
					 "gs_makefont(discarding)");
				uid_set_invalid(&((gs_font_base *)prev)->UID);
			}
		}
		else
			pdir->ssize++;
		link_first(pdir->scaled_fonts, pf_out);
	}
	if_debug2('m', "[m]new font=0x%lx can_cache=%s\n",
		  (ulong)*ppfont, (can_cache ? "true" : "false"));
	return 1;
#undef pbfont
}
/* Default (vacuous) makefont handler. */
int
gs_no_make_font(gs_font_dir *pdir, const gs_font *pfont,
  const gs_matrix *pmat, gs_font **ppfont)
{	return 0;
}

/* setfont */
int
gs_setfont(gs_state *pgs, gs_font *pfont)
{	pgs->font = pgs->root_font = pfont;
	pgs->char_tm_valid = false;
	return 0;
}

/* currentfont */
gs_font *
gs_currentfont(const gs_state *pgs)
{	return pgs->font;
}

/* rootfont */
gs_font *
gs_rootfont(const gs_state *pgs)
{	return pgs->root_font;
}

/* cachestatus */
void
gs_cachestatus(register const gs_font_dir *pdir, register uint pstat[7])
{	pstat[0] = pdir->ccache.bsize;
	pstat[1] = pdir->ccache.bmax;
	pstat[2] = pdir->fmcache.msize;
	pstat[3] = pdir->fmcache.mmax;
	pstat[4] = pdir->ccache.csize;
	pstat[5] = pdir->ccache.cmax;
	pstat[6] = pdir->ccache.upper;
}

/* setcachelimit */
int
gs_setcachelimit(gs_font_dir *pdir, uint size)
{	pdir->ccache.upper = size;
	return 0;
}

/* setcacheparams */
int
gs_setcachelower(gs_font_dir *pdir, uint size)
{	pdir->ccache.lower = size;
	return 0;
}
int
gs_setcacheupper(gs_font_dir *pdir, uint size)
{	pdir->ccache.upper = size;
	return 0;
}

/* currentcacheparams */
uint
gs_currentcachelower(const gs_font_dir *pdir)
{	return pdir->ccache.lower;
}
uint
gs_currentcacheupper(const gs_font_dir *pdir)
{	return pdir->ccache.upper;
}

/* Purge a font from all font- and character-related tables. */
/* This is only used by restore (and, someday, the GC). */
void
gs_purge_font(const gs_font *pfont)
{	gs_font_dir *pdir = pfont->dir;
	gs_font *pf;

	/* Remove the font from its list (orig_fonts or scaled_fonts). */
	gs_font *prev = pfont->prev;
	gs_font *next = pfont->next;
	if ( next != 0 )
		next->prev = prev;
	if ( prev != 0 )
		prev->next = next;
	else if ( pdir->orig_fonts == pfont )
		pdir->orig_fonts = next;
	else if ( pdir->scaled_fonts == pfont )
		pdir->scaled_fonts = next;
	else
	{	/* Shouldn't happen! */
		lprintf1("purged font 0x%lx not found\n", (ulong)pfont);
	}
	if ( pfont->base != pfont )
	{	/* I.e., this is a cached scaled font. */
		pdir->ssize--;
	}

	/* Purge the font from the scaled font cache. */
	for ( pf = pdir->scaled_fonts; pf != 0; )
	{	if ( pf->base == pfont )
		{	gs_purge_font(pf);
			pf = pdir->scaled_fonts; /* start over */
		}
		else
			pf = pf->next;
	}

	/* Purge the font from the font/matrix pair cache, */
	/* including all cached characters rendered with that font. */
	gs_purge_font_from_char_caches(pdir, pfont);

}

/* Copyright (C) 1989, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gschar.c */
/* Character writing operators for Ghostscript library */
#include "gx.h"
#include "memory_.h"
#include "string_.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gxfixed.h"			/* ditto */
#include "gxarith.h"
#include "gxmatrix.h"
#include "gzstate.h"
#include "gxcoord.h"
#include "gxdevmem.h"
#include "gxchar.h"
#include "gxfont.h"
#include "gxfont0.h"
#include "gxfcache.h"
#include "gzpath.h"

/* Define whether or not to cache characters rotated by angles other than */
/* multiples of 90 degrees. */
#define CACHE_ROTATED_CHARS 1

/* Define whether or not to oversample characters at small sizes. */
#define OVERSAMPLE 1

/* Structure descriptors */
private_st_gs_show_enum();
#define eptr ((gs_show_enum *)vptr)
private ENUM_PTRS_BEGIN(show_enum_enum_ptrs) {
	if ( index > eptr->fstack.depth + 4 )
	  return 0;
	*pep = eptr->fstack.items[index - 4].font;
	break;
	}
	ENUM_PTR(0, gs_show_enum, pgs);
	ENUM_CONST_STRING_PTR(1, gs_show_enum, str);
	ENUM_PTR(2, gs_show_enum, dev_cache);
	ENUM_PTR(3, gs_show_enum, dev_null);
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(show_enum_reloc_ptrs) {
	int i;
	RELOC_PTR(gs_show_enum, pgs);
	RELOC_CONST_STRING_PTR(gs_show_enum, str);
	RELOC_PTR(gs_show_enum, dev_cache);
	RELOC_PTR(gs_show_enum, dev_null);
	for ( i = 0; i <= eptr->fstack.depth; i++ )
	  RELOC_PTR(gs_show_enum, fstack.items[i].font);
} RELOC_PTRS_END
#undef eptr

/* Forward declarations */
private int continue_kshow(P1(gs_show_enum *));
private int continue_show(P1(gs_show_enum *));
private int continue_show_update(P1(gs_show_enum *));
private int show_setup(P3(gs_show_enum *, gs_state *, const char *));
private void show_set_scale(P1(gs_show_enum *));
private int show_cache_setup(P1(gs_show_enum *));
private int show_state_setup(P1(gs_show_enum *));
private int show_origin_setup(P4(gs_state *, fixed, fixed, int));
private int stringwidth_setup(P3(gs_show_enum *, gs_state *, const char *));

/* Print the ctm if debugging */
#define print_ctm(s,pgs)\
  dprintf7("[p]%sctm=[%g %g %g %g %g %g]\n", s,\
	   pgs->ctm.xx, pgs->ctm.xy, pgs->ctm.yx, pgs->ctm.yy,\
	   pgs->ctm.tx, pgs->ctm.ty)

/* ------ Font procedures ------ */

/* Dummy (ineffective) BuildChar/BuildGlyph procedure */
int
gs_no_build_char(gs_show_enum *penum, gs_state *pgs,
  gs_font *pfont, gs_char chr, gs_glyph glyph)
{	return 1;			/* failure, but not error */
}

/* Dummy character encoding procedure */
gs_glyph
gs_no_encode_char(gs_show_enum *penum,
  gs_font *pfont, gs_char *pchr)
{	return gs_no_glyph;
}

/* ------ String writing operators ------ */

/* Allocate a show enumerator. */
gs_show_enum *
gs_show_enum_alloc(gs_memory_t *mem, gs_state *pgs, client_name_t cname)
{	gs_show_enum *penum =
	  gs_alloc_struct(mem, gs_show_enum, &st_gs_show_enum, cname);
	if ( penum == 0 )
	  return 0;
	/* Initialize pointers for GC */
	penum->pgs = pgs;
	penum->str.data = 0;
	penum->str.size = 0;
	penum->dev_cache = 0;
	penum->dev_null = 0;
	penum->fstack.depth = -1;
	return penum;
}

/* Free the contents of a show enumerator. */
void
gs_show_enum_release(gs_show_enum *penum, gs_memory_t *emem)
{	gs_state *pgs = penum->pgs;
	gs_memory_t *mem = pgs->memory;
	if ( penum->dev_cache != 0 )
	  {	gs_free_object(mem, penum->dev_cache,
			       "gs_show_enum_release(dev_cache)");
		penum->dev_cache = 0;
	  }
	if ( penum->dev_null != 0 )
	  {	gs_free_object(mem, penum->dev_null,
			       "gs_show_enum_release(dev_null)");
		penum->dev_null = 0;
	  }
	if ( emem != 0 )
	  gs_free_object(emem, penum, "gs_show_enum_release(enum)");
}

/* Setup macros for show operators */
#define setup_show_n()\
  penum->str.size = size
#define setup_a()\
  penum->add = true, penum->ax = ax, penum->ay = ay,\
  penum->slow_show = true
#define setup_width()\
  penum->wchr = chr, penum->wcx = cx, penum->wcy = cy,\
  penum->slow_show = true

/* show[_n] */
int
gs_show_n_init(register gs_show_enum *penum,
   gs_state *pgs, const char *str, uint size)
{	setup_show_n();
	penum->slow_show = false;
	return show_setup(penum, pgs, str);
}
int
gs_show_init(gs_show_enum *penum, gs_state *pgs, const char *str)
{	return gs_show_n_init(penum, pgs, str, strlen(str));
}

/* ashow[_n] */
int
gs_ashow_n_init(register gs_show_enum *penum,
  gs_state *pgs, floatp ax, floatp ay, const char *str, uint size)
{	int code;
	setup_show_n();
	code = show_setup(penum, pgs, str);
	setup_a();
	return code;
}
int
gs_ashow_init(gs_show_enum *penum,
  gs_state *pgs, floatp ax, floatp ay, const char *str)
{	return gs_ashow_n_init(penum, pgs, ax, ay, str, strlen(str));
}

/* widthshow[_n] */
int
gs_widthshow_n_init(register gs_show_enum *penum,
  gs_state *pgs, floatp cx, floatp cy, gs_char chr, const char *str, uint size)
{	int code;
	setup_show_n();
	code = show_setup(penum, pgs, str);
	setup_width();
	return code;
}
int
gs_widthshow_init(gs_show_enum *penum,
  gs_state *pgs, floatp cx, floatp cy, gs_char chr, const char *str)
{	return gs_widthshow_n_init(penum, pgs, cx, cy, chr, str, strlen(str));
}

/* awidthshow[_n] */
int
gs_awidthshow_n_init(register gs_show_enum *penum,
  gs_state *pgs, floatp cx, floatp cy, gs_char chr, floatp ax, floatp ay,
  const char *str, uint size)
{	int code;
	setup_show_n();
	code = show_setup(penum, pgs, str);
	setup_a();
	setup_width();
	return code;
}
int
gs_awidthshow_init(gs_show_enum *penum,
  gs_state *pgs, floatp cx, floatp cy, gs_char chr, floatp ax, floatp ay,
  const char *str)
{	return gs_awidthshow_n_init(penum, pgs, cx, cy, chr, ax, ay,
				    str, strlen(str));
}

/* kshow[_n] */
int
gs_kshow_n_init(register gs_show_enum *penum,
  gs_state *pgs, const char *str, uint size)
{	int code;
	if ( pgs->font->FontType == ft_composite)
		return_error(gs_error_invalidfont);
	setup_show_n();
	code = show_setup(penum, pgs, str);
	penum->do_kern = 1;
	penum->slow_show = true;
	return code;
}
int
gs_kshow_init(gs_show_enum *penum, gs_state *pgs, const char *str)
{	return gs_kshow_n_init(penum, pgs, str, strlen(str));
}

/* xyshow[_n] */
int
gs_xyshow_n_init(register gs_show_enum *penum,
   gs_state *pgs, const char *str, uint size)
{	int code;
	setup_show_n();
	code = show_setup(penum, pgs, str);
	penum->do_kern = -1;
	penum->slow_show = true;
	return code;
}
int
gs_xyshow_init(gs_show_enum *penum, gs_state *pgs, const char *str)
{	return gs_xyshow_n_init(penum, pgs, str, strlen(str));
}

/* glyphshow */
private font_proc_encode_char(gs_glyphshow_encode_char);
int
gs_glyphshow_init(gs_show_enum *penum, gs_state *pgs, gs_glyph glyph)
{	int code;
	if ( pgs->font->FontType == ft_composite)
		return_error(gs_error_invalidfont);
	penum->str.size = 1;
	penum->slow_show = false;
	code = show_setup(penum, pgs, "\000");	/* arbitrary char */
	penum->current_glyph = glyph;
	penum->encode_char = gs_glyphshow_encode_char;
	return code;
}
private gs_glyph
gs_glyphshow_encode_char(gs_show_enum *penum, gs_font *pfont, gs_char *pchr)
{	/* We just nil out the character, and return the pre-loaded glyph. */
	*pchr = gs_no_char;
	return penum->current_glyph;
}

/* ------ Related operators ------ */

/* cshow[_n] */
int
gs_cshow_n_init(register gs_show_enum *penum,
   gs_state *pgs, const char *str, uint size)
{	int code;
	setup_show_n();
	code = show_setup(penum, pgs, str);
	penum->do_kern = -1;
	penum->stringwidth_flag = -1;
	penum->slow_show = true;
	return code;
}
int
gs_cshow_init(gs_show_enum *penum, gs_state *pgs, const char *str)
{	return gs_cshow_n_init(penum, pgs, str, strlen(str));
}

/* stringwidth[_n] */
int
gs_stringwidth_n_init(gs_show_enum *penum, gs_state *pgs, const char *str, uint size)
{	setup_show_n();
	return stringwidth_setup(penum, pgs, str);
}
int
gs_stringwidth_init(gs_show_enum *penum, gs_state *pgs, const char *str)
{	return gs_stringwidth_n_init(penum, pgs, str, strlen(str));
}

/* Common code for stringwidth[_n] */
private int
stringwidth_setup(gs_show_enum *penum, gs_state *pgs, const char *str)
{	int code = (penum->slow_show = false, show_setup(penum, pgs, str));
	gs_memory_t *mem = pgs->memory;
	gx_device_null *dev_null;
	if ( code < 0 )
	  return code;
	dev_null = gs_alloc_struct(mem, gx_device_null, &st_device_null,
				   "stringwidth_setup(dev_null)");
	if ( dev_null == 0 )
	  return_error(gs_error_VMerror);
	penum->stringwidth_flag = 1;
	/* Do an extra gsave and suppress output */
	if ( (code = gs_gsave(pgs)) < 0 ) return code;
	penum->level = pgs->level;	/* for level check in show_update */
	/* Set up a null device that forwards xfont requests properly. */
	gs_make_null_device(dev_null, mem);
	dev_null->target = gs_currentdevice_inline(pgs);
	pgs->device = (gx_device *)dev_null;
	penum->dev_null = dev_null;
	/* Establish an arbitrary current point. */
	return gx_path_add_point(pgs->path,
				 pgs->ctm.tx_fixed, pgs->ctm.ty_fixed);
}

/* charpath[_n] */
int
gs_charpath_n_init(gs_show_enum *penum, gs_state *pgs,
  const char *str, uint size, int stroke_path)
{	int code;
	setup_show_n();
	code = show_setup(penum, pgs, str);
	penum->charpath_flag = (stroke_path ? 2 : 1);
	if_debug0('k', "[k]charpath, can_cache=false");
	penum->can_cache = false;
	return code;
}
int
gs_charpath_init(gs_show_enum *penum, gs_state *pgs,
  const char *str, int stroke_path)
{	return gs_charpath_n_init(penum, pgs, str, strlen(str), stroke_path);
}

/* ------ Width/cache operators ------ */

private int set_cache_device(P3(gs_show_enum *, gs_state *,
  const float * /*[4]*/));

/* setcachedevice */
/* The elements of pw are: wx, wy, llx, lly, urx, ury. */
int
gs_setcachedevice(gs_show_enum *penum, gs_state *pgs, const float *pw)
{	int wcode = gs_setcharwidth(penum, pgs, pw[0], pw[1]);	/* default is don't cache */
	int code;
	if ( wcode < 0 )
	  return wcode;
	code = set_cache_device(penum, pgs, pw + 2);
	return (code < 0 ? code : wcode);
}

/* setcachedevice2 */
/* The elements of pw2 are: w0x, w0y, llx, lly, urx, ury, w1x, w1y, vx, vy. */
int
gs_setcachedevice2(gs_show_enum *penum, gs_state *pgs, const float *pw2)
{	int wcode, code;
	if ( gs_rootfont(pgs)->WMode )
	{	float vx = pw2[8], vy = pw2[9];
		gs_fixed_point pvxy, dvxy;
		cached_char *cc;
		if ( (code = gs_point_transform2fixed(&pgs->ctm, -vx, -vy, &pvxy)) < 0 ||
		     (code = gs_distance_transform2fixed(&pgs->ctm, vx, vy, &dvxy)) < 0 ||
		     (code = wcode = gs_setcharwidth(penum, pgs, pw2[6], pw2[7])) < 0
		   )
		  return code;
		/* Adjust the origin by (vx, vy). */
		gx_translate_to_fixed(pgs, pvxy.x, pvxy.y);
		code = set_cache_device(penum, pgs, pw2 + 2);
		if ( code != 1 )
		  return (code < 0 ? code : wcode);
		/* Adjust the character origin too. */
		cc = penum->cc;
		cc->offset.x += dvxy.x;
		cc->offset.y += dvxy.y;
		return wcode;
	}
	else
	{	wcode = gs_setcharwidth(penum, pgs, pw2[0], pw2[1]);
		if ( wcode < 0 )
		  return wcode;
		code = set_cache_device(penum, pgs, pw2 + 2);
		return (code < 0 ? code : wcode);
	}
}

/* Set up the cache device if relevant. */
/* Return 1 if we just set up a cache device. */
/* Used by setcachedevice and setcachedevice2. */
/* The elements of pb are: llx, lly, urx, ury. */
private int
set_cache_device(register gs_show_enum *penum, gs_state *pgs, const float *pb)
{	float llx = pb[0], lly = pb[1], urx = pb[2], ury = pb[3];
	gs_glyph glyph;
	/* See if we want to cache this character. */
	if ( pgs->in_cachedevice )		/* no recursion! */
	  return 0;
	pgs->in_cachedevice = 1;    /* disable color/gray/image operators */
	/* We can only use the cache if we know the glyph. */
	glyph = gs_show_current_glyph(penum);
	if ( glyph == gs_no_glyph )
	  return 0;
	/* We can only use the cache if ctm is unchanged */
	/* (aside from a possible translation). */
	if ( !penum->can_cache || !pgs->char_tm_valid )
	{	if_debug2('k', "[k]no cache: can_cache=%d, char_tm_valid=%d\n",
			  (int)penum->can_cache, (int)pgs->char_tm_valid);
		return 0;
	}
	   {	const gs_font *pfont = pgs->font;
		gs_font_dir *dir = pfont->dir;
		gx_device *dev = gs_currentdevice_inline(pgs);
		int alpha_bits =
		  (*dev_proc(dev, get_alpha_bits))(dev, go_text);
		gs_log2_scale_point log2_scale;
		static const fixed max_cdim[3] = {
#define max_cd(n)\
  (fixed_1 << (arch_sizeof_short * 8 - n)) - (fixed_1 >> n) * 3
			max_cd(0), max_cd(1), max_cd(2)
#undef max_cd
		};
		ushort iwidth, iheight;
		cached_char *cc;
		gs_fixed_rect clip_box;
		int code;
		/* Compute the bounding box of the transformed character. */
		/* Since we accept arbitrary transformations, the extrema */
		/* may occur in any order; however, we can save some work */
		/* by observing that opposite corners before transforming */
		/* are still opposite afterwards. */
		gs_fixed_point cll, clr, cul, cur, cdim;

		if ( (code = gs_distance_transform2fixed(&pgs->ctm, llx, lly, &cll)) < 0 ||
		     (code = gs_distance_transform2fixed(&pgs->ctm, llx, ury, &clr)) < 0 ||
		     (code = gs_distance_transform2fixed(&pgs->ctm, urx, lly, &cul)) < 0 ||
		     (code = gs_distance_transform2fixed(&pgs->ctm, urx, ury, &cur)) < 0
		   )
		  return code;
		{ fixed ctemp;
#define swap(a, b) ctemp = a, a = b, b = ctemp
#define make_min(a, b) if ( (a) > (b) ) swap(a, b)
		  make_min(cll.x, cur.x);
		  make_min(cll.y, cur.y);
		  make_min(clr.x, cul.x);
		  make_min(clr.y, cul.y);
#undef make_min
#undef swap
		}
		/* Now take advantage of symmetry. */
		if ( clr.x < cll.x ) cll.x = clr.x, cur.x = cul.x;
		if ( clr.y < cll.y ) cll.y = clr.y, cur.y = cul.y;
		/* Now cll and cur are the extrema of the box. */
		cdim.x = cur.x - cll.x;
		cdim.y = cur.y - cll.y;
		show_set_scale(penum);
		log2_scale.x = penum->log2_suggested_scale.x;
		log2_scale.y = penum->log2_suggested_scale.y;
#ifdef DEBUG
if ( gs_debug_c('k') )
   {	dprintf6("[k]cbox=[%g %g %g %g] scale=%dx%d\n",
		 fixed2float(cll.x), fixed2float(cll.y),
		 fixed2float(cur.x), fixed2float(cur.y),
		 1 << log2_scale.x, 1 << log2_scale.y);
	print_ctm("  ", pgs);
   }
#endif
		/*
		 * If the device wants anti-aliased text,
		 * increase the sampling scale to ensure that
		 * if we want N bits of alpha, we generate
		 * at least 2^N sampled bits per pixel.
		 */
		if ( alpha_bits > 1 )
		  {	int more_bits =
			  alpha_bits - (log2_scale.x + log2_scale.y);
			if ( more_bits > 0 )
			  {	log2_scale.x += (more_bits + 1) >> 1;
				log2_scale.y += more_bits >> 1;
			  }
		  }
		else
#if OVERSAMPLE
		  if ( pfont->PaintType == 2 )
#endif
		  {	/* Don't oversample artificially stroked fonts. */
			log2_scale.x = log2_scale.y = 0;
		  }
		if ( cdim.x > max_cdim[log2_scale.x] ||
		     cdim.y > max_cdim[log2_scale.y]
		   )
		  return 0;		/* much too big */
		iwidth = ((ushort)fixed2int_var(cdim.x)  + 2) << log2_scale.x;
		iheight = ((ushort)fixed2int_var(cdim.y) + 2) << log2_scale.y;
		if_debug3('k', "[k]iwidth=%u iheight=%u dev_cache %s\n",
			  (uint)iwidth, (uint)iheight,
			  (penum->dev_cache == 0 ? "not set" : "set"));
		if ( penum->dev_cache == 0 )
		  {	code = show_cache_setup(penum);
			if ( code < 0 )
			  return code;
		  }
		cc = gx_alloc_char_bits(dir, penum->dev_cache,
					iwidth, iheight, &log2_scale);
		if ( cc == 0 )
			return 0;	/* too big for cache */
		/* The mins handle transposed coordinate systems.... */
		/* Truncate the offsets to avoid artifacts later. */
		cc->offset.x = fixed_ceiling(-cll.x);
		cc->offset.y = fixed_ceiling(-cll.y);
		if_debug4('k', "[k]width=%u, height=%u, offset=[%g %g]\n",
			  (uint)iwidth, (uint)iheight,
			  fixed2float(cc->offset.x),
			  fixed2float(cc->offset.y));
		if ( (code = gs_gsave(pgs)) < 0 )
		   {	gx_free_cached_char(dir, cc);
			return code;
		   }
		/* Nothing can go wrong now.... */
		penum->cc = cc;
		cc->code = glyph;
		cc->wmode = gs_rootfont(pgs)->WMode;
		cc->depth = alpha_bits;
		cc->wxy = penum->wxy;
		/* Install the device */
		pgs->device = (gx_device *)penum->dev_cache;
		/* Adjust the transformation in the graphics context */
		/* so that the character lines up with the cache. */
		gx_translate_to_fixed(pgs,
				      cc->offset.x << log2_scale.x,
				      cc->offset.y << log2_scale.y);
		if ( (log2_scale.x | log2_scale.y) != 0 )
			gx_scale_char_matrix(pgs, 1 << log2_scale.x,
					     1 << log2_scale.y);
		/* Set the initial matrix for the cache device. */
		penum->dev_cache->initial_matrix = ctm_only(pgs);
		/* Set the oversampling factor. */
		penum->log2_current_scale.x = log2_scale.x;
		penum->log2_current_scale.y = log2_scale.y;
		/* Reset the clipping path to match the metrics. */
		clip_box.p.x = clip_box.p.y = 0;
		clip_box.q.x = int2fixed(iwidth);
		clip_box.q.y = int2fixed(iheight);
		if ( (code = gx_clip_to_rectangle(pgs, &clip_box)) < 0 )
		  return code;
		gx_set_device_color_1(pgs);	/* write 1's */
		pgs->in_cachedevice = 2;	/* we are caching */
	   }
	penum->width_status = sws_cache;
	return 1;
}

/* setcharwidth */
int
gs_setcharwidth(register gs_show_enum *penum, gs_state *pgs,
  floatp wx, floatp wy)
{	int code;
	if ( penum->width_status != sws_none )
	  return_error(gs_error_undefined);
	if ( (code = gs_distance_transform2fixed(&pgs->ctm, wx, wy, &penum->wxy)) < 0 )
	  return code;
	/* Check whether we're setting the scalable width */
	/* for a cached xfont character. */
	if ( penum->cc != 0 )
	  {	penum->cc->wxy = penum->wxy;
		penum->width_status = sws_cache_width_only;
	  }
	else
	  {	penum->width_status = sws_no_cache;
	  }
	return (penum->stringwidth_flag != 0 ? 1 : 0);
}

/* ------ Enumerator ------ */

/* Do the next step of a show (or stringwidth) operation */
int
gs_show_next(gs_show_enum *penum)
{	return (*penum->continue_proc)(penum);
}

/* Continuation procedures */
#define show_fast_move(wxy, pgs)\
  gx_path_add_rel_point_inline(pgs->path, wxy.x, wxy.y)
private int show_update(P1(gs_show_enum *penum));
private int show_move(P1(gs_show_enum *penum));
private int show_proceed(P1(gs_show_enum *penum));
private int show_finish(P1(gs_show_enum *penum));
private int
continue_show_update(register gs_show_enum *penum)
{	int code = show_update(penum);
	if ( code < 0 ) return code;
	code = show_move(penum);
	if ( code != 0 ) return code;
	return show_proceed(penum);
}
private int
continue_show(register gs_show_enum *penum)
{	return show_proceed(penum);
}
/* For kshow, the CTM or font may have changed, so we have to reestablish */
/* the cached values in the enumerator. */
private int
continue_kshow(register gs_show_enum *penum)
{	int code = show_state_setup(penum);
	if ( code < 0 ) return code;
	return show_proceed(penum);
}

/* Update position */
private int
show_update(register gs_show_enum *penum)
{	register gs_state *pgs = penum->pgs;
	cached_char *cc = penum->cc;
	int code;
	/* Update position for last character */
	switch ( penum->width_status )
	   {
	case sws_none:
		/* Adobe interpreters assume a character width of 0, */
		/* even though the documentation says this is an error.... */
		penum->wxy.x = penum->wxy.y = 0;
		break;
	case sws_cache:
		/* Finish installing the cache entry. */
		/* If the BuildChar/BuildGlyph procedure did a save and a */
		/* restore, it already undid the gsave in setcachedevice. */
		/* We have to check for this by comparing levels. */
		switch ( pgs->level - penum->level )
		   {
		default:
			return_error(gs_error_invalidfont);	/* WRONG */
		case 2:
			code = gs_grestore(pgs);
			if ( code < 0 ) return code;
		case 1:
			;
		   }
		gx_add_cached_char(pgs->font->dir, penum->dev_cache,
				   cc, gx_lookup_fm_pair(pgs->font, pgs),
				   &penum->log2_current_scale);
		if ( penum->stringwidth_flag || penum->charpath_flag )
			break;
		/* falls through */
	case sws_cache_width_only:
		/* Copy the bits to the real output device. */
		code = gs_grestore(pgs);
		if ( code < 0 ) return code;
		code = gx_color_load(pgs->dev_color, pgs);
		if ( code < 0 ) return code;
		return gx_image_cached_char(penum, cc);
	case sws_no_cache:
		;
	   }
	if ( penum->charpath_flag )
	{	/* Move back to the character origin, so that */
		/* show_move will get us to the right place. */
		code = gx_path_add_point(pgs->show_gstate->path,
					 penum->origin.x, penum->origin.y);
		if ( code < 0 ) return code;
	}
	return gs_grestore(pgs);
}

/* Move to next character */
private int
show_move(register gs_show_enum *penum)
{	register gs_state *pgs = penum->pgs;
	if ( penum->do_kern < 0 )
	{	/* xyshow or cshow */
		penum->continue_proc = continue_show;
		return gs_show_move;
	}
	if ( penum->add )
		gs_rmoveto(pgs, penum->ax, penum->ay);
	if ( penum->wchr != gs_no_char )
	{	gs_char chr = penum->current_char;
		int fdepth = penum->fstack.depth;
		if ( fdepth > 0 )
		{	/* Add in the shifted font number. */
			uint fidx = penum->fstack.items[fdepth].index;
			switch ( ((gs_font_type0 *)(penum->fstack.items[fdepth - 1].font))->data.FMapType )
			{
			case fmap_1_7:
			case fmap_9_7:
				chr += fidx << 7;
				break;
			default:
				chr += fidx << 8;
			}
		}
		if ( chr == penum->wchr )
			gs_rmoveto(pgs, penum->wcx, penum->wcy);
	}
	/* wxy is in device coordinates */
	   {	int code = show_fast_move(penum->wxy, pgs);
		if ( code < 0 ) return code;
	   }
	/* Check for kerning, but not on the last character. */
	if ( penum->do_kern && penum->index < penum->str.size )
	   {	penum->continue_proc = continue_kshow;
		return gs_show_kern;
	   }
	return 0;
}
/* Process next character */
private int
show_proceed(register gs_show_enum *penum)
{	register gs_state *pgs = penum->pgs;
	gs_font *pfont;
	cached_fm_pair *pair = 0;
	gs_font *rfont = gs_rootfont(pgs);
	int wmode = rfont->WMode;
	font_proc_next_char((*next_char)) = rfont->procs.next_char;
	gs_char chr;
	gs_glyph glyph;
	int code;
	cached_char *cc;
	gx_device *dev = gs_currentdevice_inline(pgs);
	int alpha_bits = (*dev_proc(dev, get_alpha_bits))(dev, go_text);

	code = gx_color_load(pgs->dev_color, pgs);
	if ( code < 0 )
	  return code;
more:	/* Proceed to next character */
	pfont = (penum->fstack.depth < 0 ? pgs->font :
		 penum->fstack.items[penum->fstack.depth].font);
	if ( penum->can_cache )
	{	/* Loop with cache */
		if ( pair == 0 )
			pair = gx_lookup_fm_pair(pfont, pgs);
		for ( ; ; )
		{   switch ( (code = (*next_char)(penum, &chr)) )
		    {
		    default:			/* error */
			return code;
		    case 2:				/* done */
			return show_finish(penum);
		    case 1:				/* font change */
			pfont = penum->fstack.items[penum->fstack.depth].font;
			pgs->char_tm_valid = false;
			show_state_setup(penum);
			pair = gx_lookup_fm_pair(pfont, pgs);
			/* falls through */
		    case 0:				/* plain char */
			glyph = (*penum->encode_char)(penum, pfont, &chr);
			if ( glyph == gs_no_glyph )
			  {	cc = 0;
				goto no_cache;
			  }
			cc = gx_lookup_cached_char(pfont, pair, glyph, wmode,
						   alpha_bits);
			if ( cc == 0 )
			  { /* Character is not in cache. */
			    /* If possible, try for an xfont before */
			    /* rendering from the outline. */
			    if ( pfont->ExactSize == fbit_use_outlines ||
				 pfont->PaintType == 2
			       )
			      goto no_cache;
			    if ( pfont->BitmapWidths )
			      { cc = gx_lookup_xfont_char(pgs, pair, chr,
					glyph, pfont->procs.glyph_name, wmode);
				if ( cc == 0 )
				  goto no_cache;
			      }
			    else
			      { if ( penum->stringwidth_flag != 0 )
				  goto no_cache;
				/* We might have an xfont, but we still */
				/* want the scalable widths. */
				cc = gx_lookup_xfont_char(pgs, pair, chr,
					glyph, pfont->procs.glyph_name, wmode);
				/* Run the scaler up to the point of */
				/* setcharwidth or setcachedevice, */
				/* just as for stringwidth. */
				/* This is the only case in which we can */
				/* to go no_cache with cc != 0. */
				goto no_cache;
			      }
			  }
			/* Character is in cache. */
			if ( !penum->stringwidth_flag )
			  { code = gx_image_cached_char(penum, cc);
			    if ( code < 0 ) return code;
			    else if ( code > 0 )
			      { cc = 0;
				goto no_cache;
			      }
			  }
			if ( penum->slow_show )
			  { penum->current_char = chr; /* for widthshow */
			    /* Split up the assignment so that the */
			    /* Watcom compiler won't reserve esi/edi. */
			    penum->wxy.x = cc->wxy.x;
			    penum->wxy.y = cc->wxy.y;
			    code = show_move(penum);
			  }
			else
				code = show_fast_move(cc->wxy, pgs);
			if ( code )
			  { /* Might be kshow, so store the state. */
			    penum->current_char = chr;
			    penum->current_glyph = glyph;
			    return code;
			  }
		    }
		}
	}
	else
	{	/* Can't use cache */
		switch ( (code = (*next_char)(penum, &chr)) )
		{
		default:
			return code;
		case 2:
			return show_finish(penum);
		case 1:
			pfont = penum->fstack.items[penum->fstack.depth].font;
			show_state_setup(penum);
		case 0:
			;
		}
		glyph = (*penum->encode_char)(penum, pfont, &chr);
		cc = 0;
	}
no_cache:
	/*
	 * We must call the client's rendering code.  Normally,
	 * we only do this if the character is not cached (cc = 0);
	 * however, we also must do this if we have an xfont but
	 * are using scalable widths.  In this case, and only this case,
	 * we get here with cc != 0.
	 */
	penum->current_char = chr;
	penum->current_glyph = glyph;
	if ( (code = gs_gsave(pgs)) < 0 )
		return code;
	/* Set the font to the current descendant font. */
	pgs->font = pfont;
	/* Reset the in_cachedevice flag, so that a recursive show */
	/* will use the cache properly. */
	pgs->in_cachedevice = 0;
	/* Reset the sampling scale. */
	penum->log2_current_scale.x = penum->log2_current_scale.y = 0;
	/* Set the charpath flag in the graphics context if necessary, */
	/* so that fill and stroke will add to the path */
	/* rather than having their usual effect. */
	pgs->in_charpath = penum->charpath_flag;
	pgs->stroke_adjust = false;		/* per specification */
	   {	gs_fixed_point cpt;
		gx_path *ppath = pgs->path;
		if ( (code = gx_path_current_point_inline(ppath, &cpt)) < 0 )
			return code;
		cpt.x -= pgs->ctm.tx_fixed;
		cpt.y -= pgs->ctm.ty_fixed;
		gs_settocharmatrix(pgs);
		penum->origin.x = cpt.x += pgs->ctm.tx_fixed;
		penum->origin.y = cpt.y += pgs->ctm.ty_fixed;
		gs_newpath(pgs);
		code = show_origin_setup(pgs, cpt.x, cpt.y,
					 penum->charpath_flag);
		if ( code < 0 ) return code;
	   }
	penum->width_status = sws_none;
	penum->continue_proc = continue_show_update;
	/* Try using the build procedure in the font. */
	/* < 0 means error, 0 means success, 1 means failure. */
	code = (*pfont->procs.build_char)(penum, pgs, pfont, chr, glyph);
	if ( code < 0 ) return_error(code);
	if ( code == 0 )
	   {	code = show_update(penum);
		if ( code < 0 ) return code;
		code = show_move(penum);
		if ( code ) return code;
		goto more;
	   }
	penum->cc = cc;
	return gs_show_render;
}

/* Finish show or stringwidth */
private int
show_finish(gs_show_enum *penum)
{	gs_state *pgs = penum->pgs;
	int code;
	gs_show_enum_release(penum, NULL);
	if ( penum->stringwidth_flag <= 0 )	/* could be cshow */
		return 0;
	/* Save the accumulated width before returning, */
	/* and undo the extra gsave. */
	code = gs_currentpoint(pgs, &penum->width);
	if ( code < 0 )
	  return code;
	return gs_grestore(pgs);
}

/* Return the current character for rendering. */
gs_char
gs_show_current_char(const gs_show_enum *penum)
{	return penum->current_char;
}

/* Return the current glyph for rendering. */
gs_glyph
gs_show_current_glyph(const gs_show_enum *penum)
{	return penum->current_glyph;
}

/* Return the width of the just-enumerated character (for cshow). */
int
gs_show_current_width(const gs_show_enum *penum, gs_point *ppt)
{	return gs_idtransform(penum->pgs, fixed2float(penum->wxy.x),
			      fixed2float(penum->wxy.y), ppt);
}

/* Return the just-displayed character for kerning. */
gs_char
gs_kshow_previous_char(const gs_show_enum *penum)
{	return penum->current_char;
}

/* Return the about-to-be-displayed character for kerning. */
gs_char
gs_kshow_next_char(const gs_show_enum *penum)
{	return penum->str.data[penum->index];
}

/* ------ Miscellaneous accessors ------ */

/* Return the accumulated width for stringwidth. */
void
gs_show_width(const gs_show_enum *penum, gs_point *ppt)
{	*ppt = penum->width;
}

/* Return the charpath flag. */
int
gs_show_in_charpath(const gs_show_enum *penum)
{	return penum->charpath_flag;
}

/* Return true if we only need the width from the rasterizer */
/* and can short-circuit the full rendering of the character, */
/* false if we need the actual character bits. */
/* This is only meaningful just before calling gs_setcharwidth or */
/* gs_setcachedevice[2]. */
/* Note that we can't do this if the procedure has done any extra [g]saves. */
bool
gs_show_width_only(const gs_show_enum *penum)
{	/* penum->cc will be non-zero iff we are calculating */
	/* the scalable width for an xfont character. */
	return ((penum->stringwidth_flag != 0 || penum->cc != 0) &&
		penum->pgs->level == penum->level + 1);
}

/* ------ Internal routines ------ */

/* Initialize a show enumerator. */
/* Note that this does not set str.size. */
private int
show_setup(register gs_show_enum *penum, gs_state *pgs, const char *str)
{	int code;
	gs_font *pfont;
	gx_set_dev_color(pgs);
	pfont = pgs->font;
	penum->pgs = pgs;
	penum->level = pgs->level;
	penum->str.data = (const byte *)str;	/* avoid signed chars */
	penum->wchr = gs_no_char;
	penum->add = false;
	penum->do_kern = 0;
	penum->charpath_flag = 0;
	penum->stringwidth_flag = 0;
	penum->dev_cache = 0;
	penum->dev_null = 0;
	penum->index = 0;
	penum->continue_proc = continue_show;
	code = (*pfont->procs.init_fstack)(penum, pfont);
	if ( code < 0 )
	  return code;
	penum->can_cache = true;	/* show_state_setup may reset */
	code = show_state_setup(penum);
	if ( code < 0 )
	  return code;
	pgs->show_gstate = pgs;
	return 0;
}

/* Initialize the gstate-derived parts of a show enumerator. */
/* We do this both when starting the show operation, */
/* and when returning from the kshow callout. */
private int
show_state_setup(gs_show_enum *penum)
{	gs_state *pgs = penum->pgs;
	const gs_font *pfont;
	if ( penum->fstack.depth <= 0 )
	{	pfont = pgs->font;
		gs_currentcharmatrix(pgs, NULL, 1); /* make char_tm valid */
	}
	else
	{	/* We have to concatenate the parent's FontMatrix as well. */
		gs_matrix mat;
		const gx_font_stack_item *pfsi =
			&penum->fstack.items[penum->fstack.depth];
		pfont = pfsi->font;
		gs_matrix_multiply(&pfsi[-1].font->FontMatrix,
				   &pfont->FontMatrix, &mat);
		gs_setcharmatrix(pgs, &mat);
	}
	if ( (penum->can_cache
#if !CACHE_ROTATED_CHARS
	      &= /* no skewing or non-rectangular rotation */
	      (is_fzero2(pgs->char_tm.xy, pgs->char_tm.yx) ||
	       is_fzero2(pgs->char_tm.xx, pgs->char_tm.yy))
#endif
	   ) )
	   {	gs_fixed_rect cbox;
		gx_cpath_inner_box(pgs->clip_path, &cbox);
		/* Since characters occupy an integral number of pixels, */
		/* we can (and should) round the inner clipping box */
		/* outward rather than inward. */
		penum->ibox.p.x = fixed2int_var(cbox.p.x);
		penum->ibox.p.y = fixed2int_var(cbox.p.y);
		penum->ibox.q.x = fixed2int_var_ceiling(cbox.q.x);
		penum->ibox.q.y = fixed2int_var_ceiling(cbox.q.y);
		gx_cpath_outer_box(pgs->clip_path, &cbox);
		penum->obox.p.x = fixed2int_var(cbox.p.x);
		penum->obox.p.y = fixed2int_var(cbox.p.y);
		penum->obox.q.x = fixed2int_var_ceiling(cbox.q.x);
		penum->obox.q.y = fixed2int_var_ceiling(cbox.q.y);
		penum->ftx = (int)fixed2long(pgs->char_tm.tx_fixed - pgs->ctm.tx_fixed);
		penum->fty = (int)fixed2long(pgs->char_tm.ty_fixed - pgs->ctm.ty_fixed);
	   }
	penum->encode_char = pfont->procs.encode_char;
	return 0;
}

/* Set the suggested oversampling scale for character rendering. */
private void
show_set_scale(gs_show_enum *penum)
{	/* Decide whether to oversample. */
	/* We have to decide this each time setcachedevice is called. */
	const gs_state *pgs = penum->pgs;
	if ( !penum->charpath_flag &&
	     !penum->stringwidth_flag &&
	     gx_path_is_void_inline(pgs->path) &&
		/* Oversampling rotated characters doesn't work well. */
	     (is_fzero2(pgs->char_tm.xy, pgs->char_tm.yx) ||
	      is_fzero2(pgs->char_tm.xx, pgs->char_tm.yy))
	   )
	  {	const gs_font_base *pfont = (gs_font_base *)pgs->font;
		gs_fixed_point extent;
		int code = gs_distance_transform2fixed(&pgs->char_tm,
			pfont->FontBBox.q.x - pfont->FontBBox.p.x,
			pfont->FontBBox.q.y - pfont->FontBBox.p.y,
			&extent);
		if ( code >= 0 )
		  {	penum->log2_suggested_scale.x =
			  (extent.x != 0 && any_abs(extent.x) < int2fixed(16) ? 2 : 0);
			penum->log2_suggested_scale.y =
			  (extent.y != 0 && any_abs(extent.y) < int2fixed(16) ? 2 : 0);
			return;
		  }
	  }
	/* By default, don't scale. */
	penum->log2_suggested_scale.x =
	  penum->log2_suggested_scale.y = 0;
}

/* Set up the cache device and related information. */
private int
show_cache_setup(gs_show_enum *penum)
{	gs_state *pgs = penum->pgs;
	gs_memory_t *mem = pgs->memory;
	gx_device_memory *dev =
	  gs_alloc_struct(mem, gx_device_memory, &st_device_memory,
			  "show_cache_setup");
	if ( dev == 0 )
	  return_error(gs_error_VMerror);
	gs_make_mem_mono_device(dev, mem, gs_currentdevice_inline(pgs));
	dev->cached.white = 1;
	dev->cached.black = 1;
	penum->dev_cache = dev;
	return 0;
}

/* Set the character origin as the origin of the coordinate system. */
/* Used before rendering characters, and for moving the origin */
/* in setcachedevice2 when WMode=1. */
private int
show_origin_setup(gs_state *pgs, fixed cpt_x, fixed cpt_y, int charpath_flag)
{	if ( !charpath_flag )
	  { /* Round the translation in the graphics state. */
	    /* This helps prevent rounding artifacts later. */
	    cpt_x = fixed_rounded(cpt_x);
	    cpt_y = fixed_rounded(cpt_y);
	  }
	gx_translate_to_fixed(pgs, cpt_x, cpt_y);
	return gx_path_add_point(pgs->path, pgs->ctm.tx_fixed,
				 pgs->ctm.ty_fixed);
}

/* Default fstack initialization procedure. */
int
gs_default_init_fstack(gs_show_enum *penum, gs_font *pfont)
{	penum->fstack.depth = -1;
	return 0;
}

/* Default next-character procedure. */
int
gs_default_next_char(gs_show_enum *penum, gs_char *pchr)
{	if ( penum->index == penum->str.size )
		return 2;
	else
	{	*pchr = penum->str.data[penum->index++];
		return 0;
	}
}

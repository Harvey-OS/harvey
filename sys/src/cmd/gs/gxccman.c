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

/* gxccman.c */
/* Character cache management routines for Ghostscript library */
#include "gx.h"
#include "memory_.h"
#include "gpcheck.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gsbitops.h"
#include "gsutil.h"			/* for gs_next_ids */
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gzstate.h"
#include "gzpath.h"
#include "gxdevmem.h"
#include "gxchar.h"
#include "gxfont.h"
#include "gxfcache.h"
#include "gxxfont.h"

/* Define the descriptors for the cache structures. */
private_st_cached_fm_pair();
private_st_cached_fm_pair_elt();
/*private_st_cached_char();*/		/* unused */
private_st_cached_char_ptr();
private_st_cached_char_ptr_elt();
/* GC procedures */
#define cptr ((cached_char *)vptr)
private ENUM_PTRS_BEGIN(cached_char_enum_ptrs) return 0;
	case 0:
	  *pep = (cc_is_free(cptr) ? NULL :
		  (void *)(cptr->head.pair - cptr->head.pair->index));
	  break;
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(cached_char_reloc_ptrs) {
	uint offset;
	if ( cc_is_free(cptr) )
	  return;
	offset = cptr->head.pair->index * sizeof(cached_fm_pair);
	RELOC_OFFSET_PTR(cached_char, head.pair, offset);
} RELOC_PTRS_END
#undef cptr
private ENUM_PTRS_BEGIN_PROC(cc_ptr_enum_ptrs) {
	cached_char *cptr = *(cached_char **)vptr;
	if ( cptr == 0 )
	  return 0;
	return cached_char_enum_ptrs(cptr, cptr->head.size, index, pep);
} ENUM_PTRS_END_PROC
private RELOC_PTRS_BEGIN(cc_ptr_reloc_ptrs) {
	cached_char *cptr = *(cached_char **)vptr;
	if ( cptr != 0 )
	  cached_char_reloc_ptrs(cptr, cptr->head.size, gcst);
} RELOC_PTRS_END

/* Forward references */
private gx_xfont *lookup_xfont_by_name(P6(gx_device *, gx_xfont_procs *, gs_font_name *, int, const cached_fm_pair *, const gs_matrix *));
private cached_char *alloc_char_in_chunk(P4(gs_font_dir *, ulong, char_cache_chunk *, uint));
private void shorten_cached_char(P3(gs_font_dir *, cached_char *, uint));
private void image_bbox(P5(const byte *, uint, uint, int, gs_int_rect *));

/* ====== Initialization ====== */

/* Allocate and initialize the character cache elements of a font directory. */
int
gx_char_cache_alloc(gs_memory_t *mem, register gs_font_dir *pdir,
  uint bmax, uint mmax, uint cmax, uint upper)
{	uint chsize = (cmax / 5) | 31;		/* a guess */
	cached_fm_pair *mdata;
	cached_char **chars;
	/* Round up chsize to a power of 2. */
	while ( chsize & (chsize + 1) )
	  chsize |= chsize >> 1;
	chsize++;
	mdata = gs_alloc_struct_array(mem, mmax, cached_fm_pair,
				      &st_cached_fm_pair_element,
				      "font_dir_alloc(mdata)");
	chars = gs_alloc_struct_array(mem, chsize, cached_char *,
				      &st_cached_char_ptr_element,
				      "font_dir_alloc(chars)");
	if ( mdata == 0 || chars == 0 )
	{	gs_free_object(mem, chars, "font_dir_alloc(chars)");
		gs_free_object(mem, mdata, "font_dir_alloc(mdata)");
		return_error(gs_error_VMerror);
	}
	pdir->fmcache.mmax = mmax;
	pdir->fmcache.mdata = mdata;
	pdir->ccache.memory = mem;
	pdir->ccache.bmax = bmax;
	pdir->ccache.cmax = cmax;
	pdir->ccache.lower = upper / 10;
	pdir->ccache.upper = upper;
	pdir->ccache.chars = chars;
	pdir->ccache.chars_mask = chsize - 1;
	gx_char_cache_init(pdir);
	return 0;
}

/* Initialize the character cache. */
void
gx_char_cache_init(register gs_font_dir *dir)
{	int i;
	cached_fm_pair *pair;
	dir->fmcache.msize = 0;
	dir->fmcache.mnext = 0;
	dir->ccache.bsize = 0;
	dir->ccache.csize = 0;
	dir->ccache.bspace = 0;
	dir->ccache.initial_chunk =
		(char_cache_chunk *)gs_malloc(1, sizeof(char_cache_chunk),
 					      "initial_chunk");
	dir->ccache.chunks = dir->ccache.initial_chunk->next =
		dir->ccache.initial_chunk;
	dir->ccache.initial_chunk->size = 0;
	dir->ccache.cnext = 0;
	memset((char *)dir->ccache.chars, 0,
	       (dir->ccache.chars_mask + 1) * sizeof(cached_char *));
	for ( i = 0, pair = dir->fmcache.mdata;
	      i < dir->fmcache.mmax; i++, pair++
	    )
	{	pair->index = i;
		fm_pair_init(pair);
	}
}

/* ====== Purging ====== */

/* Purge from the character cache all entries selected by */
/* a client-supplied procedure. */
void
gx_purge_selected_cached_chars(gs_font_dir *dir,
  bool (*proc)(P2(cached_char *, void *)), void *proc_data)
{	int chi;
	for ( chi = dir->ccache.chars_mask; chi >= 0; )
	{	cached_char **pcc = dir->ccache.chars + chi--;
		while ( *pcc != 0 )
		{	cached_char *cc = *pcc;
			if ( (*proc)(cc, proc_data) )
			{	cached_char *ccnext = cc->next;
				gx_free_cached_char(dir, cc);
				*pcc = ccnext;
			}
			else
				pcc = &cc->next;
		}
	}
}

/* ====== Font-level routines ====== */

/* Add a font/matrix pair to the cache. */
/* (This is only exported for gxccache.c.) */
cached_fm_pair *
gx_add_fm_pair(register gs_font_dir *dir, gs_font *font, const gs_uid *puid,
  const gs_state *pgs)
{	register cached_fm_pair *pair =
		dir->fmcache.mdata + dir->fmcache.mnext;
	cached_fm_pair *mend =
		dir->fmcache.mdata + dir->fmcache.mmax;
	if ( dir->fmcache.msize == dir->fmcache.mmax ) /* cache is full */
	{	/* Prefer an entry with num_chars == 0, if any. */
		int count;
		for ( count = dir->fmcache.mmax;
		      --count >= 0 && pair->num_chars != 0;
		    )
			if ( ++pair == mend ) pair = dir->fmcache.mdata;
		gs_purge_fm_pair(dir, pair, 0);
	}
	else
	{	/* Look for an empty entry.  (We know there is one.) */
		while ( !fm_pair_is_free(pair) )
			if ( ++pair == mend ) pair = dir->fmcache.mdata;
	}
	dir->fmcache.msize++;
	dir->fmcache.mnext = pair + 1 - dir->fmcache.mdata;
	if ( dir->fmcache.mnext == dir->fmcache.mmax )
		dir->fmcache.mnext = 0;
	pair->font = font;
	pair->UID = *puid;
	pair->mxx = pgs->char_tm.xx, pair->mxy = pgs->char_tm.xy;
	pair->myx = pgs->char_tm.yx, pair->myy = pgs->char_tm.yy;
	pair->num_chars = 0;
	pair->xfont_tried = false;
	pair->xfont = 0;
	if_debug8('k', "[k]adding pair 0x%lx: font=0x%lx [%g %g %g %g] UID %ld, 0x%lx\n",
		  (ulong)pair, (ulong)font,
		  pair->mxx, pair->mxy, pair->myx, pair->myy,
		  (long)pair->UID.id, (ulong)pair->UID.xvalues);
	return pair;
}

/* Look up the xfont for a font/matrix pair. */
/* (This is only exported for gxccache.c.) */
void
gx_lookup_xfont(const gs_state *pgs, cached_fm_pair *pair, int encoding_index)
{	gx_device *dev = gs_currentdevice(pgs);
	gx_device *fdev = (*dev_proc(dev, get_xfont_device))(dev);
	gs_font *font = pair->font;
	gx_xfont_procs *procs = (*dev_proc(fdev, get_xfont_procs))(fdev);
	gx_xfont *xf = 0;
	/* We mustn't attempt to use xfonts for stroked characters, */
	/* because such characters go outside their bounding box. */
	if ( procs != 0 && font->PaintType == 0 )
	{	gs_matrix mat;
		mat.xx = pair->mxx, mat.xy = pair->mxy;
		mat.yx = pair->myx, mat.yy = pair->myy;
		mat.tx = 0, mat.ty = 0;
		/* xfonts can outlive their invocations, */
		/* but restore purges them properly. */
		pair->memory = pgs->memory;
		if ( font->key_name.size != 0 )
			xf = lookup_xfont_by_name(fdev, procs,
				&font->key_name, encoding_index,
				pair, &mat);
#define font_name_eq(pfn1,pfn2)\
  ((pfn1)->size == (pfn2)->size &&\
   !memcmp((char *)(pfn1)->chars, (char *)(pfn2)->chars, (pfn1)->size))
		if ( xf == 0 && font->font_name.size != 0 &&
			     /* Avoid redundant lookup */
		     !font_name_eq(&font->font_name, &font->key_name)
		   )
			xf = lookup_xfont_by_name(fdev, procs,
				&font->font_name, encoding_index,
				pair, &mat);
		if ( xf == 0 & font->FontType != ft_composite &&
		     uid_is_valid(&((gs_font_base *)font)->UID)
		   )
		{	/* Look for an original font with the same UID. */
			gs_font_dir *pdir = font->dir;
			gs_font *pfont;
			for ( pfont = pdir->orig_fonts; pfont != 0;
			      pfont = pfont->next
			    )
			{	if ( pfont->FontType != ft_composite &&
				     uid_equal(&((gs_font_base *)pfont)->UID,
					       &((gs_font_base *)font)->UID) &&
				     pfont->key_name.size != 0 &&
				     !font_name_eq(&font->key_name,
					           &pfont->key_name)
				   )
				{	xf = lookup_xfont_by_name(fdev, procs,
						&pfont->key_name,
						encoding_index, pair, &mat);
					if ( xf != 0 )
						break;
				}
			}
		}
	}
	pair->xfont = xf;
}

/* ------ Internal routines ------ */

/* Purge from the caches all references to a given font/matrix pair, */
/* or just characters that depend on its xfont. */
#define cpair ((cached_fm_pair *)vpair)
private bool
purge_fm_pair_char(cached_char *cc, void *vpair)
{	return cc->head.pair == cpair;
}
private bool
purge_fm_pair_char_xfont(cached_char *cc, void *vpair)
{	return cc->head.pair == cpair && cpair->xfont == 0 && !cc_has_bits(cc);
}
#undef cpair
void
gs_purge_fm_pair(gs_font_dir *dir, cached_fm_pair *pair, int xfont_only)
{	if_debug2('k', "[k]purging pair 0x%lx%s\n",
		  (ulong)pair, (xfont_only ? " (xfont only)" : ""));
	if ( pair->xfont != 0 )
	{	(*pair->xfont->common.procs->release)(pair->xfont,
			pair->memory);
		pair->xfont_tried = false;
		pair->xfont = 0;
	}
	gx_purge_selected_cached_chars(dir,
				       (xfont_only ? purge_fm_pair_char_xfont :
					purge_fm_pair_char),
				       pair);
	if ( !xfont_only )
	{
#ifdef DEBUG
		if ( pair->num_chars != 0 )
		{	lprintf1("Error in gs_purge_fm_pair: num_chars =%d\n",
				 pair->num_chars);
		}
#endif
		fm_pair_set_free(pair);
		dir->fmcache.msize--;
	}
}

/* Look up an xfont by name. */
/* The caller must already have done get_xfont_device to get the proper */
/* device to pass as the first argument to lookup_font. */
private gx_xfont *
lookup_xfont_by_name(gx_device *fdev, gx_xfont_procs *procs,
  gs_font_name *pfstr, int encoding_index, const cached_fm_pair *pair,
  const gs_matrix *pmat)
{	gx_xfont *xf;
	if_debug5('k', "[k]lookup xfont %s [%g %g %g %g]\n",
		  pfstr->chars, pmat->xx, pmat->xy, pmat->yx, pmat->yy);
	xf = (*procs->lookup_font)(fdev,
		&pfstr->chars[0], pfstr->size,
		encoding_index, &pair->UID,
		pmat, pair->memory);
	if_debug1('k', "[k]... xfont=0x%lx\n", (ulong)xf);
	return xf;
}

/* ====== Character-level routines ====== */

/* Allocate storage for caching a rendered character. */
/* If dev != NULL set up the memory device; */
/* if dev == NULL, this is an xfont-only entry. */
/* Return the cached_char if OK, 0 if too big. */
/* If the character is being oversampled, make the size decision */
/* on the basis of the final (scaled-down) size. */
cached_char *
gx_alloc_char_bits(gs_font_dir *dir, gx_device_memory *dev,
  ushort iwidth, ushort iheight, const gs_log2_scale_point *pscale)
{	int log2_xscale = pscale->x;
	int log2_yscale = pscale->y;
	ulong isize, icdsize;
	uint iraster;
	char_cache_chunk *cck;
	cached_char *cc;
	gx_device_memory mdev;
	gx_device_memory *pdev = dev;
	if ( dev == NULL )
	{	gs_make_mem_mono_device(&mdev, 0, 0);
		pdev = &mdev;
	}
	/* Compute the scaled-down bitmap size. */
	pdev->width = iwidth >> log2_xscale;
	pdev->height = iheight;
	iraster = gdev_mem_raster(pdev);
	if ( iraster != 0 && iheight >> log2_yscale > dir->ccache.upper / iraster )
	{	if_debug5('k', "[k]no cache bits: scale=%dx%d, raster/scale=%u, height/scale=%u, upper=%u\n",
			  1 << log2_xscale, 1 << log2_yscale,
			  iraster, iheight, dir->ccache.upper);
		return 0;		/* too big */
	}
	/* Recompute the actual bitmap size. */
	pdev->width = iwidth;
	iraster = gdev_mem_raster(pdev);
	isize = gdev_mem_bitmap_size(pdev);
	icdsize = isize + sizeof_cached_char;
	/* Try allocating at the current position first. */
	cck = dir->ccache.chunks;
	cc = alloc_char_in_chunk(dir, icdsize, cck, dir->ccache.cnext);
	if ( cc == 0 )
	{	if ( dir->ccache.bspace < dir->ccache.bmax )
		{	/* Allocate another chunk. */
			char_cache_chunk *cck_prev = cck;
			uint cksize = dir->ccache.bmax / 5 + 1;
			uint tsize = dir->ccache.bmax - dir->ccache.bspace;
			byte *cdata;
			if ( cksize > tsize )
				cksize = tsize;
			if ( icdsize + sizeof(cached_char_head) > cksize )
			{	if_debug2('k', "[k]no cache bits: cdsize+head=%lu, cksize=%u\n",
					  icdsize + sizeof(cached_char_head),
					  cksize);
				return 0;		/* wouldn't fit */
			}
			cck = (char_cache_chunk *)gs_malloc(1, sizeof(*cck),
							"char cache chunk");
			if ( cck == 0 )
				return 0;
			cdata = (byte *)gs_malloc(cksize, 1,
						  "char cache chunk");
			if ( cdata == 0 )
			{	gs_free((char *)cck, 1, sizeof(*cck),
					"char cache chunk");
				return 0;
			}
			cck->data = cdata;
			cck->size = cksize;
			cck->next = cck_prev->next;
			cck_prev->next = cck;
			dir->ccache.bspace += cksize;
			((cached_char_head *)cdata)->size = cksize;
			((cached_char_head *)cdata)->pair = 0;
			cc = alloc_char_in_chunk(dir, icdsize, cck, 0);
		}
		else
		{	/* Cycle through chunks. */
			char_cache_chunk *cck_init = cck;
			while ( (cck = cck->next) != cck_init )
			{	cc = alloc_char_in_chunk(dir, icdsize, cck, 0);
				if ( cc != 0 ) break;
			}
			if ( cc == 0 )
				cc = alloc_char_in_chunk(dir, icdsize, cck, 0);
		}
		if ( cc == 0 )
			return 0;
		dir->ccache.chunks = cck;	/* update roving pointer */
	}
	if_debug4('k', "[k]adding char 0x%lx:%u(%u,%u)\n",
		  (ulong)cc, (uint)icdsize, iwidth, iheight);
	cc->xglyph = gx_no_xglyph;
	cc->width = iwidth;
	cc->height = iheight;
	cc->raster = iraster;
	cc->head.pair = 0;	/* not linked in yet */
	cc->id = gx_no_bitmap_id;
	if ( dev != NULL )
		gx_open_cache_device(dev, cc);
	return cc;
}

/* Open the cache device. */
void
gx_open_cache_device(gx_device_memory *dev, cached_char *cc)
{	byte *bits = cc_bits(cc);
	dev->width = cc->width;
	dev->height = cc->height;
	memset((char *)bits, 0, (uint)gdev_mem_bitmap_size(dev));
	dev->base = bits;
	(*dev_proc(dev, open_device))((gx_device *)dev);	/* initialize */
}

/* Remove a character from the cache. */
void
gx_free_cached_char(gs_font_dir *dir, cached_char *cc)
{	char_cache_chunk *cck = cc->chunk;
	dir->ccache.chunks = cck;
	dir->ccache.cnext = (byte *)cc - cck->data;
	dir->ccache.csize--;
	dir->ccache.bsize -= cc->head.size;
	if ( cc->head.pair != 0 )
	   {	/* might be allocated but not added to table yet */
		cc->head.pair->num_chars--;
	   }
	if_debug2('k', "[k]freeing char 0x%lx, pair=0x%lx\n",
		  (ulong)cc, (ulong)cc->head.pair);
	cc_set_free(cc);
}

/* Add a character to the cache */
void
gx_add_cached_char(gs_font_dir *dir, gx_device_memory *dev,
  cached_char *cc, cached_fm_pair *pair, const gs_log2_scale_point *pscale)
{	if_debug5('k', "[k]chaining char 0x%lx: pair=0x%lx, glyph=0x%lx, wmode=%d, depth=%d\n",
		  (ulong)cc, (ulong)pair, (ulong)cc->code,
		  cc->wmode, cc->depth);
	if ( dev != NULL )
		gx_add_char_bits(dir, dev, cc, pscale);
	/* Add the new character at the tail of its chain. */
	{	register cached_char **head =
		  chars_head(dir, cc->code, pair);

		while ( *head != 0 ) head = &(*head)->next;
		*head = cc;
		cc->next = 0;
		cc->head.pair = pair;
		pair->num_chars++;
	}
}

/* Adjust the bits of a newly-rendered character, by unscaling */
/* and compressing or converting to alpha values if necessary. */
void
gx_add_char_bits(gs_font_dir *dir, gx_device_memory *dev,
  cached_char *cc, const gs_log2_scale_point *plog2_scale)
{	uint raster = cc->raster;
	uint bsize, nraster;
	byte *bits = cc_bits(cc);
	gs_int_rect bbox;
	int depth = cc->depth;
	static const int log2_depths[5] = { -1, 0, 1, -1, 2 };
	int log2_depth = log2_depths[depth];

	/* If the character was oversampled, compress it now. */
	/* Note that in this case the resulting number of (alpha) bits */
	/* per pixel may still be greater than 1. */

	if ( plog2_scale->x != 0 || plog2_scale->y != 0 )

	{	int ndepth = (cc->width >> plog2_scale->x) << log2_depth;
		if_debug5('k', "[k]compressing %dx%d by %dx%d to depth=%d\n",
			  cc->width, cc->height,
			  1 << plog2_scale->x, 1 << plog2_scale->y,
			  cc->depth);
#ifdef DEBUG
		if ( gs_debug_c('K') )
			debug_dump_bitmap(bits, cc->raster, cc->height,
					  "[K]uncompressed bits");
		if ( cc->width % (1 << plog2_scale->x) != 0 ||
		     cc->height % (1 << plog2_scale->y) != 0
		   )
		  {	lprintf4("size %d,%d not multiple of scale %d,%d!\n",
				 cc->width, cc->height,
				 1 << plog2_scale->x, 1 << plog2_scale->y);
			cc->width &= -1 << plog2_scale->x;
			cc->height &= -1 << plog2_scale->y;
		  }
#endif
		nraster = bitmap_raster(ndepth);
		bits_compress_scaled(bits, cc->width, cc->height, raster,
				     bits, nraster, plog2_scale, log2_depth);
		cc->raster = raster = nraster;
		cc->width >>= plog2_scale->x;
		cc->height >>= plog2_scale->y;
	}

	/* Compress the character further by removing */
	/* white space around the edges. */

	image_bbox(bits, cc->height, raster, log2_depth, &bbox);
	cc->height = bbox.q.y - bbox.p.y;
	bsize = raster * cc->height;
	if ( bbox.p.y != 0 )
	  {	/* Move the bits down. */
		memmove(bits, bits + bbox.p.y * raster, bsize);
		cc->offset.y -= int2fixed(bbox.p.y);
	  }
	/* We'd like to trim off left and right blank space too, */
	/* but currently we're only willing to move bytes, not bits. */
	bbox.p.x &= ~0 << (3 - log2_depth);
	nraster = bitmap_raster((bbox.q.x - bbox.p.x) * depth);
	if ( nraster < raster )
	  {	/* Move the bits over. */
		register byte *to = bits;
		register const byte *from =
		  bits + (bbox.p.x >> (3 - log2_depth));
		uint n = cc->height;

		for ( ; n--; from += raster, to += nraster )
		  memmove(to, from, nraster);
		cc->raster = raster = nraster;
		bsize = raster * cc->height;
		cc->offset.x -= int2fixed(bbox.p.x);
		cc->width = bbox.q.x - bbox.p.x;
	  }

	/* Discard the memory device overhead that follows the bits, */
	/* and any space reclaimed from unscaling or compression. */

	{	uint diff = round_down(gdev_mem_bitmap_size(dev) - bsize,
					align_cached_char_mod);
		if ( diff >= sizeof(cached_char_head) )
		{	shorten_cached_char(dir, cc, diff);
			dir->ccache.bsize -= diff;
			if_debug2('K', "[K]shortening char 0x%lx by %u (mdev overhead)\n",
				  (ulong)cc, diff);
		}
	}

	/* Assign a bitmap id. */

	cc->id = gs_next_ids(1);
}

/* Purge from the caches all references to a given font. */
void
gs_purge_font_from_char_caches(gs_font_dir *dir, const gs_font *font)
{	cached_fm_pair *pair = dir->fmcache.mdata;
	int count = dir->fmcache.mmax;
	if_debug1('k', "[k]purging font 0x%lx\n",
		  (ulong)font);
	while ( count-- )
	{	if ( pair->font == font )
		{	if ( uid_is_valid(&pair->UID) )
			{	/* Keep the entry. */
				pair->font = 0;
			}
			else
				gs_purge_fm_pair(dir, pair, 0);
		}
		pair++;
	}
}

/* ------ Internal routines ------ */

/* Allocate a character in a given chunk, which the caller will make */
/* (or ensure) current. */
private cached_char *
alloc_char_in_chunk(gs_font_dir *dir, ulong icdsize,
  char_cache_chunk *cck, uint cnext)
{	uint cdsize;
	cached_char_head *cch;
#define hcc ((cached_char *)cch)
	cached_char *cc;
	uint fsize = 0;
	if ( icdsize + sizeof(cached_char_head) > cck->size - cnext )
	{	/* Not enough room to allocate here. */
		return 0;
	}
	cdsize = (uint)icdsize;
	/* Look for and/or free enough space. */
	cch = (cached_char_head *)(cck->data + cnext);
	cc = hcc;
	while ( !(fsize == cdsize ||
		  fsize >= cdsize + sizeof(cached_char_head))
	      )
	{	if ( !cc_head_is_free(cch) )
		{	/* Free the character */
			cached_char **pcc =
				chars_head(dir, hcc->code, cch->pair);
			while ( *pcc != hcc )
				pcc = &(*pcc)->next;
			*pcc = hcc->next; /* remove from chain */
			gx_free_cached_char(dir, hcc);
		}
		fsize += cch->size;
		if_debug2('K', "[K]merging free char 0x%lx(%u)\n",
			  (ulong)cch, cch->size);
		cc->head.size = fsize;
		cch = (cached_char_head *)((byte *)cc + fsize);
	}
#undef hcc
	cc->chunk = cck;
	cc->loc = (byte *)cc - cck->data;
	if ( fsize > cdsize )
	  { shorten_cached_char(dir, cc, fsize - cdsize);
	    if_debug2('K', "[K]shortening char 0x%lx by %u (initial)\n",
		      (ulong)cc, fsize - cdsize);
	  }
	dir->ccache.csize++;
	dir->ccache.bsize += cdsize;
	dir->ccache.cnext = cc->loc + cdsize;
	return cc;
}

/* Shorten a cached character. */
/* diff >= sizeof(cached_char_head). */
private void
shorten_cached_char(gs_font_dir *dir, cached_char *cc, uint diff)
{	char_cache_chunk *cck = cc->chunk;
	cached_char_head *next;
	if ( (byte *)cc + cc->head.size == cck->data + dir->ccache.cnext &&
	     cck == dir->ccache.chunks
	   )
		dir->ccache.cnext -= diff;
	cc->head.size -= diff;
	next = (cached_char_head *)((byte *)cc + cc->head.size);
	if_debug2('K', "[K]shortening creates free block 0x%lx(%u)\n",
		  (ulong)next, diff);
	cc_head_set_free(next);
	next->size = diff;
}

/* ------ Image manipulation ------ */

/* Find the bounding box of a bitmap. */
/* Assume bits beyond the width are zero. */
private void
image_bbox(const byte *data, uint height, uint raster, int log2_depth,
  gs_int_rect *pbox)
{	register const byte *p;
	register uint n;
	uint bsize = raster * height;

	/* Count trailing blank rows. */

	for ( p = data + bsize, n = bsize; n && !p[-1]; )
	  --n, --p;
	if ( n == 0 )
	  {	pbox->p.x = pbox->q.x = pbox->p.y = pbox->q.y = 0;
		return;
	  }
	pbox->q.y = height = (n + raster - 1) / raster;
	bsize = raster * height;

	/* Count leading blank rows. */

	{	uint count;

		for ( p = data; !*p; )
		  ++p;
		pbox->p.y = count = (p - data) / raster;
		if ( count )
		  height -= count,
		  count *= raster, data += count, bsize -= count;
	}

	/* Find the left and right edges. */
	/* We know that the first and last rows are non-blank. */

	{	uint left = raster - 1, right = 0;
		uint lbyte = 0, rbyte = 0;
		int depth = 1 << log2_depth;
		uint lmask = 0x100 - (0x100 >> depth);
		uint rmask = (1 << depth) - 1;
		const byte *q;
		uint h;

		for ( q = data, h = height; h-- > 0; q += raster )
		  {	for ( p = q, n = 0; n < left && !*p; p++, n++ ) ;
			if ( n < left )
			  left = n, lbyte = *p;
			else
			  lbyte |= *p;
			for ( p = q + raster - 1, n = raster - 1;
			      n > right && !*p; p--, n--
			    )
			  ;
			if ( n > right )
			  right = n, rbyte = *p;
			else
			  rbyte |= *p;
		  }
		left <<= 3 - log2_depth;
		while ( !(lbyte & lmask) )
		  left++, lbyte <<= depth;
		right = (right + 1) << (3 - log2_depth);
		while ( !(rbyte & rmask) )
		  right--, rbyte >>= depth;
		pbox->p.x = left;
		pbox->q.x = right;
	}
}	

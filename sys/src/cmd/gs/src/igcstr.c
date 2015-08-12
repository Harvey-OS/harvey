/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Copyright (C) 1994, 1995, 1996, 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: igcstr.c,v 1.7 2005/09/05 13:58:55 leonardo Exp $ */
/* String GC routines for Ghostscript */
#include "memory_.h"
#include "ghost.h"
#include "gsmdebug.h"
#include "gsstruct.h"
#include "iastate.h"
#include "igcstr.h"

/* Forward references */
private
bool gc_mark_string(const byte *, uint, bool, const chunk_t *);

/* (Un)mark the strings in a chunk. */
void
gc_strings_set_marks(chunk_t *cp, bool mark)
{
	if(cp->smark != 0) {
		if_debug3('6', "[6]clearing string marks 0x%lx[%u] to %d\n",
			  (uint32_t)cp->smark, cp->smark_size, (int)mark);
		memset(cp->smark, 0, cp->smark_size);
		if(mark)
			gc_mark_string(cp->sbase, cp->climit - cp->sbase, true, cp);
	}
}

/* We mark strings a word at a time. */
typedef string_mark_unit bword;

#define bword_log2_bytes log2_sizeof_string_mark_unit
#define bword_bytes (1 << bword_log2_bytes)
#define bword_log2_bits (bword_log2_bytes + 3)
#define bword_bits (1 << bword_log2_bits)
#define bword_1s (~(bword)0)
/* Compensate for byte order reversal if necessary. */
#if arch_is_big_endian
#if bword_bytes == 2
#define bword_swap_bytes(m) m = (m << 8) | (m >> 8)
#else /* bword_bytes == 4 */
#define bword_swap_bytes(m) \
	m = (m << 24) | ((m & 0xff00) << 8) | ((m >> 8) & 0xff00) | (m >> 24)
#endif
#else
#define bword_swap_bytes(m) DO_NOTHING
#endif

/* (Un)mark a string in a known chunk.  Return true iff any new marks. */
private
bool
gc_mark_string(const byte *ptr, uint size, bool set, const chunk_t *cp)
{
	uint offset = ptr - cp->sbase;
	bword *bp = (bword *)(cp->smark + ((offset & -bword_bits) >> 3));
	uint bn = offset & (bword_bits - 1);
	bword m = bword_1s << bn;
	uint left = size;
	bword marks = 0;

	bword_swap_bytes(m);
	if(set) {
		if(left + bn >= bword_bits) {
			marks |= ~*bp & m;
			*bp |= m;
			m = bword_1s, left -= bword_bits - bn, bp++;
			while(left >= bword_bits) {
				marks |= ~*bp;
				*bp = bword_1s;
				left -= bword_bits, bp++;
			}
		}
		if(left) {
			bword_swap_bytes(m);
			m -= m << left;
			bword_swap_bytes(m);
			marks |= ~*bp & m;
			*bp |= m;
		}
	} else {
		if(left + bn >= bword_bits) {
			*bp &= ~m;
			m = bword_1s, left -= bword_bits - bn, bp++;
			if(left >= bword_bits * 5) {
				memset(bp, 0, (left & -bword_bits) >> 3);
				bp += left >> bword_log2_bits;
				left &= bword_bits - 1;
			} else
				while(left >= bword_bits) {
					*bp = 0;
					left -= bword_bits, bp++;
				}
		}
		if(left) {
			bword_swap_bytes(m);
			m -= m << left;
			bword_swap_bytes(m);
			*bp &= ~m;
		}
	}
	return marks != 0;
}

/* Mark a string.  Return true if any new marks. */
bool
gc_string_mark(const byte *ptr, uint size, bool set, gc_state_t *gcst)
{
	const chunk_t *cp;
	bool marks;

	if(size == 0)
		return false;
#define dprintstr()                             \
	dputc('(');                             \
	fwrite(ptr, 1, min(size, 20), dstderr); \
	dputs((size <= 20 ? ")" : "...)"))
	if(!(cp = gc_locate(ptr, gcst))) { /* not in a chunk */
#ifdef DEBUG
		if(gs_debug_c('5')) {
			dlprintf2("[5]0x%lx[%u]", (uint32_t)ptr, size);
			dprintstr();
			dputs(" not in a chunk\n");
		}
#endif
		return false;
	}
	if(cp->smark == 0) /* not marking strings */
		return false;
#ifdef DEBUG
	if(ptr < cp->ctop) {
		lprintf4("String pointer 0x%lx[%u] outside [0x%lx..0x%lx)\n",
			 (uint32_t)ptr, size, (uint32_t)cp->ctop,
			 (uint32_t)cp->climit);
		return false;
	} else if(ptr + size > cp->climit) { /*
						 * If this is the bottommost string in a chunk that has
						 * an inner chunk, the string's starting address is both
						 * cp->ctop of the outer chunk and cp->climit of the inner;
						 * gc_locate may incorrectly attribute the string to the
						 * inner chunk because of this.  This doesn't affect
						 * marking or relocation, since the machinery for these
						 * is all associated with the outermost chunk,
						 * but it can cause the validity check to fail.
						 * Check for this case now.
						 */
		const chunk_t *scp = cp;

		while(ptr == scp->climit && scp->outer != 0)
			scp = scp->outer;
		if(ptr + size > scp->climit) {
			lprintf4("String pointer 0x%lx[%u] outside [0x%lx..0x%lx)\n",
				 (uint32_t)ptr, size,
				 (uint32_t)scp->ctop, (uint32_t)scp->climit);
			return false;
		}
	}
#endif
	marks = gc_mark_string(ptr, size, set, cp);
#ifdef DEBUG
	if(gs_debug_c('5')) {
		dlprintf4("[5]%s%smarked 0x%lx[%u]",
			  (marks ? "" : "already "), (set ? "" : "un"),
			  (uint32_t)ptr, size);
		dprintstr();
		dputc('\n');
	}
#endif
	return marks;
}

/* Clear the relocation for strings. */
/* This requires setting the marks. */
void
gc_strings_clear_reloc(chunk_t *cp)
{
	if(cp->sreloc != 0) {
		gc_strings_set_marks(cp, true);
		if_debug1('6', "[6]clearing string reloc 0x%lx\n",
			  (uint32_t)cp->sreloc);
		gc_strings_set_reloc(cp);
	}
}

/* Count the 0-bits in a byte. */
private
const byte count_zero_bits_table[256] =
    {
#define o4(n) n, n - 1, n - 1, n - 2
#define o16(n) o4(n), o4(n - 1), o4(n - 1), o4(n - 2)
#define o64(n) o16(n), o16(n - 1), o16(n - 1), o16(n - 2)
     o64(8), o64(7), o64(7), o64(6)};

#define byte_count_zero_bits(byt) \
	(uint)(count_zero_bits_table[byt])
#define byte_count_one_bits(byt) \
	(uint)(8 - count_zero_bits_table[byt])

/* Set the relocation for the strings in a chunk. */
/* The sreloc table stores the relocated offset from climit for */
/* the beginning of each block of string_data_quantum characters. */
void
gc_strings_set_reloc(chunk_t *cp)
{
	if(cp->sreloc != 0 && cp->smark != 0) {
		byte *bot = cp->ctop;
		byte *top = cp->climit;
		uint count =
		    (top - bot + (string_data_quantum - 1)) >>
		    log2_string_data_quantum;
		string_reloc_offset *relp =
		    cp->sreloc +
		    (cp->smark_size >> (log2_string_data_quantum - 3));
		register const byte *bitp = cp->smark + cp->smark_size;
		register string_reloc_offset reloc = 0;

/* Skip initial unrelocated strings quickly. */
#if string_data_quantum == bword_bits || string_data_quantum == bword_bits * 2
		{
			/* Work around the alignment aliasing bug. */
			const bword *wp = (const bword *)bitp;

#if string_data_quantum == bword_bits
#define RELOC_TEST_1S(wp) (wp[-1])
#else /* string_data_quantum == bword_bits * 2 */
#define RELOC_TEST_1S(wp) (wp[-1] & wp[-2])
#endif
			while(count && RELOC_TEST_1S(wp) == bword_1s) {
				wp -= string_data_quantum / bword_bits;
				*--relp = reloc += string_data_quantum;
				--count;
			}
#undef RELOC_TEST_1S
			bitp = (const byte *)wp;
		}
#endif
		while(count--) {
			bitp -= string_data_quantum / 8;
			reloc += string_data_quantum -
				 byte_count_zero_bits(bitp[0]);
			reloc -= byte_count_zero_bits(bitp[1]);
			reloc -= byte_count_zero_bits(bitp[2]);
			reloc -= byte_count_zero_bits(bitp[3]);
#if log2_string_data_quantum > 5
			reloc -= byte_count_zero_bits(bitp[4]);
			reloc -= byte_count_zero_bits(bitp[5]);
			reloc -= byte_count_zero_bits(bitp[6]);
			reloc -= byte_count_zero_bits(bitp[7]);
#endif
			*--relp = reloc;
		}
	}
	cp->sdest = cp->climit;
}

/* Relocate a string pointer. */
void
igc_reloc_string(gs_string *sptr, gc_state_t *gcst)
{
	byte *ptr;
	const chunk_t *cp;
	uint offset;
	uint reloc;
	const byte *bitp;
	byte byt;

	if(sptr->size == 0) {
		sptr->data = 0;
		return;
	}
	ptr = sptr->data;
	if(!(cp = gc_locate(ptr, gcst))) /* not in a chunk */
		return;
	if(cp->sreloc == 0 || cp->smark == 0) /* not marking strings */
		return;
	offset = ptr - cp->sbase;
	reloc = cp->sreloc[offset >> log2_string_data_quantum];
	bitp = &cp->smark[offset >> 3];
	switch(offset & (string_data_quantum - 8)) {
#if log2_string_data_quantum > 5
	case 56:
		reloc -= byte_count_one_bits(bitp[-7]);
	case 48:
		reloc -= byte_count_one_bits(bitp[-6]);
	case 40:
		reloc -= byte_count_one_bits(bitp[-5]);
	case 32:
		reloc -= byte_count_one_bits(bitp[-4]);
#endif
	case 24:
		reloc -= byte_count_one_bits(bitp[-3]);
	case 16:
		reloc -= byte_count_one_bits(bitp[-2]);
	case 8:
		reloc -= byte_count_one_bits(bitp[-1]);
	}
	byt = *bitp & (0xff >> (8 - (offset & 7)));
	reloc -= byte_count_one_bits(byt);
	if_debug2('5', "[5]relocate string 0x%lx to 0x%lx\n",
		  (uint32_t)ptr, (uint32_t)(cp->sdest - reloc));
	sptr->data = cp->sdest - reloc;
}
void
igc_reloc_const_string(gs_const_string *sptr, gc_state_t *gcst)
{ /* We assume the representation of byte * and const byte * is */
	/* the same.... */
	igc_reloc_string((gs_string *)sptr, gcst);
}
void
igc_reloc_param_string(gs_param_string *sptr, gc_state_t *gcst)
{
	if(!sptr->persistent) {
		/* We assume that gs_param_string is a subclass of gs_string. */
		igc_reloc_string((gs_string *)sptr, gcst);
	}
}

/* Compact the strings in a chunk. */
void
gc_strings_compact(chunk_t *cp)
{
	if(cp->smark != 0) {
		byte *hi = cp->climit;
		byte *lo = cp->ctop;
		const byte *from = hi;
		byte *to = hi;
		const byte *bp = cp->smark + cp->smark_size;

#ifdef DEBUG
		if(gs_debug_c('4') || gs_debug_c('5')) {
			byte *base = cp->sbase;
			uint i = (lo - base) & -string_data_quantum;
			uint n = ROUND_UP(hi - base, string_data_quantum);

#define R 16
			for(; i < n; i += R) {
				uint j;

				dlprintf1("[4]0x%lx: ", (uint32_t)(base + i));
				for(j = i; j < i + R; j++) {
					byte ch = base[j];

					if(ch <= 31) {
						dputc('^');
						dputc(ch + 0100);
					} else
						dputc(ch);
				}
				dputc(' ');
				for(j = i; j < i + R; j++)
					dputc((cp->smark[j >> 3] & (1 << (j & 7)) ? '+' : '.'));
#undef R
				if(!(i & (string_data_quantum - 1)))
					dprintf1(" %u", cp->sreloc[i >> log2_string_data_quantum]);
				dputc('\n');
			}
		}
#endif
		/*
	 * Skip unmodified strings quickly.  We know that cp->smark is
	 * aligned to a string_mark_unit.
	 */
		{
			/* Work around the alignment aliasing bug. */
			const bword *wp = (const bword *)bp;

			while(to > lo && wp[-1] == bword_1s)
				to -= bword_bits, --wp;
			bp = (const byte *)wp;
			while(to > lo && bp[-1] == 0xff)
				to -= 8, --bp;
		}
		from = to;
		while(from > lo) {
			byte b = *--bp;

			from -= 8;
			switch(b) {
			case 0xff:
				to -= 8;
				/*
		     * Since we've seen a byte other than 0xff, we know
		     * to != from at this point.
		     */
				to[7] = from[7];
				to[6] = from[6];
				to[5] = from[5];
				to[4] = from[4];
				to[3] = from[3];
				to[2] = from[2];
				to[1] = from[1];
				to[0] = from[0];
				break;
			default:
				if(b & 0x80)
					*--to = from[7];
				if(b & 0x40)
					*--to = from[6];
				if(b & 0x20)
					*--to = from[5];
				if(b & 0x10)
					*--to = from[4];
				if(b & 8)
					*--to = from[3];
				if(b & 4)
					*--to = from[2];
				if(b & 2)
					*--to = from[1];
				if(b & 1)
					*--to = from[0];
			/* falls through */
			case 0:
				;
			}
		}
		gs_alloc_fill(cp->ctop, gs_alloc_fill_collected,
			      to - cp->ctop);
		cp->ctop = to;
	}
}

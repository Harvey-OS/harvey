/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* igcstr.c */
/* String GC routines for Ghostscript */
#include "memory_.h"
#include "ghost.h"
#include "gsstruct.h"
#include "iastate.h"
#include "igc.h"
#include "isave.h"			/* for gc_locate */
#include "isstate.h"			/* ditto */

/* Import the debugging variables from gsmemory.c. */
extern byte gs_alloc_fill_collected;

/* (Un)mark the strings in a chunk. */
void
gc_strings_set_marks(chunk_t *cp, bool mark)
{	if ( cp->smark != 0 )
	{ if_debug3('6', "[6]clearing string marks 0x%lx[%u] to %d\n",
		    (ulong)cp->smark, cp->smark_size, (int)mark);
	  memset(cp->smark, (mark ? 0xff : 0), cp->smark_size);
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
#  if bword_bytes == 2
#    define bword_swap_bytes(m) m = (m << 8) | (m >> 8)
#  else				/* bword_bytes == 4 */
#    define bword_swap_bytes(m)\
	m = (m << 24) | ((m & 0xff00) << 8) | ((m >> 8) & 0xff00) | (m >> 24)
#  endif
#else
#  define bword_swap_bytes(m) DO_NOTHING
#endif

/* Mark a string.  Return true if any new marks. */
bool
gc_string_mark(const byte *ptr, uint size, bool set, gc_state_t *gcst)
{	uint left = size;
	bword *bp;
	uint bn;
	bword m;
	const chunk_t *cp;
	uint offset;
	bword marks = 0;

	if ( size == 0 )
	  return false;
#define dprintstr()\
  dputc('('); fwrite(ptr, 1, min(size, 20), dstderr);\
  dputs((size <= 20 ? ")" : "...)"))
	if ( !gc_locate(ptr, gcst) )	/* not in a chunk */
	  {
#ifdef DEBUG
		if ( gs_debug_c('5') )
		  {	dprintf2("[5]0x%lx[%u]", (ulong)ptr, size);
			dprintstr();
			dputs(" not in a chunk\n");
		  }
#endif		
		return false;
	  }
	cp = gcst->loc.cp;
	if ( cp->smark == 0 )		/* not marking strings */
	  return false;
#ifdef DEBUG
	if ( ptr < cp->ctop )
	  {	lprintf4("String pointer 0x%lx[%u] outside [0x%lx..0x%lx)\n",
			 (ulong)ptr, size, (ulong)cp->ctop, (ulong)cp->climit);
		return false;
	  }
	else if ( ptr + size > cp->climit )
	  {	/*
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
		while ( ptr == scp->climit && scp->outer != 0 )
		  scp = scp->outer;
		if ( ptr + size > scp->climit )
		  {	lprintf4("String pointer 0x%lx[%u] outside [0x%lx..0x%lx)\n",
				 (ulong)ptr, size,
				 (ulong)scp->ctop, (ulong)scp->climit);
			return false;
		  }
	  }
#endif
	offset = ptr - cp->sbase;
	bp = (bword *)(cp->smark + ((offset & -bword_bits) >> 3));
	bn = offset & (bword_bits - 1);
	m = bword_1s << bn;
	bword_swap_bytes(m);
	if ( set )
	  {	if ( left + bn >= bword_bits )
		  {	marks |= ~*bp & m;
			*bp |= m;
			m = bword_1s, left -= bword_bits - bn, bp++;
			while ( left >= bword_bits )
			  {	marks |= ~*bp;
				*bp = bword_1s;
				left -= bword_bits, bp++;
			  }
		  }
		if ( left )
		  {	bword_swap_bytes(m);
			m -= m << left;
			bword_swap_bytes(m);
			marks |= ~*bp & m;
			*bp |= m;
		  }
	  }
	else
	  {	if ( left + bn >= bword_bits )
		  {	*bp &= ~m;
			m = bword_1s, left -= bword_bits - bn, bp++;
			if ( left >= bword_bits * 5 )
			  {	memset(bp, 0, (left & -bword_bits) >> 3);
				bp += left >> bword_log2_bits;
				left &= bword_bits - 1;
			  }
			else
			  while ( left >= bword_bits )
			    {	*bp = 0;
				left -= bword_bits, bp++;
			    }
		  }
		if ( left )
		  {	bword_swap_bytes(m);
			m -= m << left;
			bword_swap_bytes(m);
			*bp &= ~m;
		  }
	  }
#ifdef DEBUG
	if ( gs_debug_c('5') )
	  {	dprintf4("[5]%s%smarked 0x%lx[%u]",
			 (marks ? "" : "already "), (set ? "" : "un"),
			 (ulong)ptr, size);
		dprintstr();
		dputc('\n');
	  }
#endif
	return marks != 0;
}

/* Clear the relocation for strings. */
void
gc_strings_clear_reloc(chunk_t *cp)
{	if ( cp->sreloc != 0 )
	{ uint sreloc_size = ((cp->smark_size + 3) >> 2) * sizeof(ushort);
	  if_debug2('6', "[6]clearing string reloc 0x%lx[%u]\n",
		    (ulong)cp->sreloc, sreloc_size);
	  memset(cp->sreloc, 0, sreloc_size);
	}
}

/* Count the 0-bits in a byte. */
private const byte count_zero_bits_table[256] = {
#define o4(n) n,n-1,n-1,n-2
#define o16(n) o4(n),o4(n-1),o4(n-1),o4(n-2)
#define o64(n) o16(n),o16(n-1),o16(n-1),o16(n-2)
	o64(8), o64(7), o64(7), o64(6)
};
#define byte_count_zero_bits(byt)\
  (uint)(count_zero_bits_table[byt])
#define byte_count_one_bits(byt)\
  (uint)(8 - count_zero_bits_table[byt])

/* Set the relocation for the strings in a chunk. */
/* The sreloc table stores the relocated offset from climit for */
/* the beginning of each block of 32 characters. */
void
gc_strings_set_reloc(chunk_t *cp)
{	if ( cp->sreloc != 0 && cp->smark != 0 )
	{	byte *bot = cp->ctop;
		byte *top = cp->climit;
		uint count = (top - bot + 31) >> 5;
		ushort *relp = cp->sreloc + (cp->smark_size >> 2);
		register byte *bitp = cp->smark + cp->smark_size;
		register ushort reloc = 0;
		while ( count-- )
		{	bitp -= 4;
			reloc += 32 - byte_count_zero_bits(bitp[0]);
			reloc -= byte_count_zero_bits(bitp[1]);
			reloc -= byte_count_zero_bits(bitp[2]);
			reloc -= byte_count_zero_bits(bitp[3]);
			*--relp = reloc;
		}
	}
	cp->sdest = cp->climit;
}

/* Relocate a string pointer. */
void
gs_reloc_string(gs_string *sptr, gc_state_t *gcst)
{	byte *ptr;
	const chunk_t *cp;
	uint offset;
	uint reloc;
	const byte *bitp;
	byte byt;
	if ( sptr->size == 0 )
	  {	sptr->data = 0;
		return;
	  }
	ptr = sptr->data;
	if ( !gc_locate(ptr, gcst) )	/* not in a chunk */
	  return;
	cp = gcst->loc.cp;
	if ( cp->sreloc == 0 || cp->smark == 0 ) /* not marking strings */
	  return;
	offset = ptr - cp->sbase;
	reloc = cp->sreloc[offset >> 5];
	bitp = &cp->smark[offset >> 3];
	switch ( offset & 24 )
	{
	case 24: reloc -= byte_count_one_bits(bitp[-3]);
	case 16: reloc -= byte_count_one_bits(bitp[-2]);
	case 8: reloc -= byte_count_one_bits(bitp[-1]);
	}
	byt = *bitp & (0xff >> (8 - (offset & 7)));
	reloc -= byte_count_one_bits(byt);
	if_debug2('5', "[5]relocate string 0x%lx to 0x%lx\n",
		  (ulong)ptr, (ulong)(cp->sdest - reloc));
	sptr->data = cp->sdest - reloc;
}
void
gs_reloc_const_string(gs_const_string *sptr, gc_state_t *gcst)
{	/* We assume the representation of byte * and const byte * is */
	/* the same.... */
	gs_reloc_string((gs_string *)sptr, gcst);
}

/* Compact the strings in a chunk. */
void
gc_strings_compact(chunk_t *cp)
{	if ( cp->smark != 0 )
	{	byte *hi = cp->climit;
		byte *lo = cp->ctop;
		register const byte *from = hi;
		register byte *to = hi;
		register const byte *bp = cp->smark + cp->smark_size;
#ifdef DEBUG
		if ( gs_debug_c('4') || gs_debug_c('5') )
		  { byte *base = cp->sbase;
		    uint i = (lo - base) & ~31;
		    uint n = (hi - base + 31) & ~31;
#define R 16
		    for ( ; i < n; i += R )
		      { uint j;
			dprintf1("[4]0x%lx: ", (ulong)(base + i));
			for ( j = i; j < i + R; j++ )
			  { byte ch = base[j];
			    if ( ch <= 31 )
			      { dputc('^'); dputc(ch + 0100); }
			    else
			      dputc(ch);
			  }
			dputc(' ');
			for ( j = i; j < i + R; j++ )
			  dputc((cp->smark[j >> 3] & (1 << (j & 7)) ?
				 '+' : '.'));
#undef R
			if ( !(i & 31) )
			  dprintf1(" %u", cp->sreloc[i >> 5]);
			dputc('\n');
		      }
		  }
#endif
		while ( from > lo )
		  { register byte b = *--bp;
		    from -= 8;
		    switch ( b )
		      {
		      case 0xff:
			to -= 8;
			if ( to != from )
			  { to[7] = from[7]; to[6] = from[6];
			    to[5] = from[5]; to[4] = from[4];
			    to[3] = from[3]; to[2] = from[2];
			    to[1] = from[1]; to[0] = from[0];
			  }
			break;
		      default:
			if ( b & 0x80 ) *--to = from[7];
			if ( b & 0x40 ) *--to = from[6];
			if ( b & 0x20 ) *--to = from[5];
			if ( b & 0x10 ) *--to = from[4];
			if ( b & 8 ) *--to = from[3];
			if ( b & 4 ) *--to = from[2];
			if ( b & 2 ) *--to = from[1];
			if ( b & 1 ) *--to = from[0];
			/* falls through */
		      case 0:
			;
		      }
		  }
		if ( gs_alloc_debug )
		  memset(cp->ctop, gs_alloc_fill_collected, to - cp->ctop);
		cp->ctop = to;
	}
}

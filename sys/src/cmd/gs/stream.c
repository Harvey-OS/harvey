/* Copyright (C) 1989, 1995 Aladdin Enterprises.  All rights reserved.
  
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

/* stream.c */
/* Stream package for Ghostscript interpreter */
#include "stdio_.h"		/* includes std.h */
#include "memory_.h"
#include "gdebug.h"
#include "gpcheck.h"
#include "stream.h"
#include "strimpl.h"

/* Forward declarations */
private int sreadbuf(P2(stream *, stream_cursor_write *));
private int swritebuf(P3(stream *, stream_cursor_read *, bool));
private void stream_compact(P1(stream *));

/* Structure types for allocating streams. */
private_st_stream();
public_st_stream_state();	/* default */
/* GC procedures */
#define st ((stream *)vptr)
private ENUM_PTRS_BEGIN(stream_enum_ptrs) return 0;
	case 0:
		if ( st->foreign )
		  *pep = NULL;
		else if ( st->cbuf_string.data != 0 )
		  { ENUM_RETURN_STRING_PTR(stream, cbuf_string);
		  }
		else
		  *pep = st->cbuf;
		break;
	ENUM_PTR(1, stream, strm);
	ENUM_PTR(2, stream, prev);
	ENUM_PTR(3, stream, next);
	ENUM_PTR(4, stream, state);
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(stream_reloc_ptrs) {
	byte *cbuf_old = st->cbuf;
	if ( cbuf_old != 0 && !st->foreign )
	{	uint reloc;
		if ( st->cbuf_string.data != 0 )
		  {	RELOC_STRING_PTR(stream, cbuf_string);
			st->cbuf = st->cbuf_string.data;
		  }
		else
		  RELOC_PTR(stream, cbuf);
		reloc = cbuf_old - st->cbuf;
		/* Relocate the other buffer pointers. */
		st->srptr -= reloc;
		st->srlimit -= reloc;		/* same as swptr */
		st->swlimit -= reloc;
	}
	RELOC_PTR(stream, strm);
	RELOC_PTR(stream, prev);
	RELOC_PTR(stream, next);
	RELOC_PTR(stream, state);
} RELOC_PTRS_END
#undef st

/* Dummy template for streams that don't have a separate state. */
private const stream_template s_no_template =
{	&st_stream_state, 0, 0, 1, 1, 0
};

/* ------ Generic procedures ------ */

/* Allocate a stream and initialize it minimally. */
stream *
s_alloc(gs_memory_t *mem, client_name_t cname)
{	stream *s = gs_alloc_struct(mem, stream, &st_stream, cname);
	if ( s == 0 )
	  return 0;
	s->memory = mem;
	s->report_error = s_no_report_error;
	return s;
}

/* Allocate a stream state and initialize it minimally. */
stream_state *
s_alloc_state(gs_memory_t *mem, gs_memory_type_ptr_t stype,
  client_name_t cname)
{	stream_state *st = gs_alloc_struct(mem, stream_state, stype, cname);
	if ( st == 0 )
	  return 0;
	st->memory = mem;
	st->report_error = s_no_report_error;
	return st;
}

/* Vacuous stream initialization */
int
s_no_init(stream_state *st)
{	return 0;
}

/* Standard stream initialization */
void
s_std_init(register stream *s, byte *ptr, uint len, const stream_procs *pp,
  int modes)
{	s->template = &s_no_template;
	s->cbuf = ptr;
	s->srptr = s->srlimit = s->swptr = ptr - 1;
	s->swlimit = ptr - 1 + len;
	s->end_status = 0;
	s->foreign = 0;
	s->modes = modes;
	s->cbuf_string.data = 0;
	s->position = 0;
	s->bsize = s->cbsize = len;
	s->strm = 0;			/* not a filter */
	s->is_temp = 0;
	s->procs = *pp;
	s->state = 0;
	s->file = 0;
	if_debug4('s', "[s]init 0x%lx, buf=0x%lx, len=%u, modes=%d\n",
		  (ulong)s, (ulong)ptr, len, modes);
}

/* Implement a stream procedure as a no-op. */
int
s_std_null(stream *s)
{	return 0;
}

/* Discard the contents of the buffer when reading. */
void
s_std_read_reset(stream *s)
{	s->srptr = s->srlimit = s->cbuf - 1;
}

/* Discard the contents of the buffer when writing. */
void
s_std_write_reset(stream *s)
{	s->swptr = s->cbuf- 1;
}

/* Flush data to end-of-file when reading. */
int
s_std_read_flush(stream *s)
{	while ( 1 )
	{	s->srptr = s->srlimit = s->cbuf - 1;
		if ( s->end_status ) break;
		s_process_read_buf(s);
	}
	return (s->end_status == EOFC ? 0 : s->end_status);
}

/* Flush buffered data when writing. */
int
s_std_write_flush(stream *s)
{	return s_process_write_buf(s, false);
}

/* Indicate that the number of available input bytes is unknown. */
int
s_std_noavailable(stream *s, long *pl)
{	*pl = -1;
	return 0;
}

/* Indicate an error when asked to seek. */
int
s_std_noseek(stream *s, long pos)
{	return ERRC;
}

/* Standard stream closing. */
int
s_std_close(stream *s)
{	return 0;
}

/* Standard stream finalization.  Disable the stream. */
void
s_disable(register stream *s)
{	s->cbuf = 0;
	s->bsize = 0;
	s->end_status = EOFC;
	s->modes = 0;
	s->cbuf_string.data = 0;
	s->cursor.r.ptr = s->cursor.r.limit = (const byte *)0 - 1;
	s->cursor.w.limit = (byte *)0 - 1;
	s->procs.close = s_std_null;
	/* Clear pointers for GC */
	s->strm = 0;
	s->state = 0;
	/****** SHOULD DO MORE THAN THIS ******/
	if_debug1('s', "[s]disable 0x%lx\n", (ulong)s);
}

/* Implement flushing for encoding filters. */
int
s_filter_write_flush(register stream *s)
{	int status = s_process_write_buf(s, false);
	if ( status != 0 )
	  return status;
	return sflush(s->strm);
}

/* Close a filter.  If this is an encoding filter, flush it first. */
int
s_filter_close(register stream *s)
{	if ( s_is_writing(s) )
	{	int status = s_process_write_buf(s, true);
		if ( status != 0 && status != EOFC )
		  return status;
	}
	return s_std_close(s);
}	

/* Disregard a stream error message. */
int
s_no_report_error(stream_state *st, const char *str)
{	return 0;
}

/* ------ Implementation-independent procedures ------ */

/* Store the amount of available data in a(n input) stream. */
int
savailable(stream *s, long *pl)
{	return (*(s)->procs.available)(s, pl);
}

/* Return the current position of a stream. */
long
stell(stream *s)
{	/* The stream might have been closed, but the position */
	/* is still meaningful in this case. */
	const byte *ptr = (s_is_writing(s) ? s->swptr : s->srptr);
	return (ptr == 0 ? 0 : ptr + 1 - s->cbuf) + s->position;
}

/* Set the position of a stream. */
int
spseek(stream *s, long pos)
{	if_debug3('s', "[s]seek 0x%lx to %ld, position was %ld\n",
		  (ulong)s, pos, stell(s));
	return (*(s)->procs.seek)(s, pos);
}

/* Close a stream, disabling it if successful. */
/* (The stream may already be closed.) */
int
sclose(register stream *s)
{	stream_state *st;
	int code = (*s->procs.close)(s);
	if ( code < 0 )
	  return code;
	st = s->state;
	if ( st != 0 )
	  { stream_proc_release((*release)) = st->template->release;
	    if ( release != 0 )
	      (*release)(st);
	    if ( st != (stream_state *)s && st->memory != 0 )
	      gs_free_object(st->memory, st, "s_std_close");
	    s->state = 0;
	  }
	s_disable(s);
	return code;
}

/* Implement sgetc when the buffer may be empty. */
/* If the buffer really is empty, refill it and then read a byte. */
int
spgetc(register stream *s)
{	while ( sendrp(s) )
	{	int status = s->end_status;
		switch ( status )
		  {
		case EOFC:
			/* Do the equivalent of stream_compact, */
			/* so stell will return the right result. */
			s->position += s->cursor.r.ptr + 1 - s->cbuf;
			s->cursor.r.ptr = s->cursor.r.limit = s->cbuf - 1;
			status = sclose(s);
			if ( status == 0 )
			  return EOFC;
			s->end_status = status;
		default:		/* ERRC */
			return status;
		case 0:
			;
		  }
		s_process_read_buf(s);
	}
	return *++(s->srptr);
}

/* Implementing sputc when the buffer is full, */
/* by flushing the buffer and then writing the byte. */
int
spputc(register stream *s, byte b)
{	for ( ; ; )
	{	if ( s->end_status )
		  return s->end_status;
		if ( !sendwp(s) )
		{	*++(s->swptr) = b;
			return b;
		}
		s_process_write_buf(s, false);
	}
}

/* Push back a character onto a (read) stream. */
/* The character must be the same as the last one read. */
/* Return 0 on success, ERRC on failure. */
int
sungetc(register stream *s, byte c)
{	if ( !s_is_reading(s) || s->srptr < s->cbuf || *(s->srptr) != c )
	  return ERRC;
	s->srptr--;
	return 0;
}

/* Get a string from a stream. */
/* Return 0 if the string was filled, or an exception status. */
int
sgets(stream *s, byte *buf, uint nmax, uint *pn)
{	stream_cursor_write cw;
	int status = 0;
	cw.ptr = buf - 1;
	cw.limit = cw.ptr + nmax;
	while ( cw.ptr < cw.limit )
	{	if ( !sendrp(s) )
		  stream_move(&s->cursor.r, &cw);
		else
		{	uint wanted = cw.limit - cw.ptr;
			int c;
			stream_state *st;
			if ( wanted >= s->bsize >> 2 &&
			     (st = s->state) != 0 &&
			     wanted >= st->template->min_out_size
			   )
			{	byte *wptr = cw.ptr;
				status = sreadbuf(s, &cw);
				/* We know the stream buffer is empty, */
				/* so it's safe to update position. */
				s->position += cw.ptr - wptr;
				if ( status != 1 || cw.ptr == cw.limit )
				  break;
			}
			c = spgetc(s);
			if ( c < 0 )
			{	status = c;
				break;
			}
			*++(cw.ptr) = c;
		}
	}
	*pn = cw.ptr + 1 - buf;
	return (status >= 0 ? 0 : status);
}

/* Write a string on a stream. */
/* Return 0 if the entire string was written, or an exception status. */
int
sputs(register stream *s, const byte *str, uint wlen, uint *pn)
{	uint len = wlen;
	int status = s->end_status;
	if ( status >= 0 )
	  while ( len > 0 )
	{	uint count = s->swlimit - s->swptr;
		if ( count > 0 )
		{	if ( count > len ) count = len;
			memcpy(s->swptr + 1, str, count);
			s->swptr += count;
			str += count;
			len -= count;
		}
		else
		{	byte ch = *str++;
			status = sputc(s, ch);
			if ( status < 0 )
			  break;
			len--;
		}
	}
	*pn = wlen - len;
	return (status >= 0 ? 0 : status);
}

/* Skip a specified distance in a read stream. */
/* Return 0, EOFC, or ERRC. */
int
spskip(register stream *s, long n)
{	if ( n < 0 || !s_is_reading(s) ) return ERRC;
	if ( s_can_seek(s) )
	  return sseek(s, stell(s) + n);
	while ( sbufavailable(s) < n )
	{	int code;
		n -= sbufavailable(s) + 1;
		s->srptr = s->srlimit;
		if ( s->end_status )
		  return s->end_status;
		code = sgetc(s);
		if ( code < 0 )
		  return code;
	}
	s->srptr += n;
	return 0;
}	

/* ------ Utilities ------ */

/* Implement the old read_buf procedure in terms of the new */
/* process procedure. */
int
s_process_read_buf(stream *s)
{	int status;
	stream_compact(s);
	status = sreadbuf(s, &s->cursor.w);
	s->end_status = (status >= 0 ? 0 : status);
	return 0;
}

/* Implement the old write_buf procedure in terms of the new */
/* process procedure. */
int
s_process_write_buf(stream *s, bool last)
{	int status;
	status = swritebuf(s, &s->cursor.r, last);
	stream_compact(s);
	if ( status >= 0 )
	  status = 0;
	s->end_status = status;
	return status;
}

/* Move forward or backward in a pipeline.  We temporarily reverse */
/* the direction of the pointers while doing this. */
/* (Cf the Deutsch-Schorr-Waite graph marking algorithm.) */
#define move_back(curr, prev)\
{ stream *back = prev->strm;\
  prev->strm = curr; curr = prev; prev = back;\
}
#define move_ahead(curr, prev)\
{ stream *ahead = curr->strm;\
  curr->strm = prev; prev = curr; curr = ahead;\
}

/* Read from a pipeline. */
private int
sreadbuf(stream *s, stream_cursor_write *pbuf)
{	stream *prev = 0;
	stream *curr = s;
	int status;
	for ( ; ; )
	{	for ( ; ; )
		{	/* Descend into the recursion. */
			stream *strm = curr->strm;
			stream_cursor_read cr;
			stream_cursor_read *pr;
			stream_cursor_write *pw;
			bool eof;
			if ( strm == 0 )
			  cr.ptr = 0, cr.limit = 0, pr = &cr,
			  eof = false;
			else
			  pr = &strm->cursor.r,
			  eof = strm->end_status == EOFC;
			pw = (prev == 0 ? pbuf : &curr->cursor.w);
			if_debug4('s', "[s]read process 0x%lx, nr=%u, nw=%u, eof=%d\n",
				  (ulong)curr, (uint)(pr->limit - pr->ptr),
				  (uint)(pw->limit - pw->ptr), eof);
			status = (*curr->procs.process)(curr->state, pr, pw, eof);
			if_debug4('s', "[s]after read 0x%lx, nr=%u, nw=%u, status=%d\n",
				  (ulong)curr, (uint)(pr->limit - pr->ptr),
				  (uint)(pw->limit - pw->ptr), status);
			if ( strm == 0 || status != 0 )
			  break;
			status = strm->end_status;
			if ( status < 0 )
			  break;
			move_ahead(curr, prev);
			stream_compact(curr);
		}
		/* Unwind from the recursion. */
		curr->end_status = (status >= 0 ? 0 : status);
		if ( prev == 0 )
		  return status;
		move_back(curr, prev);
	}
}

/* Write to a pipeline. */
private int
swritebuf(stream *s, stream_cursor_read *pbuf, bool last)
{	stream *prev = 0;
	stream *curr = s;
	int status;
	for ( ; ; )
	{	for ( ; ; )
		{	/* Descend into the recursion. */
			stream *strm = curr->strm;
			stream_cursor_write cw;
			stream_cursor_read *pr;
			stream_cursor_write *pw;
			bool end = last;
			if ( strm == 0 )
			  cw.ptr = 0, cw.limit = 0, pw = &cw;
			else
			  pw = &strm->cursor.w;
			if ( prev == 0 )
			  pr = pbuf;
			else
			  pr = &curr->cursor.r;
			if_debug4('s', "[s]write process 0x%lx, nr=%u, nw=%u, end=%d\n",
				  (ulong)curr, (uint)(pr->limit - pr->ptr),
				  (uint)(pw->limit - pw->ptr), end);
			status = (*curr->procs.process)(curr->state, pr, pw,
							end);
			if_debug4('s', "[s]after write 0x%lx, nr=%u, nw=%u, status=%d\n",
				  (ulong)curr, (uint)(pr->limit - pr->ptr),
				  (uint)(pw->limit - pw->ptr), status);
			if ( strm == 0 || status < 0 )
			  break;
			if ( status == 1 )
			  end = false;
			else
			{	/* Keep going if we are closing */
				/* a filter with a sub-stream. */
				/* We know status == 0. */
				if ( !end || !strm->is_temp )
				  break;
			}
			status = strm->end_status;
			if ( status < 0 )
			  break;
			move_ahead(curr, prev);
			stream_compact(curr);
		}
		/* Unwind from the recursion. */
		curr->end_status = (status >= 0 ? 0 : status);
		if ( prev == 0 )
		  return status;
		move_back(curr, prev);
	}
}

/* Move as much data as possible from one buffer to another. */
/* Return 0 if the input became empty, 1 if the output became full. */
int
stream_move(stream_cursor_read *pr, stream_cursor_write *pw)
{	uint rcount = pr->limit - pr->ptr;
	uint wcount = pw->limit - pw->ptr;
	uint count;
	int status;
	if ( rcount <= wcount )
	  count = rcount, status = 0;
	else
	  count = wcount, status = 1;
	memmove(pw->ptr + 1, pr->ptr + 1, count);
	pr->ptr += count;
	pw->ptr += count;
	return status;
}

/* If possible, compact the information in a stream buffer to the bottom. */
private void
stream_compact(stream *s)
{	if ( s->cursor.r.ptr >= s->cbuf && s->end_status >= 0 )
	{	uint dist = s->cursor.r.ptr + 1 - s->cbuf;
		memmove(s->cbuf, s->cursor.r.ptr + 1,
			(uint)(s->cursor.r.limit - s->cursor.r.ptr));
		s->cursor.r.ptr = s->cbuf - 1;
		s->cursor.r.limit -= dist;		/* same as w.ptr */
		s->position += dist;
	}
}

/* ------ String streams ------ */

/* String stream procedures */
private int
  s_string_available(P2(stream *, long *)),
  s_string_read_seek(P2(stream *, long)),
  s_string_write_seek(P2(stream *, long)),
  s_string_read_process(P4(stream_state *, stream_cursor_read *,
    stream_cursor_write *, bool)),
  s_string_write_process(P4(stream_state *, stream_cursor_read *,
    stream_cursor_write *, bool));

/* Initialize a stream for reading a string. */
void
sread_string(register stream *s, const byte *ptr, uint len)
{	static const stream_procs p =
	   {	s_string_available, s_string_read_seek, s_std_read_reset,
		s_std_read_flush, s_std_null, s_string_read_process
	   };
	s_std_init(s, (byte *)ptr, len, &p, s_mode_read + s_mode_seek);
	s->cbuf_string.data = (byte *)ptr;
	s->cbuf_string.size = len;
	s->end_status = EOFC;
	s->srlimit = s->swlimit;
}
/* Return the number of available bytes when reading from a string. */
private int
s_string_available(stream *s, long *pl)
{	*pl = sbufavailable(s);
	if ( *pl == 0 )		/* EOF */
	  *pl = -1;
	return 0;
}

/* Seek in a string being read.  Return 0 if OK, ERRC if not. */
private int
s_string_read_seek(register stream *s, long pos)
{	if ( pos < 0 || pos > s->bsize ) return ERRC;
	s->srptr = s->cbuf + pos - 1;
	return 0;
}

/* Initialize a stream for writing a string. */
void
swrite_string(register stream *s, byte *ptr, uint len)
{	static const stream_procs p =
	   {	s_std_noavailable, s_string_write_seek, s_std_write_reset,
		s_std_write_flush, s_std_null, s_string_write_process
	   };
	s_std_init(s, ptr, len, &p, s_mode_write + s_mode_seek);
	s->cbuf_string.data = ptr;
	s->cbuf_string.size = len;
}

/* Seek in a string being written.  Return 0 if OK, ERRC if not. */
private int
s_string_write_seek(register stream *s, long pos)
{	if ( pos < 0 || pos > s->bsize ) return ERRC;
	s->swptr = s->cbuf + pos - 1;
	return 0;
}

/* Since we initialize the input buffer of a string read stream */
/* to contain all of the data in the string, if we are ever asked */
/* to refill the buffer, we should signal EOF. */
private int
s_string_read_process(stream_state *st, stream_cursor_read *ignore_pr,
  stream_cursor_write *pw, bool last)
{	return EOFC;
}
/* Similarly, if we are ever asked to empty the buffer, it means that */
/* there has been an overrun (unless we are closing the stream). */
private int
s_string_write_process(stream_state *st, stream_cursor_read *pr,
  stream_cursor_write *ignore_pw, bool last)
{	return (last ? EOFC : ERRC);
}

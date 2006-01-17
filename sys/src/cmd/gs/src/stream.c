/* Copyright (C) 1989, 2000, 2001 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: stream.c,v 1.26 2004/10/07 05:18:34 ray Exp $ */
/* Stream package for Ghostscript interpreter */
#include "stdio_.h"		/* includes std.h */
#include "memory_.h"
#include "gdebug.h"
#include "gpcheck.h"
#include "stream.h"
#include "strimpl.h"

/* Forward declarations */
private int sreadbuf(stream *, stream_cursor_write *);
private int swritebuf(stream *, stream_cursor_read *, bool);
private void stream_compact(stream *, bool);

/* Structure types for allocating streams. */
public_st_stream();
public_st_stream_state();	/* default */
/* GC procedures */
private 
ENUM_PTRS_WITH(stream_enum_ptrs, stream *st) return 0;
case 0:
if (st->foreign)
    ENUM_RETURN(NULL);
else if (st->cbuf_string.data != 0)
    ENUM_RETURN_STRING_PTR(stream, cbuf_string);
else
    ENUM_RETURN(st->cbuf);
ENUM_PTR3(1, stream, strm, prev, next);
ENUM_PTR(4, stream, state);
case 5: return ENUM_CONST_STRING(&st->file_name);
ENUM_PTRS_END
private RELOC_PTRS_WITH(stream_reloc_ptrs, stream *st)
{
    byte *cbuf_old = st->cbuf;

    if (cbuf_old != 0 && !st->foreign) {
	long reloc;

	if (st->cbuf_string.data != 0) {
	    RELOC_STRING_VAR(st->cbuf_string);
	    st->cbuf = st->cbuf_string.data;
	} else
	    RELOC_VAR(st->cbuf);
	reloc = cbuf_old - st->cbuf;
	/* Relocate the other buffer pointers. */
	st->srptr -= reloc;
	st->srlimit -= reloc;	/* same as swptr */
	st->swlimit -= reloc;
    }
    RELOC_VAR(st->strm);
    RELOC_VAR(st->prev);
    RELOC_VAR(st->next);
    RELOC_VAR(st->state);
    RELOC_CONST_STRING_VAR(st->file_name);
}
RELOC_PTRS_END
/* Finalize a stream by closing it. */
/* We only do this for file streams, because other kinds of streams */
/* may attempt to free storage when closing. */
private void
stream_finalize(void *vptr)
{
    stream *const st = vptr;

    if_debug2('u', "[u]%s 0x%lx\n",
	      (!s_is_valid(st) ? "already closed:" :
	       st->is_temp ? "is_temp set:" :
	       st->file == 0 ? "not file:" :
	       "closing file:"), (ulong) st);
    if (s_is_valid(st) && !st->is_temp && st->file != 0) {
	/* Prevent any attempt to free the buffer. */
	st->cbuf = 0;
	st->cbuf_string.data = 0;
	sclose(st);		/* ignore errors */
    }
}

/* Dummy template for streams that don't have a separate state. */
private const stream_template s_no_template = {
    &st_stream_state, 0, 0, 1, 1, 0
};

/* ------ Generic procedures ------ */

/* Allocate a stream and initialize it minimally. */
void
s_init(stream *s, gs_memory_t * mem)
{
    s->memory = mem;
    s->report_error = s_no_report_error;
    s->min_left = 0;
    s->error_string[0] = 0;
    s->prev = s->next = 0;	/* clean for GC */
    s->file_name.data = 0;	/* ibid. */
    s->file_name.size = 0;	
    s->close_strm = false;	/* default */
    s->close_at_eod = true;	/* default */
}
stream *
s_alloc(gs_memory_t * mem, client_name_t cname)
{
    stream *s = gs_alloc_struct(mem, stream, &st_stream, cname);

    if_debug2('s', "[s]alloc(%s) = 0x%lx\n",
	      client_name_string(cname), (ulong) s);
    if (s == 0)
	return 0;
    s_init(s, mem);
    return s;
}

/* Allocate a stream state and initialize it minimally. */
void
s_init_state(stream_state *st, const stream_template *template,
	     gs_memory_t *mem)
{
    st->template = template;
    st->memory = mem;
    st->report_error = s_no_report_error;
    st->min_left = 0;
}
stream_state *
s_alloc_state(gs_memory_t * mem, gs_memory_type_ptr_t stype,
	      client_name_t cname)
{
    stream_state *st = gs_alloc_struct(mem, stream_state, stype, cname);

    if_debug3('s', "[s]alloc_state %s(%s) = 0x%lx\n",
	      client_name_string(cname),
	      client_name_string(stype->sname),
	      (ulong) st);
    if (st)
	s_init_state(st, NULL, mem);
    return st;
}

/* Standard stream initialization */
void
s_std_init(register stream * s, byte * ptr, uint len, const stream_procs * pp,
	   int modes)
{
    s->template = &s_no_template;
    s->cbuf = ptr;
    s->srptr = s->srlimit = s->swptr = ptr - 1;
    s->swlimit = ptr - 1 + len;
    s->end_status = 0;
    s->foreign = 0;
    s->modes = modes;
    s->cbuf_string.data = 0;
    s->position = 0;
    s->bsize = s->cbsize = len;
    s->strm = 0;		/* not a filter */
    s->is_temp = 0;
    s->procs = *pp;
    s->state = (stream_state *) s;	/* hack to avoid separate state */
    s->file = 0;
    s->file_name.data = 0;	/* in case stream is on stack */
    s->file_name.size = 0;
    if_debug4('s', "[s]init 0x%lx, buf=0x%lx, len=%u, modes=%d\n",
	      (ulong) s, (ulong) ptr, len, modes);
}


/* Set the file name of a stream, copying the name. */
/* Return <0 if the copy could not be allocated. */
int
ssetfilename(stream *s, const byte *data, uint size)
{
    byte *str =
	(s->file_name.data == 0 ?
	 gs_alloc_string(s->memory, size + 1, "ssetfilename") :
	 gs_resize_string(s->memory,
			  (byte *)s->file_name.data,	/* break const */
			  s->file_name.size,
			  size + 1, "ssetfilename"));

    if (str == 0)
	return -1;
    memcpy(str, data, size);
    str[size] = 0;
    s->file_name.data = str;
    s->file_name.size = size + 1;
    return 0;
}

/* Return the file name of a stream, if any. */
/* There is a guaranteed 0 byte after the string. */
int
sfilename(stream *s, gs_const_string *pfname)
{
    pfname->data = s->file_name.data;
    if (pfname->data == 0) {
	pfname->size = 0;
	return -1;
    }
    pfname->size = s->file_name.size - 1; /* omit terminator */
    return 0;
}

/* Implement a stream procedure as a no-op. */
int
s_std_null(stream * s)
{
    return 0;
}

/* Discard the contents of the buffer when reading. */
void
s_std_read_reset(stream * s)
{
    s->srptr = s->srlimit = s->cbuf - 1;
}

/* Discard the contents of the buffer when writing. */
void
s_std_write_reset(stream * s)
{
    s->swptr = s->cbuf - 1;
}

/* Flush data to end-of-file when reading. */
int
s_std_read_flush(stream * s)
{
    while (1) {
	s->srptr = s->srlimit = s->cbuf - 1;
	if (s->end_status)
	    break;
	s_process_read_buf(s);
    }
    return (s->end_status == EOFC ? 0 : s->end_status);
}

/* Flush buffered data when writing. */
int
s_std_write_flush(stream * s)
{
    return s_process_write_buf(s, false);
}

/* Indicate that the number of available input bytes is unknown. */
int
s_std_noavailable(stream * s, long *pl)
{
    *pl = -1;
    return 0;
}

/* Indicate an error when asked to seek. */
int
s_std_noseek(stream * s, long pos)
{
    return ERRC;
}

/* Standard stream closing. */
int
s_std_close(stream * s)
{
    return 0;
}

/* Standard stream mode switching. */
int
s_std_switch_mode(stream * s, bool writing)
{
    return ERRC;
}

/* Standard stream finalization.  Disable the stream. */
void
s_disable(register stream * s)
{
    s->cbuf = 0;
    s->bsize = 0;
    s->end_status = EOFC;
    s->modes = 0;
    s->cbuf_string.data = 0;
    /* The pointers in the next two statements should really be */
    /* initialized to ([const] byte *)0 - 1, but some very picky */
    /* compilers complain about this. */
    s->cursor.r.ptr = s->cursor.r.limit = 0;
    s->cursor.w.limit = 0;
    s->procs.close = s_std_null;
    /* Clear pointers for GC */
    s->strm = 0;
    s->state = (stream_state *) s;
    s->template = &s_no_template;
    /* Free the file name. */
    if (s->file_name.data) {
	gs_free_const_string(s->memory, s->file_name.data, s->file_name.size,
			     "s_disable(file_name)");
	s->file_name.data = 0;
	s->file_name.size = 0;
    }
    /****** SHOULD DO MORE THAN THIS ******/
    if_debug1('s', "[s]disable 0x%lx\n", (ulong) s);
}

/* Implement flushing for encoding filters. */
int
s_filter_write_flush(register stream * s)
{
    int status = s_process_write_buf(s, false);

    if (status != 0)
	return status;
    return sflush(s->strm);
}

/* Close a filter.  If this is an encoding filter, flush it first. */
/* If CloseTarget was specified (close_strm), then propagate the sclose */
int
s_filter_close(register stream * s)
{
    int status;
    bool close = s->close_strm;
    stream *stemp = s->strm;

    if (s_is_writing(s)) {
	int status = s_process_write_buf(s, true);

	if (status != 0 && status != EOFC)
	    return status;
        status = sflush(stemp);
	if (status != 0 && status != EOFC)
	    return status;
    }
    status = s_std_close(s);
    if (status != 0 && status != EOFC)
	return status;
    if (close && stemp != 0)
	return sclose(stemp);
    return status;
}

/* Disregard a stream error message. */
int
s_no_report_error(stream_state * st, const char *str)
{
    return 0;
}

/* Generic procedure structures for filters. */

const stream_procs s_filter_read_procs = {
    s_std_noavailable, s_std_noseek, s_std_read_reset,
    s_std_read_flush, s_filter_close
};

const stream_procs s_filter_write_procs = {
    s_std_noavailable, s_std_noseek, s_std_write_reset,
    s_filter_write_flush, s_filter_close
};

/* ------ Implementation-independent procedures ------ */

/* Store the amount of available data in a(n input) stream. */
int
savailable(stream * s, long *pl)
{
    return (*(s)->procs.available) (s, pl);
}

/* Return the current position of a stream. */
long
stell(stream * s)
{
    /*
     * The stream might have been closed, but the position
     * is still meaningful in this case.
     */
    const byte *ptr = (s_is_writing(s) ? s->swptr : s->srptr);

    return (ptr == 0 ? 0 : ptr + 1 - s->cbuf) + s->position;
}

/* Set the position of a stream. */
int
spseek(stream * s, long pos)
{
    if_debug3('s', "[s]seek 0x%lx to %ld, position was %ld\n",
	      (ulong) s, pos, stell(s));
    return (*(s)->procs.seek) (s, pos);
}

/* Switch a stream to read or write mode. */
/* Return 0 or ERRC. */
int
sswitch(register stream * s, bool writing)
{
    if (s->procs.switch_mode == 0)
	return ERRC;
    return (*s->procs.switch_mode) (s, writing);
}

/* Close a stream, disabling it if successful. */
/* (The stream may already be closed.) */
int
sclose(register stream * s)
{
    stream_state *st;
    int status = (*s->procs.close) (s);

    if (status < 0)
	return status;
    st = s->state;
    if (st != 0) {
	stream_proc_release((*release)) = st->template->release;
	if (release != 0)
	    (*release) (st);
	if (st != (stream_state *) s && st->memory != 0)
	    gs_free_object(st->memory, st, "s_std_close");
	s->state = (stream_state *) s;
    }
    s_disable(s);
    return status;
}

/*
 * Implement sgetc when the buffer may be empty.  If the buffer really is
 * empty, refill it and then read a byte.  Note that filters must read one
 * byte ahead, so that they can close immediately after the client reads the
 * last data byte if the next thing is an EOD.
 */
int
spgetcc(register stream * s, bool close_at_eod)
{
    int status, left;
    int min_left = sbuf_min_left(s);

    while (status = s->end_status,
	   left = s->srlimit - s->srptr,
	   left <= min_left && status >= 0
	)
	s_process_read_buf(s);
    if (left <= min_left &&
	(left == 0 || (status != EOFC && status != ERRC))
	) {
	/* Compact the stream so stell will return the right result. */
	stream_compact(s, true);
	if (status == EOFC && close_at_eod && s->close_at_eod) {
	    status = sclose(s);
	    if (status == 0)
		status = EOFC;
	    s->end_status = status;
	}
	return status;
    }
    return *++(s->srptr);
}

/* Implementing sputc when the buffer is full, */
/* by flushing the buffer and then writing the byte. */
int
spputc(register stream * s, byte b)
{
    for (;;) {
	if (s->end_status)
	    return s->end_status;
	if (!sendwp(s)) {
	    *++(s->swptr) = b;
	    return b;
	}
	s_process_write_buf(s, false);
    }
}

/* Push back a character onto a (read) stream. */
/* The character must be the same as the last one read. */
/* Return 0 on success, ERRC on failure. */
int
sungetc(register stream * s, byte c)
{
    if (!s_is_reading(s) || s->srptr < s->cbuf || *(s->srptr) != c)
	return ERRC;
    s->srptr--;
    return 0;
}

/* Get a string from a stream. */
/* Return 0 if the string was filled, or an exception status. */
int
sgets(stream * s, byte * buf, uint nmax, uint * pn)
{
    stream_cursor_write cw;
    int status = 0;
    int min_left = sbuf_min_left(s);

    cw.ptr = buf - 1;
    cw.limit = cw.ptr + nmax;
    while (cw.ptr < cw.limit) {
	int left;

	if ((left = s->srlimit - s->srptr) > min_left) {
	    s->srlimit -= min_left;
	    stream_move(&s->cursor.r, &cw);
	    s->srlimit += min_left;
	} else {
	    uint wanted = cw.limit - cw.ptr;
	    int c;
	    stream_state *st;

	    if (wanted >= s->bsize >> 2 &&
		(st = s->state) != 0 &&
		wanted >= st->template->min_out_size &&
		s->end_status == 0 &&
		left == 0
		) {
		byte *wptr = cw.ptr;

		cw.limit -= min_left;
		status = sreadbuf(s, &cw);
		cw.limit += min_left;
		/* Compact the stream so stell will return the right result. */
		stream_compact(s, true);
		/*
		 * We know the stream buffer is empty, so it's safe to
		 * update position.  However, we need to reset the read
		 * cursor to indicate that there is no data in the buffer.
		 */
		s->srptr = s->srlimit = s->cbuf - 1;
		s->position += cw.ptr - wptr;
		if (status != 1 || cw.ptr == cw.limit)
		    break;
	    }
	    c = spgetc(s);
	    if (c < 0) {
		status = c;
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
sputs(register stream * s, const byte * str, uint wlen, uint * pn)
{
    uint len = wlen;
    int status = s->end_status;

    if (status >= 0)
	while (len > 0) {
	    uint count = s->swlimit - s->swptr;

	    if (count > 0) {
		if (count > len)
		    count = len;
		memcpy(s->swptr + 1, str, count);
		s->swptr += count;
		str += count;
		len -= count;
	    } else {
		byte ch = *str++;

		status = sputc(s, ch);
		if (status < 0)
		    break;
		len--;
	    }
	}
    *pn = wlen - len;
    return (status >= 0 ? 0 : status);
}

/* Skip ahead a specified distance in a read stream. */
/* Return 0 or an exception status. */
/* Store the number of bytes skipped in *pskipped. */
int
spskip(register stream * s, long nskip, long *pskipped)
{
    long n = nskip;
    int min_left;

    if (nskip < 0 || !s_is_reading(s)) {
	*pskipped = 0;
	return ERRC;
    }
    if (s_can_seek(s)) {
	long pos = stell(s);
	int status = sseek(s, pos + n);

	*pskipped = stell(s) - pos;
	return status;
    }
    min_left = sbuf_min_left(s);
    while (sbufavailable(s) < n + min_left) {
	int status;

	n -= sbufavailable(s);
	s->srptr = s->srlimit;
	if (s->end_status) {
	    *pskipped = nskip - n;
	    return s->end_status;
	}
	status = sgetc(s);
	if (status < 0) {
	    *pskipped = nskip - n;
	    return status;
	}
	--n;
    }
    /* Note that if min_left > 0, n < 0 is possible; this is harmless. */
    s->srptr += n;
    *pskipped = nskip;
    return 0;
}

/* Read a line from a stream.  See srdline.h for the specification. */
int
sreadline(stream *s_in, stream *s_out, void *readline_data,
	  gs_const_string *prompt, gs_string * buf,
	  gs_memory_t * bufmem, uint * pcount, bool *pin_eol,
	  bool (*is_stdin)(const stream *))
{
    uint count = *pcount;

    /* Most systems define \n as 0xa and \r as 0xd; however, */
    /* OS-9 has \n == \r == 0xd and \l == 0xa.  The following */
    /* code works properly regardless of environment. */
#if '\n' == '\r'
#  define LF 0xa
#else
#  define LF '\n'
#endif

    if (count == 0 && s_out && prompt) {
	uint ignore_n;
	int ch = sputs(s_out, prompt->data, prompt->size, &ignore_n);

	if (ch < 0)
	    return ch;
    }

top:
    if (*pin_eol) {
	/*
	 * We're in the middle of checking for a two-character
	 * end-of-line sequence.  If we get an EOF here, stop, but
	 * don't signal EOF now; wait till the next read.
	 */
	int ch = spgetcc(s_in, false);

	if (ch == EOFC) {
	    *pin_eol = false;
	    return 0;
	} else if (ch < 0)
	    return ch;
	else if (ch != LF)
	    sputback(s_in);
	*pin_eol = false;
	return 0;
    }
    for (;;) {
	int ch = sgetc(s_in);

	if (ch < 0) {		/* EOF or exception */
	    *pcount = count;
	    return ch;
	}
	switch (ch) {
	    case '\r':
		{
#if '\n' == '\r'		/* OS-9 or similar */
		    if (!is_stdin(s_in))
#endif
		    {
			*pcount = count;
			*pin_eol = true;
			goto top;
		    }
		}
		/* falls through */
	    case LF:
#undef LF
		*pcount = count;
		return 0;
	}
	if (count >= buf->size) {	/* filled the string */
	    if (!bufmem) {
		sputback(s_in);
		*pcount = count;
		return 1;
	    }
	    {
		uint nsize = count + max(count, 20);
		byte *ndata = gs_resize_string(bufmem, buf->data, buf->size,
					       nsize, "sreadline(buffer)");

		if (ndata == 0)
		    return ERRC; /* no better choice */
		buf->data = ndata;
		buf->size = nsize;
	    }
	}
	buf->data[count++] = ch;
    }
    /*return 0; *//* not reached */
}

/* ------ Utilities ------ */

/*
 * Attempt to refill the buffer of a read stream.  Only call this if the
 * end_status is not EOFC, and if the buffer is (nearly) empty.
 */
int
s_process_read_buf(stream * s)
{
    int status;

    stream_compact(s, false);
    status = sreadbuf(s, &s->cursor.w);
    s->end_status = (status >= 0 ? 0 : status);
    return 0;
}

/*
 * Attempt to empty the buffer of a write stream.  Only call this if the
 * end_status is not EOFC.
 */
int
s_process_write_buf(stream * s, bool last)
{
    int status = swritebuf(s, &s->cursor.r, last);

    stream_compact(s, false);
    return (status >= 0 ? 0 : status);
}

/* Move forward or backward in a pipeline.  We temporarily reverse */
/* the direction of the pointers while doing this. */
/* (Cf the Deutsch-Schorr-Waite graph marking algorithm.) */
#define MOVE_BACK(curr, prev)\
  BEGIN\
    stream *back = prev->strm;\
    prev->strm = curr; curr = prev; prev = back;\
  END
#define MOVE_AHEAD(curr, prev)\
  BEGIN\
    stream *ahead = curr->strm;\
    curr->strm = prev; prev = curr; curr = ahead;\
  END

/*
 * Read from a stream pipeline.  Update end_status for all streams that were
 * actually touched.  Return the status from the outermost stream: this is
 * normally the same as s->end_status, except that if s->procs.process
 * returned 1, sreadbuf sets s->end_status to 0, but returns 1.
 */
private int
sreadbuf(stream * s, stream_cursor_write * pbuf)
{
    stream *prev = 0;
    stream *curr = s;
    int status;

    for (;;) {
	stream *strm;
	stream_cursor_write *pw;
	byte *oldpos;

	for (;;) {		/* Descend into the recursion. */
	    stream_cursor_read cr;
	    stream_cursor_read *pr;
	    int left;
	    bool eof;

	    strm = curr->strm;
	    if (strm == 0) {
		cr.ptr = 0, cr.limit = 0;
		pr = &cr;
		left = 0;
		eof = false;
	    } else {
		pr = &strm->cursor.r;
		left = sbuf_min_left(strm);
		left = min(left, pr->limit - pr->ptr);
		pr->limit -= left;
		eof = strm->end_status == EOFC;
	    }
	    pw = (prev == 0 ? pbuf : &curr->cursor.w);
	    if_debug4('s', "[s]read process 0x%lx, nr=%u, nw=%u, eof=%d\n",
		      (ulong) curr, (uint) (pr->limit - pr->ptr),
		      (uint) (pw->limit - pw->ptr), eof);
	    oldpos = pw->ptr;
	    status = (*curr->procs.process) (curr->state, pr, pw, eof);
	    pr->limit += left;
	    if_debug5('s', "[s]after read 0x%lx, nr=%u, nw=%u, status=%d, position=%d\n",
		      (ulong) curr, (uint) (pr->limit - pr->ptr),
		      (uint) (pw->limit - pw->ptr), status, s->position);
	    if (strm == 0 || status != 0)
		break;
	    if (strm->end_status < 0) {
		if (strm->end_status != EOFC || pw->ptr == oldpos)
		    status = strm->end_status;
		break;
	    }
	    MOVE_AHEAD(curr, prev);
	    stream_compact(curr, false);
	}
	/* If curr reached EOD and is a filter or file stream, close it. */
	/* (see PLRM 3rd, sec 3.8.2, p80) */
	if ((strm != 0 || curr->file) && status == EOFC &&
	    curr->cursor.r.ptr >= curr->cursor.r.limit &&
	    curr->close_at_eod
	    ) {
	    int cstat = sclose(curr);

	    if (cstat != 0)
		status = cstat;
	}
	/* Unwind from the recursion. */
	curr->end_status = (status >= 0 ? 0 : status);
	if (prev == 0)
	    return status;
	MOVE_BACK(curr, prev);
    }
}

/* Write to a pipeline. */
private int
swritebuf(stream * s, stream_cursor_read * pbuf, bool last)
{
    stream *prev = 0;
    stream *curr = s;
    int depth = 0;		/* # of non-temp streams before curr */
    int status;

    /*
     * The handling of EOFC is a little tricky.  There are two
     * invariants that keep it straight:
     *      - We only pass last = true to a stream if either it is
     * the first stream in the pipeline, or it is a temporary stream
     * below the first stream and the stream immediately above it has
     * end_status = EOFC.
     */
    for (;;) {
	for (;;) {
	    /* Move ahead in the pipeline. */
	    stream *strm = curr->strm;
	    stream_cursor_write cw;
	    stream_cursor_read *pr;
	    stream_cursor_write *pw;

	    /*
	     * We only want to set the last/end flag for
	     * the top-level stream and any temporary streams
	     * immediately below it.
	     */
	    bool end = last &&
		(prev == 0 ||
		 (depth <= 1 && prev->end_status == EOFC));

	    if (strm == 0)
		cw.ptr = 0, cw.limit = 0, pw = &cw;
	    else
		pw = &strm->cursor.w;
	    if (prev == 0)
		pr = pbuf;
	    else
		pr = &curr->cursor.r;
	    if_debug5('s',
		      "[s]write process 0x%lx(%s), nr=%u, nw=%u, end=%d\n",
		      (ulong)curr,
		      gs_struct_type_name(curr->state->template->stype),
		      (uint)(pr->limit - pr->ptr),
		      (uint)(pw->limit - pw->ptr), end);
	    status = curr->end_status;
	    if (status >= 0) {
		status = (*curr->procs.process)(curr->state, pr, pw, end);
		if_debug5('s',
			  "[s]after write 0x%lx, nr=%u, nw=%u, end=%d, status=%d\n",
			  (ulong) curr, (uint) (pr->limit - pr->ptr),
			  (uint) (pw->limit - pw->ptr), end, status);
		if (status == 0 && end)
		    status = EOFC;
		if (status == EOFC || status == ERRC)
		    curr->end_status = status;
	    }
	    if (strm == 0 || (status < 0 && status != EOFC))
		break;
	    if (status != 1) {
		/*
		 * Keep going if we are closing a filter with a sub-stream.
		 * We know status == 0 or EOFC.
		 */
		if (!end || !strm->is_temp)
		    break;
	    }
	    status = strm->end_status;
	    if (status < 0)
		break;
	    if (!curr->is_temp)
		++depth;
	    if_debug1('s', "[s]moving ahead, depth = %d\n", depth);
	    MOVE_AHEAD(curr, prev);
	    stream_compact(curr, false);
	}
	/* Move back in the pipeline. */
	curr->end_status = (status >= 0 ? 0 : status);
	if (status < 0 || prev == 0) {
	    /*
	     * All streams up to here were called with last = true
	     * and returned 0 or EOFC (so their end_status is now EOFC):
	     * finish unwinding and then return.  Change the status of
	     * the prior streams to ERRC if the new status is ERRC,
	     * otherwise leave it alone.
	     */
	    while (prev) {
		if_debug0('s', "[s]unwinding\n");
		MOVE_BACK(curr, prev);
		if (status >= 0)
		    curr->end_status = 0;
		else if (status == ERRC)
		    curr->end_status = ERRC;
	    }
	    return status;
	}
	MOVE_BACK(curr, prev);
	if (!curr->is_temp)
	    --depth;
	if_debug1('s', "[s]moving back, depth = %d\n", depth);
    }
}

/* Move as much data as possible from one buffer to another. */
/* Return 0 if the input became empty, 1 if the output became full. */
int
stream_move(stream_cursor_read * pr, stream_cursor_write * pw)
{
    uint rcount = pr->limit - pr->ptr;
    uint wcount = pw->limit - pw->ptr;
    uint count;
    int status;

    if (rcount <= wcount)
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
stream_compact(stream * s, bool always)
{
    if (s->cursor.r.ptr >= s->cbuf && (always || s->end_status >= 0)) {
	uint dist = s->cursor.r.ptr + 1 - s->cbuf;

	memmove(s->cbuf, s->cursor.r.ptr + 1,
		(uint) (s->cursor.r.limit - s->cursor.r.ptr));
	s->cursor.r.ptr = s->cbuf - 1;
	s->cursor.r.limit -= dist;	/* same as w.ptr */
	s->position += dist;
    }
}

/* ------ String streams ------ */

/* String stream procedures */
private int
    s_string_available(stream *, long *),
    s_string_read_seek(stream *, long),
    s_string_write_seek(stream *, long),
    s_string_read_process(stream_state *, stream_cursor_read *,
			  stream_cursor_write *, bool),
    s_string_write_process(stream_state *, stream_cursor_read *,
			   stream_cursor_write *, bool);

/* Initialize a stream for reading a string. */
void
sread_string(register stream *s, const byte *ptr, uint len)
{
    static const stream_procs p = {
	 s_string_available, s_string_read_seek, s_std_read_reset,
	 s_std_read_flush, s_std_null, s_string_read_process
    };

    s_std_init(s, (byte *)ptr, len, &p, s_mode_read + s_mode_seek);
    s->cbuf_string.data = (byte *)ptr;
    s->cbuf_string.size = len;
    s->end_status = EOFC;
    s->srlimit = s->swlimit;
}
/* Initialize a reusable stream for reading a string. */
private void
s_string_reusable_reset(stream *s)
{
    s->srptr = s->cbuf - 1;	/* just reset to the beginning */
    s->srlimit = s->srptr + s->bsize;  /* might have gotten reset */
}
private int
s_string_reusable_flush(stream *s)
{
    s->srptr = s->srlimit = s->cbuf + s->bsize - 1;  /* just set to the end */
    return 0;
}
void
sread_string_reusable(stream *s, const byte *ptr, uint len)
{
    static const stream_procs p = {
	 s_string_available, s_string_read_seek, s_string_reusable_reset,
	 s_string_reusable_flush, s_std_null, s_string_read_process
    };

    sread_string(s, ptr, len);
    s->procs = p;
    s->close_at_eod = false;
}

/* Return the number of available bytes when reading from a string. */
private int
s_string_available(stream *s, long *pl)
{
    *pl = sbufavailable(s);
    if (*pl == 0 && s->close_at_eod)	/* EOF */
	*pl = -1;
    return 0;
}

/* Seek in a string being read.  Return 0 if OK, ERRC if not. */
private int
s_string_read_seek(register stream * s, long pos)
{
    if (pos < 0 || pos > s->bsize)
	return ERRC;
    s->srptr = s->cbuf + pos - 1;
    /* We might be seeking after a reusable string reached EOF. */
    s->srlimit = s->cbuf + s->bsize - 1;
    /* 
     * When the file reaches EOF,
     * stream_compact sets s->position to its end. 
     * Reset it now to allow stell to work properly
     * after calls to this function.
     * Note that if the riched EOF and this fuction
     * was not called, stell still returns a wrong value.
     */
    s->position = 0;
    return 0;
}

/* Initialize a stream for writing a string. */
void
swrite_string(register stream * s, byte * ptr, uint len)
{
    static const stream_procs p = {
	s_std_noavailable, s_string_write_seek, s_std_write_reset,
	s_std_null, s_std_null, s_string_write_process
    };

    s_std_init(s, ptr, len, &p, s_mode_write + s_mode_seek);
    s->cbuf_string.data = ptr;
    s->cbuf_string.size = len;
}

/* Seek in a string being written.  Return 0 if OK, ERRC if not. */
private int
s_string_write_seek(register stream * s, long pos)
{
    if (pos < 0 || pos > s->bsize)
	return ERRC;
    s->swptr = s->cbuf + pos - 1;
    return 0;
}

/* Since we initialize the input buffer of a string read stream */
/* to contain all of the data in the string, if we are ever asked */
/* to refill the buffer, we should signal EOF. */
private int
s_string_read_process(stream_state * st, stream_cursor_read * ignore_pr,
		      stream_cursor_write * pw, bool last)
{
    return EOFC;
}
/* Similarly, if we are ever asked to empty the buffer, it means that */
/* there has been an overrun (unless we are closing the stream). */
private int
s_string_write_process(stream_state * st, stream_cursor_read * pr,
		       stream_cursor_write * ignore_pw, bool last)
{
    return (last ? EOFC : ERRC);
}

/* ------ Position-tracking stream ------ */

private int
    s_write_position_process(stream_state *, stream_cursor_read *,
			     stream_cursor_write *, bool);

/* Set up a write stream that just keeps track of the position. */
void
swrite_position_only(stream *s)
{
    static byte discard_buf[50];	/* size is arbitrary */

    swrite_string(s, discard_buf, sizeof(discard_buf));
    s->procs.process = s_write_position_process;
}

private int
s_write_position_process(stream_state * st, stream_cursor_read * pr,
			 stream_cursor_write * ignore_pw, bool last)
{
    pr->ptr = pr->limit;	/* discard data */
    return 0;
}

/* ------ Filter pipelines ------ */

/*
 * Add a filter to an output pipeline.  The client must have allocated the
 * stream state, if any, using the given allocator.  For s_init_filter, the
 * client must have called s_init and s_init_state.
 */
int
s_init_filter(stream *fs, stream_state *fss, byte *buf, uint bsize,
	      stream *target)
{
    const stream_template *template = fss->template;

    if (bsize < template->min_in_size)
	return ERRC;
    s_std_init(fs, buf, bsize, &s_filter_write_procs, s_mode_write);
    fs->procs.process = template->process;
    fs->state = fss;
    if (template->init) {
	fs->end_status = (template->init)(fss);
	if (fs->end_status < 0)
	    return fs->end_status;
    }
    fs->strm = target;
    return 0;
}
stream *
s_add_filter(stream **ps, const stream_template *template,
	     stream_state *ss, gs_memory_t *mem)
{
    stream *es;
    stream_state *ess;
    uint bsize = max(template->min_in_size, 256);	/* arbitrary */
    byte *buf;

    /*
     * Ensure enough buffering.  This may require adding an additional
     * stream.
     */
    if (bsize > (*ps)->bsize && template->process != s_NullE_template.process) {
	stream_template null_template;

	null_template = s_NullE_template;
	null_template.min_in_size = bsize;
	if (s_add_filter(ps, &null_template, NULL, mem) == 0)
	    return 0;
    }
    es = s_alloc(mem, "s_add_filter(stream)");
    buf = gs_alloc_bytes(mem, bsize, "s_add_filter(buf)");
    if (es == 0 || buf == 0) {
	gs_free_object(mem, buf, "s_add_filter(buf)");
	gs_free_object(mem, es, "s_add_filter(stream)");
	return 0;
    }
    ess = (ss == 0 ? (stream_state *)es : ss);
    ess->template = template;
    ess->memory = mem;
    es->memory = mem;
    if (s_init_filter(es, ess, buf, bsize, *ps) < 0)
	return 0;
    *ps = es;
    return es;
}

/*
 * Close the filters in a pipeline, up to a given target stream, freeing
 * their buffers and state structures.
 */
int
s_close_filters(stream **ps, stream *target)
{
    while (*ps != target) {
	stream *s = *ps;
	gs_memory_t *mem = s->state->memory;
	byte *sbuf = s->cbuf;
	stream *next = s->strm;
	int status = sclose(s);
	stream_state *ss = s->state; /* sclose may set this to s */

	if (status < 0)
	    return status;
	if (mem) {
	    gs_free_object(mem, sbuf, "s_close_filters(buf)");
	    gs_free_object(mem, s, "s_close_filters(stream)");
	    if (ss != (stream_state *)s)
		gs_free_object(mem, ss, "s_close_filters(state)");
	}
	*ps = next;
    }
    return 0;
}

/* ------ NullEncode/Decode ------ */

/* Process a buffer */
private int
s_Null_process(stream_state * st, stream_cursor_read * pr,
	       stream_cursor_write * pw, bool last)
{
    return stream_move(pr, pw);
}

/* Stream template */
const stream_template s_NullE_template = {
    &st_stream_state, NULL, s_Null_process, 1, 1
};
const stream_template s_NullD_template = {
    &st_stream_state, NULL, s_Null_process, 1, 1
};

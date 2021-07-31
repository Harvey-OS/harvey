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

/* sfileno.c */
/* File stream implementation using direct OS calls */
/******
 ****** NOTE: THIS FILE PROBABLY WILL NOT COMPILE ON NON-UNIX
 ****** PLATFORMS, AND IT MAY REQUIRE EDITING ON SOME UNIX PLATFORMS.
 ******/
#include "stdio_.h"		/* includes std.h */
#include "errno_.h"
#include "memory_.h"
/*
 * It's likely that you will have to edit the next line on some Unix
 * and most non-Unix platforms, since there is no standard (ANSI or
 * otherwise) for where to find these definitions.
 */
#include <unistd.h>		/* for read, write, fsync, lseek */
#include "gdebug.h"
#include "gpcheck.h"
#include "stream.h"
#include "strimpl.h"

/*
 * This is an alternate implementation of file streams.  It still uses
 * FILE * in the interface, but it uses direct OS calls for I/O.
 * It also includes workarounds for the nasty habit of System V Unix
 * of breaking out of read and write operations with EINTR, EAGAIN,
 * and/or EWOULDBLOCK "errors".
 *
 * The interface should be identical to that of sfile.c.  However,
 * in order to allow both implementations to exist in the same executable,
 * we use different names for sread_file, swrite_file, and sappend_file
 * (the public procedures).  If you want to use this implementation
 * instead of the one in sfile.c, uncomment the following #define.
 */
#define USE_SFILENO_FOR_SFILE

#ifdef USE_SFILENO_FOR_SFILE
#  define sread_fileno sread_file
#  define swrite_fileno swrite_file
#  define sappend_fileno sappend_file
#endif

/* Forward references for file stream procedures */
private int
  s_fileno_available(P2(stream *, long *)),
  s_fileno_read_seek(P2(stream *, long)),
  s_fileno_read_close(P1(stream *)),
  s_fileno_read_process(P4(stream_state *, stream_cursor_read *,
    stream_cursor_write *, bool));
private int
  s_fileno_write_seek(P2(stream *, long)),
  s_fileno_write_flush(P1(stream *)),
  s_fileno_write_close(P1(stream *)),
  s_fileno_write_process(P4(stream_state *, stream_cursor_read *,
    stream_cursor_write *, bool));

/* ------ File streams ------ */

/* Get the file descriptor number of a stream. */
#define sfileno(s) fileno((s)->file)

/* Initialize a stream for reading an OS file. */
void
sread_fileno(register stream *s, FILE *file, byte *buf, uint len)
{	static const stream_procs p =
	   {	s_fileno_available, s_fileno_read_seek, s_std_read_reset,
		s_std_read_flush, s_fileno_read_close, s_fileno_read_process
	   };
	s_std_init(s, buf, len, &p,
		   (file == stdin ? s_mode_read : s_mode_read + s_mode_seek));
	if_debug2('s', "[s]read file=0x%lx, fd=%d\n", (ulong)file,
		  fileno(file));
	s->file = file;
	s->file_modes = s->modes;
	s->state = (stream_state *)s;	/* hack to avoid separate state */
}
/* Procedures for reading from a file */
private int
s_fileno_available(register stream *s, long *pl)
{	int fd = sfileno(s);
	*pl = sbufavailable(s);
	if ( sseekable(s) )
	   {	long pos, end;
		pos = lseek(fd, 0L, SEEK_CUR);
		if ( pos < 0 )
		  return ERRC;
		end = lseek(fd, 0L, SEEK_END);
		if ( lseek(fd, pos, SEEK_SET) < 0 || end < 0 )
		  return ERRC;
		*pl += end - pos;
		if ( *pl == 0 ) *pl = -1;	/* EOF */
	   }
	else
	   {	if ( *pl == 0 ) *pl = -1;	/* EOF */
	   }
	return 0;
}
private int
s_fileno_read_seek(register stream *s, long pos)
{	uint end = s->srlimit - s->cbuf + 1;
	long offset = pos - s->position;
	if ( offset >= 0 && offset <= end )
	   {	/* Staying within the same buffer */
		s->srptr = s->cbuf + offset - 1;
		return 0;
	   }
	if ( lseek(sfileno(s), pos, SEEK_SET) < 0 )
	  return ERRC;
	s->srptr = s->srlimit = s->cbuf - 1;
	s->end_status = 0;
	s->position = pos;
	return 0;
}
private int
s_fileno_read_close(stream *s)
{	return fclose(s->file);
}

/* Initialize a stream for writing an OS file. */
void
swrite_fileno(register stream *s, FILE *file, byte *buf, uint len)
{	static const stream_procs p =
	   {	s_std_noavailable, s_fileno_write_seek, s_std_write_reset,
		s_fileno_write_flush, s_fileno_write_close, s_fileno_write_process
	   };
	s_std_init(s, buf, len, &p,
		   (file == stdout ? s_mode_write : s_mode_write + s_mode_seek));
	if_debug1('s', "[s]write file=0x%lx\n", (ulong)file);
	s->file = file;
	s->file_modes = s->modes;
	s->state = (stream_state *)s;	/* hack to avoid separate state */
}
/* Initialize for appending to an OS file. */
void
sappend_fileno(register stream *s, FILE *file, byte *buf, uint len)
{	swrite_fileno(s, file, buf, len);
	s->modes = s_mode_write + s_mode_append;	/* no seek */
	s->file_modes = s->modes;
	s->position = lseek(fileno(file), 0L, SEEK_END);
}
/* Procedures for writing on a file */
private int
s_fileno_write_seek(stream *s, long pos)
{	/* We must flush the buffer to reposition. */
	int code = sflush(s);
	if ( code < 0 )
	  return code;
	if ( lseek(sfileno(s), pos, SEEK_SET) < 0 )
	  return ERRC;
	s->position = pos;
	return 0;
}
private int
s_fileno_write_flush(register stream *s)
{	int result = s_process_write_buf(s, false);
	discard(fsync(sfileno(s)));
	return result;
}
private int
s_fileno_write_close(register stream *s)
{	s_process_write_buf(s, true);
	return fclose(s->file);
}

#define ss ((stream *)st)

/* Process a buffer for a file reading stream. */
/* This is the first stream in the pipeline, so pr is irrelevant. */
private int
s_fileno_read_process(stream_state *st, stream_cursor_read *ignore_pr,
  stream_cursor_write *pw, bool last)
{	int nread, status;
again:	nread = read(sfileno(ss), pw->ptr + 1, (uint)(pw->limit - pw->ptr));
	if ( nread > 0 )
	  {	pw->ptr += nread;
		status = 0;
	  }
	else if ( nread == 0 )
	  status = EOFC;
	else switch ( errno )
	  {
		/* Handle System V interrupts */
#ifdef EINTR
	  case EINTR:
#endif
#if defined(EAGAIN) && (EAGAIN != EINTR)
	  case EAGAIN:
#endif
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN) && (EWOULDBLOCK != EINTR)
	  case EWOULDBLOCK:
#endif
#if defined(EINTR) || defined(EAGAIN) || defined(EWOULDBLOCK)
	    goto again;
#endif
	  default:
	    status = ERRC;
	  }
	process_interrupts();
	return status;
}

/* Process a buffer for a file writing stream. */
/* This is the last stream in the pipeline, so pw is irrelevant. */
private int
s_fileno_write_process(stream_state *st, stream_cursor_read *pr,
  stream_cursor_write *ignore_pw, bool last)
{	int nwrite, status;
	uint count;
again:	count = pr->limit - pr->ptr;
	/* Some verstions of the DEC C library on AXP architectures */
	/* give an error on write if the count is zero! */
	if ( count == 0 )
	  {	process_interrupts();
		return 0;
	  }
	nwrite = write(sfileno(ss), pr->ptr + 1, count);
	if ( nwrite >= 0 )
	  {	pr->ptr += nwrite;
		status = 0;
	  }
	else switch ( errno )
	  {
		/* Handle System V interrupts */
#ifdef EINTR
	  case EINTR:
#endif
#if defined(EAGAIN) && (EAGAIN != EINTR)
	  case EAGAIN:
#endif
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN) && (EWOULDBLOCK != EINTR)
	  case EWOULDBLOCK:
#endif
#if defined(EINTR) || defined(EAGAIN) || defined(EWOULDBLOCK)
	    goto again;
#endif
	  default:
	    status = ERRC;
	  }
	process_interrupts();
	return status;
}

#undef ss

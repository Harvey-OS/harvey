/* Copyright (C) 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* sfile.c */
/* File stream implementation */
#include "stdio_.h"		/* includes std.h */
#include "memory_.h"
#include "gdebug.h"
#include "gpcheck.h"
#include "stream.h"
#include "strimpl.h"

/* Forward references for file stream procedures */
private int
  s_file_available(P2(stream *, long *)),
  s_file_read_seek(P2(stream *, long)),
  s_file_read_close(P1(stream *)),
  s_file_read_process(P4(stream_state *, stream_cursor_read *,
    stream_cursor_write *, bool));
private int
  s_file_write_seek(P2(stream *, long)),
  s_file_write_flush(P1(stream *)),
  s_file_write_close(P1(stream *)),
  s_file_write_process(P4(stream_state *, stream_cursor_read *,
    stream_cursor_write *, bool));

/* ------ File streams ------ */

/* Initialize a stream for reading an OS file. */
void
sread_file(register stream *s, FILE *file, byte *buf, uint len)
{	static const stream_procs p =
	   {	s_file_available, s_file_read_seek, s_std_read_reset,
		s_std_read_flush, s_file_read_close, s_file_read_process
	   };
	s_std_init(s, buf, len, &p,
		   (file == stdin ? s_mode_read : s_mode_read + s_mode_seek));
	if_debug1('s', "[s]read file=0x%lx\n", (ulong)file);
	s->file = file;
	s->file_modes = s->modes;
	s->state = (stream_state *)s;	/* hack to avoid separate state */
}
/* Procedures for reading from a file */
private int
s_file_available(register stream *s, long *pl)
{	*pl = sbufavailable(s);
	if ( sseekable(s) )
	   {	long pos, end;
		pos = ftell(s->file);
		if ( fseek(s->file, 0L, SEEK_END) ) return ERRC;
		end = ftell(s->file);
		if ( fseek(s->file, pos, SEEK_SET) ) return ERRC;
		*pl += end - pos;
		if ( *pl == 0 ) *pl = -1;	/* EOF */
	   }
	else
	   {	if ( *pl == 0 && feof(s->file) ) *pl = -1;	/* EOF */
	   }
	return 0;
}
private int
s_file_read_seek(register stream *s, long pos)
{	uint end = s->srlimit - s->cbuf + 1;
	long offset = pos - s->position;
	if ( offset >= 0 && offset <= end )
	   {	/* Staying within the same buffer */
		s->srptr = s->cbuf + offset - 1;
		return 0;
	   }
	if ( fseek(s->file, pos, SEEK_SET) != 0 )
		return ERRC;
	s->srptr = s->srlimit = s->cbuf - 1;
	s->end_status = 0;
	s->position = pos;
	return 0;
}
private int
s_file_read_close(stream *s)
{	return fclose(s->file);
}

/* Initialize a stream for writing an OS file. */
void
swrite_file(register stream *s, FILE *file, byte *buf, uint len)
{	static const stream_procs p =
	   {	s_std_noavailable, s_file_write_seek, s_std_write_reset,
		s_file_write_flush, s_file_write_close, s_file_write_process
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
sappend_file(register stream *s, FILE *file, byte *buf, uint len)
{	swrite_file(s, file, buf, len);
	s->modes = s_mode_write + s_mode_append;	/* no seek */
	s->file_modes = s->modes;
	fseek(file, 0L, SEEK_END);
	s->position = ftell(file);
}
/* Procedures for writing on a file */
private int
s_file_write_seek(stream *s, long pos)
{	/* We must flush the buffer to reposition. */
	int code = sflush(s);
	if ( code < 0 )
		return code;
	if ( fseek(s->file, pos, SEEK_SET) != 0 )
		return ERRC;
	s->position = pos;
	return 0;
}
private int
s_file_write_flush(register stream *s)
{	int result = s_process_write_buf(s, false);
	fflush(s->file);
	return result;
}
private int
s_file_write_close(register stream *s)
{	s_process_write_buf(s, true);
	return fclose(s->file);
}

#define ss ((stream *)st)

/* Process a buffer for a file reading stream. */
/* This is the first stream in the pipeline, so pr is irrelevant. */
private int
s_file_read_process(stream_state *st, stream_cursor_read *ignore_pr,
  stream_cursor_write *pw, bool last)
{	FILE *file = ss->file;
	int count = fread(pw->ptr + 1, 1, (uint)(pw->limit - pw->ptr), file);
	if ( count < 0 )
		count = 0;
	pw->ptr += count;
	process_interrupts();
	return (ferror(file) ? ERRC : feof(file) ? EOFC : 1);
}

/* Process a buffer for a file writing stream. */
/* This is the last stream in the pipeline, so pw is irrelevant. */
private int
s_file_write_process(stream_state *st, stream_cursor_read *pr,
  stream_cursor_write *ignore_pw, bool last)
{	/* The DEC C library on AXP architectures gives an error on */
	/* fwrite if the count is zero! */
	uint count = pr->limit - pr->ptr;
	if ( count != 0 )
	  {	FILE *file = ss->file;
		int written = fwrite(pr->ptr + 1, 1, count, file);
		if ( written < 0 )
		  written = 0;
		pr->ptr += written;
		process_interrupts();
		return (ferror(file) ? ERRC : 0);
	  }
	else
	  {	process_interrupts();
		return 0;
	  }
}

#undef ss

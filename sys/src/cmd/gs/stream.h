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

/* rob Dec 94: stdio.h defines streams, so we dance around it */
#ifdef Plan9
#define	sclose	GS_sclose
#define	sopenr	GS_sopenr
#define	sopenw	GS_sopenw
#endif


/* stream.h */
/* Definitions for Ghostscript stream package */
/* Requires stdio.h */
#include "scommon.h"

/* See scommon.h for documentation on the design of streams. */

/* ------ Stream structure definition ------ */

/*
 * We expose the stream structure definition to clients so that
 * they can get reasonable performance out of the basic operations.
 */

/* Define the "virtual" stream procedures. */

typedef struct {

		/* Store # available for reading. */
		/* Return 0 if OK, ERRC if error or not implemented. */
#define stream_proc_available(proc)\
  int proc(P2(stream *, long *))
		stream_proc_available((*available));

		/* Set position. */
		/* Return 0 if OK, ERRC if error or not implemented. */
#define stream_proc_seek(proc)\
  int proc(P2(stream *, long))
		stream_proc_seek((*seek));

		/* Clear buffer and, if relevant, unblock channel. */
		/* Cannot cause an error. */
#define stream_proc_reset(proc)\
  void proc(P1(stream *))
		stream_proc_reset((*reset));

		/* Flush buffered data to output, or drain input. */
		/* Return 0 if OK, ERRC if error. */
#define stream_proc_flush(proc)\
  int proc(P1(stream *))
		stream_proc_flush((*flush));

		/* Flush data (if writing) & close stream. */
		/* Return 0 if OK, ERRC if error. */
#define stream_proc_close(proc)\
  int proc(P1(stream *))
		stream_proc_close((*close));

		/* Process a buffer, updating the cursor pointers. */
		/* See strimpl.h for details. */
		stream_proc_process((*process));

} stream_procs;

/* ------ The actual stream structure ------ */

struct stream_s {
		/* To allow the stream itself to serve as the "state" */
		/* of a couple of heavily used types, we start its */
		/* definition with the common stream state. */
	stream_state_common;
	stream_cursor cursor;		/* cursor for reading/writing data */
	byte *cbuf;			/* base of buffer */
	uint bsize;			/* size of buffer, 0 if closed */
	uint cbsize;			/* size of buffer */
	short end_status;		/* status at end of buffer (when */
					/* reading) or now (when writing) */
	byte foreign;			/* true if buffer is outside heap */
	byte modes;			/* access modes allowed for this */
					/* stream */
#define s_mode_read 1
#define s_mode_write 2
#define s_mode_seek 4
#define s_mode_append 8			/* (s_mode_write also set) */
#define s_is_valid(s) ((s)->modes != 0)
#define s_is_reading(s) (((s)->modes & s_mode_read) != 0)
#define s_is_writing(s) (((s)->modes & s_mode_write) != 0)
#define s_can_seek(s) (((s)->modes & s_mode_seek) != 0)
	gs_string cbuf_string;		/* cbuf/cbsize if cbuf is a string, */
					/* 0/? if not */
	long position;			/* file position of beginning of */
					/* buffer */
	stream_procs procs;
	stream *strm;			/* the underlying stream, non-zero */
					/* iff this is a filter stream */
	int is_temp;			/* if >0, this is a temporary */
					/* stream and should be freed */
					/* when its source/sink is closed; */
					/* if >1, the buffer is also */
					/* temporary */
	ushort read_id;			/* "unique" serial # for detecting */
					/* references to closed streams */
					/* and for validating read access */
	ushort write_id;		/* ditto to validate write access */
	int inline_temp;		/* temporary for inline access */
					/* (see spgetc_inline below) */
	stream_state *state;		/* state of process */
	/*
	 * The following are for the use of the interpreter.
	 */
	stream *prev, *next;		/* keep track of all files */
	int save_count;			/* # of saves for which this stream */
					/* was the head of the file list */
	/*
	 * In order to avoid allocating a separate stream_state for
	 * file streams, which are the most heavily used stream type,
	 * we put their state here.
	 */
	FILE *file;			/* file handle for C library */
	uint file_modes;		/* access modes for the file, */
					/* may be a superset of modes */
	int (*save_close)(P1(stream *));	/* save original close proc */
};
#define private_st_stream()	/* in stream.c */\
  gs_private_st_composite(st_stream, stream, "stream",\
    stream_enum_ptrs, stream_reloc_ptrs)

/* Initialize the checking IDs of a stream. */
#define s_init_ids(s) ((s)->read_id = (s)->write_id = 1)
#define s_init_read_id(s) ((s)->read_id = 1, (s)->write_id = 0)
#define s_init_write_id(s) ((s)->read_id = 0, (s)->write_id = 1)
#define s_init_no_id(s) ((s)->read_id = (s)->write_id = 0)

/* ------ Stream functions ------ */

#define srptr cursor.r.ptr
#define srlimit cursor.r.limit
#define swptr cursor.w.ptr
#define swlimit cursor.w.limit

/* Some of these are macros -- beware. */
/* Note that unlike the C stream library, */
/* ALL stream procedures take the stream as the first argument. */
#define sendrp(s) ((s)->srptr >= (s)->srlimit)	/* NOT FOR CLIENTS */
#define sendwp(s) ((s)->swptr >= (s)->swlimit)	/* NOT FOR CLIENTS */

/* Following are valid for all streams. */
/* flush is NOT a no-op for read streams -- it discards data until EOF. */
/* close is NOT a no-op for non-file streams -- */
/* it actively disables them. */
/* The close routine must do a flush if needed. */
#define sseekable(s) s_can_seek(s)
int	savailable(P2(stream *, long *));
#define sreset(s) (*(s)->procs.reset)(s)
#define sflush(s) (*(s)->procs.flush)(s)
int	sclose(P1(stream *));

/* Following are only valid for read streams. */
int	spgetc(P1(stream *));			/* NOT FOR CLIENTS */
/* The first alternative should read */
/*	(int)(*++((s)->srptr))	*/
/* but the Borland compiler generates truly atrocious code for this. */
/* The SCO ODT compiler requires the first, pointless cast to int. */
#define sgetc(s)\
  ((int)(!sendrp(s) ? (++((s)->srptr), (int)*(s)->srptr) : spgetc(s)))
int	sgets(P4(stream *, byte *, uint, uint *));
int	sungetc(P2(stream *, byte));	/* ERRC on error, 0 if OK */
#define sputback(s) ((s)->srptr--)	/* can only do this once! */
#define seofp(s) (sendrp(s) && (s)->end_status == EOFC)
#define serrorp(s) (sendrp(s) && (s)->end_status == ERRC)
int	spskip(P2(stream *, long));
#define sskip(s,n) spskip(s,(long)(n))
/* Attempt to refill the buffer of a read stream. */
/* Only call this if the end_status is not EOFC. */
int	s_process_read_buf(P1(stream *));

/* Following are only valid for write streams. */
int	spputc(P2(stream *, byte));		/* NOT FOR CLIENTS */
/* The first alternative should read */
/*	((int)(*++((s)->swptr)=(c)))	*/
/* but the Borland compiler generates truly atrocious code for this. */
#define sputc(s,c)\
  (!sendwp(s) ? (++((s)->swptr), *(s)->swptr=(c), 0) : spputc((s),(c)))
int	sputs(P4(stream *, const byte *, uint, uint *));
/* Attempt to empty the buffer of a write stream. */
/* Only call this if the end_status is not EOFC. */
int	s_process_write_buf(P2(stream *, bool));

/* Following are only valid for positionable streams. */
long	stell(P1(stream *));
int	spseek(P2(stream *, long));
#define sseek(s,pos) spseek(s, (long)(pos))

/* Following are for high-performance reading clients. */
/* bufptr points to the next item. */
#define sbufptr(s) ((s)->srptr + 1)
#define sbufavailable(s) ((s)->srlimit - (s)->srptr)

/* The following are for very high-performance clients of read streams, */
/* who unpack the stream state into local variables. */
/* Note that any non-inline operations must do a s_end_inline before, */
/* and a s_begin_inline after. */
#define s_declare_inline(s, cp, ep)\
  register const byte *cp;\
  const byte *ep
#define s_begin_inline(s, cp, ep)\
  cp = (s)->srptr, ep = (s)->srlimit
#define s_end_inline(s, cp, ep)\
  (s)->srptr = cp
#define sbufavailable_inline(s, cp, ep)\
  (ep - cp)
#define sendbufp_inline(s, cp, ep)\
  (cp >= ep)
/* The (int) is needed to pacify the SCO ODT compiler. */
#define sgetc_inline(s, cp, ep)\
  ((int)(sendbufp_inline(s, cp, ep) ? spgetc_inline(s, cp, ep) : *++cp))
#define spgetc_inline(s, cp, ep)\
  (s_end_inline(s, cp, ep), (s)->inline_temp = spgetc(s),\
   s_begin_inline(s, cp, ep), (s)->inline_temp)
#define sputback_inline(s, cp, ep)\
  --cp

/* Allocate a stream or a stream state. */
stream *s_alloc(P2(gs_memory_t *, client_name_t));
stream_state *s_alloc_state(P3(gs_memory_t *, gs_memory_type_ptr_t, client_name_t));

/* Stream creation procedures */
void	sread_string(P3(stream *, const byte *, uint)),
	swrite_string(P3(stream *, byte *, uint));
void	sread_file(P4(stream *, FILE *, byte *, uint)),
	swrite_file(P4(stream *, FILE *, byte *, uint)),
	sappend_file(P4(stream *, FILE *, byte *, uint));

/* Standard stream initialization */
void	s_std_init(P5(stream *, byte *, uint, const stream_procs *, int	/*mode*/));

/* Standard stream finalization */
void	s_disable(P1(stream *));

/* Generic stream procedures exported for templates */
int	s_std_null(P1(stream *));
void	s_std_read_reset(P1(stream *)),
	s_std_write_reset(P1(stream *));
int	s_std_read_flush(P1(stream *)),
	s_std_write_flush(P1(stream *)),
	s_std_noavailable(P2(stream *, long *)),
	s_std_noseek(P2(stream *, long)),
	s_std_close(P1(stream *));
/* Generic procedures for filters. */
int	s_filter_write_flush(P1(stream *)),
	s_filter_close(P1(stream *));

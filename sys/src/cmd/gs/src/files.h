/* Copyright (C) 1989, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: files.h,v 1.1 2000/03/09 08:40:40 lpd Exp $ */
/* Definitions for interpreter support for file objects */
/* Requires stream.h */

#ifndef files_INCLUDED
#  define files_INCLUDED

/*
 * File objects store a pointer to a stream in value.pfile.
 * A file object is valid if its "size" matches the read_id or write_id
 * (as appropriate) in the stream it points to.  This arrangement
 * allows us to detect closed files reliably, while allowing us to
 * reuse closed streams for new files.
 */
#define fptr(pref) (pref)->value.pfile
#define make_file(pref,a,id,s)\
  make_tasv(pref,t_file,a,id,pfile,s)

/* The stdxxx files.  We have to access them through procedures, */
/* because they might have to be opened when referenced. */
int zget_stdin(P2(i_ctx_t *, stream **));
int zget_stdout(P2(i_ctx_t *, stream **));
int zget_stderr(P2(i_ctx_t *, stream **));
extern bool gs_stdin_is_interactive;
/* Test whether a stream is stdin. */
bool zis_stdin(P1(const stream *));

/* Define access to the stdio refs for operators. */
#define ref_stdio (i_ctx_p->stdio)
#define ref_stdin ref_stdio[0]
#define ref_stdout ref_stdio[1]
#define ref_stderr ref_stdio[2]
/* An invalid (closed) file. */
#define avm_invalid_file_entry avm_foreign
extern stream *const invalid_file_entry;
/* Make an invalid file object. */
void make_invalid_file(P1(ref *));

/*
 * Macros for checking file validity.
 * NOTE: in order to work around a bug in the Borland 5.0 compiler,
 * you must use file_is_invalid rather than !file_is_valid.
 */
#define file_is_valid(svar,op)\
  (svar = fptr(op), (svar->read_id | svar->write_id) == r_size(op))
#define file_is_invalid(svar,op)\
  (svar = fptr(op), (svar->read_id | svar->write_id) != r_size(op))
#define check_file(svar,op)\
  BEGIN\
    check_type(*(op), t_file);\
    if ( file_is_invalid(svar, op) ) return_error(e_invalidaccess);\
  END

/*
 * If a file is open for both reading and writing, its read_id, write_id,
 * and stream procedures and modes reflect the current mode of use;
 * an id check failure will switch it to the other mode.
 */
int file_switch_to_read(P1(const ref *));

#define check_read_file(svar,op)\
  BEGIN\
    check_read_type(*(op), t_file);\
    check_read_known_file(svar, op, return);\
  END
#define check_read_known_file(svar,op,error_return)\
  check_read_known_file_else(svar, op, error_return, svar = invalid_file_entry)
#define check_read_known_file_else(svar,op,error_return,invalid_action)\
  BEGIN\
    svar = fptr(op);\
    if (svar->read_id != r_size(op)) {\
	if (svar->read_id == 0 && svar->write_id == r_size(op)) {\
	    int fcode = file_switch_to_read(op);\
\
	    if (fcode < 0)\
		 error_return(fcode);\
	} else {\
	    invalid_action;	/* closed or reopened file */\
	}\
    }\
  END
int file_switch_to_write(P1(const ref *));

#define check_write_file(svar,op)\
  BEGIN\
    check_write_type(*(op), t_file);\
    check_write_known_file(svar, op, return);\
  END
#define check_write_known_file(svar,op,error_return)\
  BEGIN\
    svar = fptr(op);\
    if ( svar->write_id != r_size(op) )\
	{	int fcode = file_switch_to_write(op);\
		if ( fcode < 0 ) error_return(fcode);\
	}\
  END

/* Data exported by zfile.c. */
	/* for zfilter.c and ziodev.c */
extern const uint file_default_buffer_size;

/* Procedures exported by zfile.c. */
	/* for imainarg.c */
FILE *lib_fopen(P1(const char *));

	/* for imain.c */
int lib_file_open(P7(const char *, uint, byte *, uint, uint *, ref *,
		     gs_memory_t *));

	/* for imain.c */
#ifndef gs_ref_memory_DEFINED
#  define gs_ref_memory_DEFINED
typedef struct gs_ref_memory_s gs_ref_memory_t;
#endif
int file_read_string(P4(const byte *, uint, ref *, gs_ref_memory_t *));

	/* for os_open in ziodev.c */
#ifdef iodev_proc_fopen		/* in gxiodev.h */
int file_open_stream(P7(const char *, uint, const char *, uint,
			stream **, iodev_proc_fopen_t, gs_memory_t *));
#endif

	/* for zfilter.c */
int filter_open(P7(const char *, uint, ref *, const stream_procs *,
		   const stream_template *, const stream_state *,
		   gs_memory_t *));

	/* for zfileio.c */
void make_stream_file(P3(ref *, stream *, const char *));

	/* for ziodev.c */
int file_close_finish(P1(stream *));
int file_close_disable(P1(stream *));
int file_close_file(P1(stream *));

	/* for gsmain.c, interp.c */
int file_close(P1(ref *));

	/* for zfproc.c, ziodev.c */
stream *file_alloc_stream(P2(gs_memory_t *, client_name_t));

/* Procedures exported by zfileio.c. */
	/* for ziodev.c */
int zreadline_from(P5(stream *s, gs_string *buf, gs_memory_t *bufmem,
		      uint *pcount, bool *pin_eol));

#endif /* files_INCLUDED */

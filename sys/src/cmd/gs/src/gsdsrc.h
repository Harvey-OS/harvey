/* Copyright (C) 1997, 1998 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gsdsrc.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* DataSource definitions */

#ifndef gsdsrc_INCLUDED
#  define gsdsrc_INCLUDED

#include "gsstruct.h"

/* ---------------- Types and structures ---------------- */

/*
 * A gs_data_source_t represents the data source for various constructs.  It
 * can be a string (either a gs_string or a byte-type object), a
 * positionable, non-procedure-based stream, or an array of floats.  An
 * ordinary positionable file stream will do, as long as the client doesn't
 * attempt to read past the EOF.
 *
 * The handling of floats is anomalous, but we don't see a good alternative
 * at the moment.
 */

#ifndef stream_DEFINED
#  define stream_DEFINED
typedef struct stream_s stream;

#endif

/*
 * Prepare to access a block of data from a source.  buf must be a client-
 * supplied buffer of at least length bytes.  If ptr == 0, always copy the
 * data into buf.  If ptr != 0, either copy the data into buf and set *ptr =
 * buf, or set *ptr to point to the data (which might be invalidated by the
 * next call).  Note that this procedure may or may not do bounds checking.
 */
#define data_source_proc_access(proc)\
  int proc(P5(const gs_data_source_t *psrc, ulong start, uint length,\
	      byte *buf, const byte **ptr))

typedef enum {
    data_source_type_string,
    data_source_type_bytes,
    data_source_type_floats,
    data_source_type_stream
} gs_data_source_type_t;
typedef struct gs_data_source_s gs_data_source_t;
struct gs_data_source_s {
    data_source_proc_access((*access));
    gs_data_source_type_t type;
    union d_ {
	gs_const_string str;	/* also used for byte objects */
	stream *strm;
    } data;
};

#define data_source_access_only(psrc, start, length, buf, ptr)\
  (*(psrc)->access)(psrc, (ulong)(start), length, buf, ptr)
#define data_source_access(psrc, start, length, buf, ptr)\
  BEGIN\
    int code_ = data_source_access_only(psrc, start, length, buf, ptr);\
    if ( code_ < 0 ) return code_;\
  END
#define data_source_copy_only(psrc, start, length, buf)\
  data_source_access_only(psrc, start, length, buf, (const byte **)0)
#define data_source_copy(psrc, start, length, buf)\
  data_source_access(psrc, start, length, buf, (const byte **)0)

/*
 * Data sources are always embedded in other structures, but they do have
 * pointers that need to be traced and relocated, so they do have a GC
 * structure type.
 */
extern_st(st_data_source);
#define public_st_data_source()	/* in gsdsrc.c */\
  gs_public_st_composite(st_data_source, gs_data_source_t, "gs_data_source_t",\
    data_source_enum_ptrs, data_source_reloc_ptrs)
#define st_data_source_max_ptrs 1

/* ---------------- Procedures ---------------- */

/* Initialize a data source of the various known types. */
data_source_proc_access(data_source_access_string);
#define data_source_init_string(psrc, strg)\
  ((psrc)->type = data_source_type_string,\
   (psrc)->data.str = strg, (psrc)->access = data_source_access_string)
#define data_source_init_string2(psrc, bytes, len)\
  ((psrc)->type = data_source_type_string,\
   (psrc)->data.str.data = bytes, (psrc)->data.str.size = len,\
   (psrc)->access = data_source_access_string)
data_source_proc_access(data_source_access_bytes);
#define data_source_init_bytes(psrc, bytes, len)\
  ((psrc)->type = data_source_type_bytes,\
   (psrc)->data.str.data = bytes, (psrc)->data.str.size = len,\
   (psrc)->access = data_source_access_bytes)
#define data_source_init_floats(psrc, floats, count)\
  ((psrc)->type = data_source_type_floats,\
   (psrc)->data.str.data = (byte *)floats,\
   (psrc)->data.str.size = (count) * sizeof(float),\
   (psrc)->access = data_source_access_bytes)
data_source_proc_access(data_source_access_stream);
#define data_source_init_stream(psrc, s)\
  ((psrc)->type = data_source_type_stream,\
   (psrc)->data.strm = s, (psrc)->access = data_source_access_stream)

#define data_source_is_stream(dsource)\
  ((dsource).type == data_source_type_stream)
#define data_source_is_array(dsource)\
  ((dsource).type == data_source_type_floats)

#endif /* gsdsrc_INCLUDED */

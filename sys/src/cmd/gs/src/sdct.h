/* Copyright (C) 1994, 1997, 1998 Aladdin Enterprises.  All rights reserved.

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

/*$Id: sdct.h,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* Definitions for DCT filters */
/* Requires stream.h, strimpl.h, jpeg/jpeglib.h */

#ifndef sdct_INCLUDED
#  define sdct_INCLUDED

#include <setjmp.h>		/* for jmp_buf */

/* ------ DCT filters ------ */

/*
 * Define the stream state.
 * The jpeg_xxx_data structs are allocated in immovable memory
 * to simplify use of the IJG library.
 */
#define jpeg_stream_data_common\
		/* We put a copy of the stream template here, because */\
		/* the minimum buffer sizes depend on the image parameters. */\
	stream_template template;\
	struct jpeg_error_mgr err;\
	jmp_buf exit_jmpbuf;\
	gs_memory_t *memory;	/* heap for library allocations */\
		/* The following are documented in Adobe TN 5116. */\
	int Picky;		/* 0 or 1 */\
	int Relax		/* 0 or 1 */
typedef struct jpeg_stream_data_s {
    jpeg_stream_data_common;
} jpeg_stream_data;

/* Define initialization for the non-library part of the stream state. */
#define jpeg_stream_data_common_init(pdata)\
  ((pdata)->Picky = 0, (pdata)->Relax = 0)

typedef struct jpeg_compress_data_s {
    jpeg_stream_data_common;
    /* cinfo must immediately follow the common fields */
    struct jpeg_compress_struct cinfo;
    struct jpeg_destination_mgr destination;
    byte finish_compress_buf[100];
    int fcb_size, fcb_pos;
} jpeg_compress_data;

typedef struct jpeg_decompress_data_s {
    jpeg_stream_data_common;
    /* dinfo must immediately follow the common fields, */
    /* so that it has same offset as cinfo. */
    struct jpeg_decompress_struct dinfo;
    struct jpeg_source_mgr source;
    long skip;			/* # of bytes remaining to skip in input */
    bool input_eod;		/* true when no more input data available */
    bool faked_eoi;		/* true when fill_input_buffer inserted EOI */
    byte *scanline_buffer;	/* buffer for oversize scanline, or NULL */
    uint bytes_in_scanline;	/* # of bytes remaining to output from same */
} jpeg_decompress_data;

/* The stream state itself.  This is kept in garbage-collectable memory. */
typedef struct stream_DCT_state_s {
    stream_state_common;
    /* The following are set before initialization. */
    /* Note that most JPEG parameters go straight into */
    /* the IJG data structures, not into this struct. */
    gs_const_string Markers;	/* NULL if no Markers parameter */
    float QFactor;
    int ColorTransform;		/* -1 if not specified */
    bool NoMarker;		/* DCTEncode only */
    gs_memory_t *jpeg_memory;	/* heap for library allocations */
    /* This is a pointer to immovable storage. */
    union _jd {
	jpeg_stream_data *common;
	jpeg_compress_data *compress;
	jpeg_decompress_data *decompress;
    } data;
    /* DCTEncode sets this before initialization;
     * DCTDecode cannot set it until the JPEG headers are read.
     */
    uint scan_line_size;
    /* The following are updated dynamically. */
    int phase;
} stream_DCT_state;

/* The state descriptor is public only to allow us to split up */
/* the encoding and decoding filters. */
extern_st(st_DCT_state);
#define public_st_DCT_state()	/* in sdctc.c */\
  gs_public_st_const_strings1(st_DCT_state, stream_DCT_state,\
    "DCTEncode/Decode state", dct_enum_ptrs, dct_reloc_ptrs, Markers)

/*
 * NOTE: the client *must* invoke the set_defaults procedure in the
 * template before calling the init procedure.
 */
extern const stream_template s_DCTD_template;
extern const stream_template s_DCTE_template;

/* Define an internal procedure for setting stream defaults. */
/* Clients do not call this. */
void s_DCT_set_defaults(P1(stream_state * st));

#endif /* sdct_INCLUDED */

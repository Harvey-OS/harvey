/* Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpsds.c,v 1.14 2005/02/26 18:07:43 igor Exp $ */
/* Image processing streams for PostScript and PDF writers */
#include "gx.h"
#include "memory_.h"
#include "gserrors.h"
#include "gxdcconv.h"
#include "gdevpsds.h"
#include "gxbitmap.h"
#include "gxcspace.h"
#include "gsdcolor.h"
#include "gscspace.h"
#include "gxdevcli.h"

/* ---------------- Convert between 1/2/4/12 and 8 bits ---------------- */

gs_private_st_simple(st_1248_state, stream_1248_state, "stream_1248_state");

/* Initialize an expansion or reduction stream. */
int
s_1248_init(stream_1248_state *ss, int Columns, int samples_per_pixel)
{
    ss->samples_per_row = Columns * samples_per_pixel;
    return ss->template->init((stream_state *)ss);
}

/* Initialize the state. */
private int
s_1_init(stream_state * st)
{
    stream_1248_state *const ss = (stream_1248_state *) st;

    ss->left = ss->samples_per_row;
    ss->bits_per_sample = 1;
    return 0;
}
private int
s_2_init(stream_state * st)
{
    stream_1248_state *const ss = (stream_1248_state *) st;

    ss->left = ss->samples_per_row;
    ss->bits_per_sample = 2;
    return 0;
}
private int
s_4_init(stream_state * st)
{
    stream_1248_state *const ss = (stream_1248_state *) st;

    ss->left = ss->samples_per_row;
    ss->bits_per_sample = 4;
    return 0;
}
private int
s_12_init(stream_state * st)
{
    stream_1248_state *const ss = (stream_1248_state *) st;

    ss->left = ss->samples_per_row;
    ss->bits_per_sample = 12;	/* not needed */
    return 0;
}

/* Process one buffer. */
#define BEGIN_1248\
	stream_1248_state * const ss = (stream_1248_state *)st;\
	const byte *p = pr->ptr;\
	const byte *rlimit = pr->limit;\
	byte *q = pw->ptr;\
	byte *wlimit = pw->limit;\
	uint left = ss->left;\
	int status;\
	int n
#define END_1248\
	pr->ptr = p;\
	pw->ptr = q;\
	ss->left = left;\
	return status

/* N-to-8 expansion */
#define FOREACH_N_8(in, nout)\
	status = 0;\
	for ( ; p < rlimit; left -= n, q += n, ++p ) {\
	  byte in = p[1];\
	  n = min(left, nout);\
	  if ( wlimit - q < n ) {\
	    status = 1;\
	    break;\
	  }\
	  switch ( n ) {\
	    case 0: left = ss->samples_per_row; --p; continue;
#define END_FOREACH_N_8\
	  }\
	}
private int
s_N_8_process(stream_state * st, stream_cursor_read * pr,
	      stream_cursor_write * pw, bool last)
{
    BEGIN_1248;

    switch (ss->bits_per_sample) {

	case 1:{
		FOREACH_N_8(in, 8)
	case 8:
		q[8] = (byte) - (in & 1);
	case 7:
		q[7] = (byte) - ((in >> 1) & 1);
	case 6:
		q[6] = (byte) - ((in >> 2) & 1);
	case 5:
		q[5] = (byte) - ((in >> 3) & 1);
	case 4:
		q[4] = (byte) - ((in >> 4) & 1);
	case 3:
		q[3] = (byte) - ((in >> 5) & 1);
	case 2:
		q[2] = (byte) - ((in >> 6) & 1);
	case 1:
		q[1] = (byte) - (in >> 7);
		END_FOREACH_N_8;
	    }
	    break;

	case 2:{
		static const byte b2[4] =
		{0x00, 0x55, 0xaa, 0xff};

		FOREACH_N_8(in, 4)
	case 4:
		q[4] = b2[in & 3];
	case 3:
		q[3] = b2[(in >> 2) & 3];
	case 2:
		q[2] = b2[(in >> 4) & 3];
	case 1:
		q[1] = b2[in >> 6];
		END_FOREACH_N_8;
	    }
	    break;

	case 4:{
		static const byte b4[16] =
		{
		    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
		    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
		};

		FOREACH_N_8(in, 2)
	case 2:
		q[2] = b4[in & 0xf];
	case 1:
		q[1] = b4[in >> 4];
		END_FOREACH_N_8;
	    }
	    break;

	default:
	    return ERRC;
    }

    END_1248;
}

/* 12-to-8 "expansion" */
private int
s_12_8_process(stream_state * st, stream_cursor_read * pr,
	       stream_cursor_write * pw, bool last)
{
    BEGIN_1248;

    n = ss->samples_per_row;	/* misuse n to avoid a compiler warning */
    status = 0;
    for (; rlimit - p >= 2; ++q) {
	if (q >= wlimit) {
	    status = 1;
	    break;
	}
	if (left == 0)
	    left = n;
	if ((n - left) & 1) {
	    q[1] = (byte)((p[1] << 4) | (p[2] >> 4));
	    p += 2, --left;
	} else {
	    q[1] = *++p;
	    if (!--left)
		++p;
	}
    }

    END_1248;
}


/* 8-to-N reduction */
#define FOREACH_8_N(out, nin)\
	byte out;\
	status = 1;\
	for ( ; q < wlimit; left -= n, p += n, ++q ) {\
	  n = min(left, nin);\
	  if ( rlimit - p < n ) {\
	    status = 0;\
	    break;\
	  }\
	  out = 0;\
	  switch ( n ) {\
	    case 0: left = ss->samples_per_row; --q; continue;
#define END_FOREACH_8_N\
	    q[1] = out;\
	  }\
	}
private int
s_8_N_process(stream_state * st, stream_cursor_read * pr,
	      stream_cursor_write * pw, bool last)
{
    BEGIN_1248;

    switch (ss->bits_per_sample) {

	case 1:{
		FOREACH_8_N(out, 8)
	case 8:
		out = p[8] >> 7;
	case 7:
		out |= (p[7] >> 7) << 1;
	case 6:
		out |= (p[6] >> 7) << 2;
	case 5:
		out |= (p[5] >> 7) << 3;
	case 4:
		out |= (p[4] >> 7) << 4;
	case 3:
		out |= (p[3] >> 7) << 5;
	case 2:
		out |= (p[2] >> 7) << 6;
	case 1:
		out |= p[1] & 0x80;
		END_FOREACH_8_N;
	    }
	    break;

	case 2:{
		FOREACH_8_N(out, 4)
	case 4:
		out |= p[4] >> 6;
	case 3:
		out |= (p[3] >> 6) << 2;
	case 2:
		out |= (p[2] >> 6) << 4;
	case 1:
		out |= p[1] & 0xc0;
		END_FOREACH_8_N;
	    }
	    break;

	case 4:{
		FOREACH_8_N(out, 2)
	case 2:
		out |= p[2] >> 4;
	case 1:
		out |= p[1] & 0xf0;
		END_FOREACH_8_N;
	    }
	    break;

	default:
	    return ERRC;
    }

    END_1248;
}

const stream_template s_1_8_template = {
    &st_1248_state, s_1_init, s_N_8_process, 1, 8
};
const stream_template s_2_8_template = {
    &st_1248_state, s_2_init, s_N_8_process, 1, 4
};
const stream_template s_4_8_template = {
    &st_1248_state, s_4_init, s_N_8_process, 1, 2
};
const stream_template s_12_8_template = {
    &st_1248_state, s_12_init, s_12_8_process, 1, 2
};

const stream_template s_8_1_template = {
    &st_1248_state, s_1_init, s_8_N_process, 8, 1
};
const stream_template s_8_2_template = {
    &st_1248_state, s_2_init, s_8_N_process, 4, 1
};
const stream_template s_8_4_template = {
    &st_1248_state, s_4_init, s_8_N_process, 2, 1
};

/* ---------------- Color space conversion ---------------- */

/* ------ Convert CMYK to RGB ------ */

private_st_C2R_state();

/* Initialize a CMYK => RGB conversion stream. */
int
s_C2R_init(stream_C2R_state *ss, const gs_imager_state *pis)
{
    ss->pis = pis;
    return 0;
}

/* Set default parameter values (actually, just clear pointers). */
private void
s_C2R_set_defaults(stream_state * st)
{
    stream_C2R_state *const ss = (stream_C2R_state *) st;

    ss->pis = 0;
}

/* Process one buffer. */
private int
s_C2R_process(stream_state * st, stream_cursor_read * pr,
	      stream_cursor_write * pw, bool last)
{
    stream_C2R_state *const ss = (stream_C2R_state *) st;
    const byte *p = pr->ptr;
    const byte *rlimit = pr->limit;
    byte *q = pw->ptr;
    byte *wlimit = pw->limit;

    for (; rlimit - p >= 4 && wlimit - q >= 3; p += 4, q += 3) {
	byte bc = p[1], bm = p[2], by = p[3], bk = p[4];
	frac rgb[3];

	color_cmyk_to_rgb(byte2frac(bc), byte2frac(bm), byte2frac(by),
			  byte2frac(bk), ss->pis, rgb);
	q[1] = frac2byte(rgb[0]);
	q[2] = frac2byte(rgb[1]);
	q[3] = frac2byte(rgb[2]);
    }
    pr->ptr = p;
    pw->ptr = q;
    return (rlimit - p < 4 ? 0 : 1);
}

const stream_template s_C2R_template = {
    &st_C2R_state, 0 /*NULL */ , s_C2R_process, 4, 3, 0, s_C2R_set_defaults
};

/* ------ Convert any color space to Indexed ------ */

private_st_IE_state();
private
ENUM_PTRS_WITH(ie_state_enum_ptrs, stream_IE_state *st) return 0;
case 0: return ENUM_OBJ(st->Decode);
case 1: return ENUM_BYTESTRING(&st->Table);
ENUM_PTRS_END
private
RELOC_PTRS_WITH(ie_state_reloc_ptrs, stream_IE_state *st)
{
    RELOC_VAR(st->Decode);
    RELOC_BYTESTRING_VAR(st->Table);
}
RELOC_PTRS_END

/* Set defaults. */
private void
s_IE_set_defaults(stream_state * st)
{
    stream_IE_state *const ss = (stream_IE_state *) st;

    ss->Decode = 0;		/* clear pointers */
    gs_bytestring_from_string(&ss->Table, 0, 0);
}

/* Initialize the state. */
private int
s_IE_init(stream_state * st)
{
    stream_IE_state *const ss = (stream_IE_state *) st;
    int key_index = (1 << ss->BitsPerIndex) * ss->NumComponents;
    int i;

    if (ss->Table.data == 0 || ss->Table.size < key_index)
	return ERRC;		/****** WRONG ******/
    /* Initialize Table with default values. */
    memset(ss->Table.data, 0, ss->NumComponents);
    ss->Table.data[ss->Table.size - 1] = 0;
    for (i = 0; i < countof(ss->hash_table); ++i)
	ss->hash_table[i] = key_index;
    ss->next_index = 0;
    ss->in_bits_left = 0;
    ss->next_component = 0;
    ss->byte_out = 1;
    ss->x = 0;
    return 0;
}

/* Process a buffer. */
private int
s_IE_process(stream_state * st, stream_cursor_read * pr,
	     stream_cursor_write * pw, bool last)
{
    stream_IE_state *const ss = (stream_IE_state *) st;
    /* Constant values from the state */
    const int bpc = ss->BitsPerComponent;
    const int num_components = ss->NumComponents;
    const int end_index = (1 << ss->BitsPerIndex) * num_components;
    byte *const table = ss->Table.data;
    byte *const key = table + end_index;
    /* Dynamic values from the state */
    uint byte_in = ss->byte_in;
    int in_bits_left = ss->in_bits_left;
    int next_component = ss->next_component;
    uint byte_out = ss->byte_out;
    /* Other dynamic values */
    const byte *p = pr->ptr;
    const byte *rlimit = pr->limit;
    byte *q = pw->ptr;
    byte *wlimit = pw->limit;
    int status = 0;

    for (;;) {
	uint hash, reprobe;
	int i, index;

	/* Check for a filled output byte. */
	if (byte_out >= 0x100) {
	    if (q >= wlimit) {
		status = 1;
		break;
	    }
	    *++q = (byte)byte_out;
	    byte_out = 1;
	}
	/* Acquire a complete input value. */
	while (next_component < num_components) {
	    const float *decode = &ss->Decode[next_component * 2];
	    int sample;

	    if (in_bits_left == 0) {
		if (p >= rlimit)
		    goto out;
		byte_in = *++p;
		in_bits_left = 8;
	    }
	    /* An input sample can never span a byte boundary. */
	    in_bits_left -= bpc;
	    sample = (byte_in >> in_bits_left) & ((1 << bpc) - 1);
	    /* Scale the sample according to Decode. */
	    sample = (int)((decode[0] +
			    (sample / (float)((1 << bpc) - 1) *
			     (decode[1] - decode[0]))) * 255 + 0.5);
	    key[next_component++] =
		(sample < 0 ? 0 : sample > 255 ? 255 : (byte)sample);
	}
	/* Look up the input value. */
	for (hash = 0, i = 0; i < num_components; ++i)
	    hash = hash + 23 * key[i];  /* adhoc */
	reprobe = (hash / countof(ss->hash_table)) | 137;  /* adhoc */
	for (hash %= countof(ss->hash_table);
	     memcmp(table + ss->hash_table[hash], key, num_components);
	     hash = (hash + reprobe) % countof(ss->hash_table)
	     )
	    DO_NOTHING;
	index = ss->hash_table[hash];
	if (index == end_index) {
	    /* The match was on an empty entry. */
	    if (ss->next_index == end_index) {
		/* Too many different values. */
		status = ERRC;
		break;
	    }
	    ss->hash_table[hash] = index = ss->next_index;
	    ss->next_index += num_components;
	    memcpy(table + index, key, num_components);
	}
	byte_out = (byte_out << ss->BitsPerIndex) + index / num_components;
	next_component = 0;
	if (++(ss->x) == ss->Width) {
	    /* Handle input and output padding. */
	    in_bits_left = 0;
	    if (byte_out != 1)
		while (byte_out < 0x100)
		    byte_out <<= 1;
	    ss->x = 0;
	}
    }
out:
    pr->ptr = p;
    pw->ptr = q;
    ss->byte_in = byte_in;
    ss->in_bits_left = in_bits_left;
    ss->next_component = next_component;
    ss->byte_out = byte_out;
    /* For simplicity, always update the record of the table size. */
    ss->Table.data[ss->Table.size - 1] =
	(ss->next_index == 0 ? 0 :
	 ss->next_index / ss->NumComponents - 1);
    return status;
}

const stream_template s_IE_template = {
    &st_IE_state, s_IE_init, s_IE_process, 1, 1,
    0 /* NULL */, s_IE_set_defaults
};

#if 0

/* Test code */
void
test_IE(void)
{
    const stream_template *const template = &s_IE_template;
    stream_IE_state state;
    stream_state *const ss = (stream_state *)&state;
    static const float decode[6] = {1, 0, 1, 0, 1, 0};
    static const byte in[] = {
	/*
	 * Each row is 3 pixels x 3 components x 4 bits.  Processing the
	 * first two rows doesn't cause an error; processing all 3 rows
	 * does.
	 */
	0x12, 0x35, 0x67, 0x9a, 0xb0,
	0x56, 0x7d, 0xef, 0x12, 0x30,
	0x88, 0x88, 0x88, 0x88, 0x80
    };
    byte table[3 * 5];
    int n;

    template->set_defaults(ss);
    state.BitsPerComponent = 4;
    state.NumComponents = 3;
    state.Width = 3;
    state.BitsPerIndex = 2;
    state.Decode = decode;
    gs_bytestring_from_bytes(&state.Table, table, 0, sizeof(table));
    for (n = 10; n <= 15; n += 5) {
	stream_cursor_read r;
	stream_cursor_write w;
	byte out[100];
	int status;

	s_IE_init(ss);
	r.ptr = in; --r.ptr;
	r.limit = r.ptr + n;
	w.ptr = out; --w.ptr;
	w.limit = w.ptr + sizeof(out);
	memset(table, 0xcc, sizeof(table));
	memset(out, 0xff, sizeof(out));
	dprintf1("processing %d bytes\n", n);
	status = template->process(ss, &r, &w, true);
	dprintf3("%d bytes read, %d bytes written, status = %d\n",
		 (int)(r.ptr + 1 - in), (int)(w.ptr + 1 - out), status);
	debug_dump_bytes(table, table + sizeof(table), "table");
	debug_dump_bytes(out, w.ptr + 1, "out");
    }
}

#endif

/* ---------------- Downsampling ---------------- */

/* Return the number of samples after downsampling. */
int
s_Downsample_size_out(int size_in, int factor, bool pad)
{
    return ((pad ? size_in + factor - 1 : size_in) / factor);
}

private void
s_Downsample_set_defaults(register stream_state * st)
{
    stream_Downsample_state *const ss = (stream_Downsample_state *)st;

    s_Downsample_set_defaults_inline(ss);
}

/* ------ Subsample ------ */

gs_private_st_simple(st_Subsample_state, stream_Subsample_state,
		     "stream_Subsample_state");

/* Initialize the state. */
private int
s_Subsample_init(stream_state * st)
{
    stream_Subsample_state *const ss = (stream_Subsample_state *) st;

    ss->x = ss->y = 0;
    return 0;
}

/* Process one buffer. */
private int
s_Subsample_process(stream_state * st, stream_cursor_read * pr,
		    stream_cursor_write * pw, bool last)
{
    stream_Subsample_state *const ss = (stream_Subsample_state *) st;
    const byte *p = pr->ptr;
    const byte *rlimit = pr->limit;
    byte *q = pw->ptr;
    byte *wlimit = pw->limit;
    int spp = ss->Colors;
    int width = ss->WidthIn, height = ss->HeightIn;
    int xf = ss->XFactor, yf = ss->YFactor;
    int xf2 = xf / 2, yf2 = yf / 2;
    int xlimit = (width / xf) * xf, ylimit = (height / yf) * yf;
    int xlast =
	(ss->padX && xlimit < width ? xlimit + (width % xf) / 2 : -1);
    int ylast =
	(ss->padY && ylimit < height ? ylimit + (height % yf) / 2 : -1);
    int x = ss->x, y = ss->y;
    int status = 0;

    if_debug4('w', "[w]subsample: x=%d, y=%d, rcount=%ld, wcount=%ld\n",
	      x, y, (long)(rlimit - p), (long)(wlimit - q));
    for (; rlimit - p >= spp; p += spp) {
	if (((y % yf == yf2 && y < ylimit) || y == ylast) &&
	    ((x % xf == xf2 && x < xlimit) || x == xlast)
	    ) {
	    if (wlimit - q < spp) {
		status = 1;
		break;
	    }
	    memcpy(q + 1, p + 1, spp);
	    q += spp;
	}
	if (++x == width)
	    x = 0, ++y;
    }
    if_debug5('w',
	      "[w]subsample: x'=%d, y'=%d, read %ld, wrote %ld, status = %d\n",
	      x, y, (long)(p - pr->ptr), (long)(q - pw->ptr), status);
    pr->ptr = p;
    pw->ptr = q;
    ss->x = x, ss->y = y;
    return status;
}

const stream_template s_Subsample_template = {
    &st_Subsample_state, s_Subsample_init, s_Subsample_process, 4, 4,
    0 /* NULL */, s_Downsample_set_defaults
};

/* ------ Average ------ */

private_st_Average_state();

/* Set default parameter values (actually, just clear pointers). */
private void
s_Average_set_defaults(stream_state * st)
{
    stream_Average_state *const ss = (stream_Average_state *) st;

    s_Downsample_set_defaults(st);
    /* Clear pointers */
    ss->sums = 0;
}

/* Initialize the state. */
private int
s_Average_init(stream_state * st)
{
    stream_Average_state *const ss = (stream_Average_state *) st;

    ss->sum_size =
	ss->Colors * ((ss->WidthIn + ss->XFactor - 1) / ss->XFactor);
    ss->copy_size = ss->sum_size -
	(ss->padX || (ss->WidthIn % ss->XFactor == 0) ? 0 : ss->Colors);
    ss->sums =
	(uint *)gs_alloc_byte_array(st->memory, ss->sum_size,
				    sizeof(uint), "Average sums");
    if (ss->sums == 0)
	return ERRC;	/****** WRONG ******/
    memset(ss->sums, 0, ss->sum_size * sizeof(uint));
    return s_Subsample_init(st);
}

/* Release the state. */
private void
s_Average_release(stream_state * st)
{
    stream_Average_state *const ss = (stream_Average_state *) st;

    gs_free_object(st->memory, ss->sums, "Average sums");
}

/* Process one buffer. */
private int
s_Average_process(stream_state * st, stream_cursor_read * pr,
		  stream_cursor_write * pw, bool last)
{
    stream_Average_state *const ss = (stream_Average_state *) st;
    const byte *p = pr->ptr;
    const byte *rlimit = pr->limit;
    byte *q = pw->ptr;
    byte *wlimit = pw->limit;
    int spp = ss->Colors;
    int width = ss->WidthIn;
    int xf = ss->XFactor, yf = ss->YFactor;
    int x = ss->x, y = ss->y;
    uint *sums = ss->sums;
    int status = 0;

top:
    if (y == yf || (last && p >= rlimit && ss->padY && y != 0)) {
	/* We're copying averaged values to the output. */
	int ncopy = min(ss->copy_size - x, wlimit - q);

	if (ncopy) {
	    int scale = xf * y;

	    while (--ncopy >= 0)
		*++q = (byte) (sums[x++] / scale);
	}
	if (x < ss->copy_size) {
	    status = 1;
	    goto out;
	}
	/* Done copying. */
	x = y = 0;
	memset(sums, 0, ss->sum_size * sizeof(uint));
    }
    while (rlimit - p >= spp) {
	uint *bp = sums + x / xf * spp;
	int i;

	for (i = spp; --i >= 0;)
	    *bp++ += *++p;
	if (++x == width) {
	    x = 0;
	    ++y;
	    goto top;
	}
    }
out:
    pr->ptr = p;
    pw->ptr = q;
    ss->x = x, ss->y = y;
    return status;
}

const stream_template s_Average_template = {
    &st_Average_state, s_Average_init, s_Average_process, 4, 4,
    s_Average_release, s_Average_set_defaults
};

/* ---------------- Image compression chooser ---------------- */

private_st_compr_chooser_state();

/* Initialize the state. */
private int
s_compr_chooser_init(stream_state * st)
{
    stream_compr_chooser_state *const ss = (stream_compr_chooser_state *) st;

    ss->choice = 0;
    ss->width = ss->height = ss->depth = ss->bits_per_sample = 0;
    ss->sample = 0;
    ss->samples_count = 0;
    ss->bits_left = 0;
    ss->packed_data = 0;
    ss->lower_plateaus = ss->upper_plateaus = 0;
    ss->gradients = 0;
    return 0;
}

/* Set image dimensions. */
int
s_compr_chooser_set_dimensions(stream_compr_chooser_state * ss, int width, 
		    int height, int depth, int bits_per_sample)
{
    ss->width = width;
    ss->height = height;
    ss->depth = depth;
    ss->bits_per_sample = bits_per_sample;
    ss->sample = gs_alloc_bytes(ss->memory, width * depth, "s_compr_chooser_set_dimensions");
    if (ss->sample == 0)
	return_error(gs_error_VMerror);
    return 0;
}

/* Release state. */
private void
s_compr_chooser_release(stream_state * st)
{
    stream_compr_chooser_state *const ss = (stream_compr_chooser_state *) st;

    gs_free_object(ss->memory, ss->sample, "s_compr_chooser_release");
}

/* Estimate a row for photo/lineart recognition. */
private void
s_compr_chooser__estimate_row(stream_compr_chooser_state *const ss, byte *p)
{   
    /*	This function uses a statistical algorithm being not well defined.

	We compute areas covered by gradients,
	separately with small width (line art)
	and with big width (photo).
	Making the choice based on the areas.

	Note that we deal with horizontal frequencies only.
	Dealing with vertical ones would be too expensive.
    */
    const int delta = 256 / 16; /* about 1/16 of the color range */
    const int max_lineart_boundary_width = 3; /* pixels */
    const int max_gradient_constant = 10; /* pixels */
    int i, j0 = 0, j1 = 0;
    int w0 = p[0], w1 = p[0], v;
    ulong plateau_count = 0, lower_plateaus = 0;
    ulong upper_plateaus = 0, gradients = 0;
    bool lower = false, upper = false;

    for (i = 1; i < ss->width; i++) {
	v = p[i];
	if (!lower) {
	    if (w1 < v) {
		if (!upper)
		    j1 = i - 1;
		w1 = v;
		upper = true;
	    } else if (w1 == v && j1 < i - max_gradient_constant)
		j1 = i - max_gradient_constant; /* inner constant plateaw */
	    else if (upper && w1 - delta > v) {
		/* end of upper plateau at w1-delta...w1 */
		for (j0 = i - 1; j0 > j1 && w1 - delta <= p[j0]; j0--) DO_NOTHING;
		/* upper plateau j0+1...i-1 */
		if(j0 > 0 && i < ss->width - 1) /* ignore sides */
		    upper_plateaus += i - j0;
		plateau_count ++;
		if (j0 > j1) {
		    /* upgrade j1...j0 */
		    if (j0 > j1 + max_lineart_boundary_width)
			gradients += j0 - j1;
		}
		j1 = i;
		upper = false;
		w0 = w1;
		continue;
	    }
	}
	if (!upper) {
	    if (w0 > v) {
		if (!lower)
		    j1 = i - 1;
		w0 = v; 
		lower = true;
	    } else if (w0 == v && j1 < i - max_gradient_constant)
		j1 = i - max_gradient_constant; /* inner constant plateaw */
	    else if (lower && w0 + delta < v) {
		/* end of lower plateau at w0...w0+delta */
		for (j0 = i - 1; j0 > j1 && w0 + delta >= p[j0]; j0--) DO_NOTHING;
		/* lower plateau j0+1...i-1 */
		if(j0 > 0 && i < ss->width - 1) /* ignore sides */
		    lower_plateaus += i - j0;
		plateau_count ++;
		if (j0 > j1) {
		    /* downgrade j1...j0 */
		    if (j0 > j1 + max_lineart_boundary_width)
			gradients += j0 - j1;
		}
		j1 = i;
		lower = false;
		w1 = w0;
	    }
	}
    }
    if (plateau_count > ss->width / 6) {
	/*  Possibly a dithering, can't recognize.
	    It would be better to estimate frequency histogram rather than 
	    rough quantity, but we hope that the simpler test can work fine.
	*/
    } else if (!plateau_count) /* a pseudo-constant color through entire row */
	DO_NOTHING; /* ignore such lines */
    else {
	int plateaus;
	ss->lower_plateaus += lower_plateaus;
	ss->upper_plateaus += upper_plateaus;
	ss->gradients += gradients;
	plateaus = min(ss->lower_plateaus, ss->upper_plateaus); /* (fore/back)ground */
	if (ss->gradients >= 10000 && ss->gradients > plateaus / 6)
	    ss->choice = 1; /* choice is made : photo */
	else if (plateaus >= 100000 && plateaus / 5000 >= ss->gradients)
	    ss->choice = 2; /* choice is made : lineart */
    }
}

/* Recognize photo/lineart. */
private void
s_compr_chooser__recognize(stream_compr_chooser_state * ss)
{
    int i;
    byte *p = ss->sample;

    for (i = 0; i < ss->depth; i++, p += ss->width)
	s_compr_chooser__estimate_row(ss, p);
    /* todo: make decision */
}

/* Uppack data and recognize photo/lineart. */
private void
s_compr_chooser__unpack_and_recognize(stream_compr_chooser_state *const ss, 
				      const byte *data, int length)
{   
    /*
     * Input samples are packed ABCABCABC..., but the sample[] array of
     * unpacked values is stored AAA...BBB...CCC.  i counts samples within
     * a pixel, multiplied by width; j counts pixels.
     */
    uint i = (ss->samples_count % ss->depth) * ss->width;
    uint j = ss->samples_count / ss->depth;
    const byte *p = data;
    int l = length;

    while (l) {
	if (ss->bits_left < 8) {
	    uint k = (sizeof(ss->packed_data) * 8 - ss->bits_left) / 8;

	    k = min(k, l);
	    for (; k; k--, l--, p++, ss->bits_left += 8)
		ss->packed_data = (ss->packed_data << 8) + *p;
	}
	while (ss->bits_left >= ss->bits_per_sample) {
	    uint k = ss->bits_left - ss->bits_per_sample;
	    ulong v = ss->packed_data >> k;

	    ss->packed_data -= (v << k);
	    ss->bits_left -= ss->bits_per_sample;
	    if (ss->bits_per_sample > 8)
		v >>= ss->bits_per_sample - 8;
	    else
		v <<= 8 - ss->bits_per_sample;
	    ss->sample[i + j] = (byte)v;  /* scaled to 0...255 */
	    i += ss->width;
	    if (i >= ss->width * ss->depth)
		i = 0, j++;
	    ss->samples_count++;
	    if (ss->samples_count >= ss->width * ss->depth) {
		s_compr_chooser__recognize(ss);
		ss->packed_data = 0;
		ss->bits_left = 0;
		ss->samples_count = 0;
		i = j = 0;
	    }
	}
    }
}

/* Process a buffer. */
private int
s_compr_chooser_process(stream_state * st, stream_cursor_read * pr,
	     stream_cursor_write * pw, bool last)
{
    stream_compr_chooser_state *const ss = (stream_compr_chooser_state *) st;
    int l = pr->limit - pr->ptr;

    if (ss->width >= 3) /* Can't process narrow images. */
	s_compr_chooser__unpack_and_recognize(ss, pr->ptr + 1, l);
    pr->ptr += l;
    return 0;
}

const stream_template s_compr_chooser_template = {
    &st_compr_chooser_state, s_compr_chooser_init, s_compr_chooser_process, 1, 1,
    s_compr_chooser_release, 0 /* NULL */
};

/* Get choice */
uint 
s_compr_chooser__get_choice(stream_compr_chooser_state *ss, bool force)
{
    ulong plateaus = min(ss->lower_plateaus, ss->upper_plateaus);

    if (ss->choice)
	return ss->choice;
    if (force) {
	if (ss->gradients > plateaus / 12) /* messenger16.pdf, page 3. */
	    return 1; /* photo */
	else if (plateaus / 5000 >= ss->gradients)
	    return 2; /* lineart */
    }
    return 0;
}

/* ---------------- Am image color conversion filter ---------------- */

private_st_image_colors_state();

/* Initialize the state. */
private int
s_image_colors_init(stream_state * st)
{
    stream_image_colors_state *const ss = (stream_image_colors_state *) st;

    ss->width = ss->height = ss->depth = ss->bits_per_sample = 0;
    ss->output_bits_buffer = 0;
    ss->output_bits_buffered = 0;
    ss->output_depth = 1;
    ss->output_component_index = ss->output_depth;
    ss->output_bits_per_sample = 1;
    ss->output_component_bits_written = 0;
    ss->raster = 0;
    ss->row_bits = 0;
    ss->row_bits_passed = 0;
    ss->row_alignment_bytes = 0;
    ss->row_alignment_bytes_left = 0;
    ss->input_component_index = 0;
    ss->input_bits_buffer = 0;
    ss->input_bits_buffered = 0;
    ss->convert_color = 0;
    ss->pcs = 0;
    ss->pdev = 0;
    ss->pis = 0;
    return 0;
}

private int 
s_image_colors_convert_color_to_mask(stream_image_colors_state *ss)
{
    int i, ii;

    for (i = ii = 0; i < ss->depth; i++, ii += 2)
	if (ss->input_color[i] < ss->MaskColor[ii] ||
	    ss->input_color[i] > ss->MaskColor[ii + 1])
	    break;
    ss->output_color[0] = (i < ss->depth ? 1 : 0);
    return 0;
}

private int
s_image_colors_convert_to_device_color(stream_image_colors_state * ss)
{
    gs_client_color cc;
    gx_device_color dc;
    int i, code;
    double v0 = (1 << ss->bits_per_sample) - 1;
    double v1 = (1 << ss->output_bits_per_sample) - 1;

    for (i = 0; i < ss->depth; i++)
	cc.paint.values[i] = ss->input_color[i] * 
		(ss->Decode[i * 2 + 1] - ss->Decode[i * 2]) / v0 + ss->Decode[i * 2];

    code = ss->pcs->type->remap_color(&cc, ss->pcs, &dc, ss->pis,
			      ss->pdev, gs_color_select_texture);
    if (code < 0)
	return code;
    for (i = 0; i < ss->output_depth; i++) {
	uint m = (1 << ss->pdev->color_info.comp_bits[i]) - 1;
	uint w = (dc.colors.pure >> ss->pdev->color_info.comp_shift[i]) & m;

	ss->output_color[i] = (uint)(v1 * w / m + 0.5);
    }
    return 0;
}

/* Set masc colors dimensions. */
void
s_image_colors_set_mask_colors(stream_image_colors_state * ss, uint *MaskColor)
{
    ss->convert_color = s_image_colors_convert_color_to_mask;
    memcpy(ss->MaskColor, MaskColor, ss->depth * sizeof(MaskColor[0]) * 2);
}

/* Set image dimensions. */
void
s_image_colors_set_dimensions(stream_image_colors_state * ss, 
			       int width, int height, int depth, int bits_per_sample)
{
    ss->width = width;
    ss->height = height;
    ss->depth = depth;
    ss->bits_per_sample = bits_per_sample;
    ss->row_bits = bits_per_sample * depth * width;
    ss->raster = bitmap_raster(ss->row_bits);
    ss->row_alignment_bytes = 0; /* (ss->raster * 8 - ss->row_bits) / 8) doesn't work. */
}

void
s_image_colors_set_color_space(stream_image_colors_state * ss, gx_device *pdev,
			       const gs_color_space *pcs, const gs_imager_state *pis,
			       float *Decode)
{
    ss->output_depth = pdev->color_info.num_components;
    ss->output_component_index = ss->output_depth;
    ss->output_bits_per_sample = pdev->color_info.comp_bits[0]; /* Same precision for all components. */
    ss->convert_color = s_image_colors_convert_to_device_color;
    ss->pdev = pdev;
    ss->pcs = pcs;
    ss->pis = pis;
    memcpy(ss->Decode, Decode, ss->depth * sizeof(Decode[0]) * 2);
}


/* Process a buffer. */
private int
s_image_colors_process(stream_state * st, stream_cursor_read * pr,
	     stream_cursor_write * pw, bool last)
{
    stream_image_colors_state *const ss = (stream_image_colors_state *) st;

    for (;;) {
	if (pw->ptr >= pw->limit)
	    return 1;
	if (ss->row_bits_passed >= ss->row_bits) {
	    ss->row_alignment_bytes_left = ss->row_alignment_bytes;
	    ss->input_bits_buffered = 0;
	    ss->input_bits_buffer = 0; /* Just to simplify the debugging. */
	    if (ss->output_bits_buffered) {
		*(++pw->ptr) = ss->output_bits_buffer;
		ss->output_bits_buffered = 0;
		ss->output_bits_buffer = 0;
	    }
	    ss->row_bits_passed = 0;
	    continue;
	}
	if (ss->row_alignment_bytes_left) {
	    uint k = pr->limit - pr->ptr;

	    if (k > ss->row_alignment_bytes_left)
		k = ss->row_alignment_bytes_left;
	    pr->ptr += k;
	    ss->row_alignment_bytes_left -= k;
	    if (pr->ptr >= pr->limit)
		return 0;
	}
	if (ss->output_component_index < ss->output_depth) {
	    for (;ss->output_component_index < ss->output_depth;) {
		uint fitting = (uint)(8 - ss->output_bits_buffered);
		uint v, w, u, n, m;

		if (pw->ptr >= pw->limit)
		    return 1;
		v = ss->output_color[ss->output_component_index];
		n = ss->output_bits_per_sample - ss->output_component_bits_written; /* no. of bits left */
		w = v - ((v >> n) << n); /* the current component without written bits. */
		if (fitting > n)
		    fitting = n; /* no. of bits to write. */
		m = n - fitting; /* no. of bits will left. */
		u = w >> m;  /* bits to write (near lsb). */
		ss->output_bits_buffer |= u << (8 - ss->output_bits_buffered - fitting);
		ss->output_bits_buffered += fitting;
		if (ss->output_bits_buffered >= 8) {
		    *(++pw->ptr) = ss->output_bits_buffer;
		    ss->output_bits_buffered = 0;
		    ss->output_bits_buffer = 0;
		}
		ss->output_component_bits_written += fitting;
		if (ss->output_component_bits_written >= ss->output_bits_per_sample) {
		    ss->output_component_index++;
		    ss->output_component_bits_written = 0;
		}
	    }
	    ss->row_bits_passed += ss->bits_per_sample * ss->depth;
	    continue;
	}
	if (ss->input_bits_buffered < ss->bits_per_sample) {
	    if (pr->ptr >= pr->limit)
		return 0;
	    ss->input_bits_buffer = (ss->input_bits_buffer << 8) | *++pr->ptr;
	    ss->input_bits_buffered += 8;
	    /* fixme: delay shifting the input ptr until input_bits_buffer is cleaned. */
	}
	if (ss->input_bits_buffered >= ss->bits_per_sample) {
	    uint w;

	    ss->input_bits_buffered -= ss->bits_per_sample;
	    ss->input_color[ss->input_component_index] = w = ss->input_bits_buffer >> ss->input_bits_buffered;
	    ss->input_bits_buffer &= ~(w << ss->input_bits_buffered);
	    ss->input_component_index++;
	    if (ss->input_component_index >= ss->depth) {
		int code = ss->convert_color(ss);

		if (code < 0)
		    return ERRC;
		ss->output_component_index = 0;
		ss->input_component_index = 0;
	    }
	}
    }
}

const stream_template s__image_colors_template = {
    &st_stream_image_colors_state, s_image_colors_init, s_image_colors_process, 1, 1,
    NULL, NULL
};


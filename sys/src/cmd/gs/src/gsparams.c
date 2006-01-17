/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsparams.c,v 1.5 2002/06/16 05:48:55 lpd Exp $ */
/* Generic parameter list serializer & expander */

/* Initial version 2/1/98 by John Desrosiers (soho@crl.com) */
/* 11/16/98 L. Peter Deutsch (ghost@aladdin.com) edited to remove names
   put_bytes, put_word which conflicted with other modules */

#include "gx.h"
#include "memory_.h"
#include "gserrors.h"
#include "gsparams.h"

/* ----------- Local Type Decl's ------------ */
typedef struct {
    byte *buf;			/* current buffer ptr */
    byte *buf_end;		/* end of buffer */
    unsigned total_sizeof;	/* current # bytes in buf */
} WriteBuffer;

/* ---------- Forward refs ----------- */
private void
ptr_align_to(
	    const byte ** src,	/* pointer to align */
	    unsigned alignment	/* alignment, must be power of 2 */
	    );
private void
wb_put_word(
	    unsigned source,	/* number to put to buffer */
	    WriteBuffer * dest	/* destination descriptor */
	    );
private void
wb_put_bytes(
	     const byte * source,	/* bytes to put to buffer */
	     unsigned source_sizeof,	/* # bytes to put */
	     WriteBuffer * dest		/* destination descriptor */
	     );
private void
wb_put_alignment(
		 unsigned alignment,	/* alignment to match, must be power 2 */
		 WriteBuffer * dest	/* destination descriptor */
		 );

/* Get word compressed with wb_put_word */
private unsigned		/* decompressed word */
buf_get_word(
	    const byte ** src	/* UPDATES: ptr to src buf ptr */
	    );


/* ------------ Serializer ------------ */
/* Serialize the contents of a gs_param_list (including sub-dicts) */
int				/* ret -ve err, else # bytes needed to represent param list, whether */

/* or not it actually fit into buffer. List was successully */

/* serialized only if if this # is <= supplied buf size. */
gs_param_list_serialize(
			   gs_param_list * list,	/* root of list to serialize */
					/* list MUST BE IN READ MODE */
			   byte * buf,	/* destination buffer (can be 0) */
			   int buf_sizeof	/* # bytes available in buf (can be 0) */
)
{
    int code = 0;
    int temp_code;
    gs_param_enumerator_t key_enum;
    gs_param_key_t key;
    WriteBuffer write_buf;

    write_buf.buf = buf;
    write_buf.buf_end = buf + (buf ? buf_sizeof : 0);
    write_buf.total_sizeof = 0;
    param_init_enumerator(&key_enum);

    /* Each item is serialized as ("word" means compressed word):
     *  word: key sizeof + 1, or 0 if end of list/dict
     *  word: data type(gs_param_type_xxx)
     *  byte[]: key, including trailing \0 
     *  (if simple type)
     *   byte[]: unpacked representation of data
     *  (if simple array or string)
     *   byte[]: unpacked mem image of gs_param_xxx_array structure
     *   pad: to array alignment
     *   byte[]: data associated with array contents
     *  (if string/name array)
     *   byte[]: unpacked mem image of gs_param_string_array structure
     *   pad: to void *
     *   { gs_param_string structure mem image;
     *     data associated with string;
     *   } for each string in array
     *  (if dict/dict_int_keys)
     *   word: # of entries in dict,
     *   pad: to void *
     *   dict entries follow immediately until end-of-dict
     *
     * NB that this format is designed to allow using an input buffer
     * as the direct source of data when expanding a gs_c_param_list
     */
    /* Enumerate all the keys; use keys to get their typed values */
    while ((code = param_get_next_key(list, &key_enum, &key)) == 0) {
	int value_top_sizeof;
	int value_base_sizeof;

	/* Get next datum & put its type & key to buffer */
	gs_param_typed_value value;
	char string_key[256];

	if (sizeof(string_key) < key.size + 1) {
	    code = gs_note_error(gs_error_rangecheck);
	    break;
	}
	memcpy(string_key, key.data, key.size);
	string_key[key.size] = 0;
	if ((code = param_read_typed(list, string_key, &value)) != 0) {
	    code = code > 0 ? gs_note_error(gs_error_unknownerror) : code;
	    break;
	}
	wb_put_word((unsigned)key.size + 1, &write_buf);
	wb_put_word((unsigned)value.type, &write_buf);
	wb_put_bytes((byte *) string_key, key.size + 1, &write_buf);

	/* Put value & its size to buffer */
	value_top_sizeof = gs_param_type_sizes[value.type];
	value_base_sizeof = gs_param_type_base_sizes[value.type];
	switch (value.type) {
	    case gs_param_type_null:
	    case gs_param_type_bool:
	    case gs_param_type_int:
	    case gs_param_type_long:
	    case gs_param_type_float:
		wb_put_bytes((byte *) & value.value, value_top_sizeof, &write_buf);
		break;

	    case gs_param_type_string:
	    case gs_param_type_name:
	    case gs_param_type_int_array:
	    case gs_param_type_float_array:
		wb_put_bytes((byte *) & value.value, value_top_sizeof, &write_buf);
		wb_put_alignment(value_base_sizeof, &write_buf);
		value_base_sizeof *= value.value.s.size;
		wb_put_bytes(value.value.s.data, value_base_sizeof, &write_buf);
		break;

	    case gs_param_type_string_array:
	    case gs_param_type_name_array:
		value_base_sizeof *= value.value.sa.size;
		wb_put_bytes((const byte *)&value.value, value_top_sizeof, &write_buf);
		wb_put_alignment(sizeof(void *), &write_buf);

		wb_put_bytes((const byte *)value.value.sa.data, value_base_sizeof,
			  &write_buf);
		{
		    int str_count;
		    const gs_param_string *sa;

		    for (str_count = value.value.sa.size,
			 sa = value.value.sa.data; str_count-- > 0; ++sa)
			wb_put_bytes(sa->data, sa->size, &write_buf);
		}
		break;

	    case gs_param_type_dict:
	    case gs_param_type_dict_int_keys:
		wb_put_word(value.value.d.size, &write_buf);
		wb_put_alignment(sizeof(void *), &write_buf);

		{
		    int bytes_written =
		    gs_param_list_serialize(value.value.d.list,
					    write_buf.buf,
		     write_buf.buf ? write_buf.buf_end - write_buf.buf : 0);

		    temp_code = param_end_read_dict(list,
						    (const char *)key.data,
						    &value.value.d);
		    if (bytes_written < 0)
			code = bytes_written;
		    else {
			code = temp_code;
			if (bytes_written)
			    wb_put_bytes(write_buf.buf, bytes_written, &write_buf);
		    }
		}
		break;

	    default:
		code = gs_note_error(gs_error_unknownerror);
		break;
	}
	if (code < 0)
	    break;
    }

    /* Write end marker, which is an (illegal) 0 key length */
    if (code >= 0) {
	wb_put_word(0, &write_buf);
	code = write_buf.total_sizeof;
    }
    return code;
}


/* ------------ Expander --------------- */
/* Expand a buffer into a gs_param_list (including sub-dicts) */
int				/* ret -ve err, +ve # of chars read from buffer */
gs_param_list_unserialize(
			     gs_param_list * list,	/* root of list to expand to */
					/* list MUST BE IN WRITE MODE */
			     const byte * buf	/* source buffer */
)
{
    int code = 0;
    const byte *orig_buf = buf;

    do {
	gs_param_typed_value typed;
	gs_param_name key;
	unsigned key_sizeof;
	int value_top_sizeof;
	int value_base_sizeof;
	int temp_code;
	gs_param_type type;

	/* key length, 0 indicates end of data */
	key_sizeof = buf_get_word(&buf);
	if (key_sizeof == 0)	/* end of data */
	    break;

	/* data type */
	type = (gs_param_type) buf_get_word(&buf);

	/* key */
	key = (gs_param_name) buf;
	buf += key_sizeof;

	/* Data values */
	value_top_sizeof = gs_param_type_sizes[type];
	value_base_sizeof = gs_param_type_base_sizes[type];
	typed.type = type;
	if (type != gs_param_type_dict && type != gs_param_type_dict_int_keys) {
	    memcpy(&typed.value, buf, value_top_sizeof);
	    buf += value_top_sizeof;
	}
	switch (type) {
	    case gs_param_type_null:
	    case gs_param_type_bool:
	    case gs_param_type_int:
	    case gs_param_type_long:
	    case gs_param_type_float:
		break;

	    case gs_param_type_string:
	    case gs_param_type_name:
	    case gs_param_type_int_array:
	    case gs_param_type_float_array:
		ptr_align_to(&buf, value_base_sizeof);
		typed.value.s.data = buf;
		typed.value.s.persistent = false;
		buf += typed.value.s.size * value_base_sizeof;
		break;

	    case gs_param_type_string_array:
	    case gs_param_type_name_array:
		ptr_align_to(&buf, sizeof(void *));

		typed.value.sa.data = (const gs_param_string *)buf;
		typed.value.sa.persistent = false;
		buf += typed.value.s.size * value_base_sizeof;
		{
		    int str_count;
		    gs_param_string *sa;

		    for (str_count = typed.value.sa.size,
			 sa = (gs_param_string *) typed.value.sa.data;
			 str_count-- > 0; ++sa) {
			sa->data = buf;
			sa->persistent = false;
			buf += sa->size;
		    }
		}
		break;

	    case gs_param_type_dict:
	    case gs_param_type_dict_int_keys:
		typed.value.d.size = buf_get_word(&buf);
		code = param_begin_write_dict
		    (list, key, &typed.value.d, type == gs_param_type_dict_int_keys);
		if (code < 0)
		    break;
		ptr_align_to(&buf, sizeof(void *));

		code = gs_param_list_unserialize(typed.value.d.list, buf);
		temp_code = param_end_write_dict(list, key, &typed.value.d);
		if (code >= 0) {
		    buf += code;
		    code = temp_code;
		}
		break;

	    default:
		code = gs_note_error(gs_error_unknownerror);
		break;
	}
	if (code < 0)
	    break;
	if (typed.type != gs_param_type_dict && typed.type != gs_param_type_dict_int_keys)
	    code = param_write_typed(list, key, &typed);
    }
    while (code >= 0);

    return code >= 0 ? buf - orig_buf : code;
}


/* ---------- Utility functions -------- */

/* Align a byte pointer on the next Nth byte */
private void
ptr_align_to(
	    const byte ** src,	/* pointer to align */
	    unsigned alignment	/* alignment, must be power of 2 */
)
{
    *src += -(int)ALIGNMENT_MOD(*src, alignment) & (alignment - 1);
}

/* Put compressed word repr to a buffer */
private void
wb_put_word(
	    unsigned source,	/* number to put to buffer */
	    WriteBuffer * dest	/* destination descriptor */
)
{
    do {
	byte chunk = source & 0x7f;

	if (source >= 0x80)
	    chunk |= 0x80;
	source >>= 7;
	++dest->total_sizeof;
	if (dest->buf && dest->buf < dest->buf_end)
	    *dest->buf++ = chunk;
    }
    while (source != 0);
}

/* Put array of bytes to buffer */
private void
wb_put_bytes(
	     const byte * source,	/* bytes to put to buffer */
	     unsigned source_sizeof,	/* # bytes to put */
	     WriteBuffer * dest	/* destination descriptor */
)
{
    dest->total_sizeof += source_sizeof;
    if (dest->buf && dest->buf + source_sizeof <= dest->buf_end) {
	if (dest->buf != source)
	    memcpy(dest->buf, source, source_sizeof);
	dest->buf += source_sizeof;
    }
}

/* Pad destination out to req'd alignment w/zeros */
private void
wb_put_alignment(
		 unsigned alignment,	/* alignment to match, must be power 2 */
		 WriteBuffer * dest	/* destination descriptor */
)
{
    static const byte zero =
    {0};

    while ((dest->total_sizeof & (alignment - 1)) != 0)
	wb_put_bytes(&zero, 1, dest);
}

/* Get word compressed with wb_put_word */
private unsigned		/* decompressed word */
buf_get_word(
	    const byte ** src	/* UPDATES: ptr to src buf ptr */
)
{
    unsigned dest = 0;
    byte chunk;
    unsigned shift = 0;

    do {
	chunk = *(*src)++;
	dest |= (chunk & 0x7f) << shift;
	shift += 7;
    }
    while (chunk & 0x80);

    return dest;
}

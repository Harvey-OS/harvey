/* Copyright (C) 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpdfo.h,v 1.19 2004/06/08 11:43:04 igor Exp $ */
/* Internal definitions for "objects" for pdfwrite driver. */

#ifndef gdevpdfo_INCLUDED
#  define gdevpdfo_INCLUDED

/*
 * This file defines the structures and support procedures for what Adobe
 * calls "Cos objects".  (We don't know what "Cos" stands for, but in our
 * case, it's a good acronym for "Collection and stream".)  We only
 * support arrays, dictionaries, and streams: we merge all other types into a
 * single string type, which holds the printed representation that will be
 * written on the output file, and we only support that type as an element
 * of the collection types.
 *
 * Note that Cos objects are *not* reference-counted.  Objects without
 * an ID are assumed to be referenced from only one place; objects with
 * IDs are managed manually.  We also assume that strings (the keys of
 * dictionaries, and non-object values in arrays and dictionaries) are
 * "owned" by their referencing object.
 */

#include "gsparam.h"

/* Define some needed abstract types. */
#ifndef gx_device_pdf_DEFINED
#  define gx_device_pdf_DEFINED
typedef struct gx_device_pdf_s gx_device_pdf;
#endif

/* ---------------- Structures ---------------- */

/* Define the abstract types if they aren't defined already (in gdevpdfx.h). */
#ifndef cos_types_DEFINED
#  define cos_types_DEFINED
typedef struct cos_object_s cos_object_t;
typedef struct cos_stream_s cos_stream_t;
typedef struct cos_dict_s cos_dict_t;
typedef struct cos_array_s cos_array_t;
typedef struct cos_value_s cos_value_t;
typedef struct cos_object_procs_s cos_object_procs_t;
typedef const cos_object_procs_t *cos_type_t;
#endif

/* Abstract types (defined concretely in gdevpdfo.c) */
typedef struct cos_element_s cos_element_t;
typedef struct cos_stream_piece_s cos_stream_piece_t;

/*
 * Define the object procedures for Cos objects.
 */
/*typedef struct cos_object_s cos_object_t;*/
/*typedef*/ struct cos_object_procs_s {

#define cos_proc_release(proc)\
  void proc(cos_object_t *pco, client_name_t cname)
	cos_proc_release((*release));

#define cos_proc_write(proc)\
  int proc(const cos_object_t *pco, gx_device_pdf *pdev, gs_id object_id)
	cos_proc_write((*write));

#define cos_proc_equal(proc)\
  int proc(const cos_object_t *pco0, const cos_object_t *pco1, gx_device_pdf *pdev)
	cos_proc_equal((*equal));

} /*cos_object_procs_t*/;
/*typedef const cos_object_procs_t *cos_type_t;*/
#define cos_type(pco) ((pco)->cos_procs)

/*
 * Define the generic structure for Cos objects.  Note that all Cos
 * objects have the same structure and type, aside from their elements.
 * This allows us to "mutate" a forward-referenced object into its
 * proper type in place.
 *
 * Note that we distinguish elements from contents.  Arrays and
 * dictionaries have only elements; streams have both elements
 * (additional elements of the stream dictionary) and contents pieces.
 *
 * The is_open member currently has no function other than error checking.
 * It is set to true when the object is created, to false by the CLOSE
 * and EP pdfmarks, and is checked by CLOSE, PUT, and SP.
 *
 * The is_graphics member exists only to ensure that the stream argument
 * of SP was created by BP/EP.
 *
 * The written member records whether the object has been written (copied)
 * into the contents or resource file.
 */
#define cos_object_struct(otype_s, etype)\
struct otype_s {\
    const cos_object_procs_t *cos_procs;	/* must be first */\
    long id;\
    etype *elements;\
    cos_stream_piece_t *pieces;\
    gx_device_pdf *pdev;\
    pdf_resource_t *pres;	/* only for BP/EP XObjects */\
    byte is_open;		/* see above */\
    byte is_graphics;		/* see above */\
    byte written;		/* see above */\
    long length;                /* only for stream objects */\
    stream *input_strm;		/* only for stream objects */\
    /* input_strm is introduced recently for pdfmark. */\
    /* Using this field, psdf_binary_writer_s may be simplified. */\
}
cos_object_struct(cos_object_s, cos_element_t);
#define private_st_cos_object()	/* in gdevpdfo.c */\
  gs_private_st_ptrs5(st_cos_object, cos_object_t, "cos_object_t",\
    cos_object_enum_ptrs, cos_object_reloc_ptrs, elements, pieces,\
    pdev, pres, input_strm)
extern const cos_object_procs_t cos_generic_procs;
#define cos_type_generic (&cos_generic_procs)

/*
 * Define the macro for casting any cos object to type cos_object_t.
 * Using cos_procs ensures that the argument is, in fact, a cos object.
 */
#define COS_OBJECT(pc) ((cos_object_t *)&((pc)->cos_procs))
#define CONST_COS_OBJECT(pc) ((const cos_object_t *)&((pc)->cos_procs))

/*
 * Define the structure for the value of an array or dictionary element.
 * This is where we create the union of composite and scalar types.
 */
/*typedef struct cos_value_s cos_value_t;*/
typedef enum {
    COS_VALUE_SCALAR = 0,	/* heap-allocated string */
    COS_VALUE_CONST,		/* shared (constant) string */
    COS_VALUE_OBJECT,		/* object referenced by # # R */
    COS_VALUE_RESOURCE		/* object referenced by /R# */
} cos_value_type_t;
struct cos_value_s {
    cos_value_type_t value_type;
    union vc_ {
	gs_string chars;	/* SCALAR, CONST */
	cos_object_t *object;	/* OBJECT, RESOURCE */
    } contents;
};
#define private_st_cos_value()	/* in gdevpdfo.c */\
  gs_private_st_composite(st_cos_value, cos_value_t,\
    "cos_value_t", cos_value_enum_ptrs, cos_value_reloc_ptrs)

/*
 * Define Cos arrays, dictionaries, and streams.
 *
 * The elements of arrays are stored sorted in decreasing index order.
 * The elements of dictionaries/streams are not sorted.
 * The contents pieces of streams are stored in reverse order.
 */
    /* array */
typedef struct cos_array_element_s cos_array_element_t;
cos_object_struct(cos_array_s, cos_array_element_t);
extern const cos_object_procs_t cos_array_procs;
#define cos_type_array (&cos_array_procs)
    /* dict */
typedef struct cos_dict_element_s cos_dict_element_t;
cos_object_struct(cos_dict_s, cos_dict_element_t);
extern const cos_object_procs_t cos_dict_procs;
#define cos_type_dict (&cos_dict_procs)
    /* stream */
cos_object_struct(cos_stream_s, cos_dict_element_t);
extern const cos_object_procs_t cos_stream_procs;
#define cos_type_stream (&cos_stream_procs)

/* ---------------- Procedures ---------------- */

/*
 * NOTE: Procedures that include "_c_" in their name do not copy their C
 * string argument(s).  To copy the argument, use the procedure that takes
 * a byte pointer and a length.
 */

/* Create a Cos object. */
cos_object_t *cos_object_alloc(gx_device_pdf *, client_name_t);
cos_array_t *cos_array_alloc(gx_device_pdf *, client_name_t);
cos_array_t *cos_array_from_floats(gx_device_pdf *, const float *, uint,
				   client_name_t);
cos_dict_t *cos_dict_alloc(gx_device_pdf *, client_name_t);
cos_stream_t *cos_stream_alloc(gx_device_pdf *, client_name_t);

/* Get the allocator for a Cos object. */
gs_memory_t *cos_object_memory(const cos_object_t *);
#define COS_OBJECT_MEMORY(pc) cos_object_memory(CONST_COS_OBJECT(pc))

/* Set the type of a generic Cos object. */
int cos_become(cos_object_t *, cos_type_t);

/* Define wrappers for calling the object procedures. */
cos_proc_release(cos_release);
#define COS_RELEASE(pc, cname) cos_release(COS_OBJECT(pc), cname)
cos_proc_write(cos_write);
#define COS_WRITE(pc, pdev) cos_write(CONST_COS_OBJECT(pc), pdev, (pc)->id)

/* Make a value to store into a composite object. */
const cos_value_t *cos_string_value(cos_value_t *, const byte *, uint);
const cos_value_t *cos_c_string_value(cos_value_t *, const char *);
const cos_value_t *cos_object_value(cos_value_t *, cos_object_t *);
#define COS_OBJECT_VALUE(pcv, pc) cos_object_value(pcv, COS_OBJECT(pc))
/* A resource value is an object value referenced as /R#, not # # R. */
const cos_value_t *cos_resource_value(cos_value_t *, cos_object_t *);
#define COS_RESOURCE_VALUE(pcv, pc) cos_resource_value(pcv, COS_OBJECT(pc))

/* Test whether a value is an object */
#define COS_VALUE_IS_OBJECT(pv) ((pv)->value_type >= COS_VALUE_OBJECT)

/*
 * Put an element in / add an element to a Cos object.  The _no_copy
 * procedures assume that the value, if not an object, is a newly allocated,
 * unshared string, allocated by the same allocator as the collection
 * itself, that should not be copied.
 */
    /* array */
int cos_array_put(cos_array_t *, long, const cos_value_t *);
int cos_array_put_no_copy(cos_array_t *, long, const cos_value_t *);
int cos_array_add(cos_array_t *, const cos_value_t *);
int cos_array_add_no_copy(cos_array_t *, const cos_value_t *);
int cos_array_add_c_string(cos_array_t *, const char *);
int cos_array_add_int(cos_array_t *, int);
int cos_array_add_real(cos_array_t *, floatp);
int cos_array_add_object(cos_array_t *, cos_object_t *);
/* add adds at the end, unadd removes the last element */
int cos_array_unadd(cos_array_t *, cos_value_t *);
    /* dict */
int cos_dict_put(cos_dict_t *, const byte *, uint, const cos_value_t *);
int cos_dict_put_no_copy(cos_dict_t *, const byte *, uint,
			 const cos_value_t *);
int cos_dict_put_c_key(cos_dict_t *, const char *, const cos_value_t *);
int cos_dict_put_c_key_string(cos_dict_t *, const char *, const byte *, uint);
int cos_dict_put_c_key_int(cos_dict_t *, const char *, int);
int cos_dict_put_c_key_bool(cos_dict_t *pcd, const char *key, bool value);
int cos_dict_put_c_key_real(cos_dict_t *, const char *, floatp);
int cos_dict_put_c_key_floats(cos_dict_t *, const char *, const float *, uint);
int cos_dict_put_c_key_object(cos_dict_t *, const char *, cos_object_t *);
int cos_dict_put_string(cos_dict_t *, const byte *, uint, const byte *, uint);
int cos_dict_put_string_copy(cos_dict_t *pcd, const char *key, const char *value);
int cos_dict_put_c_strings(cos_dict_t *, const char *, const char *);
/* move all the elements from one dict to another */
int cos_dict_move_all(cos_dict_t *, cos_dict_t *);
    /* stream */
int cos_stream_add(cos_stream_t *, uint);
int cos_stream_add_bytes(cos_stream_t *, const byte *, uint);
int cos_stream_add_stream_contents(cos_stream_t *, stream *);
int cos_stream_release_pieces(cos_stream_t *pcs);
cos_dict_t *cos_stream_dict(cos_stream_t *);

/*
 * Get the first / next element for enumerating an array.  Usage:
 *	const cos_array_element_t *elt = cos_array_element_first(pca);
 *	while (elt) {
 *	    long idx;
 *	    const cos_value_t *pvalue;
 *	    elt = cos_array_element_next(elt, &idx, &pvalue);
 *	    ...
 *	}
 * The order in which the elements are returned is not defined.
 * If the client adds elements to the array during the enumeration,
 * they may or may not be included in the enumeration.
 */
const cos_array_element_t *
    cos_array_element_first(const cos_array_t *);
const cos_array_element_t *
    cos_array_element_next(const cos_array_element_t *, long *,
			   const cos_value_t **);

/* Look up a key in a dictionary. */
const cos_value_t *cos_dict_find(const cos_dict_t *, const byte *, uint);
const cos_value_t *cos_dict_find_c_key(const cos_dict_t *, const char *);

/* Set up a parameter list that writes into a Cos dictionary. */
typedef struct cos_param_list_writer_s {
    gs_param_list_common;
    cos_dict_t *pcd;
    int print_ok;
} cos_param_list_writer_t;
int cos_param_list_writer_init(cos_param_list_writer_t *, cos_dict_t *,
			       int print_ok);

/* Create a stream that writes into a Cos stream. */
/* Closing the stream will free it. */
stream *cos_write_stream_alloc(cos_stream_t *pcs, gx_device_pdf *pdev,
			       client_name_t cname);

/* Get cos stream from pipeline. */
cos_stream_t * cos_stream_from_pipeline(stream *s);
/* Get cos write stream from pipeline. */
stream * cos_write_stream_from_pipeline(stream *s);

/* Write a Cos value on the output. */
int cos_value_write(const cos_value_t *, gx_device_pdf *);

/* Write the elements of a dictionary/stream on the output. */
int cos_dict_elements_write(const cos_dict_t *, gx_device_pdf *);
int cos_stream_elements_write(const cos_stream_t *, gx_device_pdf *); /* = dict_elements_write */
int cos_stream_contents_write(const cos_stream_t *, gx_device_pdf *);

/* Find the total length of a stream. */
long cos_stream_length(const cos_stream_t *pcs);

/* Write/delete definitions of named objects. */
/* This is a special-purpose facility for pdf_close. */
int cos_dict_objects_write(const cos_dict_t *, gx_device_pdf *);
int cos_dict_objects_delete(cos_dict_t *);

/* Write a cos object as a PDF object. */
int cos_write_object(cos_object_t *pco, gx_device_pdf *pdev);
#define COS_WRITE_OBJECT(pc, pdev) cos_write_object(COS_OBJECT(pc), pdev)

/* Free a Cos value owned by a Cos object. */
void cos_value_free(const cos_value_t *, const cos_object_t *, client_name_t);

/* Free a cos object. */
void cos_free(cos_object_t *pco, client_name_t cname);
#define COS_FREE(pc, cname) cos_free(COS_OBJECT(pc), cname)

#endif /* gdevpdfo_INCLUDED */

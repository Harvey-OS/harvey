/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevpdfo.h,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
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
 * dictionaries, and the values in arrays and dictionaries) are "owned"
 * by their referencing object.
 */

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
typedef struct cos_object_procs_s cos_object_procs_t;
typedef const cos_object_procs_t *cos_type_t;
#endif

/*
 * Define the generic structure for elements of arrays and
 * dictionaries/streams.
 */
#define cos_element_common(etype)\
    etype *next
typedef struct cos_element_s cos_element_t;
struct cos_element_s {
    cos_element_common(cos_element_t);
};
#define private_st_cos_element()	/* in gdevpdfo.c */\
  gs_private_st_ptrs1(st_cos_element, cos_element_t, "cos_element_t",\
    cos_element_enum_ptrs, cos_element_reloc_ptrs, next)
#define cos_element_num_ptrs 1

/*
 * Define the structure for a piece of stream contents.
 */
typedef struct cos_stream_piece_s cos_stream_piece_t;
struct cos_stream_piece_s {
    cos_element_common(cos_stream_piece_t);
    long position;		/* in streams file */
    uint size;
};
#define private_st_cos_stream_piece()	/* in gdevpdfo.c */\
  gs_private_st_suffix_add0_local(st_cos_stream_piece, cos_stream_piece_t,\
    "cos_stream_piece_t", cos_element_enum_ptrs, cos_element_reloc_ptrs,\
    st_cos_element)

/*
 * Define the object procedures for Cos objects.
 */
/*typedef struct cos_object_s cos_object_t;*/
/*typedef*/ struct cos_object_procs_s {

#define cos_proc_release(proc)\
  void proc(P3(cos_object_t *pco, gs_memory_t *mem, client_name_t cname))
	cos_proc_release((*release));

#define cos_proc_write(proc)\
  int proc(P2(const cos_object_t *pco, gx_device_pdf *pdev))
	cos_proc_write((*write));

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
 */
#define cos_object_struct(otype_s, etype)\
struct otype_s {\
    const cos_object_procs_t *cos_procs;	/* must be first */\
    long id;\
    etype *elements;\
    cos_stream_piece_t *pieces;\
    bool is_open;		/* see above */\
}
cos_object_struct(cos_object_s, cos_element_t);
#define private_st_cos_object()	/* in gdevpdfo.c */\
  gs_private_st_ptrs2(st_cos_object, cos_object_t, "cos_object_t",\
    cos_object_enum_ptrs, cos_object_reloc_ptrs, elements, pieces)
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
typedef struct cos_value_s {
    bool is_object;
    union vc_ {
	cos_object_t *object;	/* is_object = true */
	gs_string chars;	/* is_object = false */
    } contents;
} cos_value_t;
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
struct cos_array_element_s {
    cos_element_common(cos_array_element_t);
    long index;
    cos_value_t value;
};
#define private_st_cos_array_element()	/* in gdevpdfo.c */\
  gs_private_st_composite(st_cos_array_element, cos_array_element_t,\
    "cos_array_element_t", cos_array_element_enum_ptrs, cos_array_element_reloc_ptrs)
/*typedef*/ cos_object_struct(cos_array_s, cos_array_element_t)
     /*cos_array_t*/;
extern const cos_object_procs_t cos_array_procs;
#define cos_type_array (&cos_array_procs)
    /* dict */
typedef struct cos_dict_element_s cos_dict_element_t;
struct cos_dict_element_s {
    cos_element_common(cos_dict_element_t);
    gs_string key;
    cos_value_t value;
};
#define private_st_cos_dict_element()	/* in gdevpdfo.c */\
  gs_private_st_composite(st_cos_dict_element, cos_dict_element_t,\
    "cos_dict_element_t", cos_dict_element_enum_ptrs, cos_dict_element_reloc_ptrs)
/*typedef*/ cos_object_struct(cos_dict_s, cos_dict_element_t)
     /*cos_dict_t*/;
extern const cos_object_procs_t cos_dict_procs;
#define cos_type_dict (&cos_dict_procs)
    /* stream */
/*typedef*/ cos_object_struct(cos_stream_s, cos_dict_element_t)
    /*cos_stream_t*/;
extern const cos_object_procs_t cos_stream_procs;
#define cos_type_stream (&cos_stream_procs)

/* ---------------- Procedures ---------------- */

/* Create a Cos object. */
cos_object_t *cos_object_alloc(P2(gs_memory_t *, client_name_t));
cos_array_t *cos_array_alloc(P2(gs_memory_t *, client_name_t));
cos_dict_t *cos_dict_alloc(P2(gs_memory_t *, client_name_t));
cos_stream_t *cos_stream_alloc(P2(gs_memory_t *, client_name_t));
int cos_become(P2(cos_object_t *, cos_type_t));

/* Define wrappers for calling the object procedures. */
cos_proc_release(cos_release);
#define COS_RELEASE(pc, mem, cname) cos_release(COS_OBJECT(pc), mem, cname)
cos_proc_write(cos_write);
#define COS_WRITE(pc, pdev) cos_write(CONST_COS_OBJECT(pc), pdev)

/* Make a value to store into a composite object. */
const cos_value_t *cos_string_value(P3(cos_value_t *, const byte *, uint));
const cos_value_t *cos_c_string_value(P2(cos_value_t *, const char *));
const cos_value_t *cos_object_value(P2(cos_value_t *, cos_object_t *));
#define COS_OBJECT_VALUE(pcv, pc) cos_object_value(pcv, COS_OBJECT(pc))

/* Put an element in / add an element to a Cos object. */
    /* array */
int cos_array_put(P4(cos_array_t *, gx_device_pdf *, long, const cos_value_t *));
int cos_array_add(P3(cos_array_t *, gx_device_pdf *, const cos_value_t *));
    /* dict */
int cos_dict_put(P5(cos_dict_t *, gx_device_pdf *, const byte *, uint, const cos_value_t *));
int cos_dict_put_string(P6(cos_dict_t *, gx_device_pdf *, const byte *, uint, const byte *, uint));
int cos_dict_put_c_strings(P4(cos_dict_t *, gx_device_pdf *, const char *, const char *));
    /* stream */
int cos_stream_add(P3(cos_stream_t *, gx_device_pdf *, uint));
int cos_stream_add_since(P3(cos_stream_t *, gx_device_pdf *, long /*start_pos*/));
int cos_stream_add_bytes(P4(cos_stream_t *, gx_device_pdf *, const byte *, uint));
int cos_stream_put(P5(cos_stream_t *, gx_device_pdf *, const byte *, uint, const cos_value_t *)); /* = dict_put */
int cos_stream_put_c_strings(P4(cos_stream_t *, gx_device_pdf *, const char *, const char *)); /* = dict_put_c_strings */

/* Look up a key in a dictionary. */
const cos_value_t *cos_dict_find(P3(const cos_dict_t *, const byte *, uint));

/* Write the elements of a dictionary/stream on the output. */
int cos_dict_elements_write(P2(const cos_dict_t *, gx_device_pdf *));
int cos_stream_elements_write(P2(const cos_stream_t *, gx_device_pdf *)); /* = dict_elements_write */
int cos_stream_contents_write(P2(const cos_stream_t *, gx_device_pdf *));

/* Write a cos object as a PDF object. */
int cos_write_object(P2(const cos_object_t *pco, gx_device_pdf *pdev));
#define COS_WRITE_OBJECT(pc, pdev) cos_write_object(CONST_COS_OBJECT(pc), pdev)

/* Free a cos object. */
void cos_free(P3(cos_object_t *pco, gs_memory_t *mem, client_name_t cname));
#define COS_FREE(pc, mem, cname) cos_free(COS_OBJECT(pc), mem, cname)

#endif /* gdevpdfo_INCLUDED */

/* Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: gdevpdfo.c,v 1.5 2000/09/19 19:00:17 lpd Exp $ */
/* Cos object support */
#include "memory_.h"
#include "string_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsparam.h"
#include "gsutil.h"		/* for bytes_compare */
#include "gdevpdfx.h"
#include "gdevpdfo.h"
#include "strimpl.h"
#include "sa85x.h"
#include "slzwx.h"
#include "sstring.h"
#include "szlibx.h"

#define CHECK(expr)\
  BEGIN if ((code = (expr)) < 0) return code; END

/* ---------------- Structure definitions ---------------- */

/*
 * Define the generic structure for elements of arrays and
 * dictionaries/streams.
 */
#define cos_element_common(etype)\
    etype *next
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
 * Define Cos arrays, dictionaries, and streams.
 */
     /* array */
struct cos_array_element_s {
    cos_element_common(cos_array_element_t);
    long index;
    cos_value_t value;
};
#define private_st_cos_array_element()	/* in gdevpdfo.c */\
  gs_private_st_composite(st_cos_array_element, cos_array_element_t,\
    "cos_array_element_t", cos_array_element_enum_ptrs, cos_array_element_reloc_ptrs)
    /* dict */
struct cos_dict_element_s {
    cos_element_common(cos_dict_element_t);
    gs_string key;
    bool owns_key;	/* if false, key is shared, do not trace or free */
    cos_value_t value;
};
#define private_st_cos_dict_element()	/* in gdevpdfo.c */\
  gs_private_st_composite(st_cos_dict_element, cos_dict_element_t,\
    "cos_dict_element_t", cos_dict_element_enum_ptrs, cos_dict_element_reloc_ptrs)

/* GC descriptors */
private_st_cos_element();
private_st_cos_stream_piece();
private_st_cos_object();
private_st_cos_value();
private_st_cos_array_element();
private_st_cos_dict_element();

/* GC procedures */
private
ENUM_PTRS_WITH(cos_value_enum_ptrs, cos_value_t *pcv) return 0;
 case 0:
    switch (pcv->value_type) {
    case COS_VALUE_SCALAR:
	return ENUM_STRING(&pcv->contents.chars);
    case COS_VALUE_CONST:
	break;
    case COS_VALUE_OBJECT:
    case COS_VALUE_RESOURCE:
	return ENUM_OBJ(pcv->contents.object);
    }
    return 0;
ENUM_PTRS_END
private
RELOC_PTRS_WITH(cos_value_reloc_ptrs, cos_value_t *pcv)
{
    switch (pcv->value_type) {
    case COS_VALUE_SCALAR:
	RELOC_STRING_VAR(pcv->contents.chars);
    case COS_VALUE_CONST:
	break;
    case COS_VALUE_OBJECT:
    case COS_VALUE_RESOURCE:
	RELOC_VAR(pcv->contents.object);
	break;
    }
}
RELOC_PTRS_END
private
ENUM_PTRS_WITH(cos_array_element_enum_ptrs, cos_array_element_t *pcae)
{
    return (index < cos_element_num_ptrs ?
	    ENUM_USING_PREFIX(st_cos_element, 0) :
	    ENUM_USING(st_cos_value, &pcae->value, sizeof(cos_value_t),
		       index - cos_element_num_ptrs));
}
ENUM_PTRS_END
private
RELOC_PTRS_WITH(cos_array_element_reloc_ptrs, cos_array_element_t *pcae)
{
    RELOC_PREFIX(st_cos_element);
    RELOC_USING(st_cos_value, &pcae->value, sizeof(cos_value_t));
}
RELOC_PTRS_END
private
ENUM_PTRS_WITH(cos_dict_element_enum_ptrs, cos_dict_element_t *pcde)
{
    return (index < cos_element_num_ptrs ?
	    ENUM_USING_PREFIX(st_cos_element, 0) :
	    (index -= cos_element_num_ptrs) > 0 ?
	    ENUM_USING(st_cos_value, &pcde->value, sizeof(cos_value_t),
		       index - 1) :
	    pcde->owns_key ? ENUM_STRING(&pcde->key) : ENUM_OBJ(NULL));
}
ENUM_PTRS_END
private
RELOC_PTRS_WITH(cos_dict_element_reloc_ptrs, cos_dict_element_t *pcde)
{
    RELOC_PREFIX(st_cos_element);
    if (pcde->owns_key)
	RELOC_STRING_VAR(pcde->key);
    RELOC_USING(st_cos_value, &pcde->value, sizeof(cos_value_t));
}
RELOC_PTRS_END

/* ---------------- Generic support ---------------- */

/* Initialize a just-allocated cos object. */
private void
cos_object_init(cos_object_t *pco, gx_device_pdf *pdev,
		const cos_object_procs_t *procs)
{
    if (pco) {
	pco->cos_procs = procs;
	pco->id = 0;
	pco->elements = 0;
	pco->pieces = 0;
	pco->pdev = pdev;
	pco->is_open = true;
	pco->is_graphics = false;
	pco->written = false;
    }
}

/* Get the allocator for a Cos object. */
gs_memory_t *
cos_object_memory(const cos_object_t *pco)
{
    return pco->pdev->pdf_memory;
}

/* Change a generic cos object into one of a specific type. */
int
cos_become(cos_object_t *pco, cos_type_t cotype)
{
    if (cos_type(pco) != cos_type_generic)
	return_error(gs_error_typecheck);
    cos_type(pco) = cotype;
    return 0;
}

/* Release a cos object. */
cos_proc_release(cos_release);	/* check prototype */
void
cos_release(cos_object_t *pco, client_name_t cname)
{
    pco->cos_procs->release(pco, cname);
}

/* Free a cos object. */
void
cos_free(cos_object_t *pco, client_name_t cname)
{
    cos_release(pco, cname);
    gs_free_object(cos_object_memory(pco), pco, cname);
}

/* Write a cos object on the output. */
cos_proc_write(cos_write);	/* check prototype */
int
cos_write(const cos_object_t *pco, gx_device_pdf *pdev)
{
    return pco->cos_procs->write(pco, pdev);
}

/* Write a cos object as a PDF object. */
int
cos_write_object(cos_object_t *pco, gx_device_pdf *pdev)
{
    int code;

    if (pco->id == 0 || pco->written)
	return_error(gs_error_Fatal);
    pdf_open_separate(pdev, pco->id);
    code = cos_write(pco, pdev);
    pdf_end_separate(pdev);
    pco->written = true;
    return code;
}

/* Make a value to store into a composite object. */
const cos_value_t *
cos_string_value(cos_value_t *pcv, const byte *data, uint size)
{
    /*
     * It's OK to break const here, because the value will be copied
     * before being stored in the collection.
     */
    pcv->contents.chars.data = (byte *)data;
    pcv->contents.chars.size = size;
    pcv->value_type = COS_VALUE_SCALAR;
    return pcv;
}
const cos_value_t *
cos_c_string_value(cos_value_t *pcv, const char *str)
{
    /*
     * We shouldn't break const here, because the value will not be copied
     * or freed (or traced), but that would require a lot of bothersome
     * casting elsewhere.
     */
    pcv->contents.chars.data = (byte *)str;
    pcv->contents.chars.size = strlen(str);
    pcv->value_type = COS_VALUE_CONST;
    return pcv;
}
const cos_value_t *
cos_object_value(cos_value_t *pcv, cos_object_t *pco)
{
    pcv->contents.object = pco;
    pcv->value_type = COS_VALUE_OBJECT;
    return pcv;
}
const cos_value_t *
cos_resource_value(cos_value_t *pcv, cos_object_t *pco)
{
    pcv->contents.object = pco;
    pcv->value_type = COS_VALUE_RESOURCE;
    return pcv;
}

/* Free a value. */
void
cos_value_free(const cos_value_t *pcv, const cos_object_t *pco,
	       client_name_t cname)
{
    switch (pcv->value_type) {
    case COS_VALUE_SCALAR:
	gs_free_string(cos_object_memory(pco), pcv->contents.chars.data,
		       pcv->contents.chars.size, cname);
    case COS_VALUE_CONST:
	break;
    case COS_VALUE_OBJECT:
	/* Free the object if this is the only reference to it. */
	if (!pcv->contents.object->id)
	    cos_free(pcv->contents.object, cname);
    case COS_VALUE_RESOURCE:
	break;
    }
}

/* Write a value on the output. */
int
cos_value_write(const cos_value_t *pcv, gx_device_pdf *pdev)
{
    switch (pcv->value_type) {
    case COS_VALUE_SCALAR:
    case COS_VALUE_CONST:
	pdf_write_value(pdev, pcv->contents.chars.data,
			pcv->contents.chars.size);
	break;
    case COS_VALUE_RESOURCE:
	pprintld1(pdev->strm, "/R%ld", pcv->contents.object->id);
	break;
    case COS_VALUE_OBJECT: {
	cos_object_t *pco = pcv->contents.object;

	if (!pco->id)
	    return cos_write(pco, pdev);
	pprintld1(pdev->strm, "%ld 0 R", pco->id);
	break;
    }
    default:			/* can't happen */
	return_error(gs_error_Fatal);
    }
    return 0;
}

/* Copy a value if necessary for putting into an array or dictionary. */
private int
cos_copy_element_value(cos_value_t *pcv, gs_memory_t *mem,
		       const cos_value_t *pvalue, bool copy)
{
    *pcv = *pvalue;
    if (pvalue->value_type == COS_VALUE_SCALAR && copy) {
	byte *value_data = gs_alloc_string(mem, pvalue->contents.chars.size,
					   "cos_copy_element_value");

	if (value_data == 0)
	    return_error(gs_error_VMerror);
	memcpy(value_data, pvalue->contents.chars.data,
	       pvalue->contents.chars.size);
	pcv->contents.chars.data = value_data;
    }
    return 0;
}

/* Release a value copied for putting, if the operation fails. */
private void
cos_uncopy_element_value(cos_value_t *pcv, gs_memory_t *mem, bool copy)
{
    if (pcv->value_type == COS_VALUE_SCALAR && copy)
	gs_free_string(mem, pcv->contents.chars.data, pcv->contents.chars.size,
		       "cos_uncopy_element_value");
}

/* ---------------- Specific object types ---------------- */

/* ------ Generic objects ------ */

private cos_proc_release(cos_generic_release);
private cos_proc_write(cos_generic_write);
const cos_object_procs_t cos_generic_procs = {
    cos_generic_release, cos_generic_write
};

cos_object_t *
cos_object_alloc(gx_device_pdf *pdev, client_name_t cname)
{
    gs_memory_t *mem = pdev->pdf_memory;
    cos_object_t *pco =
	gs_alloc_struct(mem, cos_object_t, &st_cos_object, cname);

    cos_object_init(pco, pdev, &cos_generic_procs);
    return pco;
}

private void
cos_generic_release(cos_object_t *pco, client_name_t cname)
{
    /* Do nothing. */
}

private int
cos_generic_write(const cos_object_t *pco, gx_device_pdf *pdev)
{
    return_error(gs_error_Fatal);
}

/* ------ Arrays ------ */

private cos_proc_release(cos_array_release);
private cos_proc_write(cos_array_write);
const cos_object_procs_t cos_array_procs = {
    cos_array_release, cos_array_write
};

cos_array_t *
cos_array_alloc(gx_device_pdf *pdev, client_name_t cname)
{
    gs_memory_t *mem = pdev->pdf_memory;
    cos_array_t *pca =
	gs_alloc_struct(mem, cos_array_t, &st_cos_object, cname);

    cos_object_init((cos_object_t *)pca, pdev, &cos_array_procs);
    return pca;
}

cos_array_t *
cos_array_from_floats(gx_device_pdf *pdev, const float *pf, uint size,
		      client_name_t cname)
{
    cos_array_t *pca = cos_array_alloc(pdev, cname);
    uint i;

    if (pca == 0)
	return 0;
    for (i = 0; i < size; ++i) {
	int code = cos_array_add_real(pca, pf[i]);

	if (code < 0) {
	    COS_FREE(pca, cname);
	    return 0;
	}
    }
    return pca;
}

private void
cos_array_release(cos_object_t *pco, client_name_t cname)
{
    gs_memory_t *mem = cos_object_memory(pco);
    cos_array_t *const pca = (cos_array_t *)pco;
    cos_array_element_t *cur;
    cos_array_element_t *next;

    for (cur = pca->elements; cur; cur = next) {
	next = cur->next;
	cos_value_free(&cur->value, pco, cname);
	gs_free_object(mem, cur, cname);
    }
    pca->elements = 0;
}

private int
cos_array_write(const cos_object_t *pco, gx_device_pdf *pdev)
{
    stream *s = pdev->strm;
    const cos_array_t *const pca = (const cos_array_t *)pco;
    cos_array_element_t *first = cos_array_reorder(pca, NULL);
    cos_array_element_t *pcae;
    uint last_index = 0;

    pputs(s, "[");
    for (pcae = first; pcae; ++last_index, pcae = pcae->next) {
	for (; pcae->index > last_index; ++last_index)
	    pputs(s, "null\n");
	cos_value_write(&pcae->value, pdev);
	pputc(s, '\n');
    }
    DISCARD(cos_array_reorder(pca, first));
    pputs(s, "]");
    return 0;
}

/* Put/add an element in/to an array. */
int
cos_array_put(cos_array_t *pca, long index, const cos_value_t *pvalue)
{
    gs_memory_t *mem = COS_OBJECT_MEMORY(pca);
    cos_value_t value;
    int code = cos_copy_element_value(&value, mem, pvalue, true);

    if (code >= 0) {
	code = cos_array_put_no_copy(pca, index, &value);
	if (code < 0)
	    cos_uncopy_element_value(&value, mem, true);
    }
    return code;
}
int
cos_array_put_no_copy(cos_array_t *pca, long index, const cos_value_t *pvalue)
{
    gs_memory_t *mem = COS_OBJECT_MEMORY(pca);
    cos_array_element_t **ppcae = &pca->elements;
    cos_array_element_t *pcae;
    cos_array_element_t *next;

    while ((next = *ppcae) != 0 && next->index > index)
	ppcae = &next->next;
    if (next && next->index == index) {
	/* We're replacing an existing element. */
	cos_value_free(&next->value, COS_OBJECT(pca),
		       "cos_array_put(old value)");
	pcae = next;
    } else {
	/* Create a new element. */
	pcae = gs_alloc_struct(mem, cos_array_element_t, &st_cos_array_element,
			       "cos_array_put(element)");
	if (pcae == 0)
	    return_error(gs_error_VMerror);
	pcae->index = index;
	pcae->next = next;
	*ppcae = pcae;
    }
    pcae->value = *pvalue;
    return 0;
}
private long
cos_array_next_index(const cos_array_t *pca)
{
    return (pca->elements ? pca->elements->index + 1 : 0L);
}
int
cos_array_add(cos_array_t *pca, const cos_value_t *pvalue)
{
    return cos_array_put(pca, cos_array_next_index(pca), pvalue);
}
int
cos_array_add_no_copy(cos_array_t *pca, const cos_value_t *pvalue)
{
    return cos_array_put_no_copy(pca, cos_array_next_index(pca), pvalue);
}
int
cos_array_add_c_string(cos_array_t *pca, const char *str)
{
    cos_value_t value;

    return cos_array_add(pca, cos_c_string_value(&value, str));
}
int
cos_array_add_int(cos_array_t *pca, int i)
{
    char str[sizeof(int) * 8 / 3 + 3]; /* sign, rounding, 0 terminator */
    cos_value_t v;

    sprintf(str, "%d", i);
    return cos_array_add(pca, cos_string_value(&v, (byte *)str, strlen(str)));
}
int
cos_array_add_real(cos_array_t *pca, floatp r)
{
    byte str[50];		/****** ADHOC ******/
    stream s;
    cos_value_t v;

    swrite_string(&s, str, sizeof(str));
    pprintg1(&s, "%g", r);
    return cos_array_add(pca, cos_string_value(&v, str, stell(&s)));
}
int
cos_array_add_object(cos_array_t *pca, cos_object_t *pco)
{
    cos_value_t value;

    return cos_array_add(pca, cos_object_value(&value, pco));
}

/* Get the first / next element for enumerating an array. */
const cos_array_element_t *
cos_array_element_first(const cos_array_t *pca)
{
    return pca->elements;
}
const cos_array_element_t *
cos_array_element_next(const cos_array_element_t *pca, long *pindex,
		       const cos_value_t **ppvalue)
{
    *pindex = pca->index;
    *ppvalue = &pca->value;
    return pca->next;
}

/*
 * Reorder the elements of an array for writing or after writing.  Usage:
 *	first_element = cos_array_reorder(pca, NULL);
 *	...
 *	cos_array_reorder(pca, first_element);
 */
cos_array_element_t *
cos_array_reorder(const cos_array_t *pca, cos_array_element_t *first)
{
    cos_array_element_t *last;
    cos_array_element_t *next;
    cos_array_element_t *pcae;

    for (pcae = (first ? first : pca->elements), last = NULL; pcae;
	 pcae = next)
	next = pcae->next, pcae->next = last, last = pcae;
    return last;
}

/* ------ Dictionaries ------ */

private cos_proc_release(cos_dict_release);
private cos_proc_write(cos_dict_write);
const cos_object_procs_t cos_dict_procs = {
    cos_dict_release, cos_dict_write
};

cos_dict_t *
cos_dict_alloc(gx_device_pdf *pdev, client_name_t cname)
{
    gs_memory_t *mem = pdev->pdf_memory;
    cos_dict_t *pcd =
	gs_alloc_struct(mem, cos_dict_t, &st_cos_object, cname);

    cos_object_init((cos_object_t *)pcd, pdev, &cos_dict_procs);
    return pcd;
}

private void
cos_dict_release(cos_object_t *pco, client_name_t cname)
{
    gs_memory_t *mem = cos_object_memory(pco);
    cos_dict_t *const pcd = (cos_dict_t *)pco;
    cos_dict_element_t *cur;
    cos_dict_element_t *next;

    for (cur = pcd->elements; cur; cur = next) {
	next = cur->next;
	cos_value_free(&cur->value, pco, cname);
	if (cur->owns_key)
	    gs_free_string(mem, cur->key.data, cur->key.size, cname);
	gs_free_object(mem, cur, cname);
    }
    pcd->elements = 0;
}

/* Write the elements of a dictionary. */
private int
cos_elements_write(stream *s, const cos_dict_element_t *pcde,
		   gx_device_pdf *pdev)
{
    /* Temporarily replace the output stream in pdev. */
    stream *save = pdev->strm;

    pdev->strm = s;
    for (; pcde; pcde = pcde->next) {
	pdf_write_value(pdev, pcde->key.data, pcde->key.size);
	pputc(s, ' ');
	cos_value_write(&pcde->value, pdev);
	pputc(s, '\n');
    }
    pdev->strm = save;
    return 0;
}
int
cos_dict_elements_write(const cos_dict_t *pcd, gx_device_pdf *pdev)
{
    return cos_elements_write(pdev->strm, pcd->elements, pdev);
}

private int
cos_dict_write(const cos_object_t *pco, gx_device_pdf *pdev)
{
    stream *s = pdev->strm;

    pputs(s, "<<");
    cos_dict_elements_write((const cos_dict_t *)pco, pdev);
    pputs(s, ">>");
    return 0;
}

/* Write/delete definitions of named objects. */
/* This is a special-purpose facility for pdf_close. */
int
cos_dict_objects_write(const cos_dict_t *pcd, gx_device_pdf *pdev)
{
    const cos_dict_element_t *pcde = pcd->elements;

    for (; pcde; pcde = pcde->next)
	if (COS_VALUE_IS_OBJECT(&pcde->value) &&
	    pcde->value.contents.object->id)
	    cos_write_object(pcde->value.contents.object, pdev);
    return 0;
}
int
cos_dict_objects_delete(cos_dict_t *pcd)
{
    cos_dict_element_t *pcde = pcd->elements;

    /*
     * Delete the objects' IDs so that freeing the dictionary will
     * free them.
     */
    for (; pcde; pcde = pcde->next)
	pcde->value.contents.object->id = 0;
    return 0;
}

/* Put an element in a dictionary. */
#define DICT_COPY_KEY 1
#define DICT_COPY_VALUE 2
#define DICT_FREE_KEY 4
#define DICT_COPY_ALL (DICT_COPY_KEY | DICT_COPY_VALUE | DICT_FREE_KEY)
private int
cos_dict_put_copy(cos_dict_t *pcd, const byte *key_data, uint key_size,
		  const cos_value_t *pvalue, int flags)
{
    gs_memory_t *mem = COS_OBJECT_MEMORY(pcd);
    cos_dict_element_t **ppcde = &pcd->elements;
    cos_dict_element_t *pcde;
    cos_dict_element_t *next;
    cos_value_t value;
    int code;

    while ((next = *ppcde) != 0 &&
	   bytes_compare(next->key.data, next->key.size, key_data, key_size)
	   )
	ppcde = &next->next;
    if (next) {
	/* We're replacing an existing element. */
	code = cos_copy_element_value(&value, mem, pvalue,
				      (flags & DICT_COPY_VALUE) != 0);
	if (code < 0)
	    return code;
	if (flags & DICT_FREE_KEY)
	    gs_free_const_string(mem, key_data, key_size,
				 "cos_dict_put(new key)");
	cos_value_free(&next->value, COS_OBJECT(pcd),
		       "cos_dict_put(old value)");
	pcde = next;
    } else {
	/* Create a new element. */
	byte *copied_key_data;

	if (flags & DICT_COPY_KEY) {
	    copied_key_data = gs_alloc_string(mem, key_size,
					      "cos_dict_put(key)");
	    if (copied_key_data == 0)
		return_error(gs_error_VMerror);
	    memcpy(copied_key_data, key_data, key_size);
	} else
	    copied_key_data = (byte *)key_data;	/* OK to break const */
	pcde = gs_alloc_struct(mem, cos_dict_element_t, &st_cos_dict_element,
			       "cos_dict_put(element)");
	code = cos_copy_element_value(&value, mem, pvalue,
				      (flags & DICT_COPY_VALUE) != 0);
	if (pcde == 0 || code < 0) {
	    if (code >= 0)
		cos_uncopy_element_value(&value, mem,
					 (flags & DICT_COPY_VALUE) != 0);
	    gs_free_object(mem, pcde, "cos_dict_put(element)");
	    if (flags & DICT_COPY_KEY)
		gs_free_string(mem, copied_key_data, key_size,
			       "cos_dict_put(key)");
	    return (code < 0 ? code : gs_note_error(gs_error_VMerror));
	}
	pcde->key.data = copied_key_data;
	pcde->key.size = key_size;
	pcde->owns_key = (flags & DICT_FREE_KEY) != 0;
	pcde->next = next;
	*ppcde = pcde;
    }
    pcde->value = value;
    return 0;
}
int
cos_dict_put(cos_dict_t *pcd, const byte *key_data, uint key_size,
	     const cos_value_t *pvalue)
{
    return cos_dict_put_copy(pcd, key_data, key_size, pvalue, DICT_COPY_ALL);
}
int
cos_dict_put_no_copy(cos_dict_t *pcd, const byte *key_data, uint key_size,
		     const cos_value_t *pvalue)
{
    return cos_dict_put_copy(pcd, key_data, key_size, pvalue,
			     DICT_COPY_KEY | DICT_FREE_KEY);
}
int
cos_dict_put_c_key(cos_dict_t *pcd, const char *key, const cos_value_t *pvalue)
{
    return cos_dict_put_copy(pcd, (const byte *)key, strlen(key), pvalue,
			     DICT_COPY_VALUE);
}
int
cos_dict_put_c_key_string(cos_dict_t *pcd, const char *key,
			  const byte *data, uint size)
{
    cos_value_t value;

    cos_string_value(&value, data, size);
    return cos_dict_put_c_key(pcd, key, &value);
}
int
cos_dict_put_c_key_int(cos_dict_t *pcd, const char *key, int value)
{
    char str[sizeof(int) * 8 / 3 + 3]; /* sign, rounding, 0 terminator */

    sprintf(str, "%d", value);
    return cos_dict_put_c_key_string(pcd, key, (byte *)str, strlen(str));
}
int
cos_dict_put_c_key_real(cos_dict_t *pcd, const char *key, floatp value)
{
    byte str[50];		/****** ADHOC ******/
    stream s;

    swrite_string(&s, str, sizeof(str));
    pprintg1(&s, "%g", value);
    return cos_dict_put_c_key_string(pcd, key, str, stell(&s));
}
int
cos_dict_put_c_key_floats(cos_dict_t *pcd, const char *key, const float *pf,
			  uint size)
{
    cos_array_t *pca = cos_array_from_floats(pcd->pdev, pf, size,
					     "cos_dict_put_c_key_floats");
    int code;

    if (pca == 0)
	return_error(gs_error_VMerror);
    code = cos_dict_put_c_key_object(pcd, key, COS_OBJECT(pca));
    if (code < 0)
	COS_FREE(pca, "cos_dict_put_c_key_floats");
    return code;
}
int
cos_dict_put_c_key_object(cos_dict_t *pcd, const char *key, cos_object_t *pco)
{
    cos_value_t value;

    return cos_dict_put_c_key(pcd, key, cos_object_value(&value, pco));
}
int
cos_dict_put_string(cos_dict_t *pcd, const byte *key_data, uint key_size,
		    const byte *value_data, uint value_size)
{
    cos_value_t cvalue;

    return cos_dict_put(pcd, key_data, key_size,
			cos_string_value(&cvalue, value_data, value_size));
}
int
cos_dict_put_c_strings(cos_dict_t *pcd, const char *key, const char *value)
{
    cos_value_t cvalue;

    return cos_dict_put_c_key(pcd, key, cos_c_string_value(&cvalue, value));
}

/* Look up a key in a dictionary. */
const cos_value_t *
cos_dict_find(const cos_dict_t *pcd, const byte *key_data, uint key_size)
{
    cos_dict_element_t *pcde = pcd->elements;

    for (; pcde; pcde = pcde->next)
	if (!bytes_compare(key_data, key_size, pcde->key.data, pcde->key.size))
	    return &pcde->value;
    return 0;
}
const cos_value_t *
cos_dict_find_c_key(const cos_dict_t *pcd, const char *key)
{
    return cos_dict_find(pcd, (const byte *)key, strlen(key));
}

/* Set up a parameter list that writes into a Cos dictionary. */

/* We'll implement the other printers later if we have to. */
private param_proc_xmit_typed(cos_param_put_typed);
private const gs_param_list_procs cos_param_list_writer_procs = {
    cos_param_put_typed,
    NULL /* begin_collection */ ,
    NULL /* end_collection */ ,
    NULL /* get_next_key */ ,
    gs_param_request_default,
    gs_param_requested_default
};
private int
cos_param_put_typed(gs_param_list * plist, gs_param_name pkey,
		    gs_param_typed_value * pvalue)
{
    cos_param_list_writer_t *const pclist =
	(cos_param_list_writer_t *)plist;
    gx_device_pdf *pdev = pclist->pcd->pdev;
    gs_memory_t *mem = pclist->memory;
    cos_value_t value;
    cos_array_t *pca;
    int key_len = strlen(pkey);
    byte key_chars[100];		/****** ADHOC ******/
    int code;

    if (key_len > sizeof(key_chars) - 1)
	return_error(gs_error_limitcheck);
    switch (pvalue->type) {
    default: {
	param_printer_params_t ppp;
	printer_param_list_t pplist;
	stream s;
	int len, skip;
	byte *str;

	ppp = param_printer_params_default;
	ppp.prefix = ppp.suffix = ppp.item_prefix = ppp.item_suffix = 0;
	ppp.print_ok = pclist->print_ok;
	s_init_param_printer(&pplist, &ppp, &s);
	swrite_position_only(&s);
	param_write_typed((gs_param_list *)&pplist, "", pvalue);
	len = stell(&s);
	str = gs_alloc_string(mem, len, "cos_param_put(string)");
	if (str == 0)
	    return_error(gs_error_VMerror);
	swrite_string(&s, str, len);
	param_write_typed((gs_param_list *)&pplist, "", pvalue);
	/*
	 * The string starts with an initial / or /<space>, which
	 * we need to remove.
	 */
	skip = (str[1] == ' ' ? 2 : 1);
	memmove(str, str + skip, len - skip);
	str = gs_resize_string(mem, str, len, len - skip,
			       "cos_param_put(string)");
	cos_string_value(&value, str, len - skip);
    }
	break;
    case gs_param_type_int_array: {
	uint i;

	pca = cos_array_alloc(pdev, "cos_param_put(array)");
	if (pca == 0)
	    return_error(gs_error_VMerror);
	for (i = 0; i < pvalue->value.ia.size; ++i)
	    CHECK(cos_array_add_int(pca, pvalue->value.ia.data[i]));
    }
    av:
	cos_object_value(&value, COS_OBJECT(pca));
	break;
    case gs_param_type_float_array: {
	uint i;

	pca = cos_array_alloc(pdev, "cos_param_put(array)");
	if (pca == 0)
	    return_error(gs_error_VMerror);
	for (i = 0; i < pvalue->value.ia.size; ++i)
	    CHECK(cos_array_add_real(pca, pvalue->value.fa.data[i]));
    }
	goto av;
    case gs_param_type_string_array:
    case gs_param_type_name_array:
	/****** NYI ******/
	return_error(gs_error_typecheck);
    }
    memcpy(key_chars + 1, pkey, key_len);
    key_chars[0] = '/';
    return cos_dict_put_no_copy(pclist->pcd, key_chars, key_len + 1, &value);
}

int
cos_param_list_writer_init(cos_param_list_writer_t *pclist, cos_dict_t *pcd,
			   int print_ok)
{
    gs_param_list_init((gs_param_list *)pclist, &cos_param_list_writer_procs,
		       COS_OBJECT_MEMORY(pcd));
    pclist->pcd = pcd;
    pclist->print_ok = print_ok;
    return 0;
}

/* ------ Streams ------ */

private cos_proc_release(cos_stream_release);
private cos_proc_write(cos_stream_write);
const cos_object_procs_t cos_stream_procs = {
    cos_stream_release, cos_stream_write
};

cos_stream_t *
cos_stream_alloc(gx_device_pdf *pdev, client_name_t cname)
{
    gs_memory_t *mem = pdev->pdf_memory;
    cos_stream_t *pcs =
	gs_alloc_struct(mem, cos_stream_t, &st_cos_object, cname);

    cos_object_init((cos_object_t *)pcs, pdev, &cos_stream_procs);
    return pcs;
}

private void
cos_stream_release(cos_object_t *pco, client_name_t cname)
{
    gs_memory_t *mem = cos_object_memory(pco);
    cos_stream_t *const pcs = (cos_stream_t *)pco;
    cos_stream_piece_t *cur;
    cos_stream_piece_t *next;

    for (cur = pcs->pieces; cur; cur = next) {
	next = cur->next;
	gs_free_object(mem, cur, cname);
    }
    pcs->pieces = 0;
    cos_dict_release(pco, cname);
}

/* Find the total length of a stream. */
private long
cos_stream_length(const cos_stream_t *pcs)
{
    const cos_stream_piece_t *pcsp = pcs->pieces;
    long length;

    for (length = 0; pcsp; pcsp = pcsp->next)
	length += pcsp->size;
    return length;
}

/* Write the (dictionary) elements of a stream. */
/* (This procedure is exported.) */
int
cos_stream_elements_write(const cos_stream_t *pcs, gx_device_pdf *pdev)
{
    return cos_elements_write(pdev->strm, pcs->elements, pdev);
}

/* Write the contents of a stream.  (This procedure is exported.) */
int
cos_stream_contents_write(const cos_stream_t *pcs, gx_device_pdf *pdev)
{
    stream *s = pdev->strm;
    cos_stream_piece_t *pcsp;
    cos_stream_piece_t *last;
    cos_stream_piece_t *next;
    FILE *sfile = pdev->streams.file;
    long end_pos;
    int code;

    sflush(pdev->streams.strm);
    end_pos = ftell(sfile);

    /* Reverse the elements temporarily. */
    for (pcsp = pcs->pieces, last = NULL; pcsp; pcsp = next)
	next = pcsp->next, pcsp->next = last, last = pcsp;
    for (pcsp = last, code = 0; pcsp && code >= 0; pcsp = pcsp->next) {
	fseek(sfile, pcsp->position, SEEK_SET);
	pdf_copy_data(s, sfile, pcsp->size);
    }
    /* Reverse the elements back. */
    for (pcsp = last, last = NULL; pcsp; pcsp = next)
	next = pcsp->next, pcsp->next = last, last = pcsp;

    fseek(sfile, end_pos, SEEK_SET);
    return code;
}

private int
cos_stream_write(const cos_object_t *pco, gx_device_pdf *pdev)
{
    stream *s = pdev->strm;
    const cos_stream_t *const pcs = (const cos_stream_t *)pco;
    int code;

    pputs(s, "<<");
    cos_elements_write(s, pcs->elements, pdev);
    pprintld1(s, "/Length %ld>>stream\n", cos_stream_length(pcs));
    code = cos_stream_contents_write(pcs, pdev);
    pputs(s, "\nendstream\n");

    return code;
}

/* Return a stream's dictionary (just a cast). */
cos_dict_t *
cos_stream_dict(cos_stream_t *pcs)
{
    return (cos_dict_t *)pcs;
}

/* Add a contents piece to a stream object: size bytes just written on */
/* streams.strm. */
int
cos_stream_add(cos_stream_t *pcs, uint size)
{
    gx_device_pdf *pdev = pcs->pdev;
    stream *s = pdev->streams.strm;
    long position = stell(s);
    cos_stream_piece_t *prev = pcs->pieces;

    /* Check for consecutive writing -- just an optimization. */
    if (prev != 0 && prev->position + prev->size + size == position) {
	prev->size += size;
    } else {
	gs_memory_t *mem = pdev->pdf_memory;
	cos_stream_piece_t *pcsp =
	    gs_alloc_struct(mem, cos_stream_piece_t, &st_cos_stream_piece,
			    "cos_stream_add");

	if (pcsp == 0)
	    return_error(gs_error_VMerror);
	pcsp->position = position - size;
	pcsp->size = size;
	pcsp->next = pcs->pieces;
	pcs->pieces = pcsp;
    }
    return 0;
}
int
cos_stream_add_since(cos_stream_t *pcs, long start_pos)
{
    return cos_stream_add(pcs,
			  (uint)(stell(pcs->pdev->streams.strm) - start_pos));
}

/* Add bytes to a stream object. */
int
cos_stream_add_bytes(cos_stream_t *pcs, const byte *data, uint size)
{
    pwrite(pcs->pdev->streams.strm, data, size);
    return cos_stream_add(pcs, size);
}

/* Create a stream that writes into a Cos stream. */
/* Closing the stream will free it. */
/* Note that this is not a filter. */
typedef struct cos_write_stream_state_s {
    stream_state_common;
    cos_stream_t *pcs;
    gx_device_pdf *pdev;
    stream *s;			/* pointer back to stream */
    stream *target;		/* use this instead of strm */
} cos_write_stream_state_t;
gs_private_st_suffix_add4(st_cos_write_stream_state, cos_write_stream_state_t,
			  "cos_write_stream_state_t",
			  cos_ws_state_enum_ptrs, cos_ws_state_reloc_ptrs,
			  st_stream_state, pcs, pdev, s, target);
private int
cos_write_stream_process(stream_state * st, stream_cursor_read * pr,
			 stream_cursor_write * ignore_pw, bool last)
{
    uint count = pr->limit - pr->ptr;
    cos_write_stream_state_t *ss = (cos_write_stream_state_t *)st;
    gx_device_pdf *pdev = ss->pdev;
    stream *target = ss->target;
    long start_pos = stell(pdev->streams.strm);
    int code;

    pwrite(target, pr->ptr + 1, count);
    pr->ptr = pr->limit;
    sflush(target);
    code = cos_stream_add_since(ss->pcs, start_pos);
    return (code < 0 ? ERRC : 0);
}
private int
cos_write_stream_close(stream *s)
{
    cos_write_stream_state_t *ss = (cos_write_stream_state_t *)s->state;
    int status;

    sflush(s);
    status = s_close_filters(&ss->target, ss->pdev->streams.strm);
    return (status < 0 ? status : s_std_close(s));
}

private const stream_procs cos_s_procs = {
    s_std_noavailable, s_std_noseek, s_std_write_reset,
    s_std_write_flush, cos_write_stream_close, cos_write_stream_process
};
private const stream_template cos_write_stream_template = {
    &st_cos_write_stream_state, 0, cos_write_stream_process, 1, 1
};
stream *
cos_write_stream_alloc(cos_stream_t *pcs, gx_device_pdf *pdev,
		       client_name_t cname)
{
    gs_memory_t *mem = pdev->pdf_memory;
    stream *s = s_alloc(mem, cname);
    cos_write_stream_state_t *ss = (cos_write_stream_state_t *)
	s_alloc_state(mem, &st_cos_write_stream_state, cname);
#define CWS_BUF_SIZE 512	/* arbitrary */
    byte *buf = gs_alloc_bytes(mem, CWS_BUF_SIZE, cname);

    if (s == 0 || ss == 0 || buf == 0)
	goto fail;
    ss->template = &cos_write_stream_template;
    ss->pcs = pcs;
    ss->pdev = pdev;
    ss->s = s;
    ss->target = pdev->streams.strm; /* not s->strm */
    s_std_init(s, buf, CWS_BUF_SIZE, &cos_s_procs, s_mode_write);
    s->state = (stream_state *)ss;
    return s;
#undef CWS_BUF_SIZE
 fail:
    gs_free_object(mem, buf, cname);
    gs_free_object(mem, ss, cname);
    gs_free_object(mem, s, cname);
    return 0;
}

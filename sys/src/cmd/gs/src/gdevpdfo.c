/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevpdfo.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* Cos object support */
#include "memory_.h"
#include "string_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsutil.h"		/* for bytes_compare */
#include "gdevpdfx.h"
#include "gdevpdfo.h"
#include "strimpl.h"
#include "sstring.h"

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
 case 0: return (pcv->is_object ? ENUM_OBJ(pcv->contents.object) :
		 ENUM_STRING(&pcv->contents.chars));
ENUM_PTRS_END
private
RELOC_PTRS_WITH(cos_value_reloc_ptrs, cos_value_t *pcv)
{
    if (pcv->is_object)
	RELOC_VAR(pcv->contents.object);
    else
	RELOC_STRING_VAR(pcv->contents.chars);
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
	    (index -= cos_element_num_ptrs) == 0 ?
	    ENUM_STRING(&pcde->key) :
	    ENUM_USING(st_cos_value, &pcde->value, sizeof(cos_value_t),
		       index - 1));
}
ENUM_PTRS_END
private
RELOC_PTRS_WITH(cos_dict_element_reloc_ptrs, cos_dict_element_t *pcde)
{
    RELOC_PREFIX(st_cos_element);
    RELOC_STRING_VAR(pcde->key);
    RELOC_USING(st_cos_value, &pcde->value, sizeof(cos_value_t));
}
RELOC_PTRS_END

/* ---------------- Generic support ---------------- */

/* Initialize a just-allocated cos object. */
private void
cos_object_init(cos_object_t *pco, const cos_object_procs_t *procs)
{
    if (pco) {
	pco->cos_procs = procs;
	pco->id = 0;
	pco->elements = 0;
	pco->pieces = 0;
	pco->is_open = true;
    }
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
cos_release(cos_object_t *pco, gs_memory_t *mem, client_name_t cname)
{
    pco->cos_procs->release(pco, mem, cname);
}

/* Free a cos object. */
void
cos_free(cos_object_t *pco, gs_memory_t *mem, client_name_t cname)
{
    cos_release(pco, mem, cname);
    gs_free_object(mem, pco, cname);
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
cos_write_object(const cos_object_t *pco, gx_device_pdf *pdev)
{
    int code;

    if (pco->id == 0)
	return_error(gs_error_Fatal);
    pdf_open_separate(pdev, pco->id);
    code = cos_write(pco, pdev);
    pdf_end_separate(pdev);
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
    pcv->is_object = false;
    return pcv;
}
const cos_value_t *
cos_c_string_value(cos_value_t *pcv, const char *str)
{
    return cos_string_value(pcv, (const byte *)str, strlen(str));
}
const cos_value_t *
cos_object_value(cos_value_t *pcv, cos_object_t *pco)
{
    pcv->contents.object = pco;
    pcv->is_object = true;
    return pcv;
}

/* Free a value. */
private void
cos_value_free(cos_value_t *pcv, gs_memory_t *mem, client_name_t cname)
{
    if (pcv->is_object) {
	/* Free the object if this is the only reference to it. */
	if (!pcv->contents.object->id)
	    cos_free(pcv->contents.object, mem, cname);
    } else
	gs_free_string(mem, pcv->contents.chars.data, pcv->contents.chars.size,
		       cname);
}

/* Write a value on the output. */
private void
cos_value_write(const cos_value_t *pcv, gx_device_pdf *pdev)
{
    if (pcv->is_object) {
	cos_object_t *pco = pcv->contents.object;

	if (pco->id)
	    pprintld1(pdev->strm, "%ld 0 R", pco->id);
	else
	    cos_write(pco, pdev);
    } else
	pdf_write_value(pdev, pcv->contents.chars.data,
			pcv->contents.chars.size);
}

/* Copy a value if necessary for putting into an array or dictionary. */
private int
cos_copy_element_value(cos_value_t *pcv, gs_memory_t *mem,
		       const cos_value_t *pvalue)
{
    *pcv = *pvalue;
    if (!pvalue->is_object) {
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
cos_uncopy_element_value(cos_value_t *pcv, gs_memory_t *mem)
{
    if (!pcv->is_object)
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
cos_object_alloc(gs_memory_t *mem, client_name_t cname)
{
    cos_object_t *pco =
	gs_alloc_struct(mem, cos_object_t, &st_cos_object, cname);

    cos_object_init(pco, &cos_generic_procs);
    return pco;
}

private void
cos_generic_release(cos_object_t *pco, gs_memory_t *mem, client_name_t cname)
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
cos_array_alloc(gs_memory_t *mem, client_name_t cname)
{
    cos_array_t *pca =
	gs_alloc_struct(mem, cos_array_t, &st_cos_object, cname);

    cos_object_init((cos_object_t *)pca, &cos_array_procs);
    return pca;
}

private void
cos_array_release(cos_object_t *pco, gs_memory_t *mem, client_name_t cname)
{
    cos_array_t *const pca = (cos_array_t *)pco;
    cos_array_element_t *cur;
    cos_array_element_t *next;

    for (cur = pca->elements; cur; cur = next) {
	next = cur->next;
	cos_value_free(&cur->value, mem, cname);
	gs_free_object(mem, cur, cname);
    }
    pca->elements = 0;
}

private int
cos_array_write(const cos_object_t *pco, gx_device_pdf *pdev)
{
    stream *s = pdev->strm;
    const cos_array_t *const pca = (const cos_array_t *)pco;
    cos_array_element_t *last;
    cos_array_element_t *next;
    cos_array_element_t *pcae;
    uint last_index = 0;

    pputs(s, "[");
    /* Reverse the elements temporarily. */
    for (pcae = pca->elements, last = NULL; pcae; pcae = next)
	next = pcae->next, pcae->next = last, last = pcae;
    for (pcae = last; pcae; ++last_index, pcae = pcae->next) {
	for (; pcae->index > last_index; ++last_index)
	    pputs(s, "null\n");
	cos_value_write(&pcae->value, pdev);
	pputc(s, '\n');
    }
    /* Reverse the elements back. */
    for (pcae = last, last = NULL; pcae; pcae = next)
	next = pcae->next, pcae->next = last, last = pcae;
    pputs(s, "]");
    return 0;
}

/* Put/add an element in/to an array. */
int
cos_array_put(cos_array_t *pca, gx_device_pdf *pdev, long index,
	      const cos_value_t *pvalue)
{
    gs_memory_t *mem = pdev->pdf_memory;
    cos_array_element_t **ppcae = &pca->elements;
    cos_array_element_t *pcae;
    cos_array_element_t *next;
    cos_value_t value;
    int code;

    code = cos_copy_element_value(&value, mem, pvalue);
    if (code < 0)
	return code;
    while ((next = *ppcae) != 0 && next->index > index)
	ppcae = &next->next;
    if (next && next->index == index) {
	/* We're replacing an existing element. */
	cos_value_free(&next->value, mem, "cos_array_put(old value)");
	pcae = next;
    } else {
	/* Create a new element. */
	pcae = gs_alloc_struct(mem, cos_array_element_t, &st_cos_array_element,
			       "cos_array_put(element)");
	if (pcae == 0) {
	    gs_free_object(mem, pcae, "cos_array_put(element)");
	    cos_uncopy_element_value(&value, mem);
	    return_error(gs_error_VMerror);
	}
	pcae->index = index;
	pcae->next = next;
	*ppcae = pcae;
    }
    pcae->value = value;
    return 0;
}
int
cos_array_add(cos_array_t *pca, gx_device_pdf *pdev, const cos_value_t *pvalue)
{
    return cos_array_put(pca, pdev,
			 (pca->elements ? pca->elements->index + 1 : 0L),
			 pvalue);
}

/* ------ Dictionaries ------ */

private cos_proc_release(cos_dict_release);
private cos_proc_write(cos_dict_write);
const cos_object_procs_t cos_dict_procs = {
    cos_dict_release, cos_dict_write
};

cos_dict_t *
cos_dict_alloc(gs_memory_t *mem, client_name_t cname)
{
    cos_dict_t *pcd =
	gs_alloc_struct(mem, cos_dict_t, &st_cos_object, cname);

    cos_object_init((cos_object_t *)pcd, &cos_dict_procs);
    return pcd;
}

private void
cos_dict_release(cos_object_t *pco, gs_memory_t *mem, client_name_t cname)
{
    cos_dict_t *const pcd = (cos_dict_t *)pco;
    cos_dict_element_t *cur;
    cos_dict_element_t *next;

    for (cur = pcd->elements; cur; cur = next) {
	next = cur->next;
	cos_value_free(&cur->value, mem, cname);
	gs_free_string(mem, cur->key.data, cur->key.size, cname);
	gs_free_object(mem, cur, cname);
    }
    pcd->elements = 0;
}

/* Write the elements of a dictionary.  (This procedure is exported.) */
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

/* Put an element in a dictionary. */
int
cos_dict_put(cos_dict_t *pcd, gx_device_pdf *pdev, const byte *key_data,
	     uint key_size, const cos_value_t *pvalue)
{
    gs_memory_t *mem = pdev->pdf_memory;
    cos_dict_element_t **ppcde = &pcd->elements;
    cos_dict_element_t *pcde;
    cos_dict_element_t *next;
    cos_value_t value;
    int code;

    code = cos_copy_element_value(&value, mem, pvalue);
    if (code < 0)
	return code;
    while ((next = *ppcde) != 0 &&
	   bytes_compare(next->key.data, next->key.size, key_data, key_size)
	   )
	ppcde = &next->next;
    if (next) {
	/* We're replacing an existing element. */
	cos_value_free(&next->value, mem, "cos_dict_put(old value)");
	pcde = next;
    } else {
	/* Create a new element. */
	byte *copied_key_data;

	copied_key_data = gs_alloc_string(mem, key_size, "cos_dict_put(key)");
	pcde = gs_alloc_struct(mem, cos_dict_element_t, &st_cos_dict_element,
			       "cos_dict_put(element)");
	if (copied_key_data == 0 || pcde == 0) {
	    gs_free_object(mem, pcde, "cos_dict_put(element)");
	    if (copied_key_data)
		gs_free_string(mem, copied_key_data, key_size,
			       "cos_dict_put(key)");
	    cos_uncopy_element_value(&value, mem);
	    return_error(gs_error_VMerror);
	}
	pcde->key.data = copied_key_data;
	pcde->key.size = key_size;
	memcpy(copied_key_data, key_data, key_size);
	pcde->next = next;
	*ppcde = pcde;
    }
    pcde->value = value;
    return 0;
}
int
cos_dict_put_string(cos_dict_t *pcd, gx_device_pdf *pdev, const byte *key_data,
		    uint key_size, const byte *value_data, uint value_size)
{
    cos_value_t cvalue;

    return cos_dict_put(pcd, pdev, key_data, key_size,
			cos_string_value(&cvalue, value_data, value_size));
}
int
cos_dict_put_c_strings(cos_dict_t *pcd, gx_device_pdf *pdev,
		       const char *key, const char *value)
{
    cos_value_t cvalue;

    return cos_dict_put(pcd, pdev, (const byte *)key, strlen(key),
			cos_c_string_value(&cvalue, value));
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

/* ------ Streams ------ */

private cos_proc_release(cos_stream_release);
private cos_proc_write(cos_stream_write);
const cos_object_procs_t cos_stream_procs = {
    cos_stream_release, cos_stream_write
};

cos_stream_t *
cos_stream_alloc(gs_memory_t *mem, client_name_t cname)
{
    cos_stream_t *pcs =
	gs_alloc_struct(mem, cos_stream_t, &st_cos_object, cname);

    cos_object_init((cos_object_t *)pcs, &cos_stream_procs);
    return pcs;
}

private void
cos_stream_release(cos_object_t *pco, gs_memory_t *mem, client_name_t cname)
{
    cos_stream_t *const pcs = (cos_stream_t *)pco;
    cos_stream_piece_t *cur;
    cos_stream_piece_t *next;

    for (cur = pcs->pieces; cur; cur = next) {
	next = cur->next;
	gs_free_object(mem, cur, cname);
    }
    pcs->pieces = 0;
    cos_dict_release(pco, mem, cname);
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

    /****** DOESN'T ENCODE OR COMPRESS YET ******/

    pputs(s, "<<");
    cos_elements_write(s, pcs->elements, pdev);
    pprintld1(s, "/Length %ld>>stream\n", cos_stream_length(pcs));
    code = cos_stream_contents_write(pcs, pdev);
    pputs(s, "endstream\n");

    return code;
}

/* Put an element in a stream's dictionary. */
int
cos_stream_put(cos_stream_t *pcs, gx_device_pdf *pdev, const byte *key_data,
	       uint key_size, const cos_value_t *pvalue)
{
    return cos_dict_put((cos_dict_t *)pcs, pdev, key_data, key_size, pvalue);
}
int
cos_stream_put_c_strings(cos_stream_t *pcs, gx_device_pdf *pdev,
			 const char *key, const char *value)
{
    return cos_dict_put_c_strings((cos_dict_t *)pcs, pdev, key, value);
}

/* Add a contents piece to a stream object: size bytes just written on */
/* streams.strm. */
int
cos_stream_add(cos_stream_t *pcs, gx_device_pdf * pdev, uint size)
{
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
cos_stream_add_since(cos_stream_t *pcs, gx_device_pdf * pdev, long start_pos)
{
    return cos_stream_add(pcs, pdev,
			  (uint)(stell(pdev->streams.strm) - start_pos));
}

/* Add bytes to a stream object. */
int
cos_stream_add_bytes(cos_stream_t *pcs, gx_device_pdf * pdev,
		     const byte *data, uint size)
{
    pwrite(pdev->streams.strm, data, size);
    return cos_stream_add(pcs, pdev, size);
}

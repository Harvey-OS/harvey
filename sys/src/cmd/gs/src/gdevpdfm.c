/* Copyright (C) 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevpdfm.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* pdfmark processing for PDF-writing driver */
#include "math_.h"
#include "memory_.h"
#include "string_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsutil.h"		/* for bytes_compare */
#include "gdevpdfx.h"
#include "gdevpdfo.h"
#include "scanchar.h"

/* GC descriptors */
private_st_pdf_article();
private_st_pdf_graphics_save();

/*
 * The pdfmark pseudo-parameter indicates the occurrence of a pdfmark
 * operator in the input file.  Its "value" is the arguments of the operator,
 * passed through essentially unchanged:
 *      (key, value)*, CTM, type
 */

/*
 * Define an entry in a table of pdfmark-processing procedures.
 * (The actual table is at the end of this file, to avoid the need for
 * forward declarations for the procedures.)
 */
#define PDFMARK_NAMEABLE 1	/* allows _objdef */
#define PDFMARK_ODD_OK 2	/* OK if odd # of parameters */
#define PDFMARK_KEEP_NAME 4	/* don't substitute reference for name */
				/* in 1st argument */
#define PDFMARK_NO_REFS 8	/* don't substitute references for names */
				/* anywhere */
typedef struct pdfmark_name_s {
    const char *mname;
    pdfmark_proc((*proc));
    byte options;
} pdfmark_name;

/* ---------------- Public utilities ---------------- */

/* Compare a C string and a gs_param_string. */
bool
pdf_key_eq(const gs_param_string * pcs, const char *str)
{
    return (strlen(str) == pcs->size &&
	    !strncmp(str, (const char *)pcs->data, pcs->size));
}

/* Scan an integer out of a parameter string. */
int
pdfmark_scan_int(const gs_param_string * pstr, int *pvalue)
{
#define max_int_str 20
    uint size = pstr->size;
    char str[max_int_str + 1];

    if (size > max_int_str)
	return_error(gs_error_limitcheck);
    memcpy(str, pstr->data, size);
    str[size] = 0;
    return (sscanf(str, "%d", pvalue) == 1 ? 0 :
	    gs_note_error(gs_error_rangecheck));
#undef max_int_str
}

/* ---------------- Private utilities ---------------- */

/* Find a key in a dictionary. */
private bool
pdfmark_find_key(const char *key, const gs_param_string * pairs, uint count,
		 gs_param_string * pstr)
{
    uint i;

    for (i = 0; i < count; i += 2)
	if (pdf_key_eq(&pairs[i], key)) {
	    *pstr = pairs[i + 1];
	    return true;
	}
    pstr->data = 0;
    pstr->size = 0;
    return false;
}

/*
 * Get the page number for a page referenced by number or as /Next or /Prev.
 * The result may be 0 if the page number is 0 or invalid.
 */
private int
pdfmark_page_number(gx_device_pdf * pdev, const gs_param_string * pnstr)
{
    int page = pdev->next_page + 1;

    if (pnstr->data == 0);
    else if (pdf_key_eq(pnstr, "/Next"))
	++page;
    else if (pdf_key_eq(pnstr, "/Prev"))
	--page;
    else if (pdfmark_scan_int(pnstr, &page) < 0)
	page = 0;
    return page;
}

/* Construct a destination string specified by /Page and/or /View. */
/* Return 0 if none (but still fill in a default), 1 or 2 if present */
/* (1 if only one of /Page or /View, 2 if both), <0 if error. */
private int
pdfmark_make_dest(char dstr[MAX_DEST_STRING], gx_device_pdf * pdev,
		  const char *Page_key, const char *View_key,
		  const gs_param_string * pairs, uint count)
{
    gs_param_string page_string, view_string;
    int present =
	pdfmark_find_key(Page_key, pairs, count, &page_string) +
	pdfmark_find_key(View_key, pairs, count, &view_string);
    int page = pdfmark_page_number(pdev, &page_string);
    gs_param_string action;
    int len;

    if (view_string.size == 0)
	param_string_from_string(view_string, "[/XYZ 0 0 1]");
    if (page == 0)
	strcpy(dstr, "[null ");
    else if (pdfmark_find_key("/Action", pairs, count, &action) &&
	     pdf_key_eq(&action, "/GoToR")
	)
	sprintf(dstr, "[%d ", page - 1);
    else
	sprintf(dstr, "[%ld 0 R ", pdf_page_id(pdev, page));
    len = strlen(dstr);
    if (len + view_string.size > MAX_DEST_STRING)
	return_error(gs_error_limitcheck);
    if (view_string.data[0] != '[' ||
	view_string.data[view_string.size - 1] != ']'
	)
	return_error(gs_error_rangecheck);
    memcpy(dstr + len, view_string.data + 1, view_string.size - 1);
    dstr[len + view_string.size - 1] = 0;
    return present;
}

/*
 * If a named destination is specified by a string, convert it to a name,
 * update *dstr, and return 1; otherwise return 0.
 */
private int
pdfmark_coerce_dest(gs_param_string *dstr, char dest[MAX_DEST_STRING])
{
    const byte *data = dstr->data;
    uint size = dstr->size;

    if (size == 0 || data[0] != '(')
	return 0;
    /****** HANDLE ESCAPES ******/
    memcpy(dest, data, size - 1);
    dest[0] = '/';
    dest[size - 1] = 0;
    dstr->data = (byte *)dest;
    dstr->size = size - 1;
    return 1;
}

/* Put pairs in a dictionary. */
private int
pdfmark_put_c_pair(gx_device_pdf * pdev, cos_dict_t *pcd,
		   const char *key, const gs_param_string * pvalue)
{
    return cos_dict_put_string(pcd, pdev, (const byte *)key, strlen(key),
			       pvalue->data, pvalue->size);
}
private int
pdfmark_put_pair(gx_device_pdf * pdev, cos_dict_t *pcd,
		 const gs_param_string * pair)
{
    return cos_dict_put_string(pcd, pdev, pair->data, pair->size,
			       pair[1].data, pair[1].size);
}

/* Scan a Rect value. */
private int
pdfmark_scan_rect(gs_rect * prect, const gs_param_string * str,
		  const gs_matrix * pctm)
{
    uint size = str->size;
    double v[4];
#define MAX_RECT_STRING 100
    char chars[MAX_RECT_STRING + 3];
    int end_check;

    if (str->size > MAX_RECT_STRING)
	return_error(gs_error_limitcheck);
    memcpy(chars, str->data, size);
    strcpy(chars + size, " 0");
    if (sscanf(chars, "[%lg %lg %lg %lg]%d",
	       &v[0], &v[1], &v[2], &v[3], &end_check) != 5
	)
	return_error(gs_error_rangecheck);
    gs_point_transform(v[0], v[1], pctm, &prect->p);
    gs_point_transform(v[2], v[3], pctm, &prect->q);
    return 0;
}

/* Make a Rect value. */
private void
pdfmark_make_rect(char str[MAX_RECT_STRING], const gs_rect * prect)
{
    /*
     * We have to use a stream and pprintf, rather than sprintf,
     * because printf formats can't express the PDF restriction son
     * the form of the output.
     */
    stream s;

    swrite_string(&s, (byte *)str, MAX_RECT_STRING - 1);
    pprintg4(&s, "[%g %g %g %g]",
	     prect->p.x, prect->p.y, prect->q.x, prect->q.y);
    str[stell(&s)] = 0;
}

/* Write a transformed Border value on a stream. */
private int
pdfmark_write_border(stream *s, const gs_param_string *str,
		     const gs_matrix *pctm)
{
    /*
     * We don't preserve the entire CTM in the output, and it isn't clear
     * what CTM is applicable to annotations anyway: we only attempt to
     * handle well-behaved CTMs here.
     */
    uint size = str->size;
#define MAX_BORDER_STRING 100
    char chars[MAX_BORDER_STRING + 1];
    double bx, by, c;
    gs_point bpt, cpt;
    const char *next;

    if (str->size > MAX_BORDER_STRING)
	return_error(gs_error_limitcheck);
    memcpy(chars, str->data, size);
    chars[size] = 0;
    if (sscanf(chars, "[%lg %lg %lg", &bx, &by, &c) != 3)
	return_error(gs_error_rangecheck);
    gs_distance_transform(bx, by, pctm, &bpt);
    gs_distance_transform(0.0, c, pctm, &cpt);
    pprintg3(s, "[%g %g %g", fabs(bpt.x), fabs(bpt.y), fabs(cpt.x + cpt.y));
    /*
     * We don't attempt to do 100% reliable syntax checking here --
     * it's just not worth the trouble.
     */
    next = strchr(chars + 1, ']');
    if (next == 0)
	return_error(gs_error_rangecheck);
    if (next[1] != 0) {
	/* Handle a dash array.  This is tiresome. */
	double v;

	pputc(s, '[');
	while (next != 0 && sscanf(++next, "%lg", &v) == 1) {
	    gs_point vpt;

	    gs_distance_transform(0.0, v, pctm, &vpt);
	    pprintg1(s, "%g ", fabs(vpt.x + vpt.y));
	    next = strchr(next, ' ');
	}
	pputc(s, ']');
    }
    pputc(s, ']');
    return 0;
}

/* ---------------- Miscellaneous pdfmarks ---------------- */

/*
 * Create the dictionary for an annotation or outline.  For some
 * unfathomable reason, PDF requires the following key substitutions
 * relative to pdfmarks:
 *   In annotation and link dictionaries:
 *     /Action => /A, /Color => /C, /Title => /T
 *   In outline directionaries:
 *     /Action => /A, but *not* /Color or /Title
 *   In Action subdictionaries:
 *     /Dest => /D, /File => /F, /Subtype => /S
 * and also the following substitutions:
 *     /Action /Launch /File xxx =>
 *       /A << /S /Launch /F xxx >>
 *     /Action /GoToR /File xxx /Dest yyy =>
 *       /A << /S /GoToR /F xxx /D yyy' >>
 *     /Action /Article /Dest yyy =>
 *       /A << /S /Thread /D yyy' >>
 *     /Action /GoTo => drop the Action key
 * Also, \n in Contents strings must be replaced with \r.
 * Note that for Thread actions, the Dest is not a real destination,
 * and must not be processed as one.
 *
 * We always treat /A and /F as equivalent to /Action and /File
 * respectively.  The pdfmark and PDF documentation is so confused on the
 * issue of when the long and short names should be used that we only give
 * this a 50-50 chance of being right.
 *
 * Note that we must transform Rect and Border coordinates.
 */

typedef struct ao_params_s {
    gx_device_pdf *pdev;	/* for pdfmark_make_dest */
    const char *subtype;	/* default Subtype in top-level dictionary */
    pdf_resource_t *pres;		/* resource for saving source page */
} ao_params_t;
private int
pdfmark_put_ao_pairs(gx_device_pdf * pdev, cos_dict_t *pcd,
		     const gs_param_string * pairs, uint count,
		     const gs_matrix * pctm, ao_params_t * params,
		     bool for_outline)
{
    const gs_param_string *Action = 0;
    const gs_param_string *File = 0;
    gs_param_string Dest;
    gs_param_string Subtype;
    gs_memory_t *mem = pdev->pdf_memory;
    uint i;
    int code;
    char dest[MAX_DEST_STRING];
    bool coerce_dest = false;

    Dest.data = 0;
    if (!for_outline) {
	code = pdfmark_make_dest(dest, params->pdev, "/Page", "/View",
				 pairs, count);
	if (code < 0)
	    return code;
	else if (code == 0)
	    Dest.data = 0;
	else
	    param_string_from_string(Dest, dest);
    }
    if (params->subtype)
	param_string_from_string(Subtype, params->subtype);
    else
	Subtype.data = 0;
    for (i = 0; i < count; i += 2) {
	const gs_param_string *pair = &pairs[i];
	long src_pg;

	if (params->pres != 0 && pdf_key_eq(pair, "/SrcPg") &&
	    sscanf((const char *)pair[1].data, "%ld", &src_pg) == 1
	    )
	    params->pres->rid = src_pg - 1;
	else if (!for_outline && pdf_key_eq(pair, "/Color"))
	    pdfmark_put_c_pair(pdev, pcd, "/C", pair + 1);
	else if (!for_outline && pdf_key_eq(pair, "/Title"))
	    pdfmark_put_c_pair(pdev, pcd, "/T", pair + 1);
	else if (pdf_key_eq(pair, "/Action") || pdf_key_eq(pair, "/A"))
	    Action = pair;
	else if (pdf_key_eq(pair, "/File") || pdf_key_eq(pair, "/F"))
	    File = pair;
	else if (pdf_key_eq(pair, "/Dest")) {
	    Dest = pair[1];
	    coerce_dest = true;
	}
	else if (pdf_key_eq(pair, "/Page") || pdf_key_eq(pair, "/View")) {
	    /* Make a destination even if this is for an outline. */
	    if (Dest.data == 0) {
		code = pdfmark_make_dest(dest, params->pdev, "/Page", "/View",
					 pairs, count);
		if (code < 0)
		    return code;
		param_string_from_string(Dest, dest);
		coerce_dest = false;
	    }
	} else if (pdf_key_eq(pair, "/Subtype"))
	    Subtype = pair[1];
	/*
	 * We also have to replace all occurrences of \n in Contents
	 * strings with \r.  Unfortunately, they probably have already
	 * been converted to \012....
	 */
	else if (pdf_key_eq(pair, "/Contents")) {
	    byte *cstr;
	    uint csize = pair[1].size;
	    cos_value_t *pcv;
	    uint i, j;

	    /*
	     * Copy the string into value storage, then update it in place.
	     */
	    pdfmark_put_pair(pdev, pcd, pair);
	    /* Break const so we can update the (copied) string. */
	    pcv = (cos_value_t *)
		cos_dict_find(pcd, (const byte *)"/Contents", 9);
	    cstr = pcv->contents.chars.data;
	    /* Loop invariant: j <= i < csize. */
	    for (i = j = 0; i < csize;)
		if (csize - i >= 2 && !memcmp(cstr + i, "\\n", 2) &&
		    (i == 0 || cstr[i - 1] != '\\')
		    ) {
		    cstr[j] = '\\', cstr[j + 1] = 'r';
		    i += 2, j += 2;
		} else if (csize - i >= 4 && !memcmp(cstr + i, "\\012", 4) &&
			   (i == 0 || cstr[i - 1] != '\\')
		    ) {
		    cstr[j] = '\\', cstr[j + 1] = 'r';
		    i += 4, j += 2;
		} else
		    cstr[j++] = cstr[i++];
	    if (j != i)
		pcv->contents.chars.data =
		    gs_resize_string(pdev->pdf_memory, cstr, csize, j,
				     "pdfmark_put_ao_pairs");
	} else if (pdf_key_eq(pair, "/Rect")) {
	    gs_rect rect;
	    char rstr[MAX_RECT_STRING];
	    int code = pdfmark_scan_rect(&rect, pair + 1, pctm);

	    if (code < 0)
		return code;
	    pdfmark_make_rect(rstr, &rect);
	    cos_dict_put_c_strings(pcd, pdev, "/Rect", rstr);
	} else if (pdf_key_eq(pair, "/Border")) {
	    stream s;
	    char bstr[MAX_BORDER_STRING + 1];
	    int code;

	    swrite_string(&s, (byte *)bstr, MAX_BORDER_STRING + 1);
	    code = pdfmark_write_border(&s, pair + 1, pctm);
	    if (code < 0)
		return code;
	    if (stell(&s) > MAX_BORDER_STRING)
		return_error(gs_error_limitcheck);
	    bstr[stell(&s)] = 0;
	    cos_dict_put_c_strings(pcd, pdev, "/Border", bstr);
	} else if (for_outline && pdf_key_eq(pair, "/Count"))
	    DO_NOTHING;
	else
	    pdfmark_put_pair(pdev, pcd, pair);
    }
    if (!for_outline && pdf_key_eq(&Subtype, "/Link")) {
	if (Action) {
	    /* Don't delete the Dest for GoTo or file-GoToR. */
	    if (pdf_key_eq(Action + 1, "/GoTo") ||
		(File && pdf_key_eq(Action + 1, "/GoToR"))
		)
		DO_NOTHING;
	    else
		Dest.data = 0;
	}
    }

    /* Now handle the deferred keys. */
    if (Action) {
	const byte *astr = Action[1].data;
	const uint asize = Action[1].size;

	if ((File != 0 || Dest.data != 0) &&
	    (pdf_key_eq(Action + 1, "/Launch") ||
	     (pdf_key_eq(Action + 1, "/GoToR") && File) ||
	     pdf_key_eq(Action + 1, "/Article"))
	    ) {
	    cos_dict_t *adict = cos_dict_alloc(mem, "action dict");
	    cos_value_t avalue;

	    if (adict == 0)
		return_error(gs_error_VMerror);
	    if (!for_outline) {
		/* We aren't sure whether this is really needed.... */
		cos_dict_put_c_strings(adict, pdev, "/Type", "/Action");
	    }
	    if (pdf_key_eq(Action + 1, "/Article")) {
		cos_dict_put_c_strings(adict, pdev, "/S", "/Thread");
		coerce_dest = false; /* Dest is not a real destination */
	    }
	    else
		pdfmark_put_c_pair(pdev, adict, "/S", Action + 1);
	    if (Dest.data) {
		if (coerce_dest)
		    pdfmark_coerce_dest(&Dest, dest);
		pdfmark_put_c_pair(pdev, adict, "/D", &Dest);
		Dest.data = 0;	/* so we don't write it again */
	    }
	    if (File) {
		pdfmark_put_c_pair(pdev, adict, "/F", File + 1);
		File = 0;	/* so we don't write it again */
	    }
	    cos_dict_put(pcd, pdev, (const byte *)"/A", 2,
			 COS_OBJECT_VALUE(&avalue, adict));
	} else if (asize >= 4 && !memcmp(astr, "<<", 2)) {
	    /* Replace occurrences of /Dest, /File, and /Subtype. */
	    const byte *scan = astr + 2;
	    const byte *end = astr + asize;
	    gs_param_string key, value;
	    cos_dict_t *adict = cos_dict_alloc(mem, "action dict");
	    cos_value_t avalue;
	    int code;

	    if (adict == 0)
		return_error(gs_error_VMerror);
	    while ((code = pdf_scan_token(&scan, end, &key.data)) > 0) {
		key.size = scan - key.data;
		if (key.data[0] != '/' ||
		    (code = pdf_scan_token_composite(&scan, end, &value.data)) != 1)
		    break;
		value.size = scan - value.data;
		if (pdf_key_eq(&key, "/Dest") || pdf_key_eq(&key, "/D")) {
		    param_string_from_string(key, "/D");
		    if (value.data[0] == '(') {
			/****** HANDLE ESCAPES ******/
			pdfmark_coerce_dest(&value, dest);
		    }
		} else if (pdf_key_eq(&key, "/File"))
		    param_string_from_string(key, "/F");
		else if (pdf_key_eq(&key, "/Subtype"))
		    param_string_from_string(key, "/S");
		cos_dict_put_string(adict, pdev, key.data, key.size,
				    value.data, value.size);
	    }
	    if (code <= 0 || !pdf_key_eq(&key, ">>"))
		return_error(gs_error_rangecheck);
	    cos_dict_put(pcd, pdev, (const byte *)"/A", 2,
			 COS_OBJECT_VALUE(&avalue, adict));
	} else if (pdf_key_eq(Action + 1, "/GoTo"))
	    pdfmark_put_pair(pdev, pcd, Action);
    }
    /*
     * If we have /Dest or /File without the right kind of action,
     * simply write it at the top level.  This doesn't seem right,
     * but I'm not sure what else to do.
     */
    if (Dest.data) {
	if (coerce_dest)
	    pdfmark_coerce_dest(&Dest, dest);
	pdfmark_put_c_pair(pdev, pcd, "/Dest", &Dest);
    }
    if (File)
	pdfmark_put_pair(pdev, pcd, File);
    if (Subtype.data)
	pdfmark_put_c_pair(pdev, pcd, "/Subtype", &Subtype);
    return 0;
}

/* Copy an annotation dictionary. */
private int
pdfmark_annot(gx_device_pdf * pdev, gs_param_string * pairs, uint count,
	      const gs_matrix * pctm, const gs_param_string *objname,
	      const char *subtype)
{
    ao_params_t params;
    cos_dict_t *pcd;
    int page_index = pdev->next_page;
    cos_array_t *annots;
    cos_value_t value;
    int code;

    if (pdf_page_id(pdev, page_index + 1) <= 0)
	return_error(gs_error_rangecheck);
    annots = pdev->pages[page_index].Annots;
    if (annots == 0) {
	annots = cos_array_alloc(pdev->pdf_memory, "pdfmark_annot");
	if (annots == 0)
	    return_error(gs_error_VMerror);
	pdev->pages[page_index].Annots = annots;
    }
    params.pdev = pdev;
    params.subtype = subtype;
    params.pres = 0;
    code = pdf_make_named_dict(pdev, objname, &pcd, true);
    if (code < 0)
	return code;
    code = cos_dict_put_c_strings(pcd, pdev, "/Type", "/Annot");
    if (code < 0)
	return code;
    code = pdfmark_put_ao_pairs(pdev, pcd, pairs, count, pctm, &params, false);
    if (code < 0)
	return code;
    if (!objname) {
	/* Write the annotation now. */
	COS_WRITE_OBJECT(pcd, pdev);
	COS_RELEASE(pcd, pdev->pdf_memory, "pdfmark_annot");
    }
    return cos_array_add(annots, pdev,
			 cos_object_value(&value, COS_OBJECT(pcd)));
}

/* ANN pdfmark */
private int
pdfmark_ANN(gx_device_pdf * pdev, gs_param_string * pairs, uint count,
	    const gs_matrix * pctm, const gs_param_string * objname)
{
    return pdfmark_annot(pdev, pairs, count, pctm, objname, "/Text");
}

/* LNK pdfmark (obsolescent) */
private int
pdfmark_LNK(gx_device_pdf * pdev, gs_param_string * pairs, uint count,
	    const gs_matrix * pctm, const gs_param_string * objname)
{
    return pdfmark_annot(pdev, pairs, count, pctm, objname, "/Link");
}

/* Write and release one node of the outline tree. */
private int
pdfmark_write_outline(gx_device_pdf * pdev, pdf_outline_node_t * pnode,
		      long next_id)
{
    stream *s;

    pdf_open_separate(pdev, pnode->id);
    s = pdev->strm;
    pputs(s, "<< ");
    cos_dict_elements_write(pnode->action, pdev);
    if (pnode->count)
	pprintd1(s, "/Count %d ", pnode->count);
    pprintld1(s, "/Parent %ld 0 R\n", pnode->parent_id);
    if (pnode->prev_id)
	pprintld1(s, "/Prev %ld 0 R\n", pnode->prev_id);
    if (next_id)
	pprintld1(s, "/Next %ld 0 R\n", next_id);
    if (pnode->first_id)
	pprintld2(s, "/First %ld 0 R /Last %ld 0 R\n",
		  pnode->first_id, pnode->last_id);
    pputs(s, ">>\n");
    pdf_end_separate(pdev);
    COS_FREE(pnode->action, pdev->pdf_memory, "pdfmark_write_outline");
    pnode->action = 0;
    return 0;
}

/* Adjust the parent's count when writing an outline node. */
private void
pdfmark_adjust_parent_count(pdf_outline_level_t * plevel)
{
    pdf_outline_level_t *parent = plevel - 1;
    int count = plevel->last.count;

    if (count > 0) {
	if (parent->last.count < 0)
	    parent->last.count -= count;
	else
	    parent->last.count += count;
    }
}

/* Close the current level of the outline tree. */
int
pdfmark_close_outline(gx_device_pdf * pdev)
{
    int depth = pdev->outline_depth;
    pdf_outline_level_t *plevel = &pdev->outline_levels[depth];
    int code;

    code = pdfmark_write_outline(pdev, &plevel->last, 0);
    if (code < 0)
	return code;
    if (depth > 0) {
	plevel[-1].last.last_id = plevel->last.id;
	pdfmark_adjust_parent_count(plevel);
	--plevel;
	if (plevel->last.count < 0)
	    pdev->closed_outline_depth--;
	pdev->outline_depth--;
    }
    return 0;
}

/* OUT pdfmark */
private int
pdfmark_OUT(gx_device_pdf * pdev, gs_param_string * pairs, uint count,
	    const gs_matrix * pctm, const gs_param_string * no_objname)
{
    int depth = pdev->outline_depth;
    pdf_outline_level_t *plevel = &pdev->outline_levels[depth];
    int sub_count = 0;
    uint i;
    pdf_outline_node_t node;
    ao_params_t ao;
    int code;

    for (i = 0; i < count; i += 2) {
	const gs_param_string *pair = &pairs[i];

	if (pdf_key_eq(pair, "/Count"))
	    pdfmark_scan_int(pair + 1, &sub_count);
    }
    if (sub_count != 0 && depth == MAX_OUTLINE_DEPTH - 1)
	return_error(gs_error_limitcheck);
    node.action = cos_dict_alloc(pdev->pdf_memory, "pdfmark_OUT");
    if (node.action == 0)
	return_error(gs_error_VMerror);
    ao.pdev = pdev;
    ao.subtype = 0;
    ao.pres = 0;
    code = pdfmark_put_ao_pairs(pdev, node.action, pairs, count, pctm, &ao,
				true);
    if (code < 0)
	return code;
    if (pdev->outlines_id == 0)
	pdev->outlines_id = pdf_obj_ref(pdev);
    node.id = pdf_obj_ref(pdev);
    node.parent_id =
	(depth == 0 ? pdev->outlines_id : plevel[-1].last.id);
    node.prev_id = plevel->last.id;
    node.first_id = node.last_id = 0;
    node.count = sub_count;
    /* Add this node to the outline at the current level. */
    if (plevel->first.id == 0) {	/* First node at this level. */
	if (depth > 0)
	    plevel[-1].last.first_id = node.id;
	node.prev_id = 0;
	plevel->first = node;
	plevel->first.action = 0; /* never used */
    } else {			/* Write out the previous node. */
	if (depth > 0)
	    pdfmark_adjust_parent_count(plevel);
	pdfmark_write_outline(pdev, &plevel->last, node.id);
    }
    plevel->last = node;
    plevel->left--;
    if (!pdev->closed_outline_depth)
	pdev->outlines_open++;
    /* If this node has sub-nodes, descend one level. */
    if (sub_count != 0) {
	pdev->outline_depth++;
	++plevel;
	plevel->left = (sub_count > 0 ? sub_count : -sub_count);
	plevel->first.id = 0;
	plevel->first.action = plevel->last.action = 0;	/* for GC */
	if (sub_count < 0)
	    pdev->closed_outline_depth++;
    } else {
	while ((depth = pdev->outline_depth) > 0 &&
	       pdev->outline_levels[depth].left == 0
	    )
	    pdfmark_close_outline(pdev);
    }
    return 0;
}

/* Write an article bead. */
private int
pdfmark_write_bead(gx_device_pdf * pdev, const pdf_bead_t * pbead)
{
    stream *s;
    char rstr[MAX_RECT_STRING];

    pdf_open_separate(pdev, pbead->id);
    s = pdev->strm;
    pprintld3(s, "<</T %ld 0 R/V %ld 0 R/N %ld 0 R",
	      pbead->article_id, pbead->prev_id, pbead->next_id);
    if (pbead->page_id != 0)
	pprintld1(s, "/P %ld 0 R", pbead->page_id);
    pdfmark_make_rect(rstr, &pbead->rect);
    pprints1(s, "/R%s>>\n", rstr);
    return pdf_end_separate(pdev);
}

/* Finish writing an article, and release its data. */
int
pdfmark_write_article(gx_device_pdf * pdev, const pdf_article_t * part)
{
    pdf_article_t art;
    stream *s;

    art = *part;
    if (art.last.id == 0) {
	/* Only one bead in the article. */
	art.first.prev_id = art.first.next_id = art.first.id;
    } else {
	/* More than one bead in the article. */
	art.first.prev_id = art.last.id;
	art.last.next_id = art.first.id;
	pdfmark_write_bead(pdev, &art.last);
    }
    pdfmark_write_bead(pdev, &art.first);
    pdf_open_separate(pdev, art.contents->id);
    s = pdev->strm;
    pprintld1(s, "<</F %ld 0 R/I<<", art.first.id);
    cos_dict_elements_write(art.contents, pdev);
    pputs(s, ">> >>\n");
    return pdf_end_separate(pdev);
}

/* ARTICLE pdfmark */
private int
pdfmark_ARTICLE(gx_device_pdf * pdev, gs_param_string * pairs, uint count,
		const gs_matrix * pctm, const gs_param_string * no_objname)
{
    gs_memory_t *mem = pdev->pdf_memory;
    gs_param_string title;
    gs_param_string rectstr;
    gs_rect rect;
    long bead_id;
    pdf_article_t *part;
    int code;

    if (!pdfmark_find_key("/Title", pairs, count, &title) ||
	!pdfmark_find_key("/Rect", pairs, count, &rectstr)
	)
	return_error(gs_error_rangecheck);
    if ((code = pdfmark_scan_rect(&rect, &rectstr, pctm)) < 0)
	return code;
    bead_id = pdf_obj_ref(pdev);

    /* Find the article with this title, or create one. */
    for (part = pdev->articles; part != 0; part = part->next) {
	const cos_value_t *a_title =
	    cos_dict_find(part->contents, (const byte *)"/Title", 6);

	if (a_title != 0 && !a_title->is_object &&
	    !bytes_compare(a_title->contents.chars.data,
			   a_title->contents.chars.size,
			   title.data, title.size))
	    break;
    }
    if (part == 0) {		/* Create the article. */
	cos_dict_t *contents =
	    cos_dict_alloc(mem, "pdfmark_ARTICLE(contents)");

	if (contents == 0)
	    return_error(gs_error_VMerror);
	part = gs_alloc_struct(mem, pdf_article_t, &st_pdf_article,
			       "pdfmark_ARTICLE(article)");
	if (part == 0 || contents == 0) {
	    gs_free_object(mem, part, "pdfmark_ARTICLE(article)");
	    if (contents)
		COS_FREE(contents, mem, "pdfmark_ARTICLE(contents)");
	    return_error(gs_error_VMerror);
	}
	contents->id = pdf_obj_ref(pdev);
	part->next = pdev->articles;
	pdev->articles = part;
	cos_dict_put_string(contents, pdev, (const byte *)"/Title", 6,
			    title.data, title.size);
	part->first.id = part->last.id = 0;
	part->contents = contents;
    }
    /*
     * Add the bead to the article.  This is similar to what we do for
     * outline nodes, except that articles have only a page number and
     * not View/Dest.
     */
    if (part->last.id == 0) {
	part->first.next_id = bead_id;
	part->last.id = part->first.id;
    } else {
	part->last.next_id = bead_id;
	pdfmark_write_bead(pdev, &part->last);
    }
    part->last.prev_id = part->last.id;
    part->last.id = bead_id;
    part->last.article_id = part->contents->id;
    part->last.next_id = 0;
    part->last.rect = rect;
    {
	gs_param_string page_string;
	int page = 0;
	uint i;

	if (pdfmark_find_key("/Page", pairs, count, &page_string))
	    page = pdfmark_page_number(pdev, &page_string);
	part->last.page_id = pdf_page_id(pdev, page);
	for (i = 0; i < count; i += 2) {
	    if (pdf_key_eq(&pairs[i], "/Rect") || pdf_key_eq(&pairs[i], "/Page"))
		continue;
	    pdfmark_put_pair(pdev, part->contents, &pairs[i]);
	}
    }
    if (part->first.id == 0) {	/* This is the first bead of the article. */
	part->first = part->last;
	part->last.id = 0;
    }
    return 0;
}

/* DEST pdfmark */
private int
pdfmark_DEST(gx_device_pdf * pdev, gs_param_string * pairs, uint count,
	     const gs_matrix * pctm, const gs_param_string * objname)
{
    int present;
    char dest[MAX_DEST_STRING];
    gs_param_string key;
    cos_value_t value;

    if (!pdfmark_find_key("/Dest", pairs, count, &key) ||
	(present =
	 pdfmark_make_dest(dest, pdev, "/Page", "/View", pairs, count)) < 0
	)
	return_error(gs_error_rangecheck);
    cos_c_string_value(&value, dest);
    if (!pdev->Dests) {
	pdev->Dests = cos_dict_alloc(pdev->pdf_memory, "pdfmark_DEST(Dests)");
	if (pdev->Dests == 0)
	    return_error(gs_error_VMerror);
	pdev->Dests->id = pdf_obj_ref(pdev);
    }
    if (objname || count > (present + 1) * 2) {
	/*
	 * Create the destination as a dictionary with a D key, since
	 * it has (or, if named, may have) additional key/value pairs.
	 */
	cos_dict_t *ddict;
	int i, code;

	code = pdf_make_named_dict(pdev, objname, &ddict, false);
	if (code < 0)
	    return code;
	code = cos_dict_put_c_strings(ddict, pdev, "/D", dest);
	for (i = 0; code >= 0 && i < count; i += 2)
	    if (!pdf_key_eq(&pairs[i], "/Dest") &&
		!pdf_key_eq(&pairs[i], "/Page") &&
		!pdf_key_eq(&pairs[i], "/View")
		)
		code = pdfmark_put_pair(pdev, ddict, &pairs[i]);
	if (code < 0)
	    return code;
	COS_OBJECT_VALUE(&value, ddict);
    }
    return cos_dict_put(pdev->Dests, pdev, key.data, key.size, &value);
}

/* Check that pass-through PostScript code is a string. */
private bool
ps_source_ok(const gs_param_string * psource)
{
    if (psource->size >= 2 && psource->data[0] == '(' &&
	psource->data[psource->size - 1] == ')'
	)
	return true;
    else {
	lprintf("bad PS passthrough: ");
	fwrite(psource->data, 1, psource->size, estderr);
	fputs("\n", estderr);
	return false;
    }
}

/* Write the contents of pass-through PostScript code. */
/* Return the size written on the file. */
private uint
pdfmark_write_ps(stream *s, const gs_param_string * psource)
{
    /****** REMOVE ESCAPES WITH PSSDecode, SEE gdevpdfr p. 2 ******/
    uint size = psource->size - 2;

    pwrite(s, psource->data + 1, size);
    pputc(s, '\n');
    return size + 1;
}

/* PS pdfmark */
#define MAX_PS_INLINE 100
private int
pdfmark_PS(gx_device_pdf * pdev, gs_param_string * pairs, uint count,
	   const gs_matrix * pctm, const gs_param_string * objname)
{
    gs_param_string source;
    gs_param_string level1;

    if (!pdfmark_find_key("/DataSource", pairs, count, &source) ||
	!ps_source_ok(&source) ||
	(pdfmark_find_key("/Level1", pairs, count, &level1) &&
	 !ps_source_ok(&level1))
	)
	return_error(gs_error_rangecheck);
    if (level1.data == 0 && source.size <= MAX_PS_INLINE &&
	pdev->CompatibilityLevel >= 1.2 && objname == 0
	) {
	/* Insert the PostScript code in-line */
	int code = pdf_open_contents(pdev, PDF_IN_STREAM);
	stream *s;

	if (code < 0)
	    return code;
	s = pdev->strm;
	pwrite(s, source.data, source.size);
	pputs(s, " PS\n");
    } else {
	/* Put the PostScript code in a resource. */
	pdf_resource_t *pres;
	cos_stream_t *pcs;
	uint size;
	int code;

	code = pdf_make_named(pdev, objname, cos_type_stream,
			      (cos_object_t **)&pcs, true);
	if (code < 0)
	    return code;
	code = pdf_alloc_resource(pdev, resourceXObject, gs_no_id, &pres,
				  pcs->id);
	if (code < 0)
	    return code;
	pres->object = COS_OBJECT(pcs);
	code = cos_stream_put_c_strings(pcs, pdev, "/Type", "/XObject");
	if (code < 0)
	    return code;
	code = cos_stream_put_c_strings(pcs, pdev, "/Subtype", "/PS");
	if (code < 0)
	    return code;
	if (level1.data != 0) {
	    long level1_id = pdf_obj_ref(pdev);
	    char r[10 + 5];	/* %ld 0 R\0 */
	    stream *s;
	    long length_id = pdf_obj_ref(pdev);

	    sprintf(r, "%ld 0 R", level1_id);
	    code = cos_stream_put_c_strings(pcs, pdev, "/Level1", r);
	    if (code < 0)
		return code;
	    pdf_open_separate(pdev, level1_id);
	    s = pdev->strm;
	    pprintld1(s, "<</Length %ld 0 R>>stream\n", length_id);
	    size = pdfmark_write_ps(s, &level1);
	    pputs(s, "endstream\n");
	    pdf_end_separate(pdev);
	    pdf_open_separate(pdev, length_id);
	    pprintld1(s, "%ld\n", (long)size);
	    pdf_end_separate(pdev);
	}
	size = pdfmark_write_ps(pdev->streams.strm, &source);
	code = cos_stream_add(pcs, pdev, size);
	if (code < 0)
	    return code;
	if (objname)
	    pres->named = true;
	else {
	    /* Write the resource now, since it won't be written later. */
	    COS_WRITE_OBJECT(pcs, pdev);
	    COS_RELEASE(pcs, pdev->pdf_memory, "pdfmark_PS");
	}
	code = pdf_open_contents(pdev, PDF_IN_STREAM);
	if (code < 0)
	    return code;
	pprintld1(pdev->strm, "/R%ld Do\n", pcs->id);
    }
    return 0;
}

/* Common code for pdfmarks that do PUT into a specific object. */
private int
pdfmark_put_pairs(gx_device_pdf *pdev, cos_dict_t *pcd,
		  gs_param_string * pairs, uint count)
{
    int code = 0, i;

    if (count & 1)
	return_error(gs_error_rangecheck);
    for (i = 0; code >= 0 && i < count; i += 2)
	code = pdfmark_put_pair(pdev, pcd, pairs + i);
    return code;
}

/* PAGES pdfmark */
private int
pdfmark_PAGES(gx_device_pdf * pdev, gs_param_string * pairs, uint count,
	      const gs_matrix * pctm, const gs_param_string * no_objname)
{
    return pdfmark_put_pairs(pdev, pdev->Pages, pairs, count);
}

/* PAGE pdfmark */
private int
pdfmark_PAGE(gx_device_pdf * pdev, gs_param_string * pairs, uint count,
	     const gs_matrix * pctm, const gs_param_string * no_objname)
{
    return pdfmark_put_pairs(pdev, pdf_current_page_dict(pdev), pairs, count);
}

/* DOCINFO pdfmark */
private int
pdfmark_DOCINFO(gx_device_pdf * pdev, gs_param_string * pairs, uint count,
		const gs_matrix * pctm, const gs_param_string * no_objname)
{
    return pdfmark_put_pairs(pdev, pdev->Info, pairs, count);
}

/* DOCVIEW pdfmark */
private int
pdfmark_DOCVIEW(gx_device_pdf * pdev, gs_param_string * pairs, uint count,
		const gs_matrix * pctm, const gs_param_string * no_objname)
{
    char dest[MAX_DEST_STRING];
    int code = 0;

    if (count & 1)
	return_error(gs_error_rangecheck);
    if (pdfmark_make_dest(dest, pdev, "/Page", "/View", pairs, count)) {
	int i;

	code = cos_dict_put_c_strings(pdev->Catalog, pdev, "/OpenAction", dest);
	for (i = 0; code >= 0 && i < count; i += 2)
	    if (!(pdf_key_eq(&pairs[i], "/Page") ||
		  pdf_key_eq(&pairs[i], "/View"))
		)
		code = pdfmark_put_pair(pdev, pdev->Catalog, pairs + i);
	return code;
    } else
	return pdfmark_put_pairs(pdev, pdev->Catalog, pairs, count);
}

/* ---------------- Named object pdfmarks ---------------- */

/* [ /BBox [llx lly urx ury] /_objdef {obj} /BP pdfmark */
private int
pdfmark_BP(gx_device_pdf * pdev, gs_param_string * pairs, uint count,
	   const gs_matrix * pctm, const gs_param_string * objname)
{
    gs_rect bbox;
    cos_stream_t *pcs;
    pdf_graphics_save_t *pdgs;
    int code;
    double xscale = pdev->HWResolution[0] / 72.0,
	yscale = pdev->HWResolution[1] / 72.0;
    char bbox_str[6 + 4 * 12];

    if (objname == 0 || count != 2 || !pdf_key_eq(&pairs[0], "/BBox"))
	return_error(gs_error_rangecheck);
    if (sscanf((const char *)pairs[1].data, "[%lg %lg %lg %lg]",
	       &bbox.p.x, &bbox.p.y, &bbox.q.x, &bbox.q.y) != 4
	)
	return_error(gs_error_rangecheck);
    code = pdf_make_named(pdev, objname, cos_type_stream,
			  (cos_object_t **)&pcs, true);
    if (code < 0)
	return code;
    gs_bbox_transform(&bbox, pctm, &bbox);
    sprintf(bbox_str, "[%.8g %.8g %.8g %.8g]",
	    bbox.p.x * xscale, bbox.p.y * yscale,
	    bbox.q.x * xscale, bbox.q.y * yscale);
    if ((code = cos_stream_put_c_strings(pcs, pdev, "/Type", "/XObject")) < 0 ||
	(code = cos_stream_put_c_strings(pcs, pdev, "/Subtype", "/Form")) < 0 ||
	(code = cos_stream_put_c_strings(pcs, pdev, "/FormType", "1")) < 0 ||
	(code = cos_stream_put_c_strings(pcs, pdev, "/Matrix", "[1 0 0 1 0 0]")) < 0 ||
	(code = cos_stream_put_c_strings(pcs, pdev, "/BBox", bbox_str)) < 0
	)
	return code;
    pdgs = gs_alloc_struct(pdev->pdf_memory, pdf_graphics_save_t,
			   &st_pdf_graphics_save, "pdfmark_BP");
    if (pdgs == 0)
	return_error(gs_error_VMerror);
    if (pdev->context != PDF_IN_NONE) {
	code = pdf_open_page(pdev, PDF_IN_STREAM);
	if (code < 0) {
	    gs_free_object(pdev->pdf_memory, pdgs, "pdfmark_BP");
	    return code;
	}
    }
    if (!pdev->open_graphics) {
	pdev->pictures.save_strm = pdev->strm;
	pdev->strm = pdev->pictures.strm;
    }
    pdgs->prev = pdev->open_graphics;
    pdgs->object = pcs;
    pdgs->position = stell(pdev->pictures.strm);
    pdgs->save_context = pdev->context;
    pdgs->save_contents_id = pdev->contents_id;
    pdev->open_graphics = pdgs;
    pdev->context = PDF_IN_STREAM;
    pdev->contents_id = pcs->id;
    return 0;
}

/* [ /EP pdfmark */
private int
pdfmark_EP(gx_device_pdf * pdev, gs_param_string * pairs, uint count,
	   const gs_matrix * pctm, const gs_param_string * no_objname)
{
    pdf_graphics_save_t *pdgs = pdev->open_graphics;
    pdf_resource_t *pres;
    cos_stream_t *pcs;
    long start;
    uint size;
    int code;

    if (count != 0 || pdgs == 0 || !(pcs = pdgs->object)->is_open)
	return_error(gs_error_rangecheck);
    code = pdf_open_contents(pdev, PDF_IN_STREAM);
    if (code < 0)
	return code;
    code = pdf_alloc_resource(pdev, resourceXObject, gs_no_id, &pres,
			      pcs->id);
    if (code < 0)
	return code;
    pres->object = COS_OBJECT(pcs);
    start = pdgs->position;
    pcs->is_open = false;
    size = stell(pdev->strm) - start;
    pdev->open_graphics = pdgs->prev;
    pdev->context = pdgs->save_context;
    pdev->contents_id = pdgs->save_contents_id;
    gs_free_object(pdev->pdf_memory, pdgs, "pdfmark_EP");
    /* Copy the data to the streams file. */
    sflush(pdev->strm);
    sseek(pdev->strm, start);
    fseek(pdev->pictures.file, start, SEEK_SET);
    pdf_copy_data(pdev->streams.strm, pdev->pictures.file, size);
    code = cos_stream_add(pcs, pdev, size);
    /* Keep the file in sync with the stream. */
    fseek(pdev->pictures.file, start, SEEK_SET);
    if (!pdev->open_graphics) {
	pdev->strm = pdev->pictures.save_strm;
	pdev->pictures.save_strm = 0;
    }
    return code;
}

/* [ {obj} /SP pdfmark */
private int
pdfmark_SP(gx_device_pdf * pdev, gs_param_string * pairs, uint count,
	   const gs_matrix * pctm, const gs_param_string * no_objname)
{
    cos_object_t *pco;		/* stream */
    gs_matrix ctm;
    int code;

    if (count != 1)
	return_error(gs_error_rangecheck);
    if ((code = pdf_get_named(pdev, &pairs[0], cos_type_stream, &pco)) < 0)
	return code;
    /****** HOW TO CHECK FOR GRAPHICS STREAM? ******/
    if (pco->is_open)
	return_error(gs_error_rangecheck);
    code = pdf_open_contents(pdev, PDF_IN_STREAM);
    if (code < 0)
	return code;
    ctm = *pctm;
    ctm.tx *= pdev->HWResolution[0] / 72.0;
    ctm.ty *= pdev->HWResolution[1] / 72.0;
    pdf_put_matrix(pdev, "q ", &ctm, "cm\n");
    pprintld1(pdev->strm, "/R%ld Do Q\n", pco->id);
    return 0;
}

/* [ /_objdef {array} /type /array /OBJ pdfmark */
/* [ /_objdef {dict} /type /dict /OBJ pdfmark */
/* [ /_objdef {stream} /type /stream /OBJ pdfmark */
private int
pdfmark_OBJ(gx_device_pdf * pdev, gs_param_string * pairs, uint count,
	    const gs_matrix * pctm, const gs_param_string * objname)
{
    cos_type_t cotype;
    cos_object_t *pco;
    int code;

    if (objname == 0 || count != 2 || !pdf_key_eq(&pairs[0], "/type"))
	return_error(gs_error_rangecheck);
    if (pdf_key_eq(&pairs[1], "/array"))
	cotype = cos_type_array;
    else if (pdf_key_eq(&pairs[1], "/dict"))
	cotype = cos_type_dict;
    else if (pdf_key_eq(&pairs[1], "/stream"))
	cotype = cos_type_stream;
    else
	return_error(gs_error_rangecheck);
    if ((code = pdf_make_named(pdev, objname, cotype, &pco, true)) < 0) {
	/*
	 * For Distiller compatibility, allows multiple /OBJ pdfmarks with
	 * the same name and type, even though the pdfmark specification
	 * doesn't say anything about this being legal.
	 */
	if (code == gs_error_rangecheck &&
	    pdf_refer_named(pdev, objname, &pco) >= 0 &&
	    cos_type(pco) == cotype
	    )
	    return 0;		/* already exists, but OK */
	return code;
    }
    return 0;
}

/* [ {array} index value /PUT pdfmark */
/* Dictionaries are converted to .PUTDICT */
/* Streams are converted to .PUTSTREAM */
private int
pdfmark_PUT(gx_device_pdf * pdev, gs_param_string * pairs, uint count,
	    const gs_matrix * pctm, const gs_param_string * no_objname)
{
    cos_object_t *pco;
    cos_value_t value;
    int code, index;

    if (count != 3)
	return_error(gs_error_rangecheck);
    if ((code = pdf_get_named(pdev, &pairs[0], cos_type_array, &pco)) < 0)
	return code;
    if ((code = pdfmark_scan_int(&pairs[1], &index)) < 0)
	return code;
    if (index < 0)
	return_error(gs_error_rangecheck);
    return cos_array_put((cos_array_t *)pco, pdev, index,
		cos_string_value(&value, pairs[2].data, pairs[2].size));
}

/* [ {dict} key value ... /.PUTDICT pdfmark */
/* [ {stream} key value ... /.PUTDICT pdfmark */
/*
 * Adobe's pdfmark documentation doesn't allow PUTDICT with a stream,
 * but it's reasonable and unambiguous, and Acrobat Distiller accepts it,
 * so we do too.
 */
private int
pdfmark_PUTDICT(gx_device_pdf * pdev, gs_param_string * pairs, uint count,
		const gs_matrix * pctm, const gs_param_string * no_objname)
{
    cos_object_t *pco;
    int code;

    if ((code = pdf_refer_named(pdev, &pairs[0], &pco)) < 0)
	return code;
    if (cos_type(pco) != cos_type_dict && cos_type(pco) != cos_type_stream)
	return_error(gs_error_typecheck);
    return pdfmark_put_pairs(pdev, (cos_dict_t *)pco, pairs + 1, count - 1);
}

/* [ {stream} string ... /.PUTSTREAM pdfmark */
private int
pdfmark_PUTSTREAM(gx_device_pdf * pdev, gs_param_string * pairs, uint count,
		  const gs_matrix * pctm, const gs_param_string * no_objname)
{
    cos_object_t *pco;
    int code, i;

    if (count < 2)
	return_error(gs_error_rangecheck);
    if ((code = pdf_get_named(pdev, &pairs[0], cos_type_stream, &pco)) < 0)
	return code;
    if (!pco->is_open)
	return_error(gs_error_rangecheck);
    for (i = 1; code >= 0 && i < count; ++i)
	code = cos_stream_add_bytes((cos_stream_t *)pco, pdev, pairs[i].data,
				    pairs[i].size);
    return code;
}

/* [ {array} index value ... /.PUTINTERVAL pdfmark */
private int
pdfmark_PUTINTERVAL(gx_device_pdf * pdev, gs_param_string * pairs, uint count,
		 const gs_matrix * pctm, const gs_param_string * no_objname)
{
    cos_object_t *pco;
    cos_value_t value;
    int code, index, i;

    if (count < 2)
	return_error(gs_error_rangecheck);
    if ((code = pdf_get_named(pdev, &pairs[0], cos_type_array, &pco)) < 0)
	return code;
    if ((code = pdfmark_scan_int(&pairs[1], &index)) < 0)
	return code;
    if (index < 0)
	return_error(gs_error_rangecheck);
    for (i = 2; code >= 0 && i < count; ++i)
	code = cos_array_put((cos_array_t *)pco, pdev, index + i - 2,
		cos_string_value(&value, pairs[i].data, pairs[i].size));
    return code;
}

/* [ {stream} /CLOSE pdfmark */
private int
pdfmark_CLOSE(gx_device_pdf * pdev, gs_param_string * pairs, uint count,
	      const gs_matrix * pctm, const gs_param_string * no_objname)
{
    cos_object_t *pco;
    int code;

    if (count != 1)
	return_error(gs_error_rangecheck);
    if ((code = pdf_get_named(pdev, &pairs[0], cos_type_stream, &pco)) < 0)
	return code;
    if (!pco->is_open)
	return_error(gs_error_rangecheck);
    /* Currently we don't do anything special when closing a stream. */
    pco->is_open = false;
    return 0;
}

/* [ /NamespacePush pdfmark */
private int
pdfmark_NamespacePush(gx_device_pdf *pdev, gs_param_string *pairs, uint count,
		      const gs_matrix *pctm, const gs_param_string *objname)
{
    return 0;
}

/* [ /NamespacePop pdfmark */
private int
pdfmark_NamespacePop(gx_device_pdf *pdev, gs_param_string *pairs, uint count,
		     const gs_matrix *pctm, const gs_param_string *objname)
{
    return 0;
}

/* [ /_objdef {image} /NI pdfmark */
private int
pdfmark_NI(gx_device_pdf *pdev, gs_param_string *pairs, uint count,
	   const gs_matrix *pctm, const gs_param_string *objname)
{
    return 0;
}

/* ---------------- Document structure pdfmarks ---------------- */

/* [ newsubtype1 stdsubtype1 ... /StRoleMap pdfmark */
private int
pdfmark_StRoleMap(gx_device_pdf *pdev, gs_param_string *pairs, uint count,
		  const gs_matrix *pctm, const gs_param_string *objname)
{
    return 0;
}

/* [ class1 {attrobj1} ... /StClassMap pdfmark */
private int
pdfmark_StClassMap(gx_device_pdf *pdev, gs_param_string *pairs, uint count,
		   const gs_matrix *pctm, const gs_param_string *objname)
{
    return 0;
}

/*
 * [ [/_objdef {objname}] /Subtype name [/Title string] [/Alt string]
 *   [/ID string] [/Class name] [/At index] [/Bookmark dict] [action_pairs...]
 *   /StPNE pdfmark
 */
private int
pdfmark_StPNE(gx_device_pdf *pdev, gs_param_string *pairs, uint count,
	      const gs_matrix *pctm, const gs_param_string *objname)
{
    return 0;
}

/* [ [/Title string] [/Open bool] [action_pairs...] /StBookmarkRoot pdfmark */
private int
pdfmark_StBookmarkRoot(gx_device_pdf *pdev, gs_param_string *pairs, uint count,
		       const gs_matrix *pctm, const gs_param_string *objname)
{
    return 0;
}

/* [ [/E {elt}] /StPush pdfmark */
private int
pdfmark_StPush(gx_device_pdf *pdev, gs_param_string *pairs, uint count,
	       const gs_matrix *pctm, const gs_param_string *objname)
{
     return 0;
}

/* [ /StPop pdfmark */
private int
pdfmark_StPop(gx_device_pdf *pdev, gs_param_string *pairs, uint count,
	      const gs_matrix *pctm, const gs_param_string *objname)
{
    return 0;
}

/* [ /StPopAll pdfmark */
private int
pdfmark_StPopAll(gx_device_pdf *pdev, gs_param_string *pairs, uint count,
		 const gs_matrix *pctm, const gs_param_string *objname)
{
    return 0;
}

/* [ [/T tagname] [/At index] /StBMC pdfmark */
private int
pdfmark_StBMC(gx_device_pdf *pdev, gs_param_string *pairs, uint count,
	      const gs_matrix *pctm, const gs_param_string *objname)
{
    return 0;
}

/* [ [/P propdict] [/T tagname] [/At index] /StBDC pdfmark */
private int
pdfmark_StBDC(gx_device_pdf *pdev, gs_param_string *pairs, uint count,
	      const gs_matrix *pctm, const gs_param_string *objname)
{
    return 0;
}

/* [ /EMC pdfmark */
private int
pdfmark_EMC(gx_device_pdf *pdev, gs_param_string *pairs, uint count,
	    const gs_matrix *pctm, const gs_param_string *objname)
{
    return 0;
}

/* [ /Obj {obj} [/At index] /StOBJ pdfmark */
private int
pdfmark_StOBJ(gx_device_pdf *pdev, gs_param_string *pairs, uint count,
	      const gs_matrix *pctm, const gs_param_string *objname)
{
    return 0;
}

/* [ /Obj {obj} /StAttr pdfmark */
private int
pdfmark_StAttr(gx_device_pdf *pdev, gs_param_string *pairs, uint count,
	       const gs_matrix *pctm, const gs_param_string *objname)
{
    return 0;
}

/* [ /StoreName name /StStore pdfmark */
private int
pdfmark_StStore(gx_device_pdf *pdev, gs_param_string *pairs, uint count,
		const gs_matrix *pctm, const gs_param_string *objname)
{
    return 0;
}

/* [ /StoreName name /StRetrieve pdfmark */
private int
pdfmark_StRetrieve(gx_device_pdf *pdev, gs_param_string *pairs, uint count,
		   const gs_matrix *pctm, const gs_param_string *objname)
{
    return 0;
}

/* ---------------- Dispatch ---------------- */

/*
 * Define the pdfmark types we know about.
 */
private const pdfmark_name mark_names[] =
{
	/* Miscellaneous. */
    {"ANN",          pdfmark_ANN,         PDFMARK_NAMEABLE},
    {"LNK",          pdfmark_LNK,         PDFMARK_NAMEABLE},
    {"OUT",          pdfmark_OUT,         0},
    {"ARTICLE",      pdfmark_ARTICLE,     0},
    {"DEST",         pdfmark_DEST,        PDFMARK_NAMEABLE},
    {"PS",           pdfmark_PS,          PDFMARK_NAMEABLE},
    {"PAGES",        pdfmark_PAGES,       0},
    {"PAGE",         pdfmark_PAGE,        0},
    {"DOCINFO",      pdfmark_DOCINFO,     0},
    {"DOCVIEW",      pdfmark_DOCVIEW,     0},
	/* Named objects. */
    {"BP",           pdfmark_BP,          PDFMARK_NAMEABLE},
    {"EP",           pdfmark_EP,          0},
    {"SP",           pdfmark_SP,          PDFMARK_ODD_OK | PDFMARK_KEEP_NAME},
    {"OBJ",          pdfmark_OBJ,         PDFMARK_NAMEABLE},
    {"PUT",          pdfmark_PUT,         PDFMARK_ODD_OK | PDFMARK_KEEP_NAME},
    {".PUTDICT",     pdfmark_PUTDICT,     PDFMARK_ODD_OK | PDFMARK_KEEP_NAME},
    {".PUTINTERVAL", pdfmark_PUTINTERVAL, PDFMARK_ODD_OK | PDFMARK_KEEP_NAME},
    {".PUTSTREAM",   pdfmark_PUTSTREAM,   PDFMARK_ODD_OK | PDFMARK_KEEP_NAME |
                                          PDFMARK_NO_REFS},
    {"CLOSE",        pdfmark_CLOSE,       PDFMARK_ODD_OK | PDFMARK_KEEP_NAME},
    {"NamespacePush", pdfmark_NamespacePush, 0},
    {"NamespacePop", pdfmark_NamespacePop, 0},
    {"NI",           pdfmark_NI,          PDFMARK_NAMEABLE},
	/* Document structure. */
    {"StRoleMap",    pdfmark_StRoleMap,   0},
    {"StClassMap",   pdfmark_StClassMap,  0},
    {"StPNE",        pdfmark_StPNE,       PDFMARK_NAMEABLE},
    {"StBookmarkRoot", pdfmark_StBookmarkRoot, 0},
    {"StPush",       pdfmark_StPush,       0},
    {"StPop",        pdfmark_StPop,        0},
    {"StPopAll",     pdfmark_StPopAll,     0},
    {"StBMC",        pdfmark_StBMC,        0},
    {"StBDC",        pdfmark_StBDC,        0},
    {"EMC",          pdfmark_EMC,          0},
    {"StOBJ",        pdfmark_StOBJ,        0},
    {"StAttr",       pdfmark_StAttr,       0},
    {"StStore",      pdfmark_StStore,      0},
    {"StRetrieve",   pdfmark_StRetrieve,   0},
	/* End of list. */
    {0, 0}
};

/* Process a pdfmark. */
int
pdfmark_process(gx_device_pdf * pdev, const gs_param_string_array * pma)
{
    const gs_param_string *data = pma->data;
    uint size = pma->size;
    const gs_param_string *pts = &data[size - 1];
    const gs_param_string *objname = 0;
    gs_matrix ctm;
    const pdfmark_name *pmn;
    int code = 0;

    if (size < 2 ||
	sscanf((const char *)pts[-1].data, "[%g %g %g %g %g %g]",
	       &ctm.xx, &ctm.xy, &ctm.yx, &ctm.yy, &ctm.tx, &ctm.ty) != 6
	)
	return_error(gs_error_rangecheck);
    /*
     * Our coordinate system is scaled so that user space is always
     * default user space.  Adjust the CTM to match this.
     */
    {
	double xscale = 72.0 / pdev->HWResolution[0],
	    yscale = 72.0 / pdev->HWResolution[1];

	ctm.xx *= xscale, ctm.xy *= yscale;
	ctm.yx *= xscale, ctm.yy *= yscale;
	ctm.tx *= xscale, ctm.ty *= yscale;
    }
    size -= 2;			/* remove CTM & pdfmark name */
    for (pmn = mark_names; pmn->mname != 0; ++pmn)
	if (pdf_key_eq(pts, pmn->mname)) {
	    gs_memory_t *mem = pdev->pdf_memory;
	    int odd_ok = (pmn->options & PDFMARK_ODD_OK) != 0;
	    gs_param_string *pairs;
	    int j;

	    if (size & !odd_ok)
		return_error(gs_error_rangecheck);
	    if (pmn->options & PDFMARK_NAMEABLE) {
		/* Look for an object name. */
		for (j = 0; j < size; j += 2) {
		    if (pdf_key_eq(&data[j], "/_objdef")) {
			objname = &data[j + 1];
			if (!pdf_objname_is_valid(objname->data,
						  objname->size)
			    )
			    return_error(gs_error_rangecheck);
			/* Save the pairs without the name. */
			size -= 2;
			pairs = (gs_param_string *)
			    gs_alloc_byte_array(mem, size,
						sizeof(gs_param_string),
						"pdfmark_process(pairs)");
			if (!pairs)
			    return_error(gs_error_VMerror);
			memcpy(pairs, data, j * sizeof(*data));
			memcpy(pairs + j, data + j + 2,
			       (size - j) * sizeof(*data));
			goto copied;
		    }
		}
	    }
	    /* Save all the pairs. */
	    pairs = (gs_param_string *)
		gs_alloc_byte_array(mem, size, sizeof(gs_param_string),
				    "pdfmark_process(pairs)");
	    if (!pairs)
		return_error(gs_error_VMerror);
	    memcpy(pairs, data, size * sizeof(*data));
	copied:		/* Substitute object references for names. */
	    if (!(pmn->options & PDFMARK_NO_REFS)) {
		for (j = (pmn->options & PDFMARK_KEEP_NAME ? 1 : 1 - odd_ok);
		     j < size; j += 2 - odd_ok
		     ) {
		    code = pdf_replace_names(pdev, &pairs[j], &pairs[j]);
		    if (code < 0) {
			gs_free_object(mem, pairs, "pdfmark_process(pairs)");
			return code;
		    }
		}
	    }
	    code = (*pmn->proc) (pdev, pairs, size, &ctm, objname);
	    gs_free_object(mem, pairs, "pdfmark_process(pairs)");
	    break;
	}
    return code;
}
